/**
 * @file copy_propagation.c
 * @brief 复写传播（Copy Propagation）Pass实现
 * 
 * 算法原理：
 * 跟踪MOV指令定义的值等价关系，将后续对目标寄存器的引用替换为源操作数，
 * 消除不必要的复写指令。
 * 
 * 示例：
 * 优化前：t1 = a; t2 = t1 + b;  // 使用 t1
 * 优化后：t2 = a + b;           // 直接使用 a
 * 
 * 实现要点：
 * 1. 只传播寄存器到寄存器的MOV（不传播立即数，那是常量折叠的工作）
 * 2. 当寄存器被重新定义时，需要使相关映射失效
 * 3. CALL指令可能修改内存，需要保守处理
 * 4. STORE指令后，需要使相关映射失效
 * 5. 每个基本块独立处理
 * 
 * 与CSE的配合：
 * 执行顺序：CSE → 复写传播 → 死代码消除
 * 原因：CSE会产生新的MOV指令，复写传播可以消除这些MOV
 */

#include "cnlang/ir/pass.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* ========== 复写映射表数据结构 ========== */

#define COPY_MAP_SIZE 1024  // 映射表大小（支持最多1024个寄存器）

/**
 * @brief 复写映射表
 * 
 * 记录 reg_id -> source_operand 的映射关系
 * 当遇到 MOV dest, src 时，记录 dest 可以用 src 替代
 */
typedef struct CnIrCopyMap {
    CnIrOperand mappings[COPY_MAP_SIZE];  // 映射表：reg_id -> source operand
    bool valid[COPY_MAP_SIZE];             // 标记哪些映射有效
} CnIrCopyMap;

/* ========== 辅助函数 ========== */

/**
 * @brief 创建新的复写映射表
 * 
 * @return CnIrCopyMap* 新映射表指针
 */
static CnIrCopyMap *copy_map_new(void) {
    CnIrCopyMap *map = (CnIrCopyMap *)calloc(1, sizeof(CnIrCopyMap));
    return map;
}

/**
 * @brief 释放复写映射表
 * 
 * @param map 映射表指针
 */
static void copy_map_free(CnIrCopyMap *map) {
    free(map);
}

/**
 * @brief 清空复写映射表
 * 
 * 在每个基本块开始时调用，或在CALL/STORE后调用
 * 
 * @param map 映射表指针
 */
static void copy_map_clear(CnIrCopyMap *map) {
    if (!map) return;
    memset(map->valid, 0, sizeof(map->valid));
}

/**
 * @brief 设置复写映射
 * 
 * 记录 reg_id 可以用 operand 替代
 * 
 * @param map 映射表
 * @param reg_id 目标寄存器ID
 * @param operand 源操作数
 */
static void copy_map_set(CnIrCopyMap *map, int reg_id, CnIrOperand operand) {
    if (!map || reg_id < 0 || reg_id >= COPY_MAP_SIZE) return;
    
    map->mappings[reg_id] = operand;
    map->valid[reg_id] = true;
}

/**
 * @brief 获取复写映射
 * 
 * 查找 reg_id 对应的源操作数
 * 
 * @param map 映射表
 * @param reg_id 寄存器ID
 * @param out_operand 输出操作数（如果找到）
 * @return true 找到映射
 * @return false 未找到映射
 */
static bool copy_map_get(CnIrCopyMap *map, int reg_id, CnIrOperand *out_operand) {
    if (!map || reg_id < 0 || reg_id >= COPY_MAP_SIZE || !out_operand) {
        return false;
    }
    
    if (map->valid[reg_id]) {
        *out_operand = map->mappings[reg_id];
        return true;
    }
    
    return false;
}

/**
 * @brief 使复写映射失效
 * 
 * 当寄存器被重新定义时，需要使其映射失效
 * 同时需要使所有映射到该寄存器的映射失效
 * 
 * @param map 映射表
 * @param reg_id 被重新定义的寄存器ID
 */
static void copy_map_invalidate(CnIrCopyMap *map, int reg_id) {
    if (!map || reg_id < 0 || reg_id >= COPY_MAP_SIZE) return;
    
    // 1. 使该寄存器的映射失效
    map->valid[reg_id] = false;
    
    // 2. 使所有映射到该寄存器的映射失效
    // 例如：如果有 %1 -> %0，然后 %0 被重新定义，则 %1 的映射也应失效
    for (int i = 0; i < COPY_MAP_SIZE; i++) {
        if (map->valid[i] && 
            map->mappings[i].kind == CN_IR_OP_REG &&
            map->mappings[i].as.reg_id == reg_id) {
            map->valid[i] = false;
        }
    }
}

/**
 * @brief 传播复写到操作数
 * 
 * 如果操作数是寄存器且有对应的复写映射，则替换为源操作数
 * 
 * @param map 映射表
 * @param op 操作数指针（可能被修改）
 */
static void propagate_copy(CnIrCopyMap *map, CnIrOperand *op) {
    if (!map || !op || op->kind != CN_IR_OP_REG) return;
    
    int reg_id = op->as.reg_id;
    CnIrOperand source;
    
    // 查找复写映射
    if (copy_map_get(map, reg_id, &source)) {
        // 替换为源操作数
        *op = source;
        
        // 如果源操作数也是寄存器，递归查找（处理传递性复写）
        // 例如：%1 -> %0, %0 -> %3，则 %1 最终应该替换为 %3
        if (source.kind == CN_IR_OP_REG) {
            CnIrOperand transitive_source;
            if (copy_map_get(map, source.as.reg_id, &transitive_source)) {
                *op = transitive_source;
            }
        }
    }
}

