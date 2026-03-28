/**
 * @file strength_reduction.c
 * @brief 强度削弱优化Pass
 * 
 * 用低代价操作替代高代价操作：
 * - 乘法转移位：x * 2^n → x << n
 * - 除法转移位：x / 2^n → x >> n（仅无符号数或正数）
 * - 取模转位与：x % 2^n → x & (2^n - 1)
 * - 特殊值优化：x * 0 → 0, x * 1 → x, x / 1 → x, x % 1 → 0
 */

#include "cnlang/ir/pass.h"
#include <stdbool.h>

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 检查值是否是2的幂次
 * @param value 要检查的值
 * @param power 输出参数，存储幂次（如 value=8 则 power=3）
 * @return true 如果是2的幂次
 */
static bool is_power_of_two(long long value, int *power) {
    if (value <= 0) return false;
    
    // 检查是否只有一个位为1
    if ((value & (value - 1)) != 0) return false;
    
    // 计算幂次
    *power = 0;
    while ((1LL << *power) < value) {
        (*power)++;
    }
    return true;
}

/**
 * @brief 检查类型是否是无符号整数类型
 * @param type 类型指针
 * @return true 如果是无符号类型
 */
static bool is_unsigned_type(CnType *type) {
    if (!type) return false;
    
    switch (type->kind) {
        case CN_TYPE_UINT32:
        case CN_TYPE_UINT64:
        case CN_TYPE_UINT64_LL:
            return true;
        default:
            return false;
    }
}

/**
 * @brief 检查操作数是否是整数立即数
 * @param op 操作数
 * @return true 如果是整数立即数
 */
static bool is_imm_int(CnIrOperand op) {
    return op.kind == CN_IR_OP_IMM_INT;
}

/**
 * @brief 创建整数立即数操作数
 * @param value 整数值
 * @return 操作数结构
 */
static CnIrOperand make_imm_int(long long value) {
    CnIrOperand op;
    op.kind = CN_IR_OP_IMM_INT;
    op.as.imm_int = value;
    op.type = NULL;
    return op;
}

/**
 * @brief 创建空操作数
 * @return 空操作数结构
 */
static CnIrOperand make_none_operand(void) {
    CnIrOperand op;
    op.kind = CN_IR_OP_NONE;
    op.type = NULL;
    return op;
}

// ============================================================================
// 优化转换函数
// ============================================================================

/**
 * @brief 尝试将乘法转换为移位操作
 * 
 * 优化模式：
 * - x * 2^n → x << n
 * - x * 0 → 0（特殊值优化）
 * - x * 1 → x（特殊值优化）
 * 
 * @param inst 指令指针
 * @return true 如果进行了优化
 */
static bool try_optimize_mul(CnIrInst *inst) {
    // 检查src2是否是立即数
    if (!is_imm_int(inst->src2)) return false;
    
    long long multiplier = inst->src2.as.imm_int;
    
    // 特殊值优化：x * 0 → 0
    if (multiplier == 0) {
        inst->kind = CN_IR_INST_MOV;
        inst->src1 = make_imm_int(0);
        inst->src2 = make_none_operand();
        return true;
    }
    
    // 特殊值优化：x * 1 → x
    if (multiplier == 1) {
        inst->kind = CN_IR_INST_MOV;
        // src1保持不变（被乘数），src2清空
        inst->src2 = make_none_operand();
        return true;
    }
    
    // 乘法转移位：x * 2^n → x << n
    int power;
    if (is_power_of_two(multiplier, &power)) {
        inst->kind = CN_IR_INST_SHL;
        inst->src2 = make_imm_int(power);
        return true;
    }
    
    return false;
}

/**
 * @brief 尝试将除法转换为移位操作
 * 
 * 优化模式：
 * - x / 2^n → x >> n（仅对无符号数或已知为正数安全）
 * - x / 1 → x（特殊值优化）
 * 
 * 注意：对于有符号数，右移是算术右移，与除法行为不同（负数除法向零截断，右移向下取整）
 * 
 * @param inst 指令指针
 * @return true 如果进行了优化
 */
