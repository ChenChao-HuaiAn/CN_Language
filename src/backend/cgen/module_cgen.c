/**
 * @file module_cgen.c
 * @brief 阶段E：多文件模块代码生成实现
 * 
 * 实现多文件编译单元管理、模块头文件/实现文件生成、
 * 初始化函数生成、符号命名规则和增量编译支持。
 */

#include "cnlang/backend/cgen.h"
#include "cnlang/frontend/module_loader.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/ir/ir.h"
#include "cnlang/support/diagnostics.h"
#include "cnlang/runtime/cli.h"  // 命令行参数接口
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

// --- 辅助函数声明 ---

static char *cn_alloc_string(const char *src, size_t len);
static uint64_t cn_get_file_mtime(const char *path);
static char *cn_generate_output_path(const char *output_dir, 
                                      CnModuleId *module_id,
                                      const char *extension);
static void cn_sanitize_module_name(CnModuleId *module_id,
                                     char *out_buffer,
                                     size_t buffer_size);
static const char *cn_module_id_get_name(CnModuleId *id);

// ============================================================================
// E1: 多文件编译单元管理
// ============================================================================

CnMultiFileCompileContext *cn_multi_compile_ctx_create(const char *output_dir,
                                                        struct CnModuleLoader *loader) {
    if (!output_dir || !loader) {
        return NULL;
    }
    
    CnMultiFileCompileContext *ctx = calloc(1, sizeof(CnMultiFileCompileContext));
    if (!ctx) {
        return NULL;
    }
    
    size_t dir_len = strlen(output_dir);
    ctx->output_dir = cn_alloc_string(output_dir, dir_len);
    if (!ctx->output_dir) {
        free(ctx);
        return NULL;
    }
    ctx->output_dir_length = dir_len;
    
    ctx->loader = loader;
    ctx->units = NULL;
    ctx->unit_count = 0;
    ctx->unit_capacity = 0;
    
    ctx->init_order = NULL;
    ctx->init_order_count = 0;
    
    return ctx;
}

void cn_multi_compile_ctx_free(CnMultiFileCompileContext *ctx) {
    if (!ctx) return;
    
    // 释放每个编译单元
    for (size_t i = 0; i < ctx->unit_count; i++) {
        CnModuleCompileUnit *unit = &ctx->units[i];
        if (unit->source_path) free(unit->source_path);
        if (unit->header_path) free(unit->header_path);
        if (unit->impl_path) free(unit->impl_path);
        // program 和 ir_module 由外部管理，不在此释放
    }
    
    free(ctx->units);
    free(ctx->output_dir);
    free(ctx->init_order);  // 模块ID由加载器管理，不释放
    free(ctx);
}

int cn_multi_compile_ctx_add_unit(CnMultiFileCompileContext *ctx,
                                   struct CnModuleId *module_id,
                                   const char *source_path,
                                   int is_entry) {
    if (!ctx || !module_id || !source_path) {
        return 0;
    }
    
    // 扩容检查
    if (ctx->unit_count >= ctx->unit_capacity) {
        size_t new_cap = ctx->unit_capacity == 0 ? 8 : ctx->unit_capacity * 2;
        CnModuleCompileUnit *new_units = realloc(ctx->units, 
                                                   new_cap * sizeof(CnModuleCompileUnit));
        if (!new_units) {
            return 0;
        }
        ctx->units = new_units;
        ctx->unit_capacity = new_cap;
    }
    
    // 初始化新单元
    CnModuleCompileUnit *unit = &ctx->units[ctx->unit_count];
    memset(unit, 0, sizeof(CnModuleCompileUnit));
    
    unit->module_id = module_id;
    unit->source_path = cn_alloc_string(source_path, strlen(source_path));
    if (!unit->source_path) {
        return 0;
    }
    
    // 生成头文件和实现文件路径
    unit->header_path = cn_generate_output_path(ctx->output_dir, module_id, ".h");
    unit->impl_path = cn_generate_output_path(ctx->output_dir, module_id, ".c");
    
    if (!unit->header_path || !unit->impl_path) {
        free(unit->source_path);
        free(unit->header_path);
        return 0;
    }
    
    unit->is_entry = is_entry;
    unit->source_mtime = cn_get_file_mtime(source_path);
    unit->output_mtime = cn_get_file_mtime(unit->impl_path);
    unit->needs_rebuild = (unit->source_mtime > unit->output_mtime);
    
    ctx->unit_count++;
    return 1;
}

