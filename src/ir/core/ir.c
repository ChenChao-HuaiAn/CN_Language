#include "cnlang/ir/ir.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

CnIrModule *cn_ir_module_new() {
    CnIrModule *module = (CnIrModule *)malloc(sizeof(CnIrModule));
    if (module) {
        module->first_func = NULL;
        module->last_func = NULL;
        /* 默认将目标三元组置零，具体值由前端/CLI 在生成 IR 前设置 */
        memset(&module->target, 0, sizeof(module->target));
    }
    return module;
}

static void cn_ir_block_list_free(CnIrBasicBlockList *list) {
    while (list) {
        CnIrBasicBlockList *next = list->next;
        free(list);
        list = next;
    }
}

void cn_ir_module_free(CnIrModule *module) {
    if (!module) return;
    CnIrFunction *func = module->first_func;
    while (func) {
        CnIrFunction *next = func->next;
        CnIrBasicBlock *block = func->first_block;
        while (block) {
            CnIrBasicBlock *next_b = block->next;
            CnIrInst *inst = block->first_inst;
            while (inst) {
                CnIrInst *next_i = inst->next;
                if (inst->extra_args) free(inst->extra_args);
                free(inst);
                inst = next_i;
            }
            cn_ir_block_list_free(block->preds);
            cn_ir_block_list_free(block->succs);
            if (block->name) free((void *)block->name);
            free(block);
            block = next_b;
        }
        if (func->name) free((void *)func->name);
        if (func->params) free(func->params);
        if (func->locals) free(func->locals);
        free(func);
        func = next;
    }
    free(module);
}

CnIrFunction *cn_ir_function_new(const char *name, CnType *return_type) {
    CnIrFunction *func = (CnIrFunction *)malloc(sizeof(CnIrFunction));
    if (func) {
        func->name = name ? strdup(name) : NULL;
        // 注意：这里的 name 应该是已经 null-terminated 的。
        // 如果是从 AST 获取的，irgen 应该负责传递 null-terminated 的字符串。
        func->return_type = return_type;
        func->params = NULL;
        func->param_count = 0;
        func->locals = NULL;
        func->local_count = 0;
        func->first_block = NULL;
        func->last_block = NULL;
        func->next_reg_id = 0;
        func->next = NULL;
    }
    return func;
}

void cn_ir_function_add_param(CnIrFunction *func, CnIrOperand param) {
    if (!func) return;
    func->params = realloc(func->params, (func->param_count + 1) * sizeof(CnIrOperand));
    func->params[func->param_count++] = param;
}

void cn_ir_function_add_local(CnIrFunction *func, CnIrOperand local) {
    if (!func) return;
    func->locals = realloc(func->locals, (func->local_count + 1) * sizeof(CnIrOperand));
    func->locals[func->local_count++] = local;
}

void cn_ir_function_add_block(CnIrFunction *func, CnIrBasicBlock *block) {
    if (!func || !block) return;
    if (!func->first_block) {
        func->first_block = block;
        func->last_block = block;
    } else {
        func->last_block->next = block;
        block->prev = func->last_block;
        func->last_block = block;
    }
}

CnIrBasicBlock *cn_ir_basic_block_new(const char *name_hint) {
    CnIrBasicBlock *block = (CnIrBasicBlock *)malloc(sizeof(CnIrBasicBlock));
    if (block) {
        block->name = name_hint ? strdup(name_hint) : NULL;
        block->first_inst = NULL;
        block->last_inst = NULL;
        block->preds = NULL;
        block->succs = NULL;
        block->next = NULL;
        block->prev = NULL;
    }
    return block;
}

void cn_ir_basic_block_add_inst(CnIrBasicBlock *block, CnIrInst *inst) {
    if (!block || !inst) return;
    if (!block->first_inst) {
        block->first_inst = inst;
        block->last_inst = inst;
    } else {
        block->last_inst->next = inst;
        inst->prev = block->last_inst;
        block->last_inst = inst;
    }
}

