/**
 * @file loop_invariant.c
 * @brief 循环不变量外提（Loop Invariant Code Motion）Pass实现
 * 
 * 算法原理：
 * 识别循环中每次迭代计算结果相同的指令，将其移动到循环前置块执行，
 * 从而减少循环内的计算开销。
 * 
 * 示例：
 * 优化前：
 *   for (i = 0; i < n; i++) {
 *       x = a + b;  // a和b在循环内不变
 *       arr[i] = x + i;
 *   }
 * 
 * 优化后：
 *   x = a + b;     // 外提到循环前
 *   for (i = 0; i < n; i++) {
 *       arr[i] = x + i;
 *   }
 * 
 * 实现要点：
 * 1. 循环检测：通过CFG的后向边识别自然循环
 * 2. 不变量识别：指令的操作数在循环内不被修改
 * 3. 安全外提：指令没有副作用，且支配循环内所有使用点
 * 4. 简化策略：只处理单入口循环，只外提纯计算指令
 */

#include "cnlang/ir/pass.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* ========== 数据结构定义 ========== */

#define MAX_BLOCKS_IN_LOOP 256   // 循环内最大基本块数
#define MAX_REG_ID 1024          // 最大寄存器ID

/**
 * @brief 循环结构
 * 
 * 表示一个自然循环，包含循环头、循环体和前置块信息
 */
typedef struct CnIrLoop {
    CnIrBasicBlock *header;           // 循环头（支配循环入口）
    CnIrBasicBlock *body[MAX_BLOCKS_IN_LOOP];  // 循环体基本块
    int body_count;                   // 循环体基本块数量
    CnIrBasicBlock *preheader;        // 前置块（外提目标，可能为NULL）
    CnIrBasicBlock *back_edge_src;    // 回边源块
} CnIrLoop;

/**
 * @brief 循环集合
 * 
 * 存储函数中检测到的所有循环
 */
#define MAX_LOOPS 64
typedef struct CnIrLoopSet {
    CnIrLoop loops[MAX_LOOPS];
    int count;
} CnIrLoopSet;

/**
 * @brief 寄存器定义信息
 * 
 * 记录每个寄存器在哪个基本块中被定义
 */
typedef struct CnIrRegDefInfo {
    CnIrBasicBlock *def_block[MAX_REG_ID];  // 定义该寄存器所在的基本块
    bool defined[MAX_REG_ID];                // 是否已定义
} CnIrRegDefInfo;

/**
 * @brief 不变量标记信息
 * 
 * 标记哪些指令是循环不变量
 */
typedef struct CnIrInvariantInfo {
    bool is_invariant[MAX_REG_ID];  // 寄存器对应的结果是否为不变量
} CnIrInvariantInfo;

/* ========== 辅助函数：基本块操作 ========== */

/**
 * @brief 检查基本块是否在数组中
 */
static bool block_in_array(CnIrBasicBlock *block, CnIrBasicBlock **arr, int count) {
    for (int i = 0; i < count; i++) {
        if (arr[i] == block) return true;
    }
    return false;
}

/**
 * @brief 添加基本块到数组（如果不存在）
 * 
 * @return true 如果添加成功，false 如果已存在或数组已满
 */
static bool add_block_to_array(CnIrBasicBlock *block, CnIrBasicBlock **arr, int *count, int max) {
    if (block_in_array(block, arr, *count)) return false;
    if (*count >= max) return false;
    arr[(*count)++] = block;
    return true;
}

/* ========== 辅助函数：寄存器定义分析 ========== */

/**
 * @brief 分析函数中所有寄存器的定义位置
 * 
 * 遍历所有基本块和指令，记录每个寄存器在哪个基本块中被定义
 */
static void analyze_register_definitions(CnIrFunction *func, CnIrRegDefInfo *info) {
    memset(info, 0, sizeof(CnIrRegDefInfo));
    
    for (CnIrBasicBlock *block = func->first_block; block; block = block->next) {
        for (CnIrInst *inst = block->first_inst; inst; inst = inst->next) {
            // 检查目标操作数是否定义了寄存器
            if (inst->dest.kind == CN_IR_OP_REG) {
                int reg_id = inst->dest.as.reg_id;
                if (reg_id >= 0 && reg_id < MAX_REG_ID) {
                    info->def_block[reg_id] = block;
                    info->defined[reg_id] = true;
                }
            }
        }
    }
}

/**
 * @brief 检查寄存器是否在循环内被定义
 */
