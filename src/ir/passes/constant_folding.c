#include "cnlang/ir/pass.h"
#include <stdbool.h>

static bool is_constant(CnIrOperand op) {
    return op.kind == CN_IR_OP_IMM_INT;
}

static void fold_inst(CnIrInst *inst) {
    if (inst->kind >= CN_IR_INST_ADD && inst->kind <= CN_IR_INST_GE) {
        if (is_constant(inst->src1) && is_constant(inst->src2)) {
            long long v1 = inst->src1.as.imm_int;
            long long v2 = inst->src2.as.imm_int;
            long long result = 0;
            bool foldable = true;

            switch (inst->kind) {
                case CN_IR_INST_ADD: result = v1 + v2; break;
                case CN_IR_INST_SUB: result = v1 - v2; break;
                case CN_IR_INST_MUL: result = v1 * v2; break;
                case CN_IR_INST_DIV: if (v2 != 0) result = v1 / v2; else foldable = false; break;
                case CN_IR_INST_MOD: if (v2 != 0) result = v1 % v2; else foldable = false; break;
                case CN_IR_INST_AND: result = v1 & v2; break;
                case CN_IR_INST_OR:  result = v1 | v2; break;
                case CN_IR_INST_XOR: result = v1 ^ v2; break;
                case CN_IR_INST_SHL: result = v1 << v2; break;
                case CN_IR_INST_SHR: result = v1 >> v2; break;
                case CN_IR_INST_EQ:  result = v1 == v2; break;
                case CN_IR_INST_NE:  result = v1 != v2; break;
                case CN_IR_INST_LT:  result = v1 < v2; break;
                case CN_IR_INST_LE:  result = v1 <= v2; break;
                case CN_IR_INST_GT:  result = v1 > v2; break;
                case CN_IR_INST_GE:  result = v1 >= v2; break;
                default: foldable = false; break;
            }

            if (foldable) {
                inst->kind = CN_IR_INST_MOV;
                inst->src1.kind = CN_IR_OP_IMM_INT;
                inst->src1.as.imm_int = result;
                inst->src2.kind = CN_IR_OP_NONE;
            }
        }
    } else if (inst->kind == CN_IR_INST_NEG || inst->kind == CN_IR_INST_NOT) {
        if (is_constant(inst->src1)) {
            long long v1 = inst->src1.as.imm_int;
            long long result = (inst->kind == CN_IR_INST_NEG) ? -v1 : !v1;
            inst->kind = CN_IR_INST_MOV;
            inst->src1.as.imm_int = result;
        }
    }
}

void cn_ir_pass_constant_folding(CnIrModule *module) {
    if (!module) return;
    for (CnIrFunction *func = module->first_func; func; func = func->next) {
        for (CnIrBasicBlock *block = func->first_block; block; block = block->next) {
            for (CnIrInst *inst = block->first_inst; inst; inst = inst->next) {
                fold_inst(inst);
            }
        }
    }
}
