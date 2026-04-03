#include "cnlang/backend/cgen.h"
#include "cnlang/backend/cgen/class_cgen.h"  // 类代码生成接口
#include "cnlang/backend/template_cgen.h"    // 模板代码生成接口
#include "cnlang/support/diagnostics.h"
#include "cnlang/support/config.h"
#include "cnlang/frontend/semantics.h"  // 引入作用域和类型信息
#include "cnlang/frontend/ast/class_node.h"  // 类AST节点定义
#include "cnlang/runtime/cli.h"              // 命令行参数接口
#include "cnlang/frontend/module_loader.h"   // 模块加载器接口
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/mman.h>
#endif

/* 性能优化：输出缓冲区大小 */
#define CGEN_BUFFER_SIZE 8192

static CnTargetDataLayout g_target_layout;
static bool g_target_layout_valid = false;

/* 运行时宏冲突检测：检测函数名是否与运行时宏定义冲突 */
static bool is_runtime_macro_conflict(const char *func_name) {
    if (!func_name) return false;
    
    /* 运行时宏定义的中文函数名列表（来自 runtime.h） */
    static const char *runtime_macros[] = {
        "初始化运行时", "退出运行时",
        "打印整数", "打印小数", "打印布尔", "打印换行",
        "连接字符串新", "字符串长度", "整数转字符串", "小数转字符串", "布尔转字符串",
        "分配数组", "获取数组长度", "检查数组越界", "释放数组", "设置数组元素", "获取数组元素",
        "取子串", "字符串比较", "查找子串位置", "去除空白", "格式化字符串新",
        NULL
    };
    
    for (int i = 0; runtime_macros[i]; i++) {
        if (strcmp(func_name, runtime_macros[i]) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 检查表达式是否为字符串类型
 */
static bool cgen_is_string_expr(CnAstExpr *expr) {
    if (!expr) return false;
    
    // 检查表达式本身的类型
    if (expr->type && expr->type->kind == CN_TYPE_STRING) {
        return true;
    }
    
    // 对于字符串字面量，直接返回true
    if (expr->kind == CN_AST_EXPR_STRING_LITERAL) {
        return true;
    }
    
    return false;
}

/**
 * @brief 检查二元表达式是否为字符串拼接
 */
static bool cgen_is_string_concat(CnAstExpr *expr) {
    if (!expr || expr->kind != CN_AST_EXPR_BINARY) return false;
    if (expr->as.binary.op != CN_AST_BINARY_OP_ADD) return false;
    
    // 检查表达式类型
    if (expr->type && expr->type->kind == CN_TYPE_STRING) {
        return true;
    }
    
    // 检查任一操作数是否为字符串
    if (cgen_is_string_expr(expr->as.binary.left) || cgen_is_string_expr(expr->as.binary.right)) {
        return true;
    }
    
    return false;
}

// --- 辅助函数 ---

const char *get_c_type_string(CnType *type) {
    if (!type) return "void";
    switch (type->kind) {
        case CN_TYPE_INT:
            // CN语言规范：整数类型建议对应 C 的 long long 以保证 64 位
            return "long long";
        case CN_TYPE_FLOAT: return "double";
        case CN_TYPE_BOOL: return "_Bool";
        case CN_TYPE_STRING: return "char*";
        
        // 新增的数字类型（支持数字字面量后缀）
        case CN_TYPE_INT32: return "int";
        case CN_TYPE_INT64: return "long long";
        case CN_TYPE_UINT32: return "unsigned int";
        case CN_TYPE_UINT64: return "unsigned long long";
        case CN_TYPE_UINT64_LL: return "unsigned long long";
        case CN_TYPE_FLOAT32: return "float";
        case CN_TYPE_FLOAT64: return "double";
        
        case CN_TYPE_POINTER: {
            // 检查是否是函数指针类型
            if (type->as.pointer_to && type->as.pointer_to->kind == CN_TYPE_FUNCTION) {
                // 函数指针类型：返回类型(*)(param1_type, param2_type, ...)
                static _Thread_local char buffer[512];
                CnType *func_type = type->as.pointer_to;
                
                char params_str[256] = {0};
                if (func_type->as.function.param_count == 0) {
                    snprintf(params_str, sizeof(params_str), "void");
                } else {
                    size_t offset = 0;
                    for (size_t i = 0; i < func_type->as.function.param_count; i++) {
                        const char *param_type = get_c_type_string(func_type->as.function.param_types[i]);
                        offset += snprintf(params_str + offset, sizeof(params_str) - offset, "%s", param_type);
                        if (i < func_type->as.function.param_count - 1) {
                            offset += snprintf(params_str + offset, sizeof(params_str) - offset, ", ");
                        }
                    }
                }
                
                snprintf(buffer, sizeof(buffer), "%s(*)(%s)", 
                         get_c_type_string(func_type->as.function.return_type), 
                         params_str);
                return buffer;
            }
            
            /* 普通指针类型 */
            /* 性能优化：使用线程局部缓冲区避免静态缓冲区竞争 */
            static _Thread_local char buffer[256];
            snprintf(buffer, sizeof(buffer), "%s*", get_c_type_string(type->as.pointer_to));
            return buffer;
        }
        case CN_TYPE_VOID: return "void";
        case CN_TYPE_ARRAY: {
            /* 数组类型：生成元素指针类型 */
            /* 对于结构体字段，动态数组（长度为0）生成指针类型 */
            /* 固定大小数组也生成指针类型，因为C结构体不支持变长数组字段 */
            static _Thread_local char buffer[256];
            snprintf(buffer, sizeof(buffer), "%s*", get_c_type_string(type->as.array.element_type));
            return buffer;
        }
        case CN_TYPE_STRUCT: {
            /* 结构体类型：检查是否是局部结构体 */
            static _Thread_local char buffer[512];
            
            // 检查是否是局部结构体（通过owner_func_name判断）
            // 注意：不再依赖decl_scope指针，因为它可能指向已释放的内存
            if (type->as.struct_type.owner_func_name) {
                // 局部结构体：生成 __local_函数名_结构体名
                snprintf(buffer, sizeof(buffer), "struct __local_%.*s_%.*s", 
                         (int)type->as.struct_type.owner_func_name_length, 
                         type->as.struct_type.owner_func_name,
                         (int)type->as.struct_type.name_length, 
                         type->as.struct_type.name);
            } else {
                // 全局/模块结构体
                snprintf(buffer, sizeof(buffer), "struct %.*s", 
                         (int)type->as.struct_type.name_length, 
                         type->as.struct_type.name);
            }
            return buffer;
        }
        case CN_TYPE_ENUM: {
            /* 枚举类型：返回枚举名 */
            static _Thread_local char buffer[256];
            snprintf(buffer, sizeof(buffer), "enum %.*s", 
                     (int)type->as.enum_type.name_length, 
                     type->as.enum_type.name);
            return buffer;
        }
        case CN_TYPE_MEMORY_ADDRESS: {
            /* 内存地址类型：对应 uintptr_t */
            return "uintptr_t";
        }
        case CN_TYPE_SELF:
            /* 自身类型：接口方法中使用 void* 作为占位符 */
            return "void*";
        default: return "int";
    }
}

static const char *get_c_function_name(const char *name) {
    if (!name) return "unnamed_func";
    
    /* 性能优化：提前返回已知的运行时函数名 */
    if (strncmp(name, "cn_rt_", 6) == 0) {
        return name;
    }
    
    /* 性能优化：使用查找表加速常用中文函数名映射 */
    static const struct {
        const char *utf8_name;
        const char *c_name;
    } function_map[] = {
        {"\xe6\x89\x93\xe5\x8d\xb0", "cn_rt_print_string"},  /* 打印 */
        {"\xe9\x95\xbf\xe5\xba\xa6", "cn_rt_string_length"}, /* 长度 */
        {"\xe4\xb8\xbb\xe7\xa8\x8b\xe5\xba\x8f", "main"},      /* 主程序 */
        {"\xe4\xb8\xbb\xe5\x87\xbd\xe6\x95\xb0", "main"},      /* 主函数 */
        /* 输入函数映射 */
        {"\xe8\xaf\xbb\xe5\x8f\x96\xe8\xa1\x8c", "cn_rt_read_line"},         /* 读取行 */
        {"\xe8\xaf\xbb\xe5\x8f\x96\xe6\x95\xb4\xe6\x95\xb0", "cn_rt_read_int"},     /* 读取整数 */
        {"\xe8\xaf\xbb\xe5\x8f\x96\xe5\xb0\x8f\xe6\x95\xb0", "cn_rt_read_float"},   /* 读取小数 */
        {"\xe8\xaf\xbb\xe5\x8f\x96\xe5\xad\x97\xe7\xac\xa6\xe4\xb8\xb2", "cn_rt_read_string"}, /* 读取字符串 */
        {"\xe8\xaf\xbb\xe5\x8f\x96\xe5\xad\x97\xe7\xac\xa6", "cn_rt_read_char"},   /* 读取字符 */
        /* 通用读取函数映射 */
        {"\xe8\xaf\xbb\xe5\x8f\x96", "cn_rt_read"},           /* 读取 */
        {"\xe6\x98\xaf\xe6\x95\xb4\xe6\x95\xb0", "cn_rt_input_is_int"},     /* 是整数 */
        {"\xe6\x98\xaf\xe5\xb0\x8f\xe6\x95\xb0", "cn_rt_input_is_float"},   /* 是小数 */
        {"\xe6\x98\xaf\xe5\xad\x97\xe7\xac\xa6\xe4\xb8\xb2", "cn_rt_input_is_string"}, /* 是字符串 */
        {"\xe6\x98\xaf\xe6\x95\xb0\xe5\x80\xbc", "cn_rt_input_is_number"},  /* 是数值 */
        {"\xe5\x8f\x96\xe6\x95\xb4\xe6\x95\xb0", "cn_rt_input_to_int"},     /* 取整数 */
        {"\xe5\x8f\x96\xe5\xb0\x8f\xe6\x95\xb0", "cn_rt_input_to_float"},   /* 取小数 */
        {"\xe5\x8f\x96\xe6\x96\x87\xe6\x9c\xac", "cn_rt_input_to_string"},  /* 取文本 */
        {"\xe9\x87\x8a\xe6\x94\xbe\xe8\xbe\x93\xe5\x85\xa5", "cn_rt_input_free"},  /* 释放输入 */
        /* 字符串转换函数映射 */
        {"\xe8\xbd\xac\xe6\x95\xb4\xe6\x95\xb0", "cn_rt_str_to_int"},      /* 转整数 */
        {"\xe8\xbd\xac\xe5\xb0\x8f\xe6\x95\xb0", "cn_rt_str_to_float"},    /* 转小数 */
        {"\xe6\x98\xaf\xe6\x95\xb0\xe5\xad\x97\xe6\x96\x87\xe6\x9c\xac", "cn_rt_is_numeric_str"},  /* 是数字文本 */
        {"\xe6\x98\xaf\xe6\x95\xb4\xe6\x95\xb0\xe6\x96\x87\xe6\x9c\xac", "cn_rt_is_int_str"},      /* 是整数文本 */
        {NULL, NULL}
    };
    
    for (int i = 0; function_map[i].utf8_name; i++) {
        if (strcmp(name, function_map[i].utf8_name) == 0) {
            return function_map[i].c_name;
        }
    }
    
    /* 性能优化：使用线程局部缓冲区 */
    static _Thread_local char buffer[256];
    /* 方案C：未映射的中文函数直接使用原名称，不添加 cn_func_ 前缀 */
    /* 这样 stdlib.h 中定义的中文函数（如 分配内存、释放内存）可以直接被调用 */
    snprintf(buffer, sizeof(buffer), "%s", name);
    return buffer;
}

// 获取静态变量的C名称
static const char *get_static_var_name(const char *func_name, const char *var_name) {
    static _Thread_local char buffer[256];
    snprintf(buffer, sizeof(buffer), "cn_static_%s_%s", func_name, var_name);
    return buffer;
}

static const char *get_c_variable_name(const char *name) {
    /* 性能优化：使用线程局部缓冲区 */
    static _Thread_local char buffer[256];
    
    // 检查是否已经有 cn_module_、cn_var_ 或 cn_static_ 前缀（避免重复加前缀）
    if (strncmp(name, "cn_module_", 10) == 0 ||
        strncmp(name, "cn_var_", 7) == 0 ||
        strncmp(name, "cn_static_", 10) == 0) {
        return name;
    }
    
    // 检查是否为模块成员名（包含双下划线 "__"）
    const char *delimiter = strstr(name, "__");
    if (delimiter) {
        // 模块成员名：模块名__成员名 -> cn_module_模块名__成员名
        snprintf(buffer, sizeof(buffer), "cn_module_%s", name);
    } else {
        // 普通变量名
        snprintf(buffer, sizeof(buffer), "cn_var_%s", name);
    }
    return buffer;
}

// 前向声明
static void cn_cgen_expr_simple(CnCCodeGenContext *ctx, CnAstExpr *expr);

static void print_operand(CnCCodeGenContext *ctx, CnIrOperand op) {
    switch (op.kind) {
        case CN_IR_OP_NONE: fprintf(ctx->output_file, "/* NONE */"); break;
        case CN_IR_OP_REG: fprintf(ctx->output_file, "r%d", op.as.reg_id); break;
        case CN_IR_OP_IMM_INT: fprintf(ctx->output_file, "%lld", op.as.imm_int); break;
        case CN_IR_OP_IMM_FLOAT: fprintf(ctx->output_file, "%f", op.as.imm_float); break;
        case CN_IR_OP_IMM_STR: 
            // 字符串字面量：需要加引号并处理转义
            fprintf(ctx->output_file, "\"");
            for (const char *p = op.as.imm_str; p && *p; p++) {
                switch (*p) {
                    case '\\': fprintf(ctx->output_file, "\\\\"); break;
                    case '\"': fprintf(ctx->output_file, "\\\""); break;
                    case '\n': fprintf(ctx->output_file, "\\n"); break;
                    case '\r': fprintf(ctx->output_file, "\\r"); break;
                    case '\t': fprintf(ctx->output_file, "\\t"); break;
                    default: fprintf(ctx->output_file, "%c", *p); break;
                }
            }
            fprintf(ctx->output_file, "\"");
            break;
        case CN_IR_OP_SYMBOL: fprintf(ctx->output_file, "%s", get_c_variable_name(op.as.sym_name)); break;
        case CN_IR_OP_LABEL: fprintf(ctx->output_file, "%s", op.as.label->name ? op.as.label->name : "unnamed_label"); break;
        case CN_IR_OP_AST_EXPR:
            // AST表达式：直接生成 C 代码（用于结构体字面量等）
            cn_cgen_expr_simple(ctx, op.as.ast_expr);
            break;
        default: fprintf(ctx->output_file, "/* UNKNOWN */"); break;
    }
}

// 生成简单表达式的 C 代码（用于模块变量初始化）
static void cn_cgen_expr_simple(CnCCodeGenContext *ctx, CnAstExpr *expr) {
    if (!ctx || !expr) return;
    
    switch (expr->kind) {
        case CN_AST_EXPR_INTEGER_LITERAL:
            fprintf(ctx->output_file, "%lld", expr->as.integer_literal.value);
            break;
        case CN_AST_EXPR_FLOAT_LITERAL:
            fprintf(ctx->output_file, "%f", expr->as.float_literal.value);
            break;
        case CN_AST_EXPR_STRING_LITERAL:
            // 输出字符串字面量，需要处理转义
            fprintf(ctx->output_file, "\"");
            for (const char *p = expr->as.string_literal.value; *p; p++) {
                switch (*p) {
                    case '\\': fprintf(ctx->output_file, "\\\\"); break;
                    case '\"': fprintf(ctx->output_file, "\\\""); break;
                    case '\n': fprintf(ctx->output_file, "\\n"); break;
                    case '\r': fprintf(ctx->output_file, "\\r"); break;
                    case '\t': fprintf(ctx->output_file, "\\t"); break;
                    default: fprintf(ctx->output_file, "%c", *p); break;
                }
            }
            fprintf(ctx->output_file, "\"");
            break;
        case CN_AST_EXPR_BOOL_LITERAL:
            fprintf(ctx->output_file, "%s", expr->as.bool_literal.value ? "true" : "false");
            break;
        case CN_AST_EXPR_IDENTIFIER:
            // 检查是否为自身指针（this/self）
            if (expr->is_this_pointer) {
                // 自身指针直接输出 "self"
                fprintf(ctx->output_file, "self");
            } else if (expr->is_base_pointer) {
                // 基类指针：输出 "base" 标识符
                fprintf(ctx->output_file, "base");
            } else {
                // 支持引用其他模块变量
                // 先简单处理：直接输出变量名（需要结合符号表处理模块前缀）
                fprintf(ctx->output_file, "cn_var_%.*s",
                        (int)expr->as.identifier.name_length,
                        expr->as.identifier.name);
            }
            break;
        case CN_AST_EXPR_MEMBER_ACCESS:
            // 成员访问表达式：obj.member 或 ptr->member 或 类名.静态成员 或 基类.成员
            {
                // 检查是否为静态成员访问
                if (expr->as.member.is_static_member && expr->as.member.class_name) {
                    // 静态成员访问：生成 类名_成员名
                    fprintf(ctx->output_file, "%.*s_%.*s",
                            (int)expr->as.member.class_name_length, expr->as.member.class_name,
                            (int)expr->as.member.member_name_length, expr->as.member.member_name);
                }
                // 检查对象是否为自身指针
                else if (expr->as.member.object &&
                    expr->as.member.object->is_this_pointer) {
                    // 自身.成员 -> self->成员
                    fprintf(ctx->output_file, "self->%.*s",
                            (int)expr->as.member.member_name_length,
                            expr->as.member.member_name);
                }
                // 检查对象是否为基类指针
                else if (expr->as.member.object &&
                    expr->as.member.object->is_base_pointer) {
                    // 基类访问：base->member
                    // 生成基类指针转换代码
                    if (ctx->current_class && ctx->current_class->base_count > 0) {
                        CnInheritanceInfo *base_info = &ctx->current_class->bases[0];
                        fprintf(ctx->output_file, "((struct %.*s*)((char*)self + %.*s_%.*s_OFFSET))->%.*s",
                                (int)base_info->base_class_name_length, base_info->base_class_name,
                                (int)ctx->current_class->name_length, ctx->current_class->name,
                                (int)base_info->base_class_name_length, base_info->base_class_name,
                                (int)expr->as.member.member_name_length,
                                expr->as.member.member_name);
                    } else {
                        // 没有基类，生成错误占位符
                        fprintf(ctx->output_file, "/* 错误: 当前类没有基类 */ %.*s",
                                (int)expr->as.member.member_name_length,
                                expr->as.member.member_name);
                    }
                } else {
                    // 普通成员访问
                    cn_cgen_expr_simple(ctx, expr->as.member.object);
                    if (expr->as.member.is_arrow) {
                        fprintf(ctx->output_file, "->");
                    } else {
                        fprintf(ctx->output_file, ".");
                    }
                    fprintf(ctx->output_file, "%.*s",
                            (int)expr->as.member.member_name_length,
                            expr->as.member.member_name);
                }
            }
            break;
        case CN_AST_EXPR_BINARY:
            // 检查是否为字符串拼接
            if (cgen_is_string_concat(expr)) {
                fprintf(ctx->output_file, "cn_rt_string_concat(");
                cn_cgen_expr_simple(ctx, expr->as.binary.left);
                fprintf(ctx->output_file, ", ");
                cn_cgen_expr_simple(ctx, expr->as.binary.right);
                fprintf(ctx->output_file, ")");
                break;
            }
            fprintf(ctx->output_file, "(");
            cn_cgen_expr_simple(ctx, expr->as.binary.left);
            
            // 生成运算符
            switch (expr->as.binary.op) {
                case CN_AST_BINARY_OP_ADD: fprintf(ctx->output_file, " + "); break;
                case CN_AST_BINARY_OP_SUB: fprintf(ctx->output_file, " - "); break;
                case CN_AST_BINARY_OP_MUL: fprintf(ctx->output_file, " * "); break;
                case CN_AST_BINARY_OP_DIV: fprintf(ctx->output_file, " / "); break;
                case CN_AST_BINARY_OP_MOD: fprintf(ctx->output_file, " %% "); break;
                case CN_AST_BINARY_OP_EQ: fprintf(ctx->output_file, " == "); break;
                case CN_AST_BINARY_OP_NE: fprintf(ctx->output_file, " != "); break;
                case CN_AST_BINARY_OP_LT: fprintf(ctx->output_file, " < "); break;
                case CN_AST_BINARY_OP_GT: fprintf(ctx->output_file, " > "); break;
                case CN_AST_BINARY_OP_LE: fprintf(ctx->output_file, " <= "); break;
                case CN_AST_BINARY_OP_GE: fprintf(ctx->output_file, " >= "); break;
                case CN_AST_BINARY_OP_BITWISE_AND: fprintf(ctx->output_file, " & "); break;
                case CN_AST_BINARY_OP_BITWISE_OR: fprintf(ctx->output_file, " | "); break;
                case CN_AST_BINARY_OP_BITWISE_XOR: fprintf(ctx->output_file, " ^ "); break;
                case CN_AST_BINARY_OP_LEFT_SHIFT: fprintf(ctx->output_file, " << "); break;
                case CN_AST_BINARY_OP_RIGHT_SHIFT: fprintf(ctx->output_file, " >> "); break;
                default: fprintf(ctx->output_file, " ? "); break;
            }
            
            cn_cgen_expr_simple(ctx, expr->as.binary.right);
            fprintf(ctx->output_file, ")");
            break;
        case CN_AST_EXPR_UNARY:
            // 一元运算
            switch (expr->as.unary.op) {
                case CN_AST_UNARY_OP_MINUS: fprintf(ctx->output_file, "-"); break;
                case CN_AST_UNARY_OP_NOT: fprintf(ctx->output_file, "!"); break;
                case CN_AST_UNARY_OP_BITWISE_NOT: fprintf(ctx->output_file, "~"); break;
                case CN_AST_UNARY_OP_PRE_INC:
                    fprintf(ctx->output_file, "++");
                    cn_cgen_expr_simple(ctx, expr->as.unary.operand);
                    return;
                case CN_AST_UNARY_OP_PRE_DEC:
                    fprintf(ctx->output_file, "--");
                    cn_cgen_expr_simple(ctx, expr->as.unary.operand);
                    return;
                case CN_AST_UNARY_OP_POST_INC:
                    cn_cgen_expr_simple(ctx, expr->as.unary.operand);
                    fprintf(ctx->output_file, "++");
                    return;
                case CN_AST_UNARY_OP_POST_DEC:
                    cn_cgen_expr_simple(ctx, expr->as.unary.operand);
                    fprintf(ctx->output_file, "--");
                    return;
                default: break;
            }
            cn_cgen_expr_simple(ctx, expr->as.unary.operand);
            break;
        case CN_AST_EXPR_TERNARY:
            // 三元运算符: condition ? true_expr : false_expr
            fprintf(ctx->output_file, "(");
            cn_cgen_expr_simple(ctx, expr->as.ternary.condition);
            fprintf(ctx->output_file, " ? ");
            cn_cgen_expr_simple(ctx, expr->as.ternary.true_expr);
            fprintf(ctx->output_file, " : ");
            cn_cgen_expr_simple(ctx, expr->as.ternary.false_expr);
            fprintf(ctx->output_file, ")");
            break;
        case CN_AST_EXPR_MEMORY_READ:
            // 读取内存: 读取内存(地址) -> *(uintptr_t*)addr
            fprintf(ctx->output_file, "*(uintptr_t*)");
            cn_cgen_expr_simple(ctx, expr->as.memory_read.address);
            break;
        case CN_AST_EXPR_MEMORY_WRITE:
            // 写入内存: 写入内存(地址, 值) -> *(uintptr_t*)addr = value
            fprintf(ctx->output_file, "*(uintptr_t*)");
            cn_cgen_expr_simple(ctx, expr->as.memory_write.address);
            fprintf(ctx->output_file, " = ");
            cn_cgen_expr_simple(ctx, expr->as.memory_write.value);
            break;
        case CN_AST_EXPR_MEMORY_COPY:
            // 内存复制: 内存复制(目标, 源, 大小) -> memcpy(dest, src, size)
            fprintf(ctx->output_file, "memcpy(");
            cn_cgen_expr_simple(ctx, expr->as.memory_copy.dest);
            fprintf(ctx->output_file, ", ");
            cn_cgen_expr_simple(ctx, expr->as.memory_copy.src);
            fprintf(ctx->output_file, ", ");
            cn_cgen_expr_simple(ctx, expr->as.memory_copy.size);
            fprintf(ctx->output_file, ")");
            break;
        case CN_AST_EXPR_MEMORY_SET:
            // 内存设置: 内存设置(地址, 值, 大小) -> memset(addr, value, size)
            fprintf(ctx->output_file, "memset(");
            cn_cgen_expr_simple(ctx, expr->as.memory_set.address);
            fprintf(ctx->output_file, ", ");
            cn_cgen_expr_simple(ctx, expr->as.memory_set.value);
            fprintf(ctx->output_file, ", ");
            cn_cgen_expr_simple(ctx, expr->as.memory_set.size);
            fprintf(ctx->output_file, ")");
            break;
        case CN_AST_EXPR_MEMORY_MAP:
            // 内存映射: 映射内存(地址, 大小, 保护, 标志) -> mmap(addr, size, prot, flags, -1, 0)
            fprintf(ctx->output_file, "mmap(");
            if (expr->as.memory_map.address) {
                cn_cgen_expr_simple(ctx, expr->as.memory_map.address);
            } else {
                fprintf(ctx->output_file, "NULL");
            }
            fprintf(ctx->output_file, ", ");
            cn_cgen_expr_simple(ctx, expr->as.memory_map.size);
            fprintf(ctx->output_file, ", ");
            cn_cgen_expr_simple(ctx, expr->as.memory_map.prot);
            fprintf(ctx->output_file, ", ");
            cn_cgen_expr_simple(ctx, expr->as.memory_map.flags);
            fprintf(ctx->output_file, ", -1, 0)");
            break;
        case CN_AST_EXPR_MEMORY_UNMAP:
            // 解除内存映射: 解除映射(地址, 大小) -> munmap(addr, size)
            fprintf(ctx->output_file, "munmap(");
            cn_cgen_expr_simple(ctx, expr->as.memory_unmap.address);
            fprintf(ctx->output_file, ", ");
            cn_cgen_expr_simple(ctx, expr->as.memory_unmap.size);
            fprintf(ctx->output_file, ")");
            break;
        case CN_AST_EXPR_INLINE_ASM:
            // 内联汇编: inline_asm("code", outputs, inputs, clobbers)
            fprintf(ctx->output_file, "asm volatile (");
            cn_cgen_expr_simple(ctx, expr->as.inline_asm.asm_code);
            fprintf(ctx->output_file, "\n");
            
            // 输出操作数
            if (expr->as.inline_asm.outputs && expr->as.inline_asm.output_count > 0) {
                fprintf(ctx->output_file, "    : ");
                for (size_t i = 0; i < expr->as.inline_asm.output_count; i++) {
                    if (i > 0) {
                        fprintf(ctx->output_file, ", ");
                    }
                    fprintf(ctx->output_file, "\"=r\"(");
                    cn_cgen_expr_simple(ctx, expr->as.inline_asm.outputs[i]);
                    fprintf(ctx->output_file, ")");
                }
                fprintf(ctx->output_file, "\n");
            }
            
            // 输入操作数
            if (expr->as.inline_asm.inputs && expr->as.inline_asm.input_count > 0) {
                fprintf(ctx->output_file, "    : ");
                for (size_t i = 0; i < expr->as.inline_asm.input_count; i++) {
                    if (i > 0) {
                        fprintf(ctx->output_file, ", ");
                    }
                    fprintf(ctx->output_file, "\"r\"(");
                    cn_cgen_expr_simple(ctx, expr->as.inline_asm.inputs[i]);
                    fprintf(ctx->output_file, ")");
                }
                fprintf(ctx->output_file, "\n");
            }
            
            // 破坏列表
            if (expr->as.inline_asm.clobbers) {
                fprintf(ctx->output_file, "    : ");
                cn_cgen_expr_simple(ctx, expr->as.inline_asm.clobbers);
                fprintf(ctx->output_file, "\n");
            }
            
            fprintf(ctx->output_file, ");");
            break;
        case CN_AST_EXPR_STRUCT_LITERAL:
            // 结构体字面量：点 { x: 10, y: 20 } -> (struct 点){.x = 10, .y = 20}
            // 使用expr->type中的类型信息来决定如何生成
            {
                if (expr->type && expr->type->kind == CN_TYPE_STRUCT) {
                    // 使用get_c_type_string来生成结构体类型名（已经考虑了局部/全局区分）
                    const char *c_type = get_c_type_string(expr->type);
                    fprintf(ctx->output_file, "(%s){", c_type);
                } else {
                    // fallback: 使用结构体名
                    fprintf(ctx->output_file, "(struct %.*s){",
                            (int)expr->as.struct_lit.struct_name_length,
                            expr->as.struct_lit.struct_name);
                }
                for (size_t i = 0; i < expr->as.struct_lit.field_count; i++) {
                    if (i > 0) {
                        fprintf(ctx->output_file, ", ");
                    }
                    fprintf(ctx->output_file, ".%.*s = ",
                            (int)expr->as.struct_lit.fields[i].field_name_length,
                            expr->as.struct_lit.fields[i].field_name);
                    cn_cgen_expr_simple(ctx, expr->as.struct_lit.fields[i].value);
                }
                fprintf(ctx->output_file, "}");
            }
            break;
        default:
            fprintf(ctx->output_file, "0"); // 不支持的表达式，用0作为默认值
            break;
    }
}

// --- 核心生成函数 ---

void cn_cgen_inst(CnCCodeGenContext *ctx, CnIrInst *inst) {
    if (!ctx || !inst) return;
    switch (inst->kind) {
        case CN_IR_INST_LABEL: fprintf(ctx->output_file, "  %s:\n", inst->dest.as.sym_name); break;
        case CN_IR_INST_ALLOCA: fprintf(ctx->output_file, "  %s %s;\n", get_c_type_string(inst->dest.type), get_c_variable_name(inst->dest.as.sym_name)); break;
        case CN_IR_INST_LOAD: fprintf(ctx->output_file, "  "); print_operand(ctx, inst->dest); fprintf(ctx->output_file, " = "); print_operand(ctx, inst->src1); fprintf(ctx->output_file, ";\n"); break;
        case CN_IR_INST_STORE: fprintf(ctx->output_file, "  "); print_operand(ctx, inst->dest); fprintf(ctx->output_file, " = "); print_operand(ctx, inst->src1); fprintf(ctx->output_file, ";\n"); break;
        case CN_IR_INST_ADD:
        case CN_IR_INST_SUB:
        case CN_IR_INST_MUL:
        case CN_IR_INST_DIV:
        case CN_IR_INST_MOD:
        case CN_IR_INST_EQ:
        case CN_IR_INST_NE:
        case CN_IR_INST_LT:
        case CN_IR_INST_GT:
        case CN_IR_INST_LE:
        case CN_IR_INST_GE:
        case CN_IR_INST_AND:
        case CN_IR_INST_OR:
        case CN_IR_INST_XOR:
        case CN_IR_INST_SHL:
        case CN_IR_INST_SHR:
            fprintf(ctx->output_file, "  "); print_operand(ctx, inst->dest); fprintf(ctx->output_file, " = "); print_operand(ctx, inst->src1);
            switch (inst->kind) {
                case CN_IR_INST_ADD: fprintf(ctx->output_file, " + "); break;
                case CN_IR_INST_SUB: fprintf(ctx->output_file, " - "); break;
                case CN_IR_INST_MUL: fprintf(ctx->output_file, " * "); break;
                case CN_IR_INST_DIV: fprintf(ctx->output_file, " / "); break;
                case CN_IR_INST_MOD: fprintf(ctx->output_file, " %% "); break;
                case CN_IR_INST_EQ: fprintf(ctx->output_file, " == "); break;
                case CN_IR_INST_NE: fprintf(ctx->output_file, " != "); break;
                case CN_IR_INST_LT: fprintf(ctx->output_file, " < "); break;
                case CN_IR_INST_GT: fprintf(ctx->output_file, " > "); break;
                case CN_IR_INST_LE: fprintf(ctx->output_file, " <= "); break;
                case CN_IR_INST_GE: fprintf(ctx->output_file, " >= "); break;
                case CN_IR_INST_AND: fprintf(ctx->output_file, " & "); break;
                case CN_IR_INST_OR: fprintf(ctx->output_file, " | "); break;
                case CN_IR_INST_XOR: fprintf(ctx->output_file, " ^ "); break;
                case CN_IR_INST_SHL: fprintf(ctx->output_file, " << "); break;
                case CN_IR_INST_SHR: fprintf(ctx->output_file, " >> "); break;
                default: break;
            }
            print_operand(ctx, inst->src2); fprintf(ctx->output_file, ";\n"); break;
        case CN_IR_INST_RET: fprintf(ctx->output_file, "  return"); if (inst->src1.kind != CN_IR_OP_NONE) { fprintf(ctx->output_file, " "); print_operand(ctx, inst->src1); } fprintf(ctx->output_file, ";\n"); break;
        case CN_IR_INST_JUMP: fprintf(ctx->output_file, "  goto %s;\n", inst->dest.as.label->name); break;
        case CN_IR_INST_BRANCH: fprintf(ctx->output_file, "  if ("); print_operand(ctx, inst->src1); fprintf(ctx->output_file, ") goto %s; else goto %s;\n", inst->dest.as.label->name, inst->src2.as.label->name); break;
        case CN_IR_INST_CALL:
            fprintf(ctx->output_file, "  ");
            
            // 特殊处理运行时系统API函数
            // 这些函数替代了已删除的关键字(读取内存、写入内存、内存复制等)
            
            // 1. cn_rt_mem_read: 内存读取 (替代"读取内存"关键字)
            if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                strcmp(inst->src1.as.sym_name, "cn_rt_mem_read") == 0) {
                // 生成: dest = (type)cn_rt_mem_read(addr, size)
                if (inst->dest.kind != CN_IR_OP_NONE) {
                    print_operand(ctx, inst->dest);
                    fprintf(ctx->output_file, " = (%s)", get_c_type_string(inst->dest.type));
                }
                fprintf(ctx->output_file, "cn_rt_mem_read(");
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            // 2. cn_rt_mem_write: 内存写入 (替代"写入内存"关键字)
            else if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                strcmp(inst->src1.as.sym_name, "cn_rt_mem_write") == 0) {
                // 生成: cn_rt_mem_write(addr, value, size)
                fprintf(ctx->output_file, "cn_rt_mem_write(");
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            // 3. cn_rt_mem_copy: 内存复制 (替代"内存复制"关键字)
            else if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                strcmp(inst->src1.as.sym_name, "cn_rt_mem_copy") == 0) {
                // 生成: cn_rt_mem_copy(dest, src, size)
                fprintf(ctx->output_file, "cn_rt_mem_copy(");
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            // 4. cn_rt_mem_set: 内存设置 (替代"内存设置"关键字)
            else if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                strcmp(inst->src1.as.sym_name, "cn_rt_mem_set") == 0) {
                // 生成: cn_rt_mem_set(addr, value, size)
                fprintf(ctx->output_file, "cn_rt_mem_set(");
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            // 5. cn_rt_mem_map: 内存映射 (替代"映射内存"关键字)
            else if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                strcmp(inst->src1.as.sym_name, "cn_rt_mem_map") == 0) {
                // 生成: dest = cn_rt_mem_map(addr, size, prot, flags)
                if (inst->dest.kind != CN_IR_OP_NONE) {
                    print_operand(ctx, inst->dest);
                    fprintf(ctx->output_file, " = ");
                }
                fprintf(ctx->output_file, "cn_rt_mem_map(");
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            // 6. cn_rt_mem_unmap: 解除内存映射 (替代"解除映射"关键字)
            else if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                strcmp(inst->src1.as.sym_name, "cn_rt_mem_unmap") == 0) {
                // 生成: result = cn_rt_mem_unmap(addr, size)
                if (inst->dest.kind != CN_IR_OP_NONE) {
                    print_operand(ctx, inst->dest);
                    fprintf(ctx->output_file, " = ");
                }
                fprintf(ctx->output_file, "cn_rt_mem_unmap(");
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            // 7. cn_rt_inline_asm: 内联汇编 (替代"内联汇编"关键字)
            else if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                strcmp(inst->src1.as.sym_name, "cn_rt_inline_asm") == 0) {
                // 生成平台特定的内联汇编代码
                // 参数: asm_code, outputs, inputs, clobbers
                if (inst->extra_args_count >= 1 && 
                    inst->extra_args[0].kind == CN_IR_OP_IMM_STR) {
                    // 使用GCC内联汇编语法
                    fprintf(ctx->output_file, "__asm__ volatile (");
                    fprintf(ctx->output_file, "\"");
                    // 打印汇编代码(第一个参数)
                    for (const char *p = inst->extra_args[0].as.imm_str; p && *p; p++) {
                        if (*p == '\\') fprintf(ctx->output_file, "\\\\");
                        else if (*p == '\"') fprintf(ctx->output_file, "\\\"");
                        else fprintf(ctx->output_file, "%c", *p);
                    }
                    fprintf(ctx->output_file, "\"\n");
                    
                    // outputs (第二个参数)
                    fprintf(ctx->output_file, "  : ");
                    if (inst->extra_args_count >= 2 && 
                        inst->extra_args[1].kind == CN_IR_OP_IMM_STR &&
                        inst->extra_args[1].as.imm_str != NULL) {
                        fprintf(ctx->output_file, "\"%s\"", inst->extra_args[1].as.imm_str);
                    }
                    fprintf(ctx->output_file, "\n");
                    
                    // inputs (第三个参数)
                    fprintf(ctx->output_file, "  : ");
                    if (inst->extra_args_count >= 3 && 
                        inst->extra_args[2].kind == CN_IR_OP_IMM_STR &&
                        inst->extra_args[2].as.imm_str != NULL) {
                        fprintf(ctx->output_file, "\"%s\"", inst->extra_args[2].as.imm_str);
                    }
                    fprintf(ctx->output_file, "\n");
                    
                    // clobbers (第四个参数)
                    fprintf(ctx->output_file, "  : ");
                    if (inst->extra_args_count >= 4 && 
                        inst->extra_args[3].kind == CN_IR_OP_IMM_STR &&
                        inst->extra_args[3].as.imm_str != NULL) {
                        fprintf(ctx->output_file, "\"%s\"", inst->extra_args[3].as.imm_str);
                    }
                    fprintf(ctx->output_file, "\n);");
                } else {
                    // 汇编代码不是字符串字面量,生成占位注释
                    fprintf(ctx->output_file, "/* 内联汇编: 参数类型错误 */");
                }
                fprintf(ctx->output_file, "\n");
            }
            // 8. cn_rt_interrupt_register: 中断注册 (替代"中断处理"关键字)
            // 注意: 这个函数在main函数中自动生成(见L630),这里只处理显式调用
            else if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_register") == 0) {
                // 生成: result = cn_rt_interrupt_register(vector, handler, name)
                if (inst->dest.kind != CN_IR_OP_NONE) {
                    print_operand(ctx, inst->dest);
                    fprintf(ctx->output_file, " = ");
                }
                fprintf(ctx->output_file, "cn_rt_interrupt_register(");
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            // 9. 其他中断管理函数
            else if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                (strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_init") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_enable") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_disable") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_enable_all") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_disable_all") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_trigger") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_unregister") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_is_enabled") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_is_pending") == 0 ||
                 strcmp(inst->src1.as.sym_name, "cn_rt_interrupt_get_current") == 0)) {
                // 标准运行时函数调用,直接透传
                if (inst->dest.kind != CN_IR_OP_NONE) {
                    print_operand(ctx, inst->dest);
                    fprintf(ctx->output_file, " = ");
                }
                fprintf(ctx->output_file, "%s(", inst->src1.as.sym_name);
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            // 特殊处理 cn_rt_array_get_element：需要解引用返回的指针
            else if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                strcmp(inst->src1.as.sym_name, "cn_rt_array_get_element") == 0 &&
                inst->extra_args_count >= 3 &&
                inst->dest.kind != CN_IR_OP_NONE) {
                
                // 生成: dest = *(type*)cn_rt_array_get_element(arr, idx, size)
                print_operand(ctx, inst->dest);
                fprintf(ctx->output_file, " = *(%s*)", get_c_type_string(inst->dest.type));
                fprintf(ctx->output_file, "%s(", get_c_function_name(inst->src1.as.sym_name));
                
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            } else if (inst->src1.kind == CN_IR_OP_SYMBOL &&
                strcmp(inst->src1.as.sym_name, "cn_rt_array_set_element") == 0 &&
                inst->extra_args_count >= 3) {
                // 特殊处理 cn_rt_array_set_element：第3个参数需要是指针
                // 使用MSVC兼容的方式：先声明临时变量，再取地址
                
                CnIrOperand elem = inst->extra_args[2];
                
                // 根据元素类型生成临时变量并赋值
                if (elem.kind == CN_IR_OP_IMM_INT) {
                    // 整数常量：使用long long临时变量
                    static int temp_int_counter = 0;
                    fprintf(ctx->output_file, "  { long long _tmp_i%d = ", temp_int_counter);
                    print_operand(ctx, elem);
                    fprintf(ctx->output_file, "; %s(", get_c_function_name(inst->src1.as.sym_name));
                    print_operand(ctx, inst->extra_args[0]);
                    fprintf(ctx->output_file, ", ");
                    print_operand(ctx, inst->extra_args[1]);
                    fprintf(ctx->output_file, ", &_tmp_i%d", temp_int_counter);
                    if (inst->extra_args_count >= 4) {
                        fprintf(ctx->output_file, ", ");
                        print_operand(ctx, inst->extra_args[3]);
                    }
                    fprintf(ctx->output_file, "); }\n");
                    temp_int_counter++;
                } else if (elem.kind == CN_IR_OP_IMM_FLOAT) {
                    // 浮点常量：使用double临时变量
                    static int temp_float_counter = 0;
                    fprintf(ctx->output_file, "  { double _tmp_f%d = ", temp_float_counter);
                    print_operand(ctx, elem);
                    fprintf(ctx->output_file, "; %s(", get_c_function_name(inst->src1.as.sym_name));
                    print_operand(ctx, inst->extra_args[0]);
                    fprintf(ctx->output_file, ", ");
                    print_operand(ctx, inst->extra_args[1]);
                    fprintf(ctx->output_file, ", &_tmp_f%d", temp_float_counter);
                    if (inst->extra_args_count >= 4) {
                        fprintf(ctx->output_file, ", ");
                        print_operand(ctx, inst->extra_args[3]);
                    }
                    fprintf(ctx->output_file, "); }\n");
                    temp_float_counter++;
                } else if (elem.kind == CN_IR_OP_IMM_STR) {
                    // 字符串常量：使用char*临时变量
                    static int temp_str_counter = 0;
                    fprintf(ctx->output_file, "  { char* _tmp_s%d = ", temp_str_counter);
                    print_operand(ctx, elem);
                    fprintf(ctx->output_file, "; %s(", get_c_function_name(inst->src1.as.sym_name));
                    print_operand(ctx, inst->extra_args[0]);
                    fprintf(ctx->output_file, ", ");
                    print_operand(ctx, inst->extra_args[1]);
                    fprintf(ctx->output_file, ", &_tmp_s%d", temp_str_counter);
                    if (inst->extra_args_count >= 4) {
                        fprintf(ctx->output_file, ", ");
                        print_operand(ctx, inst->extra_args[3]);
                    }
                    fprintf(ctx->output_file, "); }\n");
                    temp_str_counter++;
                } else if (elem.kind == CN_IR_OP_REG) {
                    // 寄存器变量：根据类型生成临时变量
                    static int temp_reg_counter = 0;
                    const char *tmp_type = "long long";
                    if (elem.type) {
                        switch (elem.type->kind) {
                            case CN_TYPE_FLOAT: tmp_type = "double"; break;
                            case CN_TYPE_STRING: tmp_type = "char*"; break;
                            default: tmp_type = "long long"; break;
                        }
                    }
                    fprintf(ctx->output_file, "  { %s _tmp_r%d = ", tmp_type, temp_reg_counter);
                    print_operand(ctx, elem);
                    fprintf(ctx->output_file, "; %s(", get_c_function_name(inst->src1.as.sym_name));
                    print_operand(ctx, inst->extra_args[0]);
                    fprintf(ctx->output_file, ", ");
                    print_operand(ctx, inst->extra_args[1]);
                    fprintf(ctx->output_file, ", &_tmp_r%d", temp_reg_counter);
                    if (inst->extra_args_count >= 4) {
                        fprintf(ctx->output_file, ", ");
                        print_operand(ctx, inst->extra_args[3]);
                    }
                    fprintf(ctx->output_file, "); }\n");
                    temp_reg_counter++;
                } else {
                    // 已经是符号类型，直接取地址
                    fprintf(ctx->output_file, "  %s(", get_c_function_name(inst->src1.as.sym_name));
                    print_operand(ctx, inst->extra_args[0]);
                    fprintf(ctx->output_file, ", ");
                    print_operand(ctx, inst->extra_args[1]);
                    fprintf(ctx->output_file, ", &");
                    print_operand(ctx, elem);
                    if (inst->extra_args_count >= 4) {
                        fprintf(ctx->output_file, ", ");
                        print_operand(ctx, inst->extra_args[3]);
                    }
                    fprintf(ctx->output_file, ");\n");
                }
            } else {
                // 普通函数调用
                if (inst->dest.kind != CN_IR_OP_NONE) { print_operand(ctx, inst->dest); fprintf(ctx->output_file, " = "); }
                
                if (inst->src1.kind == CN_IR_OP_SYMBOL) {
                    fprintf(ctx->output_file, "%s(", get_c_function_name(inst->src1.as.sym_name));
                } else {
                    print_operand(ctx, inst->src1);
                    fprintf(ctx->output_file, "(");
                }
                for (size_t i = 0; i < inst->extra_args_count; i++) { 
                    print_operand(ctx, inst->extra_args[i]); 
                    if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); 
                }
                fprintf(ctx->output_file, ");\n");
            }
            break;
        case CN_IR_INST_MOV: fprintf(ctx->output_file, "  "); print_operand(ctx, inst->dest); fprintf(ctx->output_file, " = "); print_operand(ctx, inst->src1); fprintf(ctx->output_file, ";\n"); break;
        case CN_IR_INST_SELECT: 
            // 三元运算符：dest = condition ? true_val : false_val
            // 指令格式：dest, src1=condition, src2=true_val, extra_args[0]=false_val
            fprintf(ctx->output_file, "  ");
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, " = (");
            print_operand(ctx, inst->src1);  // condition
            fprintf(ctx->output_file, " ? ");
            print_operand(ctx, inst->src2);  // true_val
            fprintf(ctx->output_file, " : ");
            if (inst->extra_args_count > 0) {
                print_operand(ctx, inst->extra_args[0]);  // false_val
            } else {
                fprintf(ctx->output_file, "0");  // fallback
            }
            fprintf(ctx->output_file, ");\n");
            break;
        case CN_IR_INST_ADDRESS_OF: fprintf(ctx->output_file, "  "); print_operand(ctx, inst->dest); fprintf(ctx->output_file, " = &"); print_operand(ctx, inst->src1); fprintf(ctx->output_file, ";\n"); break;
        case CN_IR_INST_DEREF: fprintf(ctx->output_file, "  "); print_operand(ctx, inst->dest); fprintf(ctx->output_file, " = *"); print_operand(ctx, inst->src1); fprintf(ctx->output_file, ";\n"); break;
        case CN_IR_INST_MEMBER_ACCESS: 
            // 结构体成员访问：dest = obj.member
            fprintf(ctx->output_file, "  "); 
            print_operand(ctx, inst->dest); 
            fprintf(ctx->output_file, " = "); 
            print_operand(ctx, inst->src1);
            fprintf(ctx->output_file, ".");
            // src2 为成员名（符号类型）
            if (inst->src2.kind == CN_IR_OP_SYMBOL) {
                fprintf(ctx->output_file, "%s", inst->src2.as.sym_name);
            }
            fprintf(ctx->output_file, ";\n");
            break;
        case CN_IR_INST_STRUCT_INIT:
            // 结构体初始化（构造函数调用）：dest = (struct 类型名){arg1, arg2, ...}
            fprintf(ctx->output_file, "  ");
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, " = (struct ");
            // src1 为类型名（符号类型）
            if (inst->src1.kind == CN_IR_OP_SYMBOL) {
                fprintf(ctx->output_file, "%s", inst->src1.as.sym_name);
            }
            fprintf(ctx->output_file, "){");
            // 打印参数
            for (size_t i = 0; i < inst->extra_args_count; i++) {
                if (i > 0) fprintf(ctx->output_file, ", ");
                print_operand(ctx, inst->extra_args[i]);
            }
            fprintf(ctx->output_file, "};\n");
            break;
        case CN_IR_INST_NEG:
            // 一元负号：dest = -src1
            fprintf(ctx->output_file, "  ");
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, " = -");
            print_operand(ctx, inst->src1);
            fprintf(ctx->output_file, ";\n");
            break;
        case CN_IR_INST_NOT:
            // 逻辑非：dest = !src1
            fprintf(ctx->output_file, "  ");
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, " = !");
            print_operand(ctx, inst->src1);
            fprintf(ctx->output_file, ";\n");
            break;
        default: fprintf(ctx->output_file, "  /* Unsupported inst %d */\n", inst->kind); break;
    }
}

