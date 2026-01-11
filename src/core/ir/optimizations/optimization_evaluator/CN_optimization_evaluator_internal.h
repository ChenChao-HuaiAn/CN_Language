/******************************************************************************
 * 文件名: CN_optimization_evaluator_internal.h
 * 功能: CN_Language 优化效果评估器内部头文件
 
 * 定义优化效果评估器的内部数据结构和共享声明。
 * 这个头文件只用于模块内部使用，不对外暴露。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_OPTIMIZATION_EVALUATOR_INTERNAL_H
#define CN_OPTIMIZATION_EVALUATOR_INTERNAL_H

#include "../CN_optimization_evaluator.h"
#include <stddef.h>
#include <stdbool.h>

// ============================================================================
// 内部数据结构定义
// ============================================================================

/**
 * @brief 优化效果评估器内部状态结构体
 
 * 存储优化效果评估器的内部状态。
 */
struct Stru_OptimizationEvaluatorState_t {
    size_t total_evaluations;               ///< 总评估次数
    size_t successful_evaluations;          ///< 成功评估次数
    double total_evaluation_time;           ///< 总评估时间（秒）
    Stru_OptimizationMetrics_t* metrics_history; ///< 指标历史记录
    size_t metrics_history_size;            ///< 指标历史记录大小
    size_t metrics_history_capacity;        ///< 指标历史记录容量
};

// ============================================================================
// 全局变量声明
// ============================================================================

/**
 * @brief 当前活动的评估器实例
 
 * 用于解决接口中destroy函数无参数的问题。
 * 注意：这是一个简化实现，假设只有一个活动的评估器实例。
 * 在实际的多实例场景中，需要更复杂的实现。
 */
extern Stru_OptimizationEvaluatorInterface_t* g_current_evaluator;

// ============================================================================
// 内部函数声明
// ============================================================================

/**
 * @brief 添加指标到历史记录
 
 * 将优化指标添加到历史记录中。
 * 
 * @param state 内部状态
 * @param metrics 优化指标
 * @return bool 成功返回true，失败返回false
 */
bool add_metrics_to_history(Stru_OptimizationEvaluatorState_t* state,
                           const Stru_OptimizationMetrics_t* metrics);

/**
 * @brief 获取当前时间（秒）
 
 * 获取当前时间，以秒为单位。
 * 
 * @return double 当前时间（秒）
 */
double get_current_time_seconds(void);

#endif /* CN_OPTIMIZATION_EVALUATOR_INTERNAL_H */
