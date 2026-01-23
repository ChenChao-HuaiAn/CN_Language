#include "cnlang/backend/cgen.h"
#include "cnlang/support/diagnostics.h"
#include "cnlang/support/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

static CnTargetDataLayout g_target_layout;
static bool g_target_layout_valid = false;

// --- 辅助函数 ---

static const char *get_c_type_string(CnType *type) {
    if (!type) return "void";
    switch (type->kind) {
        case CN_TYPE_INT:
            if (g_target_layout_valid) {
                if (g_target_layout.int_size_in_bits == 32) return "int";
                if (g_target_layout.int_size_in_bits == 64) return "long long";
            }
            return "long long";
        case CN_TYPE_FLOAT: return "double";
        case CN_TYPE_BOOL: return "_Bool";
        case CN_TYPE_STRING: return "char*";
        case CN_TYPE_POINTER: {
            static char buffer[256];
            snprintf(buffer, sizeof(buffer), "%s*", get_c_type_string(type->as.pointer_to));
            return buffer;
        }
        case CN_TYPE_VOID: return "void";
        default: return "int";
    }
}

static const char *get_c_function_name(const char *name) {
    if (!name) return "unnamed_func";
    // 使用 hex 编码匹配 UTF-8 标识符，避免编译器执行字符集干扰
    // 打印: \xe6\x89\x93\xe5\x8d\xb0
    if (strcmp(name, "\xe6\x89\x93\xe5\x8d\xb0") == 0) return "cn_rt_print_string";
    // 长度: \xe9\x95\xbf\xe5\xba\xa6
    if (strcmp(name, "\xe9\x95\xbf\xe5\xba\xa6") == 0) return "cn_rt_string_length";
    // 主程序: \xe4\xb8\xbb\xe7\xa8\x8b\xe5\xba\x8f
    if (strcmp(name, "\xe4\xb8\xbb\xe7\xa8\x8b\xe5\xba\x8f") == 0) return "main";
    static char buffer[256];
    snprintf(buffer, sizeof(buffer), "cn_func_%s", name);
    return buffer;
}

static const char *get_c_variable_name(const char *name) {
    static char buffer[256];
    snprintf(buffer, sizeof(buffer), "cn_var_%s", name);
    return buffer;
}