void cn_cgen_block(CnCCodeGenContext *ctx, CnIrBasicBlock *block) {
    if (!ctx || !block) return;
    if (block->name) fprintf(ctx->output_file, "\n  %s:\n", block->name);
    CnIrInst *inst = block->first_inst;
    while (inst) { cn_cgen_inst(ctx, inst); inst = inst->next; }
}

void cn_cgen_function(CnCCodeGenContext *ctx, CnIrFunction *func) {
    if (!ctx || !func) return;
    ctx->current_func = func;
    
    // 生成函数名：直接使用原始函数名（不再使用模块前缀编码）
    // 模块系统通过作用域管理避免命名冲突
    const char *c_func_name = get_c_function_name(func->name);
    
    bool is_main = strcmp(c_func_name, "main") == 0;
    
    // 注意：运行时宏冲突的 #undef 已在前向声明之前统一处理，此处不再重复
    
    // 中断处理函数：生成注释和属性
    if (func->is_interrupt_handler) {
        fprintf(ctx->output_file, "// 中断处理函数 (ISR) - 中断向量号: %u\n", func->interrupt_vector);
    }
    
    fprintf(ctx->output_file, "%s %s(", get_c_type_string(func->return_type), c_func_name);
    for (size_t i = 0; i < func->param_count; i++) {
        fprintf(ctx->output_file, "%s %s", get_c_type_string(func->params[i].type), get_c_variable_name(func->params[i].as.sym_name));
        if (i < func->param_count - 1) fprintf(ctx->output_file, ", ");
    }
    fprintf(ctx->output_file, ") {\n");
    
    // 生成静态局部变量声明（在函数体开头）
    if (func->first_static_var) {
        fprintf(ctx->output_file, "    /* 静态局部变量 */\n");
        CnIrStaticVar *static_var = func->first_static_var;
        while (static_var) {
            const char *type_str = get_c_type_string(static_var->type);
            // 生成静态变量名：cn_static_{函数名}_{变量名}
            fprintf(ctx->output_file, "    static %s cn_static_%s_%s",
                    type_str, func->name, static_var->name);
            
            // 如果有初始化值，生成初始化
            if (static_var->initializer.kind != CN_IR_OP_NONE) {
                fprintf(ctx->output_file, " = ");
                if (static_var->initializer.kind == CN_IR_OP_IMM_INT) {
                    fprintf(ctx->output_file, "%lld", static_var->initializer.as.imm_int);
                } else if (static_var->initializer.kind == CN_IR_OP_IMM_FLOAT) {
                    fprintf(ctx->output_file, "%f", static_var->initializer.as.imm_float);
                } else if (static_var->initializer.kind == CN_IR_OP_IMM_STR) {
                    // 字符串字面量：需要加引号并处理转义
                    fprintf(ctx->output_file, "\"");
                    for (const char *p = static_var->initializer.as.imm_str; p && *p; p++) {
                        switch (*p) {
                            case '\\': fprintf(ctx->output_file, "\\\\"); break;
                            case '\"': fprintf(ctx->output_file, "\\\""); break;
                            case '\n': fprintf(ctx->output_file, "\\n"); break;
                            case '\r': fprintf(ctx->output_file, "\\r"); break;
                            case '\t': fprintf(ctx->output_file, "\\t"); break;
                            default: fprintf(ctx->output_file, "%c", *p); break;
                        }
                    }
                    fprintf(ctx->output_file, "\"");
                }
            }
            fprintf(ctx->output_file, ";\n");
            static_var = static_var->next;
        }
        fprintf(ctx->output_file, "\n");
    }
    
    // 注入运行时初始化（仅 hosted 模式）
    if (is_main && ctx->module && ctx->module->compile_mode != CN_COMPILE_MODE_FREESTANDING) {
        fprintf(ctx->output_file, "  cn_rt_init();\n");
        
        // 自动注册ISR函数
        CnIrFunction *isr_func = ctx->module->first_func;
        while (isr_func) {
            if (isr_func->is_interrupt_handler) {
                const char *isr_c_name = get_c_function_name(isr_func->name);
                fprintf(ctx->output_file, "  cn_rt_interrupt_register(%u, %s, \"%s\");\n",
                        isr_func->interrupt_vector, isr_c_name, isr_c_name);
            }
            isr_func = isr_func->next;
        }
    }
    
    // 声明虚拟寄存器：根据IR指令中的类型信息收集寄存器类型
    if (func->next_reg_id > 0) {
        /* 首先扫描所有指令以找出实际使用的最大寄存器ID */
        int max_reg_id = -1;
        CnIrBasicBlock *prescan_block = func->first_block;
        while (prescan_block) {
            CnIrInst *prescan_inst = prescan_block->first_inst;
            while (prescan_inst) {
                if (prescan_inst->dest.kind == CN_IR_OP_REG && prescan_inst->dest.as.reg_id > max_reg_id) {
                    max_reg_id = prescan_inst->dest.as.reg_id;
                }
                if (prescan_inst->src1.kind == CN_IR_OP_REG && prescan_inst->src1.as.reg_id > max_reg_id) {
                    max_reg_id = prescan_inst->src1.as.reg_id;
                }
                if (prescan_inst->src2.kind == CN_IR_OP_REG && prescan_inst->src2.as.reg_id > max_reg_id) {
                    max_reg_id = prescan_inst->src2.as.reg_id;
                }
                for (size_t i = 0; i < prescan_inst->extra_args_count; i++) {
                    if (prescan_inst->extra_args[i].kind == CN_IR_OP_REG && prescan_inst->extra_args[i].as.reg_id > max_reg_id) {
                        max_reg_id = prescan_inst->extra_args[i].as.reg_id;
                    }
                }
                prescan_inst = prescan_inst->next;
            }
            prescan_block = prescan_block->next;
        }
        
        // 使用实际的最大寄存器ID+1或func->next_reg_id中的较大值
        int scan_reg_count = (max_reg_id >= 0) ? (max_reg_id + 1) : 0;
        int actual_reg_count = (scan_reg_count > func->next_reg_id) ? scan_reg_count : func->next_reg_id;
        
        /* 性能优化：使用栈分配替代 calloc，避免堆分配开销 */
        CnType *reg_types_stack[256];  /* 大多数函数的寄存器数量 < 256 */
        CnType **reg_types = NULL;
        bool use_heap = false;
        
        if (actual_reg_count <= 256) {
            /* 小寄存器集合使用栈分配 */
            memset(reg_types_stack, 0, actual_reg_count * sizeof(CnType*));
            reg_types = reg_types_stack;
        } else {
            /* 大寄存器集合使用堆分配 */
            reg_types = calloc(actual_reg_count, sizeof(CnType*));
            use_heap = true;
            if (!reg_types) {
                /* 内存分配失败，使用默认类型 */
                fprintf(ctx->output_file, "  long long r0");
                for (int i = 1; i < actual_reg_count; i++) {
                    fprintf(ctx->output_file, ", r%d", i);
                }
                fprintf(ctx->output_file, ";\n");
                goto skip_register_type_scan;
            }
        }
        
        /* 性能优化：单次遍历收集寄存器类型 */
        CnIrBasicBlock *scan_block = func->first_block;
        while (scan_block) {
            CnIrInst *scan_inst = scan_block->first_inst;
            while (scan_inst) {
                // 收集dest寄存器类型
                if (scan_inst->dest.kind == CN_IR_OP_REG) {
                    if (scan_inst->dest.as.reg_id < actual_reg_count &&
                        !reg_types[scan_inst->dest.as.reg_id] && 
                        scan_inst->dest.type) {
                        reg_types[scan_inst->dest.as.reg_id] = scan_inst->dest.type;
                    }
                }
                // 收集src1寄存器类型
                if (scan_inst->src1.kind == CN_IR_OP_REG) {
                    if (scan_inst->src1.as.reg_id < actual_reg_count &&
                        !reg_types[scan_inst->src1.as.reg_id] && 
                        scan_inst->src1.type) {
                        reg_types[scan_inst->src1.as.reg_id] = scan_inst->src1.type;
                    }
                }
                // 收集src2寄存器类型
                if (scan_inst->src2.kind == CN_IR_OP_REG) {
                    if (scan_inst->src2.as.reg_id < actual_reg_count &&
                        !reg_types[scan_inst->src2.as.reg_id] && 
                        scan_inst->src2.type) {
                        reg_types[scan_inst->src2.as.reg_id] = scan_inst->src2.type;
                    }
                }
                // 收集extra_args中的寄存器类型
                for (size_t i = 0; i < scan_inst->extra_args_count; i++) {
                    if (scan_inst->extra_args[i].kind == CN_IR_OP_REG) {
                        if (scan_inst->extra_args[i].as.reg_id < actual_reg_count &&
                            !reg_types[scan_inst->extra_args[i].as.reg_id] &&
                            scan_inst->extra_args[i].type) {
                            reg_types[scan_inst->extra_args[i].as.reg_id] = scan_inst->extra_args[i].type;
                        }
                    }
                }
                scan_inst = scan_inst->next;
            }
            scan_block = scan_block->next;
        }
        
        /* 性能优化：按类型分组声明寄存器，减少 fprintf 调用 */
        bool has_int_regs = false;
        for (int i = 0; i < actual_reg_count; i++) {
            // 对于NULL、CN_TYPE_INT或CN_TYPE_UNKNOWN类型，都使用long long
            if (!reg_types[i] || reg_types[i]->kind == CN_TYPE_INT || reg_types[i]->kind == CN_TYPE_UNKNOWN) {
                if (!has_int_regs) {
                    fprintf(ctx->output_file, "  long long ");
                    has_int_regs = true;
                } else {
                    fprintf(ctx->output_file, ", ");
                }
                fprintf(ctx->output_file, "r%d", i);
            }
        }
        if (has_int_regs) fprintf(ctx->output_file, ";\n");
        
        /* 声明字符串寄存器（每个寄存器单独声明以确保每个都是指针类型） */
        for (int i = 0; i < actual_reg_count; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_STRING) {
                fprintf(ctx->output_file, "  char* r%d;\n", i);
            }
        }
        
        /* 声明数组寄存器（void* 用于数组指针） */
        for (int i = 0; i < actual_reg_count; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_ARRAY) {
                fprintf(ctx->output_file, "  void* r%d;\n", i);
            }
        }
        
        /* 声明指针寄存器 */
        for (int i = 0; i < actual_reg_count; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_POINTER) {
                fprintf(ctx->output_file, "  %s r%d;\n", get_c_type_string(reg_types[i]), i);
            }
        }
        
        /* 声明布尔寄存器 */
        for (int i = 0; i < actual_reg_count; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_BOOL) {
                fprintf(ctx->output_file, "  _Bool r%d;\n", i);
            }
        }
        
        /* 声明浮点寄存器 */
        for (int i = 0; i < actual_reg_count; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_FLOAT) {
                fprintf(ctx->output_file, "  double r%d;\n", i);
            }
        }
        
        /* 声明结构体寄存器 */
        for (int i = 0; i < actual_reg_count; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_STRUCT) {
                fprintf(ctx->output_file, "  %s r%d;\n", get_c_type_string(reg_types[i]), i);
            }
        }
        
        /* 声明枚举寄存器 */
        for (int i = 0; i < actual_reg_count; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_ENUM) {
                fprintf(ctx->output_file, "  %s r%d;\n", get_c_type_string(reg_types[i]), i);
            }
        }
        
        if (use_heap) {
            free(reg_types);
        }
        
    skip_register_type_scan:;
    }

    CnIrBasicBlock *block = func->first_block;
    while (block) { cn_cgen_block(ctx, block); block = block->next; }

    // 检查函数是否以返回语句结束
    bool has_return = false;
    if (func->last_block && func->last_block->last_inst) {
        has_return = (func->last_block->last_inst->kind == CN_IR_INST_RET);
    }

    // 如果函数没有返回语句，添加默认返回
    if (!has_return) {
        // 中断处理函数默认返回0
        if (func->is_interrupt_handler) {
            fprintf(ctx->output_file, "  return 0;\n");
        }
        // 根据返回类型添加默认返回
        else if (func->return_type) {
            switch (func->return_type->kind) {
                case CN_TYPE_VOID:
                    // void函数不需要返回值
                    fprintf(ctx->output_file, "  return;\n");
                    break;
                case CN_TYPE_INT:
                case CN_TYPE_INT32:
                case CN_TYPE_INT64:
                case CN_TYPE_UINT32:
                case CN_TYPE_UINT64_LL:
                case CN_TYPE_BOOL:
                    fprintf(ctx->output_file, "  return 0;\n");
                    break;
                case CN_TYPE_FLOAT:
                case CN_TYPE_FLOAT32:
                case CN_TYPE_FLOAT64:
                    fprintf(ctx->output_file, "  return 0.0;\n");
                    break;
                case CN_TYPE_STRING:
                case CN_TYPE_POINTER:
                    fprintf(ctx->output_file, "  return NULL;\n");
                    break;
                default:
                    // 其他类型默认返回0
                    fprintf(ctx->output_file, "  return 0;\n");
                    break;
            }
        } else {
            // 无返回类型声明的函数，默认返回0
            fprintf(ctx->output_file, "  return 0;\n");
        }
    }

    // 注入运行时退出（仅 hosted 模式）
    if (is_main && ctx->module && ctx->module->compile_mode != CN_COMPILE_MODE_FREESTANDING) {
        fprintf(ctx->output_file, "  cn_rt_exit();\n");
    }
    
    fprintf(ctx->output_file, "}\n\n");
}