// ============================================================================
// E2: 模块头文件生成
// ============================================================================

// C 类型转字符串的辅助函数
static const char *get_c_type_str(CnType *type) {
    if (!type) return "void";
    switch (type->kind) {
        case CN_TYPE_INT: return "long long";
        case CN_TYPE_FLOAT: return "double";
        case CN_TYPE_CHAR: return "char";
        case CN_TYPE_BOOL: return "_Bool";
        case CN_TYPE_STRING: return "char*";
        case CN_TYPE_INT32: return "int";
        case CN_TYPE_INT64: return "long long";
        case CN_TYPE_UINT32: return "unsigned int";
        case CN_TYPE_UINT64: return "unsigned long long";
        case CN_TYPE_UINT64_LL: return "unsigned long long";
        case CN_TYPE_FLOAT32: return "float";
        case CN_TYPE_FLOAT64: return "double";
        case CN_TYPE_VOID: return "void";
        case CN_TYPE_POINTER: {
            static _Thread_local char buffer[256];
            snprintf(buffer, sizeof(buffer), "%s*", 
                     get_c_type_str(type->as.pointer_to));
            return buffer;
        }
        case CN_TYPE_STRUCT: {
            static _Thread_local char buffer[256];
            snprintf(buffer, sizeof(buffer), "struct %.*s", 
                     (int)type->as.struct_type.name_length,
                     type->as.struct_type.name);
            return buffer;
        }
        case CN_TYPE_ENUM: {
            static _Thread_local char buffer[256];
            snprintf(buffer, sizeof(buffer), "enum %.*s", 
                     (int)type->as.enum_type.name_length,
                     type->as.enum_type.name);
            return buffer;
        }
        default: return "int";
    }
}

int cn_cgen_module_header(CnMultiFileCompileContext *ctx, CnModuleCompileUnit *unit) {
    if (!ctx || !unit || !unit->header_path) {
        return -1;
    }
    
    FILE *f = fopen(unit->header_path, "w");
    if (!f) {
        return -1;
    }
    
    // 生成头文件保护符
    char guard_name[256];
    cn_sanitize_module_name(unit->module_id, guard_name, sizeof(guard_name));
    
    fprintf(f, "/* 由 CN 语言编译器自动生成 */\n");
    fprintf(f, "#ifndef __%s_H__\n", guard_name);
    fprintf(f, "#define __%s_H__\n\n", guard_name);
    
    fprintf(f, "#include <stdint.h>\n");
    fprintf(f, "#include <stdbool.h>\n\n");
    
    fprintf(f, "#ifdef __cplusplus\n");
    fprintf(f, "extern \"C\" {\n");
    fprintf(f, "#endif\n\n");
    
    // 生成模块初始化函数声明
    fprintf(f, "/* 模块初始化函数 */\n");
    fprintf(f, "void __cn_init_%s__(void);\n\n", guard_name);
    
    // 遍历 IR 模块，导出公开函数声明
    if (unit->ir_module) {
        fprintf(f, "/* 公开函数声明 */\n");
        
        // 遍历函数链表
        CnIrFunction *func = unit->ir_module->first_func;
        while (func) {
            if (!func->name) {
                func = func->next;
                continue;
            }
            
            // 跳过私有函数（以下划线开头约定）
            if (func->name[0] == '_') {
                func = func->next;
                continue;
            }
            
            // 直接使用原始函数名（不再使用编码名称）
            // 模块系统通过作用域管理避免命名冲突
            
            // 返回类型
            const char *ret_type = get_c_type_str(func->return_type);
            fprintf(f, "%s %s(", ret_type, func->name);
            
            // 参数列表
            if (func->param_count == 0) {
                fprintf(f, "void");
            } else {
                for (size_t p = 0; p < func->param_count; p++) {
                    if (p > 0) fprintf(f, ", ");
                    const char *param_type = get_c_type_str(func->params[p].type);
                    fprintf(f, "%s arg%zu", param_type, p);
                }
            }
            fprintf(f, ");\n");
            
            func = func->next;
        }
    }
    
    fprintf(f, "\n#ifdef __cplusplus\n");
    fprintf(f, "}\n");
    fprintf(f, "#endif\n\n");
    
    fprintf(f, "#endif /* __%s_H__ */\n", guard_name);
    
    fclose(f);
    return 0;
}

// ============================================================================
// E3: 模块实现文件生成
// ============================================================================

