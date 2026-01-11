/******************************************************************************
 * 文件名: CN_optimization_evaluator_utils.c
 * 功能: CN_Language 优化效果评估器工具函数模块
 
 * 实现优化效果评估器的工具函数。
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
#include <math.h>
#include <stdio.h>
#include <time.h>

// ============================================================================
// 工具函数实现
// ============================================================================

/**
 * @brief 创建优化指标
 
 * 创建新的优化指标结构体。
 * 
 * @return Stru_OptimizationMetrics_t* 新创建的优化指标，失败返回NULL
 */
Stru_OptimizationMetrics_t* F_create_optimization_metrics(void)
{
    Stru_OptimizationMetrics_t* metrics = 
        (Stru_OptimizationMetrics_t*)cn_malloc(sizeof(Stru_OptimizationMetrics_t));
    
    if (!metrics)
    {
        return NULL;
    }
    
    // 初始化所有字段为0
    memset(metrics, 0, sizeof(Stru_OptimizationMetrics_t));
    
    return metrics;
}

/**
 * @brief 销毁优化指标
 
 * 销毁优化指标结构体。
 * 
 * @param metrics 优化指标
 */
void F_destroy_optimization_metrics(Stru_OptimizationMetrics_t* metrics)
{
    if (!metrics)
    {
        return;
    }
    
    cn_free(metrics);
}

/**
 * @brief 创建优化评估结果
 
 * 创建新的优化评估结果。
 * 
 * @return Stru_OptimizationEvaluationResult_t* 新创建的评估结果，失败返回NULL
 */
Stru_OptimizationEvaluationResult_t* F_create_optimization_evaluation_result(void)
{
    Stru_OptimizationEvaluationResult_t* result = 
        (Stru_OptimizationEvaluationResult_t*)cn_malloc(sizeof(Stru_OptimizationEvaluationResult_t));
    
    if (!result)
    {
        return NULL;
    }
    
    // 初始化所有字段
    result->evaluation_successful = false;
    memset(&result->metrics, 0, sizeof(Stru_OptimizationMetrics_t));
    result->evaluation_report = NULL;
    result->recommendations = NULL;
    result->evaluation_time = 0.0;
    
    return result;
}

/**
 * @brief 设置评估结果错误
 
 * 设置评估结果的错误信息。
 * 
 * @param result 评估结果
 * @param error_message 错误信息
 */
void F_set_optimization_evaluation_result_error(Stru_OptimizationEvaluationResult_t* result,
                                               const char* error_message)
{
    if (!result || !error_message)
    {
        return;
    }
    
    // 如果已有报告，先释放
    if (result->evaluation_report)
    {
        cn_free(result->evaluation_report);
        result->evaluation_report = NULL;
    }
    
    // 创建错误报告
    size_t error_len = strlen(error_message) + 50;
    result->evaluation_report = (char*)cn_malloc(error_len);
    if (result->evaluation_report)
    {
        snprintf(result->evaluation_report, error_len, 
                "优化评估失败: %s", error_message);
    }
    
    result->evaluation_successful = false;
}

/**
 * @brief 计算指令减少比例
 
 * 计算优化后指令减少的比例。
 * 
 * @param original_count 原始指令数量
 * @param optimized_count 优化后指令数量
 * @return double 减少比例（0.0-1.0）
 */
double F_calculate_instruction_reduction_ratio(size_t original_count,
                                              size_t optimized_count)
{
    if (original_count == 0)
    {
        return 0.0;
    }
    
    if (optimized_count > original_count)
    {
        return 0.0; // 优化后指令更多，没有减少
    }
    
    return (double)(original_count - optimized_count) / original_count;
}

/**
 * @brief 估算性能改进
 
 * 根据优化类型和指标估算性能改进。
 * 
 * @param metrics 优化指标
 * @param opt_type 优化类型
 * @return double 估计性能改进比例（0.0-1.0）
 */