static bool try_optimize_div(CnIrInst *inst) {
    // 检查src2是否是立即数
    if (!is_imm_int(inst->src2)) return false;
    
    long long divisor = inst->src2.as.imm_int;
    
    // 除以0不进行优化（语义错误，应由常量折叠或其他Pass处理）
    if (divisor == 0) return false;
    
    // 特殊值优化：x / 1 → x
    if (divisor == 1) {
        inst->kind = CN_IR_INST_MOV;
        // src1保持不变（被除数），src2清空
        inst->src2 = make_none_operand();
        return true;
    }
    
    // 除法转移位：x / 2^n → x >> n
    // 仅对无符号类型安全
    int power;
    if (is_power_of_two(divisor, &power)) {
        // 检查是否是无符号类型
        bool is_unsigned = is_unsigned_type(inst->src1.type);
        
        if (is_unsigned) {
            inst->kind = CN_IR_INST_SHR;  // 逻辑右移
            inst->src2 = make_imm_int(power);
            return true;
        }
        
        // 对于有符号数，暂不优化（需要更复杂的处理）
        // 未来可以添加：对于已知为正数的情况也可以优化
    }
    
    return false;
}

/**
 * @brief 尝试将取模转换为位与操作
 * 
 * 优化模式：
 * - x % 2^n → x & (2^n - 1)
 * - x % 1 → 0（特殊值优化）
 * 
 * 注意：对于负数，取模和位与的结果不同，需要谨慎处理
 * 
 * @param inst 指令指针
 * @return true 如果进行了优化
 */
static bool try_optimize_mod(CnIrInst *inst) {
    // 检查src2是否是立即数
    if (!is_imm_int(inst->src2)) return false;
    
    long long modulus = inst->src2.as.imm_int;
    
    // 模0不进行优化（语义错误）
    if (modulus == 0) return false;
    
    // 特殊值优化：x % 1 → 0
    if (modulus == 1) {
        inst->kind = CN_IR_INST_MOV;
        inst->src1 = make_imm_int(0);
        inst->src2 = make_none_operand();
        return true;
    }
    
    // 取模转位与：x % 2^n → x & (2^n - 1)
    int power;
    if (is_power_of_two(modulus, &power)) {
        // 检查是否是无符号类型
        bool is_unsigned = is_unsigned_type(inst->src1.type);
        
        if (is_unsigned) {
            inst->kind = CN_IR_INST_AND;
            inst->src2 = make_imm_int(modulus - 1);  // 2^n - 1
            return true;
        }
        
        // 对于有符号数，暂不优化（需要更复杂的处理）
        // 未来可以添加：对于已知为正数的情况也可以优化
    }
    
    return false;
}

/**
 * @brief 对单条指令进行强度削弱优化
 * @param inst 指令指针
 * @return true 如果进行了优化
 */
static bool optimize_inst(CnIrInst *inst) {
    switch (inst->kind) {
        case CN_IR_INST_MUL:
            return try_optimize_mul(inst);
        
        case CN_IR_INST_DIV:
            return try_optimize_div(inst);
        
        case CN_IR_INST_MOD:
            return try_optimize_mod(inst);
        
        default:
            return false;
    }
}

// ============================================================================
// Pass入口函数
// ============================================================================

/**
 * @brief 强度削弱优化Pass
 * 
 * 遍历所有函数的所有基本块的所有指令，尝试进行强度削弱优化。
 * 优化后可能产生死代码，需要后续死代码消除Pass清理。
 * 
 * @param module IR模块指针
 */
void cn_ir_pass_strength_reduction(CnIrModule *module) {
    if (!module) return;
    
    int optimized_count = 0;  // 统计优化次数
    
    // 遍历所有函数
    for (CnIrFunction *func = module->first_func; func; func = func->next) {
        // 遍历所有基本块
        for (CnIrBasicBlock *block = func->first_block; block; block = block->next) {
            // 遍历所有指令
            for (CnIrInst *inst = block->first_inst; inst; inst = inst->next) {
                if (optimize_inst(inst)) {
                    optimized_count++;
                }
            }
        }
    }
    
    // 可选：输出优化统计信息（调试用）
    // if (optimized_count > 0) {
    //     fprintf(stderr, "[强度削弱] 优化了 %d 条指令\n", optimized_count);
    // }
}