int cn_cgen_module_impl(CnMultiFileCompileContext *ctx, CnModuleCompileUnit *unit) {
    if (!ctx || !unit || !unit->impl_path || !unit->ir_module) {
        return -1;
    }
    
    FILE *f = fopen(unit->impl_path, "w");
    if (!f) {
        return -1;
    }
    
    fprintf(f, "/* 由 CN 语言编译器自动生成 */\n\n");
    
    // 包含自身头文件
    char module_name[256];
    cn_sanitize_module_name(unit->module_id, module_name, sizeof(module_name));
    fprintf(f, "#include \"%s.h\"\n", module_name);
    
    // 包含运行时头文件
    fprintf(f, "#include \"cnrt.h\"\n");
    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "#include <stdlib.h>\n");
    fprintf(f, "#include <string.h>\n\n");
    
    // 创建代码生成上下文
    CnCCodeGenContext cgen_ctx = {
        .output_file = f,
        .module = unit->ir_module,
        .current_func = NULL,
        .current_block = NULL,
        .label_counter = 0,
        .temp_var_counter = 0,
        .module_init_infos = ctx,
        .module_init_count = ctx->unit_count,
        .module_id = unit->module_id  // 传递模块ID用于生成带前缀的函数名
    };
    
    // 生成模块初始化函数
    cn_cgen_module_init_func(&cgen_ctx, unit);
    
    // 生成全局变量
    CnIrGlobalVar *global = unit->ir_module->first_global;
    if (global) {
        fprintf(f, "/* 全局变量 */\n");
        while (global) {
            // 直接使用原始变量名（不再使用编码名称）
            const char *type_str = get_c_type_str(global->type);
            fprintf(f, "static %s %s;\n", type_str, global->name);
            
            global = global->next;
        }
        fprintf(f, "\n");
    }
    
    // 生成函数实现
    fprintf(f, "/* 函数实现 */\n");
    CnIrFunction *func = unit->ir_module->first_func;
    while (func) {
        cgen_ctx.current_func = func;
        cn_cgen_function(&cgen_ctx, func);
        fprintf(f, "\n");
        func = func->next;
    }
    
    fclose(f);
    return 0;
}

// ============================================================================
// E4: 模块初始化函数生成
// ============================================================================

void cn_cgen_module_init_func(CnCCodeGenContext *ctx, CnModuleCompileUnit *unit) {
    if (!ctx || !ctx->output_file || !unit || !unit->module_id) {
        return;
    }
    
    FILE *f = ctx->output_file;
    
    // 生成初始化函数名称
    char module_name[256];
    cn_sanitize_module_name(unit->module_id, module_name, sizeof(module_name));
    
    // 生成静态初始化标志
    fprintf(f, "static _Bool __cn_init_%s_done__ = 0;\n\n", module_name);
    
    fprintf(f, "void __cn_init_%s__(void) {\n", module_name);
    fprintf(f, "    if (__cn_init_%s_done__) return;\n", module_name);
    fprintf(f, "    __cn_init_%s_done__ = 1;\n\n", module_name);
    
    // 初始化全局变量
    if (unit->ir_module && unit->ir_module->first_global) {
        fprintf(f, "    /* 初始化全局变量 */\n");
        CnIrGlobalVar *global = unit->ir_module->first_global;
        while (global) {
            // 直接使用原始变量名（不再使用编码名称）
            
            // 根据初始化值类型生成初始化代码
            CnIrOperand *init = &global->initializer;
            switch (init->kind) {
                case CN_IR_OP_IMM_INT:
                    fprintf(f, "    %s = %lld;\n", global->name,
                            (long long)init->as.imm_int);
                    break;
                case CN_IR_OP_IMM_FLOAT:
                    fprintf(f, "    %s = %f;\n", global->name,
                            init->as.imm_float);
                    break;
                default:
                    // 默认零初始化
                    break;
            }
            
            global = global->next;
        }
    }
    
    fprintf(f, "}\n\n");
}

// ============================================================================
// E5: 模块初始化调用顺序
// ============================================================================

