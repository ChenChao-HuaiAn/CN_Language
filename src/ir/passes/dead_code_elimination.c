#include "cnlang/ir/pass.h"
#include <stdlib.h>
#include <stdbool.h>

static void mark_reachable(CnIrBasicBlock *block, bool *reachable, CnIrBasicBlock **blocks, int count) {
    int idx = -1;
    for (int i = 0; i < count; i++) {
        if (blocks[i] == block) {
            idx = i;
            break;
        }
    }
    if (idx == -1 || reachable[idx]) return;
    reachable[idx] = true;

    for (CnIrBasicBlockList *succ = block->succs; succ; succ = succ->next) {
        mark_reachable(succ->block, reachable, blocks, count);
    }
}

void cn_ir_pass_dead_code_elimination(CnIrModule *module) {
    if (!module) return;

    for (CnIrFunction *func = module->first_func; func; func = func->next) {
        if (!func->first_block) continue;

        // 1. 收集所有基本块
        int block_count = 0;
        for (CnIrBasicBlock *b = func->first_block; b; b = b->next) block_count++;

        CnIrBasicBlock **blocks = malloc(sizeof(CnIrBasicBlock *) * block_count);
        bool *reachable = calloc(block_count, sizeof(bool));
        
        int i = 0;
        for (CnIrBasicBlock *b = func->first_block; b; b = b->next) blocks[i++] = b;

        // 2. 从入口块开始标记可达性
        mark_reachable(func->first_block, reachable, blocks, block_count);

        // 3. 移除不可达块
        for (int j = 0; j < block_count; j++) {
            if (!reachable[j]) {
                CnIrBasicBlock *b = blocks[j];
                // 从函数链表中移除
                if (b->prev) b->prev->next = b->next;
                else func->first_block = b->next;

                if (b->next) b->next->prev = b->prev;
                else func->last_block = b->prev;

                // 注意：这里需要清理指向该块的引用（preds/succs），简便起见暂时只做链表移除
                // 真正的实现需要更精细的 CFG 维护
            }
        }

        free(blocks);
        free(reachable);
    }
}

void cn_ir_run_default_passes(CnIrModule *module) {
    cn_ir_pass_constant_folding(module);
    cn_ir_pass_dead_code_elimination(module);
}
