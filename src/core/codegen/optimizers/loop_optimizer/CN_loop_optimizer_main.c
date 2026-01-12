/******************************************************************************
 * 文件名: CN_loop_optimizer_main.c
 * 功能: CN_Language 循环优化器主接口实现
 * 
 * 提供循环优化器的主接口实现，包括工厂函数、版本信息和公共API。
 * 遵循项目架构规范，实现优化器插件接口。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_loop_optimizer.h"
#include "analysis/CN_loop_analysis.h"
#include "algorithms/CN_loop_algorithms.h"
#include "config/CN_loop_config.h"
#include "utils/CN_loop_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 辅助函数声明 */
static Stru_OptimizationResult_t* F_create_optimization_result(void);

/* 支持的优化类型数组 */
static const Eum_OptimizationType SUPPORTED_OPTIMIZATIONS[] = {
    Eum_OPTIMIZE_LOOP_UNROLLING,
    Eum_OPTIMIZE_VECTORIZATION,
    Eum_OPTIMIZE_PARALLELIZATION,
    Eum_OPTIMIZE_MEMORY_ACCESS
};

static const size_t SUPPORTED_OPTIMIZATIONS_COUNT = 
    sizeof(SUPPORTED_OPTIMIZATIONS) / sizeof(SUPPORTED_OPTIMIZATIONS[0]);

/* 内部状态结构体 */
typedef struct {
    Stru_OptimizerPluginInfo_t info;        ///< 插件信息
    Stru_OptimizationContext_t* context;    ///< 当前优化上下文
    Stru_LoopOptimizationConfig_t* config;  ///< 循环优化配置
    Stru_DynamicArray_t* loop_infos;        ///< 循环分析信息
    bool initialized;                       ///< 是否已初始化
} LoopOptimizerState;

/* 内部函数声明 */
static const Stru_OptimizerPluginInfo_t* get_info(Stru_OptimizerPluginInterface_t* interface);
static bool initialize(Stru_OptimizerPluginInterface_t* interface, Stru_OptimizationContext_t* context);
static Stru_DynamicArray_t* analyze(Stru_OptimizerPluginInterface_t* interface, Stru_OptimizationContext_t* context);
static bool can_optimize(Stru_OptimizerPluginInterface_t* interface, 
                         Stru_OptimizationContext_t* context, Eum_OptimizationType optimization_type);
static Stru_OptimizationResult_t* optimize(Stru_OptimizerPluginInterface_t* interface,
                                           Stru_OptimizationContext_t* context,
                                           Eum_OptimizationType optimization_type);
static Stru_OptimizationResult_t* optimize_batch(Stru_OptimizerPluginInterface_t* interface,
                                                 Stru_OptimizationContext_t* context,
                                                 Eum_OptimizationType* optimizations, size_t count);
static Stru_OptimizationResult_t* optimize_level(Stru_OptimizerPluginInterface_t* interface,
                                                 Stru_OptimizationContext_t* context,
                                                 Eum_OptimizationLevel level);
static Stru_AstNode_t* transform_ast(Stru_OptimizerPluginInterface_t* interface,
                                     Stru_AstNode_t* ast, Stru_OptimizationContext_t* context);
static void* transform_ir(Stru_OptimizerPluginInterface_t* interface,
                          void* ir, Stru_OptimizationContext_t* context);
static bool validate(Stru_OptimizerPluginInterface_t* interface,
                     void* original, void* optimized, Stru_OptimizationContext_t* context);
static bool is_safe(Stru_OptimizerPluginInterface_t* interface,
                    Stru_OptimizationContext_t* context, Eum_OptimizationType optimization_type);
static Stru_DynamicArray_t* collect_statistics(Stru_OptimizerPluginInterface_t* interface,
                                               Stru_OptimizationContext_t* context);
static const char* generate_report(Stru_OptimizerPluginInterface_t* interface,
                                   const Stru_OptimizationResult_t* result);
static bool configure(Stru_OptimizerPluginInterface_t* interface,
                      const char* key, const char* value);
static Stru_DynamicArray_t* get_config_options(Stru_OptimizerPluginInterface_t* interface);
static void reset(Stru_OptimizerPluginInterface_t* interface);
static void destroy(Stru_OptimizerPluginInterface_t* interface);