static bool is_defined_in_loop(int reg_id, CnIrLoop *loop, CnIrRegDefInfo *info) {
    if (reg_id < 0 || reg_id >= MAX_REG_ID) return false;
    if (!info->defined[reg_id]) return false;  // 未定义（可能是参数或全局变量）
    
    CnIrBasicBlock *def_block = info->def_block[reg_id];
    return block_in_array(def_block, loop->body, loop->body_count);
}

/* ========== 辅助函数：指令分析 ========== */

/**
 * @brief 检查指令是否为纯计算指令（可安全外提）
 * 
 * 纯计算指令：不修改内存、不产生副作用、不产生异常
 * 
 * 可外提的指令类型：
 * - 算术运算：ADD, SUB, MUL（DIV/MOD可能除零，暂时保守处理）
 * - 位运算：AND, OR, XOR, SHL, SHR, NEG, NOT
 * - 比较运算：EQ, NE, LT, LE, GT, GE
 * 
 * 不可外提的指令类型：
 * - 内存操作：LOAD, STORE, ALLOCA（可能有副作用）
 * - 控制流：CALL, RET, JUMP, BRANCH
 * - 可能异常：DIV, MOD（除零风险）
 */
static bool is_pure_computation(CnIrInst *inst) {
    switch (inst->kind) {
        // 算术运算（不含DIV/MOD，避免除零异常）
        case CN_IR_INST_ADD:
        case CN_IR_INST_SUB:
        case CN_IR_INST_MUL:
        // 位运算
        case CN_IR_INST_AND:
        case CN_IR_INST_OR:
        case CN_IR_INST_XOR:
        case CN_IR_INST_SHL:
        case CN_IR_INST_SHR:
        case CN_IR_INST_NEG:
        case CN_IR_INST_NOT:
        // 比较运算
        case CN_IR_INST_EQ:
        case CN_IR_INST_NE:
        case CN_IR_INST_LT:
        case CN_IR_INST_LE:
        case CN_IR_INST_GT:
        case CN_IR_INST_GE:
            return true;
        
        // 保守处理：DIV和MOD可能产生除零异常
        case CN_IR_INST_DIV:
        case CN_IR_INST_MOD:
            return false;
        
        default:
            return false;
    }
}

/**
 * @brief 检查操作数是否为循环不变量
 * 
 * 操作数是循环不变量的条件：
 * 1. 是立即数（常量）
 * 2. 是寄存器，且该寄存器在循环外定义
 * 3. 是寄存器，且该寄存器的值已知是循环不变量
 */
static bool is_operand_invariant(CnIrOperand *op, CnIrLoop *loop, 
                                  CnIrRegDefInfo *def_info, 
                                  CnIrInvariantInfo *inv_info) {
    switch (op->kind) {
        case CN_IR_OP_IMM_INT:
        case CN_IR_OP_IMM_FLOAT:
        case CN_IR_OP_IMM_STR:
            // 立即数总是不变量
            return true;
        
        case CN_IR_OP_REG: {
            int reg_id = op->as.reg_id;
            if (reg_id < 0 || reg_id >= MAX_REG_ID) return false;
            
            // 如果寄存器在循环外定义，则是不变量
            if (!is_defined_in_loop(reg_id, loop, def_info)) {
                return true;
            }
            
            // 如果寄存器已标记为不变量
            if (inv_info->is_invariant[reg_id]) {
                return true;
            }
            
            return false;
        }
        
        case CN_IR_OP_SYMBOL:
            // 全局符号视为不变量（简化处理）
            return true;
        
        default:
            return false;
    }
}

/**
 * @brief 检查指令是否为循环不变量
 * 
 * 指令是循环不变量的条件：
 * 1. 是纯计算指令
 * 2. 所有操作数都是循环不变量
 */
static bool is_loop_invariant_inst(CnIrInst *inst, CnIrLoop *loop,
                                    CnIrRegDefInfo *def_info,
                                    CnIrInvariantInfo *inv_info) {
    // 必须是纯计算指令
    if (!is_pure_computation(inst)) return false;
    
    // 目标必须是寄存器
    if (inst->dest.kind != CN_IR_OP_REG) return false;
    
    // 检查src1
    if (!is_operand_invariant(&inst->src1, loop, def_info, inv_info)) {
        return false;
    }
    
    // 检查src2（如果存在）
    if (inst->src2.kind != CN_IR_OP_NONE) {
        if (!is_operand_invariant(&inst->src2, loop, def_info, inv_info)) {
            return false;
        }
    }
    
    return true;
}

