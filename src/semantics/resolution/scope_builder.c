#include "cnlang/frontend/semantics.h"
#include "cnlang/frontend/module_loader.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/preprocessor.h"
#include "cnlang/support/diagnostics.h"
#include "cnlang/ir/ir.h"  // CnIrModule 类型定义

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

// MSVC 兼容性：较旧版本的 MSVC (< VS2015) 中 snprintf 未定义
#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

// ============================================================================
// 路径规范化函数（用于模块去重）
// ============================================================================

/**
 * @brief 规范化文件路径，转换为绝对路径并统一路径分隔符
 *
 * 解决模块重复加载问题：
 * - 同一模块可能通过不同路径访问（相对路径、绝对路径、不同分隔符）
 * - 规范化后确保相同文件返回相同路径，便于缓存查找
 *
 * @param path 输入路径（可以是相对或绝对路径）
 * @return 规范化后的绝对路径（调用者负责释放），失败返回 NULL
 */
static char *normalize_file_path(const char *path) {
    if (!path) {
        return NULL;
    }
    
#ifdef _WIN32
    // Windows: 使用 GetFullPathNameW 获取绝对路径
    // 先转换为宽字符
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    if (wlen == 0) {
        return strdup(path);  // 转换失败，返回原路径副本
    }
    
    WCHAR *wpath = (WCHAR *)malloc(wlen * sizeof(WCHAR));
    if (!wpath) {
        return strdup(path);
    }
    
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, wlen);
    
    // 获取绝对路径
    DWORD abs_len = GetFullPathNameW(wpath, 0, NULL, NULL);
    if (abs_len == 0) {
        free(wpath);
        return strdup(path);
    }
    
    WCHAR *abs_wpath = (WCHAR *)malloc(abs_len * sizeof(WCHAR));
    if (!abs_wpath) {
        free(wpath);
        return strdup(path);
    }
    
    GetFullPathNameW(wpath, abs_len, abs_wpath, NULL);
    free(wpath);
    
    // 转换回 UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, abs_wpath, -1, NULL, 0, NULL, NULL);
    if (utf8_len == 0) {
        free(abs_wpath);
        return strdup(path);
    }
    
    char *result = (char *)malloc(utf8_len);
    if (!result) {
        free(abs_wpath);
        return strdup(path);
    }
    
    WideCharToMultiByte(CP_UTF8, 0, abs_wpath, -1, result, utf8_len, NULL, NULL);
    free(abs_wpath);
    
    // 统一使用反斜杠作为路径分隔符（Windows标准）
    for (char *p = result; *p; p++) {
        if (*p == '/') {
            *p = '\\';
        }
    }
    
    return result;
#else
    // POSIX: 使用 realpath 获取规范化的绝对路径
    char *resolved = realpath(path, NULL);
    if (resolved) {
        return resolved;
    }
    
    // realpath 失败（文件可能不存在），尝试手动规范化
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return strdup(path);
    }
    
    // 如果是相对路径，拼接当前目录
    if (path[0] != '/') {
        size_t cwd_len = strlen(cwd);
        size_t path_len = strlen(path);
        char *full_path = (char *)malloc(cwd_len + 1 + path_len + 1);
        if (!full_path) {
            return strdup(path);
        }
        
        sprintf(full_path, "%s/%s", cwd, path);
        
        // 简单规范化：移除 ./ 和 ../
        // 这里只做简单处理，完整实现需要更复杂的逻辑
        char *result = (char *)malloc(strlen(full_path) + 1);
        if (!result) {
            free(full_path);
            return strdup(path);
        }
        
        char *dst = result;
        char *src = full_path;
        while (*src) {
            if (src[0] == '.' && src[1] == '/') {
                src += 2;  // 跳过 ./
            } else if (src[0] == '/' && src[1] == '.' && src[2] == '/') {
                src += 2;  // 跳过 /./
            } else {
                *dst++ = *src++;
            }
        }
        *dst = '\0';
        
        free(full_path);
        return result;
    }
    
    return strdup(path);
#endif
}

// 辅助函数：检查两个符号是否是同一个符号（来自同一模块的同一声明）
//
// 判断逻辑：
// 1. 符号种类和名字必须相同
// 2. 如果类型指针相同，则认为是同一个符号（最可靠的判断）
// 3. 如果 decl_scope 相同，则认为是同一个符号
// 4. 对于函数类型，如果参数数量相同，则认为是同一个符号
//
// 这个逻辑确保：当模块A导入模块B和模块C，而模块B也导入模块C时，
// 模块C的符号不会被重复注册到模块A的全局作用域
static int cn_sem_is_same_symbol(const CnSemSymbol *sym1, const CnSemSymbol *sym2) {
    if (!sym1 || !sym2) {
        return 0;
    }
    
    // 检查符号种类是否相同
    if (sym1->kind != sym2->kind) {
        return 0;
    }
    
    // 检查名字是否相同
    if (sym1->name_length != sym2->name_length) {
        return 0;
    }
    if (memcmp(sym1->name, sym2->name, sym1->name_length) != 0) {
        return 0;
    }
    
    // 调试输出
    char name_buf[64];
    size_t copy_len = sym1->name_length < 63 ? sym1->name_length : 63;
    memcpy(name_buf, sym1->name, copy_len);
    name_buf[copy_len] = '\0';
    
    // 【关键修复】首先检查源模块路径是否相同
    // 这是解决跨编译会话符号唯一性问题的关键
    // 当编译脚本逐个文件独立编译时，不同编译会话中同一模块的符号具有不同的指针值
    // 但如果源模块路径相同且符号名相同，则认为是同一个符号
    if (sym1->source_module_path && sym2->source_module_path &&
        sym1->source_module_path_length == sym2->source_module_path_length &&
        memcmp(sym1->source_module_path, sym2->source_module_path, sym1->source_module_path_length) == 0) {
        fprintf(stderr, "[DEBUG] cn_sem_is_same_symbol: name=%s, kind=%d, source_module_path match (path=%.*s)\n",
                name_buf, sym1->kind, (int)sym1->source_module_path_length, sym1->source_module_path);
        return 1;  // 来自同一个模块且名称相同，是同一个符号
    }
    
    // 【关键修复】检查类型指针是否相同
    // 当符号被复制时，type 指针也被复制了，所以类型指针相同意味着来自同一声明
    // 这是最可靠的判断方式，因为类型对象是唯一的
    if (sym1->type && sym2->type && sym1->type == sym2->type) {
        fprintf(stderr, "[DEBUG] cn_sem_is_same_symbol: name=%s, kind=%d, type match (type=%p)\n",
                name_buf, sym1->kind, (void*)sym1->type);
        return 1;  // 类型指针相同，肯定是同一个符号
    }
    
    // 检查是否来自同一个声明作用域
    // 【注意】仅当两个符号都有有效的类型信息时才使用 decl_scope 判断
    // 否则可能导致类型信息丢失
    if (sym1->decl_scope && sym2->decl_scope && sym1->decl_scope == sym2->decl_scope) {
        // 如果其中一个符号缺少类型信息，不能认为是同一个符号
        // 需要让调用者处理类型信息补充
        if ((sym1->type && !sym2->type) || (!sym1->type && sym2->type)) {
            fprintf(stderr, "[DEBUG] cn_sem_is_same_symbol: name=%s, kind=%d, decl_scope match but type mismatch (type1=%p, type2=%p), returning 0\n",
                    name_buf, sym1->kind, (void*)sym1->type, (void*)sym2->type);
            return 0;  // 类型信息不一致，需要处理
        }
        fprintf(stderr, "[DEBUG] cn_sem_is_same_symbol: name=%s, kind=%d, decl_scope match (scope=%p)\n",
                name_buf, sym1->kind, (void*)sym1->decl_scope);
        return 1;  // 声明作用域相同，是同一个符号
    }
    
    // 对于函数类型，如果类型指针不同但函数签名相同，也认为是同一个符号
    // 这种情况发生在：同一个函数被多次声明（原型声明和完整声明）
    if (sym1->type && sym2->type &&
        sym1->type->kind == CN_TYPE_FUNCTION && sym2->type->kind == CN_TYPE_FUNCTION) {
        // 比较函数类型的结构：参数数量
        if (sym1->type->as.function.param_count == sym2->type->as.function.param_count) {
            fprintf(stderr, "[DEBUG] cn_sem_is_same_symbol: name=%s, kind=%d, function param count match (%zu)\n",
                    name_buf, sym1->kind, sym1->type->as.function.param_count);
            return 1;  // 同名函数且参数数量相同，认为是同一个函数
        }
    }
    
    // 对于结构体和枚举类型，检查 module_scope 是否相同
    // module_scope 存储了结构体/枚举的成员作用域
    if ((sym1->kind == CN_SEM_SYMBOL_STRUCT || sym1->kind == CN_SEM_SYMBOL_ENUM) &&
        (sym2->kind == CN_SEM_SYMBOL_STRUCT || sym2->kind == CN_SEM_SYMBOL_ENUM)) {
        if (sym1->as.module_scope && sym2->as.module_scope &&
            sym1->as.module_scope == sym2->as.module_scope) {
            fprintf(stderr, "[DEBUG] cn_sem_is_same_symbol: name=%s, kind=%d, module_scope match (scope=%p)\n",
                    name_buf, sym1->kind, (void*)sym1->as.module_scope);
            return 1;  // 成员作用域相同，是同一个类型定义
        }
    }
    
    // 【关键修复】对于模块符号，使用 module_scope 进行比较
    // 模块符号的 type 字段为 NULL，但 module_scope 存储了模块的作用域
    // 如果两个模块符号的 module_scope 相同，则它们是同一个模块
    if (sym1->kind == CN_SEM_SYMBOL_MODULE && sym2->kind == CN_SEM_SYMBOL_MODULE) {
        if (sym1->as.module_scope && sym2->as.module_scope &&
            sym1->as.module_scope == sym2->as.module_scope) {
            fprintf(stderr, "[DEBUG] cn_sem_is_same_symbol: name=%s, kind=%d, module symbol module_scope match (scope=%p)\n",
                    name_buf, sym1->kind, (void*)sym1->as.module_scope);
            return 1;  // 模块作用域相同，是同一个模块
        }
    }
    
    // 默认情况：同名同种类的符号，但无法确定是否来自同一声明
    // 返回 0 表示不是同一个符号，让调用者决定如何处理
    fprintf(stderr, "[DEBUG] cn_sem_is_same_symbol: name=%s, kind=%d, NO MATCH (type1=%p, type2=%p, scope1=%p, scope2=%p)\n",
            name_buf, sym1->kind, (void*)sym1->type, (void*)sym2->type,
            (void*)sym1->decl_scope, (void*)sym2->decl_scope);
    return 0;
}

/**
 * @brief 复制符号的类型信息（处理枚举类型为 NULL 的特殊情况）
 *
 * 当模块导入时，枚举符号的 type 字段可能为 NULL（模块缓存时类型信息丢失）。
 * 此函数检测这种情况并创建新的枚举类型，同时复制枚举作用域。
 *
 * @param src_sym 源符号
 * @param dst_sym 目标符号（已创建，需要设置 type 字段）
 */
static void cn_sem_copy_symbol_type(CnSemSymbol *src_sym, CnSemSymbol *dst_sym) {
    if (!src_sym || !dst_sym) {
        return;
    }
    
    // 【关键修复】处理枚举符号类型为 NULL 的情况
    if (src_sym->kind == CN_SEM_SYMBOL_ENUM && !src_sym->type) {
        // 枚举符号的 type 字段为 NULL，需要创建新的枚举类型
        fprintf(stderr, "[DEBUG] cn_sem_copy_symbol_type: enum '%.*s' has NULL type, creating new enum type.\n",
                (int)src_sym->name_length, src_sym->name);
        // 创建新的枚举类型
        dst_sym->type = cn_type_new_enum(src_sym->name, src_sym->name_length);
        // 复制枚举作用域（需要检查 dst_sym->type 是否创建成功）
        if (dst_sym->type && src_sym->as.module_scope) {
            dst_sym->type->as.enum_type.enum_scope = cn_sem_scope_deep_copy_enum(src_sym->as.module_scope);
        }
    }
    // 【新增】处理结构体符号类型为 NULL 的情况
    else if (src_sym->kind == CN_SEM_SYMBOL_STRUCT && !src_sym->type) {
        fprintf(stderr, "[DEBUG] cn_sem_copy_symbol_type: struct '%.*s' has NULL type, creating new struct type.\n",
                (int)src_sym->name_length, src_sym->name);
        // 创建一个不完整的结构体类型（字段信息可能缺失）
        dst_sym->type = cn_type_new_struct(src_sym->name, src_sym->name_length, NULL, 0, NULL, NULL, 0);
    } else {
        // 正常情况：深度复制类型
        dst_sym->type = cn_type_deep_copy(src_sym->type);
    }
}

// 符号链表节点，用于在作用域中维护符号列表
// （与 symbol_table.c 中的定义保持一致）
typedef struct CnSemSymbolNode {
    CnSemSymbol symbol;
    struct CnSemSymbolNode *next;
} CnSemSymbolNode;

// 作用域结构体的内部实现
// （与 symbol_table.c 中的定义保持一致）
struct CnSemScope {
    CnSemScopeKind kind;
    CnSemScope *parent;
    CnSemSymbolNode *symbols;
    const char *name;
    size_t name_length;
    CnFileModuleSemInfo *file_module_info;  // 文件模块信息（仅当kind==CN_SEM_SCOPE_FILE_MODULE时有效）
};

// 模块编译栈，用于检测循环导入
#define MAX_MODULE_COMPILE_DEPTH 64
static const char *g_compiling_modules[MAX_MODULE_COMPILE_DEPTH];
static int g_compile_depth = 0;

// 模块缓存结构
#define MAX_CACHED_MODULES 256
typedef struct {
    char *file_path;
    CnSemScope *scope;
    CnAstProgram *program;    // AST程序（用于代码生成）
    CnIrModule *ir_module;    // IR模块（用于代码生成）
} CachedModule;

static CachedModule g_module_cache[MAX_CACHED_MODULES];
static int g_cached_module_count = 0;

// 查找缓存的模块（使用规范化路径）
static CnSemScope *find_cached_module(const char *file_path) {
    // 规范化路径以确保相同文件的不同路径表示能匹配
    char *normalized = normalize_file_path(file_path);
    const char *search_path = normalized ? normalized : file_path;
    
    for (int i = 0; i < g_cached_module_count; i++) {
        if (strcmp(g_module_cache[i].file_path, search_path) == 0) {
            if (normalized) free(normalized);
            return g_module_cache[i].scope;
        }
    }
    
    if (normalized) free(normalized);
    return NULL;
}

// 缓存模块（带AST，使用规范化路径）
// 返回值：1 表示成功缓存，0 表示失败（缓存已满或已存在）
static int cache_module_with_program(const char *file_path, CnSemScope *scope, CnAstProgram *program) {
    if (g_cached_module_count >= MAX_CACHED_MODULES) {
        fprintf(stderr, "[WARNING] 模块缓存已满，无法缓存: %s\n", file_path);
        return 0;  // 缓存已满
    }
    
    // 先检查是否已缓存（避免重复缓存）
    CnSemScope *existing = find_cached_module(file_path);
    if (existing) {
        return 0;  // 已缓存，不重复添加
    }
    
    // 【关键修复】在缓存前，确保所有符号的类型信息被深度复制
    // 这样即使原模块被重新编译，缓存的符号仍然有有效的类型信息
    // 解决问题：模块导入时类型信息丢失（sym->type 变为 NULL 或悬空指针）
    if (scope && scope->symbols) {
        CnSemSymbolNode *node = scope->symbols;
        while (node) {
            CnSemSymbol *sym = &node->symbol;
            
            // 深度复制类型信息，确保缓存中的符号有独立的类型对象
            // 【重要】只有当深度复制成功时才更新类型，否则保留原始类型
            if (sym->type) {
                CnType *copied_type = cn_type_deep_copy(sym->type);
                if (copied_type) {
                    sym->type = copied_type;
                } else {
                    // 深度复制失败，记录警告但保留原始类型指针
                    // 注意：这可能导致悬空指针问题，但比 NULL 类型更好调试
                    fprintf(stderr, "[WARNING] cache_module_with_program: cn_type_deep_copy failed for symbol '%.*s' (kind=%d), keeping original type=%p\n",
                            (int)sym->name_length, sym->name, sym->kind, (void*)sym->type);
                }
            } else {
                // 【调试】符号类型为 NULL，记录警告
                fprintf(stderr, "[DEBUG] cache_module_with_program: symbol '%.*s' (kind=%d) has NULL type before caching\n",
                        (int)sym->name_length, sym->name, sym->kind);
            }
            
            // 对于枚举类型，还需要确保 enum_scope 被正确复制
            // 枚举符号的 as.module_scope 存储枚举成员作用域
            if (sym->kind == CN_SEM_SYMBOL_ENUM && sym->as.module_scope) {
                CnSemScope *copied_scope = cn_sem_scope_deep_copy_enum(sym->as.module_scope);
                if (copied_scope) {
                    sym->as.module_scope = copied_scope;
                }
            }
            
            // 对于结构体类型，也需要复制成员作用域
            if (sym->kind == CN_SEM_SYMBOL_STRUCT && sym->as.module_scope) {
                // 结构体的 module_scope 存储字段作用域
                // 注意：结构体作用域的深度复制需要保留字段信息
                // 这里暂时只复制指针，因为 cn_type_deep_copy 已经处理了结构体字段
            }
            
            node = node->next;
        }
    }
    
    // 使用规范化路径存储
    char *normalized = normalize_file_path(file_path);
    g_module_cache[g_cached_module_count].file_path = normalized ? normalized : strdup(file_path);
    g_module_cache[g_cached_module_count].scope = scope;
    g_module_cache[g_cached_module_count].program = program;  // 缓存AST
    g_module_cache[g_cached_module_count].ir_module = NULL;   // IR稍后填充
    g_cached_module_count++;
    return 1;  // 缓存成功
}

// 缓存模块（兼容旧接口）
static void cache_module(const char *file_path, CnSemScope *scope) {
    cache_module_with_program(file_path, scope, NULL);
}

// 获取缓存的模块数量
int cn_sem_get_cached_module_count(void) {
    return g_cached_module_count;
}

// 获取缓存的模块文件路径
const char *cn_sem_get_cached_module_path(int index) {
    if (index < 0 || index >= g_cached_module_count) {
        return NULL;
    }
    return g_module_cache[index].file_path;
}

// 获取缓存的模块AST程序
CnAstProgram *cn_sem_get_cached_module_program(int index) {
    if (index < 0 || index >= g_cached_module_count) {
        return NULL;
    }
    return g_module_cache[index].program;
}

// 获取缓存的模块IR
CnIrModule *cn_sem_get_cached_module_ir(int index) {
    if (index < 0 || index >= g_cached_module_count) {
        return NULL;
    }
    return g_module_cache[index].ir_module;
}

// 设置缓存的模块IR
void cn_sem_set_cached_module_ir(int index, CnIrModule *ir_module) {
    if (index >= 0 && index < g_cached_module_count) {
        g_module_cache[index].ir_module = ir_module;
    }
}

// 检查是否正在编译该模块（循环导入检测，使用规范化路径）
static int is_module_compiling(const char *file_path) {
    // 规范化路径以确保相同文件的不同路径表示能匹配
    char *normalized = normalize_file_path(file_path);
    const char *search_path = normalized ? normalized : file_path;
    
    for (int i = 0; i < g_compile_depth; i++) {
        // 编译栈中存储的已经是规范化路径
        if (strcmp(g_compiling_modules[i], search_path) == 0) {
            if (normalized) free(normalized);
            return 1;
        }
    }
    
    if (normalized) free(normalized);
    return 0;
}

// 将模块压入编译栈（存储规范化路径）
static int push_compiling_module(const char *file_path) {
    if (g_compile_depth >= MAX_MODULE_COMPILE_DEPTH) {
        return 0;  // 栈溢出
    }
    // 存储规范化路径的副本
    char *normalized = normalize_file_path(file_path);
    g_compiling_modules[g_compile_depth++] = normalized ? normalized : strdup(file_path);
    return 1;
}

// 将模块弹出编译栈
static void pop_compiling_module(void) {
    if (g_compile_depth > 0) {
        g_compile_depth--;
        // 释放规范化路径的内存
        if (g_compiling_modules[g_compile_depth]) {
            free((void *)g_compiling_modules[g_compile_depth]);
            g_compiling_modules[g_compile_depth] = NULL;
        }
    }
}

static void cn_sem_build_function_scope(CnSemScope *parent_scope,
                                         CnAstFunctionDecl *function_decl,
                                         CnDiagnostics *diagnostics);
static void cn_sem_build_block_scope(CnSemScope *parent_scope,
                                     CnAstBlockStmt *block,
                                     CnDiagnostics *diagnostics);
static void cn_sem_build_module_scope(CnSemScope *parent_scope,
                                      CnAstStmt *module_stmt,
                                      CnDiagnostics *diagnostics);
static void cn_sem_build_stmt(CnSemScope *scope, CnAstStmt *stmt, CnDiagnostics *diagnostics);
static void cn_sem_build_expr(CnSemScope *scope, CnAstExpr *expr, CnDiagnostics *diagnostics);
static void cn_sem_build_if_stmt(CnSemScope *scope, CnAstIfStmt *if_stmt, CnDiagnostics *diagnostics);
static void cn_sem_build_while_stmt(CnSemScope *scope, CnAstWhileStmt *while_stmt, CnDiagnostics *diagnostics);
static void cn_sem_build_for_stmt(CnSemScope *scope, CnAstForStmt *for_stmt, CnDiagnostics *diagnostics);