// 用于存储局部结构体与其所属函数的映射关系
typedef struct {
    CnAstStmt *struct_stmt;      // 结构体定义语句
    const char *function_name;   // 所属函数名
    size_t function_name_length;
    const char *struct_name;     // 结构体名
    size_t struct_name_length;
} LocalStructInfo;

// 全局局部结构体信息表（用于 C 代码生成时查找）
static LocalStructInfo *g_local_struct_infos = NULL;
static size_t g_local_struct_count = 0;

// 查找局部结构体信息
static LocalStructInfo *find_local_struct_info(const char *struct_name, size_t struct_name_len) {
    for (size_t i = 0; i < g_local_struct_count; i++) {
        if (g_local_struct_infos[i].struct_name_length == struct_name_len &&
            strncmp(g_local_struct_infos[i].struct_name, struct_name, struct_name_len) == 0) {
            return &g_local_struct_infos[i];
        }
    }
    return NULL;
}

// 递归收集函数体内的结构体定义，记录所属函数
static void collect_local_structs_from_block(CnAstBlockStmt *block, LocalStructInfo **struct_infos, size_t *count, size_t *capacity, const char *func_name, size_t func_name_len) {
    if (!block || !struct_infos || !count || !capacity) return;
    
    for (size_t i = 0; i < block->stmt_count; i++) {
        CnAstStmt *stmt = block->stmts[i];
        if (!stmt) continue;
        
        // 如果是结构体定义，添加到列表并记录所属函数
        if (stmt->kind == CN_AST_STMT_STRUCT_DECL) {
            if (*count >= *capacity) {
                *capacity = (*capacity == 0) ? 4 : (*capacity * 2);
                LocalStructInfo *new_array = (LocalStructInfo *)realloc(*struct_infos, *capacity * sizeof(LocalStructInfo));
                if (!new_array) return;
                *struct_infos = new_array;
            }
            (*struct_infos)[*count].struct_stmt = stmt;
            (*struct_infos)[*count].function_name = func_name;
            (*struct_infos)[*count].function_name_length = func_name_len;
            // 保存结构体名称
            (*struct_infos)[*count].struct_name = stmt->as.struct_decl.name;
            (*struct_infos)[*count].struct_name_length = stmt->as.struct_decl.name_length;
            (*count)++;
        }
        
        // 递归处理嵌套块
        if (stmt->kind == CN_AST_STMT_IF) {
            collect_local_structs_from_block(stmt->as.if_stmt.then_block, struct_infos, count, capacity, func_name, func_name_len);
            collect_local_structs_from_block(stmt->as.if_stmt.else_block, struct_infos, count, capacity, func_name, func_name_len);
        } else if (stmt->kind == CN_AST_STMT_WHILE) {
            collect_local_structs_from_block(stmt->as.while_stmt.body, struct_infos, count, capacity, func_name, func_name_len);
        } else if (stmt->kind == CN_AST_STMT_FOR) {
            collect_local_structs_from_block(stmt->as.for_stmt.body, struct_infos, count, capacity, func_name, func_name_len);
        } else if (stmt->kind == CN_AST_STMT_SWITCH) {
            for (size_t j = 0; j < stmt->as.switch_stmt.case_count; j++) {
                collect_local_structs_from_block(stmt->as.switch_stmt.cases[j].body, struct_infos, count, capacity, func_name, func_name_len);
            }
        }
    }
}

