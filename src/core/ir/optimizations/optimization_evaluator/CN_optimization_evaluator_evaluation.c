/******************************************************************************
 * 文件名: CN_optimization_evaluator_evaluation.c
 * 功能: CN_Language 优化效果评估器效果评估模块
 
 * 实现优化效果评估器的效果评估函数。
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
#include "../../implementations/tac/CN_tac_interface.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// 函数原型声明
// ============================================================================

// 内部函数原型
Stru_OptimizationMetrics_t* calculate_optimization_metrics(
    void* original_ir,
    void* optimized_ir,
    Eum_IrOptimizationType opt_type);
size_t count_instructions(void* ir);
size_t estimate_code_size(void* ir);

// 外部函数原型（从其他模块）
extern char* generate_evaluation_report(const Stru_OptimizationEvaluationResult_t* result);

// ============================================================================
// 效果评估函数实现
// ============================================================================

/**
 * @brief 评估优化效果
 
 * 评估优化前后的效果，计算改进比例。
 * 
 * @param original_ir 原始IR
 * @param optimized_ir 优化后IR
 * @param opt_type 优化类型
 * @return Stru_OptimizationEvaluationResult_t* 评估结果，失败返回NULL
 */
Stru_OptimizationEvaluationResult_t* evaluate_optimization(
    void* original_ir,
    void* optimized_ir,
    Eum_IrOptimizationType opt_type)
{
    // 创建评估结果
    Stru_OptimizationEvaluationResult_t* result = F_create_optimization_evaluation_result();
    if (!result)
    {
        return NULL;
    }
    
    // 记录开始时间
    double start_time = get_current_time_seconds();
    
    // 检查输入参数
    if (!original_ir || !optimized_ir)
    {
        F_set_optimization_evaluation_result_error(result, "无效的IR输入");
        return result;
    }
    
    // 计算优化指标
    Stru_OptimizationMetrics_t* metrics = calculate_optimization_metrics(original_ir, optimized_ir, opt_type);
    if (!metrics)
    {
        F_set_optimization_evaluation_result_error(result, "无法计算优化指标");
        return result;
    }
    
    // 复制指标到结果
    result->metrics = *metrics;
    F_destroy_optimization_metrics(metrics);
    
    // 生成评估报告
    result->evaluation_report = generate_evaluation_report(result);
    
    // 生成优化建议
    result->recommendations = F_generate_optimization_recommendations(result);
    
    // 计算评估时间
    double end_time = get_current_time_seconds();
    result->evaluation_time = end_time - start_time;
    
    result->evaluation_successful = true;
    
    // 更新内部状态（通过全局变量）
    extern Stru_OptimizationEvaluatorInterface_t* g_current_evaluator;
    if (g_current_evaluator && g_current_evaluator->internal_state)
    {
        Stru_OptimizationEvaluatorState_t* state = (Stru_OptimizationEvaluatorState_t*)g_current_evaluator->internal_state;
        state->total_evaluations++;
        state->successful_evaluations++;
        state->total_evaluation_time += result->evaluation_time;
        add_metrics_to_history(state, &result->metrics);
    }
    
    return result;
}

/**
 * @brief 评估优化级别效果
 
 * 评估优化级别前后的效果。
 * 
 * @param original_ir 原始IR
 * @param optimized_ir 优化后IR
 * @param level 优化级别
 * @return Stru_OptimizationEvaluationResult_t* 评估结果，失败返回NULL
 */
Stru_OptimizationEvaluationResult_t* evaluate_optimization_level(
    void* original_ir,
    void* optimized_ir,
    Eum_IrOptimizationLevel level)
{
    // 简化实现：调用evaluate_optimization，使用默认优化类型
    return evaluate_optimization(original_ir, optimized_ir, Eum_IR_OPT_CONSTANT_FOLDING);
}

/**
 * @brief 计算优化指标
 
 * 计算优化前后的指标。
 * 
 * @param original_ir 原始IR
 * @param optimized_ir 优化后IR
 * @param opt_type 优化类型
 * @return Stru_OptimizationMetrics_t* 优化指标，失败返回NULL
 */
Stru_OptimizationMetrics_t* calculate_optimization_metrics(
    void* original_ir,
    void* optimized_ir,
    Eum_IrOptimizationType opt_type)
{
    Stru_OptimizationMetrics_t* metrics = F_create_optimization_metrics();
    if (!metrics)
    {
        return NULL;
    }
    
    // 计算指令数量
    size_t original_count = count_instructions(original_ir);
    size_t optimized_count = count_instructions(optimized_ir);
    
    // 计算指令减少比例
    metrics->instruction_reduction_ratio = F_calculate_instruction_reduction_ratio(original_count, optimized_count);
    
    // 估算代码大小减少比例
    size_t original_size = estimate_code_size(original_ir);
    size_t optimized_size = estimate_code_size(optimized_ir);
    
    if (original_size > 0)
    {
        metrics->code_size_reduction_ratio = (double)(original_size - optimized_size) / original_size;
    }
    
    // 估算性能改进
    metrics->estimated_performance_improvement = F_estimate_performance_improvement(metrics, opt_type);
    
    // 估算内存使用改进（简化实现）
    metrics->memory_usage_improvement = metrics->code_size_reduction_ratio * 0.8;
    
    // 估算编译时间开销（简化实现）
    metrics->compilation_time_overhead = 0.05; // 假设5%的开销
    
    // 计算优化质量评分
    metrics->optimization_quality_score = F_calculate_optimization_quality_score(metrics);
    
    return metrics;
}

/**
 * @brief 计算指令数量
 
 * 计算IR中的指令数量。
 * 
 * @param ir IR数据
 * @return size_t 指令数量
 */
size_t count_instructions(void* ir)
{
    if (!ir)
    {
        return 0;
    }
    
    // 检查IR类型，如果是TAC数据，使用TAC接口函数
    // 注意：这里假设ir是Stru_TacData_t*类型
    // 在实际使用中，可能需要更复杂的类型检查
    Stru_TacData_t* tac_data = (Stru_TacData_t*)ir;
    
    // 使用TAC数据获取指令数量
    return F_tac_data_get_instruction_count(tac_data);
}

/**
 * @brief 估算代码大小
 
 * 估算IR的代码大小。
 * 
 * @param ir IR数据
 * @return size_t 代码大小（字节）
 */
size_t estimate_code_size(void* ir)
{
    if (!ir)
    {
        return 0;
    }
    
    // 计算指令数量
    size_t instruction_count = count_instructions(ir);
    
    // 估算代码大小：假设每条指令平均8字节
    // 这包括操作码、操作数和标签等
    return instruction_count * 8;
}
