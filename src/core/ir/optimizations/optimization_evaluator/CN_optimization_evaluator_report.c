/******************************************************************************
 * 文件名: CN_optimization_evaluator_report.c
 * 功能: CN_Language 优化效果评估器报告生成模块
 
 * 实现优化效果评估器的报告生成函数。
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
#include "../../../../infrastructure/memory/utilities/CN_memory_utilities.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// 函数原型声明
// ============================================================================

// 内部函数原型
char* format_metrics_report(const Stru_OptimizationMetrics_t* metrics);

// ============================================================================
// 报告生成函数实现
// ============================================================================

/**
 * @brief 生成评估报告
 
 * 生成优化评估报告。
 * 
 * @param result 评估结果
 * @return char* 评估报告字符串，需要调用者释放
 */
char* generate_evaluation_report(const Stru_OptimizationEvaluationResult_t* result)
{
    if (!result)
    {
        return NULL;
    }
    
    // 生成报告
    char* report = format_metrics_report(&result->metrics);
    if (!report)
    {
        return NULL;
    }
    
    // 添加评估信息
    size_t report_len = strlen(report) + 200;
    char* full_report = (char*)cn_malloc(report_len);
    if (!full_report)
    {
        cn_free(report);
        return NULL;
    }
    
    snprintf(full_report, report_len,
             "优化评估报告\n"
             "============\n"
             "评估状态: %s\n"
             "评估时间: %.3f秒\n"
             "\n%s",
             result->evaluation_successful ? "成功" : "失败",
             result->evaluation_time,
             report);
    
    cn_free(report);
    return full_report;
}

/**
 * @brief 保存报告到文件
 
 * 将评估报告保存到文件。
 * 
 * @param result 评估结果
 * @param filename 文件名
 * @return bool 成功返回true，失败返回false
 */
bool save_report_to_file(const Stru_OptimizationEvaluationResult_t* result,
                        const char* filename)
{
    if (!result || !filename)
    {
        return false;
    }
    
    // 生成报告
    char* report = generate_evaluation_report(result);
    if (!report)
    {
        return false;
    }
    
    // 保存到文件
    FILE* file = fopen(filename, "w");
    if (!file)
    {
        cn_free(report);
        return false;
    }
    
    fprintf(file, "%s", report);
    fclose(file);
    cn_free(report);
    
    return true;
}

/**
 * @brief 格式化指标报告
 
 * 格式化优化指标报告。
 * 
 * @param metrics 优化指标
 * @return char* 格式化报告字符串，需要调用者释放
 */
char* format_metrics_report(const Stru_OptimizationMetrics_t* metrics)
{
    if (!metrics)
    {
        return NULL;
    }
    
    size_t buffer_size = 512;
    char* report = (char*)cn_malloc(buffer_size);
    if (!report)
    {
        return NULL;
    }
    
    snprintf(report, buffer_size,
             "优化指标:\n"
             "  指令减少比例: %.2f%%\n"
             "  代码大小减少比例: %.2f%%\n"
             "  估计性能改进: %.2f%%\n"
             "  内存使用改进: %.2f%%\n"
             "  编译时间开销: %.2f%%\n"
             "  优化质量评分: %.1f/100\n",
             metrics->instruction_reduction_ratio * 100,
             metrics->code_size_reduction_ratio * 100,
             metrics->estimated_performance_improvement * 100,
             metrics->memory_usage_improvement * 100,
             metrics->compilation_time_overhead * 100,
             metrics->optimization_quality_score);
    
    return report;
}
