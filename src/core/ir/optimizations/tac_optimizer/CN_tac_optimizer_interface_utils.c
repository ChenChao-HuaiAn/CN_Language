/******************************************************************************
 * 文件名: CN_tac_optimizer_interface_utils.c
 * 功能: CN_Language 三地址码优化器接口工具函数实现
 
 * 实现TAC优化器接口的工具函数，包括内部辅助函数。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_tac_optimizer_interface_internal.h"

// ============================================================================
// 内部工具函数实现
// ============================================================================

/**
 * @brief 创建默认优化结果
 */
Stru_IrOptimizationResult_t* F_create_default_optimization_result(void)
{
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
    
    return result;
}

/**
 * @brief 复制TAC数据
 */
Stru_TacData_t* F_copy_tac_data(const Stru_TacData_t* tac_data)
{
    if (!tac_data)
    {
        return NULL;
    }
    
    Stru_TacData_t* copied_tac = F_create_tac_data();
    if (!copied_tac)
    {
        return NULL;
    }
    
    size_t original_count = F_tac_data_get_instruction_count(tac_data);
    
    // 复制原始指令
    for (size_t i = 0; i < original_count; i++)
    {
        Stru_TacInstruction_t* instr = F_tac_data_get_instruction(tac_data, i);
        if (instr)
        {
            Stru_TacInstruction_t* copied_instr = F_create_tac_instruction(
                instr->opcode,
                instr->result,
                instr->operand1,
                instr->operand2,
                instr->label,
                instr->line,
                instr->column
            );
            if (copied_instr)
            {
                F_tac_data_add_instruction(copied_tac, copied_instr);
            }
        }
    }
    
    return copied_tac;
}

/**
 * @brief 应用优化到TAC数据
 */
bool F_apply_optimizations_to_tac(Stru_TacData_t* tac_data,
                                 const Stru_IrOptimizationConfig_t* config,
                                 Stru_TacData_t** optimized_tac)
{
    if (!tac_data || !config || !optimized_tac)
    {
        return false;
    }
    
    Stru_TacData_t* current_tac = *optimized_tac;
    
    // 根据配置应用优化
    if (config->enable_constant_folding)
    {
        Stru_IrOptimizationResult_t* cf_result = F_apply_tac_constant_folding(current_tac, config);
        if (cf_result && cf_result->success && cf_result->optimized_ir)
        {
            // 销毁旧的优化后数据，使用新的
            F_destroy_tac_data(current_tac);
            current_tac = (Stru_TacData_t*)cf_result->optimized_ir;
            cf_result->optimized_ir = NULL; // 防止被销毁
        }
        F_handle_optimization_result(cf_result, NULL);
    }
    
    if (config->enable_dead_code_elimination)
    {
        Stru_IrOptimizationResult_t* dce_result = F_apply_tac_dead_code_elimination(current_tac, config);
        if (dce_result && dce_result->success && dce_result->optimized_ir)
        {
            F_destroy_tac_data(current_tac);
            current_tac = (Stru_TacData_t*)dce_result->optimized_ir;
            dce_result->optimized_ir = NULL;
        }
        F_handle_optimization_result(dce_result, NULL);
    }
    
    if (config->enable_common_subexpression)
    {
        Stru_IrOptimizationResult_t* cse_result = F_apply_tac_common_subexpression_elimination(current_tac, config);
        if (cse_result && cse_result->success && cse_result->optimized_ir)
        {
            F_destroy_tac_data(current_tac);
            current_tac = (Stru_TacData_t*)cse_result->optimized_ir;
            cse_result->optimized_ir = NULL;
        }
        F_handle_optimization_result(cse_result, NULL);
    }
    
    if (config->enable_strength_reduction)
    {
        Stru_IrOptimizationResult_t* sr_result = F_apply_tac_strength_reduction(current_tac, config);
        if (sr_result && sr_result->success && sr_result->optimized_ir)
        {
            F_destroy_tac_data(current_tac);
            current_tac = (Stru_TacData_t*)sr_result->optimized_ir;
            sr_result->optimized_ir = NULL;
        }
        F_handle_optimization_result(sr_result, NULL);
    }
    
    if (config->enable_peephole_optimization)
    {
        Stru_IrOptimizationResult_t* ph_result = F_apply_tac_peephole_optimization(current_tac, config);
        if (ph_result && ph_result->success && ph_result->optimized_ir)
        {
            F_destroy_tac_data(current_tac);
            current_tac = (Stru_TacData_t*)ph_result->optimized_ir;
            ph_result->optimized_ir = NULL;
        }
        F_handle_optimization_result(ph_result, NULL);
    }
    
    *optimized_tac = current_tac;
    return true;
}

/**
 * @brief 计算优化改进比例
 */
double F_calculate_improvement_ratio(size_t original_count,
                                    size_t optimized_count,
                                    size_t removed_instructions,
                                    size_t added_instructions)
{
    if (original_count == 0)
    {
        return 0.0;
    }
    
    // 改进比例 = 移除的指令数 / 原始指令数
    return (double)removed_instructions / original_count;
}

/**
 * @brief 处理优化结果
 */
void F_handle_optimization_result(Stru_IrOptimizationResult_t* result,
                                 Stru_TacData_t* optimized_tac)
{
    if (!result)
    {
        return;
    }
    
    if (result->error_message)
    {
        cn_free(result->error_message);
    }
    
    if (result->warning_message)
    {
        cn_free(result->warning_message);
    }
    
    if (result->optimized_ir && result->optimized_ir != optimized_tac)
    {
        F_destroy_tac_data((Stru_TacData_t*)result->optimized_ir);
    }
    
    cn_free(result);
}
