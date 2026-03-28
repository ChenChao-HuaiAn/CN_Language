/**
 * @file cse.c
 * @brief 公共子表达式消除（Common Subexpression Elimination）Pass实现
 * 
 * 算法原理：
 * 识别程序中相同的表达式计算，将其结果缓存复用，避免重复计算。
 * 
 * 示例：
 * 优化前：a = b + c; d = b + c;  // 重复计算 b + c
 * 优化后：t = b + c; a = t; d = t;
 * 
 * 实现要点：
 * 1. 只在同一个基本块内进行CSE（跨基本块需要更复杂的数据流分析）
 * 2. 调用指令可能修改内存，需要在调用后清空哈希表
 * 3. STORE指令可能修改内存，需要保守处理
 */

#include "cnlang/ir/pass.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* ========== 表达式哈希表数据结构 ========== */

#define EXPR_HASH_SIZE 256  // 哈希表桶数量

/**
 * @brief 表达式规范化表示（用于哈希键）
 * 
 * 对于可交换操作（ADD, MUL, AND, OR, XOR），保证 src1_id <= src2_id
 * 这样 add a, b 和 add b, a 会产生相同的键
 */
typedef struct CnIrExprKey {
    CnIrInstKind kind;  // 操作类型
    int src1_id;        // 操作数1的寄存器ID（规范化后）
    int src2_id;        // 操作数2的寄存器ID（规范化后）
} CnIrExprKey;

/**
 * @brief 表达式哈希表项
 */
typedef struct CnIrExprEntry {
    CnIrExprKey key;            // 表达式键
    int result_reg;             // 计算结果存放的寄存器ID
    CnIrInst *inst;             // 对应的指令指针
    struct CnIrExprEntry *next; // 链表下一个节点（处理哈希冲突）
} CnIrExprEntry;

/**
 * @brief 表达式哈希表
 */
typedef struct CnIrExprTable {
    CnIrExprEntry *buckets[EXPR_HASH_SIZE];  // 哈希桶数组
} CnIrExprTable;

/* ========== 辅助函数 ========== */

/**
 * @brief 判断指令是否可进行CSE
 * 
 * 支持的指令类型：
 * - 算术运算：ADD, SUB, MUL, DIV, MOD
 * - 位运算：AND, OR, XOR, SHL, SHR
 * - 比较运算：EQ, NE, LT, LE, GT, GE
 * 
 * @param inst IR指令指针
 * @return true 可以进行CSE
 * @return false 不可以进行CSE
 */
static bool is_cse_candidate(CnIrInst *inst) {
    switch (inst->kind) {
        // 算术运算
        case CN_IR_INST_ADD:
        case CN_IR_INST_SUB:
        case CN_IR_INST_MUL:
        case CN_IR_INST_DIV:
        case CN_IR_INST_MOD:
        // 位运算
        case CN_IR_INST_AND:
        case CN_IR_INST_OR:
        case CN_IR_INST_XOR:
        case CN_IR_INST_SHL:
        case CN_IR_INST_SHR:
        // 比较运算
        case CN_IR_INST_EQ:
        case CN_IR_INST_NE:
        case CN_IR_INST_LT:
        case CN_IR_INST_LE:
        case CN_IR_INST_GT:
        case CN_IR_INST_GE:
            // 两个源操作数都必须是寄存器
            return inst->src1.kind == CN_IR_OP_REG && 
                   inst->src2.kind == CN_IR_OP_REG;
        default:
            return false;
    }
}

/**
 * @brief 判断操作是否满足交换律
 * 
 * 可交换操作：ADD, MUL, AND, OR, XOR
 * 
 * @param kind 指令类型
 * @return true 满足交换律
 * @return false 不满足交换律
 */
static bool is_commutative(CnIrInstKind kind) {
    switch (kind) {
        case CN_IR_INST_ADD:
        case CN_IR_INST_MUL:
        case CN_IR_INST_AND:
        case CN_IR_INST_OR:
        case CN_IR_INST_XOR:
            return true;
        default:
            return false;
    }
}