CnSemScope *cn_sem_build_scopes(CnAstProgram *program, CnDiagnostics *diagnostics)
{
    CnSemScope *global_scope;
    size_t i;

    if (!program) {
        return NULL;
    }

    global_scope = cn_sem_scope_new(CN_SEM_SCOPE_GLOBAL, NULL);
    if (!global_scope) {
        return NULL;
    }

    // 注册内置函数：打印 (print)
    CnSemSymbol *print_sym = cn_sem_scope_insert_symbol(global_scope, "打印", strlen("打印"), CN_SEM_SYMBOL_FUNCTION);
    if (print_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);
        print_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_VOID), param_types, 1);
    }

    // 注册内置函数：打印整数 (print_int)
    CnSemSymbol *print_int_sym = cn_sem_scope_insert_symbol(global_scope, "打印整数", strlen("打印整数"), CN_SEM_SYMBOL_FUNCTION);
    if (print_int_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_INT);
        print_int_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_VOID), param_types, 1);
    }

    // 注册内置函数：打印字符串 (print_string)
    CnSemSymbol *print_str_sym = cn_sem_scope_insert_symbol(global_scope, "打印字符串", strlen("打印字符串"), CN_SEM_SYMBOL_FUNCTION);
    if (print_str_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);
        print_str_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_VOID), param_types, 1);
    }

    // 注册内置函数：打印格式 (printf)
    // 特殊处理：支持可变参数，返回整数
    CnSemSymbol *printf_sym = cn_sem_scope_insert_symbol(global_scope, "打印格式", strlen("打印格式"), CN_SEM_SYMBOL_FUNCTION);
    if (printf_sym) {
        // 使用 UNKNOWN 类型标记，让类型检查器特殊处理可变参数
        printf_sym->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
    }

    // 注册内置函数：字符串格式 (sprintf)
    // 特殊处理：支持可变参数，返回字符串
    CnSemSymbol *sprintf_sym = cn_sem_scope_insert_symbol(global_scope, "字符串格式", strlen("字符串格式"), CN_SEM_SYMBOL_FUNCTION);
    if (sprintf_sym) {
        // 使用 UNKNOWN 类型标记，让类型检查器特殊处理可变参数
        sprintf_sym->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
    }

    // 注册内置函数：字符串格式化 (snprintf)
    // 特殊处理：支持可变参数，返回整数
    CnSemSymbol *snprintf_sym = cn_sem_scope_insert_symbol(global_scope, "字符串格式化", strlen("字符串格式化"), CN_SEM_SYMBOL_FUNCTION);
    if (snprintf_sym) {
        // 使用 UNKNOWN 类型标记，让类型检查器特殊处理可变参数
        snprintf_sym->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
    }

    // 注册内置函数：格式化字符串 (sprintf的别名)
    // 特殊处理：支持可变参数，返回整数
    CnSemSymbol *format_str_sym = cn_sem_scope_insert_symbol(global_scope, "格式化字符串", strlen("格式化字符串"), CN_SEM_SYMBOL_FUNCTION);
    if (format_str_sym) {
        // 使用 UNKNOWN 类型标记，让类型检查器特殊处理可变参数
        format_str_sym->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
    }

    // 注册内置函数：长度 (length)
    // 注意：长度函数是特殊的，它可以接受字符串或数组参数
    // 我们在符号表中标记它，但在 semantic_passes.c 中特殊处理其类型检查
    // 为了避免类型冲突，我们将其标记为 UNKNOWN 类型，让特殊处理逻辑来判断
    CnSemSymbol *length_sym = cn_sem_scope_insert_symbol(global_scope, "长度", strlen("长度"), CN_SEM_SYMBOL_FUNCTION);
    if (length_sym) {
        // 使用 UNKNOWN 类型标记，让类型检查器特殊处理
        length_sym->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
    }

    // =============================================================================
    // 注册输入函数
    // =============================================================================
    
    // 注册内置函数：读取行 (read_line)
    // 返回字符串，无参数
    CnSemSymbol *read_line_sym = cn_sem_scope_insert_symbol(global_scope, "读取行", strlen("读取行"), CN_SEM_SYMBOL_FUNCTION);
    if (read_line_sym) {
        read_line_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_STRING), NULL, 0);
    }
    
    // 注册内置函数：读取整数 (read_int)
    // 返回整数，参数为整数指针（传递地址）
    CnSemSymbol *read_int_sym = cn_sem_scope_insert_symbol(global_scope, "读取整数", strlen("读取整数"), CN_SEM_SYMBOL_FUNCTION);
    if (read_int_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_INT));
        read_int_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 1);
    }
    
    // 注册内置函数：读取小数 (read_float)
    // 返回整数（成功/失败），参数为小数指针（传递地址）
    CnSemSymbol *read_float_sym = cn_sem_scope_insert_symbol(global_scope, "读取小数", strlen("读取小数"), CN_SEM_SYMBOL_FUNCTION);
    if (read_float_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_FLOAT));
        read_float_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 1);
    }
    
    // 注册内置函数：读取字符串 (read_string)
    // 返回整数（成功/失败），参数为字符串缓冲区和大小
    CnSemSymbol *read_string_sym = cn_sem_scope_insert_symbol(global_scope, "读取字符串", strlen("读取字符串"), CN_SEM_SYMBOL_FUNCTION);
    if (read_string_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *) * 2);
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);  // char* buffer
        param_types[1] = cn_type_new_primitive(CN_TYPE_INT);     // size_t size
        read_string_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 2);
    }
    
    // 注册内置函数：读取字符 (read_char)
    // 返回整数（成功/失败），参数为字符指针
    CnSemSymbol *read_char_sym = cn_sem_scope_insert_symbol(global_scope, "读取字符", strlen("读取字符"), CN_SEM_SYMBOL_FUNCTION);
    if (read_char_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_INT));  // char* 简化为 int*
        read_char_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 1);
    }
    
    // =============================================================================
    // 注册通用输入函数（自动类型识别）
    // =============================================================================
    
    // 注册内置函数：读取 (read)
    // 返回输入值结构体，无参数
    CnSemSymbol *read_sym = cn_sem_scope_insert_symbol(global_scope, "读取", strlen("读取"), CN_SEM_SYMBOL_FUNCTION);
    if (read_sym) {
        // 返回类型为通用输入值（在运行时是结构体）
        read_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_VOID), NULL, 0);
    }
    
    // 注册内置函数：是整数 (is_int)
    // 返回整数（布尔值），参数为输入值指针
    CnSemSymbol *is_int_sym = cn_sem_scope_insert_symbol(global_scope, "是整数", strlen("是整数"), CN_SEM_SYMBOL_FUNCTION);
    if (is_int_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID));
        is_int_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 1);
    }
    
    // 注册内置函数：是小数 (is_float)
    CnSemSymbol *is_float_sym = cn_sem_scope_insert_symbol(global_scope, "是小数", strlen("是小数"), CN_SEM_SYMBOL_FUNCTION);
    if (is_float_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID));
        is_float_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 1);
    }
    
    // 注册内置函数：是字符串 (is_string)
    CnSemSymbol *is_string_sym = cn_sem_scope_insert_symbol(global_scope, "是字符串", strlen("是字符串"), CN_SEM_SYMBOL_FUNCTION);
    if (is_string_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID));
        is_string_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 1);
    }
    
    // 注册内置函数：是数值 (is_number)
    CnSemSymbol *is_number_sym = cn_sem_scope_insert_symbol(global_scope, "是数值", strlen("是数值"), CN_SEM_SYMBOL_FUNCTION);
    if (is_number_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID));
        is_number_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 1);
    }
    
    // 注册内置函数：取整数 (to_int)
    CnSemSymbol *to_int_sym = cn_sem_scope_insert_symbol(global_scope, "取整数", strlen("取整数"), CN_SEM_SYMBOL_FUNCTION);
    if (to_int_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID));
        to_int_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 1);
    }
    
    // 注册内置函数：取小数 (to_float)
    CnSemSymbol *to_float_sym = cn_sem_scope_insert_symbol(global_scope, "取小数", strlen("取小数"), CN_SEM_SYMBOL_FUNCTION);
    if (to_float_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID));
        to_float_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_FLOAT), param_types, 1);
    }
    
    // 注册内置函数：取文本 (to_string)
    CnSemSymbol *to_string_sym = cn_sem_scope_insert_symbol(global_scope, "取文本", strlen("取文本"), CN_SEM_SYMBOL_FUNCTION);
    if (to_string_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID));
        to_string_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_STRING), param_types, 1);
    }
    
    // =============================================================================
    // 注册内存管理函数
    // =============================================================================
    
    // 注册内置函数：分配内存 (malloc)
    CnSemSymbol *malloc_sym = cn_sem_scope_insert_symbol(global_scope, "分配内存", strlen("分配内存"), CN_SEM_SYMBOL_FUNCTION);
    if (malloc_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_INT);  // size_t 简化为 int
        malloc_sym->type = cn_type_new_function(cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID)), param_types, 1);
    }
    
    // 注册内置函数：释放内存 (free)
    CnSemSymbol *free_sym = cn_sem_scope_insert_symbol(global_scope, "释放内存", strlen("释放内存"), CN_SEM_SYMBOL_FUNCTION);
    if (free_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID));
        free_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_VOID), param_types, 1);
    }
    
    // 注册内置函数：重新分配内存 (realloc)
    CnSemSymbol *realloc_sym = cn_sem_scope_insert_symbol(global_scope, "重新分配内存", strlen("重新分配内存"), CN_SEM_SYMBOL_FUNCTION);
    if (realloc_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *) * 2);
        param_types[0] = cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID));
        param_types[1] = cn_type_new_primitive(CN_TYPE_INT);  // size_t 简化为 int
        realloc_sym->type = cn_type_new_function(cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID)), param_types, 2);
    }
    
    // 注册内置函数：分配清零内存 (calloc)
    CnSemSymbol *calloc_sym = cn_sem_scope_insert_symbol(global_scope, "分配清零内存", strlen("分配清零内存"), CN_SEM_SYMBOL_FUNCTION);
    if (calloc_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *) * 2);
        param_types[0] = cn_type_new_primitive(CN_TYPE_INT);  // count
        param_types[1] = cn_type_new_primitive(CN_TYPE_INT);  // size
        calloc_sym->type = cn_type_new_function(cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID)), param_types, 2);
    }

    // 注册内置函数：分配内存数组 (malloc array)
    // 参数：类型大小, 数量
    CnSemSymbol *malloc_array_sym = cn_sem_scope_insert_symbol(global_scope, "分配内存数组", strlen("分配内存数组"), CN_SEM_SYMBOL_FUNCTION);
    if (malloc_array_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *) * 2);
        param_types[0] = cn_type_new_primitive(CN_TYPE_INT);  // type_size
        param_types[1] = cn_type_new_primitive(CN_TYPE_INT);  // count
        malloc_array_sym->type = cn_type_new_function(cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID)), param_types, 2);
    }

    // 注册内置函数：类型大小 (sizeof)
    // 参数：类型名（编译时计算）
    CnSemSymbol *sizeof_sym = cn_sem_scope_insert_symbol(global_scope, "类型大小", strlen("类型大小"), CN_SEM_SYMBOL_FUNCTION);
    if (sizeof_sym) {
        // 类型大小 接受任意类型参数，返回整数类型
        // 使用 UNKNOWN 类型表示接受任意类型
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_UNKNOWN);  // 接受任意类型
        sizeof_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 1);
    }
    
    // 注册内置函数：释放输入 (free_input)
    CnSemSymbol *free_input_sym = cn_sem_scope_insert_symbol(global_scope, "释放输入", strlen("释放输入"), CN_SEM_SYMBOL_FUNCTION);
    if (free_input_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID));
        free_input_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_VOID), param_types, 1);
    }
    
    // =============================================================================
    // 注册字符串转换函数（简化版，直接用于 CN 语言）
    // =============================================================================
    
    // 注册内置函数：转整数 (str_to_int)
    CnSemSymbol *str_to_int_sym = cn_sem_scope_insert_symbol(global_scope, "转整数", strlen("转整数"), CN_SEM_SYMBOL_FUNCTION);
    if (str_to_int_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);
        str_to_int_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 1);
    }
    
    // 注册内置函数：转小数 (str_to_float)
    CnSemSymbol *str_to_float_sym = cn_sem_scope_insert_symbol(global_scope, "转小数", strlen("转小数"), CN_SEM_SYMBOL_FUNCTION);
    if (str_to_float_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);
        str_to_float_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_FLOAT), param_types, 1);
    }
    
    // 注册内置函数：是数字文本 (is_numeric_str)
    CnSemSymbol *is_numeric_sym = cn_sem_scope_insert_symbol(global_scope, "是数字文本", strlen("是数字文本"), CN_SEM_SYMBOL_FUNCTION);
    if (is_numeric_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);
        is_numeric_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 1);
    }
    
    // 注册内置函数：是整数文本 (is_int_str)
    CnSemSymbol *is_int_str_sym = cn_sem_scope_insert_symbol(global_scope, "是整数文本", strlen("是整数文本"), CN_SEM_SYMBOL_FUNCTION);
    if (is_int_str_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);
        is_int_str_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 1);
    }
    
    // =============================================================================
    // 注册字符串操作函数
    // =============================================================================
    
    // 注册内置函数：比较字符串 (strcmp)
    CnSemSymbol *strcmp_sym = cn_sem_scope_insert_symbol(global_scope, "比较字符串", strlen("比较字符串"), CN_SEM_SYMBOL_FUNCTION);
    if (strcmp_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *) * 2);
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);
        param_types[1] = cn_type_new_primitive(CN_TYPE_STRING);
        strcmp_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 2);
    }
    
    // 注册内置函数：字符串长度 (strlen)
    CnSemSymbol *strlen_sym = cn_sem_scope_insert_symbol(global_scope, "字符串长度", strlen("字符串长度"), CN_SEM_SYMBOL_FUNCTION);
    if (strlen_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);
        strlen_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 1);
    }
    
    // 注册内置函数：复制字符串 (strcpy)
    CnSemSymbol *strcpy_sym = cn_sem_scope_insert_symbol(global_scope, "复制字符串", strlen("复制字符串"), CN_SEM_SYMBOL_FUNCTION);
    if (strcpy_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *) * 2);
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);  // dest
        param_types[1] = cn_type_new_primitive(CN_TYPE_STRING);  // src
        strcpy_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_STRING), param_types, 2);
    }
    
    // 注册内置函数：连接字符串 (strcat)
    CnSemSymbol *strcat_sym = cn_sem_scope_insert_symbol(global_scope, "连接字符串", strlen("连接字符串"), CN_SEM_SYMBOL_FUNCTION);
    if (strcat_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *) * 2);
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);  // dest
        param_types[1] = cn_type_new_primitive(CN_TYPE_STRING);  // src
        strcat_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_STRING), param_types, 2);
    }

    // =============================================================================
    // 先注册枚举声明到全局作用域（必须在结构体之前，因为结构体可能使用枚举类型）
    // =============================================================================
    for (i = 0; i < program->enum_count; ++i) {
        CnAstStmt *enum_stmt = program->enums[i];
        if (!enum_stmt || enum_stmt->kind != CN_AST_STMT_ENUM_DECL) {
            continue;
        }

        CnAstEnumDecl *enum_decl = &enum_stmt->as.enum_decl;
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(global_scope,
                                   enum_decl->name,
                                   enum_decl->name_length,
                                   CN_SEM_SYMBOL_ENUM);
        if (sym) {
            // 创建枚举类型
            sym->type = cn_type_new_enum(enum_decl->name, enum_decl->name_length);
            
            // 为枚举创建一个作用域来存储其成员
            CnSemScope *enum_scope = cn_sem_scope_new(CN_SEM_SCOPE_ENUM, global_scope);
            if (enum_scope && sym->type) {
                sym->type->as.enum_type.enum_scope = enum_scope;
                // 【关键修复】同时设置 sym->as.module_scope，以便模块导入时能正确复制枚举作用域
                sym->as.module_scope = enum_scope;
                
                // 注册枚举成员到枚举作用域和全局作用域
                // 这样可以直接通过成员名访问枚举成员（如：红、绿、蓝）
                for (size_t j = 0; j < enum_decl->member_count; j++) {
                    CnAstEnumMember *member = &enum_decl->members[j];
                    
                    // 先注册到枚举作用域
                    CnSemSymbol *member_sym = cn_sem_scope_insert_symbol(enum_scope,
                                                       member->name,
                                                       member->name_length,
                                                       CN_SEM_SYMBOL_ENUM_MEMBER);
                    if (member_sym) {
                        // 枚举成员的类型是整数
                        member_sym->type = cn_type_new_primitive(CN_TYPE_INT);
                        // 保存枚举成员的值
                        member_sym->as.enum_value = member->value;
                    }
                    // 注意：枚举作用域中的重复定义不报错，因为可能是导入的
                    
                    // 同时注册到全局作用域（如果不存在同名符号）
                    CnSemSymbol *global_member_sym = cn_sem_scope_insert_symbol(global_scope,
                                                       member->name,
                                                       member->name_length,
                                                       CN_SEM_SYMBOL_ENUM_MEMBER);
                    if (global_member_sym) {
                        // 枚举成员的类型应该是枚举类型，而不是整数类型
                        // 这样在代码生成时可以获取正确的枚举类型名称
                        global_member_sym->type = sym->type; // 使用枚举类型
                        global_member_sym->as.enum_value = member->value;
                    }
                    // 注意：全局作用域中的重复定义不报错，因为可能是导入的
                }
            }
        } else {
            // 插入失败，检查是否是导入的符号
            CnSemSymbol *existing = cn_sem_scope_lookup_shallow(global_scope, enum_decl->name, enum_decl->name_length);
            fprintf(stderr, "[DEBUG] scope_builder: enum '%.*s' insert failed, existing=%p, existing->source_module_path=%p\n",
                    (int)enum_decl->name_length, enum_decl->name, (void*)existing,
                    existing ? (void*)existing->source_module_path : NULL);
            if (existing && existing->kind == CN_SEM_SYMBOL_ENUM && existing->source_module_path != NULL) {
                // 是导入的枚举（source_module_path 不为空表示来自其他模块），静默跳过（不报错）
                fprintf(stderr, "[DEBUG] scope_builder: skipping imported enum '%.*s'\n",
                        (int)enum_decl->name_length, enum_decl->name);
            } else {
                // 真正的重复定义，报告错误
                fprintf(stderr, "[DEBUG] scope_builder: reporting duplicate enum '%.*s'\n",
                        (int)enum_decl->name_length, enum_decl->name);
                cn_support_diag_semantic_error_duplicate_symbol(
                    diagnostics, NULL, 0, 0, enum_decl->name, enum_decl->name_length);
            }
        }
    }

    // =============================================================================
    // 注册结构体声明到全局作用域（在枚举之后，支持结构体使用枚举类型字段）
    // =============================================================================
    for (i = 0; i < program->struct_count; ++i) {
        CnAstStmt *struct_stmt = program->structs[i];
        if (!struct_stmt || struct_stmt->kind != CN_AST_STMT_STRUCT_DECL) {
            continue;
        }

        CnAstStructDecl *struct_decl = &struct_stmt->as.struct_decl;
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(global_scope,
                                   struct_decl->name,
                                   struct_decl->name_length,
                                   CN_SEM_SYMBOL_STRUCT);
        if (sym) {
            // 创建结构体类型，包含字段信息
            CnStructField *fields = NULL;
            if (struct_decl->field_count > 0) {
                fields = (CnStructField *)malloc(sizeof(CnStructField) * struct_decl->field_count);
                for (size_t j = 0; j < struct_decl->field_count; j++) {
                    fields[j].name = struct_decl->fields[j].name;
                    fields[j].name_length = struct_decl->fields[j].name_length;
                    
                    // 解析字段类型：如果是自定义类型（结构体类型表示），从符号表查找真实类型
                    CnType *field_type = struct_decl->fields[j].field_type;
                    if (field_type && field_type->kind == CN_TYPE_STRUCT && field_type->as.struct_type.name) {
                        CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                                    field_type->as.struct_type.name,
                                                    field_type->as.struct_type.name_length);
                        if (type_sym && type_sym->type) {
                            // 使用符号表中的真实类型（可能是枚举或结构体）
                            field_type = type_sym->type;
                        }
                    }
                    // 处理指针类型：如果字段是指针指向自定义类型，需要解析指针指向的类型
                    // 注意：字符* 等基础类型指针不需要解析，只有指向自定义类型的指针需要
                    // 例如：结构体* 字段 需要解析 结构体 类型
                    // 但 字符* 不需要解析，因为 CHAR 是基础类型
                    else if (field_type && field_type->kind == CN_TYPE_POINTER &&
                             field_type->as.pointer_to &&
                             field_type->as.pointer_to->kind == CN_TYPE_STRUCT &&
                             field_type->as.pointer_to->as.struct_type.name) {
                        CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                                    field_type->as.pointer_to->as.struct_type.name,
                                                    field_type->as.pointer_to->as.struct_type.name_length);
                        if (type_sym && type_sym->type) {
                            // 创建新的指针类型，指向解析后的类型
                            field_type = cn_type_new_pointer(type_sym->type);
                        }
                    }
                    fields[j].field_type = field_type;
                    fields[j].is_const = struct_decl->fields[j].is_const;  // 传递常量字段标记
                }
            }
            sym->type = cn_type_new_struct(struct_decl->name,
                                          struct_decl->name_length,
                                          fields,
                                          struct_decl->field_count,
                                          global_scope,
                                          NULL, 0);  // 全局结构体，无所属函数
        } else {
            // 插入失败，检查是否是导入的符号
            CnSemSymbol *existing = cn_sem_scope_lookup_shallow(global_scope, struct_decl->name, struct_decl->name_length);
            fprintf(stderr, "[DEBUG] scope_builder: struct '%.*s' insert failed, existing=%p, existing->source_module_path=%p\n",
                    (int)struct_decl->name_length, struct_decl->name, (void*)existing,
                    existing ? (void*)existing->source_module_path : NULL);
            if (existing && existing->kind == CN_SEM_SYMBOL_STRUCT && existing->source_module_path != NULL) {
                // 是导入的结构体（source_module_path 不为空表示来自其他模块），静默跳过（不报错）
                fprintf(stderr, "[DEBUG] scope_builder: skipping imported struct '%.*s'\n",
                        (int)struct_decl->name_length, struct_decl->name);
            } else {
                // 真正的重复定义，报告错误
                fprintf(stderr, "[DEBUG] scope_builder: reporting duplicate struct '%.*s'\n",
                        (int)struct_decl->name_length, struct_decl->name);
                cn_support_diag_semantic_error_duplicate_symbol(
                    diagnostics, NULL, 0, 0, struct_decl->name, struct_decl->name_length);
            }
        }
    }
    
    // =============================================================================
    // 延迟解析：更新所有不完整的结构体字段类型（主程序）
    // 解决结构体指针前向引用问题
    // =============================================================================
    {
        CnSemSymbolNode *node = global_scope->symbols;
        while (node) {
            CnSemSymbol *sym = &node->symbol;
            if (sym->kind == CN_SEM_SYMBOL_STRUCT && sym->type &&
                sym->type->kind == CN_TYPE_STRUCT &&
                sym->type->as.struct_type.fields) {
                for (size_t j = 0; j < sym->type->as.struct_type.field_count; j++) {
                    CnStructField *field = &sym->type->as.struct_type.fields[j];
                    CnType *field_type = field->field_type;
                    
                    // 情况1：字段类型是结构体但没有字段信息
                    if (field_type && field_type->kind == CN_TYPE_STRUCT &&
                        !field_type->as.struct_type.fields &&
                        field_type->as.struct_type.name) {
                        CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                                    field_type->as.struct_type.name,
                                                    field_type->as.struct_type.name_length);
                        if (type_sym && type_sym->type &&
                            type_sym->kind == CN_SEM_SYMBOL_STRUCT &&
                            type_sym->type->as.struct_type.fields) {
                            field->field_type = type_sym->type;
                        }
                    }
                    // 情况2：字段类型是指针，指向的结构体没有字段信息
                    else if (field_type && field_type->kind == CN_TYPE_POINTER &&
                             field_type->as.pointer_to &&
                             field_type->as.pointer_to->kind == CN_TYPE_STRUCT &&
                             !field_type->as.pointer_to->as.struct_type.fields &&
                             field_type->as.pointer_to->as.struct_type.name) {
                        CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                                    field_type->as.pointer_to->as.struct_type.name,
                                                    field_type->as.pointer_to->as.struct_type.name_length);
                        if (type_sym && type_sym->type &&
                            type_sym->kind == CN_SEM_SYMBOL_STRUCT &&
                            type_sym->type->as.struct_type.fields) {
                            field->field_type = cn_type_new_pointer(type_sym->type);
                        }
                    }
                }
            }
            node = node->next;
        }
    }

    // 处理导入语句：将被导入模块的符号添加到全局作用域
    for (i = 0; i < program->import_count; ++i) {
        CnAstStmt *import_stmt = program->imports[i];
        if (!import_stmt || import_stmt->kind != CN_AST_STMT_IMPORT) {
            continue;
        }

        CnAstImportStmt *import = &import_stmt->as.import_stmt;
        
        // 查找被导入的模块
        CnSemSymbol *module_sym = NULL;
        
        // 支持两种方式：传统 module_name 和 module_path
        if (import->module_path) {
            // 使用模块路径查找（支持相对路径和点/斜杠路径）
            // 对于相对路径，暂时忽略，因为需要 module_loader 支持
            // 这里先处理简单的绝对路径：使用最后一段作为模块名
            if (import->module_path->segment_count > 0) {
                // 取最后一段作为模块名
                CnAstModulePathSegment *last_segment = 
                    &import->module_path->segments[import->module_path->segment_count - 1];
                module_sym = cn_sem_scope_lookup_shallow(global_scope,
                                                         last_segment->name,
                                                         last_segment->name_length);
            }
        } else if (import->module_name) {
            // 传统的简单模块名查找
            module_sym = cn_sem_scope_lookup_shallow(global_scope,
                                                     import->module_name,
                                                     import->module_name_length);
        }
        
        if (!module_sym) {
            // 模块不存在，报错
            cn_support_diag_semantic_error_generic(
                diagnostics,
                CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                NULL, 0, 0,
                "语义错误：导入的模块不存在");
            continue;
        }
        
        if (module_sym->kind != CN_SEM_SYMBOL_MODULE || !module_sym->as.module_scope) {
            // 符号不是模块，报错
            cn_support_diag_semantic_error_generic(
                diagnostics,
                CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                NULL, 0, 0,
                "语义错误：导入的符号不是模块");
            continue;
        }
        
        CnSemScope *module_scope = module_sym->as.module_scope;
        
        // 如果指定了别名，则以别名注册模块符号
        if (import->alias) {
            // 使用别名注册模块
            CnSemSymbol *existing_alias = cn_sem_scope_lookup_shallow(global_scope,
                                                                      import->alias,
                                                                      import->alias_length);
            if (existing_alias) {
                // 别名冲突，输出详细诊断信息
                char alias_name[256];
                size_t copy_len = import->alias_length < 255 ? import->alias_length : 255;
                memcpy(alias_name, import->alias, copy_len);
                alias_name[copy_len] = '\0';
                
                char module_name[256];
                copy_len = import->module_name_length < 255 ? import->module_name_length : 255;
                memcpy(module_name, import->module_name, copy_len);
                module_name[copy_len] = '\0';
                
                char error_msg[512];
                snprintf(error_msg, sizeof(error_msg),
                        "语义错误：模块别名 '%s' 已存在（导入模块 '%s' 时发生冲突）",
                        alias_name, module_name);
                        
                cn_support_diag_semantic_error_generic(
                    diagnostics,
                    CN_DIAG_CODE_SEM_DUPLICATE_SYMBOL,
                    NULL, 0, 0,
                    error_msg);
                continue;
            }
            
            // 以别名注册模块符号
            CnSemSymbol *alias_sym = cn_sem_scope_insert_symbol(global_scope,
                                                                import->alias,
                                                                import->alias_length,
                                                                CN_SEM_SYMBOL_MODULE);
            if (alias_sym) {
                alias_sym->type = module_sym->type;
                alias_sym->is_public = module_sym->is_public;
                alias_sym->is_const = module_sym->is_const;
                alias_sym->as.module_scope = module_sym->as.module_scope;
            }
            continue;  // 使用别名时不进行成员导入
        }
        
        // 如果不是「从 ... 导入」语法，则不进行成员导入
        // 「导入 xxx;」 语法只注册模块符号，不导入其成员
        if (!import->use_from_syntax) {
            continue;
        }
        
        // 判断是全量导入还是选择性导入
        if (import->member_count == 0) {
            // 全量导入：遍历模块作用域中的所有符号，添加到全局作用域
            CnSemSymbolNode *node = module_scope->symbols;
            while (node) {
                CnSemSymbol *sym = &node->symbol;
                
                // 检查名称冲突
                CnSemSymbol *existing_sym = cn_sem_scope_lookup_shallow(global_scope,
                                                                        sym->name,
                                                                        sym->name_length);
                if (existing_sym) {
                    // 检查是否是同一个符号（同一模块的同一成员）
                    fprintf(stderr, "[DEBUG] Import: checking symbol %.*s, existing_kind=%d, new_kind=%d, existing_type=%p, new_type=%p\n",
                            (int)sym->name_length, sym->name, existing_sym->kind, sym->kind,
                            (void*)existing_sym->type, (void*)sym->type);
                    if (cn_sem_is_same_symbol(existing_sym, sym)) {
                        // 同一符号，但如果现有符号缺少类型信息，需要补充
                        if (!existing_sym->type && sym->type) {
                            fprintf(stderr, "[DEBUG] Import: same symbol, updating missing type info\n");
                            existing_sym->type = cn_type_deep_copy(sym->type);
                        }
                        fprintf(stderr, "[DEBUG] Import: same symbol, skipping\n");
                        node = node->next;
                        continue;
                    }
                    // 【关键修复】同名不同类型的符号可以共存
                    // 例如：词元 结构体 和 词元类型枚举 枚举 可以同时存在
                    // 只有当符号种类相同且不是同一个符号时，才报告冲突
                    else if (existing_sym->kind == sym->kind && existing_sym->kind != CN_SEM_SYMBOL_MODULE) {
                        // 同名同种类但不是同一个符号，报错
                        fprintf(stderr, "[DEBUG] Import: same name and kind but different symbol, reporting error\n");
                        cn_support_diag_semantic_error_duplicate_symbol(
                            diagnostics, NULL, 0, 0, sym->name, sym->name_length);
                        node = node->next;
                        continue;
                    }
                    // 【新增】同名不同类型的符号，允许共存
                    // 例如：existing_sym 是结构体，sym 是枚举，两者可以共存
                    else if (existing_sym->kind != sym->kind) {
                        fprintf(stderr, "[DEBUG] Import: same name but different kinds (existing=%d, new=%d), allowing coexistence\n",
                                existing_sym->kind, sym->kind);
                        // 继续添加新符号（不跳过）
                    }
                }
                
                // 没有冲突，添加符号
                if (!existing_sym) {
                    CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(global_scope,
                                                                      sym->name,
                                                                      sym->name_length,
                                                                      sym->kind);
                    if (new_sym) {
                        // 复制符号信息
                        // 【关键修复】使用深度复制确保类型信息完整保留
                        // 解决模块导入时结构体字段信息丢失问题
                        
                        // 【调试日志】检查源符号的类型信息
                        fprintf(stderr, "[DEBUG] Importing symbol '%.*s', kind=%d, src_type=%p\n",
                                (int)sym->name_length, sym->name, sym->kind, (void*)sym->type);
                        
                        // 使用辅助函数复制类型信息（处理枚举类型为 NULL 的特殊情况）
                        cn_sem_copy_symbol_type(sym, new_sym);
                        
                        // 【调试日志】检查复制后的类型信息
                        fprintf(stderr, "[DEBUG] After copy: new_type=%p\n", (void*)new_sym->type);
                        
                        new_sym->is_public = sym->is_public;
                        new_sym->is_const = sym->is_const;
                        // 保留原始 decl_scope 以便区分导入符号
                        new_sym->decl_scope = sym->decl_scope;
                        // 复制源模块路径（关键：用于跨编译会话的符号唯一性判断）
                        new_sym->source_module_path = sym->source_module_path;
                        new_sym->source_module_path_length = sym->source_module_path_length;
                        // 复制 module_scope 以支持递归处理嵌套导入
                        if (sym->kind == CN_SEM_SYMBOL_MODULE ||
                            sym->kind == CN_SEM_SYMBOL_STRUCT ||
                            sym->kind == CN_SEM_SYMBOL_ENUM) {
                            new_sym->as.module_scope = sym->as.module_scope;
                        } else if (sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                            new_sym->as.enum_value = sym->as.enum_value;
                        }
                        // 注意：CN_SEM_SYMBOL_ENUM 不需要特殊处理，
                        // 因为枚举作用域已经存储在 sym->type->as.enum_type.enum_scope 中
                    }
                }
                
                node = node->next;
            }
        } else {
            // 选择性导入：只导入指定的成员
            for (size_t j = 0; j < import->member_count; ++j) {
                const char *member_name = import->members[j].name;
                size_t member_name_length = import->members[j].name_length;
                
                // 在模块作用域中查找指定成员
                CnSemSymbol *member_sym = cn_sem_scope_lookup_shallow(module_scope,
                                                                      member_name,
                                                                      member_name_length);
                
                if (!member_sym) {
                    // 成员不存在，报错
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                        NULL, 0, 0,
                        "语义错误：导入的成员不存在");
                    continue;
                }
                
                // 检查可见性：只能导入公开成员
                if (!member_sym->is_public) {
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_ACCESS_DENIED,
                        NULL, 0, 0,
                        "语义错误：无法导入私有成员");
                    continue;
                }
                
                // 检查名称冲突
                CnSemSymbol *existing_sym = cn_sem_scope_lookup_shallow(global_scope,
                                                                        member_name,
                                                                        member_name_length);
                if (existing_sym) {
                    // 检查是否是同一个符号（同一模块的同一成员）
                    if (cn_sem_is_same_symbol(existing_sym, member_sym)) {
                        // 同一符号，静默忽略，不报错也不重复添加
                        continue;
                    }
                    // 【关键修复】同名不同类型的符号可以共存
                    // 只有当符号种类相同且不是同一个符号时，才报告冲突
                    else if (existing_sym->kind == member_sym->kind && existing_sym->kind != CN_SEM_SYMBOL_MODULE) {
                        // 同名同种类但不是同一个符号，报错
                        cn_support_diag_semantic_error_duplicate_symbol(
                            diagnostics, NULL, 0, 0, member_name, member_name_length);
                        continue;
                    }
                    // 【新增】同名不同类型的符号，允许共存
                    else if (existing_sym->kind != member_sym->kind) {
                        fprintf(stderr, "[DEBUG] Selective import: same name but different kinds (existing=%d, new=%d), allowing coexistence\n",
                                existing_sym->kind, member_sym->kind);
                        // 继续添加新符号（不跳过）
                    }
                }
                
                // 没有冲突，添加符号
                if (!existing_sym) {
                    CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(global_scope,
                                                                      member_name,
                                                                      member_name_length,
                                                                      member_sym->kind);
                    if (new_sym) {
                        // 复制符号信息
                        // 【关键修复】使用辅助函数复制类型信息（处理枚举类型为 NULL 的特殊情况）
                        cn_sem_copy_symbol_type(member_sym, new_sym);
                        
                        new_sym->is_public = member_sym->is_public;
                        new_sym->decl_scope = member_sym->decl_scope;
                        // 复制源模块路径（关键：用于跨编译会话的符号唯一性判断）
                        new_sym->source_module_path = member_sym->source_module_path;
                        new_sym->source_module_path_length = member_sym->source_module_path_length;
                        // 【关键修复】复制 module_scope 以支持枚举和结构体类型
                        if (member_sym->kind == CN_SEM_SYMBOL_MODULE ||
                            member_sym->kind == CN_SEM_SYMBOL_STRUCT ||
                            member_sym->kind == CN_SEM_SYMBOL_ENUM) {
                            new_sym->as.module_scope = member_sym->as.module_scope;
                        } else if (member_sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                            new_sym->as.enum_value = member_sym->as.enum_value;
                        }
                    }
                }
            }
        }
    }

    // 注册全局变量声明到全局作用域
    for (i = 0; i < program->global_var_count; ++i) {
        CnAstStmt *var_stmt = program->global_vars[i];
        if (!var_stmt || var_stmt->kind != CN_AST_STMT_VAR_DECL) {
            continue;
        }

        CnAstVarDecl *var_decl = &var_stmt->as.var_decl;
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(global_scope,
                                   var_decl->name,
                                   var_decl->name_length,
                                   CN_SEM_SYMBOL_VARIABLE);
        if (sym) {
            // 全局变量类型处理：需要对结构体、结构体指针、结构体数组进行特殊处理
            if (var_decl->declared_type && var_decl->declared_type->kind == CN_TYPE_STRUCT) {
                // 结构体类型：需要查找结构体定义
                CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                        var_decl->declared_type->as.struct_type.name,
                                        var_decl->declared_type->as.struct_type.name_length);
                if (type_sym && type_sym->type &&
                    (type_sym->kind == CN_SEM_SYMBOL_STRUCT || type_sym->kind == CN_SEM_SYMBOL_ENUM)) {
                    sym->type = type_sym->type;
                } else {
                    sym->type = var_decl->declared_type;
                }
            } else if (var_decl->declared_type && var_decl->declared_type->kind == CN_TYPE_POINTER &&
                       var_decl->declared_type->as.pointer_to &&
                       var_decl->declared_type->as.pointer_to->kind == CN_TYPE_STRUCT) {
                // 指向结构体的指针类型
                CnType *ptr_type = var_decl->declared_type;
                CnType *pointee_type = ptr_type->as.pointer_to;
                CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                        pointee_type->as.struct_type.name,
                                        pointee_type->as.struct_type.name_length);
                if (type_sym && type_sym->type &&
                    (type_sym->kind == CN_SEM_SYMBOL_STRUCT || type_sym->kind == CN_SEM_SYMBOL_ENUM)) {
                    sym->type = cn_type_new_pointer(type_sym->type);
                } else {
                    sym->type = var_decl->declared_type;
                }
            } else if (var_decl->declared_type && var_decl->declared_type->kind == CN_TYPE_ARRAY &&
                       var_decl->declared_type->as.array.element_type &&
                       var_decl->declared_type->as.array.element_type->kind == CN_TYPE_STRUCT) {
                // 结构体数组类型：需要查找结构体定义并更新元素类型
                CnType *arr_type = var_decl->declared_type;
                CnType *elem_type = arr_type->as.array.element_type;
                CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                        elem_type->as.struct_type.name,
                                        elem_type->as.struct_type.name_length);
                if (type_sym && type_sym->type &&
                    (type_sym->kind == CN_SEM_SYMBOL_STRUCT || type_sym->kind == CN_SEM_SYMBOL_ENUM)) {
                    // 创建新的数组类型，元素类型为完整的结构体类型
                    sym->type = cn_type_new_array(type_sym->type, arr_type->as.array.length);
                } else {
                    sym->type = var_decl->declared_type;
                }
            } else {
                sym->type = var_decl->declared_type;
            }
            sym->is_const = var_decl->is_const;
        } else {
            // 插入失败，检查是否是导入的符号
            CnSemSymbol *existing = cn_sem_scope_lookup_shallow(global_scope, var_decl->name, var_decl->name_length);
            fprintf(stderr, "[DEBUG] scope_builder: var '%.*s' insert failed, existing=%p, existing->source_module_path=%p\n",
                    (int)var_decl->name_length, var_decl->name, (void*)existing,
                    existing ? (void*)existing->source_module_path : NULL);
            if (existing && existing->kind == CN_SEM_SYMBOL_VARIABLE && existing->source_module_path != NULL) {
                // 是导入的变量（source_module_path 不为空表示来自其他模块），静默跳过（不报错）
                fprintf(stderr, "[DEBUG] scope_builder: skipping imported var '%.*s'\n",
                        (int)var_decl->name_length, var_decl->name);
            } else {
                // 真正的重复定义，报告错误
                fprintf(stderr, "[DEBUG] scope_builder: reporting duplicate var '%.*s'\n",
                        (int)var_decl->name_length, var_decl->name);
                cn_support_diag_semantic_error_duplicate_symbol(
                    diagnostics, NULL, 0, 0, var_decl->name, var_decl->name_length);
            }
        }
    }

    for (i = 0; i < program->function_count; ++i) {
        CnAstFunctionDecl *function_decl = program->functions[i];

        if (!function_decl) {
            continue;
        }

        CnSemSymbol *sym = cn_sem_scope_insert_symbol(global_scope,
                                   function_decl->name,
                                   function_decl->name_length,
                                   CN_SEM_SYMBOL_FUNCTION);
        if (sym) {
            // 构建完整的函数类型
            CnType **param_types = NULL;
            if (function_decl->parameter_count > 0) {
                param_types = (CnType **)malloc(sizeof(CnType *) * function_decl->parameter_count);
                for (size_t j = 0; j < function_decl->parameter_count; j++) {
                    CnType *param_type = function_decl->parameters[j].declared_type;
                    // 特殊处理：如果参数类型是结构体类型，可能是枚举类型或类类型
                    // 需要从符号表查找真实类型
                    if (param_type && param_type->kind == CN_TYPE_STRUCT) {
                        CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                                param_type->as.struct_type.name,
                                                param_type->as.struct_type.name_length);
                        if (type_sym && type_sym->type) {
                            if (type_sym->kind == CN_SEM_SYMBOL_ENUM) {
                                // 替换为枚举类型
                                param_type = type_sym->type;
                            } else if (type_sym->kind == CN_SEM_SYMBOL_STRUCT) {
                                // 替换为完整的结构体/类类型
                                param_type = type_sym->type;
                            }
                        }
                    }
                    param_types[j] = param_type;
                }
            }
            // 使用函数声明中的返回类型，如果没有则使用UNKNOWN（后续通过return语句推断）
            CnType *return_type = function_decl->return_type;
            if (!return_type) {
                return_type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
            } else if (return_type->kind == CN_TYPE_STRUCT) {
                // 特殊处理：如果返回类型是结构体类型，可能是枚举类型或类类型
                // 需要从符号表查找真实类型
                CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                        return_type->as.struct_type.name,
                                        return_type->as.struct_type.name_length);
                if (type_sym && type_sym->type) {
                    if (type_sym->kind == CN_SEM_SYMBOL_ENUM) {
                        // 替换为枚举类型
                        return_type = type_sym->type;
                    } else if (type_sym->kind == CN_SEM_SYMBOL_STRUCT) {
                        // 替换为完整的结构体/类类型
                        return_type = type_sym->type;
                    }
                }
            }
            sym->type = cn_type_new_function(return_type,
                                            param_types,
                                            function_decl->parameter_count);
        } else {
            // 插入失败，检查是否是导入的符号
            CnSemSymbol *existing_sym = cn_sem_scope_lookup_shallow(global_scope,
                                                                    function_decl->name,
                                                                    function_decl->name_length);
            if (existing_sym && existing_sym->kind == CN_SEM_SYMBOL_FUNCTION) {
                // 检查是否是函数原型声明（无函数体）
                if (function_decl->is_prototype) {
                    // 函数原型声明：允许重复声明，不报错
                    // 不需要做任何事情，保留现有符号
                } else if (existing_sym->source_module_path != NULL) {
                    // 这是导入的函数（source_module_path 不为空表示来自其他模块），用当前模块定义的函数覆盖它
                    // 构建完整的函数类型
                    CnType **param_types = NULL;
                    if (function_decl->parameter_count > 0) {
                        param_types = (CnType **)malloc(sizeof(CnType *) * function_decl->parameter_count);
                        for (size_t j = 0; j < function_decl->parameter_count; j++) {
                            CnType *param_type = function_decl->parameters[j].declared_type;
                            // 特殊处理：如果参数类型是结构体类型，可能是枚举类型或类类型
                            if (param_type && param_type->kind == CN_TYPE_STRUCT) {
                                CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                                        param_type->as.struct_type.name,
                                                        param_type->as.struct_type.name_length);
                                if (type_sym && type_sym->type) {
                                    if (type_sym->kind == CN_SEM_SYMBOL_ENUM) {
                                        param_type = type_sym->type;
                                    } else if (type_sym->kind == CN_SEM_SYMBOL_STRUCT) {
                                        param_type = type_sym->type;
                                    }
                                }
                            }
                            param_types[j] = param_type;
                        }
                    }
                    CnType *return_type = function_decl->return_type;
                    if (!return_type) {
                        return_type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                    } else if (return_type->kind == CN_TYPE_STRUCT) {
                        CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                                return_type->as.struct_type.name,
                                                return_type->as.struct_type.name_length);
                        if (type_sym && type_sym->type) {
                            if (type_sym->kind == CN_SEM_SYMBOL_ENUM) {
                                return_type = type_sym->type;
                            } else if (type_sym->kind == CN_SEM_SYMBOL_STRUCT) {
                                return_type = type_sym->type;
                            }
                        }
                    }
                    // 更新现有符号的类型和声明作用域
                    existing_sym->type = cn_type_new_function(return_type,
                                                    param_types,
                                                    function_decl->parameter_count);
                    existing_sym->decl_scope = global_scope;  // 标记为当前模块定义
                } else {
                    // 检查是否是导入的函数
                    fprintf(stderr, "[DEBUG] scope_builder: func '%.*s' kind mismatch, existing->source_module_path=%p\n",
                            (int)function_decl->name_length, function_decl->name, (void*)existing_sym->source_module_path);
                    if (existing_sym->source_module_path != NULL) {
                        // 是导入的函数（source_module_path 不为空表示来自其他模块），静默跳过（不报错）
                        fprintf(stderr, "[DEBUG] scope_builder: skipping imported func '%.*s'\n",
                                (int)function_decl->name_length, function_decl->name);
                    } else {
                        // 真正的重复定义（同一模块内）
                        fprintf(stderr, "[DEBUG] scope_builder: reporting duplicate func '%.*s'\n",
                                (int)function_decl->name_length, function_decl->name);
                        cn_support_diag_semantic_error_duplicate_symbol(
                            diagnostics, NULL, 0, 0, function_decl->name, function_decl->name_length);
                    }
                }
            } else {
                // 其他类型的符号冲突，检查是否是导入的符号
                fprintf(stderr, "[DEBUG] scope_builder: func '%.*s' other kind conflict, existing->source_module_path=%p\n",
                        (int)function_decl->name_length, function_decl->name, (void*)existing_sym->source_module_path);
                if (existing_sym->source_module_path != NULL) {
                    // 是导入的符号（source_module_path 不为空表示来自其他模块），静默跳过（不报错）
                    fprintf(stderr, "[DEBUG] scope_builder: skipping imported symbol '%.*s'\n",
                            (int)function_decl->name_length, function_decl->name);
                } else {
                    // 真正的符号冲突
                    fprintf(stderr, "[DEBUG] scope_builder: reporting duplicate symbol '%.*s'\n",
                            (int)function_decl->name_length, function_decl->name);
                    cn_support_diag_semantic_error_duplicate_symbol(
                        diagnostics, NULL, 0, 0, function_decl->name, function_decl->name_length);
                }
            }
        }

        cn_sem_build_function_scope(global_scope, function_decl, diagnostics);
    }

    return global_scope;
}

