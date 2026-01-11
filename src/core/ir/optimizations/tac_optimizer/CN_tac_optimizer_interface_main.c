/******************************************************************************
 * 文件名: CN_tac_optimizer_interface_main.c
 * 功能: CN_Language 三地址码优化器接口主要函数实现
 
 * 实现TAC优化器接口的主要函数，包括优化应用、优化级别应用等。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_tac_optimizer_interface_internal.h"

// 从CN_tac_optimizer_main.c导入的函数声明
extern Stru_IrOptimizationConfig_t* F_create_tac_default_config(void);

// ============================================================================
// 主要接口函数实现
// ============================================================================

/**
 * @brief 应用TAC优化
 */
Stru_IrOptimizationResult_t* F_apply_tac_optimization(void* ir_data,
                                                     Eum_IrOptimizationType optimization_type)
{
    // 简化实现：根据优化类型调用相应的函数
    Stru_TacData_t* tac_data = (Stru_TacData_t*)ir_data;
    Stru_IrOptimizationConfig_t* config = F_create_tac_default_config();
    
    if (!config)
    {
        return NULL;
    }
    
    Stru_IrOptimizationResult_t* result = NULL;
    
    switch (optimization_type)
    {
        case Eum_IR_OPT_CONSTANT_FOLDING:
            result = F_apply_tac_constant_folding(tac_data, config);
            break;
        case Eum_IR_OPT_DEAD_CODE_ELIMINATION:
            result = F_apply_tac_dead_code_elimination(tac_data, config);
            break;
        case Eum_IR_OPT_COMMON_SUBEXPRESSION:
            result = F_apply_tac_common_subexpression_elimination(tac_data, config);
            break;
        case Eum_IR_OPT_STRENGTH_REDUCTION:
            result = F_apply_tac_strength_reduction(tac_data, config);
            break;
        case Eum_IR_OPT_PEEPHOLE:
            result = F_apply_tac_peephole_optimization(tac_data, config);
            break;
        default:
            result = cn_malloc(sizeof(Stru_IrOptimizationResult_t));
            if (result)
            {
                result->success = false;
                // 使用strdup复制字符串
                result->error_message = strdup("Unsupported optimization type");
            }
            break;
    }
    
    cn_free(config);
    return result;
}

/**
 * @brief 应用TAC优化级别
 */
Stru_IrOptimizationResult_t* F_apply_tac_optimization_level(void* ir_data,
                                                           Eum_IrOptimizationLevel level)
{
    Stru_TacData_t* tac_data = (Stru_TacData_t*)ir_data;
    Stru_IrOptimizationConfig_t* config = F_create_tac_default_config();
    
    if (!config)
    {
        return NULL;
    }
    
    // 根据优化级别调整配置
    switch (level)
    {
        case Eum_IR_OPT_LEVEL_NONE:
            // 无优化
            config->enable_constant_folding = false;
            config->enable_dead_code_elimination = false;
            config->enable_common_subexpression = false;
            config->enable_strength_reduction = false;
            config->enable_peephole_optimization = false;
            config->aggressive_optimization = false;
            break;
            
        case Eum_IR_OPT_LEVEL_O1:
            // 基本优化
            config->enable_constant_folding = true;
            config->enable_dead_code_elimination = true;
            config->enable_common_subexpression = false;
            config->enable_strength_reduction = false;
            config->enable_peephole_optimization = true;
            config->aggressive_optimization = false;
            break;
            
        case Eum_IR_OPT_LEVEL_O2:
            // 标准优化
            config->enable_constant_folding = true;
            config->enable_dead_code_elimination = true;
            config->enable_common_subexpression = true;
            config->enable_strength_reduction = true;
            config->enable_peephole_optimization = true;
            config->aggressive_optimization = false;
            break;
            
        case Eum_IR_OPT_LEVEL_O3:
            // 激进优化
            config->enable_constant_folding = true;
            config->enable_dead_code_elimination = true;
            config->enable_common_subexpression = true;
            config->enable_strength_reduction = true;
            config->enable_peephole_optimization = true;
            config->aggressive_optimization = true;
            break;
            
        case Eum_IR_OPT_LEVEL_Os:
            // 代码大小优化
            config->enable_constant_folding = true;
            config->enable_dead_code_elimination = true;
            config->enable_common_subexpression = true;
            config->enable_strength_reduction = true;
            config->enable_peephole_optimization = true;
            config->aggressive_optimization = false;
            // 对于代码大小优化，我们可能希望更积极地消除死代码
            break;
            
        case Eum_IR_OPT_LEVEL_Oz:
            // 最大化代码大小优化
            config->enable_constant_folding = true;
            config->enable_dead_code_elimination = true;
            config->enable_common_subexpression = true;
            config->enable_strength_reduction = true;
            config->enable_peephole_optimization = true;
            config->aggressive_optimization = true;
            break;
            
        default:
            // 未知级别，使用默认配置
            break;
    }
    
    // 记录开始时间
    clock_t start_time = clock();
    
    // 创建优化结果
    Stru_IrOptimizationResult_t* result = F_create_default_optimization_result();
    if (!result)
    {
        cn_free(config);
        return NULL;
    }
    
    // 获取原始指令数
    size_t original_count = F_tac_data_get_instruction_count(tac_data);
    if (original_count == 0)
    {
        result->success = true;
        result->error_message = strdup("No instructions to optimize");
        result->original_instruction_count = 0;
        result->optimized_instruction_count = 0;
        cn_free(config);
        return result;
    }
    
    result->original_instruction_count = original_count;
    
    // 对于NONE级别，直接返回原始数据的副本
    if (level == Eum_IR_OPT_LEVEL_NONE)
    {
        Stru_TacData_t* optimized_tac = F_copy_tac_data(tac_data);
        if (!optimized_tac)
        {
            result->error_message = strdup("Failed to copy TAC data");
            cn_free(config);
            cn_free(result);
            return NULL;
        }
        
        size_t optimized_count = F_tac_data_get_instruction_count(optimized_tac);
        result->optimized_instruction_count = optimized_count;
        result->removed_instructions = 0;
        result->added_instructions = 0;
        result->improvement_ratio = 0.0;
        result->success = true;
        result->optimized_ir = optimized_tac;
        
        // 计算优化时间
        clock_t end_time = clock();
        result->optimization_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        
        cn_free(config);
        return result;
    }
    
    // 创建优化后的TAC数据
    Stru_TacData_t* optimized_tac = F_copy_tac_data(tac_data);
    if (!optimized_tac)
    {
        result->error_message = strdup("Failed to create optimized TAC data");
        cn_free(config);
        cn_free(result);
        return NULL;
    }
    
    // 根据配置应用优化
    if (!F_apply_optimizations_to_tac(optimized_tac, config, &optimized_tac))
    {
        result->error_message = strdup("Failed to apply optimizations");
        F_destroy_tac_data(optimized_tac);
        cn_free(config);
        cn_free(result);
        return NULL;
    }
    
    // 计算优化结果
    size_t optimized_count = F_tac_data_get_instruction_count(optimized_tac);
    result->optimized_instruction_count = optimized_count;
    result->removed_instructions = (original_count > optimized_count) ? (original_count - optimized_count) : 0;
    result->added_instructions = (optimized_count > original_count) ? (optimized_count - original_count) : 0;
    
    if (original_count > 0)
    {
        result->improvement_ratio = F_calculate_improvement_ratio(
            original_count, optimized_count, 
            result->removed_instructions, result->added_instructions
        );
    }
    
    // 计算优化时间
    clock_t end_time = clock();
    result->optimization_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    result->success = true;
    result->optimized_ir = optimized_tac;
    
    cn_free(config);
    return result;
}