static void add_block_to_list(CnIrBasicBlockList **list, CnIrBasicBlock *block) {
    CnIrBasicBlockList *node = malloc(sizeof(CnIrBasicBlockList));
    node->block = block;
    node->next = *list;
    *list = node;
}

void cn_ir_basic_block_connect(CnIrBasicBlock *from, CnIrBasicBlock *to) {
    if (!from || !to) return;
    add_block_to_list(&from->succs, to);
    add_block_to_list(&to->preds, from);
}

CnIrInst *cn_ir_inst_new(CnIrInstKind kind, CnIrOperand dest, CnIrOperand src1, CnIrOperand src2) {
    CnIrInst *inst = (CnIrInst *)malloc(sizeof(CnIrInst));
    if (inst) {
        inst->kind = kind;
        inst->dest = dest;
        inst->src1 = src1;
        inst->src2 = src2;
        inst->extra_args = NULL;
        inst->extra_args_count = 0;
        inst->next = NULL;
        inst->prev = NULL;
    }
    return inst;
}

CnIrOperand cn_ir_op_none() {
    CnIrOperand op;
    op.kind = CN_IR_OP_NONE;
    op.type = NULL;
    return op;
}

CnIrOperand cn_ir_op_reg(int reg_id, CnType *type) {
    CnIrOperand op;
    op.kind = CN_IR_OP_REG;
    op.as.reg_id = reg_id;
    op.type = type;
    return op;
}

CnIrOperand cn_ir_op_imm_int(long long val, CnType *type) {
    CnIrOperand op;
    op.kind = CN_IR_OP_IMM_INT;
    op.as.imm_int = val;
    op.type = type;
    return op;
}

CnIrOperand cn_ir_op_label(CnIrBasicBlock *block) {
    CnIrOperand op;
    op.kind = CN_IR_OP_LABEL;
    op.as.label = block;
    op.type = NULL;
    return op;
}

CnIrOperand cn_ir_op_symbol(const char *name, CnType *type) {
    CnIrOperand op;
    op.kind = CN_IR_OP_SYMBOL;
    op.as.sym_name = name ? strdup(name) : NULL;
    op.type = type;
    return op;
}

static const char *inst_names[] = {
    "add", "sub", "mul", "div", "mod", "and", "or", "xor", "shl", "shr", "neg", "not",
    "eq", "ne", "lt", "le", "gt", "ge",
    "alloca", "load", "store", "mov",
    "label", "jump", "branch", "call", "ret",
    "phi"
};

void cn_ir_dump_operand_to_file(CnIrOperand op, FILE *file) {
    switch (op.kind) {
        case CN_IR_OP_NONE: fprintf(file, "none"); break;
        case CN_IR_OP_REG: fprintf(file, "%%r%d", op.as.reg_id); break;
        case CN_IR_OP_IMM_INT: fprintf(file, "%lld", op.as.imm_int); break;
        case CN_IR_OP_IMM_STR: fprintf(file, "\"%s\"", op.as.imm_str); break;
        case CN_IR_OP_SYMBOL: fprintf(file, "@%s", op.as.sym_name); break;
        case CN_IR_OP_LABEL: fprintf(file, "%s", op.as.label->name ? op.as.label->name : "unnamed"); break;
        default: fprintf(file, "unknown"); break;
    }
}

void cn_ir_dump_operand(CnIrOperand op) {
    cn_ir_dump_operand_to_file(op, stdout);
}