static void cn_sem_build_function_scope(CnSemScope *parent_scope,
                                        CnAstFunctionDecl *function_decl,
                                        CnDiagnostics *diagnostics)
{
    CnSemScope *function_scope;
    size_t i;

    if (!parent_scope || !function_decl) {
        return;
    }

    function_scope = cn_sem_scope_new(CN_SEM_SCOPE_FUNCTION, parent_scope);
    if (!function_scope) {
        return;
    }
    
    // 设置函数作用域的名称为函数名
    cn_sem_scope_set_name(function_scope, function_decl->name, function_decl->name_length);

    for (i = 0; i < function_decl->parameter_count; ++i) {
        CnAstParameter *param = &function_decl->parameters[i];

        CnSemSymbol *sym = cn_sem_scope_insert_symbol(function_scope,
                                   param->name,
                                   param->name_length,
                                   CN_SEM_SYMBOL_VARIABLE);
        if (sym) {
            sym->type = param->declared_type;
            sym->is_const = param->is_const;  // 传递常量参数标记
        } else {
            // 参数重复定义，检查是否是导入的符号
            CnSemSymbol *existing = cn_sem_scope_lookup_shallow(function_scope, param->name, param->name_length);
            if (existing && existing->decl_scope != function_scope) {
                // 是导入的符号，静默跳过（不报错）
            } else {
                // 真正的重复定义，报告错误
                cn_support_diag_semantic_error_duplicate_symbol(
                    diagnostics, NULL, 0, 0, param->name, param->name_length);
            }
        }
    }

    cn_sem_build_block_scope(function_scope, function_decl->body, diagnostics);
}

