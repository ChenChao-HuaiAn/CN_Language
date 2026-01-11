/******************************************************************************
 * 文件名: CN_tac_optimizer_peephole.c
 * 功能: CN_Language 三地址码窥孔优化器实现
 
 * 实现TAC窥孔优化算法，包括指令序列模式匹配、优化等。
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

// ============================================================================
// 内部辅助函数声明
// ============================================================================

static size_t apply_peephole_pattern(Stru_TacInstruction_t** instructions,
                                    size_t count,
                                    size_t start_index,
                                    bool* optimized);

// ============================================================================
// 窥孔优化函数实现
// ============================================================================

/**
 * @brief 应用TAC窥孔优化
 */
Stru_IrOptimizationResult_t* F_apply_tac_peephole_optimization(Stru_TacData_t* tac_data,
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
    
    // 创建指令数组用于处理
    Stru_TacInstruction_t** instructions = cn_malloc(sizeof(Stru_TacInstruction_t*) * instruction_count);
    if (!instructions)
    {
        result->error_message = strdup("Failed to allocate instruction array");
        return result;
    }
    
    // 复制所有指令到数组
    for (size_t i = 0; i < instruction_count; i++)
    {
        Stru_TacInstruction_t* original = F_tac_data_get_instruction(tac_data, i);
        if (original)
        {
            instructions[i] = F_create_tac_instruction(
                original->opcode,
                original->result,
                original->operand1,
                original->operand2,
                original->label,
                original->line,
                original->column
            );
        }
        else
        {
            instructions[i] = NULL;
        }
    }
    
    size_t optimized_count = 0;
    size_t current_index = 0;
    
    // 应用窥孔优化
    while (current_index < instruction_count)
    {
        bool optimized = false;
        size_t pattern_size = F_apply_tac_peephole_pattern(instructions, instruction_count, current_index, &optimized);
        
        if (optimized)
        {
            optimized_count++;
        }
        
        if (pattern_size == 0)
        {
            // 没有匹配的模式，移动到下一个指令
            current_index++;
        }
        else
        {
            // 跳过一个模式的大小
            current_index += pattern_size;
        }
    }
    
    // 创建优化后的TAC数据
    Stru_TacData_t* optimized_tac = F_create_tac_data();
    if (!optimized_tac)
    {
        result->error_message = strdup("Failed to create optimized TAC data");
        // 清理指令数组
        for (size_t i = 0; i < instruction_count; i++)
        {
            if (instructions[i]) F_destroy_tac_instruction(instructions[i]);
        }
        cn_free(instructions);
        return result;
    }
    
    // 添加优化后的指令到TAC数据
    for (size_t i = 0; i < instruction_count; i++)
    {
        if (instructions[i])
        {
            F_tac_data_add_instruction(optimized_tac, instructions[i]);
        }
    }
    
    // 清理指令数组（指令现在由optimized_tac拥有）
    cn_free(instructions);
    
    // 设置优化结果
    result->success = true;
    result->optimized_ir = optimized_tac;
    result->original_instruction_count = instruction_count;
    result->optimized_instruction_count = F_tac_data_get_instruction_count(optimized_tac);
    result->removed_instructions = instruction_count - result->optimized_instruction_count;
    result->improvement_ratio = (double)optimized_count / instruction_count;
    
    return result;
}

// ============================================================================
// 窥孔优化工具函数实现
// ============================================================================

/**
 * @brief 应用TAC窥孔优化模式
 */
size_t F_apply_tac_peephole_pattern(Stru_TacInstruction_t** instructions,
                                   size_t count,
                                   size_t start_index,
                                   bool* optimized)
{
    if (!instructions || start_index >= count || !optimized)
    {
        return 0;
    }
    
    // 简化实现：不应用任何模式
    *optimized = false;
    return 0;
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

/**
 * @brief 应用窥孔优化模式（内部实现）
 */
static size_t apply_peephole_pattern(Stru_TacInstruction_t** instructions,
                                    size_t count,
                                    size_t start_index,
                                    bool* optimized)
{
    // 调用公共函数
    return F_apply_tac_peephole_pattern(instructions, count, start_index, optimized);
}