/**
 * @brief 计算表达式键的哈希值
 * 
 * @param key 表达式键
 * @return unsigned int 哈希值
 */
static unsigned int expr_key_hash(CnIrExprKey *key) {
    // 简单的哈希函数：将各字段组合
    unsigned int hash = (unsigned int)key->kind;
    hash = hash * 31 + (unsigned int)key->src1_id;
    hash = hash * 31 + (unsigned int)key->src2_id;
    return hash % EXPR_HASH_SIZE;
}

/**
 * @brief 比较两个表达式键是否相等
 * 
 * @param a 键A
 * @param b 键B
 * @return true 相等
 * @return false 不相等
 */
static bool expr_key_equal(CnIrExprKey *a, CnIrExprKey *b) {
    return a->kind == b->kind && 
           a->src1_id == b->src1_id && 
           a->src2_id == b->src2_id;
}

/**
 * @brief 规范化表达式，生成哈希键
 * 
 * 对于可交换运算，保证操作数顺序一致（较小的ID在前）
 * 
 * @param inst IR指令
 * @return CnIrExprKey 规范化的表达式键
 */
static CnIrExprKey normalize_expr(CnIrInst *inst) {
    CnIrExprKey key;
    key.kind = inst->kind;
    
    int id1 = inst->src1.as.reg_id;
    int id2 = inst->src2.as.reg_id;
    
    // 对于交换律运算，保证顺序一致
    if (is_commutative(inst->kind) && id1 > id2) {
        key.src1_id = id2;
        key.src2_id = id1;
    } else {
        key.src1_id = id1;
        key.src2_id = id2;
    }
    
    return key;
}

/* ========== 哈希表操作函数 ========== */

/**
 * @brief 创建新的表达式哈希表
 * 
 * @return CnIrExprTable* 新哈希表指针
 */
static CnIrExprTable *expr_table_new(void) {
    CnIrExprTable *table = (CnIrExprTable *)calloc(1, sizeof(CnIrExprTable));
    return table;
}

/**
 * @brief 释放表达式哈希表
 * 
 * @param table 哈希表指针
 */
static void expr_table_free(CnIrExprTable *table) {
    if (!table) return;
    
    // 释放所有桶中的链表节点
    for (int i = 0; i < EXPR_HASH_SIZE; i++) {
        CnIrExprEntry *entry = table->buckets[i];
        while (entry) {
            CnIrExprEntry *next = entry->next;
            free(entry);
            entry = next;
        }
    }
    
    free(table);
}

/**
 * @brief 在哈希表中查找表达式
 * 
 * @param table 哈希表
 * @param key 表达式键
 * @return CnIrExprEntry* 找到的表项，未找到返回NULL
 */
static CnIrExprEntry *expr_table_lookup(CnIrExprTable *table, CnIrExprKey *key) {
    if (!table || !key) return NULL;
    
    unsigned int idx = expr_key_hash(key);
    CnIrExprEntry *entry = table->buckets[idx];
    
    // 遍历链表查找匹配项
    while (entry) {
        if (expr_key_equal(&entry->key, key)) {
            return entry;
        }
        entry = entry->next;
    }
    
    return NULL;
}

/**
 * @brief 向哈希表插入表达式
 * 
 * @param table 哈希表
 * @param key 表达式键
 * @param result_reg 结果寄存器ID
 * @param inst 对应指令
 */
static void expr_table_insert(CnIrExprTable *table, CnIrExprKey *key, 
                               int result_reg, CnIrInst *inst) {
    if (!table || !key) return;
    
    unsigned int idx = expr_key_hash(key);
    
    // 创建新表项
    CnIrExprEntry *entry = (CnIrExprEntry *)malloc(sizeof(CnIrExprEntry));
    entry->key = *key;
    entry->result_reg = result_reg;
    entry->inst = inst;
    
    // 插入到链表头部
    entry->next = table->buckets[idx];
    table->buckets[idx] = entry;
}

/**
 * @brief 清空哈希表（用于处理CALL/STORE后）
 * 
 * @param table 哈希表
 */