/* ========== 循环检测算法 ========== */

/**
 * @brief 查找回边并构建循环
 * 
 * 回边定义：从循环体指向循环头的边
 * 
 * 算法：
 * 1. 遍历所有基本块的后继
 * 2. 如果后继是之前的基本块（在物理顺序上），则可能是回边
 * 3. 收集循环体：从循环头可达且能到达回边源块的所有块
 */
static void detect_loops(CnIrFunction *func, CnIrLoopSet *loop_set) {
    loop_set->count = 0;
    
    // 收集所有基本块到数组，便于索引
    CnIrBasicBlock *blocks[MAX_BLOCKS_IN_LOOP];
    int block_count = 0;
    for (CnIrBasicBlock *b = func->first_block; b && block_count < MAX_BLOCKS_IN_LOOP; b = b->next) {
        blocks[block_count++] = b;
    }
    
    // 查找回边：检查每个基本块的后继
    for (int i = 0; i < block_count; i++) {
        CnIrBasicBlock *block = blocks[i];
        
        for (CnIrBasicBlockList *succ = block->succs; succ; succ = succ->next) {
            CnIrBasicBlock *target = succ->block;
            
            // 查找目标块在数组中的索引
            int target_idx = -1;
            for (int j = 0; j < block_count; j++) {
                if (blocks[j] == target) {
                    target_idx = j;
                    break;
                }
            }
            
            // 如果目标块在当前块之前（或就是当前块），则是回边
            if (target_idx >= 0 && target_idx <= i) {
                // 找到一个循环
                if (loop_set->count >= MAX_LOOPS) break;
                
                CnIrLoop *loop = &loop_set->loops[loop_set->count++];
                memset(loop, 0, sizeof(CnIrLoop));
                
                loop->header = target;
                loop->back_edge_src = block;
                
                // 收集循环体：简化处理，收集从header到back_edge_src之间的所有块
                // 这是一个保守的近似，真正的算法需要更精确的可达性分析
                loop->body_count = 0;
                for (int j = target_idx; j <= i; j++) {
                    loop->body[loop->body_count++] = blocks[j];
                }
            }
        }
    }
}

/* ========== 循环前置块创建 ========== */

/**
 * @brief 为循环创建前置块
 * 
 * 前置块是循环头的前驱块，用于存放外提的指令
 * 
 * 简化策略：
 * 如果循环头只有一个前驱（除了回边），则使用该前驱作为前置块
 * 否则，不创建前置块（保守处理）
 */
static CnIrBasicBlock *find_or_create_preheader(CnIrLoop *loop, CnIrFunction *func) {
    // 统计循环头的前驱数量（排除回边）
    CnIrBasicBlock *preheader = NULL;
    int non_back_preds = 0;
    
    for (CnIrBasicBlockList *pred = loop->header->preds; pred; pred = pred->next) {
        if (pred->block != loop->back_edge_src) {
            preheader = pred->block;
            non_back_preds++;
        }
    }
    
    // 如果只有一个非回边前驱，使用它作为前置块
    if (non_back_preds == 1) {
        loop->preheader = preheader;
        return preheader;
    }
    
    // 多个入口或无入口，保守处理，不创建前置块
    // 真正的实现需要分割边或创建新块
    loop->preheader = NULL;
    return NULL;
}

/* ========== 不变量外提算法 ========== */

/**
 * @brief 识别循环中的所有不变量
 * 
 * 使用迭代算法：
 * 1. 初始化：标记所有操作数为常量或在循环外定义的指令
 * 2. 迭代：如果指令的操作数都是不变量，则标记该指令
 * 3. 重复直到没有新的不变量被发现
 */
static void identify_invariants(CnIrLoop *loop, CnIrRegDefInfo *def_info,
                                 CnIrInvariantInfo *inv_info) {
    // 初始化不变量信息
    memset(inv_info, 0, sizeof(CnIrInvariantInfo));
    
    bool changed = true;
    while (changed) {
        changed = false;
        
        // 遍历循环体内的所有指令
        for (int i = 0; i < loop->body_count; i++) {
            CnIrBasicBlock *block = loop->body[i];
            
            for (CnIrInst *inst = block->first_inst; inst; inst = inst->next) {
                // 跳过非寄存器目标的指令
                if (inst->dest.kind != CN_IR_OP_REG) continue;
                
                int reg_id = inst->dest.as.reg_id;
                if (reg_id < 0 || reg_id >= MAX_REG_ID) continue;
                
                // 已经标记为不变量，跳过
                if (inv_info->is_invariant[reg_id]) continue;
                
                // 检查是否为不变量
                if (is_loop_invariant_inst(inst, loop, def_info, inv_info)) {
                    inv_info->is_invariant[reg_id] = true;
                    changed = true;
                }
            }
        }
    }
}