int cn_cgen_init_call_sequence(CnMultiFileCompileContext *ctx, FILE *output_file) {
    if (!ctx || !output_file) {
        return -1;
    }
    
    // 获取拓扑排序后的初始化顺序
    if (!ctx->init_order && ctx->loader) {
        CnModuleId **order = NULL;
        size_t order_count = 0;
        if (cn_module_loader_get_init_order(ctx->loader, &order, &order_count)) {
            ctx->init_order = order;
            ctx->init_order_count = order_count;
        }
    }
    
    // 生成初始化序列函数
    fprintf(output_file, "/* 模块初始化序列 - 按依赖顺序调用 */\n");
    fprintf(output_file, "void __cn_init_all_modules__(void) {\n");
    
    if (ctx->init_order && ctx->init_order_count > 0) {
        for (size_t i = 0; i < ctx->init_order_count; i++) {
            char module_name[256];
            cn_sanitize_module_name(ctx->init_order[i], module_name, sizeof(module_name));
            fprintf(output_file, "    __cn_init_%s__();\n", module_name);
        }
    }
    
    fprintf(output_file, "}\n\n");
    return 0;
}

// ============================================================================
// E6: 符号命名规则
// ============================================================================

size_t cn_cgen_qualified_symbol_name(struct CnModuleId *module_id,
                                      const char *symbol_name,
                                      size_t symbol_name_length,
                                      char *out_buffer,
                                      size_t buffer_size) {
    if (!out_buffer || buffer_size == 0) {
        return 0;
    }
    
    // 获取模块限定前缀
    char module_prefix[256];
    if (module_id) {
        cn_sanitize_module_name(module_id, module_prefix, sizeof(module_prefix));
    } else {
        module_prefix[0] = '\0';
    }
    
    // 对符号名称进行安全转换（中文转拼音或哈希）
    char safe_symbol[256];
    size_t safe_len = 0;
    
    for (size_t i = 0; i < symbol_name_length && safe_len < sizeof(safe_symbol) - 1; i++) {
        unsigned char c = (unsigned char)symbol_name[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_') {
            safe_symbol[safe_len++] = c;
        } else if (c >= 0x80) {
            // UTF-8 多字节字符 - 使用十六进制编码
            safe_symbol[safe_len++] = '_';
            if (safe_len < sizeof(safe_symbol) - 3) {
                snprintf(safe_symbol + safe_len, 3, "%02x", c);
                safe_len += 2;
            }
        } else {
            safe_symbol[safe_len++] = '_';
        }
    }
    safe_symbol[safe_len] = '\0';
    
    // 组合完整限定名
    size_t written;
    if (module_prefix[0] != '\0') {
        written = snprintf(out_buffer, buffer_size, "%s__%s", module_prefix, safe_symbol);
    } else {
        written = snprintf(out_buffer, buffer_size, "%s", safe_symbol);
    }
    
    return (written < buffer_size) ? written : buffer_size - 1;
}

// ============================================================================
// E7: 增量编译支持
// ============================================================================

int cn_cgen_needs_rebuild(CnModuleCompileUnit *unit) {
    if (!unit) {
        return 1;  // 无效单元视为需要重建
    }
    
    // 如果没有输出文件，需要编译
    if (!unit->impl_path) {
        return 1;
    }
    
    // 获取文件时间戳
    uint64_t src_mtime = cn_get_file_mtime(unit->source_path);
    uint64_t out_mtime = cn_get_file_mtime(unit->impl_path);
    
    // 源文件不存在也需要报错
    if (src_mtime == 0) {
        return 1;
    }
    
    // 输出文件不存在，需要编译
    if (out_mtime == 0) {
        return 1;
    }
    
    // 比较时间戳
    return (src_mtime > out_mtime) ? 1 : 0;
}