static void cn_sem_build_block_scope(CnSemScope *parent_scope,
                                     CnAstBlockStmt *block,
                                     CnDiagnostics *diagnostics)
{
    CnSemScope *block_scope;
    size_t i;

    if (!parent_scope || !block) {
        return;
    }

    block_scope = cn_sem_scope_new(CN_SEM_SCOPE_BLOCK, parent_scope);
    if (!block_scope) {
        return;
    }

    for (i = 0; i < block->stmt_count; ++i) {
        cn_sem_build_stmt(block_scope, block->stmts[i], diagnostics);
    }
}

static void cn_sem_build_stmt(CnSemScope *scope, CnAstStmt *stmt, CnDiagnostics *diagnostics)
{
    if (!scope || !stmt) {
        return;
    }

    switch (stmt->kind) {
    case CN_AST_STMT_BLOCK:
        cn_sem_build_block_scope(scope, stmt->as.block, diagnostics);
        break;
    case CN_AST_STMT_VAR_DECL: {
        CnAstVarDecl *var_decl = &stmt->as.var_decl;

        CnSemSymbol *sym = cn_sem_scope_insert_symbol(scope,
                                   var_decl->name,
                                   var_decl->name_length,
                                   CN_SEM_SYMBOL_VARIABLE);
        if (sym) {
            // 如果声明类型是结构体或枚举,需要从符号表查找真实的类型定义(含有正确的decl_scope)
            if (var_decl->declared_type && var_decl->declared_type->kind == CN_TYPE_STRUCT) {
                // 从符号表中查找类型定义（可能是结构体或枚举）
                CnSemSymbol *type_sym = cn_sem_scope_lookup(scope,
                                        var_decl->declared_type->as.struct_type.name,
                                        var_decl->declared_type->as.struct_type.name_length);
                if (type_sym && type_sym->type) {
                    // 根据符号类型使用对应的类型定义
                    if (type_sym->kind == CN_SEM_SYMBOL_STRUCT) {
                        // 结构体类型(含有正确的decl_scope)
                        sym->type = type_sym->type;
                    } else if (type_sym->kind == CN_SEM_SYMBOL_ENUM) {
                        // 枚举类型
                        sym->type = type_sym->type;
                    } else {
                        // 其他类型,使用原始类型
                        sym->type = var_decl->declared_type;
                    }
                } else {
                    // 找不到类型定义,使用原始类型
                    sym->type = var_decl->declared_type;
                }
            } else if (var_decl->declared_type && var_decl->declared_type->kind == CN_TYPE_POINTER &&
                       var_decl->declared_type->as.pointer_to &&
                       var_decl->declared_type->as.pointer_to->kind == CN_TYPE_STRUCT) {
                // 指向结构体的指针类型：需要查找结构体定义并更新指针指向的类型
                CnType *ptr_type = var_decl->declared_type;
                CnType *pointee_type = ptr_type->as.pointer_to;
                CnSemSymbol *type_sym = cn_sem_scope_lookup(scope,
                                        pointee_type->as.struct_type.name,
                                        pointee_type->as.struct_type.name_length);
                if (type_sym && type_sym->type &&
                    (type_sym->kind == CN_SEM_SYMBOL_STRUCT || type_sym->kind == CN_SEM_SYMBOL_ENUM)) {
                    // 创建新的指针类型，指向完整的结构体类型
                    sym->type = cn_type_new_pointer(type_sym->type);
                } else {
                    // 找不到类型定义,使用原始类型
                    sym->type = var_decl->declared_type;
                }
            } else if (var_decl->declared_type && var_decl->declared_type->kind == CN_TYPE_ARRAY &&
                       var_decl->declared_type->as.array.element_type &&
                       var_decl->declared_type->as.array.element_type->kind == CN_TYPE_STRUCT) {
                // 结构体数组类型：需要查找结构体定义并更新元素类型
                CnType *arr_type = var_decl->declared_type;
                CnType *elem_type = arr_type->as.array.element_type;
                CnSemSymbol *type_sym = cn_sem_scope_lookup(scope,
                                        elem_type->as.struct_type.name,
                                        elem_type->as.struct_type.name_length);
                if (type_sym && type_sym->type &&
                    (type_sym->kind == CN_SEM_SYMBOL_STRUCT || type_sym->kind == CN_SEM_SYMBOL_ENUM)) {
                    // 创建新的数组类型，元素类型为完整的结构体类型
                    sym->type = cn_type_new_array(type_sym->type, arr_type->as.array.length);
                } else {
                    // 找不到类型定义,使用原始类型
                    sym->type = var_decl->declared_type;
                }
            } else {
                // 非结构体类型,直接使用
                sym->type = var_decl->declared_type;
            }
            sym->is_const = var_decl->is_const;
            sym->is_static = var_decl->is_static;  // 传递静态变量标记
            // 设置可见性（根据 AST 中的可见性标志）
            if (var_decl->visibility == CN_VISIBILITY_PUBLIC) {
                sym->is_public = 1;  // 显式标记为公开
            } else if (var_decl->visibility == CN_VISIBILITY_PRIVATE) {
                sym->is_public = 0;  // 显式标记为私有
            }
            // CN_VISIBILITY_DEFAULT 保持默认值（已在 symbol_table.c 中设置为 0，即私有）
        } else {
            // 插入失败，检查是否是导入的符号
            CnSemSymbol *existing = cn_sem_scope_lookup_shallow(scope, var_decl->name, var_decl->name_length);
            if (existing && existing->source_module_path != NULL) {
                // 是导入的符号（source_module_path 不为空表示来自其他模块），静默跳过（不报错）
            } else {
                // 真正的重复定义，报告错误
                cn_support_diag_semantic_error_duplicate_symbol(
                    diagnostics, NULL, 0, 0, var_decl->name, var_decl->name_length);
            }
        }

        cn_sem_build_expr(scope, var_decl->initializer, diagnostics);
        break;
    }
    case CN_AST_STMT_EXPR:
        cn_sem_build_expr(scope, stmt->as.expr.expr, diagnostics);
        break;
    case CN_AST_STMT_RETURN:
        cn_sem_build_expr(scope, stmt->as.return_stmt.expr, diagnostics);
        break;
    case CN_AST_STMT_IF:
        cn_sem_build_if_stmt(scope, &stmt->as.if_stmt, diagnostics);
        break;
    case CN_AST_STMT_WHILE:
        cn_sem_build_while_stmt(scope, &stmt->as.while_stmt, diagnostics);
        break;
    case CN_AST_STMT_FOR:
        cn_sem_build_for_stmt(scope, &stmt->as.for_stmt, diagnostics);
        break;
    case CN_AST_STMT_SWITCH: {
        // 解析 switch 表达式
        cn_sem_build_expr(scope, stmt->as.switch_stmt.expr, diagnostics);
        // 解析每个 case/default 分支
        for (size_t i = 0; i < stmt->as.switch_stmt.case_count; i++) {
            CnAstSwitchCase *case_stmt = &stmt->as.switch_stmt.cases[i];
            // 解析 case 值表达式（如果有）
            cn_sem_build_expr(scope, case_stmt->value, diagnostics);
            // 解析 case 体
            cn_sem_build_block_scope(scope, case_stmt->body, diagnostics);
        }
        break;
    }
    case CN_AST_STMT_BREAK:
    case CN_AST_STMT_CONTINUE:
        break;
    case CN_AST_STMT_STRUCT_DECL: {
        // 局部结构体定义：注册到当前作用域
        CnAstStructDecl *struct_decl = &stmt->as.struct_decl;
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(scope,
                                   struct_decl->name,
                                   struct_decl->name_length,
                                   CN_SEM_SYMBOL_STRUCT);
        if (sym) {
            // 创建结构体类型，包含字段信息
            CnStructField *fields = NULL;
            if (struct_decl->field_count > 0) {
                fields = (CnStructField *)malloc(sizeof(CnStructField) * struct_decl->field_count);
                for (size_t j = 0; j < struct_decl->field_count; j++) {
                    fields[j].name = struct_decl->fields[j].name;
                    fields[j].name_length = struct_decl->fields[j].name_length;
                    fields[j].field_type = struct_decl->fields[j].field_type;
                    fields[j].is_const = struct_decl->fields[j].is_const;
                }
            }
            
            // 局部结构体应该绑定到函数作用域,向上查找直到函数作用域
            CnSemScope *decl_scope = scope;
            while (decl_scope && cn_sem_scope_get_kind(decl_scope) != CN_SEM_SCOPE_FUNCTION 
                   && cn_sem_scope_get_kind(decl_scope) != CN_SEM_SCOPE_GLOBAL
                   && cn_sem_scope_get_kind(decl_scope) != CN_SEM_SCOPE_MODULE) {
                decl_scope = cn_sem_scope_parent(decl_scope);
            }
            
            // 获取函数名(如果是局部结构体)
            const char *owner_func_name = NULL;
            size_t owner_func_name_length = 0;
            if (decl_scope && cn_sem_scope_get_kind(decl_scope) == CN_SEM_SCOPE_FUNCTION) {
                owner_func_name = cn_sem_scope_get_name(decl_scope, &owner_func_name_length);
            }
            
            sym->type = cn_type_new_struct(struct_decl->name,
                                          struct_decl->name_length,
                                          fields,
                                          struct_decl->field_count,
                                          decl_scope,
                                          owner_func_name,
                                          owner_func_name_length);
        } else {
            // 插入失败，检查是否是导入的符号
            CnSemSymbol *existing = cn_sem_scope_lookup_shallow(scope, struct_decl->name, struct_decl->name_length);
            if (existing && existing->kind == CN_SEM_SYMBOL_STRUCT && existing->source_module_path != NULL) {
                // 是导入的结构体（source_module_path 不为空表示来自其他模块），静默跳过（不报错）
            } else {
                // 真正的重复定义，报告错误
                cn_support_diag_semantic_error_duplicate_symbol(
                    diagnostics, NULL, 0, 0, struct_decl->name, struct_decl->name_length);
            }
        }
        break;
    }
    case CN_AST_STMT_ENUM_DECL: {
        // 局部枚举定义：注册到当前作用域
        CnAstEnumDecl *enum_decl = &stmt->as.enum_decl;
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(scope,
                                   enum_decl->name,
                                   enum_decl->name_length,
                                   CN_SEM_SYMBOL_ENUM);
        if (sym) {
            // 创建枚举类型
            sym->type = cn_type_new_enum(enum_decl->name, enum_decl->name_length);
            
            // 为枚举创建一个作用域来存储其成员
            CnSemScope *enum_scope = cn_sem_scope_new(CN_SEM_SCOPE_ENUM, scope);
            if (enum_scope && sym->type) {
                sym->type->as.enum_type.enum_scope = enum_scope;
                // 【关键修复】同时设置 sym->as.module_scope，以便模块导入时能正确复制枚举作用域
                sym->as.module_scope = enum_scope;
                
                // 注册枚举成员到枚举作用域和当前作用域
                // 这样可以直接通过成员名访问枚举成员（如：红、绿、蓝）
                for (size_t j = 0; j < enum_decl->member_count; j++) {
                    CnAstEnumMember *member = &enum_decl->members[j];
                    
                    // 先注册到枚举作用域
                    CnSemSymbol *member_sym = cn_sem_scope_insert_symbol(enum_scope,
                                                       member->name,
                                                       member->name_length,
                                                       CN_SEM_SYMBOL_ENUM_MEMBER);
                    if (member_sym) {
                        // 枚举成员的类型应该是枚举类型
                        member_sym->type = sym->type; // 使用枚举类型
                        // 保存枚举成员的值
                        member_sym->as.enum_value = member->value;
                    }
                    // 注意：枚举作用域中的重复定义不报错，因为可能是导入的
                    
                    // 同时注册到当前作用域（如果不存在同名符号）
                    CnSemSymbol *scope_member_sym = cn_sem_scope_insert_symbol(scope,
                                                       member->name,
                                                       member->name_length,
                                                       CN_SEM_SYMBOL_ENUM_MEMBER);
                    if (scope_member_sym) {
                        // 枚举成员的类型应该是枚举类型
                        scope_member_sym->type = sym->type; // 使用枚举类型
                        scope_member_sym->as.enum_value = member->value;
                    }
                    // 注意：当前作用域中的重复定义不报错，因为可能是导入的
                }
            }
        } else {
            // 插入失败，检查是否是导入的符号
            CnSemSymbol *existing = cn_sem_scope_lookup_shallow(scope, enum_decl->name, enum_decl->name_length);
            if (existing && existing->kind == CN_SEM_SYMBOL_ENUM && existing->source_module_path != NULL) {
                // 是导入的枚举（source_module_path 不为空表示来自其他模块），静默跳过（不报错）
            } else {
                // 真正的重复定义，报告错误
                cn_support_diag_semantic_error_duplicate_symbol(
                    diagnostics, NULL, 0, 0, enum_decl->name, enum_decl->name_length);
            }
        }
        break;
    }
    case CN_AST_STMT_IMPORT:
        // 导入语句在后续的名称解析阶段处理
        break;
    }
}

static void cn_sem_build_if_stmt(CnSemScope *scope, CnAstIfStmt *if_stmt, CnDiagnostics *diagnostics)
{
    if (!scope || !if_stmt) {
        return;
    }

    cn_sem_build_expr(scope, if_stmt->condition, diagnostics);
    cn_sem_build_block_scope(scope, if_stmt->then_block, diagnostics);
    cn_sem_build_block_scope(scope, if_stmt->else_block, diagnostics);
}

static void cn_sem_build_while_stmt(CnSemScope *scope, CnAstWhileStmt *while_stmt, CnDiagnostics *diagnostics)
{
    if (!scope || !while_stmt) {
        return;
    }

    cn_sem_build_expr(scope, while_stmt->condition, diagnostics);
    cn_sem_build_block_scope(scope, while_stmt->body, diagnostics);
}

static void cn_sem_build_for_stmt(CnSemScope *scope, CnAstForStmt *for_stmt, CnDiagnostics *diagnostics)
{
    CnSemScope *for_scope;

    if (!scope || !for_stmt) {
        return;
    }

    for_scope = cn_sem_scope_new(CN_SEM_SCOPE_BLOCK, scope);
    if (!for_scope) {
        return;
    }

    cn_sem_build_stmt(for_scope, for_stmt->init, diagnostics);
    cn_sem_build_expr(for_scope, for_stmt->condition, diagnostics);
    cn_sem_build_expr(for_scope, for_stmt->update, diagnostics);
    cn_sem_build_block_scope(for_scope, for_stmt->body, diagnostics);
}

static void cn_sem_build_expr(CnSemScope *scope, CnAstExpr *expr, CnDiagnostics *diagnostics)
{
    size_t i;

    if (!scope || !expr) {
        return;
    }

    switch (expr->kind) {
    case CN_AST_EXPR_BINARY:
        cn_sem_build_expr(scope, expr->as.binary.left, diagnostics);
        cn_sem_build_expr(scope, expr->as.binary.right, diagnostics);
        break;
    case CN_AST_EXPR_CALL:
        cn_sem_build_expr(scope, expr->as.call.callee, diagnostics);
        for (i = 0; i < expr->as.call.argument_count; ++i) {
            cn_sem_build_expr(scope, expr->as.call.arguments[i], diagnostics);
        }
        break;
    case CN_AST_EXPR_IDENTIFIER:
    case CN_AST_EXPR_INTEGER_LITERAL:
    case CN_AST_EXPR_STRING_LITERAL:
        break;
    case CN_AST_EXPR_ASSIGN:
        cn_sem_build_expr(scope, expr->as.assign.target, diagnostics);
        cn_sem_build_expr(scope, expr->as.assign.value, diagnostics);
        break;
    case CN_AST_EXPR_LOGICAL:
        cn_sem_build_expr(scope, expr->as.logical.left, diagnostics);
        cn_sem_build_expr(scope, expr->as.logical.right, diagnostics);
        break;
    case CN_AST_EXPR_UNARY:
        cn_sem_build_expr(scope, expr->as.unary.operand, diagnostics);
        break;
    case CN_AST_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expr->as.array_literal.element_count; ++i) {
            cn_sem_build_expr(scope, expr->as.array_literal.elements[i], diagnostics);
        }
        break;
    case CN_AST_EXPR_INDEX:
        cn_sem_build_expr(scope, expr->as.index.array, diagnostics);
        cn_sem_build_expr(scope, expr->as.index.index, diagnostics);
        break;
    case CN_AST_EXPR_MEMBER_ACCESS:
        cn_sem_build_expr(scope, expr->as.member.object, diagnostics);
        break;
    case CN_AST_EXPR_STRUCT_LITERAL:
        for (i = 0; i < expr->as.struct_lit.field_count; ++i) {
            cn_sem_build_expr(scope, expr->as.struct_lit.fields[i].value, diagnostics);
        }
        break;
    }
}

// ============================================================================
// 跨文件模块加载支持
// ============================================================================

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

// 读取文件内容
static char *read_file_content(const char *filename, size_t *out_size)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }
    
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(fp);
        return NULL;
    }
    
    char *content = (char *)malloc((size_t)file_size + 1);
    if (!content) {
        fclose(fp);
        return NULL;
    }
    
    size_t read_size = fread(content, 1, (size_t)file_size, fp);
    fclose(fp);
    
    content[read_size] = '\0';
    if (out_size) {
        *out_size = read_size;
    }
    
    return content;
}

// 编译外部模块文件，返回其作用域
// 注意：为了支持嵌套导入，需要传入loader和source_file
static CnSemScope *compile_external_module_recursive(const char *file_path,
                                                     CnDiagnostics *diagnostics,
                                                     CnSemScope *global_scope,
                                                     CnModuleLoader *loader,
                                                     const char *importing_file);

static CnSemScope *compile_external_module(const char *file_path,
                                            CnDiagnostics *diagnostics,
                                            CnSemScope *global_scope)
{
    // 调用递归版本，但不支持嵌套导入（loader=NULL）
    return compile_external_module_recursive(file_path, diagnostics, global_scope, NULL, NULL);
}

