/******************************************************************************
 * 文件名: CN_tac_optimizer_cse.c
 * 功能: CN_Language 三地址码公共子表达式消除优化器实现
 
 * 实现TAC公共子表达式消除优化算法，包括表达式哈希、值编号等。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "../CN_tac_optimizer.h"
#include "../../../../infrastructure/memory/utilities/CN_memory_utilities.h"
#include "../../../../infrastructure/containers/string/CN_string.h"
#include "../../../../infrastructure/containers/hash/CN_hash_table.h"
#include <string.h>
#include <stdlib.h>

// ============================================================================
// 内部辅助函数声明
// ============================================================================

static TacCommonSubexpressionPrivateData* create_cse_private_data(void);
static void destroy_cse_private_data(TacCommonSubexpressionPrivateData* data);
static uint64_t hash_tac_expression(const Stru_TacInstruction_t* instruction);
static bool are_tac_expressions_equal(const Stru_TacInstruction_t* instr1,
                                     const Stru_TacInstruction_t* instr2);

// ============================================================================
// 公共子表达式消除优化函数实现
// ============================================================================

/**
 * @brief 应用TAC公共子表达式消除
 */
Stru_IrOptimizationResult_t* F_apply_tac_common_subexpression_elimination(Stru_TacData_t* tac_data,
                                                                         const Stru_IrOptimizationConfig_t* config)
{
    if (!tac_data || !config)
    {
        return NULL;
    }
    
    // 创建优化结果
    Stru_IrOptimizationResult_t* result = cn_malloc(sizeof(Stru_IrOptimizationResult_t));
    if (!result)
    {
        return NULL;
    }
    
    // 初始化结果
    result->success = false;
    result->optimized_ir = NULL;
    result->original_instruction_count = 0;
    result->optimized_instruction_count = 0;
    result->removed_instructions = 0;
    result->added_instructions = 0;
    result->improvement_ratio = 0.0;
    result->error_message = NULL;
    result->warning_message = NULL;
    result->optimization_time = 0.0;
    
    // 获取指令数量
    size_t instruction_count = F_tac_data_get_instruction_count(tac_data);
    if (instruction_count == 0)
    {
        result->success = true;
        result->error_message = strdup("No instructions to optimize");
        return result;
    }
    
    // 创建优化后的TAC数据
    Stru_TacData_t* optimized_tac = F_create_tac_data();
    if (!optimized_tac)
    {
        result->error_message = strdup("Failed to create optimized TAC data");
        return result;
    }
    
    // 简化实现：复制所有指令
    // 在实际实现中，应该检测并消除公共子表达式
    for (size_t i = 0; i < instruction_count; i++)
    {
        Stru_TacInstruction_t* instruction = F_tac_data_get_instruction(tac_data, i);
        if (!instruction)
        {
            continue;
        }
        
        Stru_TacInstruction_t* copied_instr = F_create_tac_instruction(
            instruction->opcode,
            instruction->result,
            instruction->operand1,
            instruction->operand2,
            instruction->label,
            instruction->line,
            instruction->column
        );
        if (copied_instr)
        {
            F_tac_data_add_instruction(optimized_tac, copied_instr);
        }
    }
    
    // 设置优化结果
    result->success = true;
    result->optimized_ir = optimized_tac;
    result->original_instruction_count = instruction_count;
    result->optimized_instruction_count = F_tac_data_get_instruction_count(optimized_tac);
    result->removed_instructions = 0; // 简化实现，没有移除指令
    result->improvement_ratio = 0.0; // 简化实现，没有改进
    
    return result;
}

// ============================================================================
// 公共子表达式消除工具函数实现
// ============================================================================

/**
 * @brief 计算TAC表达式哈希值
 */
uint64_t F_hash_tac_expression(const Stru_TacInstruction_t* instruction)
{
    if (!instruction)
    {
        return 0;
    }
    
    // 简化实现：返回一个简单的哈希值
    // 在实际实现中，应该计算表达式的完整哈希
    uint64_t hash = 0;
    if (instruction->result)
    {
        hash += (uint64_t)instruction->result[0];
    }
    if (instruction->operand1)
    {
        hash += (uint64_t)instruction->operand1[0];
    }
    if (instruction->operand2)
    {
        hash += (uint64_t)instruction->operand2[0];
    }
    
    return hash;
}

/**
 * @brief 检查TAC表达式是否等价
 */
bool F_are_tac_expressions_equivalent(const Stru_TacInstruction_t* instr1,
                                     const Stru_TacInstruction_t* instr2)
{
    if (!instr1 || !instr2)
    {
        return false;
    }
    
    // 简化实现：比较操作码和操作数
    if (instr1->opcode != instr2->opcode)
    {
        return false;
    }
    
    // 比较操作数
    bool op1_equal = (!instr1->operand1 && !instr2->operand1) ||
                     (instr1->operand1 && instr2->operand1 && 
                      strcmp(instr1->operand1, instr2->operand1) == 0);
    
    bool op2_equal = (!instr1->operand2 && !instr2->operand2) ||
                     (instr1->operand2 && instr2->operand2 && 
                      strcmp(instr1->operand2, instr2->operand2) == 0);
    
    return op1_equal && op2_equal;
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

/**
 * @brief 计算TAC表达式哈希值（内部实现）
 */
static uint64_t hash_tac_expression(const Stru_TacInstruction_t* instruction)
{
    // 调用公共函数
    return F_hash_tac_expression(instruction);
}

/**
 * @brief 检查TAC表达式是否相等（内部实现）
 */
static bool are_tac_expressions_equal(const Stru_TacInstruction_t* instr1,
                                     const Stru_TacInstruction_t* instr2)
{
    // 调用公共函数
    return F_are_tac_expressions_equivalent(instr1, instr2);
}

/**
 * @brief 创建公共子表达式消除私有数据
 */
static TacCommonSubexpressionPrivateData* create_cse_private_data(void)
{
    TacCommonSubexpressionPrivateData* data = cn_malloc(sizeof(TacCommonSubexpressionPrivateData));
    if (!data)
    {
        return NULL;
    }
    
    data->expression_table = NULL;
    data->value_numbering = NULL;
    data->next_value_number = 1;
    data->enable_global_cse = false;
    data->eliminated_expressions = 0;
    
    return data;
}

/**
 * @brief 销毁公共子表达式消除私有数据
 */
static void destroy_cse_private_data(TacCommonSubexpressionPrivateData* data)
{
    if (!data)
    {
        return;
    }
    
    if (data->expression_table)
    {
        F_destroy_hash_table(data->expression_table);
    }
    
    if (data->value_numbering)
    {
        F_destroy_hash_table(data->value_numbering);
    }
    
    cn_free(data);
}