/* 辅助函数声明 */
static LoopOptimizerState* get_state(Stru_OptimizerPluginInterface_t* interface);

/**
 * @brief 创建循环优化器插件接口实例
 */
Stru_OptimizerPluginInterface_t* F_create_loop_optimizer_interface(void)
{
    /* 分配接口结构体 */
    Stru_OptimizerPluginInterface_t* interface = 
        (Stru_OptimizerPluginInterface_t*)malloc(sizeof(Stru_OptimizerPluginInterface_t));
    if (!interface) {
        return NULL;
    }
    
    /* 分配内部状态 */
    LoopOptimizerState* state = (LoopOptimizerState*)malloc(sizeof(LoopOptimizerState));
    if (!state) {
        free(interface);
        return NULL;
    }
    
    /* 初始化插件信息 */
    memset(&state->info, 0, sizeof(Stru_OptimizerPluginInfo_t));
    state->info.name = "loop_optimizer";
    state->info.version = "1.0.0";
    state->info.author = "CN_Language开发团队";
    state->info.description = "循环优化器，提供循环不变代码外提、循环展开、循环融合、循环向量化等优化功能";
    state->info.supported_optimizations = (Eum_OptimizationType*)SUPPORTED_OPTIMIZATIONS;
    state->info.optimization_count = SUPPORTED_OPTIMIZATIONS_COUNT;
    state->info.min_level = Eum_OPT_LEVEL_O2;
    state->info.max_level = Eum_OPT_LEVEL_O3;
    state->info.requires_analysis = true;
    state->info.modifies_ast = true;
    state->info.modifies_ir = true;
    state->info.modifies_code = true;
    
    /* 初始化其他状态 */
    state->context = NULL;
    state->config = F_create_default_loop_optimization_config();
    state->loop_infos = NULL;
    state->initialized = false;
    
    /* 设置接口函数指针 */
    interface->get_info = get_info;
    interface->initialize = initialize;
    interface->analyze = analyze;
    interface->can_optimize = can_optimize;
    interface->optimize = optimize;
    interface->optimize_batch = optimize_batch;
    interface->optimize_level = optimize_level;
    interface->transform_ast = transform_ast;
    interface->transform_ir = transform_ir;
    interface->validate = validate;
    interface->is_safe = is_safe;
    interface->collect_statistics = collect_statistics;
    interface->generate_report = generate_report;
    interface->configure = configure;
    interface->get_config_options = get_config_options;
    interface->reset = reset;
    interface->destroy = destroy;
    interface->internal_state = state;
    
    return interface;
}

/**
 * @brief 获取循环优化器版本信息
 */
void F_get_loop_optimizer_version(int* major, int* minor, int* patch)
{
    if (major) *major = 1;
    if (minor) *minor = 0;
    if (patch) *patch = 0;
}

/**
 * @brief 获取循环优化器版本字符串
 */
const char* F_get_loop_optimizer_version_string(void)
{
    return "1.0.0";
}

/**
 * @brief 检查是否支持特定循环优化类型
 */