/**
 * @brief 判断指令是否会修改内存
 * 
 * CALL和STORE指令可能修改内存状态，需要清空复写映射
 * 
 * @param inst IR指令
 * @return true 会修改内存
 * @return false 不会修改内存
 */
static bool may_modify_memory(CnIrInst *inst) {
    return inst->kind == CN_IR_INST_CALL || inst->kind == CN_IR_INST_STORE;
}

/**
 * @brief 判断指令是否定义了目标寄存器
 * 
 * 大多数指令都会定义目标寄存器，但有些不会（如STORE、JUMP等）
 * 
 * @param inst IR指令
 * @return true 定义了目标寄存器
 * @return false 没有定义目标寄存器
 */
static bool defines_dest_reg(CnIrInst *inst) {
    switch (inst->kind) {
        // 这些指令定义目标寄存器
        case CN_IR_INST_ADD:
        case CN_IR_INST_SUB:
        case CN_IR_INST_MUL:
        case CN_IR_INST_DIV:
        case CN_IR_INST_MOD:
        case CN_IR_INST_AND:
        case CN_IR_INST_OR:
        case CN_IR_INST_XOR:
        case CN_IR_INST_SHL:
        case CN_IR_INST_SHR:
        case CN_IR_INST_NEG:
        case CN_IR_INST_NOT:
        case CN_IR_INST_EQ:
        case CN_IR_INST_NE:
        case CN_IR_INST_LT:
        case CN_IR_INST_LE:
        case CN_IR_INST_GT:
        case CN_IR_INST_GE:
        case CN_IR_INST_ALLOCA:
        case CN_IR_INST_LOAD:
        case CN_IR_INST_MOV:
        case CN_IR_INST_ADDRESS_OF:
        case CN_IR_INST_DEREF:
        case CN_IR_INST_CALL:
        case CN_IR_INST_GET_ELEMENT_PTR:
        case CN_IR_INST_MEMBER_ACCESS:
        case CN_IR_INST_PHI:
        case CN_IR_INST_SELECT:
            return inst->dest.kind == CN_IR_OP_REG;
        
        // 这些指令不定义目标寄存器
        case CN_IR_INST_STORE:
        case CN_IR_INST_LABEL:
        case CN_IR_INST_JUMP:
        case CN_IR_INST_BRANCH:
        case CN_IR_INST_RET:
        default:
            return false;
    }
}

/* ========== 复写传播核心逻辑 ========== */

/**
 * @brief 对单个基本块执行复写传播
 * 
 * @param block 基本块
 * @param map 复写映射表
 * @return int 传播的复写数量
 */
static int copy_propagation_process_block(CnIrBasicBlock *block, CnIrCopyMap *map) {
    if (!block || !map) return 0;
    
    int propagated = 0;
    
    // 遍历基本块中的每条指令
    for (CnIrInst *inst = block->first_inst; inst; inst = inst->next) {
        
        // 检查是否需要清空映射（CALL/STORE可能修改内存）
        if (may_modify_memory(inst)) {
            copy_map_clear(map);
        }
        
        // 处理MOV指令：记录复写关系
        if (inst->kind == CN_IR_INST_MOV) {
            // 只处理寄存器到寄存器的MOV
            // 不处理立即数（那是常量折叠的工作）
            if (inst->dest.kind == CN_IR_OP_REG && 
                inst->src1.kind == CN_IR_OP_REG) {
                
                // 记录复写关系：dest 可以用 src1 替代
                copy_map_set(map, inst->dest.as.reg_id, inst->src1);
            }
            continue;
        }
        
        // 对于非MOV指令：传播复写
        // 处理 src1 操作数
        if (inst->src1.kind == CN_IR_OP_REG) {
            CnIrOperand original = inst->src1;
            propagate_copy(map, &inst->src1);
            if (inst->src1.as.reg_id != original.as.reg_id) {
                propagated++;
            }
        }
        
        // 处理 src2 操作数
        if (inst->src2.kind == CN_IR_OP_REG) {
            CnIrOperand original = inst->src2;
            propagate_copy(map, &inst->src2);
            if (inst->src2.as.reg_id != original.as.reg_id) {
                propagated++;
            }
        }
        
        // 处理额外参数（如CALL指令的参数）
        for (size_t i = 0; i < inst->extra_args_count; i++) {
            if (inst->extra_args[i].kind == CN_IR_OP_REG) {
                CnIrOperand original = inst->extra_args[i];
                propagate_copy(map, &inst->extra_args[i]);
                if (inst->extra_args[i].as.reg_id != original.as.reg_id) {
                    propagated++;
                }
            }
        }
        
        // 如果指令定义了目标寄存器，使该寄存器的映射失效
        if (defines_dest_reg(inst)) {
            copy_map_invalidate(map, inst->dest.as.reg_id);
        }
    }
    
    return propagated;
}

/* ========== 公共接口 ========== */

/**
 * @brief 复写传播Pass主函数
 * 
 * 遍历模块中的所有函数和基本块，传播复写关系
 * 
 * @param module IR模块
 */
void cn_ir_pass_copy_propagation(CnIrModule *module) {
    if (!module) return;
    
    // 创建复写映射表
    CnIrCopyMap *map = copy_map_new();
    if (!map) return;
    
    // 遍历所有函数
    for (CnIrFunction *func = module->first_func; func; func = func->next) {
        // 遍历函数中的所有基本块
        for (CnIrBasicBlock *block = func->first_block; block; block = block->next) {
            // 每个基本块开始时清空映射表（不跨基本块传播）
            copy_map_clear(map);
            
            // 对基本块执行复写传播
            copy_propagation_process_block(block, map);
        }
    }
    
    // 释放映射表
    copy_map_free(map);
}