// 从函数声明中收集局部结构体定义
static void collect_local_structs_from_function(CnAstFunctionDecl *func, LocalStructInfo **struct_infos, size_t *count, size_t *capacity) {
    if (!func || !func->body) return;
    collect_local_structs_from_block(func->body, struct_infos, count, capacity, func->name, func->name_length);
}

// 生成结构体定义（从 AST 结构体声明）
// 如果提供了函数名前缀，则生成局部结构体的唯一名称
void cn_cgen_struct_decl_with_prefix(CnCCodeGenContext *ctx, CnAstStmt *struct_stmt, const char *func_prefix, size_t func_prefix_len) {
    if (!ctx || !struct_stmt || struct_stmt->kind != CN_AST_STMT_STRUCT_DECL) return;
    
    CnAstStructDecl *decl = &struct_stmt->as.struct_decl;
    
    // 生成结构体定义
    // 如果有函数前缀，生成: struct __local_函数名_结构体名
    // 否则生成: struct 结构体名
    if (func_prefix && func_prefix_len > 0) {
        fprintf(ctx->output_file, "struct __local_%.*s_%.*s {\n", 
                (int)func_prefix_len, func_prefix,
                (int)decl->name_length, decl->name);
    } else {
        fprintf(ctx->output_file, "struct %.*s {\n", 
                (int)decl->name_length, decl->name);
    }
    
    // 生成字段
    for (size_t i = 0; i < decl->field_count; i++) {
        fprintf(ctx->output_file, "    %s %.*s;\n",
                get_c_type_string(decl->fields[i].field_type),
                (int)decl->fields[i].name_length,
                decl->fields[i].name);
    }
    
    fprintf(ctx->output_file, "};\n\n");
}

