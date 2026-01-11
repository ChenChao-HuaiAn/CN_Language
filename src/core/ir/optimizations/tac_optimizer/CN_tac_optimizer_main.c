/******************************************************************************
 * 文件名: CN_tac_optimizer_main.c
 * 功能: CN_Language 三地址码优化器主接口实现
 
 * 实现TAC优化器的主接口函数，包括创建和销毁优化器接口。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "../CN_tac_optimizer.h"
#include "../../../../infrastructure/memory/utilities/CN_memory_utilities.h"
#include <stdlib.h>

// ============================================================================
// 内部辅助函数声明
// ============================================================================

// 私有数据管理函数
static TacOptimizerPrivateData* create_tac_optimizer_private_data(void);
static void destroy_tac_optimizer_private_data(TacOptimizerPrivateData* data);
static bool initialize_tac_optimizer_private(TacOptimizerPrivateData* private_data,
                                            const Stru_IrOptimizationConfig_t* config);

// 接口函数实现
static const char* get_tac_optimizer_name(void);
static const char* get_tac_optimizer_version_string(void);
static Eum_IrType get_tac_optimizer_supported_ir_type(void);
static bool set_tac_optimizer_config(Stru_IrOptimizationConfig_t* config);
static Stru_IrOptimizationConfig_t* get_tac_optimizer_config(void);
Stru_IrOptimizationConfig_t* F_create_tac_default_config(void);

// 从CN_tac_optimizer_interface.c导入的函数声明
extern Stru_IrOptimizationResult_t* F_apply_tac_optimization(void* ir_data,
                                                           Eum_IrOptimizationType optimization_type);
extern Stru_IrOptimizationResult_t* F_apply_tac_optimization_level(void* ir_data,
                                                                 Eum_IrOptimizationLevel level);
extern Stru_IrOptimizationResult_t* F_apply_all_tac_optimizations(void* ir_data);
extern bool F_analyze_tac_ir(void* ir_data,
                           Stru_IrOptimizationStats_t* stats);
extern bool F_can_apply_tac_optimization(void* ir_data,
                                        Eum_IrOptimizationType optimization_type);
extern double F_estimate_tac_improvement(void* ir_data,
                                        Eum_IrOptimizationType optimization_type);
extern Stru_IrOptimizationStats_t* F_get_tac_optimizer_statistics(void);
extern bool F_reset_tac_optimizer_statistics(void);
extern bool F_generate_tac_optimization_report(const char* filename);
extern bool F_validate_tac_optimization(void* original_ir,
                                       void* optimized_ir);
extern bool F_test_tac_optimization_correctness(Eum_IrOptimizationType opt_type);
extern void F_destroy_tac_optimization_result(Stru_IrOptimizationResult_t* result);
extern void F_destroy_tac_optimization_stats(Stru_IrOptimizationStats_t* stats);

// ============================================================================
// 公共接口函数实现
// ============================================================================

/**
 * @brief 创建TAC优化器接口实例
 */
Stru_IrOptimizerInterface_t* F_create_tac_optimizer_interface(void)
{
    Stru_IrOptimizerInterface_t* interface = cn_malloc(sizeof(Stru_IrOptimizerInterface_t));
    if (!interface)
    {
        return NULL;
    }
    
    // 初始化接口函数指针
    interface->get_name = get_tac_optimizer_name;
    interface->get_version = get_tac_optimizer_version_string;
    interface->get_supported_ir_type = get_tac_optimizer_supported_ir_type;
    
    interface->set_config = set_tac_optimizer_config;
    interface->get_config = get_tac_optimizer_config;
    interface->create_default_config = F_create_tac_default_config;
    
    // 这些函数在CN_tac_optimizer_interface.c中实现
    interface->apply_optimization = F_apply_tac_optimization;
    interface->apply_optimization_level = F_apply_tac_optimization_level;
    interface->apply_all_optimizations = F_apply_all_tac_optimizations;
    
    interface->analyze_ir = F_analyze_tac_ir;
    interface->can_apply_optimization = F_can_apply_tac_optimization;
    interface->estimate_improvement = F_estimate_tac_improvement;
    
    interface->get_statistics = F_get_tac_optimizer_statistics;
    interface->reset_statistics = F_reset_tac_optimizer_statistics;
    interface->generate_report = F_generate_tac_optimization_report;
    
    interface->validate_optimization = F_validate_tac_optimization;
    interface->test_optimization_correctness = F_test_tac_optimization_correctness;
    
    interface->destroy_optimization_result = F_destroy_tac_optimization_result;
    interface->destroy_optimization_stats = F_destroy_tac_optimization_stats;
    interface->destroy = NULL; // 将在下面设置
    
    // 创建私有数据
    interface->internal_state = create_tac_optimizer_private_data();
    if (!interface->internal_state)
    {
        cn_free(interface);
        return NULL;
    }
    
    // 设置destroy函数，使用适配器
    interface->destroy = (void (*)(void))F_destroy_tac_optimizer_interface;
    
    return interface;
}