void cn_ir_dump_inst_to_file(CnIrInst *inst, FILE *file) {
    if (!inst) return;
    fprintf(file, "  ");
    if (inst->kind == CN_IR_INST_LABEL) {
        fprintf(file, "%s:", inst->dest.as.sym_name);
    } else if (inst->kind == CN_IR_INST_STORE) {
        fprintf(file, "store ");
        cn_ir_dump_operand_to_file(inst->src1, file); // value
        fprintf(file, ", ");
        cn_ir_dump_operand_to_file(inst->dest, file); // address
    } else if (inst->kind == CN_IR_INST_JUMP) {
        fprintf(file, "jump ");
        cn_ir_dump_operand_to_file(inst->dest, file);
    } else if (inst->kind == CN_IR_INST_BRANCH) {
        fprintf(file, "branch ");
        cn_ir_dump_operand_to_file(inst->src1, file); // cond
        fprintf(file, ", ");
        cn_ir_dump_operand_to_file(inst->dest, file); // true label
        fprintf(file, ", ");
        cn_ir_dump_operand_to_file(inst->src2, file); // false label
    } else if (inst->kind == CN_IR_INST_RET) {
        fprintf(file, "ret ");
        if (inst->src1.kind != CN_IR_OP_NONE) cn_ir_dump_operand_to_file(inst->src1, file);
    } else if (inst->kind == CN_IR_INST_CALL) {
        if (inst->dest.kind != CN_IR_OP_NONE) {
            cn_ir_dump_operand_to_file(inst->dest, file);
            fprintf(file, " = ");
        }
        fprintf(file, "call ");
        cn_ir_dump_operand_to_file(inst->src1, file); // callee
        fprintf(file, "(");
        for (size_t i = 0; i < inst->extra_args_count; i++) {
            cn_ir_dump_operand_to_file(inst->extra_args[i], file);
            if (i < inst->extra_args_count - 1) fprintf(file, ", ");
        }
        fprintf(file, ")");
    } else {
        // Default binary/unary format: dest = op src1 [, src2]
        if (inst->dest.kind != CN_IR_OP_NONE) {
            cn_ir_dump_operand_to_file(inst->dest, file);
            fprintf(file, " = ");
        }
        fprintf(file, "%s ", inst_names[inst->kind]);
        cn_ir_dump_operand_to_file(inst->src1, file);
        if (inst->src2.kind != CN_IR_OP_NONE) {
            fprintf(file, ", ");
            cn_ir_dump_operand_to_file(inst->src2, file);
        }
    }
    fprintf(file, "\n");
}

void cn_ir_dump_inst(CnIrInst *inst) {
    cn_ir_dump_inst_to_file(inst, stdout);
}

void cn_ir_dump_block_to_file(CnIrBasicBlock *block, FILE *file) {
    if (!block) return;
    fprintf(file, "%s:\n", block->name ? block->name : "unnamed");
    CnIrInst *inst = block->first_inst;
    while (inst) {
        cn_ir_dump_inst_to_file(inst, file);
        inst = inst->next;
    }
}

void cn_ir_dump_block(CnIrBasicBlock *block) {
    cn_ir_dump_block_to_file(block, stdout);
}

void cn_ir_dump_function_to_file(CnIrFunction *func, FILE *file) {
    if (!func) return;
    fprintf(file, "fn @%s(", func->name);
    for (size_t i = 0; i < func->param_count; i++) {
        cn_ir_dump_operand_to_file(func->params[i], file);
        if (i < func->param_count - 1) fprintf(file, ", ");
    }
    fprintf(file, ") {\n");
    CnIrBasicBlock *block = func->first_block;
    while (block) {
        cn_ir_dump_block_to_file(block, file);
        block = block->next;
    }
    fprintf(file, "}\n\n");
}

void cn_ir_dump_function(CnIrFunction *func) {
    cn_ir_dump_function_to_file(func, stdout);
}

void cn_ir_dump_module_to_file(CnIrModule *module, FILE *file) {
    if (!module) return;
    CnIrFunction *func = module->first_func;
    while (func) {
        cn_ir_dump_function_to_file(func, file);
        func = func->next;
    }
}

void cn_ir_dump_module(CnIrModule *module) {
    cn_ir_dump_module_to_file(module, stdout);
}