// 兼容旧接口：生成全局结构体定义
void cn_cgen_struct_decl(CnCCodeGenContext *ctx, CnAstStmt *struct_stmt) {
    cn_cgen_struct_decl_with_prefix(ctx, struct_stmt, NULL, 0);
}

// 生成枚举定义（从 AST 枚举声明）
void cn_cgen_enum_decl(CnCCodeGenContext *ctx, CnAstStmt *enum_stmt) {
    if (!ctx || !enum_stmt || enum_stmt->kind != CN_AST_STMT_ENUM_DECL) return;
    
    CnAstEnumDecl *decl = &enum_stmt->as.enum_decl;
    
    // 生成枚举定义
    fprintf(ctx->output_file, "enum %.*s {\n", 
            (int)decl->name_length, decl->name);
    
    // 生成枚举成员
    for (size_t i = 0; i < decl->member_count; i++) {
        fprintf(ctx->output_file, "    %.*s",
                (int)decl->members[i].name_length,
                decl->members[i].name);
        
        // 如果有显式值，生成赋值
        if (decl->members[i].has_value) {
            fprintf(ctx->output_file, " = %ld", decl->members[i].value);
        }
        
        // 添加逗号（最后一个除外）
        if (i < decl->member_count - 1) {
            fprintf(ctx->output_file, ",\n");
        } else {
            fprintf(ctx->output_file, "\n");
        }
    }
    
    fprintf(ctx->output_file, "};\n\n");
}

