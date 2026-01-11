/******************************************************************************
 * 文件名: CN_optimization_evaluator_main.c
 * 功能: CN_Language 优化效果评估器主模块
 
 * 实现优化效果评估器的主工厂函数和接口管理。
 * 
 * 遵循项目架构规范：
 * 1. 模块化、解耦简单、可读性高的结构
 * 2. 单一职责原则：每个函数不超过50行
 * 3. 头文件只暴露必要的接口
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "../CN_optimization_evaluator.h"
#include "CN_optimization_evaluator_internal.h"
#include "../../../../infrastructure/memory/utilities/CN_memory_utilities.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 全局变量定义
// ============================================================================

/**
 * @brief 当前活动的评估器实例
 
 * 用于解决接口中destroy函数无参数的问题。
 * 注意：这是一个简化实现，假设只有一个活动的评估器实例。
 * 在实际的多实例场景中，需要更复杂的实现。
 */
Stru_OptimizationEvaluatorInterface_t* g_current_evaluator = NULL;

// ============================================================================
// 内部状态管理函数声明
// ============================================================================

static Stru_OptimizationEvaluatorState_t* create_internal_state(void);
static void destroy_internal_state(Stru_OptimizationEvaluatorState_t* state);
bool add_metrics_to_history(Stru_OptimizationEvaluatorState_t* state,
                           const Stru_OptimizationMetrics_t* metrics);

// ============================================================================
// 接口函数声明
// ============================================================================

// 基本信息函数
const char* get_name(void);
const char* get_version(void);

// 效果评估函数
Stru_OptimizationEvaluationResult_t* evaluate_optimization(
    void* original_ir,
    void* optimized_ir,
    Eum_IrOptimizationType opt_type);
Stru_OptimizationEvaluationResult_t* evaluate_optimization_level(
    void* original_ir,
    void* optimized_ir,
    Eum_IrOptimizationLevel level);

// 基准测试函数
bool run_benchmark(Stru_OptimizationBenchmark_t* benchmark);
Stru_OptimizationBenchmark_t* create_benchmark(const char* name,
                                                     void* original_ir,
                                                     void* optimized_ir);
void destroy_benchmark(Stru_OptimizationBenchmark_t* benchmark);

// 报告生成函数
char* generate_evaluation_report(const Stru_OptimizationEvaluationResult_t* result);
bool save_report_to_file(const Stru_OptimizationEvaluationResult_t* result,
                               const char* filename);

// 统计分析函数
bool analyze_optimization_trends(Stru_OptimizationEvaluationResult_t** results,
                                       size_t count,
                                       Stru_OptimizationMetrics_t* trends);

// 资源管理函数
void destroy_evaluation_result(Stru_OptimizationEvaluationResult_t* result);
void destroy_interface(void);

// ============================================================================
// 工厂函数实现
// ============================================================================

/**
 * @brief 创建优化效果评估器接口实例
 
 * 创建优化效果评估器接口实例。
 * 
 * @return Stru_OptimizationEvaluatorInterface_t* 评估器接口实例，失败返回NULL
 */
Stru_OptimizationEvaluatorInterface_t* F_create_optimization_evaluator_interface(void)
{
    Stru_OptimizationEvaluatorInterface_t* evaluator = 
        (Stru_OptimizationEvaluatorInterface_t*)cn_malloc(sizeof(Stru_OptimizationEvaluatorInterface_t));
    
    if (!evaluator)
    {
        return NULL;
    }
    
    // 创建内部状态
    evaluator->internal_state = create_internal_state();
    if (!evaluator->internal_state)
    {
        cn_free(evaluator);
        return NULL;
    }
    
    // 设置接口函数指针
    evaluator->get_name = get_name;
    evaluator->get_version = get_version;
    evaluator->evaluate_optimization = evaluate_optimization;
    evaluator->evaluate_optimization_level = evaluate_optimization_level;
    evaluator->run_benchmark = run_benchmark;
    evaluator->create_benchmark = create_benchmark;
    evaluator->destroy_benchmark = destroy_benchmark;
    evaluator->generate_evaluation_report = generate_evaluation_report;
    evaluator->save_report_to_file = save_report_to_file;
    evaluator->analyze_optimization_trends = analyze_optimization_trends;
    evaluator->destroy_evaluation_result = destroy_evaluation_result;
    
    // 设置destroy函数为适配器函数
    evaluator->destroy = destroy_interface;
    
    // 存储当前评估器实例到全局变量
    g_current_evaluator = evaluator;
    
    return evaluator;
}

