#include "cnlang/ir/ir.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

CnIrModule *cn_ir_module_new() {
    CnIrModule *module = (CnIrModule *)malloc(sizeof(CnIrModule));
    if (module) {
        module->first_func = NULL;
        module->last_func = NULL;
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
    "alloca", "load", "store",
    "label", "jump", "branch", "call", "ret",
    "phi"
};

void cn_ir_dump_operand(CnIrOperand op) {
    switch (op.kind) {
        case CN_IR_OP_NONE: printf("none"); break;
        case CN_IR_OP_REG: printf("%%r%d", op.as.reg_id); break;
        case CN_IR_OP_IMM_INT: printf("%lld", op.as.imm_int); break;
        case CN_IR_OP_IMM_STR: printf("\"%s\"", op.as.imm_str); break;
        case CN_IR_OP_SYMBOL: printf("@%s", op.as.sym_name); break;
        case CN_IR_OP_LABEL: printf("%s", op.as.label->name ? op.as.label->name : "unnamed"); break;
        default: printf("unknown"); break;
    }
}

void cn_ir_dump_inst(CnIrInst *inst) {
    if (!inst) return;
    printf("  ");
    if (inst->kind == CN_IR_INST_LABEL) {
        printf("%s:", inst->dest.as.sym_name);
    } else if (inst->kind == CN_IR_INST_STORE) {
        printf("store ");
        cn_ir_dump_operand(inst->src1); // value
        printf(", ");
        cn_ir_dump_operand(inst->dest); // address
    } else if (inst->kind == CN_IR_INST_JUMP) {
        printf("jump ");
        cn_ir_dump_operand(inst->dest);
    } else if (inst->kind == CN_IR_INST_BRANCH) {
        printf("branch ");
        cn_ir_dump_operand(inst->src1); // cond
        printf(", ");
        cn_ir_dump_operand(inst->dest); // true label
        printf(", ");
        cn_ir_dump_operand(inst->src2); // false label
    } else if (inst->kind == CN_IR_INST_RET) {
        printf("ret ");
        if (inst->src1.kind != CN_IR_OP_NONE) cn_ir_dump_operand(inst->src1);
    } else if (inst->kind == CN_IR_INST_CALL) {
        if (inst->dest.kind != CN_IR_OP_NONE) {
            cn_ir_dump_operand(inst->dest);
            printf(" = ");
        }
        printf("call ");
        cn_ir_dump_operand(inst->src1); // callee
        printf("(");
        for (size_t i = 0; i < inst->extra_args_count; i++) {
            cn_ir_dump_operand(inst->extra_args[i]);
            if (i < inst->extra_args_count - 1) printf(", ");
        }
        printf(")");
    } else {
        // Default binary/unary format: dest = op src1 [, src2]
        if (inst->dest.kind != CN_IR_OP_NONE) {
            cn_ir_dump_operand(inst->dest);
            printf(" = ");
        }
        printf("%s ", inst_names[inst->kind]);
        cn_ir_dump_operand(inst->src1);
        if (inst->src2.kind != CN_IR_OP_NONE) {
            printf(", ");
            cn_ir_dump_operand(inst->src2);
        }
    }
    printf("\n");
}

void cn_ir_dump_block(CnIrBasicBlock *block) {
    if (!block) return;
    printf("%s:\n", block->name ? block->name : "unnamed");
    CnIrInst *inst = block->first_inst;
    while (inst) {
        cn_ir_dump_inst(inst);
        inst = inst->next;
    }
}

void cn_ir_dump_function(CnIrFunction *func) {
    if (!func) return;
    printf("fn @%s(", func->name);
    for (size_t i = 0; i < func->param_count; i++) {
        cn_ir_dump_operand(func->params[i]);
        if (i < func->param_count - 1) printf(", ");
    }
    printf(") {\n");
    CnIrBasicBlock *block = func->first_block;
    while (block) {
        cn_ir_dump_block(block);
        block = block->next;
    }
    printf("}\n\n");
}

void cn_ir_dump_module(CnIrModule *module) {
    if (!module) return;
    CnIrFunction *func = module->first_func;
    while (func) {
        cn_ir_dump_function(func);
        func = func->next;
    }
}