static void expr_table_clear(CnIrExprTable *table) {
    if (!table) return;
    
    // 释放所有桶中的链表节点
    for (int i = 0; i < EXPR_HASH_SIZE; i++) {
        CnIrExprEntry *entry = table->buckets[i];
        while (entry) {
            CnIrExprEntry *next = entry->next;
            free(entry);
            entry = next;
        }
        table->buckets[i] = NULL;
    }
}

/* ========== CSE核心逻辑 ========== */

/**
 * @brief 将指令替换为MOV指令
 * 
 * 当发现公共子表达式时，将原指令替换为从已计算结果复制的MOV指令
 * 
 * @param inst 原指令
 * @param result_reg 已计算结果的寄存器ID
 */
static void replace_with_copy(CnIrInst *inst, int result_reg) {
    // 保存目标操作数
    CnIrOperand dest = inst->dest;
    
    // 将指令转换为MOV
    inst->kind = CN_IR_INST_MOV;
    
    // 设置源操作数为已计算结果的寄存器
    inst->src1.kind = CN_IR_OP_REG;
    inst->src1.as.reg_id = result_reg;
    inst->src1.type = inst->dest.type;  // 保持类型一致
    
    // 清空第二个源操作数
    inst->src2.kind = CN_IR_OP_NONE;
    inst->src2.type = NULL;
    
    // 目标操作数保持不变
    inst->dest = dest;
}

/**
 * @brief 判断指令是否会修改内存
 * 
 * CALL和STORE指令可能修改内存状态，需要清空表达式缓存
 * 
 * @param inst IR指令
 * @return true 会修改内存
 * @return false 不会修改内存
 */
static bool may_modify_memory(CnIrInst *inst) {
    return inst->kind == CN_IR_INST_CALL || inst->kind == CN_IR_INST_STORE;
}

/**
 * @brief 对单个基本块执行CSE
 * 
 * @param block 基本块
 * @param table 表达式哈希表
 * @return int 消除的公共子表达式数量
 */
static int cse_process_block(CnIrBasicBlock *block, CnIrExprTable *table) {
    if (!block || !table) return 0;
    
    int eliminated = 0;
    
    // 遍历基本块中的每条指令
    for (CnIrInst *inst = block->first_inst; inst; inst = inst->next) {
        // 检查是否需要清空缓存（CALL/STORE可能修改内存）
        if (may_modify_memory(inst)) {
            expr_table_clear(table);
            continue;
        }
        
        // 检查是否是CSE候选指令
        if (!is_cse_candidate(inst)) {
            continue;
        }
        
        // 规范化表达式并生成键
        CnIrExprKey key = normalize_expr(inst);
        
        // 在哈希表中查找
        CnIrExprEntry *entry = expr_table_lookup(table, &key);
        
        if (entry) {
            // 找到公共子表达式，替换为复写
            replace_with_copy(inst, entry->result_reg);
            eliminated++;
        } else {
            // 新表达式，插入哈希表
            if (inst->dest.kind == CN_IR_OP_REG) {
                expr_table_insert(table, &key, inst->dest.as.reg_id, inst);
            }
        }
    }
    
    return eliminated;
}

/* ========== 公共接口 ========== */

/**
 * @brief 公共子表达式消除Pass主函数
 * 
 * 遍历模块中的所有函数和基本块，消除重复的表达式计算
 * 
 * @param module IR模块
 */
void cn_ir_pass_cse(CnIrModule *module) {
    if (!module) return;
    
    // 遍历所有函数
    for (CnIrFunction *func = module->first_func; func; func = func->next) {
        // 为每个函数创建表达式哈希表
        // 注意：每个基本块独立处理，所以可以在函数级别复用哈希表
        CnIrExprTable *table = expr_table_new();
        if (!table) continue;
        
        // 遍历函数中的所有基本块
        for (CnIrBasicBlock *block = func->first_block; block; block = block->next) {
            // 每个基本块开始时清空哈希表（局部CSE，不跨基本块）
            expr_table_clear(table);
            
            // 对基本块执行CSE
            cse_process_block(block, table);
        }
        
        // 释放哈希表
        expr_table_free(table);
    }
}