/**
 * @brief 销毁优化效果评估器接口实例
 
 * 销毁优化效果评估器接口实例及其所有资源。
 * 
 * @param evaluator 评估器接口实例
 */
void F_destroy_optimization_evaluator_interface(Stru_OptimizationEvaluatorInterface_t* evaluator)
{
    if (!evaluator)
    {
        return;
    }
    
    // 销毁内部状态
    if (evaluator->internal_state)
    {
        destroy_internal_state(evaluator->internal_state);
        evaluator->internal_state = NULL;
    }
    
    cn_free(evaluator);
    
    // 如果销毁的是当前评估器，清空全局变量
    if (g_current_evaluator == evaluator)
    {
        g_current_evaluator = NULL;
    }
}

// ============================================================================
// 内部状态管理函数实现
// ============================================================================

/**
 * @brief 创建内部状态
 
 * 创建优化效果评估器的内部状态。
 * 
 * @return Stru_OptimizationEvaluatorState_t* 内部状态，失败返回NULL
 */
static Stru_OptimizationEvaluatorState_t* create_internal_state(void)
{
    Stru_OptimizationEvaluatorState_t* state = 
        (Stru_OptimizationEvaluatorState_t*)cn_malloc(sizeof(Stru_OptimizationEvaluatorState_t));
    
    if (!state)
    {
        return NULL;
    }
    
    // 初始化状态
    state->total_evaluations = 0;
    state->successful_evaluations = 0;
    state->total_evaluation_time = 0.0;
    state->metrics_history = NULL;
    state->metrics_history_size = 0;
    state->metrics_history_capacity = 0;
    
    return state;
}

/**
 * @brief 销毁内部状态
 
 * 销毁优化效果评估器的内部状态。
 * 
 * @param state 内部状态
 */
static void destroy_internal_state(Stru_OptimizationEvaluatorState_t* state)
{
    if (!state)
    {
        return;
    }
    
    // 释放指标历史记录
    if (state->metrics_history)
    {
        cn_free(state->metrics_history);
    }
    
    cn_free(state);
}

/**
 * @brief 添加指标到历史记录
 
 * 将优化指标添加到历史记录中。
 * 
 * @param state 内部状态
 * @param metrics 优化指标
 * @return bool 成功返回true，失败返回false
 */
bool add_metrics_to_history(Stru_OptimizationEvaluatorState_t* state,
                           const Stru_OptimizationMetrics_t* metrics)
{
    if (!state || !metrics)
    {
        return false;
    }
    
    // 检查是否需要扩展容量
    if (state->metrics_history_size >= state->metrics_history_capacity)
    {
        size_t new_capacity = state->metrics_history_capacity == 0 ? 16 : state->metrics_history_capacity * 2;
        Stru_OptimizationMetrics_t* new_history = 
            (Stru_OptimizationMetrics_t*)cn_realloc(state->metrics_history, 
                                                   new_capacity * sizeof(Stru_OptimizationMetrics_t));
        
        if (!new_history)
        {
            return false;
        }
        
        state->metrics_history = new_history;
        state->metrics_history_capacity = new_capacity;
    }
    
    // 添加指标到历史记录
    state->metrics_history[state->metrics_history_size] = *metrics;
    state->metrics_history_size++;
    
    return true;
}

// ============================================================================
// 基本信息函数实现
// ============================================================================

/**
 * @brief 获取评估器名称
 
 * 获取优化效果评估器的名称。
 * 
 * @return const char* 评估器名称
 */
const char* get_name(void)
{
    return "CN_Language优化效果评估器";
}

/**
 * @brief 获取评估器版本
 
 * 获取优化效果评估器的版本。
 * 
 * @return const char* 评估器版本
 */
const char* get_version(void)
{
    return F_get_optimization_evaluator_version_string();
}

// ============================================================================
// 资源管理函数实现
// ============================================================================

/**
 * @brief 销毁评估结果
 
 * 销毁优化评估结果。
 * 
 * @param result 评估结果
 */
void destroy_evaluation_result(Stru_OptimizationEvaluationResult_t* result)
{
    if (!result)
    {
        return;
    }
    
    if (result->evaluation_report)
    {
        cn_free(result->evaluation_report);
    }
    
    if (result->recommendations)
    {
        cn_free(result->recommendations);
    }
    
    cn_free(result);
}

/**
 * @brief 销毁接口
 
 * 销毁优化效果评估器接口。
 * 这是一个适配器函数，用于解决接口中destroy函数无参数的问题。
 */
void destroy_interface(void)
{
    if (g_current_evaluator)
    {
        F_destroy_optimization_evaluator_interface(g_current_evaluator);
    }
}