int cn_cgen_module_to_file(CnIrModule *module, const char *filename) {
    return cn_cgen_module_with_structs_to_file(module, NULL, filename);
}

// 完整的代码生成函数，包含结构体定义
int cn_cgen_module_with_structs_to_file(CnIrModule *module, CnAstProgram *program, const char *filename) {
    if (!module || !filename) return -1;

    /* 根据 IR 模块上的目标三元组获取预设数据布局（若存在）。 */
    CnTargetDataLayout layout;
    bool layout_ok = cn_support_target_get_data_layout(&module->target, &layout);
    if (layout_ok) {
        g_target_layout = layout;
        g_target_layout_valid = true;
    } else {
        g_target_layout_valid = false;
    }

    FILE *file = fopen(filename, "w");
    if (!file) return -1;
    
    /* 性能优化：设置较大的缓冲区，减少系统调用 */
    char *buffer = malloc(CGEN_BUFFER_SIZE);
    if (buffer) {
        setvbuf(file, buffer, _IOFBF, CGEN_BUFFER_SIZE);
    }
    
    CnCCodeGenContext ctx = {0};
    ctx.output_file = file;
    ctx.module = module;
    ctx.program = program;  // 设置程序AST，用于查找类定义

    // 根据编译模式生成不同的头文件
    if (module->compile_mode == CN_COMPILE_MODE_FREESTANDING) {
        // Freestanding 模式：不包含运行时库头文件，但声明必要的运行时函数
        fprintf(file, "#include <stddef.h>\n#include <stdbool.h>\n#include <stdint.h>\n\n");
        fprintf(file, "// CN Language Runtime Function Declarations (Freestanding Mode)\n");
        fprintf(file, "void cn_rt_print_string(const char *str);\n");
        fprintf(file, "void cn_rt_print_int(long long val);\n");
        fprintf(file, "void cn_rt_print_float(double val);\n");
        fprintf(file, "void cn_rt_print_bool(int val);\n");
        fprintf(file, "size_t cn_rt_string_length(const char *str);\n");
        fprintf(file, "char* cn_rt_string_concat(const char *a, const char *b);\n");
        fprintf(file, "char* cn_rt_int_to_string(long long val);\n");
        fprintf(file, "char* cn_rt_bool_to_string(int val);\n");
        fprintf(file, "char* cn_rt_float_to_string(double val);\n");
        fprintf(file, "void* cn_rt_array_alloc(size_t elem_size, size_t count);\n");
        fprintf(file, "size_t cn_rt_array_length(void *arr);\n");
        fprintf(file, "int cn_rt_array_set_element(void *arr, size_t index, const void *element, size_t elem_size);\n");
        fprintf(file, "\n");
    } else {
        // Hosted 模式：包含完整运行时库
        fprintf(file, "#include <stdio.h>\n#include <stdbool.h>\n#include <stdint.h>\n#include \"cnrt.h\"\n");
        // 包含系统API头文件(替代已删除关键字的运行时函数)
        fprintf(file, "#include \"cnlang/runtime/system_api.h\"\n\n");
    }
    
    // 生成结构体定义（如果提供了 AST）
    // 首先收集全局结构体和局部结构体
    LocalStructInfo *local_struct_infos = NULL;
    size_t local_struct_count = 0;
    size_t local_struct_capacity = 0;
    
    if (program) {
        // 输出全局结构体定义
        if (program->struct_count > 0) {
            fprintf(file, "// CN Language Global Struct Definitions\n");
            for (size_t i = 0; i < program->struct_count; i++) {
                cn_cgen_struct_decl(&ctx, program->structs[i]);
            }
        }
        
        // 收集所有函数中的局部结构体（带函数名信息）
        for (size_t i = 0; i < program->function_count; i++) {
            collect_local_structs_from_function(program->functions[i], &local_struct_infos, &local_struct_count, &local_struct_capacity);
        }
        
        // 输出局部结构体定义（提升到全局，但使用特殊命名）
        if (local_struct_count > 0) {
            fprintf(file, "// CN Language Local Struct Definitions (hoisted for C compatibility)\n");
            fprintf(file, "// Note: These are local to their respective functions in CN semantics\n");
            for (size_t i = 0; i < local_struct_count; i++) {
                cn_cgen_struct_decl_with_prefix(&ctx, 
                                               local_struct_infos[i].struct_stmt,
                                               local_struct_infos[i].function_name,
                                               local_struct_infos[i].function_name_length);
            }
            
            // 设置全局查找表
            g_local_struct_infos = local_struct_infos;
            g_local_struct_count = local_struct_count;
        }
    }
        
    // 生成枚举定义（如果提供了 AST）
    if (program && program->enum_count > 0) {
        fprintf(file, "// CN Language Enum Definitions\n");
        for (size_t i = 0; i < program->enum_count; i++) {
            cn_cgen_enum_decl(&ctx, program->enums[i]);
        }
    }
    
    // 生成接口定义（如果提供了 AST）- 必须在类定义之前，因为类的vtable引用接口vtable
    if (program && program->interface_count > 0) {
        fprintf(file, "// CN Language Interface Definitions\n\n");
        for (size_t i = 0; i < program->interface_count; i++) {
            CnAstStmt *interface_stmt = program->interfaces[i];
            if (interface_stmt && interface_stmt->kind == CN_AST_STMT_INTERFACE_DECL) {
                cn_cgen_interface_decl(&ctx, interface_stmt->as.interface_decl);
            }
        }
    }
    
    // 生成类定义（如果提供了 AST）- 阶段11 面向对象编程支持
    if (program && program->class_count > 0) {
        fprintf(file, "// CN Language Class Definitions\n\n");
        for (size_t i = 0; i < program->class_count; i++) {
            CnAstStmt *class_stmt = program->classes[i];
            if (class_stmt && class_stmt->kind == CN_AST_STMT_CLASS_DECL) {
                cn_cgen_class_decl(&ctx, class_stmt->as.class_decl);
            }
        }
    }
    
    // 生成全局变量声明
    fprintf(file, "// Global Variables\n");
    CnIrGlobalVar *global = module->first_global;
    while (global) {
        fprintf(file, "%s cn_var_%s",
                get_c_type_string(global->type),
                global->name);
        
        // 生成初始化值
        if (global->initializer.kind != CN_IR_OP_NONE) {
            fprintf(file, " = ");
            if (global->initializer.kind == CN_IR_OP_IMM_INT) {
                fprintf(file, "%lld", global->initializer.as.imm_int);
            } else if (global->initializer.kind == CN_IR_OP_IMM_FLOAT) {
                fprintf(file, "%f", global->initializer.as.imm_float);
            }
        }
        
        fprintf(file, ";\n");
        global = global->next;
    }
    fprintf(file, "\n");
    
    // 生成函数前置声明（Forward Declarations）
    fprintf(file, "// Forward Declarations\n");
    CnIrFunction *func = module->first_func;
    
    // 首先收集所有需要 #undef 的运行时宏冲突函数名
    // 在前向声明之前统一生成 #undef，避免宏展开导致重定义
    bool has_macro_conflict = false;
    CnIrFunction *temp_func = func;
    while (temp_func) {
        if (is_runtime_macro_conflict(temp_func->name)) {
            if (!has_macro_conflict) {
                fprintf(file, "// 取消运行时宏定义，避免与用户函数名冲突\n");
                has_macro_conflict = true;
            }
            fprintf(file, "#undef %s\n", temp_func->name);
        }
        temp_func = temp_func->next;
    }
    if (has_macro_conflict) {
        fprintf(file, "\n");
    }
    
    // 生成前向声明
    while (func) {
        fprintf(file, "%s %s(", get_c_type_string(func->return_type), get_c_function_name(func->name));
        for (size_t i = 0; i < func->param_count; i++) {
            fprintf(file, "%s", get_c_type_string(func->params[i].type));
            if (i < func->param_count - 1) fprintf(file, ", ");
        }
        fprintf(file, ");\n");
        func = func->next;
    }
    fprintf(file, "\n");

    func = module->first_func;
    while (func) {
        cn_cgen_function(&ctx, func);
        func = func->next;
    }
    
    // 清理全局查找表
    g_local_struct_infos = NULL;
    g_local_struct_count = 0;
    
    // 释放局部结构体信息列表
    if (local_struct_infos) {
        free(local_struct_infos);
    }
    
    fclose(file);
    
    /* 释放缓冲区 */
    if (buffer) {
        free(buffer);
    }
    
    return 0;
}

