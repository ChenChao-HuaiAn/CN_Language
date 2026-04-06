#include "cnlang/frontend/semantics.h"
#include "cnlang/frontend/module_loader.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/preprocessor.h"
#include "cnlang/support/diagnostics.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

// MSVC 兼容性：较旧版本的 MSVC (< VS2015) 中 snprintf 未定义
#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

// 辅助函数：检查两个符号是否是同一个符号（同一个作用域、同一个类型、同一个名字）
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
    
    // 检查是否来自同一个声明作用域（最关键的判断）
    if (sym1->decl_scope && sym2->decl_scope) {
        // 两个符号都有声明作用域，比较作用域指针
        return (sym1->decl_scope == sym2->decl_scope) ? 1 : 0;
    }
    
    // 如果至少有一个符号没有 decl_scope，回退到比较类型指针
    // 同一个模块的同一个函数应该共享同一个类型对象
    if (sym1->type && sym2->type) {
        if (sym1->type == sym2->type) {
            return 1;  // 类型指针相同，认为是同一个符号
        }
        // 对于函数类型，延迟到类型结构比较
        if (sym1->type->kind == CN_TYPE_FUNCTION && sym2->type->kind == CN_TYPE_FUNCTION) {
            // 比较函数类型的结构：参数数量
            if (sym1->type->as.function.param_count != sym2->type->as.function.param_count) {
                return 0;
            }
            // 参数类型和返回类型在这里不进一步比较，
            // 因为同名同种类的符号且参数数量相同，极有可能是同一函数
            return 1;
        }
    }
    
    // 默认情况：同名同种类的符号认为是同一个符号
    // 这种情况主要适用于从不同路径导入同一个模块的符号
    return 1;
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
} CachedModule;

static CachedModule g_module_cache[MAX_CACHED_MODULES];
static int g_cached_module_count = 0;

// 查找缓存的模块
static CnSemScope *find_cached_module(const char *file_path) {
    for (int i = 0; i < g_cached_module_count; i++) {
        if (strcmp(g_module_cache[i].file_path, file_path) == 0) {
            return g_module_cache[i].scope;
        }
    }
    return NULL;
}

