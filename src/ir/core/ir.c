#include "cnlang/ir/ir.h"
#include <stdlib.h>
#include <string.h>

CnIrModule *cn_ir_module_new() {
    CnIrModule *module = (CnIrModule *)malloc(sizeof(CnIrModule));
    if (module) {
        module->first_func = NULL;
        module->last_func = NULL;
    }
    return module;
}

void cn_ir_module_free(CnIrModule *module) {
    if (!module) return;
    CnIrFunction *func = module->first_func;
    while (func) {
        CnIrFunction *next = func->next;
        // 这里需要更深层次的释放逻辑，目前简写
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
            free(block);
            block = next_b;
        }
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
        func->first_block = NULL;
        func->last_block = NULL;
        func->next_reg_id = 0;
        func->next = NULL;
    }
    return func;
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