int cn_cgen_header_to_file(CnIrModule *module, const char *filename) {
    if (!module || !filename) return -1;
    FILE *file = fopen(filename, "w");
    if (!file) return -1;
    fprintf(file, "#ifndef CN_MODULE_AUTOGEN_H\n#define CN_MODULE_AUTOGEN_H\n\n#include \"cnrt.h\"\n\n");
    CnIrFunction *func = module->first_func;
    while (func) {
        fprintf(file, "%s %s(", get_c_type_string(func->return_type), get_c_function_name(func->name));
        for (size_t i = 0; i < func->param_count; i++) {
            fprintf(file, "%s %s", get_c_type_string(func->params[i].type), get_c_variable_name(func->params[i].as.sym_name));
            if (i < func->param_count - 1) fprintf(file, ", ");
        }
        fprintf(file, ");\n");
        func = func->next;
    }
    fprintf(file, "\n#endif\n");
    fclose(file);
    return 0;
}

char *cn_cgen_module_to_string(CnIrModule *module) { return strdup("// Not implemented"); }

// ============================================================================
// 导入模块前向声明生成
// ============================================================================

// 前向声明
static void cn_cgen_function_forward_declaration(FILE *file, const char *func_name, size_t func_name_len, CnType *func_type);

// 通配符导入回调上下文结构
typedef struct {
    FILE *file;
    const char *module_name;      // 模块名（用于生成前缀）
    size_t module_name_len;       // 模块名长度
} WildcardImportContext;

// 通配符导入回调函数（遍历模块作用域符号时调用）
static void wildcard_import_callback(CnSemSymbol *sym, void *user_data) {
    WildcardImportContext *ctx = (WildcardImportContext *)user_data;
    // 只导出公开的符号
    if (!sym->is_public) {
        return;
    }
    
    if (sym->kind == CN_SEM_SYMBOL_FUNCTION && sym->type) {
        // 直接使用原始函数名生成前向声明（不再使用编码名称）
        // 模块系统通过作用域管理避免命名冲突
        cn_cgen_function_forward_declaration(ctx->file, sym->name, sym->name_length, sym->type);
    } else if (sym->kind == CN_SEM_SYMBOL_VARIABLE && sym->type) {
        // 生成变量的 extern 声明
        const char *var_type = get_c_type_string(sym->type);
        fprintf(ctx->file, "extern %s cn_var_%.*s;\n", var_type, (int)sym->name_length, sym->name);
    }
}

// 全量导入回调上下文结构
typedef struct {
    FILE *file;
    const char *module_name;
    size_t module_name_len;
} FullImportContext;

// 全量导入回调函数（遍历模块作用域符号时调用）
static void full_import_callback(CnSemSymbol *sym, void *user_data) {
    FullImportContext *ctx = (FullImportContext *)user_data;
    // 只导出公开的符号
    if (!sym->is_public) {
        return;
    }
    
    if (sym->kind == CN_SEM_SYMBOL_FUNCTION && sym->type) {
        // 直接使用原始函数名生成前向声明（不再使用编码名称）
        // 模块系统通过作用域管理避免命名冲突
        cn_cgen_function_forward_declaration(ctx->file, sym->name, sym->name_length, sym->type);
    } else if (sym->kind == CN_SEM_SYMBOL_VARIABLE && sym->type) {
        // 生成变量的 extern 声明
        const char *var_type = get_c_type_string(sym->type);
        fprintf(ctx->file, "extern %s cn_var_%.*s;\n", var_type, (int)sym->name_length, sym->name);
    }
}

/**
 * @brief 生成单个函数的前向声明
 *
 * @param file 输出文件
 * @param func_name 函数名称（非空字符结尾）
 * @param func_name_len 函数名称长度
 * @param func_type 函数类型
 */
static void cn_cgen_function_forward_declaration(FILE *file, const char *func_name, size_t func_name_len, CnType *func_type) {
    if (!file || !func_name || !func_type || func_type->kind != CN_TYPE_FUNCTION) {
        return;
    }
    
    // 检测运行时宏冲突：如果函数名与运行时宏同名，需要先 #undef 取消宏定义
    // 创建临时空字符结尾的字符串用于检测
    char temp_name[256];
    size_t safe_len = func_name_len < sizeof(temp_name) - 1 ? func_name_len : sizeof(temp_name) - 1;
    memcpy(temp_name, func_name, safe_len);
    temp_name[safe_len] = '\0';
    
    if (is_runtime_macro_conflict(temp_name)) {
        fprintf(file, "#undef %s\n", temp_name);
    }
    
    // 返回类型
    const char *ret_type = get_c_type_string(func_type->as.function.return_type);
    fprintf(file, "%s %.*s(", ret_type, (int)func_name_len, func_name);
    
    // 参数列表
    if (func_type->as.function.param_count == 0) {
        fprintf(file, "void");
    } else {
        for (size_t i = 0; i < func_type->as.function.param_count; i++) {
            if (i > 0) fprintf(file, ", ");
            const char *param_type = get_c_type_string(func_type->as.function.param_types[i]);
            fprintf(file, "%s", param_type);
        }
    }
    
    fprintf(file, ");\n");
}