// 缓存模块
static void cache_module(const char *file_path, CnSemScope *scope) {
    if (g_cached_module_count >= MAX_CACHED_MODULES) {
        return;  // 缓存已满
    }
    g_module_cache[g_cached_module_count].file_path = strdup(file_path);
    g_module_cache[g_cached_module_count].scope = scope;
    g_cached_module_count++;
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

// 检查是否正在编译该模块（循环导入检测）
static int is_module_compiling(const char *file_path) {
    for (int i = 0; i < g_compile_depth; i++) {
        if (strcmp(g_compiling_modules[i], file_path) == 0) {
            return 1;
        }
    }
    return 0;
}

// 将模块压入编译栈
static int push_compiling_module(const char *file_path) {
    if (g_compile_depth >= MAX_MODULE_COMPILE_DEPTH) {
        return 0;  // 栈溢出
    }
    g_compiling_modules[g_compile_depth++] = file_path;
    return 1;
}

// 将模块弹出编译栈
static void pop_compiling_module(void) {
    if (g_compile_depth > 0) {
        g_compile_depth--;
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
        // 使用 UNKNOWN 类型标记，让类型检查器特殊处理
        sizeof_sym->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
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
                    } else {
                        // 报告重复定义
                        cn_support_diag_semantic_error_duplicate_symbol(
                            diagnostics, NULL, 0, 0, member->name);
                    }
                    
                    // 同时注册到全局作用域（如果不存在同名符号）
                    CnSemSymbol *global_member_sym = cn_sem_scope_insert_symbol(global_scope,
                                                       member->name,
                                                       member->name_length,
                                                       CN_SEM_SYMBOL_ENUM_MEMBER);
                    if (global_member_sym) {
                        global_member_sym->type = cn_type_new_primitive(CN_TYPE_INT);
                        global_member_sym->as.enum_value = member->value;
                    }
                }
            }
        } else {
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, enum_decl->name);
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
                    fprintf(stderr, "[DEBUG] 字段类型解析: 字段 %.*s, 原始类型 kind=%d\n",
                            (int)fields[j].name_length, fields[j].name,
                            field_type ? field_type->kind : -1);
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
                        fprintf(stderr, "[DEBUG] 字段指针类型解析: 字段 %.*s, 指向结构体='%.*s'\n",
                                (int)fields[j].name_length, fields[j].name,
                                (int)field_type->as.pointer_to->as.struct_type.name_length,
                                field_type->as.pointer_to->as.struct_type.name);
                        CnSemSymbol *type_sym = cn_sem_scope_lookup(global_scope,
                                                    field_type->as.pointer_to->as.struct_type.name,
                                                    field_type->as.pointer_to->as.struct_type.name_length);
                        if (type_sym && type_sym->type) {
                            // 创建新的指针类型，指向解析后的类型
                            field_type = cn_type_new_pointer(type_sym->type);
                            fprintf(stderr, "[DEBUG] 字段指针类型解析成功: 新指针指向类型 kind=%d\n",
                                    field_type->as.pointer_to->kind);
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
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, struct_decl->name);
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
                    if (cn_sem_is_same_symbol(existing_sym, sym)) {
                        // 同一符号，静默忽略，不报错也不重复添加
                        node = node->next;
                        continue;
                    } else if (existing_sym->kind != CN_SEM_SYMBOL_MODULE) {
                        // 不同符号但名字冲突，报错
                        cn_support_diag_semantic_error_duplicate_symbol(
                            diagnostics, NULL, 0, 0, sym->name);
                        node = node->next;
                        continue;
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
                        new_sym->type = sym->type;
                        new_sym->is_public = sym->is_public;
                        new_sym->is_const = sym->is_const;
                        // 保留原始 decl_scope 以便区分导入符号
                        new_sym->decl_scope = sym->decl_scope;
                        // 调试输出
                        if (sym->kind == CN_SEM_SYMBOL_FUNCTION) {
                            fprintf(stderr, "[DEBUG] 导入函数 '%.*s': decl_scope=%p (原始), global_scope=%p\n",
                                    (int)sym->name_length, sym->name,
                                    (void*)sym->decl_scope, (void*)global_scope);
                        }
                        if (sym->kind == CN_SEM_SYMBOL_MODULE) {
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
                    } else if (existing_sym->kind != CN_SEM_SYMBOL_MODULE) {
                        // 不同符号但名字冲突，报错
                        cn_support_diag_semantic_error_duplicate_symbol(
                            diagnostics, NULL, 0, 0, member_name);
                        continue;
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
                        new_sym->type = member_sym->type;
                        new_sym->is_public = member_sym->is_public;
                        new_sym->decl_scope = member_sym->decl_scope;
                        if (member_sym->kind == CN_SEM_SYMBOL_MODULE) {
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
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, var_decl->name);
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
                // 调试输出
                fprintf(stderr, "[DEBUG] 函数 '%.*s' 插入失败，existing_sym->decl_scope=%p, global_scope=%p\n",
                        (int)function_decl->name_length, function_decl->name,
                        (void*)existing_sym->decl_scope, (void*)global_scope);
                // 检查是否是导入的符号（decl_scope 不同表示来自其他模块）
                if (existing_sym->decl_scope != global_scope) {
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
                    // 真正的重复定义（同一模块内）
                    cn_support_diag_semantic_error_duplicate_symbol(
                        diagnostics, NULL, 0, 0, function_decl->name);
                }
            } else {
                // 其他类型的符号冲突
                cn_support_diag_semantic_error_duplicate_symbol(
                    diagnostics, NULL, 0, 0, function_decl->name);
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
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, param->name);
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
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, var_decl->name);
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
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, struct_decl->name);
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
                        // 枚举成员的类型是整数
                        member_sym->type = cn_type_new_primitive(CN_TYPE_INT);
                        // 保存枚举成员的值
                        member_sym->as.enum_value = member->value;
                    } else {
                        // 报告重复定义
                        cn_support_diag_semantic_error_duplicate_symbol(
                            diagnostics, NULL, 0, 0, member->name);
                    }
                    
                    // 同时注册到当前作用域（如果不存在同名符号）
                    CnSemSymbol *scope_member_sym = cn_sem_scope_insert_symbol(scope,
                                                       member->name,
                                                       member->name_length,
                                                       CN_SEM_SYMBOL_ENUM_MEMBER);
                    if (scope_member_sym) {
                        scope_member_sym->type = cn_type_new_primitive(CN_TYPE_INT);
                        scope_member_sym->as.enum_value = member->value;
                    }
                }
            }
        } else {
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, enum_decl->name);
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
    CnSemScope *cached = find_cached_module(file_path);
    if (cached) {
        fprintf(stderr, "[DEBUG] 模块缓存命中: %s\n", file_path);
        return cached;  // 返回缓存的作用域
    }
    
    fprintf(stderr, "[DEBUG] 开始编译外部模块: %s\n", file_path);
    
    // 检测循环导入
    if (is_module_compiling(file_path)) {
        fprintf(stderr, "[DEBUG] 检测到循环导入: %s\n", file_path);
        cn_support_diag_semantic_error_generic(
            diagnostics,
            CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
            NULL, 0, 0,
            "语义错误：检测到循环导入");
        return NULL;
    }
    
    // 压入编译栈
    if (!push_compiling_module(file_path)) {
        fprintf(stderr, "[DEBUG] 模块导入嵌套层级太深: %s\n", file_path);
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
        fprintf(stderr, "[DEBUG] 读取文件失败: %s, errno: %d\n", file_path, errno);
        pop_compiling_module();
        return NULL;
    }
    fprintf(stderr, "[DEBUG] 文件读取成功，大小: %zu 字节\n", file_size);
    
    // 预处理
    CnPreprocessor preprocessor;
    cn_frontend_preprocessor_init(&preprocessor, source, file_size, file_path);
    
    if (!cn_frontend_preprocessor_process(&preprocessor)) {
        fprintf(stderr, "[DEBUG] 预处理失败: %s\n", file_path);
        cn_frontend_preprocessor_free(&preprocessor);
        free(source);
        pop_compiling_module();
        return NULL;
    }
    fprintf(stderr, "[DEBUG] 预处理成功\n");
    
    // 词法分析
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, preprocessor.output, preprocessor.output_length, file_path);
    
    // 语法分析
    CnParser *parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "[DEBUG] 创建解析器失败: %s\n", file_path);
        cn_frontend_preprocessor_free(&preprocessor);
        free(source);
        pop_compiling_module();
        return NULL;
    }
    
    CnAstProgram *module_program = NULL;
    int ok = cn_frontend_parse_program(parser, &module_program);
    
    if (!ok || !module_program) {
        fprintf(stderr, "[DEBUG] 语法分析失败: %s, ok=%d, module_program=%p\n", file_path, ok, (void*)module_program);
        cn_frontend_parser_free(parser);
        cn_frontend_preprocessor_free(&preprocessor);
        free(source);
        pop_compiling_module();
        return NULL;
    }
    fprintf(stderr, "[DEBUG] 语法分析成功，函数数: %zu, 全局变量数: %zu\n",
            module_program->function_count, module_program->global_var_count);
    
    // 为外部模块创建作用域
    CnSemScope *module_scope = cn_sem_scope_new(CN_SEM_SCOPE_FILE_MODULE, global_scope);
    if (!module_scope) {
        fprintf(stderr, "[DEBUG] 创建模块作用域失败: %s\n", file_path);
        cn_frontend_ast_program_free(module_program);
        cn_frontend_parser_free(parser);
        cn_frontend_preprocessor_free(&preprocessor);
        free(source);
        pop_compiling_module();
        return NULL;
    }
    fprintf(stderr, "[DEBUG] 模块作用域创建成功: %p\n", (void*)module_scope);
    
    // =============================================================================
    // 重要：先处理导入语句，再注册结构体
    // 这样被导入模块中的类型才能在结构体字段类型解析时被找到
    // =============================================================================
    
    fprintf(stderr, "[DEBUG] 检查导入处理条件: loader=%p, importing_file=%p, import_count=%zu\n",
            (void*)loader, (void*)importing_file, module_program->import_count);
    
    // 如果提供了loader,则处理模块内部的导入语句（支持嵌套导入）
    if (loader && importing_file && module_program->import_count > 0) {
        fprintf(stderr, "[DEBUG] 开始处理模块内部导入语句，模块: %s\n", file_path);
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
                                            new_sym->type = sym->type;
                                            new_sym->is_public = sym->is_public;
                                            new_sym->is_const = sym->is_const;
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
                                            new_sym->type = member_sym->type;
                                            new_sym->is_public = member_sym->is_public;
                                            new_sym->is_const = member_sym->is_const;
                                        }
                                    }
                                }
                            }
                        } else {
                            // 纯「导入 路径」语法：全量导入模块的所有公开成员
                            fprintf(stderr, "[DEBUG] 纯路径导入，将所有公开符号导入到模块作用域\n");
                            CnSemSymbolNode *node = nested_scope->symbols;
                            while (node) {
                                CnSemSymbol *sym = &node->symbol;
                                if (sym->is_public) {
                                    CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(
                                        module_scope, sym->name, sym->name_length, sym->kind);
                                    if (new_sym) {
                                        new_sym->type = sym->type;
                                        new_sym->is_public = sym->is_public;
                                        new_sym->is_const = sym->is_const;
                                        fprintf(stderr, "[DEBUG] 导入符号: %.*s, kind=%d\n",
                                                (int)sym->name_length, sym->name, sym->kind);
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
        
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(module_scope,
                                   function_decl->name,
                                   function_decl->name_length,
                                   CN_SEM_SYMBOL_FUNCTION);
        if (sym) {
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
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(module_scope,
                                   var_decl->name,
                                   var_decl->name_length,
                                   CN_SEM_SYMBOL_VARIABLE);
        if (sym) {
            sym->type = var_decl->declared_type;
            sym->is_const = var_decl->is_const;
            // 根据AST中的visibility字段设置可见性
            sym->is_public = (var_decl->visibility == CN_VISIBILITY_PUBLIC) ? 1 : 0;
            
            // 如果没有显式类型，从初始化表达式推断类型
            if (!sym->type && var_decl->initializer) {
                // 简单类型推断：根据初始化表达式的类型
                CnAstExpr *init = var_decl->initializer;
                if (init->kind == CN_AST_EXPR_STRING_LITERAL) {
                    sym->type = cn_type_new_primitive(CN_TYPE_STRING);
                } else if (init->kind == CN_AST_EXPR_INTEGER_LITERAL) {
                    sym->type = cn_type_new_primitive(CN_TYPE_INT);
                } else if (init->kind == CN_AST_EXPR_FLOAT_LITERAL) {
                    sym->type = cn_type_new_primitive(CN_TYPE_FLOAT);
                } else if (init->kind == CN_AST_EXPR_BOOL_LITERAL) {
                    sym->type = cn_type_new_primitive(CN_TYPE_BOOL);
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
        CnSemSymbol *sym = cn_sem_scope_insert_symbol(module_scope,
                                   enum_decl->name,
                                   enum_decl->name_length,
                                   CN_SEM_SYMBOL_ENUM);
        if (sym) {
            // 创建枚举类型
            sym->type = cn_type_new_enum(enum_decl->name, enum_decl->name_length);
            
            // 为枚举创建一个作用域来存储其成员
            CnSemScope *enum_scope = cn_sem_scope_new(CN_SEM_SCOPE_ENUM, module_scope);
            if (enum_scope && sym->type) {
                sym->type->as.enum_type.enum_scope = enum_scope;
                
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
                        member_sym->type = cn_type_new_primitive(CN_TYPE_INT);
                        member_sym->as.enum_value = member->value;
                        // 枚举成员继承枚举的可见性
                        member_sym->is_public = 1;  // 枚举成员默认公开
                    }
                    
                    // 同时注册到模块作用域（如果不存在同名符号）
                    CnSemSymbol *module_member_sym = cn_sem_scope_insert_symbol(module_scope,
                                                       member->name,
                                                       member->name_length,
                                                       CN_SEM_SYMBOL_ENUM_MEMBER);
                    if (module_member_sym) {
                        module_member_sym->type = cn_type_new_primitive(CN_TYPE_INT);
                        module_member_sym->as.enum_value = member->value;
                        module_member_sym->is_public = 1;  // 枚举成员默认公开
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
        if (existing && existing->kind == CN_SEM_SYMBOL_STRUCT) {
            // 已存在，跳过
            continue;
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
        sym->type->as.struct_type.fields = fields;
        sym->type->as.struct_type.field_count = struct_decl->field_count;
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
                            fprintf(stderr, "[DEBUG] 延迟解析字段类型: %.*s.%.*s -> 完整类型\n",
                                    (int)sym->name_length, sym->name,
                                    (int)field->name_length, field->name);
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
                            fprintf(stderr, "[DEBUG] 延迟解析指针字段类型: %.*s.%.*s -> 完整指针类型\n",
                                    (int)sym->name_length, sym->name,
                                    (int)field->name_length, field->name);
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
    
    // 缓存模块作用域
    cache_module(file_path, module_scope);
    
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
        sizeof_sym->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
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
                        member_sym->type = cn_type_new_primitive(CN_TYPE_INT);
                        member_sym->as.enum_value = member->value;
                    }
                    
                    // 同时注册到全局作用域（如果不存在同名符号）
                    CnSemSymbol *global_member_sym = cn_sem_scope_insert_symbol(global_scope,
                                                       member->name,
                                                       member->name_length,
                                                       CN_SEM_SYMBOL_ENUM_MEMBER);
                    if (global_member_sym) {
                        global_member_sym->type = cn_type_new_primitive(CN_TYPE_INT);
                        global_member_sym->as.enum_value = member->value;
                    }
                }
            }
        }
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
                                            new_sym->type = member_sym->type;
                                            new_sym->is_public = member_sym->is_public;
                                            new_sym->is_const = member_sym->is_const;
                                            // 调试：检查导入的结构体类型是否有字段信息
                                            if (member_sym->kind == CN_SEM_SYMBOL_STRUCT && member_sym->type) {
                                                fprintf(stderr, "[DEBUG] 导入结构体 %.*s, type=%p, fields=%p, field_count=%zu\n",
                                                        (int)member_name_length, member_name,
                                                        (void*)member_sym->type,
                                                        (void*)member_sym->type->as.struct_type.fields,
                                                        member_sym->type->as.struct_type.field_count);
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
                                            new_sym->type = sym->type;
                                            new_sym->is_public = sym->is_public;
                                            new_sym->is_const = sym->is_const;
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
                                    fprintf(stderr, "[DEBUG] 相对路径导入符号: %s, kind=%d, is_public=%d\n",
                                            debug_name, sym->kind, sym->is_public);
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
                                    if (existing_sym->kind == CN_SEM_SYMBOL_MODULE &&
                                        (sym->kind == CN_SEM_SYMBOL_STRUCT || sym->kind == CN_SEM_SYMBOL_ENUM)) {
                                        // 更新现有符号的类型和属性
                                        existing_sym->kind = sym->kind;
                                        existing_sym->type = sym->type;
                                        existing_sym->is_public = sym->is_public;
                                        existing_sym->is_const = sym->is_const;
                                        existing_sym->decl_scope = sym->decl_scope;
                                        // 调试：检查替换时的类型信息
                                        fprintf(stderr, "[DEBUG] 模块符号 %.*s 被替换为类型定义, kind=%d, type=%p",
                                                (int)sym->name_length, sym->name, sym->kind, (void*)sym->type);
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
                                    // 其他名称冲突，跳过
                                    node = node->next;
                                    continue;
                                }
                                
                                // 添加符号
                                CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(
                                    global_scope, sym->name, sym->name_length, sym->kind);
                                if (new_sym) {
                                    new_sym->type = sym->type;
                                    new_sym->is_public = sym->is_public;
                                    new_sym->is_const = sym->is_const;
                                    // 保留原始 decl_scope 以便区分导入符号
                                    new_sym->decl_scope = sym->decl_scope;
                                    // 调试输出
                                    if (sym->kind == CN_SEM_SYMBOL_FUNCTION) {
                                        fprintf(stderr, "[DEBUG] 相对路径导入函数 '%.*s': decl_scope=%p, global_scope=%p\n",
                                                (int)sym->name_length, sym->name,
                                                (void*)sym->decl_scope, (void*)global_scope);
                                    }
                                    // 调试：检查导入的结构体类型是否有字段信息
                                    if (sym->kind == CN_SEM_SYMBOL_STRUCT) {
                                        fprintf(stderr, "[DEBUG] 导入结构体符号 %.*s, type=%p",
                                                (int)sym->name_length, sym->name,
                                                (void*)sym->type);
                                        if (sym->type) {
                                            fprintf(stderr, ", fields=%p, field_count=%zu\n",
                                                    (void*)sym->type->as.struct_type.fields,
                                                    sym->type->as.struct_type.field_count);
                                        } else {
                                            fprintf(stderr, " (type is NULL!)\n");
                                        }
                                    }
                                    if (sym->kind == CN_SEM_SYMBOL_MODULE) {
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
                                            new_sym->type = member_sym->type;
                                            new_sym->is_public = member_sym->is_public;
                                            new_sym->is_const = member_sym->is_const;
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
                                CnSemSymbolNode *node = external_scope->symbols;
                                while (node) {
                                    CnSemSymbol *sym = &node->symbol;
                                    if (sym->is_public) {
                                        CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(
                                            global_scope, sym->name, sym->name_length, sym->kind);
                                        if (new_sym) {
                                            new_sym->type = sym->type;
                                            new_sym->is_public = sym->is_public;
                                            new_sym->is_const = sym->is_const;
                                        }
                                    }
                                    node = node->next;
                                }
                            }
                        } else {
                            // 纯导入语法：全量导入模块的所有公开成员
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
                                    node = node->next;
                                    continue;  // 已存在，跳过
                                }
                                
                                // 添加符号
                                CnSemSymbol *new_sym = cn_sem_scope_insert_symbol(
                                    global_scope, sym->name, sym->name_length, sym->kind);
                                if (new_sym) {
                                    new_sym->type = sym->type;
                                    new_sym->is_public = sym->is_public;
                                    new_sym->is_const = sym->is_const;
                                    new_sym->decl_scope = sym->decl_scope;
                                    if (sym->kind == CN_SEM_SYMBOL_MODULE) {
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

                // [DEBUG] 调试输出
                fprintf(stderr, "[DEBUG] 尝试加载模块: %s\n", module_name_str);
                fprintf(stderr, "[DEBUG] 源文件: %s\n", source_file);

                char *resolved_path = NULL;
                
                // 1. 首先尝试在当前文件目录中查找
                // 获取源文件目录
                const char *last_sep = strrchr(source_file, '\\');
                const char *last_fwd_sep = strrchr(source_file, '/');
                if (last_fwd_sep > last_sep) {
                    last_sep = last_fwd_sep;
                }
                
                fprintf(stderr, "[DEBUG] last_sep: %s\n", last_sep ? last_sep : "NULL");
                
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
                            
                            fprintf(stderr, "[DEBUG] 构建路径: %s\n", resolved_path);
                            
                            // 检查文件是否存在
                            FILE *test_file = fopen(resolved_path, "r");
                            if (test_file) {
                                fprintf(stderr, "[DEBUG] 文件打开成功\n");
                                fclose(test_file);
                            } else {
                                fprintf(stderr, "[DEBUG] 文件打开失败，errno: %d\n", errno);
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
                    fprintf(stderr, "[DEBUG] 调用 compile_external_module_recursive: %s\n", resolved_path);
                    CnSemScope *external_scope = compile_external_module_recursive(
                        resolved_path,
                        diagnostics,
                        global_scope,
                        loader,
                        source_file);
                    free(resolved_path);

                    fprintf(stderr, "[DEBUG] external_scope: %p\n", (void*)external_scope);
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
            CnSemSymbolNode *node = module_scope->symbols;
            while (node) {
                CnSemSymbol *sym = &node->symbol;
                
                // 调试输出：显示正在导入的符号
                if (sym->name && sym->name_length > 0) {
                    char debug_name[256];
                    size_t copy_len = sym->name_length < 255 ? sym->name_length : 255;
                    memcpy(debug_name, sym->name, copy_len);
                    debug_name[copy_len] = '\0';
                    fprintf(stderr, "[DEBUG] 导入符号: %s, kind=%d, is_public=%d\n",
                            debug_name, sym->kind, sym->is_public);
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
                    } else if (existing_sym->kind != CN_SEM_SYMBOL_MODULE) {
                        // 不同符号但名字冲突，报错
                        cn_support_diag_semantic_error_duplicate_symbol(
                            diagnostics, NULL, 0, 0, sym->name);
                        node = node->next;
                        continue;
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
                        new_sym->type = sym->type;
                        new_sym->is_public = sym->is_public;
                        new_sym->is_const = sym->is_const;
                        new_sym->decl_scope = sym->decl_scope;
                        if (sym->kind == CN_SEM_SYMBOL_MODULE) {
                            new_sym->as.module_scope = sym->as.module_scope;
                        } else if (sym->kind == CN_SEM_SYMBOL_ENUM_MEMBER) {
                            new_sym->as.enum_value = sym->as.enum_value;
                        } else if (sym->kind == CN_SEM_SYMBOL_ENUM) {
                            // 枚举类型：确保类型信息完整（包含枚举作用域）
                            // 注意：sym->type 已经包含 enum_scope，直接复制即可
                        }
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
                    } else if (existing_sym->kind != CN_SEM_SYMBOL_MODULE) {
                        // 不同符号但名字冲突，报错
                        cn_support_diag_semantic_error_duplicate_symbol(
                            diagnostics, NULL, 0, 0, member_name);
                        continue;
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
                        new_sym->type = member_sym->type;
                        new_sym->is_public = member_sym->is_public;
                        new_sym->decl_scope = member_sym->decl_scope;
                        if (member_sym->kind == CN_SEM_SYMBOL_MODULE) {
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
    }

    // 注册函数并构建函数作用域
    fprintf(stderr, "[DEBUG] cn_sem_build_scopes_with_loader: 主模块函数数量=%zu\n", program->function_count);
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
            fprintf(stderr, "[DEBUG] cn_sem_build_scopes_with_loader: 函数 '%.*s' 插入失败，existing_sym=%p, kind=%d, decl_scope=%p, global_scope=%p\n",
                    (int)function_decl->name_length, function_decl->name,
                    (void*)existing_sym, existing_sym ? existing_sym->kind : -1,
                    existing_sym ? (void*)existing_sym->decl_scope : NULL, (void*)global_scope);
            if (existing_sym && existing_sym->kind == CN_SEM_SYMBOL_FUNCTION) {
                // 检查是否是导入的符号（decl_scope 不同表示来自其他模块）
                if (existing_sym->decl_scope != global_scope) {
                    fprintf(stderr, "[DEBUG] 覆盖导入函数 '%.*s' 的类型\n", (int)function_decl->name_length, function_decl->name);
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
                    // 真正的重复定义（同一模块内）
                    cn_support_diag_semantic_error_duplicate_symbol(
                        diagnostics, NULL, 0, 0, function_decl->name);
                }
            } else {
                // 其他类型的符号冲突
                cn_support_diag_semantic_error_duplicate_symbol(
                    diagnostics, NULL, 0, 0, function_decl->name);
            }
        }

        cn_sem_build_function_scope(global_scope, function_decl, diagnostics);
    }

    return global_scope;
}
