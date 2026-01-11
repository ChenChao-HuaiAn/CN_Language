/******************************************************************************
 * 文件名: CN_tac_optimizer_constant_folding.c
 * 功能: CN_Language 三地址码常量折叠优化器实现
 
 * 实现TAC常量折叠优化算法，包括常量检测、表达式折叠等。
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
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

// ============================================================================
// 内部辅助函数声明
// ============================================================================

static TacConstantFoldingPrivateData* create_constant_folding_private_data(void);
static void destroy_constant_folding_private_data(TacConstantFoldingPrivateData* data);
static bool fold_tac_constant_expression(const Stru_TacInstruction_t* instruction, char** result);
static bool is_tac_operand_constant(const char* operand);

// ============================================================================
// 常量折叠优化函数实现
// ============================================================================

/**
 * @brief 应用TAC常量折叠
 */
Stru_IrOptimizationResult_t* F_apply_tac_constant_folding(Stru_TacData_t* tac_data,
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
    
    size_t folded_count = 0;
    size_t total_instructions = 0;
    
    // 遍历所有指令
    for (size_t i = 0; i < instruction_count; i++)
    {
        Stru_TacInstruction_t* instruction = F_tac_data_get_instruction(tac_data, i);
        if (!instruction)
        {
            continue;
        }
        
        total_instructions++;
        
        // 检查是否为常量表达式
        if (F_is_tac_instruction_constant(instruction))
        {
            char* folded_value = NULL;
            if (F_fold_constant_tac_instruction(instruction, &folded_value))
            {
                // 创建折叠后的指令
                Stru_TacInstruction_t* folded_instr = F_create_tac_instruction(
                    Eum_TAC_OP_ASSIGN,
                    instruction->result,
                    folded_value,
                    NULL,
                    NULL,
                    instruction->line,
                    instruction->column
                );
                
                if (folded_instr)
                {
                    F_tac_data_add_instruction(optimized_tac, folded_instr);
                    folded_count++;
                    cn_free(folded_value);
                    continue;
                }
                cn_free(folded_value);
            }
        }
        
        // 如果不是常量表达式，复制原始指令
        // 简化实现：创建新指令而不是复制
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
    result->removed_instructions = instruction_count - result->optimized_instruction_count;
    
    // 改进比例应该基于实际移除的指令数，而不是折叠的指令数
    // 因为常量折叠不会移除指令，只是简化它们
    if (instruction_count > 0)
    {
        result->improvement_ratio = (double)result->removed_instructions / instruction_count;
    }
    else
    {
        result->improvement_ratio = 0.0;
    }
    
    return result;
}

// ============================================================================
// 常量折叠工具函数实现
// ============================================================================

/**
 * @brief 检查TAC指令是否为常量表达式
 */
bool F_is_tac_instruction_constant(const Stru_TacInstruction_t* instruction)
{
    if (!instruction)
    {
        return false;
    }
    
    // 简化实现：检查操作数是否为数字
    // 在实际实现中，应该进行更复杂的常量检测
    if (instruction->operand1 && is_tac_operand_constant(instruction->operand1))
    {
        if (!instruction->operand2 || is_tac_operand_constant(instruction->operand2))
        {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 折叠常量TAC指令
 */
bool F_fold_constant_tac_instruction(const Stru_TacInstruction_t* instruction,
                                    char** result)
{
    if (!instruction || !result)
    {
        return false;
    }
    
    // 检查是否为常量赋值指令
    if (instruction->opcode == Eum_TAC_OP_ASSIGN && instruction->operand1)
    {
        // 如果是赋值指令，直接返回操作数的值
        *result = strdup(instruction->operand1);
        return (*result != NULL);
    }
    
    // 检查是否为二元运算指令
    if (instruction->operand1 && instruction->operand2)
    {
        char* endptr1, *endptr2;
        double val1 = strtod(instruction->operand1, &endptr1);
        double val2 = strtod(instruction->operand2, &endptr2);
        
        // 检查两个操作数是否都是数字
        if (*endptr1 == '\0' && *endptr2 == '\0')
        {
            double result_val = 0.0;
            
            switch (instruction->opcode)
            {
                case Eum_TAC_OP_ADD:
                    result_val = val1 + val2;
                    break;
                case Eum_TAC_OP_SUB:
                    result_val = val1 - val2;
                    break;
                case Eum_TAC_OP_MUL:
                    result_val = val1 * val2;
                    break;
                case Eum_TAC_OP_DIV:
                    if (val2 != 0.0)
                    {
                        result_val = val1 / val2;
                    }
                    else
                    {
                        return false; // 除零错误
                    }
                    break;
                default:
                    return false; // 不支持的操作
            }
            
            // 将结果转换为字符串
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%.6g", result_val);
            *result = strdup(buffer);
            return (*result != NULL);
        }
    }
    
    return false;
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

/**
 * @brief 检查TAC操作数是否为常量
 */
static bool is_tac_operand_constant(const char* operand)
{
    if (!operand)
    {
        return false;
    }
    
    // 简化实现：检查是否为数字
    char* endptr;
    strtod(operand, &endptr);
    return (*endptr == '\0');
}

/**
 * @brief 创建常量折叠私有数据
 */
static TacConstantFoldingPrivateData* create_constant_folding_private_data(void)
{
    TacConstantFoldingPrivateData* data = cn_malloc(sizeof(TacConstantFoldingPrivateData));
    if (!data)
    {
        return NULL;
    }
    
    data->constant_values = NULL;
    data->expression_cache = NULL;
    data->aggressive_folding = false;
    data->max_cache_size = 1000;
    data->cache_hits = 0;
    data->cache_misses = 0;
    
    return data;
}

/**
 * @brief 销毁常量折叠私有数据
 */
static void destroy_constant_folding_private_data(TacConstantFoldingPrivateData* data)
{
    if (!data)
    {
        return;
    }
    
    if (data->constant_values)
    {
        F_destroy_hash_table(data->constant_values);
    }
    
    if (data->expression_cache)
    {
        F_destroy_hash_table(data->expression_cache);
    }
    
    cn_free(data);
}