/**
 * @brief 从全局作用域获取导入符号的类型信息并生成前向声明
 *
 * @param file 输出文件
 * @param program AST程序节点
 * @param global_scope 全局作用域
 */
static void cn_cgen_import_forward_declarations(FILE *file, CnAstProgram *program, CnSemScope *global_scope) {
    if (!file || !program || !global_scope) {
        return;
    }
    
    // 如果没有导入语句，直接返回
    if (program->import_count == 0) {
        return;
    }
    
    fprintf(file, "// Forward Declarations - 从导入模块\n");
    
    // 遍历所有导入语句
    for (size_t i = 0; i < program->import_count; i++) {
        CnAstStmt *import_stmt = program->imports[i];
        if (!import_stmt || import_stmt->kind != CN_AST_STMT_IMPORT) {
            continue;
        }
        
        CnAstImportStmt *import = &import_stmt->as.import_stmt;
        
        // 处理选择性导入（从 模块 导入 { 成员1, 成员2 }）
        if (import->kind == CN_IMPORT_FROM_SELECTIVE && import->members && import->member_count > 0) {
            // 遍历导入的成员
            for (size_t j = 0; j < import->member_count; j++) {
                CnAstImportMember *member = &import->members[j];
                const char *symbol_name = member->name;
                size_t symbol_name_len = member->name_length;
                
                // 从全局作用域查找符号获取类型信息
                CnSemSymbol *symbol = cn_sem_scope_lookup(global_scope, symbol_name, symbol_name_len);
                if (symbol && symbol->type) {
                    if (symbol->kind == CN_SEM_SYMBOL_FUNCTION) {
                        // 生成函数前向声明
                        cn_cgen_function_forward_declaration(file, symbol_name, symbol_name_len, symbol->type);
                    } else if (symbol->kind == CN_SEM_SYMBOL_VARIABLE) {
                        // 生成变量的 extern 声明
                        const char *var_type = get_c_type_string(symbol->type);
                        fprintf(file, "extern %s cn_var_%.*s;\n", var_type, (int)symbol_name_len, symbol_name);
                    }
                }
            }
        }
        // 处理通配符导入（从 模块 导入 *）
        else if (import->kind == CN_IMPORT_FROM_WILDCARD) {
            // 通配符导入：需要为模块中的公开函数生成带模块前缀的前向声明
            const char *module_name = import->module_name;
            size_t module_name_len = import->module_name_length;
            
            // 从全局作用域查找模块符号
            CnSemSymbol *module_sym = cn_sem_scope_lookup(global_scope, module_name, module_name_len);
            if (module_sym && module_sym->kind == CN_SEM_SYMBOL_MODULE && module_sym->as.module_scope) {
                // 使用回调遍历模块作用域中的所有符号
                WildcardImportContext ctx = { file, module_name, module_name_len };
                cn_sem_scope_foreach_symbol(module_sym->as.module_scope, wildcard_import_callback, &ctx);
            }
        }
        // 处理全量导入（导入 模块）
        else if (import->kind == CN_IMPORT_FULL) {
            // 全量导入：模块名作为前缀访问成员
            // 需要为模块中的公开函数生成前向声明，格式：cn_module_模块名__成员名
            const char *module_name = import->module_name;
            size_t module_name_len = import->module_name_length;
            
            // 从全局作用域查找模块符号
            CnSemSymbol *module_sym = cn_sem_scope_lookup(global_scope, module_name, module_name_len);
            if (module_sym && module_sym->kind == CN_SEM_SYMBOL_MODULE && module_sym->as.module_scope) {
                // 使用回调遍历模块作用域中的所有符号
                FullImportContext fctx = { file, module_name, module_name_len };
                cn_sem_scope_foreach_symbol(module_sym->as.module_scope, full_import_callback, &fctx);
            }
        }
    }
    
    fprintf(file, "\n");
}

/**
 * @brief 带导入支持的完整代码生成函数
 */
int cn_cgen_module_with_imports_to_file(CnIrModule *module, CnAstProgram *program,
                                         CnModuleLoader *loader,
                                         CnSemScope *global_scope,
                                         CnModuleId *module_id,
                                         const char *filename) {
    // 首先调用原有的生成函数（不包含导入声明）
    // 然后在适当位置插入导入声明
    // 为了简化实现，我们直接修改现有函数的行为
    
    if (!module || !filename) return -1;

    /* 根据 IR 模块上的目标三元组获取预设数据布局（若存在）。 */
    CnTargetDataLayout layout;
    bool layout_ok = cn_support_target_get_data_layout(&module->target, &layout);
    if (layout_ok) {
        g_target_layout = layout;
        g_target_layout_valid = true;
    } else {
        g_target_layout_valid = false;
    }

    FILE *file = fopen(filename, "w");
    if (!file) return -1;
    
    /* 性能优化：设置较大的缓冲区，减少系统调用 */
    char *buffer = malloc(CGEN_BUFFER_SIZE);
    if (buffer) {
        setvbuf(file, buffer, _IOFBF, CGEN_BUFFER_SIZE);
    }
    
    CnCCodeGenContext ctx = {0};
    ctx.output_file = file;
    ctx.module = module;
    ctx.module_id = module_id;  // 设置模块ID用于生成带前缀的函数名
    ctx.program = program;      // 设置程序AST，用于查找类定义

    // 根据编译模式生成不同的头文件
    if (module->compile_mode == CN_COMPILE_MODE_FREESTANDING) {
        fprintf(file, "#include <stddef.h>\n#include <stdbool.h>\n#include <stdint.h>\n\n");
        fprintf(file, "// CN Language Runtime Function Declarations (Freestanding Mode)\n");
        fprintf(file, "void cn_rt_print_string(const char *str);\n");
        fprintf(file, "void cn_rt_print_int(long long val);\n");
        fprintf(file, "void cn_rt_print_float(double val);\n");
        fprintf(file, "void cn_rt_print_bool(int val);\n");
        fprintf(file, "size_t cn_rt_string_length(const char *str);\n");
        fprintf(file, "char* cn_rt_string_concat(const char *a, const char *b);\n");
        fprintf(file, "char* cn_rt_int_to_string(long long val);\n");
        fprintf(file, "char* cn_rt_bool_to_string(int val);\n");
        fprintf(file, "char* cn_rt_float_to_string(double val);\n");
        fprintf(file, "void* cn_rt_array_alloc(size_t elem_size, size_t count);\n");
        fprintf(file, "size_t cn_rt_array_length(void *arr);\n");
        fprintf(file, "int cn_rt_array_set_element(void *arr, size_t index, const void *element, size_t elem_size);\n");
        fprintf(file, "\n");
    } else {
        fprintf(file, "#include <stdio.h>\n#include <stdbool.h>\n#include <stdint.h>\n#include \"cnrt.h\"\n");
        fprintf(file, "#include \"cnlang/runtime/system_api.h\"\n\n");
    }
    
    // 生成结构体定义（如果提供了 AST）
    LocalStructInfo *local_struct_infos = NULL;
    size_t local_struct_count = 0;
    size_t local_struct_capacity = 0;
    
    if (program) {
        if (program->struct_count > 0) {
            fprintf(file, "// CN Language Global Struct Definitions\n");
            for (size_t i = 0; i < program->struct_count; i++) {
                cn_cgen_struct_decl(&ctx, program->structs[i]);
            }
        }
        
        for (size_t i = 0; i < program->function_count; i++) {
            collect_local_structs_from_function(program->functions[i], &local_struct_infos, &local_struct_count, &local_struct_capacity);
        }
        
        if (local_struct_count > 0) {
            fprintf(file, "// CN Language Local Struct Definitions (hoisted for C compatibility)\n");
            fprintf(file, "// Note: These are local to their respective functions in CN semantics\n");
            for (size_t i = 0; i < local_struct_count; i++) {
                cn_cgen_struct_decl_with_prefix(&ctx,
                                               local_struct_infos[i].struct_stmt,
                                               local_struct_infos[i].function_name,
                                               local_struct_infos[i].function_name_length);
            }
            
            g_local_struct_infos = local_struct_infos;
            g_local_struct_count = local_struct_count;
        }
    }
        
    if (program && program->enum_count > 0) {
        fprintf(file, "// CN Language Enum Definitions\n");
        for (size_t i = 0; i < program->enum_count; i++) {
            cn_cgen_enum_decl(&ctx, program->enums[i]);
        }
    }
    
    // 生成接口定义（必须在类定义之前，因为类的vtable引用接口vtable）
    if (program && program->interface_count > 0) {
        fprintf(file, "// CN Language Interface Definitions\n\n");
        for (size_t i = 0; i < program->interface_count; i++) {
            CnAstStmt *interface_stmt = program->interfaces[i];
            if (interface_stmt && interface_stmt->kind == CN_AST_STMT_INTERFACE_DECL) {
                cn_cgen_interface_decl(&ctx, interface_stmt->as.interface_decl);
            }
        }
    }
    
    if (program && program->class_count > 0) {
        fprintf(file, "// CN Language Class Definitions\n\n");
        for (size_t i = 0; i < program->class_count; i++) {
            CnAstStmt *class_stmt = program->classes[i];
            if (class_stmt && class_stmt->kind == CN_AST_STMT_CLASS_DECL) {
                cn_cgen_class_decl(&ctx, class_stmt->as.class_decl);
            }
        }
    }
    
    // 生成全局变量声明
    fprintf(file, "// Global Variables\n");
    CnIrGlobalVar *global = module->first_global;
    while (global) {
        fprintf(file, "%s cn_var_%s",
                get_c_type_string(global->type),
                global->name);
        
        if (global->initializer.kind != CN_IR_OP_NONE) {
            fprintf(file, " = ");
            if (global->initializer.kind == CN_IR_OP_IMM_INT) {
                fprintf(file, "%lld", global->initializer.as.imm_int);
            } else if (global->initializer.kind == CN_IR_OP_IMM_FLOAT) {
                fprintf(file, "%f", global->initializer.as.imm_float);
            }
        }
        
        fprintf(file, ";\n");
        global = global->next;
    }
    fprintf(file, "\n");
    
    // 生成导入模块的前向声明（新增功能）
    if (program && global_scope) {
        cn_cgen_import_forward_declarations(file, program, global_scope);
    }
    
    // 生成函数前置声明（Forward Declarations）
    fprintf(file, "// Forward Declarations\n");
    CnIrFunction *func = module->first_func;
    
    // 首先收集所有需要 #undef 的运行时宏冲突函数名
    // 在前向声明之前统一生成 #undef，避免宏展开导致重定义
    bool has_macro_conflict = false;
    CnIrFunction *temp_func = func;
    while (temp_func) {
        if (is_runtime_macro_conflict(temp_func->name)) {
            if (!has_macro_conflict) {
                fprintf(file, "// 取消运行时宏定义，避免与用户函数名冲突\n");
                has_macro_conflict = true;
            }
            fprintf(file, "#undef %s\n", temp_func->name);
        }
        temp_func = temp_func->next;
    }
    if (has_macro_conflict) {
        fprintf(file, "\n");
    }
    
    // 生成前向声明
    while (func) {
        // 直接使用原始函数名（不再使用编码名称）
        const char *c_func_name = get_c_function_name(func->name);
        
        fprintf(file, "%s %s(", get_c_type_string(func->return_type), c_func_name);
        for (size_t i = 0; i < func->param_count; i++) {
            fprintf(file, "%s", get_c_type_string(func->params[i].type));
            if (i < func->param_count - 1) fprintf(file, ", ");
        }
        fprintf(file, ");\n");
        func = func->next;
    }
    fprintf(file, "\n");

    func = module->first_func;
    while (func) {
        cn_cgen_function(&ctx, func);
        func = func->next;
    }
    
    // 清理全局查找表
    g_local_struct_infos = NULL;
    g_local_struct_count = 0;
    
    // 释放局部结构体信息列表
    if (local_struct_infos) {
        free(local_struct_infos);
    }
    
    fclose(file);
    
    /* 释放缓冲区 */
    if (buffer) {
        free(buffer);
    }
    
    return 0;
}