static CnSemScope *compile_external_module_recursive(const char *file_path,
                                                     CnDiagnostics *diagnostics,
                                                     CnSemScope *global_scope,
                                                     CnModuleLoader *loader,
                                                     const char *importing_file)
{
    // 检查缓存
    char *normalized_path = normalize_file_path(file_path);
    const char *cache_key = normalized_path ? normalized_path : file_path;
    
    fprintf(stderr, "[DEBUG] compile_external_module_recursive: file_path=%s, normalized=%s\n",
            file_path, cache_key);
    
    CnSemScope *cached = find_cached_module(file_path);
    if (cached) {
        fprintf(stderr, "[DEBUG] 模块已缓存，直接返回: %s\n", cache_key);
        // 【调试】检查缓存的作用域中的符号是否有正确的 type 字段和 module_scope
        CnSemSymbolNode *debug_node = cached->symbols;
        while (debug_node) {
            fprintf(stderr, "[DEBUG] Cached symbol '%.*s', kind=%d, type=%p, module_scope=%p\n",
                    (int)debug_node->symbol.name_length, debug_node->symbol.name,
                    debug_node->symbol.kind, (void*)debug_node->symbol.type,
                    (void*)debug_node->symbol.as.module_scope);
            debug_node = debug_node->next;
        }
        if (normalized_path) free(normalized_path);
        return cached;  // 返回缓存的作用域
    }
    
    // 检测循环导入
    if (is_module_compiling(file_path)) {
        cn_support_diag_semantic_error_generic(
            diagnostics,
            CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
            NULL, 0, 0,
            "语义错误：检测到循环导入");
        return NULL;
    }
    
    // 压入编译栈
    if (!push_compiling_module(file_path)) {
        cn_support_diag_semantic_error_generic(
            diagnostics,
            CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
            NULL, 0, 0,
            "语义错误：模块导入嵌套层级太深");
        return NULL;
    }
    
    // 读取文件内容
    size_t file_size = 0;
    char *source = read_file_content(file_path, &file_size);
    if (!source) {
        pop_compiling_module();
        return NULL;
    }
    
    // 预处理
    CnPreprocessor preprocessor;
    cn_frontend_preprocessor_init(&preprocessor, source, file_size, file_path);
    
    if (!cn_frontend_preprocessor_process(&preprocessor)) {
        cn_frontend_preprocessor_free(&preprocessor);
        free(source);
        pop_compiling_module();
        return NULL;
    }
    
    // 词法分析
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, preprocessor.output, preprocessor.output_length, file_path);
    
    // 语法分析
    CnParser *parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        cn_frontend_preprocessor_free(&preprocessor);
        free(source);
        pop_compiling_module();
        return NULL;
    }
    
    CnAstProgram *module_program = NULL;
    int ok = cn_frontend_parse_program(parser, &module_program);
    
    if (!ok || !module_program) {
        cn_frontend_parser_free(parser);
        cn_frontend_preprocessor_free(&preprocessor);
        free(source);
        pop_compiling_module();
        return NULL;
    }
    
    // 为外部模块创建作用域
    CnSemScope *module_scope = cn_sem_scope_new(CN_SEM_SCOPE_FILE_MODULE, global_scope);
    if (!module_scope) {
        cn_frontend_ast_program_free(module_program);
        cn_frontend_parser_free(parser);
        cn_frontend_preprocessor_free(&preprocessor);
        free(source);
        pop_compiling_module();
        return NULL;
    }
    
    // =============================================================================
    // 重要：先处理导入语句，再注册结构体
    // 这样被导入模块中的类型才能在结构体字段类型解析时被找到
    // =============================================================================
    
    // 如果提供了loader,则处理模块内部的导入语句（支持嵌套导入）
    if (loader && importing_file && module_program->import_count > 0) {
        for (size_t i = 0; i < module_program->import_count; ++i) {
            CnAstStmt *import_stmt = module_program->imports[i];
            if (!import_stmt || import_stmt->kind != CN_AST_STMT_IMPORT) {
                continue;
            }
            
            CnAstImportStmt *import = &import_stmt->as.import_stmt;
            
            // 只处理路径导入（module_path）
            if (import->module_path && import->module_path->is_relative) {
                // 使用相对路径加载器（以当前模块文件为基准）
                // 根据 target_type 决定查找包还是模块
                CnModuleMetadata *metadata = cn_module_loader_load_relative_typed(
                    loader, file_path, import->module_path, import->target_type);
                
                if (metadata && metadata->file_path) {
                    // 递归加载外部模块
                    CnSemScope *nested_scope = compile_external_module_recursive(
                        metadata->file_path, diagnostics, module_scope, loader, file_path);
                    
                    if (nested_scope) {
                        if (import->use_from_syntax) {
                            // 「从 ... 导入」语法：选择性导入成员
                            if (import->is_wildcard) {
                                // 通配符导入
                                CnSemSymbolNode *node = nested_scope->symbols;
                                while (node) {
                                    CnSemSymbol *sym = &node->symbol;
                                    if (sym->is_public) {
                                        CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(
                                            module_scope, sym->name, sym->name_length, sym->kind);
                                        if (new_sym) {
                                            // 【关键修复】使用辅助函数复制类型信息（处理枚举类型为 NULL 的特殊情况）
                                            cn_sem_copy_symbol_type(sym, new_sym);
                                            new_sym->is_public = sym->is_public;
                                            new_sym->is_const = sym->is_const;
                                            // 【关键修复】保留原始 decl_scope 以便区分导入符号
                                            new_sym->decl_scope = sym->decl_scope;
                                            // 复制源模块路径（关键：用于跨编译会话的符号唯一性判断）
                                            new_sym->source_module_path = sym->source_module_path;
                                            new_sym->source_module_path_length = sym->source_module_path_length;
                                            // 复制 module_scope 以支持递归处理嵌套导入
                                            if (sym->kind == CN_SEM_SYMBOL_MODULE ||
                                                sym->kind == CN_SEM_SYMBOL_STRUCT ||
                                                sym->kind == CN_SEM_SYMBOL_ENUM) {
                                                new_sym->as.module_scope = sym->as.module_scope;
                                            }
                                        }
                                    }
                                    node = node->next;
                                }
                            } else if (import->member_count > 0) {
                                // 选择性导入
                                for (size_t j = 0; j < import->member_count; ++j) {
                                    const char *member_name = import->members[j].name;
                                    size_t member_name_length = import->members[j].name_length;
                                    
                                    CnSemSymbol *member_sym = cn_sem_scope_lookup_shallow(
                                        nested_scope, member_name, member_name_length);
                                    
                                    if (member_sym && member_sym->is_public) {
                                        CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(
                                            module_scope, member_name, member_name_length, member_sym->kind);
                                        if (new_sym) {
                                            // 【关键修复】使用辅助函数复制类型信息（处理枚举类型为 NULL 的特殊情况）
                                            cn_sem_copy_symbol_type(member_sym, new_sym);
                                            new_sym->is_public = member_sym->is_public;
                                            new_sym->is_const = member_sym->is_const;
                                            // 【关键修复】保留原始 decl_scope 以便区分导入符号
                                            new_sym->decl_scope = member_sym->decl_scope;
                                            // 复制源模块路径（关键：用于跨编译会话的符号唯一性判断）
                                            new_sym->source_module_path = member_sym->source_module_path;
                                            new_sym->source_module_path_length = member_sym->source_module_path_length;
                                            // 复制 module_scope 以支持递归处理嵌套导入
                                            if (member_sym->kind == CN_SEM_SYMBOL_MODULE ||
                                                member_sym->kind == CN_SEM_SYMBOL_STRUCT ||
                                                member_sym->kind == CN_SEM_SYMBOL_ENUM) {
                                                new_sym->as.module_scope = member_sym->as.module_scope;
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            // 纯「导入 路径」语法：全量导入模块的所有公开成员
                            // 首先创建一个模块符号，用于保存整个模块的作用域
                            if (import->module_path && import->module_path->segment_count > 0) {
                                const char *module_name = import->module_path->segments[import->module_path->segment_count - 1].name;
                                size_t module_name_len = import->module_path->segments[import->module_path->segment_count - 1].name_length;
                                
                                CnSemSymbol *module_sym = cn_sem_scope_insert_symbol(
                                    module_scope, module_name, module_name_len, CN_SEM_SYMBOL_MODULE);
                                if (module_sym) {
                                    module_sym->is_public = 1;
                                    module_sym->as.module_scope = nested_scope;
                                }
                            }
                            
                            // 然后导入所有公开成员
                            CnSemSymbolNode *node = nested_scope->symbols;
                            while (node) {
                                CnSemSymbol *sym = &node->symbol;
                                if (sym->is_public) {
                                        // 检查是否已存在同名符号
                                        CnSemSymbol *existing = cn_sem_scope_lookup_shallow(module_scope,
                                                sym->name, sym->name_length);
                                        if (existing) {
                                            // 【关键修复】检查是否是同一个符号（来自同一模块）
                                            // 如果是同一个符号，静默跳过，不报错
                                            if (cn_sem_is_same_symbol(existing, sym)) {
                                                node = node->next;
                                                continue;
                                            }
                                            // 特殊处理：如果新符号是结构体/枚举类型，而现有符号是枚举成员
                                            // 则用结构体/枚举类型替换枚举成员（结构体定义更重要）
                                            // 【关键修复】只有当新符号有有效的 type 时才进行替换
                                            if ((sym->kind == CN_SEM_SYMBOL_STRUCT || sym->kind == CN_SEM_SYMBOL_ENUM) &&
                                                existing->kind == CN_SEM_SYMBOL_ENUM_MEMBER &&
                                                sym->type != NULL) {
                                                existing->kind = sym->kind;
                                                existing->type = sym->type;
                                                existing->as.module_scope = sym->as.module_scope;
                                            }
                                            // 【关键修复】如果现有符号是模块符号，而新符号是结构体/枚举
                                            // 不替换，而是插入新符号（共存）
                                            // 模块符号用于点访问语法，结构体/枚举符号用于类型上下文
                                            if (existing->kind == CN_SEM_SYMBOL_MODULE &&
                                                (sym->kind == CN_SEM_SYMBOL_STRUCT || sym->kind == CN_SEM_SYMBOL_ENUM)) {
                                                // 检查是否已存在同名的结构体/枚举符号（在当前作用域的符号链表中查找）
                                                bool type_exists = false;
                                                CnSemSymbolNode *check_node = module_scope->symbols;
                                                while (check_node) {
                                                    // 直接比较符号名称和类型
                                                    if (check_node->symbol.name_length == sym->name_length &&
                                                        check_node->symbol.name != NULL && sym->name != NULL &&
                                                        memcmp(check_node->symbol.name, sym->name, sym->name_length) == 0 &&
                                                        check_node->symbol.kind == sym->kind) {
                                                        type_exists = true;
                                                        break;
                                                    }
                                                    check_node = check_node->next;
                                                }
                                                
                                                if (type_exists) {
                                                    // 已存在同名的结构体/枚举符号，跳过
                                                    fprintf(stderr, "[DEBUG] Type symbol '%.*s' already exists, skipping\n",
                                                            (int)sym->name_length, sym->name);
                                                    node = node->next;
                                                    continue;
                                                }
                                                
                                                // 插入新的结构体/枚举符号
                                                CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(
                                                    module_scope, sym->name, sym->name_length, sym->kind);
                                                if (new_sym) {
                                                    cn_sem_copy_symbol_type(sym, new_sym);
                                                    new_sym->is_public = sym->is_public;
                                                    new_sym->is_const = sym->is_const;
                                                    new_sym->decl_scope = sym->decl_scope;
                                                    new_sym->source_module_path = sym->source_module_path;
                                                    new_sym->source_module_path_length = sym->source_module_path_length;
                                                    if (sym->as.module_scope) {
                                                        new_sym->as.module_scope = sym->as.module_scope;
                                                    }
                                                    fprintf(stderr, "[DEBUG] Inserted type symbol '%.*s' (kind=%d) alongside module symbol\n",
                                                            (int)sym->name_length, sym->name, sym->kind);
                                                }
                                                node = node->next;
                                                continue;
                                            }
                                            node = node->next;
                                            continue;
                                        }
                                    
                                    CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(
                                        module_scope, sym->name, sym->name_length, sym->kind);
                                    if (new_sym) {
                                        // 【关键修复】使用辅助函数复制类型信息（处理枚举类型为 NULL 的特殊情况）
                                        cn_sem_copy_symbol_type(sym, new_sym);
                                        new_sym->is_public = sym->is_public;
                                        new_sym->is_const = sym->is_const;
                                        // 【关键修复】保留原始 decl_scope 以便区分导入符号
                                        // 这样 cn_sem_is_same_symbol 可以正确识别来自同一模块的符号
                                        new_sym->decl_scope = sym->decl_scope;
                                        // 复制源模块路径（关键：用于跨编译会话的符号唯一性判断）
                                        new_sym->source_module_path = sym->source_module_path;
                                        new_sym->source_module_path_length = sym->source_module_path_length;
                                        // 复制 module_scope 以支持递归处理嵌套导入
                                        if (sym->kind == CN_SEM_SYMBOL_MODULE ||
                                            sym->kind == CN_SEM_SYMBOL_STRUCT ||
                                            sym->kind == CN_SEM_SYMBOL_ENUM) {
                                            new_sym->as.module_scope = sym->as.module_scope;
                                        }
                                    }
                                }
                                node = node->next;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 注册模块中的函数到模块作用域
    for (size_t i = 0; i < module_program->function_count; ++i) {
        CnAstFunctionDecl *function_decl = module_program->functions[i];
        if (!function_decl) {
            continue;
        }
        
        // 检查是否已存在同名符号
        CnSemSymbol *existing = cn_sem_scope_lookup_shallow(module_scope,
                                                function_decl->name,
                                                function_decl->name_length);
        if (existing) {
            // 如果已存在同名符号，跳过注册
            continue;
        }
        
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(module_scope,
                                   function_decl->name,
                                   function_decl->name_length,
                                   CN_SEM_SYMBOL_FUNCTION);
        if (sym) {
            // 设置源模块路径（关键：用于跨编译会话的符号唯一性判断）
            sym->source_module_path = cache_key;
            sym->source_module_path_length = strlen(cache_key);
            CnType **param_types = NULL;
            if (function_decl->parameter_count > 0) {
                param_types = (CnType **)malloc(sizeof(CnType *) * function_decl->parameter_count);
                for (size_t j = 0; j < function_decl->parameter_count; j++) {
                    CnType *param_type = function_decl->parameters[j].declared_type;
                    // 特殊处理：如果参数类型是结构体类型，可能是枚举类型或类类型
                    // 需要从符号表查找真实类型
                    if (param_type && param_type->kind == CN_TYPE_STRUCT) {
                        CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                                param_type->as.struct_type.name,
                                                param_type->as.struct_type.name_length);
                        if (type_sym && type_sym->type) {
                            if (type_sym->kind == CN_SEM_SYMBOL_ENUM) {
                                param_type = type_sym->type;
                            } else if (type_sym->kind == CN_SEM_SYMBOL_STRUCT) {
                                param_type = type_sym->type;
                            }
                        }
                    }
                    param_types[j] = param_type;
                }
            }
            // 使用函数声明中的返回类型,如果没有则使用VOID
            CnType *return_type = function_decl->return_type;
            if (!return_type) {
                return_type = cn_type_new_primitive(CN_TYPE_VOID);
            }
            sym->type = cn_type_new_function(return_type,
                                            param_types,
                                            function_decl->parameter_count);
            // 根据AST中的visibility字段设置可见性
            sym->is_public = (function_decl->visibility == CN_VISIBILITY_PUBLIC) ? 1 : 0;
        }
    }
    
    // 注册模块中的全局变量
    for (size_t i = 0; i < module_program->global_var_count; ++i) {
        CnAstStmt *var_stmt = module_program->global_vars[i];
        if (!var_stmt || var_stmt->kind != CN_AST_STMT_VAR_DECL) {
            continue;
        }
        
        CnAstVarDecl *var_decl = &var_stmt->as.var_decl;
        
        // 检查是否已存在同名符号
        CnSemSymbol *existing = cn_sem_scope_lookup_shallow(module_scope,
                                                var_decl->name,
                                                var_decl->name_length);
        if (existing) {
            // 如果已存在同名符号，跳过注册
            continue;
        }
        
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(module_scope,
                                   var_decl->name,
                                   var_decl->name_length,
                                   CN_SEM_SYMBOL_VARIABLE);
        if (sym) {
            // 设置源模块路径（关键：用于跨编译会话的符号唯一性判断）
            sym->source_module_path = cache_key;
            sym->source_module_path_length = strlen(cache_key);
            sym->type = var_decl->declared_type;
            sym->is_const = var_decl->is_const;
            // 根据AST中的visibility字段设置可见性
            sym->is_public = (var_decl->visibility == CN_VISIBILITY_PUBLIC) ? 1 : 0;
            
            // 如果没有显式类型，从初始化表达式推断类型
            if (!sym->type && var_decl->initializer) {
                // 增强类型推断：支持更多表达式类型
                CnAstExpr *init = var_decl->initializer;
                
                switch (init->kind) {
                    case CN_AST_EXPR_STRING_LITERAL:
                        sym->type = cn_type_new_primitive(CN_TYPE_STRING);
                        break;
                    case CN_AST_EXPR_INTEGER_LITERAL:
                        sym->type = cn_type_new_primitive(CN_TYPE_INT);
                        break;
                    case CN_AST_EXPR_FLOAT_LITERAL:
                        sym->type = cn_type_new_primitive(CN_TYPE_FLOAT);
                        break;
                    case CN_AST_EXPR_BOOL_LITERAL:
                        sym->type = cn_type_new_primitive(CN_TYPE_BOOL);
                        break;
                    case CN_AST_EXPR_CHAR_LITERAL:
                        sym->type = cn_type_new_primitive(CN_TYPE_CHAR);
                        break;
                    case CN_AST_EXPR_CAST:
                        // 强制类型转换表达式：直接使用目标类型
                        // 例如：变量 符号指针 = (符号*)分配内存(...)
                        if (init->as.cast.target_type) {
                            sym->type = init->as.cast.target_type;
                        }
                        break;
                    case CN_AST_EXPR_CALL:
                        // 函数调用表达式：查找函数返回类型
                        // 例如：变量 缓冲区 = 创建输出缓冲区(256)
                        if (init->as.call.callee &&
                            init->as.call.callee->kind == CN_AST_EXPR_IDENTIFIER) {
                            const char *func_name = init->as.call.callee->as.identifier.name;
                            size_t func_name_len = init->as.call.callee->as.identifier.name_length;
                            CnSemSymbol *func_sym = cn_sem_scope_lookup(module_scope, func_name, func_name_len);
                            if (func_sym && func_sym->type &&
                                func_sym->type->kind == CN_TYPE_FUNCTION) {
                                sym->type = func_sym->type->as.function.return_type;
                            }
                        }
                        break;
                    case CN_AST_EXPR_MEMBER_ACCESS:
                        // 成员访问表达式：从结构体/类定义获取成员类型
                        // 例如：变量 值 = 对象.成员
                        if (init->as.member.object &&
                            init->as.member.object->kind == CN_AST_EXPR_IDENTIFIER) {
                            const char *obj_name = init->as.member.object->as.identifier.name;
                            size_t obj_name_len = init->as.member.object->as.identifier.name_length;
                            CnSemSymbol *obj_sym = cn_sem_scope_lookup(module_scope, obj_name, obj_name_len);
                            if (obj_sym && obj_sym->type &&
                                obj_sym->type->kind == CN_TYPE_STRUCT) {
                                // 在结构体中查找成员
                                CnStructField *field = cn_type_struct_find_field(
                                    obj_sym->type,
                                    init->as.member.member_name,
                                    init->as.member.member_name_length);
                                if (field) {
                                    sym->type = field->field_type;
                                }
                            }
                        }
                        break;
                    case CN_AST_EXPR_IDENTIFIER:
                        // 标识符引用：从符号表查找变量类型
                        // 例如：变量 新变量 = 已有变量
                        {
                            const char *ident_name = init->as.identifier.name;
                            size_t ident_name_len = init->as.identifier.name_length;
                            CnSemSymbol *ident_sym = cn_sem_scope_lookup(module_scope, ident_name, ident_name_len);
                            if (ident_sym) {
                                sym->type = ident_sym->type;
                            }
                        }
                        break;
                    default:
                        // 其他表达式类型暂不处理
                        break;
                }
            }
        }
    }
    
    // 注册模块中的枚举声明
    for (size_t i = 0; i < module_program->enum_count; ++i) {
        CnAstStmt *enum_stmt = module_program->enums[i];
        if (!enum_stmt || enum_stmt->kind != CN_AST_STMT_ENUM_DECL) {
            continue;
        }
        
        CnAstEnumDecl *enum_decl = &enum_stmt->as.enum_decl;
        
        // 检查是否已存在同名符号
        CnSemSymbol *existing = cn_sem_scope_lookup_shallow(module_scope,
                                                enum_decl->name,
                                                enum_decl->name_length);
        if (existing) {
            // 如果已存在同名符号，跳过注册
            continue;
        }
        
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(module_scope,
                                   enum_decl->name,
                                   enum_decl->name_length,
                                   CN_SEM_SYMBOL_ENUM);
        if (sym) {
            // 设置源模块路径（关键：用于跨编译会话的符号唯一性判断）
            sym->source_module_path = cache_key;
            sym->source_module_path_length = strlen(cache_key);
            // 创建枚举类型
            sym->type = cn_type_new_enum(enum_decl->name, enum_decl->name_length);
            
            // 为枚举创建一个作用域来存储其成员
            CnSemScope *enum_scope = cn_sem_scope_new(CN_SEM_SCOPE_ENUM, module_scope);
            if (enum_scope && sym->type) {
                sym->type->as.enum_type.enum_scope = enum_scope;
                // 【关键修复】同时设置 sym->as.module_scope，以便模块导入时能正确复制枚举作用域
                sym->as.module_scope = enum_scope;
                fprintf(stderr, "[DEBUG] Registered enum '%.*s' with type=%p, module_scope=%p\n",
                        (int)enum_decl->name_length, enum_decl->name, (void*)sym->type, (void*)sym->as.module_scope);
                
                // 注册枚举成员到枚举作用域和模块作用域
                // 这样可以直接通过成员名访问枚举成员（如：关键字_如果、标识符等）
                for (size_t j = 0; j < enum_decl->member_count; j++) {
                    CnAstEnumMember *member = &enum_decl->members[j];
                    
                    // 先注册到枚举作用域
                    CnSemSymbol *member_sym = cn_sem_scope_insert_symbol(enum_scope,
                                                       member->name,
                                                       member->name_length,
                                                       CN_SEM_SYMBOL_ENUM_MEMBER);
                    if (member_sym) {
                        // 枚举成员的类型应该是枚举类型
                        member_sym->type = sym->type; // 使用枚举类型
                        member_sym->as.enum_value = member->value;
                        // 枚举成员继承枚举的可见性
                        member_sym->is_public = 1;  // 枚举成员默认公开
                        // 设置源模块路径
                        member_sym->source_module_path = cache_key;
                        member_sym->source_module_path_length = strlen(cache_key);
                    }
                    
                    // 同时注册到模块作用域（如果不存在同名符号）
                    CnSemSymbol *module_member_sym = cn_sem_scope_insert_symbol(module_scope,
                                                       member->name,
                                                       member->name_length,
                                                       CN_SEM_SYMBOL_ENUM_MEMBER);
                    if (module_member_sym) {
                        // 枚举成员的类型应该是枚举类型
                        module_member_sym->type = sym->type; // 使用枚举类型
                        module_member_sym->as.enum_value = member->value;
                        module_member_sym->is_public = 1;  // 枚举成员默认公开
                        // 设置源模块路径
                        module_member_sym->source_module_path = cache_key;
                        module_member_sym->source_module_path_length = strlen(cache_key);
                    }
                }
            }
            
            // 枚举在"公开:"块中声明时设置为公开
            // 由于CN语言使用"公开:"块声明，所有在公开块后的枚举都应该是公开的
            // 这里暂时默认设置为公开（后续可以通过AST扩展支持可见性）
            sym->is_public = 1;  // 枚举类型默认公开
        }
    }
    
    // =============================================================================
    // 两阶段结构体类型解析
    // =============================================================================
    // 问题：结构体可能包含自引用指针（如 struct A { A* next; }）或相互引用
    // 解决方案：
    //   第一阶段：注册所有结构体名称到符号表（前向声明），创建不完整类型
    //   第二阶段：遍历所有结构体，解析字段类型并构建完整类型
    // =============================================================================
    
    // 第一阶段：注册所有结构体名称（前向声明）
    for (size_t i = 0; i < module_program->struct_count; ++i) {
        CnAstStmt *struct_stmt = module_program->structs[i];
        if (!struct_stmt || struct_stmt->kind != CN_AST_STMT_STRUCT_DECL) {
            continue;
        }
        
        CnAstStructDecl *struct_decl = &struct_stmt->as.struct_decl;
        
        // 检查是否已存在同名符号
        CnSemSymbol *existing = cn_sem_scope_lookup(module_scope,
                                    struct_decl->name,
                                    struct_decl->name_length);
        if (existing) {
            // 如果已存在的是结构体，跳过
            if (existing->kind == CN_SEM_SYMBOL_STRUCT) {
                continue;
            }
            // 【关键修复】如果已存在的是模块符号，不跳过结构体注册
            // 模块符号和结构体符号可以共存：
            // - 模块符号用于点访问语法（如 `词元.词元类型枚举`）
            // - 结构体符号用于类型上下文（如 `词元 变量名`）
            // 符号插入函数 cn_sem_scope_insert_symbol 已修改为允许这种共存
            if (existing->kind == CN_SEM_SYMBOL_MODULE) {
                fprintf(stderr, "[DEBUG] Module symbol '%.*s' exists, registering struct symbol alongside\n",
                        (int)struct_decl->name_length, struct_decl->name);
                // 不跳过，继续注册结构体符号
            }
            // 如果已存在的是枚举成员，用结构体替换它
            if (existing->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                // 创建不完整的结构体类型（只有名称，没有字段）
                CnType *incomplete_type = cn_type_new_struct(struct_decl->name,
                                                              struct_decl->name_length,
                                                              NULL, 0,  // 暂时没有字段
                                                              module_scope,
                                                              NULL, 0);
                // 更新现有符号
                existing->kind = CN_SEM_SYMBOL_STRUCT;
                existing->type = incomplete_type;
                existing->is_public = 1;
                existing->as.module_scope = NULL;
                continue;
            }
            // 如果已存在的是模块符号，继续执行（不跳过）
            // 其他类型的符号冲突，跳过
            if (existing->kind != CN_SEM_SYMBOL_MODULE) {
                continue;
            }
        }
        
        // 创建不完整的结构体类型（只有名称，没有字段）
        CnType *incomplete_type = cn_type_new_struct(struct_decl->name,
                                                      struct_decl->name_length,
                                                      NULL, 0,  // 暂时没有字段
                                                      module_scope,
                                                      NULL, 0);
        
        // 注册到符号表
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(module_scope,
                                   struct_decl->name,
                                   struct_decl->name_length,
                                   CN_SEM_SYMBOL_STRUCT);
        if (sym) {
            sym->type = incomplete_type;
            sym->is_public = 1;  // 结构体类型默认公开
            // 设置源模块路径（关键：用于跨编译会话的符号唯一性判断）
            sym->source_module_path = cache_key;
            sym->source_module_path_length = strlen(cache_key);
            // 【调试日志】验证结构体符号的 type 字段
            fprintf(stderr, "[DEBUG] Registered struct '%.*s' with type=%p\n",
                    (int)sym->name_length, sym->name, (void*)sym->type);
        }
    }
    
    // 第二阶段：构建完整的结构体类型（解析字段类型）
    for (size_t i = 0; i < module_program->struct_count; ++i) {
        CnAstStmt *struct_stmt = module_program->structs[i];
        if (!struct_stmt || struct_stmt->kind != CN_AST_STMT_STRUCT_DECL) {
            continue;
        }
        
        CnAstStructDecl *struct_decl = &struct_stmt->as.struct_decl;
        
        // 查找已注册的符号
        CnSemSymbol *sym = cn_sem_scope_lookup(module_scope,
                                struct_decl->name,
                                struct_decl->name_length);
        if (!sym || sym->kind != CN_SEM_SYMBOL_STRUCT || !sym->type) {
            continue;
        }
        
        // 构建结构体字段
        CnStructField *fields = NULL;
        if (struct_decl->field_count > 0) {
            fields = (CnStructField *)malloc(sizeof(CnStructField) * struct_decl->field_count);
            for (size_t j = 0; j < struct_decl->field_count; j++) {
                fields[j].name = struct_decl->fields[j].name;
                fields[j].name_length = struct_decl->fields[j].name_length;
                
                // 解析字段类型：如果是自定义类型（结构体类型表示），从符号表查找真实类型
                CnType *field_type = struct_decl->fields[j].field_type;
                if (field_type && field_type->kind == CN_TYPE_STRUCT && field_type->as.struct_type.name) {
                    // 先在模块作用域查找
                    CnSemSymbol *type_sym = cn_sem_scope_lookup(module_scope,
                                                field_type->as.struct_type.name,
                                                field_type->as.struct_type.name_length);
                    if (type_sym && type_sym->type) {
                        // 可能是枚举类型或结构体类型
                        if (type_sym->kind == CN_SEM_SYMBOL_ENUM || type_sym->kind == CN_SEM_SYMBOL_STRUCT) {
                            field_type = type_sym->type;
                        }
                    }
                }
                // 处理指针类型：如果字段是指针指向自定义类型
                else if (field_type && field_type->kind == CN_TYPE_POINTER &&
                         field_type->as.pointer_to &&
                         field_type->as.pointer_to->kind == CN_TYPE_STRUCT &&
                         field_type->as.pointer_to->as.struct_type.name) {
                    CnSemSymbol *type_sym = cn_sem_scope_lookup(module_scope,
                                                field_type->as.pointer_to->as.struct_type.name,
                                                field_type->as.pointer_to->as.struct_type.name_length);
                    if (type_sym && type_sym->type) {
                        // 找到了完整类型，创建新的指针类型
                        field_type = cn_type_new_pointer(type_sym->type);
                    } else if (type_sym && !type_sym->type) {
                        // 符号存在但类型不完整 - 这不应该发生，因为我们已经完成了第一阶段
                        // 但为了安全，保留原来的不完整指针类型
                    }
                    // 如果 type_sym 不存在，保留原来的不完整指针类型
                }
                fields[j].field_type = field_type;
                fields[j].is_const = struct_decl->fields[j].is_const;
            }
        }
        
        // 更新结构体类型（添加字段信息）
        // 【修复】添加空指针检查，防止 sym->type 为 NULL 时崩溃
        if (sym->type && sym->type->kind == CN_TYPE_STRUCT) {
            sym->type->as.struct_type.fields = fields;
            sym->type->as.struct_type.field_count = struct_decl->field_count;
        }
    }
    
    // =============================================================================
    // 延迟解析：更新所有不完整的结构体字段类型
    // 解决结构体指针前向引用问题：当结构体A引用结构体B（B*），而B在A之后定义时，
    // 字段类型会指向一个不完整的结构体类型（没有字段信息）
    // 这里遍历所有结构体，更新这些不完整的字段类型
    // =============================================================================
    {
        CnSemSymbolNode *node = module_scope->symbols;
        while (node) {
            CnSemSymbol *sym = &node->symbol;
            if (sym->kind == CN_SEM_SYMBOL_STRUCT && sym->type &&
                sym->type->kind == CN_TYPE_STRUCT &&
                sym->type->as.struct_type.fields) {
                // 遍历结构体的所有字段
                for (size_t j = 0; j < sym->type->as.struct_type.field_count; j++) {
                    CnStructField *field = &sym->type->as.struct_type.fields[j];
                    CnType *field_type = field->field_type;
                    
                    // 情况1：字段类型是结构体但没有字段信息（前向引用）
                    if (field_type && field_type->kind == CN_TYPE_STRUCT &&
                        !field_type->as.struct_type.fields &&
                        field_type->as.struct_type.name) {
                        CnSemSymbol *type_sym = cn_sem_scope_lookup(module_scope,
                                                    field_type->as.struct_type.name,
                                                    field_type->as.struct_type.name_length);
                        if (type_sym && type_sym->type &&
                            type_sym->kind == CN_SEM_SYMBOL_STRUCT &&
                            type_sym->type->as.struct_type.fields) {
                            field->field_type = type_sym->type;
                        }
                    }
                    // 情况2：字段类型是指针，指向的结构体没有字段信息
                    else if (field_type && field_type->kind == CN_TYPE_POINTER &&
                             field_type->as.pointer_to &&
                             field_type->as.pointer_to->kind == CN_TYPE_STRUCT &&
                             !field_type->as.pointer_to->as.struct_type.fields &&
                             field_type->as.pointer_to->as.struct_type.name) {
                        CnSemSymbol *type_sym = cn_sem_scope_lookup(module_scope,
                                                    field_type->as.pointer_to->as.struct_type.name,
                                                    field_type->as.pointer_to->as.struct_type.name_length);
                        if (type_sym && type_sym->type &&
                            type_sym->kind == CN_SEM_SYMBOL_STRUCT &&
                            type_sym->type->as.struct_type.fields) {
                            field->field_type = cn_type_new_pointer(type_sym->type);
                        }
                    }
                }
            }
            node = node->next;
        }
    }
    
    // 清理（注意：符号表需要保持，不能释放 program）
    cn_frontend_parser_free(parser);
    // 注意：不能释放 preprocessor，因为 lexer 中的 token 指向 preprocessor.output！
    // cn_frontend_preprocessor_free(&preprocessor);  // 不释放，避免悬空指针
    // 注意：不能释放 source，因为 preprocessor 可能引用 source
    // free(source);  // 不释放，避免悬空指针
    // 注意：module_program 也不能释放，因为符号可能引用 AST 节点
    
    // 弹出编译栈
    pop_compiling_module();
    
    // 缓存模块作用域和AST（用于后续代码生成）
    fprintf(stderr, "[DEBUG] 缓存模块: %s\n", cache_key);
    int cache_result = cache_module_with_program(file_path, module_scope, module_program);
    
    // 设置模块作用域中所有符号的源模块路径
    // 【修复】如果缓存成功，使用缓存中的规范化路径；否则使用当前的 cache_key
    const char *cached_path;
    size_t cached_path_len;
    
    if (cache_result) {
        // 缓存成功，从缓存中获取规范化路径
        cached_path = g_module_cache[g_cached_module_count - 1].file_path;
        cached_path_len = strlen(cached_path);
    } else {
        // 缓存失败（缓存已满或已存在），使用当前的 cache_key
        // 注意：这种情况下路径内存可能不是持久的，但至少不会崩溃
        cached_path = cache_key;
        cached_path_len = strlen(cache_key);
        fprintf(stderr, "[WARNING] 使用临时路径作为 source_module_path: %s\n", cache_key);
    }
    
    fprintf(stderr, "[DEBUG] Setting source_module_path for module %s: %s (len=%zu)\n",
            cache_key, cached_path, cached_path_len);
    
    // 遍历模块作用域中的所有符号，设置源模块路径
    CnSemSymbolNode *node = module_scope->symbols;
    while (node) {
        node->symbol.source_module_path = cached_path;
        node->symbol.source_module_path_length = cached_path_len;
        fprintf(stderr, "[DEBUG] Set source_module_path for symbol %.*s: %s\n",
                (int)node->symbol.name_length, node->symbol.name, cached_path);
        node = node->next;
    }
    
    if (normalized_path) free(normalized_path);
    return module_scope;
}

// 获取父目录路径
static char *get_parent_dir(const char *file_path)
{
    if (!file_path) {
        return NULL;
    }
    
    size_t len = strlen(file_path);
    if (len == 0) {
        return NULL;
    }
    
    // 查找最后一个路径分隔符
    const char *last_sep = NULL;
    for (size_t i = len; i > 0; --i) {
        if (file_path[i - 1] == '/' || file_path[i - 1] == '\\') {
            last_sep = &file_path[i - 1];
            break;
        }
    }
    
    if (!last_sep) {
        // 没有路径分隔符，返回当前目录
        return strdup(".");
    }
    
    size_t dir_len = last_sep - file_path;
    char *dir = (char *)malloc(dir_len + 1);
    if (!dir) {
        return NULL;
    }
    
    memcpy(dir, file_path, dir_len);
    dir[dir_len] = '\0';
    
    return dir;
}

// 带模块加载器的作用域构建
CnSemScope *cn_sem_build_scopes_with_loader(CnAstProgram *program, 
                                             CnDiagnostics *diagnostics,
                                             CnModuleLoader *loader,
                                             const char *source_file)
{
    CnSemScope *global_scope;
    size_t i;

    if (!program) {
        return NULL;
    }

    global_scope = cn_sem_scope_new(CN_SEM_SCOPE_GLOBAL, NULL);
    if (!global_scope) {
        return NULL;
    }

    // 注册内置函数：打印 (print)
    CnSemSymbol *print_sym = cn_sem_scope_insert_symbol(global_scope, "打印", strlen("打印"), CN_SEM_SYMBOL_FUNCTION);
    if (print_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);
        print_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_VOID), param_types, 1);
    }

    // 注册其他内置函数（简化，复用 cn_sem_build_scopes 的逻辑）
    CnSemSymbol *print_int_sym = cn_sem_scope_insert_symbol(global_scope, "打印整数", strlen("打印整数"), CN_SEM_SYMBOL_FUNCTION);
    if (print_int_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_INT);
        print_int_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_VOID), param_types, 1);
    }

    // 注册内置函数：打印格式 (printf) - 支持可变参数
    CnSemSymbol *printf_sym = cn_sem_scope_insert_symbol(global_scope, "打印格式", strlen("打印格式"), CN_SEM_SYMBOL_FUNCTION);
    if (printf_sym) {
        printf_sym->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
    }

    // 注册内置函数：字符串格式 (sprintf) - 支持可变参数
    CnSemSymbol *sprintf_sym = cn_sem_scope_insert_symbol(global_scope, "字符串格式", strlen("字符串格式"), CN_SEM_SYMBOL_FUNCTION);
    if (sprintf_sym) {
        sprintf_sym->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
    }

    // 注册内置函数：字符串格式化 (snprintf) - 支持可变参数
    CnSemSymbol *snprintf_sym = cn_sem_scope_insert_symbol(global_scope, "字符串格式化", strlen("字符串格式化"), CN_SEM_SYMBOL_FUNCTION);
    if (snprintf_sym) {
        snprintf_sym->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
    }

    // 注册内置函数：格式化字符串 (sprintf的别名) - 支持可变参数
    CnSemSymbol *format_str_sym = cn_sem_scope_insert_symbol(global_scope, "格式化字符串", strlen("格式化字符串"), CN_SEM_SYMBOL_FUNCTION);
    if (format_str_sym) {
        format_str_sym->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
    }

    // =============================================================================
    // 注册内存管理函数
    // =============================================================================
    
    // 注册内置函数：分配内存 (malloc)
    CnSemSymbol *malloc_sym = cn_sem_scope_insert_symbol(global_scope, "分配内存", strlen("分配内存"), CN_SEM_SYMBOL_FUNCTION);
    if (malloc_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_INT);  // size_t 简化为 int
        malloc_sym->type = cn_type_new_function(cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID)), param_types, 1);
    }
    
    // 注册内置函数：释放内存 (free)
    CnSemSymbol *free_sym = cn_sem_scope_insert_symbol(global_scope, "释放内存", strlen("释放内存"), CN_SEM_SYMBOL_FUNCTION);
    if (free_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID));
        free_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_VOID), param_types, 1);
    }
    
    // 注册内置函数：重新分配内存 (realloc)
    CnSemSymbol *realloc_sym = cn_sem_scope_insert_symbol(global_scope, "重新分配内存", strlen("重新分配内存"), CN_SEM_SYMBOL_FUNCTION);
    if (realloc_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *) * 2);
        param_types[0] = cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID));
        param_types[1] = cn_type_new_primitive(CN_TYPE_INT);  // size_t 简化为 int
        realloc_sym->type = cn_type_new_function(cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID)), param_types, 2);
    }
    
    // 注册内置函数：分配清零内存 (calloc)
    CnSemSymbol *calloc_sym = cn_sem_scope_insert_symbol(global_scope, "分配清零内存", strlen("分配清零内存"), CN_SEM_SYMBOL_FUNCTION);
    if (calloc_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *) * 2);
        param_types[0] = cn_type_new_primitive(CN_TYPE_INT);  // count
        param_types[1] = cn_type_new_primitive(CN_TYPE_INT);  // size
        calloc_sym->type = cn_type_new_function(cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID)), param_types, 2);
    }

    // 注册内置函数：分配内存数组 (malloc array)
    CnSemSymbol *malloc_array_sym = cn_sem_scope_insert_symbol(global_scope, "分配内存数组", strlen("分配内存数组"), CN_SEM_SYMBOL_FUNCTION);
    if (malloc_array_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *) * 2);
        param_types[0] = cn_type_new_primitive(CN_TYPE_INT);  // type_size
        param_types[1] = cn_type_new_primitive(CN_TYPE_INT);  // count
        malloc_array_sym->type = cn_type_new_function(cn_type_new_pointer(cn_type_new_primitive(CN_TYPE_VOID)), param_types, 2);
    }

    // 注册内置函数：类型大小 (sizeof)
    CnSemSymbol *sizeof_sym = cn_sem_scope_insert_symbol(global_scope, "类型大小", strlen("类型大小"), CN_SEM_SYMBOL_FUNCTION);
    if (sizeof_sym) {
        // 类型大小 接受任意类型参数，返回整数类型
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_UNKNOWN);  // 接受任意类型
        sizeof_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 1);
    }
    
    // =============================================================================
    // 注册字符串操作函数
    // =============================================================================
    
    // 注册内置函数：比较字符串 (strcmp)
    CnSemSymbol *strcmp_sym = cn_sem_scope_insert_symbol(global_scope, "比较字符串", strlen("比较字符串"), CN_SEM_SYMBOL_FUNCTION);
    if (strcmp_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *) * 2);
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);
        param_types[1] = cn_type_new_primitive(CN_TYPE_STRING);
        strcmp_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 2);
    }
    
    // 注册内置函数：字符串长度 (strlen)
    CnSemSymbol *strlen_sym = cn_sem_scope_insert_symbol(global_scope, "字符串长度", strlen("字符串长度"), CN_SEM_SYMBOL_FUNCTION);
    if (strlen_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);
        strlen_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT), param_types, 1);
    }
    
    // 注册内置函数：复制字符串 (strcpy)
    CnSemSymbol *strcpy_sym = cn_sem_scope_insert_symbol(global_scope, "复制字符串", strlen("复制字符串"), CN_SEM_SYMBOL_FUNCTION);
    if (strcpy_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *) * 2);
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);  // dest
        param_types[1] = cn_type_new_primitive(CN_TYPE_STRING);  // src
        strcpy_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_STRING), param_types, 2);
    }
    
    // 注册内置函数：连接字符串 (strcat)
    CnSemSymbol *strcat_sym = cn_sem_scope_insert_symbol(global_scope, "连接字符串", strlen("连接字符串"), CN_SEM_SYMBOL_FUNCTION);
    if (strcat_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *) * 2);
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);  // dest
        param_types[1] = cn_type_new_primitive(CN_TYPE_STRING);  // src
        strcat_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_STRING), param_types, 2);
    }

    // 设置模块加载器的搜索路径
    if (loader && source_file) {
        char *parent_dir = get_parent_dir(source_file);
        if (parent_dir) {
            cn_search_config_set_project_root(loader->search_config, parent_dir);
            free(parent_dir);
        }
    }

    // =============================================================================
    // 先处理枚举声明（必须在结构体之前，因为结构体可能使用枚举类型）
    // =============================================================================
    for (i = 0; i < program->enum_count; ++i) {
        CnAstStmt *enum_stmt = program->enums[i];
        if (!enum_stmt || enum_stmt->kind != CN_AST_STMT_ENUM_DECL) {
            continue;
        }

        CnAstEnumDecl *enum_decl = &enum_stmt->as.enum_decl;
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(global_scope,
                                   enum_decl->name,
                                   enum_decl->name_length,
                                   CN_SEM_SYMBOL_ENUM);
        if (sym) {
            sym->type = cn_type_new_enum(enum_decl->name, enum_decl->name_length);
            CnSemScope *enum_scope = cn_sem_scope_new(CN_SEM_SCOPE_ENUM, global_scope);
            if (enum_scope && sym->type) {
                sym->type->as.enum_type.enum_scope = enum_scope;
                // 【关键修复】同时设置 sym->as.module_scope，以便模块导入时能正确复制枚举作用域
                // 参见 module_semantics.c 第278-279行的导入逻辑
                sym->as.module_scope = enum_scope;
                // 注册枚举成员到枚举作用域和全局作用域
                // 这样可以直接通过成员名访问枚举成员（如：红、绿、蓝）
                for (size_t j = 0; j < enum_decl->member_count; j++) {
                    CnAstEnumMember *member = &enum_decl->members[j];
                    
                    // 先注册到枚举作用域
                    CnSemSymbol *member_sym = cn_sem_scope_insert_symbol(enum_scope,
                                                       member->name,
                                                       member->name_length,
                                                       CN_SEM_SYMBOL_ENUM_MEMBER);
                    if (member_sym) {
                        // 枚举成员的类型应该是枚举类型
                        member_sym->type = sym->type; // 使用枚举类型
                        member_sym->as.enum_value = member->value;
                    }
                    
                    // 同时注册到全局作用域（如果不存在同名符号）
                    CnSemSymbol *global_member_sym = cn_sem_scope_insert_symbol(global_scope,
                                                       member->name,
                                                       member->name_length,
                                                       CN_SEM_SYMBOL_ENUM_MEMBER);
                    if (global_member_sym) {
                        // 枚举成员的类型应该是枚举类型
                        global_member_sym->type = sym->type; // 使用枚举类型
                        global_member_sym->as.enum_value = member->value;
                    }
                }
            }
        }
        // 注意：如果 sym 为 NULL，说明全局作用域已有同名符号
        // 这可能是导入的符号，静默跳过（不报错）
    }

    // =============================================================================
    // 处理结构体声明（在枚举之后，支持结构体使用枚举类型字段）
    // =============================================================================
    for (i = 0; i < program->struct_count; ++i) {
        CnAstStmt *struct_stmt = program->structs[i];
        if (!struct_stmt || struct_stmt->kind != CN_AST_STMT_STRUCT_DECL) {
            continue;
        }

        CnAstStructDecl *struct_decl = &struct_stmt->as.struct_decl;
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(global_scope,
                                   struct_decl->name,
                                   struct_decl->name_length,
                                   CN_SEM_SYMBOL_STRUCT);
        if (sym) {
            CnStructField *fields = NULL;
            if (struct_decl->field_count > 0) {
                fields = (CnStructField *)malloc(sizeof(CnStructField) * struct_decl->field_count);
                for (size_t j = 0; j < struct_decl->field_count; j++) {
                    fields[j].name = struct_decl->fields[j].name;
                    fields[j].name_length = struct_decl->fields[j].name_length;
                    
                    // 解析字段类型：如果是自定义类型（结构体类型表示），从符号表查找真实类型
                    CnType *field_type = struct_decl->fields[j].field_type;
                    if (field_type && field_type->kind == CN_TYPE_STRUCT && field_type->as.struct_type.name) {
                        CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                                    field_type->as.struct_type.name,
                                                    field_type->as.struct_type.name_length);
                        if (type_sym && type_sym->type) {
                            // 使用符号表中的真实类型（可能是枚举或结构体）
                            field_type = type_sym->type;
                        }
                    }
                    fields[j].field_type = field_type;
                    fields[j].is_const = struct_decl->fields[j].is_const;
                }
            }
            sym->type = cn_type_new_struct(struct_decl->name,
                                          struct_decl->name_length,
                                          fields,
                                          struct_decl->field_count,
                                          global_scope,
                                          NULL, 0);
        }
        // 注意：如果 sym 为 NULL，说明全局作用域已有同名符号
        // 这可能是导入的符号，静默跳过（不报错）
    }
    
    // =============================================================================
    // 延迟解析：更新所有不完整的结构体字段类型（主程序第四个位置）
    // 解决结构体指针前向引用问题
    // =============================================================================
    {
        CnSemSymbolNode *node = global_scope->symbols;
        while (node) {
            CnSemSymbol *sym = &node->symbol;
            if (sym->kind == CN_SEM_SYMBOL_STRUCT && sym->type &&
                sym->type->kind == CN_TYPE_STRUCT &&
                sym->type->as.struct_type.fields) {
                for (size_t j = 0; j < sym->type->as.struct_type.field_count; j++) {
                    CnStructField *field = &sym->type->as.struct_type.fields[j];
                    CnType *field_type = field->field_type;
                    
                    // 情况1：字段类型是结构体但没有字段信息
                    if (field_type && field_type->kind == CN_TYPE_STRUCT &&
                        !field_type->as.struct_type.fields &&
                        field_type->as.struct_type.name) {
                        CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                                    field_type->as.struct_type.name,
                                                    field_type->as.struct_type.name_length);
                        if (type_sym && type_sym->type &&
                            type_sym->kind == CN_SEM_SYMBOL_STRUCT &&
                            type_sym->type->as.struct_type.fields) {
                            field->field_type = type_sym->type;
                        }
                    }
                    // 情况2：字段类型是指针，指向的结构体没有字段信息
                    else if (field_type && field_type->kind == CN_TYPE_POINTER &&
                             field_type->as.pointer_to &&
                             field_type->as.pointer_to->kind == CN_TYPE_STRUCT &&
                             !field_type->as.pointer_to->as.struct_type.fields &&
                             field_type->as.pointer_to->as.struct_type.name) {
                        CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                                    field_type->as.pointer_to->as.struct_type.name,
                                                    field_type->as.pointer_to->as.struct_type.name_length);
                        if (type_sym && type_sym->type &&
                            type_sym->kind == CN_SEM_SYMBOL_STRUCT &&
                            type_sym->type->as.struct_type.fields) {
                            field->field_type = cn_type_new_pointer(type_sym->type);
                        }
                    }
                }
            }
            node = node->next;
        }
    }

    // 处理导入语句：支持 Python 风格跨文件导入
    for (i = 0; i < program->import_count; ++i) {
        CnAstStmt *import_stmt = program->imports[i];
        if (!import_stmt || import_stmt->kind != CN_AST_STMT_IMPORT) {
            continue;
        }

        CnAstImportStmt *import = &import_stmt->as.import_stmt;
        
        // 检查是否为路径导入（使用module_path字段）
        if (import->module_path && loader && source_file) {
            CnAstModulePath *module_path = import->module_path;
            
            // 处理相对路径导入
            if (module_path->is_relative) {
                // 使用相对路径加载器
                // 根据 target_type 决定查找包还是模块
                CnModuleMetadata *metadata = cn_module_loader_load_relative_typed(
                    loader, source_file, module_path, import->target_type);
                
                if (metadata && metadata->file_path) {
                    // 加载外部模块（支持嵌套导入）
                    CnSemScope *external_scope = compile_external_module_recursive(
                        metadata->file_path, diagnostics, global_scope, loader, source_file);
                    
                    if (external_scope) {
                        // 处理导入逻辑
                        if (import->use_from_syntax) {
                            // 「从 ... 导入」语法：选择性导入成员
                            if (import->member_count > 0) {
                                // 选择性导入
                                for (size_t j = 0; j < import->member_count; ++j) {
                                    const char *member_name = import->members[j].name;
                                    size_t member_name_length = import->members[j].name_length;
                                    
                                    CnSemSymbol *member_sym = cn_sem_scope_lookup_shallow(
                                        external_scope, member_name, member_name_length);
                                    
                                    if (member_sym) {
                                        // 检查可见性：只能导入公开成员
                                        if (!member_sym->is_public) {
                                            cn_support_diag_semantic_error_generic(
                                                diagnostics,
                                                CN_DIAG_CODE_SEM_ACCESS_DENIED,
                                                NULL, 0, 0,
                                                "语义错误：无法导入私有成员");
                                            continue;
                                        }
                                        
                                        // 添加到全局作用域
                                        CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(
                                            global_scope, member_name, member_name_length, member_sym->kind);
                                        if (new_sym) {
                                            // 【关键修复】使用辅助函数复制类型信息（处理枚举类型为 NULL 的特殊情况）
                                            cn_sem_copy_symbol_type(member_sym, new_sym);
                                            new_sym->is_public = member_sym->is_public;
                                            new_sym->is_const = member_sym->is_const;
                                            // 复制源模块路径（关键：用于跨编译会话的符号唯一性判断）
                                            new_sym->source_module_path = member_sym->source_module_path;
                                            new_sym->source_module_path_length = member_sym->source_module_path_length;
                                            // 调试：检查导入的结构体类型是否有字段信息
                                            if (member_sym->kind == CN_SEM_SYMBOL_STRUCT && member_sym->type) {
                                            }
                                        }
                                    } else {
                                        cn_support_diag_semantic_error_generic(
                                            diagnostics,
                                            CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                                            NULL, 0, 0,
                                            "语义错误：导入的成员不存在");
                                    }
                                }
                            } else if (import->is_wildcard) {
                                // 通配符导入：导入所有公开符号
                                CnSemSymbolNode *node = external_scope->symbols;
                                while (node) {
                                    CnSemSymbol *sym = &node->symbol;
                                    // 只导入公开成员
                                    if (sym->is_public) {
                                        CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(
                                            global_scope, sym->name, sym->name_length, sym->kind);
                                        if (new_sym) {
                                            // 【关键修复】使用辅助函数复制类型信息（处理枚举类型为 NULL 的特殊情况）
                                            cn_sem_copy_symbol_type(sym, new_sym);
                                            new_sym->is_public = sym->is_public;
                                            new_sym->is_const = sym->is_const;
                                            // 【关键修复】保留原始 decl_scope 以便区分导入符号
                                            new_sym->decl_scope = sym->decl_scope;
                                            // 复制源模块路径（关键：用于跨编译会话的符号唯一性判断）
                                            new_sym->source_module_path = sym->source_module_path;
                                            new_sym->source_module_path_length = sym->source_module_path_length;
                                            // 复制 module_scope 以支持递归处理嵌套导入
                                            if (sym->kind == CN_SEM_SYMBOL_MODULE ||
                                                sym->kind == CN_SEM_SYMBOL_STRUCT ||
                                                sym->kind == CN_SEM_SYMBOL_ENUM) {
                                                new_sym->as.module_scope = sym->as.module_scope;
                                            }
                                        }
                                    }
                                    node = node->next;
                                }
                            }
                        } else {
                            // 纯「导入 路径」语法：全量导入模块的所有公开成员
                            // 从模块路径提取模块名（最后一个段）
                            const char *module_name = module_path->segments[module_path->segment_count - 1].name;
                            size_t module_name_length = module_path->segments[module_path->segment_count - 1].name_length;
                            
                            // 创建模块符号（用于点访问语法）
                            CnSemSymbol *module_sym = cn_sem_scope_insert_symbol(
                                global_scope, module_name, module_name_length, CN_SEM_SYMBOL_MODULE);
                            if (module_sym) {
                                module_sym->type = cn_type_new_primitive(CN_TYPE_VOID);
                                module_sym->is_public = 1;
                                module_sym->as.module_scope = external_scope;
                            }
                            
                            // 同时导入所有公开成员到当前作用域
                            CnSemSymbolNode *node = external_scope->symbols;
                            while (node) {
                                CnSemSymbol *sym = &node->symbol;
                                
                                // 调试输出
                                if (sym->name && sym->name_length > 0) {
                                    char debug_name[256];
                                    size_t copy_len = sym->name_length < 255 ? sym->name_length : 255;
                                    memcpy(debug_name, sym->name, copy_len);
                                    debug_name[copy_len] = '\0';
                                }
                                
                                // 只导入公开成员
                                if (!sym->is_public) {
                                    node = node->next;
                                    continue;
                                }
                                
                                // 检查名称冲突
                                CnSemSymbol *existing_sym = cn_sem_scope_lookup_shallow(global_scope,
                                                                                        sym->name,
                                                                                        sym->name_length);
                                if (existing_sym) {
                                    // 特殊处理：如果现有符号是模块符号，而新符号是结构体或枚举类型，
                                    // 则用新符号替换模块符号（因为类型定义比模块符号更重要）
                                    // 但需要保留 module_scope 以便递归处理嵌套导入
                                    // 【关键修复】只有当新符号有有效的 type 时才进行替换
                                    if (existing_sym->kind == CN_SEM_SYMBOL_MODULE &&
                                        (sym->kind == CN_SEM_SYMBOL_STRUCT || sym->kind == CN_SEM_SYMBOL_ENUM) &&
                                        sym->type != NULL) {
                                        // 保存 module_scope 以便递归处理嵌套导入
                                        CnSemScope *saved_module_scope = existing_sym->as.module_scope;
                                        
                                        // 更新现有符号的类型和属性
                                        existing_sym->kind = sym->kind;
                                        existing_sym->type = sym->type;
                                        existing_sym->is_public = sym->is_public;
                                        existing_sym->is_const = sym->is_const;
                                        existing_sym->decl_scope = sym->decl_scope;
                                        
                                        // 如果新符号也是模块，使用新符号的 module_scope
                                        // 否则保留原来的 module_scope（用于递归处理嵌套导入）
                                        if (sym->kind == CN_SEM_SYMBOL_MODULE && sym->as.module_scope) {
                                            existing_sym->as.module_scope = sym->as.module_scope;
                                        } else {
                                            existing_sym->as.module_scope = saved_module_scope;
                                        }
                                        
                                        // 调试：检查替换时的类型信息
                                        if (sym->type && sym->kind == CN_SEM_SYMBOL_STRUCT) {
                                            fprintf(stderr, ", fields=%p, field_count=%zu\n",
                                                    (void*)sym->type->as.struct_type.fields,
                                                    sym->type->as.struct_type.field_count);
                                        } else {
                                            fprintf(stderr, "\n");
                                        }
                                        node = node->next;
                                        continue;
                                    }
                                    // 【关键修复】如果新符号是结构体/枚举但 type 为 NULL，保持现有符号不变
                                    // 这通常发生在模块名称与结构体名称冲突时
                                    // 例如：导入 ./运算符 时，运算符 是模块名，不应该被替换为结构体
                                    if (existing_sym->kind == CN_SEM_SYMBOL_MODULE &&
                                        (sym->kind == CN_SEM_SYMBOL_STRUCT || sym->kind == CN_SEM_SYMBOL_ENUM) &&
                                        sym->type == NULL) {
                                        fprintf(stderr, "[DEBUG] Skipping replacement of module symbol '%.*s' with struct/enum symbol that has NULL type\n",
                                                (int)sym->name_length, sym->name);
                                        node = node->next;
                                        continue;
                                    }
                                    // 其他名称冲突，跳过
                                    node = node->next;
                                    continue;
                                }
                                
                                // 添加符号
                                CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(
                                    global_scope, sym->name, sym->name_length, sym->kind);
                                if (new_sym) {
                                    // 【关键修复】使用辅助函数复制类型信息（处理枚举类型为 NULL 的特殊情况）
                                    cn_sem_copy_symbol_type(sym, new_sym);
                                    new_sym->is_public = sym->is_public;
                                    new_sym->is_const = sym->is_const;
                                    // 保留原始 decl_scope 以便区分导入符号
                                    new_sym->decl_scope = sym->decl_scope;
                                    // 复制源模块路径（关键：用于跨编译会话的符号唯一性判断）
                                    new_sym->source_module_path = sym->source_module_path;
                                    new_sym->source_module_path_length = sym->source_module_path_length;
                                    // 调试输出
                                    if (sym->kind == CN_SEM_SYMBOL_FUNCTION) {
                                    }
                                    // 调试：检查导入的结构体类型是否有字段信息
                                    if (sym->kind == CN_SEM_SYMBOL_STRUCT) {
                                        if (sym->type) {
                                            fprintf(stderr, ", fields=%p, field_count=%zu\n",
                                                    (void*)sym->type->as.struct_type.fields,
                                                    sym->type->as.struct_type.field_count);
                                        } else {
                                            fprintf(stderr, " (type is NULL!)\n");
                                        }
                                    }
                                    // 复制 module_scope（对于模块符号和被替换为结构体的模块符号）
                                    if (sym->kind == CN_SEM_SYMBOL_MODULE || sym->kind == CN_SEM_SYMBOL_STRUCT) {
                                        if (sym->as.module_scope) {
                                            new_sym->as.module_scope = sym->as.module_scope;
                                        }
                                    }
                                    if (sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                                        new_sym->as.enum_value = sym->as.enum_value;
                                    }
                                }
                                
                                node = node->next;
                            }
                        }
                    } else {
                        cn_support_diag_semantic_error_generic(
                            diagnostics,
                            CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                            NULL, 0, 0,
                            "语义错误：无法编译外部模块");
                    }
                } else {
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                        NULL, 0, 0,
                        "语义错误：导入的模块不存在");
                }
            } else {
                // 绝对路径导入：使用模块加载器解析
                // 构建模块标识符
                // 【修复】添加空指针检查，防止 module_path->segments 为 NULL 时崩溃
                if (!module_path->segments || module_path->segment_count == 0) {
                    continue;
                }
                char qualified_name[1024];
                size_t offset = 0;
                for (size_t j = 0; j < module_path->segment_count && offset < sizeof(qualified_name) - 1; j++) {
                    if (j > 0) {
                        qualified_name[offset++] = '.';
                    }
                    size_t seg_len = module_path->segments[j].name_length;
                    if (offset + seg_len >= sizeof(qualified_name)) {
                        break;
                    }
                    memcpy(qualified_name + offset, module_path->segments[j].name, seg_len);
                    offset += seg_len;
                }
                qualified_name[offset] = '\0';
                
                CnModuleId *module_id = cn_module_id_create(qualified_name);
                if (!module_id) {
                    continue;
                }
                
                // 使用模块加载器解析文件路径
                // 根据 target_type 决定查找包还是模块
                char *resolved_path = NULL;
                if (cn_module_loader_resolve_path_typed(loader, module_id, &resolved_path, import->target_type)) {
                    // 加载外部模块（支持嵌套导入）
                    CnSemScope *external_scope = compile_external_module_recursive(resolved_path, 
                                                                          diagnostics, 
                                                                          global_scope,
                                                                          loader,
                                                                          source_file);
                    
                    if (external_scope) {
                        // 处理导入逻辑（同相对路径）
                        if (import->use_from_syntax) {
                            if (import->member_count > 0) {
                                for (size_t j = 0; j < import->member_count; ++j) {
                                    const char *member_name = import->members[j].name;
                                    size_t member_name_length = import->members[j].name_length;
                                    
                                    CnSemSymbol *member_sym = cn_sem_scope_lookup_shallow(
                                        external_scope, member_name, member_name_length);
                                    
                                    if (member_sym) {
                                        if (!member_sym->is_public) {
                                            cn_support_diag_semantic_error_generic(
                                                diagnostics,
                                                CN_DIAG_CODE_SEM_ACCESS_DENIED,
                                                NULL, 0, 0,
                                                "语义错误：无法导入私有成员");
                                            continue;
                                        }
                                        
                                        CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(
                                            global_scope, member_name, member_name_length, member_sym->kind);
                                        if (new_sym) {
                                            // 【关键修复】使用辅助函数复制类型信息（处理枚举类型为 NULL 的特殊情况）
                                            cn_sem_copy_symbol_type(member_sym, new_sym);
                                            new_sym->is_public = member_sym->is_public;
                                            new_sym->is_const = member_sym->is_const;
                                            // 复制源模块路径（关键：用于跨编译会话的符号唯一性判断）
                                            new_sym->source_module_path = member_sym->source_module_path;
                                            new_sym->source_module_path_length = member_sym->source_module_path_length;
                                        }
                                    } else {
                                        cn_support_diag_semantic_error_generic(
                                            diagnostics,
                                            CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                                            NULL, 0, 0,
                                            "语义错误：导入的成员不存在");
                                    }
                                }
                            } else if (import->is_wildcard) {
                                // 【修复】添加空指针检查，防止 external_scope 为 NULL 时崩溃
                                if (!external_scope) {
                                    continue;
                                }
                                CnSemSymbolNode *node = external_scope->symbols;
                                while (node) {
                                    CnSemSymbol *sym = &node->symbol;
                                    if (sym->is_public) {
                                        CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(
                                            global_scope, sym->name, sym->name_length, sym->kind);
                                        if (new_sym) {
                                            // 【关键修复】使用辅助函数复制类型信息（处理枚举类型为 NULL 的特殊情况）
                                            cn_sem_copy_symbol_type(sym, new_sym);
                                            new_sym->is_public = sym->is_public;
                                            new_sym->is_const = sym->is_const;
                                            // 【关键修复】保留原始 decl_scope 以便区分导入符号
                                            new_sym->decl_scope = sym->decl_scope;
                                            // 复制源模块路径（关键：用于跨编译会话的符号唯一性判断）
                                            new_sym->source_module_path = sym->source_module_path;
                                            new_sym->source_module_path_length = sym->source_module_path_length;
                                            // 复制 module_scope 以支持递归处理嵌套导入
                                            if (sym->kind == CN_SEM_SYMBOL_MODULE ||
                                                sym->kind == CN_SEM_SYMBOL_STRUCT ||
                                                sym->kind == CN_SEM_SYMBOL_ENUM) {
                                                new_sym->as.module_scope = sym->as.module_scope;
                                            }
                                        }
                                    }
                                    node = node->next;
                                }
                            }
                        } else {
                            // 纯导入语法：全量导入模块的所有公开成员
                            // 【修复】添加空指针检查，防止 module_path->segments 为 NULL 时崩溃
                            if (!module_path->segments || module_path->segment_count == 0) {
                                continue;
                            }
                            const char *module_name = module_path->segments[module_path->segment_count - 1].name;
                            size_t module_name_length = module_path->segments[module_path->segment_count - 1].name_length;
                            
                            CnSemSymbol *module_sym = cn_sem_scope_insert_symbol(
                                global_scope, module_name, module_name_length, CN_SEM_SYMBOL_MODULE);
                            if (module_sym) {
                                module_sym->type = cn_type_new_primitive(CN_TYPE_VOID);
                                module_sym->is_public = 1;
                                module_sym->as.module_scope = external_scope;
                            }
                            
                            // 同时导入所有公开成员到当前作用域
                            // 【修复】添加空指针检查，防止 external_scope->symbols 为 NULL 时崩溃
                            if (!external_scope) {
                                continue;
                            }
                            CnSemSymbolNode *node = external_scope->symbols;
                            while (node) {
                                CnSemSymbol *sym = &node->symbol;
                                
                                // 只导入公开成员
                                if (!sym->is_public) {
                                    node = node->next;
                                    continue;
                                }
                                
                                // 检查名称冲突
                                CnSemSymbol *existing_sym = cn_sem_scope_lookup_shallow(global_scope,
                                                                                        sym->name,
                                                                                        sym->name_length);
                                if (existing_sym) {
                                    // 特殊处理：如果新符号是结构体/枚举类型，而现有符号是枚举成员
                                    // 则用结构体/枚举类型替换枚举成员（结构体定义更重要）
                                    if ((sym->kind == CN_SEM_SYMBOL_STRUCT || sym->kind == CN_SEM_SYMBOL_ENUM) &&
                                        existing_sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                                        // 更新现有符号的类型和属性
                                        existing_sym->kind = sym->kind;
                                        existing_sym->type = sym->type;
                                        existing_sym->as.module_scope = sym->as.module_scope;
                                    }
                                    node = node->next;
                                    continue;  // 已存在，跳过
                                }
                                
                                // 添加符号
                                CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(
                                    global_scope, sym->name, sym->name_length, sym->kind);
                                if (new_sym) {
                                    // 【关键修复】使用辅助函数复制类型信息（处理枚举类型为 NULL 的特殊情况）
                                    cn_sem_copy_symbol_type(sym, new_sym);
                                    new_sym->is_public = sym->is_public;
                                    new_sym->is_const = sym->is_const;
                                    new_sym->decl_scope = sym->decl_scope;
                                    // 复制源模块路径（关键：用于跨编译会话的符号唯一性判断）
                                    new_sym->source_module_path = sym->source_module_path;
                                    new_sym->source_module_path_length = sym->source_module_path_length;
                                    // 复制 module_scope 以支持递归处理嵌套导入
                                    if (sym->kind == CN_SEM_SYMBOL_MODULE ||
                                        sym->kind == CN_SEM_SYMBOL_STRUCT ||
                                        sym->kind == CN_SEM_SYMBOL_ENUM) {
                                        new_sym->as.module_scope = sym->as.module_scope;
                                    } else if (sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                                        new_sym->as.enum_value = sym->as.enum_value;
                                    }
                                }
                                
                                node = node->next;
                            }
                        }
                    } else {
                        cn_support_diag_semantic_error_generic(
                            diagnostics,
                            CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                            NULL, 0, 0,
                            "语义错误：无法编译外部模块");
                    }
                    
                    free(resolved_path);
                } else {
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                        NULL, 0, 0,
                        "语义错误：导入的模块不存在");
                }
                
                cn_module_id_free(module_id);
            }
            
            continue;
        }
        
        // 传统导入：优先查找同文件内模块，如果不存在则通过模块加载器从搜索路径加载文件模块/包
        CnSemSymbol *module_sym = cn_sem_scope_lookup_shallow(global_scope,
                                                              import->module_name,
                                                              import->module_name_length);

        // 如果当前文件中没有内联模块定义，且存在模块加载器，则尝试跨文件加载
        if (!module_sym && loader && source_file &&
            import->module_name && import->module_name_length > 0) {
            char *module_name_str = (char *)malloc(import->module_name_length + 1);
            if (module_name_str) {
                memcpy(module_name_str, import->module_name, import->module_name_length);
                module_name_str[import->module_name_length] = '\0';

                char *resolved_path = NULL;
                
                // 1. 首先尝试在当前文件目录中查找
                // 获取源文件目录
                const char *last_sep = strrchr(source_file, '\\');
                const char *last_fwd_sep = strrchr(source_file, '/');
                if (last_fwd_sep > last_sep) {
                    last_sep = last_fwd_sep;
                }
                
                if (last_sep) {
                    // 构建当前目录路径
                    size_t dir_len = last_sep - source_file;
                    
                    if (import->target_type == CN_IMPORT_TARGET_MODULE) {
                        // 模块导入（导入 xxx;）：优先查找 .cn 文件，找不到再查找包
                        size_t path_len = dir_len + 1 + import->module_name_length + 3 + 1;
                        resolved_path = (char *)malloc(path_len);
                        if (resolved_path) {
                            memcpy(resolved_path, source_file, dir_len);
                            resolved_path[dir_len] = '\\';
                            memcpy(resolved_path + dir_len + 1, import->module_name, import->module_name_length);
                            strcpy(resolved_path + dir_len + 1 + import->module_name_length, ".cn");
                            
                            // 检查文件是否存在
                            FILE *test_file = fopen(resolved_path, "r");
                            if (test_file) {
                                fclose(test_file);
                            } else {
                                free(resolved_path);
                                resolved_path = NULL;
                                
                                // 模块文件不存在，回退到包目录查找
                                size_t pkg_path_len = dir_len + 1 + import->module_name_length + 1 + 12 + 1;
                                resolved_path = (char *)malloc(pkg_path_len);
                                if (resolved_path) {
                                    memcpy(resolved_path, source_file, dir_len);
                                    resolved_path[dir_len] = '\\';
                                    memcpy(resolved_path + dir_len + 1, import->module_name, import->module_name_length);
                                    resolved_path[dir_len + 1 + import->module_name_length] = '\\';
                                    strcpy(resolved_path + dir_len + 2 + import->module_name_length, "__\xe5\x8c\x85__.cn");
                                    
                                    FILE *pkg_file = fopen(resolved_path, "r");
                                    if (pkg_file) {
                                        fclose(pkg_file);
                                    } else {
                                        free(resolved_path);
                                        resolved_path = NULL;
                                    }
                                }
                            }
                        }
                    } else {
                        // 包导入（导入 ./xxx;）：优先查找包目录，找不到再查找 .cn 文件
                        size_t path_len = dir_len + 1 + import->module_name_length + 1 + 12 + 1;
                        resolved_path = (char *)malloc(path_len);
                        if (resolved_path) {
                            memcpy(resolved_path, source_file, dir_len);
                            resolved_path[dir_len] = '\\';
                            memcpy(resolved_path + dir_len + 1, import->module_name, import->module_name_length);
                            resolved_path[dir_len + 1 + import->module_name_length] = '\\';
                            strcpy(resolved_path + dir_len + 2 + import->module_name_length, "__\xe5\x8c\x85__.cn");
                            
                            // 检查文件是否存在
                            FILE *test_file = fopen(resolved_path, "r");
                            if (test_file) {
                                fclose(test_file);
                            } else {
                                free(resolved_path);
                                resolved_path = NULL;
                                
                                // 包不存在，回退到模块文件查找
                                size_t mod_path_len = dir_len + 1 + import->module_name_length + 3 + 1;
                                resolved_path = (char *)malloc(mod_path_len);
                                if (resolved_path) {
                                    memcpy(resolved_path, source_file, dir_len);
                                    resolved_path[dir_len] = '\\';
                                    memcpy(resolved_path + dir_len + 1, import->module_name, import->module_name_length);
                                    strcpy(resolved_path + dir_len + 1 + import->module_name_length, ".cn");
                                    
                                    FILE *mod_file = fopen(resolved_path, "r");
                                    if (mod_file) {
                                        fclose(mod_file);
                                    } else {
                                        free(resolved_path);
                                        resolved_path = NULL;
                                    }
                                }
                            }
                        }
                    }
                }
                
                // 2. 如果当前目录没找到，再通过模块加载器在搜索路径中查找
                if (!resolved_path) {
                    CnModuleId *module_id = cn_module_id_create(module_name_str);
                    if (module_id) {
                        cn_module_loader_resolve_path_typed(loader, module_id, &resolved_path, import->target_type);
                        cn_module_id_free(module_id);
                    }
                }
                
                free(module_name_str);
                
                // 3. 如果找到路径，编译外部模块
                if (resolved_path) {
                    CnSemScope *external_scope = compile_external_module_recursive(
                        resolved_path,
                        diagnostics,
                        global_scope,
                        loader,
                        source_file);
                    free(resolved_path);
                    if (external_scope) {
                        module_sym = cn_sem_scope_insert_symbol(
                            global_scope,
                            import->module_name,
                            import->module_name_length,
                            CN_SEM_SYMBOL_MODULE);
                        if (module_sym) {
                            module_sym->type = cn_type_new_primitive(CN_TYPE_VOID);
                            module_sym->is_public = 1;
                            module_sym->as.module_scope = external_scope;
                        }
                    }
                }
            }
        }
        
        if (!module_sym) {
            cn_support_diag_semantic_error_generic(
                diagnostics,
                CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                NULL, 0, 0,
                "语义错误：导入的模块不存在");
            continue;
        }
        
        if (module_sym->kind != CN_SEM_SYMBOL_MODULE || !module_sym->as.module_scope) {
            cn_support_diag_semantic_error_generic(
                diagnostics,
                CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                NULL, 0, 0,
                "语义错误：导入的符号不是模块");
            continue;
        }
        
        // 处理传统导入（复用原来的逻辑）
        CnSemScope *module_scope = module_sym->as.module_scope;
        
        // 如果指定了别名，则以别名注册模块符号
        if (import->alias) {
            // 使用别名注册模块
            CnSemSymbol *existing_alias = cn_sem_scope_lookup_shallow(global_scope,
                                                                      import->alias,
                                                                      import->alias_length);
            if (existing_alias) {
                // 别名冲突，输出详细诊断信息
                char alias_name[256];
                size_t copy_len = import->alias_length < 255 ? import->alias_length : 255;
                memcpy(alias_name, import->alias, copy_len);
                alias_name[copy_len] = '\0';
                
                char module_name[256];
                copy_len = import->module_name_length < 255 ? import->module_name_length : 255;
                memcpy(module_name, import->module_name, copy_len);
                module_name[copy_len] = '\0';
                
                char error_msg[512];
                snprintf(error_msg, sizeof(error_msg),
                        "语义错误：模块别名 '%s' 已存在（导入模块 '%s' 时发生冲突）",
                        alias_name, module_name);
                        
                cn_support_diag_semantic_error_generic(
                    diagnostics,
                    CN_DIAG_CODE_SEM_DUPLICATE_SYMBOL,
                    NULL, 0, 0,
                    error_msg);
                continue;
            }
            
            // 以别名注册模块符号
            CnSemSymbol *alias_sym = cn_sem_scope_insert_symbol(global_scope,
                                                                import->alias,
                                                                import->alias_length,
                                                                CN_SEM_SYMBOL_MODULE);
            if (alias_sym) {
                alias_sym->type = module_sym->type;
                alias_sym->is_public = module_sym->is_public;
                alias_sym->is_const = module_sym->is_const;
                alias_sym->as.module_scope = module_sym->as.module_scope;
            }
            continue;  // 使用别名时不进行成员导入
        }
        
        // 「导入 xxx;」 语法：全量导入模块的所有公开成员
        // 「从 xxx 导入 ...」 语法：选择性导入成员
        // 两种语法都需要导入成员到当前作用域
        
        // 判断是全量导入还是选择性导入
        if (import->member_count == 0) {
            // 全量导入：遍历模块作用域中的所有公开符号，添加到全局作用域
            // 【修复】添加空指针检查，防止 module_scope 为 NULL 时崩溃
            if (!module_scope) {
                continue;
            }
            CnSemSymbolNode *node = module_scope->symbols;
            while (node) {
                CnSemSymbol *sym = &node->symbol;
                
                // 调试输出：显示正在导入的符号
                if (sym->name && sym->name_length > 0) {
                    char debug_name[256];
                    size_t copy_len = sym->name_length < 255 ? sym->name_length : 255;
                    memcpy(debug_name, sym->name, copy_len);
                    debug_name[copy_len] = '\0';
                }
                
                // 只导入公开成员
                if (!sym->is_public) {
                    node = node->next;
                    continue;
                }
                
                // 检查名称冲突
                CnSemSymbol *existing_sym = cn_sem_scope_lookup_shallow(global_scope,
                                                                        sym->name,
                                                                        sym->name_length);
                if (existing_sym) {
                    // 检查是否是同一个符号（同一模块的同一成员）
                    if (cn_sem_is_same_symbol(existing_sym, sym)) {
                        // 同一符号，静默忽略，不报错也不重复添加
                        node = node->next;
                        continue;
                    }
                    // 【关键修复】同名不同类型的符号可以共存
                    // 只有当符号种类相同且不是同一个符号时，才报告冲突
                    else if (existing_sym->kind == sym->kind && existing_sym->kind != CN_SEM_SYMBOL_MODULE) {
                        // 同名同种类但不是同一个符号，报错
                        cn_support_diag_semantic_error_duplicate_symbol(
                            diagnostics, NULL, 0, 0, sym->name, sym->name_length);
                        node = node->next;
                        continue;
                    }
                    // 【新增】同名不同类型的符号，允许共存
                    else if (existing_sym->kind != sym->kind) {
                        fprintf(stderr, "[DEBUG] Import: same name but different kinds (existing=%d, new=%d), allowing coexistence\n",
                                existing_sym->kind, sym->kind);
                        // 继续添加新符号（不跳过）
                    }
                }
                
                // 没有冲突，添加符号
                if (!existing_sym) {
                    CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(global_scope,
                                                                      sym->name,
                                                                      sym->name_length,
                                                                      sym->kind);
                    if (new_sym) {
                        // 复制符号信息
                        // 【关键修复】使用辅助函数复制类型信息（处理枚举类型为 NULL 的特殊情况）
                        cn_sem_copy_symbol_type(sym, new_sym);
                        new_sym->is_public = sym->is_public;
                        new_sym->is_const = sym->is_const;
                        new_sym->decl_scope = sym->decl_scope;
                        // 复制源模块路径（关键：用于跨编译会话的符号唯一性判断）
                        new_sym->source_module_path = sym->source_module_path;
                        new_sym->source_module_path_length = sym->source_module_path_length;
                        // 复制 module_scope 以支持递归处理嵌套导入
                        if (sym->kind == CN_SEM_SYMBOL_MODULE ||
                            sym->kind == CN_SEM_SYMBOL_STRUCT ||
                            sym->kind == CN_SEM_SYMBOL_ENUM) {
                            new_sym->as.module_scope = sym->as.module_scope;
                        } else if (sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                            new_sym->as.enum_value = sym->as.enum_value;
                        }
                    }
                }
                
                node = node->next;
            }
        } else {
            // 选择性导入：只导入指定的成员
            // 【修复】添加空指针检查，防止 module_scope 为 NULL 时崩溃
            if (!module_scope) {
                continue;
            }
            for (size_t j = 0; j < import->member_count; ++j) {
                const char *member_name = import->members[j].name;
                size_t member_name_length = import->members[j].name_length;
                
                // 在模块作用域中查找指定成员
                CnSemSymbol *member_sym = cn_sem_scope_lookup_shallow(module_scope,
                                                                      member_name,
                                                                      member_name_length);
                
                if (!member_sym) {
                    // 成员不存在，报错
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                        NULL, 0, 0,
                        "语义错误：导入的成员不存在");
                    continue;
                }
                
                // 检查可见性：只能导入公开成员
                if (!member_sym->is_public) {
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_ACCESS_DENIED,
                        NULL, 0, 0,
                        "语义错误：无法导入私有成员");
                    continue;
                }
                
                // 检查名称冲突
                CnSemSymbol *existing_sym = cn_sem_scope_lookup_shallow(global_scope,
                                                                        member_name,
                                                                        member_name_length);
                if (existing_sym) {
                    // 检查是否是同一个符号（同一模块的同一成员）
                    if (cn_sem_is_same_symbol(existing_sym, member_sym)) {
                        // 同一符号，静默忽略，不报错也不重复添加
                        continue;
                    }
                    // 【关键修复】同名不同类型的符号可以共存
                    // 只有当符号种类相同且不是同一个符号时，才报告冲突
                    else if (existing_sym->kind == member_sym->kind && existing_sym->kind != CN_SEM_SYMBOL_MODULE) {
                        // 同名同种类但不是同一个符号，报错
                        cn_support_diag_semantic_error_duplicate_symbol(
                            diagnostics, NULL, 0, 0, member_name, member_name_length);
                        continue;
                    }
                    // 【新增】同名不同类型的符号，允许共存
                    else if (existing_sym->kind != member_sym->kind) {
                        fprintf(stderr, "[DEBUG] Selective import: same name but different kinds (existing=%d, new=%d), allowing coexistence\n",
                                existing_sym->kind, member_sym->kind);
                        // 继续添加新符号（不跳过）
                    }
                }
                
                // 没有冲突，添加符号
                if (!existing_sym) {
                    CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(global_scope,
                                                                      member_name,
                                                                      member_name_length,
                                                                      member_sym->kind);
                    if (new_sym) {
                        // 复制符号信息
                        // 【关键修复】使用辅助函数复制类型信息（处理枚举类型为 NULL 的特殊情况）
                        cn_sem_copy_symbol_type(member_sym, new_sym);
                        new_sym->is_public = member_sym->is_public;
                        new_sym->decl_scope = member_sym->decl_scope;
                        // 复制源模块路径（关键：用于跨编译会话的符号唯一性判断）
                        new_sym->source_module_path = member_sym->source_module_path;
                        new_sym->source_module_path_length = member_sym->source_module_path_length;
                        // 复制 module_scope 以支持结构体和枚举类型的成员访问
                        if (member_sym->kind == CN_SEM_SYMBOL_MODULE ||
                            member_sym->kind == CN_SEM_SYMBOL_STRUCT ||
                            member_sym->kind == CN_SEM_SYMBOL_ENUM) {
                            new_sym->as.module_scope = member_sym->as.module_scope;
                        } else if (member_sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                            new_sym->as.enum_value = member_sym->as.enum_value;
                        }
                    }
                }
            }
        }
    }

    // 注册全局变量
    for (i = 0; i < program->global_var_count; ++i) {
        CnAstStmt *var_stmt = program->global_vars[i];
        if (!var_stmt || var_stmt->kind != CN_AST_STMT_VAR_DECL) {
            continue;
        }

        CnAstVarDecl *var_decl = &var_stmt->as.var_decl;
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(global_scope,
                                   var_decl->name,
                                   var_decl->name_length,
                                   CN_SEM_SYMBOL_VARIABLE);
        if (sym) {
            sym->type = var_decl->declared_type;
            sym->is_const = var_decl->is_const;
        }
        // 注意：如果 sym 为 NULL，说明全局作用域已有同名符号
        // 这可能是导入的符号，静默跳过（不报错）
    }

    // 注册函数并构建函数作用域
    for (i = 0; i < program->function_count; ++i) {
        CnAstFunctionDecl *function_decl = program->functions[i];
        if (!function_decl) {
            continue;
        }

        CnSemSymbol *sym = cn_sem_scope_insert_symbol(global_scope,
                                   function_decl->name,
                                   function_decl->name_length,
                                   CN_SEM_SYMBOL_FUNCTION);
        if (sym) {
            // 构建完整的函数类型
            CnType **param_types = NULL;
            if (function_decl->parameter_count > 0) {
                param_types = (CnType **)malloc(sizeof(CnType *) * function_decl->parameter_count);
                for (size_t j = 0; j < function_decl->parameter_count; j++) {
                    CnType *param_type = function_decl->parameters[j].declared_type;
                    // 特殊处理：如果参数类型是结构体类型，可能是枚举类型或类类型
                    // 需要从符号表查找真实类型
                    if (param_type && param_type->kind == CN_TYPE_STRUCT) {
                        CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                                param_type->as.struct_type.name,
                                                param_type->as.struct_type.name_length);
                        if (type_sym && type_sym->type) {
                            if (type_sym->kind == CN_SEM_SYMBOL_ENUM) {
                                // 替换为枚举类型
                                param_type = type_sym->type;
                            } else if (type_sym->kind == CN_SEM_SYMBOL_STRUCT) {
                                // 替换为完整的结构体/类类型
                                param_type = type_sym->type;
                            }
                        }
                    }
                    param_types[j] = param_type;
                }
            }
            // 使用函数声明中的返回类型，如果没有则使用UNKNOWN（后续通过return语句推断）
            CnType *return_type = function_decl->return_type;
            if (!return_type) {
                return_type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
            } else if (return_type->kind == CN_TYPE_STRUCT) {
                // 特殊处理：如果返回类型是结构体类型，可能是枚举类型或类类型
                // 需要从符号表查找真实类型
                CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                        return_type->as.struct_type.name,
                                        return_type->as.struct_type.name_length);
                if (type_sym && type_sym->type) {
                    if (type_sym->kind == CN_SEM_SYMBOL_ENUM) {
                        // 替换为枚举类型
                        return_type = type_sym->type;
                    } else if (type_sym->kind == CN_SEM_SYMBOL_STRUCT) {
                        // 替换为完整的结构体/类类型
                        return_type = type_sym->type;
                    }
                }
            }
            sym->type = cn_type_new_function(return_type,
                                            param_types,
                                            function_decl->parameter_count);
        } else {
            // 插入失败，检查是否是导入的符号
            CnSemSymbol *existing_sym = cn_sem_scope_lookup_shallow(global_scope,
                                                                    function_decl->name,
                                                                    function_decl->name_length);
            if (existing_sym && existing_sym->kind == CN_SEM_SYMBOL_FUNCTION) {
                // 检查是否是函数原型声明（无函数体）
                if (function_decl->is_prototype) {
                    // 函数原型声明：允许重复声明，不报错
                    // 不需要做任何事情，保留现有符号
                } else if (existing_sym->source_module_path != NULL) {
                    // 检查是否是导入的符号（source_module_path 不为空表示来自其他模块）
                    // 这是导入的函数，用当前模块定义的函数覆盖它
                    // 构建完整的函数类型
                    CnType **param_types = NULL;
                    if (function_decl->parameter_count > 0) {
                        param_types = (CnType **)malloc(sizeof(CnType *) * function_decl->parameter_count);
                        for (size_t j = 0; j < function_decl->parameter_count; j++) {
                            CnType *param_type = function_decl->parameters[j].declared_type;
                            // 特殊处理：如果参数类型是结构体类型，可能是枚举类型或类类型
                            if (param_type && param_type->kind == CN_TYPE_STRUCT) {
                                CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                                        param_type->as.struct_type.name,
                                                        param_type->as.struct_type.name_length);
                                if (type_sym && type_sym->type) {
                                    if (type_sym->kind == CN_SEM_SYMBOL_ENUM) {
                                        param_type = type_sym->type;
                                    } else if (type_sym->kind == CN_SEM_SYMBOL_STRUCT) {
                                        param_type = type_sym->type;
                                    }
                                }
                            }
                            param_types[j] = param_type;
                        }
                    }
                    CnType *return_type = function_decl->return_type;
                    if (!return_type) {
                        return_type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                    } else if (return_type->kind == CN_TYPE_STRUCT) {
                        CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                                return_type->as.struct_type.name,
                                                return_type->as.struct_type.name_length);
                        if (type_sym && type_sym->type) {
                            if (type_sym->kind == CN_SEM_SYMBOL_ENUM) {
                                return_type = type_sym->type;
                            } else if (type_sym->kind == CN_SEM_SYMBOL_STRUCT) {
                                return_type = type_sym->type;
                            }
                        }
                    }
                    // 更新现有符号的类型和声明作用域
                    existing_sym->type = cn_type_new_function(return_type,
                                                    param_types,
                                                    function_decl->parameter_count);
                    existing_sym->decl_scope = global_scope;  // 标记为当前模块定义
                } else {
                    // 检查是否是同一个符号（来自同一模块）
                    // 如果是同一个符号，静默跳过，不报错
                    // 注意：这里需要创建一个临时符号来比较
                    CnSemSymbol temp_sym;
                    memset(&temp_sym, 0, sizeof(CnSemSymbol));
                    temp_sym.name = function_decl->name;
                    temp_sym.name_length = function_decl->name_length;
                    temp_sym.kind = CN_SEM_SYMBOL_FUNCTION;
                    temp_sym.type = existing_sym->type;  // 使用现有符号的类型进行比较
                    temp_sym.decl_scope = existing_sym->decl_scope;
                    temp_sym.source_module_path = existing_sym->source_module_path;
                    temp_sym.source_module_path_length = existing_sym->source_module_path_length;
                    
                    if (!cn_sem_is_same_symbol(existing_sym, &temp_sym)) {
                        // 真正的重复定义（不同模块的同名符号）
                        cn_support_diag_semantic_error_duplicate_symbol(
                            diagnostics, NULL, 0, 0, function_decl->name, function_decl->name_length);
                    }
                    // 如果是同一个符号，静默跳过
                }
            } else {
                // 其他类型的符号冲突
                cn_support_diag_semantic_error_duplicate_symbol(
                    diagnostics, NULL, 0, 0, function_decl->name, function_decl->name_length);
            }
        }

        cn_sem_build_function_scope(global_scope, function_decl, diagnostics);
    }

    return global_scope;
}
