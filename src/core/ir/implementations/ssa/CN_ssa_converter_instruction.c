/******************************************************************************
 * 文件名: CN_ssa_converter_instruction.c
 * 功能: CN_Language SSA转换器指令创建辅助函数
 * 
 * 实现SSA转换器的指令创建辅助函数。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_ssa_converter_internal.h"

// ============================================================================
// 指令创建辅助函数实现
// ============================================================================

/**
 * @brief 创建二元操作指令
 */
Stru_SsaInstruction_t* F_create_binary_op_instruction(Eum_SsaOpcode opcode,
                                                    Stru_SsaVariable_t* result,
                                                    Stru_SsaVariable_t* left,
                                                    Stru_SsaVariable_t* right,
                                                    size_t line, size_t column)
{
    if (result == NULL || left == NULL || right == NULL) {
        return NULL;
    }
    
    Stru_SsaInstruction_t* instr = F_create_ssa_instruction(
        opcode, result, line, column);
    if (instr == NULL) {
        return NULL;
    }
    
    // 设置操作数
    instr->operands[0] = left;
    instr->operands[1] = right;
    instr->operand_count = 2;
    
    return instr;
}

/**
 * @brief 创建一元操作指令
 */
Stru_SsaInstruction_t* F_create_unary_op_instruction(Eum_SsaOpcode opcode,
                                                   Stru_SsaVariable_t* result,
                                                   Stru_SsaVariable_t* operand,
                                                   size_t line, size_t column)
{
    if (result == NULL || operand == NULL) {
        return NULL;
    }
    
    Stru_SsaInstruction_t* instr = F_create_ssa_instruction(
        opcode, result, line, column);
    if (instr == NULL) {
        return NULL;
    }
    
    // 设置操作数
    instr->operands[0] = operand;
    instr->operand_count = 1;
    
    return instr;
}

/**
 * @brief 创建赋值指令
 */
Stru_SsaInstruction_t* F_create_assignment_instruction(Stru_SsaVariable_t* result,
                                                     Stru_SsaVariable_t* value,
                                                     size_t line, size_t column)
{
    if (result == NULL || value == NULL) {
        return NULL;
    }
    
    Stru_SsaInstruction_t* instr = F_create_ssa_instruction(
        Eum_SSA_OP_COPY, result, line, column);
    if (instr == NULL) {
        return NULL;
    }
    
    // 设置操作数
    instr->operands[0] = value;
    instr->operand_count = 1;
    
    return instr;
}