double F_estimate_performance_improvement(const Stru_OptimizationMetrics_t* metrics,
                                         Eum_IrOptimizationType opt_type)
{
    if (!metrics)
    {
        return 0.0;
    }
    
    // 根据优化类型计算性能改进
    double base_improvement = metrics->instruction_reduction_ratio * 0.7;
    
    switch (opt_type)
    {
        case Eum_IR_OPT_CONSTANT_FOLDING:
            return base_improvement * 1.2;
        case Eum_IR_OPT_DEAD_CODE_ELIMINATION:
            return base_improvement * 1.5;
        case Eum_IR_OPT_COMMON_SUBEXPRESSION:
            return base_improvement * 1.3;
        case Eum_IR_OPT_STRENGTH_REDUCTION:
            return base_improvement * 1.1;
        case Eum_IR_OPT_PEEPHOLE:
            return base_improvement * 0.9;
        default:
            return base_improvement;
    }
}

/**
 * @brief 计算优化质量评分
 
 * 计算优化的质量评分。
 * 
 * @param metrics 优化指标
 * @return double 质量评分（0-100）
 */
double F_calculate_optimization_quality_score(const Stru_OptimizationMetrics_t* metrics)
{
    if (!metrics)
    {
        return 0.0;
    }
    
    // 计算质量评分
    double score = 0.0;
    
    // 指令减少比例贡献（最大40分）
    score += metrics->instruction_reduction_ratio * 40;
    
    // 代码大小减少比例贡献（最大20分）
    score += metrics->code_size_reduction_ratio * 20;
    
    // 性能改进贡献（最大30分）
    score += metrics->estimated_performance_improvement * 30;
    
    // 内存使用改进贡献（最大10分）
    score += metrics->memory_usage_improvement * 10;
    
    // 编译时间开销惩罚（最大-20分）
    score -= metrics->compilation_time_overhead * 20;
    
    // 确保分数在0-100范围内
    if (score < 0.0) score = 0.0;
    if (score > 100.0) score = 100.0;
    
    return score;
}

/**
 * @brief 生成优化建议
 
 * 根据评估结果生成优化建议。
 * 
 * @param result 评估结果
 * @return char* 优化建议字符串，需要调用者释放
 */
char* F_generate_optimization_recommendations(const Stru_OptimizationEvaluationResult_t* result)
{
    if (!result)
    {
        return NULL;
    }
    
    const Stru_OptimizationMetrics_t* metrics = &result->metrics;
    
    // 根据评分生成建议
    char* recommendations = NULL;
    
    if (metrics->optimization_quality_score >= 80.0)
    {
        recommendations = strdup("优化效果优秀，建议保持当前优化策略。");
    }
    else if (metrics->optimization_quality_score >= 60.0)
    {
        recommendations = strdup("优化效果良好，可以考虑进一步优化。");
    }
    else if (metrics->optimization_quality_score >= 40.0)
    {
        recommendations = strdup("优化效果一般，建议重新评估优化策略。");
    }
    else
    {
        recommendations = strdup("优化效果较差，建议调整优化参数或选择其他优化方法。");
    }
    
    return recommendations;
}

// ============================================================================
// 版本信息函数实现
// ============================================================================

/**
 * @brief 获取优化评估器版本信息
 
 * 获取优化评估器的版本信息。
 * 
 * @param major 输出参数，主版本号
 * @param minor 输出参数，次版本号
 * @param patch 输出参数，修订号
 */
void F_get_optimization_evaluator_version(int* major, int* minor, int* patch)
{
    if (major) *major = 1;
    if (minor) *minor = 0;
    if (patch) *patch = 0;
}

/**
 * @brief 获取优化评估器版本字符串
 
 * 获取优化评估器的版本字符串。
 * 
 * @return const char* 版本字符串
 */
const char* F_get_optimization_evaluator_version_string(void)
{
    return "1.0.0";
}

// ============================================================================
// 内部工具函数
// ============================================================================

/**
 * @brief 获取当前时间（秒）
 
 * 获取当前时间，精度为秒。
 * 
 * @return double 当前时间（秒）
 */
double get_current_time_seconds(void)
{
    return (double)clock() / CLOCKS_PER_SEC;
}