/**
 * @brief 应用所有TAC优化
 */
Stru_IrOptimizationResult_t* F_apply_all_tac_optimizations(void* ir_data)
{
    Stru_TacData_t* tac_data = (Stru_TacData_t*)ir_data;
    Stru_IrOptimizationConfig_t* config = F_create_tac_default_config();
    
    if (!config)
    {
        return NULL;
    }
    
    // 记录开始时间
    clock_t start_time = clock();
    
    // 创建优化结果
    Stru_IrOptimizationResult_t* result = F_create_default_optimization_result();
    if (!result)
    {
        cn_free(config);
        return NULL;
    }
    
    // 获取原始指令数
    size_t original_count = F_tac_data_get_instruction_count(tac_data);
    if (original_count == 0)
    {
        result->success = true;
        result->error_message = strdup("No instructions to optimize");
        result->original_instruction_count = 0;
        result->optimized_instruction_count = 0;
        cn_free(config);
        return result;
    }
    
    result->original_instruction_count = original_count;
    
    // 创建优化后的TAC数据
    Stru_TacData_t* optimized_tac = F_copy_tac_data(tac_data);
    if (!optimized_tac)
    {
        result->error_message = strdup("Failed to create optimized TAC data");
        cn_free(config);
        cn_free(result);
        return NULL;
    }
    
    // 启用所有优化
    config->enable_constant_folding = true;
    config->enable_dead_code_elimination = true;
    config->enable_common_subexpression = true;
    config->enable_strength_reduction = true;
    config->enable_peephole_optimization = true;
    config->aggressive_optimization = true;
    
    // 根据配置应用优化
    if (!F_apply_optimizations_to_tac(optimized_tac, config, &optimized_tac))
    {
        result->error_message = strdup("Failed to apply optimizations");
        F_destroy_tac_data(optimized_tac);
        cn_free(config);
        cn_free(result);
        return NULL;
    }
    
    // 计算优化结果
    size_t optimized_count = F_tac_data_get_instruction_count(optimized_tac);
    result->optimized_instruction_count = optimized_count;
    result->removed_instructions = (original_count > optimized_count) ? (original_count - optimized_count) : 0;
    result->added_instructions = (optimized_count > original_count) ? (optimized_count - original_count) : 0;
    
    if (original_count > 0)
    {
        result->improvement_ratio = F_calculate_improvement_ratio(
            original_count, optimized_count, 
            result->removed_instructions, result->added_instructions
        );
    }
    
    // 计算优化时间
    clock_t end_time = clock();
    result->optimization_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    result->success = true;
    result->optimized_ir = optimized_tac;
    
    cn_free(config);
    return result;
}
