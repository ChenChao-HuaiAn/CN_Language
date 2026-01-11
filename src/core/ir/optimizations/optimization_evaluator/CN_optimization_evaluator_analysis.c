/******************************************************************************
 * 文件名: CN_optimization_evaluator_analysis.c
 * 功能: CN_Language 优化效果评估器统计分析模块
 
 * 实现优化效果评估器的统计分析函数。
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
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 统计分析函数实现
// ============================================================================

/**
 * @brief 分析优化趋势
 
 * 分析多个优化评估结果的趋势。
 * 
 * @param results 评估结果数组
 * @param count 结果数量
 * @param trends 输出趋势指标
 * @return bool 成功返回true，失败返回false
 */
bool analyze_optimization_trends(Stru_OptimizationEvaluationResult_t** results,
                                size_t count,
                                Stru_OptimizationMetrics_t* trends)
{
    if (!results || count == 0 || !trends)
    {
        return false;
    }
    
    // 初始化趋势指标
    memset(trends, 0, sizeof(Stru_OptimizationMetrics_t));
    
    // 计算平均值
    for (size_t i = 0; i < count; i++)
    {
        if (results[i])
        {
            trends->instruction_reduction_ratio += results[i]->metrics.instruction_reduction_ratio;
            trends->code_size_reduction_ratio += results[i]->metrics.code_size_reduction_ratio;
            trends->estimated_performance_improvement += results[i]->metrics.estimated_performance_improvement;
            trends->memory_usage_improvement += results[i]->metrics.memory_usage_improvement;
            trends->compilation_time_overhead += results[i]->metrics.compilation_time_overhead;
            trends->optimization_quality_score += results[i]->metrics.optimization_quality_score;
        }
    }
    
    // 计算平均值
    trends->instruction_reduction_ratio /= count;
    trends->code_size_reduction_ratio /= count;
    trends->estimated_performance_improvement /= count;
    trends->memory_usage_improvement /= count;
    trends->compilation_time_overhead /= count;
    trends->optimization_quality_score /= count;
    
    return true;
}