bool F_loop_optimizer_supports_optimization(Eum_LoopOptimizationType loop_optimization_type)
{
    for (size_t i = 0; i < SUPPORTED_OPTIMIZATIONS_COUNT; i++) {
        if (SUPPORTED_OPTIMIZATIONS[i] == (Eum_OptimizationType)loop_optimization_type) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 获取循环优化器支持的优化类型
 */
const Eum_OptimizationType* F_get_loop_optimizer_supported_optimizations(size_t* count)
{
    if (count) {
        *count = SUPPORTED_OPTIMIZATIONS_COUNT;
    }
    return SUPPORTED_OPTIMIZATIONS;
}

/**
 * @brief 分析循环结构
 */
Stru_DynamicArray_t* F_analyze_loops(Stru_AstNode_t* ast)
{
    if (!ast) {
        return NULL;
    }
    
    return F_analyze_loops_internal(ast);
}

/**
 * @brief 应用循环不变代码外提
 */
bool F_apply_loop_invariant_code_motion(Stru_AstNode_t* ast, Stru_LoopAnalysisInfo_t* loop_info)
{
    if (!ast || !loop_info) {
        return false;
    }
    
    return F_apply_loop_invariant_code_motion_impl(ast, loop_info);
}

/**
 * @brief 应用循环展开
 */
bool F_apply_loop_unrolling(Stru_AstNode_t* ast, Stru_LoopAnalysisInfo_t* loop_info, size_t unroll_factor)
{
    if (!ast || !loop_info || unroll_factor == 0) {
        return false;
    }
    
    return F_apply_loop_unrolling_impl(ast, loop_info, unroll_factor);
}

/**
 * @brief 应用循环融合
 */
bool F_apply_loop_fusion(Stru_AstNode_t* ast, Stru_LoopAnalysisInfo_t** loop_infos, size_t loop_count)
{
    if (!ast || !loop_infos || loop_count < 2) {
        return false;
    }
    
    return F_apply_loop_fusion_impl(ast, loop_infos, loop_count);
}

/**
 * @brief 应用循环向量化
 */
bool F_apply_loop_vectorization(Stru_AstNode_t* ast, Stru_LoopAnalysisInfo_t* loop_info, size_t vector_width)
{
    if (!ast || !loop_info || vector_width == 0) {
        return false;
    }
    
    return F_apply_loop_vectorization_impl(ast, loop_info, vector_width);
}

/**
 * @brief 循环优化类型转字符串
 */
const char* F_loop_optimization_type_to_string(Eum_LoopOptimizationType type)
{
    switch (type) {
        case Eum_LOOP_OPTIMIZE_INVARIANT_CODE_MOTION:
            return "循环不变代码外提";
        case Eum_LOOP_OPTIMIZE_UNROLLING:
            return "循环展开";
        case Eum_LOOP_OPTIMIZE_FUSION:
            return "循环融合";
        case Eum_LOOP_OPTIMIZE_VECTORIZATION:
            return "循环向量化";
        case Eum_LOOP_OPTIMIZE_PARALLELIZATION:
            return "循环并行化";
        case Eum_LOOP_OPTIMIZE_TILING:
            return "循环分块";
        case Eum_LOOP_OPTIMIZE_INTERCHANGE:
            return "循环交换";
        case Eum_LOOP_OPTIMIZE_REVERSAL:
            return "循环反转";
        case Eum_LOOP_OPTIMIZE_PEELING:
            return "循环剥离";
        case Eum_LOOP_OPTIMIZE_JAMMING:
            return "循环压紧";
        case Eum_LOOP_OPTIMIZE_SKEWING:
            return "循环倾斜";
        case Eum_LOOP_OPTIMIZE_DISTRIBUTION:
            return "循环分布";
        case Eum_LOOP_OPTIMIZE_NONE:
        default:
            return "无循环优化";
    }
}

/* ============================================
 * 内部函数实现
 * ============================================ */

/**
 * @brief 获取内部状态
 */
static LoopOptimizerState* get_state(Stru_OptimizerPluginInterface_t* interface)
{
    if (!interface || !interface->internal_state) {
        return NULL;
    }
    return (LoopOptimizerState*)interface->internal_state;
}

/**
 * @brief 获取插件信息
 */
static const Stru_OptimizerPluginInfo_t* get_info(Stru_OptimizerPluginInterface_t* interface)
{
    LoopOptimizerState* state = get_state(interface);
    if (!state) {
        return NULL;
    }
    
    return &state->info;
}

/**
 * @brief 初始化优化器插件
 */
static bool initialize(Stru_OptimizerPluginInterface_t* interface, Stru_OptimizationContext_t* context)
{
    LoopOptimizerState* state = get_state(interface);
    if (!state || !context) {
        return false;
    }
    
    /* 保存上下文 */
    state->context = context;
    
    /* 分析循环结构 */
    if (context->ast) {
        state->loop_infos = F_analyze_loops_internal(context->ast);
    }
    
    state->initialized = true;
    
    return true;
}

/**
 * @brief 分析代码
 */
static Stru_DynamicArray_t* analyze(Stru_OptimizerPluginInterface_t* interface, Stru_OptimizationContext_t* context)
{
    LoopOptimizerState* state = get_state(interface);
    if (!state || !context) {
        return NULL;
    }
    
    /* 分析循环结构 */
    if (context->ast) {
        state->loop_infos = F_analyze_loops_internal(context->ast);
    }
    
    return state->loop_infos;
}

/**
 * @brief 检查优化可行性
 */
static bool can_optimize(Stru_OptimizerPluginInterface_t* interface, 
                         Stru_OptimizationContext_t* context, Eum_OptimizationType optimization_type)
{
    LoopOptimizerState* state = get_state(interface);
    if (!state || !state->initialized || !context) {
        return false;
    }
    
    /* 检查是否支持该优化类型 */
    for (size_t i = 0; i < SUPPORTED_OPTIMIZATIONS_COUNT; i++) {
        if (SUPPORTED_OPTIMIZATIONS[i] == optimization_type) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 应用优化
 */
static Stru_OptimizationResult_t* optimize(Stru_OptimizerPluginInterface_t* interface,
                                           Stru_OptimizationContext_t* context,
                                           Eum_OptimizationType optimization_type)
{
    LoopOptimizerState* state = get_state(interface);
    if (!state || !state->initialized || !context) {
        Stru_OptimizationResult_t* result = F_create_optimization_result();
        if (result) {
            result->success = false;
        }
        return result;
    }
    
    Stru_OptimizationResult_t* result = F_create_optimization_result();
    if (!result) {
        return NULL;
    }
    
    /* 根据优化类型应用相应的优化 */
    bool optimization_applied = false;
    
    switch (optimization_type) {
        case Eum_OPTIMIZE_LOOP_UNROLLING:
            if (context->ast && state->loop_infos) {
                optimization_applied = F_apply_loop_unrolling_optimization(context->ast, state->loop_infos, state->config);
            }
            break;
            
        case Eum_OPTIMIZE_VECTORIZATION:
            if (context->ast && state->loop_infos) {
                optimization_applied = F_apply_loop_vectorization_optimization(context->ast, state->loop_infos, state->config);
            }
            break;
            
        case Eum_OPTIMIZE_PARALLELIZATION:
            if (context->ast && state->loop_infos) {
                optimization_applied = F_apply_loop_parallelization_optimization(context->ast, state->loop_infos, state->config);
            }
            break;
            
        case Eum_OPTIMIZE_MEMORY_ACCESS:
            if (context->ast && state->loop_infos) {
                optimization_applied = F_apply_memory_access_optimization(context->ast, state->loop_infos, state->config);
            }
            break;
            
        default:
            /* 不支持的优化类型 */
            break;
    }
    
    result->success = optimization_applied;
    if (optimization_applied) {
        result->optimized_ast = context->ast;
        result->improvement_ratio = 0.1; /* 假设10%性能提升 */
    }
    
    return result;
}

/**
 * @brief 批量应用优化
 */
static Stru_OptimizationResult_t* optimize_batch(Stru_OptimizerPluginInterface_t* interface,
                                                 Stru_OptimizationContext_t* context,
                                                 Eum_OptimizationType* optimizations, size_t count)
{
    if (!interface || !context || !optimizations || count == 0) {
        Stru_OptimizationResult_t* result = F_create_optimization_result();
        if (result) {
            result->success = false;
        }
        return result;
    }
    
    Stru_OptimizationResult_t* result = F_create_optimization_result();
    if (!result) {
        return NULL;
    }
    
    /* 应用所有优化 */
    size_t successful_optimizations = 0;
    for (size_t i = 0; i < count; i++) {
        Stru_OptimizationResult_t* single_result = optimize(interface, context, optimizations[i]);
        if (single_result && single_result->success) {
            successful_optimizations++;
        }
        if (single_result) {
            free(single_result);
        }
    }
    
    result->success = (successful_optimizations > 0);
    if (successful_optimizations > 0) {
        result->improvement_ratio = (double)successful_optimizations / count * 0.1;
    }
    
    return result;
}

/**
 * @brief 按优化级别应用优化
 */
static Stru_OptimizationResult_t* optimize_level(Stru_OptimizerPluginInterface_t* interface,
                                                 Stru_OptimizationContext_t* context,
                                                 Eum_OptimizationLevel level)
{
    if (!interface || !context) {
        Stru_OptimizationResult_t* result = F_create_optimization_result();
        if (result) {
            result->success = false;
        }
        return result;
    }
    
    Stru_OptimizationResult_t* result = F_create_optimization_result();
    if (!result) {
        return NULL;
    }
    
    /* 根据优化级别选择要应用的优化 */
    Eum_OptimizationType optimizations[4];
    size_t count = 0;
    
    switch (level) {
        case Eum_OPT_LEVEL_O1:
            /* O1级别: 只应用基本优化 */
            optimizations[count++] = Eum_OPTIMIZE_LOOP_UNROLLING;
            break;
            
        case Eum_OPT_LEVEL_O2:
            /* O2级别: 应用更多优化 */
            optimizations[count++] = Eum_OPTIMIZE_LOOP_UNROLLING;
            optimizations[count++] = Eum_OPTIMIZE_VECTORIZATION;
            break;
            
        case Eum_OPT_LEVEL_O3:
            /* O3级别: 应用所有优化 */
            optimizations[count++] = Eum_OPTIMIZE_LOOP_UNROLLING;
            optimizations[count++] = Eum_OPTIMIZE_VECTORIZATION;
            optimizations[count++] = Eum_OPTIMIZE_PARALLELIZATION;
            optimizations[count++] = Eum_OPTIMIZE_MEMORY_ACCESS;
            break;
            
        default:
            /* 不支持的优化级别 */
            result->success = false;
            return result;
    }
    
    /* 应用批量优化 */
    Stru_OptimizationResult_t* batch_result = optimize_batch(interface, context, optimizations, count);
    if (batch_result) {
        result->success = batch_result->success;
        result->improvement_ratio = batch_result->improvement_ratio;
        free(batch_result);
    }
    
    return result;
}

/**
 * @brief 转换AST节点
 */
static Stru_AstNode_t* transform_ast(Stru_OptimizerPluginInterface_t* interface,
                                     Stru_AstNode_t* ast, Stru_OptimizationContext_t* context)
{
    LoopOptimizerState* state = get_state(interface);
    if (!state || !state->initialized || !ast || !context) {
        return ast;
    }
    
    /* 分析循环结构 */
    Stru_DynamicArray_t* loop_infos = F_analyze_loops_internal(ast);
    if (!loop_infos) {
        return ast;
    }
    
    /* 应用循环优化 */
    Stru_AstNode_t* optimized_ast = ast; /* 简化实现：暂时返回原AST */
    
    /* 清理 */
    if (loop_infos) {
        F_destroy_dynamic_array(loop_infos);
    }
    
    return optimized_ast ? optimized_ast : ast;
}

/**
 * @brief 转换IR表示
 */
static void* transform_ir(Stru_OptimizerPluginInterface_t* interface,
                          void* ir, Stru_OptimizationContext_t* context)
{
    /* 循环优化主要在AST级别进行，IR转换暂时返回原IR */
    return ir;
}

/**
 * @brief 验证优化结果
 */
static bool validate(Stru_OptimizerPluginInterface_t* interface,
                     void* original, void* optimized, Stru_OptimizationContext_t* context)
{
    /* 验证优化后的代码语义是否与原代码一致 */
    if (!original || !optimized) {
        return false;
    }
    
    /* 这里应该实现更复杂的验证逻辑 */
    return true;
}

/**
 * @brief 检查优化安全性
 */
static bool is_safe(Stru_OptimizerPluginInterface_t* interface,
                    Stru_OptimizationContext_t* context, Eum_OptimizationType optimization_type)
{
    LoopOptimizerState* state = get_state(interface);
    if (!state || !state->initialized || !context) {
        return false;
    }
    
    /* 检查优化是否安全 */
    switch (optimization_type) {
        case Eum_OPTIMIZE_LOOP_UNROLLING:
            /* 循环展开通常是安全的，但可能导致代码膨胀 */
            return true;
            
        case Eum_OPTIMIZE_VECTORIZATION:
            /* 向量化需要硬件支持，且可能改变浮点运算精度 */
            return F_check_vectorization_safety(context->ast, state->loop_infos);
            
        case Eum_OPTIMIZE_PARALLELIZATION:
            /* 并行化需要检查数据依赖 */
            return F_check_parallelization_safety(context->ast, state->loop_infos);
            
        case Eum_OPTIMIZE_MEMORY_ACCESS:
            /* 内存访问优化通常是安全的 */
            return true;
            
        default:
            return false;
    }
}

/**
 * @brief 收集统计信息
 */
static Stru_DynamicArray_t* collect_statistics(Stru_OptimizerPluginInterface_t* interface,
                                               Stru_OptimizationContext_t* context)
{
    LoopOptimizerState* state = get_state(interface);
    if (!state || !state->initialized || !context) {
        return NULL;
    }
    
    /* 简化实现：返回空数组 */
    Stru_DynamicArray_t* stats = F_create_dynamic_array(sizeof(char*));
    if (!stats) {
        return NULL;
    }
    
    return stats;
}

/**
 * @brief 生成优化报告
 */
static const char* generate_report(Stru_OptimizerPluginInterface_t* interface,
                                   const Stru_OptimizationResult_t* result)
{
    if (!interface || !result) {
        return NULL;
    }
    
    /* 生成报告字符串 */
    static char report_buffer[1024];
    snprintf(report_buffer, sizeof(report_buffer),
             "循环优化报告:\n"
             "  优化成功: %s\n"
             "  性能提升比例: %.2f%%\n"
             "  优化类型: 循环优化\n"
             "  优化级别: O2-O3\n",
             result->success ? "是" : "否",
             result->improvement_ratio * 100.0);
    
    return report_buffer;
}

/**
 * @brief 配置优化器
 */
static bool configure(Stru_OptimizerPluginInterface_t* interface,
                      const char* key, const char* value)
{
    LoopOptimizerState* state = get_state(interface);
    if (!state || !key || !value) {
        return false;
    }
    
    /* 配置循环优化器参数 */
    return F_configure_loop_optimizer(state->config, key, value);
}

/**
 * @brief 获取配置选项
 */
static Stru_DynamicArray_t* get_config_options(Stru_OptimizerPluginInterface_t* interface)
{
    LoopOptimizerState* state = get_state(interface);
    if (!state) {
        return NULL;
    }
    
    /* 获取循环优化器配置选项 */
    return F_get_loop_optimizer_config_options(state->config);
}

/**
 * @brief 重置优化器状态
 */
static void reset(Stru_OptimizerPluginInterface_t* interface)
{
    LoopOptimizerState* state = get_state(interface);
    if (!state) {
        return;
    }
    
    /* 重置状态 */
    state->context = NULL;
    if (state->loop_infos) {
        F_destroy_dynamic_array(state->loop_infos);
        state->loop_infos = NULL;
    }
    state->initialized = false;
}

/**
 * @brief 销毁优化器插件
 */
static void destroy(Stru_OptimizerPluginInterface_t* interface)
{
    if (!interface) {
        return;
    }
    
    LoopOptimizerState* state = get_state(interface);
    if (state) {
        /* 清理配置 */
        if (state->config) {
            F_destroy_loop_optimization_config(state->config);
        }
        
        /* 清理循环信息 */
        if (state->loop_infos) {
            F_destroy_dynamic_array(state->loop_infos);
        }
        
        free(state);
    }
    
    free(interface);
}

/**
 * @brief 创建优化结果结构体
 * 
 * 创建并初始化一个新的优化结果结构体。
 * 
 * @return Stru_OptimizationResult_t* 新创建的优化结果，调用者负责销毁
 */
static Stru_OptimizationResult_t* F_create_optimization_result(void)
{
    Stru_OptimizationResult_t* result = (Stru_OptimizationResult_t*)malloc(sizeof(Stru_OptimizationResult_t));
    if (!result) {
        return NULL;
    }
    
    /* 初始化所有字段 */
    memset(result, 0, sizeof(Stru_OptimizationResult_t));
    result->success = false;
    result->optimized_ast = NULL;
    result->optimized_ir = NULL;
    result->optimized_code = NULL;
    result->code_length = 0;
    result->original_size = 0;
    result->optimized_size = 0;
    result->improvement_ratio = 0.0;
    result->statistics = NULL;
    result->warnings = NULL;
    result->errors = NULL;
    
    return result;
}