/**
 * @brief 销毁TAC优化器接口实例
 */
void F_destroy_tac_optimizer_interface(Stru_IrOptimizerInterface_t* optimizer)
{
    if (!optimizer)
    {
        return;
    }
    
    // 销毁内部状态
    if (optimizer->internal_state)
    {
        TacOptimizerPrivateData* private_data = (TacOptimizerPrivateData*)optimizer->internal_state;
        destroy_tac_optimizer_private_data(private_data);
    }
    
    // 销毁接口本身
    cn_free(optimizer);
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

/**
 * @brief 创建TAC优化器私有数据
 */
static TacOptimizerPrivateData* create_tac_optimizer_private_data(void)
{
    TacOptimizerPrivateData* data = cn_malloc(sizeof(TacOptimizerPrivateData));
    if (!data)
    {
        return NULL;
    }
    
    // 初始化所有字段
    data->constant_folder = NULL;
    data->dead_code_eliminator = NULL;
    data->cse_eliminator = NULL;
    data->strength_reducer = NULL;
    data->stats = NULL;
    data->config = F_create_tac_default_config();
    data->initialized = false;
    data->optimization_in_progress = false;
    data->total_optimization_time = 0.0;
    data->total_optimizations_applied = 0;
    
    return data;
}

/**
 * @brief 销毁TAC优化器私有数据
 */
static void destroy_tac_optimizer_private_data(TacOptimizerPrivateData* data)
{
    if (!data)
    {
        return;
    }
    
    // 销毁各个组件
    if (data->constant_folder)
    {
        // 在CN_tac_optimizer_constant_folding.c中实现
    }
    
    if (data->dead_code_eliminator)
    {
        // 在CN_tac_optimizer_dead_code.c中实现
    }
    
    if (data->cse_eliminator)
    {
        // 在CN_tac_optimizer_cse.c中实现
    }
    
    if (data->strength_reducer)
    {
        // 在CN_tac_optimizer_strength_reduction.c中实现
    }
    
    if (data->stats)
    {
        // 在CN_tac_optimizer_interface.c中实现
    }
    
    if (data->config)
    {
        cn_free(data->config);
    }
    
    cn_free(data);
}

/**
 * @brief 初始化TAC优化器私有数据
 */
static bool initialize_tac_optimizer_private(TacOptimizerPrivateData* private_data,
                                            const Stru_IrOptimizationConfig_t* config)
{
    if (!private_data || !config)
    {
        return false;
    }
    
    // 简化实现：总是成功
    private_data->initialized = true;
    return true;
}

// ============================================================================
// 接口函数实现
// ============================================================================

/**
 * @brief 获取TAC优化器名称
 */
static const char* get_tac_optimizer_name(void)
{
    return "TAC Optimizer";
}

/**
 * @brief 获取TAC优化器版本字符串
 */
static const char* get_tac_optimizer_version_string(void)
{
    return "1.0.0";
}

/**
 * @brief 获取TAC优化器支持的IR类型
 */
static Eum_IrType get_tac_optimizer_supported_ir_type(void)
{
    return Eum_IR_TYPE_TAC;
}

/**
 * @brief 设置TAC优化器配置
 */
static bool set_tac_optimizer_config(Stru_IrOptimizationConfig_t* config)
{
    if (!config)
    {
        return false;
    }
    
    // 简化实现：总是成功
    return true;
}

/**
 * @brief 获取TAC优化器配置
 */
static Stru_IrOptimizationConfig_t* get_tac_optimizer_config(void)
{
    Stru_IrOptimizationConfig_t* config = cn_malloc(sizeof(Stru_IrOptimizationConfig_t));
    if (!config)
    {
        return NULL;
    }
    
    // 返回默认配置
    config->enable_constant_folding = true;
    config->enable_dead_code_elimination = true;
    config->enable_common_subexpression = true;
    config->enable_strength_reduction = true;
    config->enable_loop_optimization = false;
    config->enable_function_inlining = false;
    config->enable_peephole_optimization = true;
    config->max_inlining_depth = 3;
    config->max_loop_unroll_factor = 4;
    config->aggressive_optimization = false;
    config->preserve_debug_info = true;
    config->generate_optimization_report = false;
    
    return config;
}

/**
 * @brief 创建TAC默认配置
 */
Stru_IrOptimizationConfig_t* F_create_tac_default_config(void)
{
    Stru_IrOptimizationConfig_t* config = cn_malloc(sizeof(Stru_IrOptimizationConfig_t));
    if (!config)
    {
        return NULL;
    }
    
    config->enable_constant_folding = true;
    config->enable_dead_code_elimination = true;
    config->enable_common_subexpression = true;
    config->enable_strength_reduction = true;
    config->enable_loop_optimization = false;
    config->enable_function_inlining = false;
    config->enable_peephole_optimization = true;
    config->max_inlining_depth = 3;
    config->max_loop_unroll_factor = 4;
    config->aggressive_optimization = false;
    config->preserve_debug_info = true;
    config->generate_optimization_report = false;
    
    return config;
}