static void print_operand(CnCCodeGenContext *ctx, CnIrOperand op) {
    switch (op.kind) {
        case CN_IR_OP_NONE: fprintf(ctx->output_file, "/* NONE */"); break;
        case CN_IR_OP_REG: fprintf(ctx->output_file, "r%d", op.as.reg_id); break;
        case CN_IR_OP_IMM_INT: fprintf(ctx->output_file, "%lld", op.as.imm_int); break;
        case CN_IR_OP_IMM_STR: fprintf(ctx->output_file, "%s", op.as.imm_str); break;
        case CN_IR_OP_SYMBOL: fprintf(ctx->output_file, "%s", get_c_variable_name(op.as.sym_name)); break;
        case CN_IR_OP_LABEL: fprintf(ctx->output_file, "%s", op.as.label->name ? op.as.label->name : "unnamed_label"); break;
        default: fprintf(ctx->output_file, "/* UNKNOWN */"); break;
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
                default: break;
            }
            print_operand(ctx, inst->src2); fprintf(ctx->output_file, ";\n"); break;
        case CN_IR_INST_RET: fprintf(ctx->output_file, "  return"); if (inst->src1.kind != CN_IR_OP_NONE) { fprintf(ctx->output_file, " "); print_operand(ctx, inst->src1); } fprintf(ctx->output_file, ";\n"); break;
        case CN_IR_INST_JUMP: fprintf(ctx->output_file, "  goto %s;\n", inst->dest.as.label->name); break;
        case CN_IR_INST_BRANCH: fprintf(ctx->output_file, "  if ("); print_operand(ctx, inst->src1); fprintf(ctx->output_file, ") goto %s; else goto %s;\n", inst->dest.as.label->name, inst->src2.as.label->name); break;
        case CN_IR_INST_CALL:
            fprintf(ctx->output_file, "  ");
            if (inst->dest.kind != CN_IR_OP_NONE) { print_operand(ctx, inst->dest); fprintf(ctx->output_file, " = "); }
            if (inst->src1.kind == CN_IR_OP_SYMBOL) {
                fprintf(ctx->output_file, "%s(", get_c_function_name(inst->src1.as.sym_name));
            } else {
                print_operand(ctx, inst->src1);
                fprintf(ctx->output_file, "(");
            }
            for (size_t i = 0; i < inst->extra_args_count; i++) { print_operand(ctx, inst->extra_args[i]); if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", "); }
            fprintf(ctx->output_file, ");\n"); break;
        case CN_IR_INST_MOV: fprintf(ctx->output_file, "  "); print_operand(ctx, inst->dest); fprintf(ctx->output_file, " = "); print_operand(ctx, inst->src1); fprintf(ctx->output_file, ";\n"); break;
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
    
    fprintf(ctx->output_file, "%s %s(", get_c_type_string(func->return_type), c_func_name);
    for (size_t i = 0; i < func->param_count; i++) {
        fprintf(ctx->output_file, "%s %s", get_c_type_string(func->params[i].type), get_c_variable_name(func->params[i].as.sym_name));
        if (i < func->param_count - 1) fprintf(ctx->output_file, ", ");
    }
    fprintf(ctx->output_file, ") {\n");
    
    // 注入运行时初始化（仅 hosted 模式）
    if (is_main && ctx->module && ctx->module->compile_mode != CN_COMPILE_MODE_FREESTANDING) {
        fprintf(ctx->output_file, "  cn_rt_init();\n");
    }

    // 声明虚拟寄存器：根据IR指令中的类型信息收集寄存器类型
    if (func->next_reg_id > 0) {
        // 第一步：遍历所有指令收集寄存器类型信息
        CnType **reg_types = calloc(func->next_reg_id, sizeof(CnType*));
        CnIrBasicBlock *scan_block = func->first_block;
        while (scan_block) {
            CnIrInst *scan_inst = scan_block->first_inst;
            while (scan_inst) {
                // 收集目标寄存器类型
                if (scan_inst->dest.kind == CN_IR_OP_REG && scan_inst->dest.as.reg_id < func->next_reg_id) {
                    if (!reg_types[scan_inst->dest.as.reg_id] && scan_inst->dest.type) {
                        reg_types[scan_inst->dest.as.reg_id] = scan_inst->dest.type;
                    }
                }
                scan_inst = scan_inst->next;
            }
            scan_block = scan_block->next;
        }
        
        // 第二步：按类型分组声明寄存器
        // 声明整型寄存器
        bool has_int_regs = false;
        for (int i = 0; i < func->next_reg_id; i++) {
            if (!reg_types[i] || reg_types[i]->kind == CN_TYPE_INT) {
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
        
        // 声明字符串寄存器（每个寄存器单独声明以确保每个都是指针类型）
        for (int i = 0; i < func->next_reg_id; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_STRING) {
                fprintf(ctx->output_file, "  char* r%d;\n", i);
            }
        }
        
        free(reg_types);
    }

    CnIrBasicBlock *block = func->first_block;
    while (block) { cn_cgen_block(ctx, block); block = block->next; }

    // 注入运行时退出（仅 hosted 模式）
    if (is_main && ctx->module && ctx->module->compile_mode != CN_COMPILE_MODE_FREESTANDING) {
        fprintf(ctx->output_file, "  cn_rt_exit();\n");
    }
    
    fprintf(ctx->output_file, "}\n\n");
}

int cn_cgen_module_to_file(CnIrModule *module, const char *filename) {
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
    CnCCodeGenContext ctx = {0};
    ctx.output_file = file;
    ctx.module = module;  // 设置模块引用

    // 根据编译模式生成不同的头文件
    if (module->compile_mode == CN_COMPILE_MODE_FREESTANDING) {
        // Freestanding 模式：不包含运行时库头文件，但声明必要的运行时函数
        fprintf(file, "#include <stddef.h>\n#include <stdbool.h>\n#include <stdint.h>\n\n");
        fprintf(file, "// CN Language Runtime Function Declarations (Freestanding Mode)\n");
        fprintf(file, "void cn_rt_print_string(const char *str);\n");
        fprintf(file, "void cn_rt_print_int(long long val);\n");
        fprintf(file, "size_t cn_rt_string_length(const char *str);\n\n");
    } else {
        // Hosted 模式：包含完整运行时库
        fprintf(file, "#include <stdio.h>\n#include <stdbool.h>\n#include <stdint.h>\n#include \"cnrt.h\"\n\n");
    }

    CnIrFunction *func = module->first_func;
    while (func) { cn_cgen_function(&ctx, func); func = func->next; }
    fclose(file);
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
