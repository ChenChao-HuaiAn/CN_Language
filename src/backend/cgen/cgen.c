#include "cnlang/backend/cgen.h"
#include "cnlang/support/diagnostics.h"
#include "cnlang/support/config.h"
#include "cnlang/frontend/semantics.h"  // 引入作用域和类型信息
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

// --- 辅助函数 ---

static const char *get_c_type_string(CnType *type) {
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
        case CN_TYPE_ARRAY: return "void*";
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
        {NULL, NULL}
    };
    
    for (int i = 0; function_map[i].utf8_name; i++) {
        if (strcmp(name, function_map[i].utf8_name) == 0) {
            return function_map[i].c_name;
        }
    }
    
    /* 性能优化：使用线程局部缓冲区 */
    static _Thread_local char buffer[256];
    snprintf(buffer, sizeof(buffer), "cn_func_%s", name);
    return buffer;
}

static const char *get_c_variable_name(const char *name) {
    /* 性能优化：使用线程局部缓冲区 */
    static _Thread_local char buffer[256];
    
    // 检查是否已经有 cn_module_ 或 cn_var_ 前缀（避免重复加前缀）
    if (strncmp(name, "cn_module_", 10) == 0 || strncmp(name, "cn_var_", 7) == 0) {
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
            // 支持引用其他模块变量
            // 先简单处理：直接输出变量名（需要结合符号表处理模块前缀）
            fprintf(ctx->output_file, "cn_var_%.*s",
                    (int)expr->as.identifier.name_length,
                    expr->as.identifier.name);
            break;
        case CN_AST_EXPR_BINARY:
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
                
                fprintf(ctx->output_file, "%s(", get_c_function_name(inst->src1.as.sym_name));
                
                // 第1个参数：数组指针
                print_operand(ctx, inst->extra_args[0]);
                fprintf(ctx->output_file, ", ");
                
                // 第2个参数：索引
                print_operand(ctx, inst->extra_args[1]);
                fprintf(ctx->output_file, ", ");
                
                // 第3个参数：元素值 - 需要取地址
                CnIrOperand elem = inst->extra_args[2];
                if (elem.kind == CN_IR_OP_IMM_INT || elem.kind == CN_IR_OP_REG) {
                    // 生成临时变量
                    static int temp_var_counter = 0;
                    fprintf(ctx->output_file, "({");
                    fprintf(ctx->output_file, "long long _tmp_%d = ", temp_var_counter);
                    print_operand(ctx, elem);
                    fprintf(ctx->output_file, "; &_tmp_%d;", temp_var_counter);
                    fprintf(ctx->output_file, "})");
                    temp_var_counter++;
                } else {
                    // 已经是符号类型，直接取地址
                    fprintf(ctx->output_file, "&");
                    print_operand(ctx, elem);
                }
                
                // 第4个参数：元素大小
                if (inst->extra_args_count >= 4) {
                    fprintf(ctx->output_file, ", ");
                    print_operand(ctx, inst->extra_args[3]);
                }
                
                fprintf(ctx->output_file, ");\n");
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
    const char *c_func_name = get_c_function_name(func->name);
    bool is_main = strcmp(c_func_name, "main") == 0;
    
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
    
    // 在 main 函数中调用模块初始化函数
    if (is_main && ctx->module_init_infos && ctx->module_init_count > 0) {
        typedef struct {
            CnAstModuleDecl *module_decl;
            bool has_complex_init;
        } ModuleInitInfo;
        
        ModuleInitInfo *infos = (ModuleInitInfo *)ctx->module_init_infos;
        for (size_t i = 0; i < ctx->module_init_count; i++) {
            if (infos[i].has_complex_init && infos[i].module_decl) {
                fprintf(ctx->output_file, "  __cn_init_module_%.*s();\n",
                        (int)infos[i].module_decl->name_length, 
                        infos[i].module_decl->name);
            }
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
        
        if (use_heap) {
            free(reg_types);
        }
        
    skip_register_type_scan:;
    }

    CnIrBasicBlock *block = func->first_block;
    while (block) { cn_cgen_block(ctx, block); block = block->next; }

    // 中断处理函数默认返回0（如果没有显式return）
    if (func->is_interrupt_handler) {
        fprintf(ctx->output_file, "  return 0;\n");
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
        
        // 收集模块内函数中的局部结构体
        for (size_t i = 0; i < program->module_count; i++) {
            CnAstStmt *module_stmt = program->modules[i];
            if (module_stmt && module_stmt->kind == CN_AST_STMT_MODULE_DECL) {
                CnAstModuleDecl *module_decl = &module_stmt->as.module_decl;
                for (size_t j = 0; j < module_decl->function_count; j++) {
                    collect_local_structs_from_function(module_decl->functions[j], &local_struct_infos, &local_struct_count, &local_struct_capacity);
                }
            }
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
    
    // 生成模块变量定义（如果提供了 AST）
    // 同时收集需要复杂初始化的模块
    typedef struct {
        CnAstModuleDecl *module_decl;
        bool has_complex_init;
    } ModuleInitInfo;
    
    ModuleInitInfo *module_init_infos = NULL;
    size_t module_init_count = 0;
    
    if (program && program->module_count > 0) {
        fprintf(file, "// CN Language Module Variables\n");
        module_init_infos = calloc(program->module_count, sizeof(ModuleInitInfo));
        
        for (size_t i = 0; i < program->module_count; i++) {
            CnAstStmt *module_stmt = program->modules[i];
            if (module_stmt && module_stmt->kind == CN_AST_STMT_MODULE_DECL) {
                CnAstModuleDecl *module_decl = &module_stmt->as.module_decl;
                bool has_complex = false;
                
                // 遍历模块中的语句，生成变量声明
                for (size_t j = 0; j < module_decl->stmt_count; j++) {
                    CnAstStmt *stmt = module_decl->stmts[j];
                    if (stmt && stmt->kind == CN_AST_STMT_VAR_DECL) {
                        CnAstVarDecl *var_decl = &stmt->as.var_decl;
                        
                        // 生成带模块前缀的变量名
                        fprintf(file, "%s cn_module_%.*s__%.*s",
                                get_c_type_string(var_decl->declared_type ? var_decl->declared_type : 
                                                  (var_decl->initializer ? var_decl->initializer->type : NULL)),
                                (int)module_decl->name_length, module_decl->name,
                                (int)var_decl->name_length, var_decl->name);
                        
                        // 如果有初始化表达式，生成初始化值
                        if (var_decl->initializer) {
                            // 检查是否是字面量初始化
                            bool is_literal = false;
                            
                            if (var_decl->initializer->kind == CN_AST_EXPR_INTEGER_LITERAL) {
                                fprintf(file, " = %lld", var_decl->initializer->as.integer_literal.value);
                                is_literal = true;
                            } else if (var_decl->initializer->kind == CN_AST_EXPR_FLOAT_LITERAL) {
                                fprintf(file, " = %f", var_decl->initializer->as.float_literal.value);
                                is_literal = true;
                            } else if (var_decl->initializer->kind == CN_AST_EXPR_STRING_LITERAL) {
                                // 字符串字面量：需要处理转义字符
                                fprintf(file, " = \"");
                                for (const char *p = var_decl->initializer->as.string_literal.value; *p; p++) {
                                    switch (*p) {
                                        case '\\': fprintf(file, "\\\\"); break;
                                        case '\"': fprintf(file, "\\\""); break;
                                        case '\n': fprintf(file, "\\n"); break;
                                        case '\r': fprintf(file, "\\r"); break;
                                        case '\t': fprintf(file, "\\t"); break;
                                        default: fprintf(file, "%c", *p); break;
                                    }
                                }
                                fprintf(file, "\"");
                                is_literal = true;
                            } else if (var_decl->initializer->kind == CN_AST_EXPR_BOOL_LITERAL) {
                                fprintf(file, " = %s", var_decl->initializer->as.bool_literal.value ? "true" : "false");
                                is_literal = true;
                            } else {
                                // 复杂表达式：先用默认值0，稍后在初始化函数中赋值
                                fprintf(file, " = 0");
                                has_complex = true;
                            }
                        }
                        
                        fprintf(file, ";\n");
                    }
                }
                fprintf(file, "\n");
                
                // 记录模块信息
                if (module_init_infos) {
                    module_init_infos[module_init_count].module_decl = module_decl;
                    module_init_infos[module_init_count].has_complex_init = has_complex;
                    module_init_count++;
                }
            }
        }
    }
    
    // 生成模块初始化函数（如果需要）
    if (module_init_infos && module_init_count > 0) {
        for (size_t i = 0; i < module_init_count; i++) {
            if (module_init_infos[i].has_complex_init) {
                CnAstModuleDecl *module_decl = module_init_infos[i].module_decl;
                fprintf(file, "void __cn_init_module_%.*s();", 
                        (int)module_decl->name_length, module_decl->name);
                fprintf(file, "\n");
            }
        }
        fprintf(file, "\n");
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

    // 生成模块初始化函数实现
    if (module_init_infos && module_init_count > 0) {
        for (size_t i = 0; i < module_init_count; i++) {
            if (module_init_infos[i].has_complex_init) {
                CnAstModuleDecl *module_decl = module_init_infos[i].module_decl;
                
                fprintf(file, "void __cn_init_module_%.*s() {\n",
                        (int)module_decl->name_length, module_decl->name);
                
                // 遍历模块变量，生成复杂初始化代码
                for (size_t j = 0; j < module_decl->stmt_count; j++) {
                    CnAstStmt *stmt = module_decl->stmts[j];
                    if (stmt && stmt->kind == CN_AST_STMT_VAR_DECL) {
                        CnAstVarDecl *var_decl = &stmt->as.var_decl;
                        
                        if (var_decl->initializer) {
                            // 检查是否是非字面量表达式
                            bool is_complex = true;
                            if (var_decl->initializer->kind == CN_AST_EXPR_INTEGER_LITERAL ||
                                var_decl->initializer->kind == CN_AST_EXPR_FLOAT_LITERAL ||
                                var_decl->initializer->kind == CN_AST_EXPR_STRING_LITERAL ||
                                var_decl->initializer->kind == CN_AST_EXPR_BOOL_LITERAL) {
                                is_complex = false;
                            }
                            
                            if (is_complex) {
                                // 生成赋值语句
                                fprintf(file, "  cn_module_%.*s__%.*s = ",
                                        (int)module_decl->name_length, module_decl->name,
                                        (int)var_decl->name_length, var_decl->name);
                                
                                // 生成表达式代码（简化版：只支持二元运算）
                                cn_cgen_expr_simple(&ctx, var_decl->initializer);
                                fprintf(file, ";\n");
                            }
                        }
                    }
                }
                
                fprintf(file, "}\n\n");
            }
        }
    }

    func = module->first_func;
    while (func) { 
        // 传递模块初始化信息到函数生成
        ctx.module_init_infos = module_init_infos;
        ctx.module_init_count = module_init_count;
        cn_cgen_function(&ctx, func); 
        func = func->next; 
    }
    
    // 释放模块初始化信息
    if (module_init_infos) {
        free(module_init_infos);
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