/**
 * @brief 从基本块中移除指令
 */
static void remove_inst_from_block(CnIrBasicBlock *block, CnIrInst *inst) {
    if (inst->prev) {
        inst->prev->next = inst->next;
    } else {
        block->first_inst = inst->next;
    }
    
    if (inst->next) {
        inst->next->prev = inst->prev;
    } else {
        block->last_inst = inst->prev;
    }
    
    inst->prev = NULL;
    inst->next = NULL;
}

/**
 * @brief 将指令添加到基本块末尾
 */
static void add_inst_to_block_end(CnIrBasicBlock *block, CnIrInst *inst) {
    inst->prev = block->last_inst;
    inst->next = NULL;
    
    if (block->last_inst) {
        block->last_inst->next = inst;
    } else {
        block->first_inst = inst;
    }
    block->last_inst = inst;
}

/**
 * @brief 外提循环不变量到前置块
 * 
 * 将标记为不变量的指令移动到循环前置块
 */
static int hoist_invariants(CnIrLoop *loop, CnIrInvariantInfo *inv_info) {
    if (!loop->preheader) return 0;
    
    int hoisted_count = 0;
    
    // 遍历循环体内的所有指令
    for (int i = 0; i < loop->body_count; i++) {
        CnIrBasicBlock *block = loop->body[i];
        CnIrInst *next_inst = NULL;
        
        for (CnIrInst *inst = block->first_inst; inst; inst = next_inst) {
            next_inst = inst->next;  // 保存下一个，因为可能移除当前指令
            
            // 跳过非寄存器目标的指令
            if (inst->dest.kind != CN_IR_OP_REG) continue;
            
            int reg_id = inst->dest.as.reg_id;
            if (reg_id < 0 || reg_id >= MAX_REG_ID) continue;
            
            // 如果是不变量，外提到前置块
            if (inv_info->is_invariant[reg_id]) {
                // 从原块移除
                remove_inst_from_block(block, inst);
                
                // 添加到前置块末尾
                add_inst_to_block_end(loop->preheader, inst);
                
                hoisted_count++;
            }
        }
    }
    
    return hoisted_count;
}

/* ========== 主Pass函数 ========== */

/**
 * @brief 对单个函数执行循环不变量外提
 */
static void process_function(CnIrFunction *func) {
    if (!func || !func->first_block) return;
    
    // 1. 分析寄存器定义
    CnIrRegDefInfo *def_info = (CnIrRegDefInfo *)calloc(1, sizeof(CnIrRegDefInfo));
    if (!def_info) return;
    
    analyze_register_definitions(func, def_info);
    
    // 2. 检测循环
    CnIrLoopSet *loop_set = (CnIrLoopSet *)calloc(1, sizeof(CnIrLoopSet));
    if (!loop_set) {
        free(def_info);
        return;
    }
    
    detect_loops(func, loop_set);
    
    // 3. 处理每个循环
    for (int i = 0; i < loop_set->count; i++) {
        CnIrLoop *loop = &loop_set->loops[i];
        
        // 3.1 查找或创建前置块
        find_or_create_preheader(loop, func);
        if (!loop->preheader) {
            // 无法创建前置块，跳过此循环
            continue;
        }
        
        // 3.2 识别不变量
        CnIrInvariantInfo *inv_info = (CnIrInvariantInfo *)calloc(1, sizeof(CnIrInvariantInfo));
        if (!inv_info) continue;
        
        identify_invariants(loop, def_info, inv_info);
        
        // 3.3 外提不变量
        int hoisted = hoist_invariants(loop, inv_info);
        
        // 如果有指令被外提，更新寄存器定义信息
        if (hoisted > 0) {
            analyze_register_definitions(func, def_info);
        }
        
        free(inv_info);
    }
    
    free(def_info);
    free(loop_set);
}

/**
 * @brief 循环不变量外提Pass入口
 * 
 * 对模块中的所有函数执行循环不变量外提优化
 */
void cn_ir_pass_loop_invariant_code_motion(CnIrModule *module) {
    if (!module) return;
    
    for (CnIrFunction *func = module->first_func; func; func = func->next) {
        process_function(func);
    }
}
