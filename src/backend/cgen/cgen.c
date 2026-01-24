#include "cnlang/backend/cgen.h"
#include "cnlang/support/diagnostics.h"
#include "cnlang/support/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

/* 性能优化：输出缓冲区大小 */
#define CGEN_BUFFER_SIZE 8192

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
            /* 性能优化：使用线程局部缓冲区避免静态缓冲区竞争 */
            static _Thread_local char buffer[256];
            snprintf(buffer, sizeof(buffer), "%s*", get_c_type_string(type->as.pointer_to));
            return buffer;
        }
        case CN_TYPE_VOID: return "void";
        case CN_TYPE_ARRAY: return "void*";
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
        {"xe6x89x93xe5x8dxb0", "cn_rt_print_string"},  /* 打印 */
        {"xe9x95xbfxe5xbaxa6", "cn_rt_string_length"}, /* 长度 */
        {"xe4xb8xbbxe7xa8x8bxe5xbax8f", "main"},      /* 主程序 */
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
            
            // 特殊处理 cn_rt_array_get_element：需要解引用返回的指针
            if (inst->src1.kind == CN_IR_OP_SYMBOL && 
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
        /* 性能优化：使用栈分配替代 calloc，避免堆分配开销 */
        CnType *reg_types_stack[256];  /* 大多数函数的寄存器数量 < 256 */
        CnType **reg_types = NULL;
        bool use_heap = false;
        
        if (func->next_reg_id <= 256) {
            /* 小寄存器集合使用栈分配 */
            memset(reg_types_stack, 0, func->next_reg_id * sizeof(CnType*));
            reg_types = reg_types_stack;
        } else {
            /* 大寄存器集合使用堆分配 */
            reg_types = calloc(func->next_reg_id, sizeof(CnType*));
            use_heap = true;
            if (!reg_types) {
                /* 内存分配失败，使用默认类型 */
                fprintf(ctx->output_file, "  long long r0");
                for (int i = 1; i < func->next_reg_id; i++) {
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
                if (scan_inst->dest.kind == CN_IR_OP_REG && 
                    scan_inst->dest.as.reg_id < func->next_reg_id &&
                    !reg_types[scan_inst->dest.as.reg_id] && 
                    scan_inst->dest.type) {
                    reg_types[scan_inst->dest.as.reg_id] = scan_inst->dest.type;
                }
                scan_inst = scan_inst->next;
            }
            scan_block = scan_block->next;
        }
        
        /* 性能优化：按类型分组声明寄存器，减少 fprintf 调用 */
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
        
        /* 声明字符串寄存器（每个寄存器单独声明以确保每个都是指针类型） */
        for (int i = 0; i < func->next_reg_id; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_STRING) {
                fprintf(ctx->output_file, "  char* r%d;\n", i);
            }
        }
        
        /* 声明数组寄存器（void* 用于数组指针） */
        for (int i = 0; i < func->next_reg_id; i++) {
            if (reg_types[i] && reg_types[i]->kind == CN_TYPE_ARRAY) {
                fprintf(ctx->output_file, "  void* r%d;\n", i);
            }
        }
        
        if (use_heap) {
            free(reg_types);
        }
        
    skip_register_type_scan:;
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
        fprintf(file, "size_t cn_rt_string_length(const char *str);\n");
        fprintf(file, "void* cn_rt_array_alloc(size_t elem_size, size_t count);\n");
        fprintf(file, "size_t cn_rt_array_length(void *arr);\n");
        fprintf(file, "int cn_rt_array_set_element(void *arr, size_t index, const void *element, size_t elem_size);\n");
        fprintf(file, "\n");
    } else {
        // Hosted 模式：包含完整运行时库
        fprintf(file, "#include <stdio.h>\n#include <stdbool.h>\n#include <stdint.h>\n#include \"cnrt.h\"\n\n");
    }

    CnIrFunction *func = module->first_func;
    while (func) { cn_cgen_function(&ctx, func); func = func->next; }
    
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