int cn_multi_compile_execute(CnMultiFileCompileContext *ctx) {
    if (!ctx) {
        return -1;
    }
    
    int result = 0;
    
    // 1. 分析初始化顺序
    if (ctx->loader) {
        CnModuleId **order = NULL;
        size_t order_count = 0;
        if (cn_module_loader_get_init_order(ctx->loader, &order, &order_count)) {
            ctx->init_order = order;
            ctx->init_order_count = order_count;
        }
    }
    
    // 2. 更新每个单元的重建状态
    for (size_t i = 0; i < ctx->unit_count; i++) {
        ctx->units[i].needs_rebuild = cn_cgen_needs_rebuild(&ctx->units[i]);
    }
    
    // 3. 为需要重建的模块生成代码
    for (size_t i = 0; i < ctx->unit_count; i++) {
        if (!ctx->units[i].needs_rebuild) {
            continue;
        }
        
        // 生成头文件
        if (cn_cgen_module_header(ctx, &ctx->units[i]) != 0) {
            result = -1;
            continue;
        }
        
        // 生成实现文件
        if (cn_cgen_module_impl(ctx, &ctx->units[i]) != 0) {
            result = -1;
            continue;
        }
    }
    
    // 4. 生成主入口文件（如果有入口模块）
    for (size_t i = 0; i < ctx->unit_count; i++) {
        if (ctx->units[i].is_entry) {
            char main_path[512];
            snprintf(main_path, sizeof(main_path), "%s/__cn_main__.c", ctx->output_dir);
            
            FILE *main_file = fopen(main_path, "w");
            if (main_file) {
                fprintf(main_file, "/* CN 语言程序入口 - 自动生成 */\n\n");
                
                // 包含所有模块头文件
                for (size_t j = 0; j < ctx->unit_count; j++) {
                    char mod_name[256];
                    cn_sanitize_module_name(ctx->units[j].module_id, 
                                             mod_name, sizeof(mod_name));
                    fprintf(main_file, "#include \"%s.h\"\n", mod_name);
                }
                fprintf(main_file, "\n");
                
                // 生成初始化序列
                cn_cgen_init_call_sequence(ctx, main_file);
                
                // 生成 main 函数
                fprintf(main_file, "int main(int argc, char **argv) {\n");
                fprintf(main_file, "    cn_rt_cli_init(argc, argv);\n");  // 初始化命令行参数
                fprintf(main_file, "    __cn_init_all_modules__();\n");
                
                // 调用入口模块的 main 函数
                char entry_name[256];
                cn_sanitize_module_name(ctx->units[i].module_id, 
                                         entry_name, sizeof(entry_name));
                fprintf(main_file, "    return %s__main();\n", entry_name);
                fprintf(main_file, "}\n");
                
                fclose(main_file);
            }
            break;  // 只处理第一个入口模块
        }
    }
    
    return result;
}

// ============================================================================
// 辅助函数实现
// ============================================================================

static char *cn_alloc_string(const char *src, size_t len) {
    if (!src) return NULL;
    char *dst = malloc(len + 1);
    if (!dst) return NULL;
    memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

static uint64_t cn_get_file_mtime(const char *path) {
    if (!path) return 0;
    
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &attr)) {
        return 0;
    }
    ULARGE_INTEGER time;
    time.LowPart = attr.ftLastWriteTime.dwLowDateTime;
    time.HighPart = attr.ftLastWriteTime.dwHighDateTime;
    return time.QuadPart;
#else
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return (uint64_t)st.st_mtime;
#endif
}

static const char *cn_module_id_get_name(CnModuleId *id) {
    if (!id || !id->qualified_name) {
        return "";
    }
    return id->qualified_name;
}

static char *cn_generate_output_path(const char *output_dir,
                                      CnModuleId *module_id,
                                      const char *extension) {
    if (!output_dir || !module_id || !extension) {
        return NULL;
    }
    
    char module_name[256];
    cn_sanitize_module_name(module_id, module_name, sizeof(module_name));
    
    size_t dir_len = strlen(output_dir);
    size_t name_len = strlen(module_name);
    size_t ext_len = strlen(extension);
    
    // 格式: output_dir/module_name.extension
    char *path = malloc(dir_len + 1 + name_len + ext_len + 1);
    if (!path) return NULL;
    
    snprintf(path, dir_len + 1 + name_len + ext_len + 1, 
             "%s/%s%s", output_dir, module_name, extension);
    
    return path;
}

static void cn_sanitize_module_name(CnModuleId *module_id,
                                     char *out_buffer,
                                     size_t buffer_size) {
    if (!module_id || !out_buffer || buffer_size == 0) {
        if (out_buffer && buffer_size > 0) out_buffer[0] = '\0';
        return;
    }
    
    // 获取完全限定名
    const char *fqn = cn_module_id_get_name(module_id);
    if (!fqn || fqn[0] == '\0') {
        out_buffer[0] = '\0';
        return;
    }
    
    size_t fqn_len = strlen(fqn);
    size_t out_idx = 0;
    
    for (size_t i = 0; i < fqn_len && out_idx < buffer_size - 1; i++) {
        unsigned char c = (unsigned char)fqn[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9')) {
            out_buffer[out_idx++] = c;
        } else if (c == '.') {
            out_buffer[out_idx++] = '_';
        } else if (c >= 0x80) {
            // UTF-8 多字节字符 - 使用十六进制编码
            if (out_idx + 3 < buffer_size) {
                out_buffer[out_idx++] = '_';
                snprintf(out_buffer + out_idx, 3, "%02x", c);
                out_idx += 2;
            }
        } else {
            out_buffer[out_idx++] = '_';
        }
    }
    
    out_buffer[out_idx] = '\0';
}
