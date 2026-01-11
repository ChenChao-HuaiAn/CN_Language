/******************************************************************************
 * 文件名: CN_optimization_evaluator.h
 * 功能: CN_Language 优化效果评估器接口定义
 
 * 定义优化效果评估器接口，用于评估优化前后的效果，
 * 计算改进比例，生成优化报告。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_OPTIMIZATION_EVALUATOR_H
#define CN_OPTIMIZATION_EVALUATOR_H

#include "../CN_ir_optimizer.h"
#include <stddef.h>
#include <stdbool.h>

// ============================================================================
// 类型定义
// ============================================================================

/**
 * @brief 优化效果指标结构体
 * 
 * 存储优化效果的各项指标。
 */
typedef struct {
    double instruction_reduction_ratio;      ///< 指令减少比例
    double code_size_reduction_ratio;        ///< 代码大小减少比例
    double estimated_performance_improvement; ///< 估计性能改进
    double memory_usage_improvement;         ///< 内存使用改进
    double compilation_time_overhead;        ///< 编译时间开销
    double optimization_quality_score;       ///< 优化质量评分（0-100）
} Stru_OptimizationMetrics_t;

/**
 * @brief 优化评估结果结构体
 * 
 * 存储优化评估的完整结果。
 */
typedef struct {
    bool evaluation_successful;              ///< 评估是否成功
    Stru_OptimizationMetrics_t metrics;      ///< 优化指标
    char* evaluation_report;                 ///< 评估报告
    char* recommendations;                   ///< 优化建议
    double evaluation_time;                  ///< 评估时间（秒）
} Stru_OptimizationEvaluationResult_t;

/**
 * @brief 优化基准测试结构体
 * 
 * 存储优化基准测试的信息。
 */
typedef struct {
    const char* benchmark_name;              ///< 基准测试名称
    void* original_ir;                       ///< 原始IR
    void* optimized_ir;                      ///< 优化后IR
    size_t iteration_count;                  ///< 迭代次数
    double* execution_times;                 ///< 执行时间数组
    size_t time_count;                       ///< 时间数量
} Stru_OptimizationBenchmark_t;

/**
 * @brief 优化效果评估器内部状态结构体（不完整类型）
 * 
 * 存储优化效果评估器的内部状态。
 * 具体定义在实现文件中。
 */
typedef struct Stru_OptimizationEvaluatorState_t Stru_OptimizationEvaluatorState_t;

/**
 * @brief 优化效果评估器接口结构体
 * 
 * 定义优化效果评估器的接口。
 */
typedef struct Stru_OptimizationEvaluatorInterface_t {
    // 基本信息
    const char* (*get_name)(void);
    const char* (*get_version)(void);
    
    // 效果评估
    Stru_OptimizationEvaluationResult_t* (*evaluate_optimization)(
        void* original_ir,
        void* optimized_ir,
        Eum_IrOptimizationType opt_type);
    
    Stru_OptimizationEvaluationResult_t* (*evaluate_optimization_level)(
        void* original_ir,
        void* optimized_ir,
        Eum_IrOptimizationLevel level);
    
    // 基准测试
    bool (*run_benchmark)(Stru_OptimizationBenchmark_t* benchmark);
    Stru_OptimizationBenchmark_t* (*create_benchmark)(const char* name,
                                                     void* original_ir,
                                                     void* optimized_ir);
    void (*destroy_benchmark)(Stru_OptimizationBenchmark_t* benchmark);
    
    // 报告生成
    char* (*generate_evaluation_report)(const Stru_OptimizationEvaluationResult_t* result);
    bool (*save_report_to_file)(const Stru_OptimizationEvaluationResult_t* result,
                               const char* filename);
    
    // 统计分析
    bool (*analyze_optimization_trends)(Stru_OptimizationEvaluationResult_t** results,
                                       size_t count,
                                       Stru_OptimizationMetrics_t* trends);
    
    // 资源管理
    void (*destroy_evaluation_result)(Stru_OptimizationEvaluationResult_t* result);
    void (*destroy)(void);
    
    // 内部状态
    Stru_OptimizationEvaluatorState_t* internal_state;
} Stru_OptimizationEvaluatorInterface_t;

// ============================================================================
// 工厂函数声明
// ============================================================================

/**
 * @brief 创建优化效果评估器接口实例
 * 
 * 创建优化效果评估器接口实例。
 * 
 * @return Stru_OptimizationEvaluatorInterface_t* 评估器接口实例，失败返回NULL
 */
Stru_OptimizationEvaluatorInterface_t* F_create_optimization_evaluator_interface(void);

/**
 * @brief 销毁优化效果评估器接口实例
 * 
 * 销毁优化效果评估器接口实例及其所有资源。
 * 
 * @param evaluator 评估器接口实例
 */
void F_destroy_optimization_evaluator_interface(Stru_OptimizationEvaluatorInterface_t* evaluator);

// ============================================================================
// 工具函数声明
// ============================================================================

/**
 * @brief 创建优化指标
 * 
 * 创建新的优化指标结构体。
 * 
 * @return Stru_OptimizationMetrics_t* 新创建的优化指标，失败返回NULL
 */
Stru_OptimizationMetrics_t* F_create_optimization_metrics(void);

/**
 * @brief 销毁优化指标
 * 
 * 销毁优化指标结构体。
 * 
 * @param metrics 优化指标
 */
void F_destroy_optimization_metrics(Stru_OptimizationMetrics_t* metrics);

/**
 * @brief 创建优化评估结果
 * 
 * 创建新的优化评估结果。
 * 
 * @return Stru_OptimizationEvaluationResult_t* 新创建的评估结果，失败返回NULL
 */
Stru_OptimizationEvaluationResult_t* F_create_optimization_evaluation_result(void);

/**
 * @brief 设置评估结果错误
 * 
 * 设置评估结果的错误信息。
 * 
 * @param result 评估结果
 * @param error_message 错误信息
 */
void F_set_optimization_evaluation_result_error(Stru_OptimizationEvaluationResult_t* result,
                                               const char* error_message);

/**
 * @brief 计算指令减少比例
 * 
 * 计算优化后指令减少的比例。
 * 
 * @param original_count 原始指令数量
 * @param optimized_count 优化后指令数量
 * @return double 减少比例（0.0-1.0）
 */
double F_calculate_instruction_reduction_ratio(size_t original_count,
                                              size_t optimized_count);

/**
 * @brief 估算性能改进
 * 
 * 根据优化类型和指标估算性能改进。
 * 
 * @param metrics 优化指标
 * @param opt_type 优化类型
 * @return double 估计性能改进比例（0.0-1.0）
 */
double F_estimate_performance_improvement(const Stru_OptimizationMetrics_t* metrics,
                                         Eum_IrOptimizationType opt_type);

/**
 * @brief 计算优化质量评分
 * 
 * 计算优化的质量评分。
 * 
 * @param metrics 优化指标
 * @return double 质量评分（0-100）
 */
double F_calculate_optimization_quality_score(const Stru_OptimizationMetrics_t* metrics);

/**
 * @brief 生成优化建议
 * 
 * 根据评估结果生成优化建议。
 * 
 * @param result 评估结果
 * @return char* 优化建议字符串，需要调用者释放
 */
char* F_generate_optimization_recommendations(const Stru_OptimizationEvaluationResult_t* result);

// ============================================================================
// 版本信息函数
// ============================================================================

/**
 * @brief 获取优化评估器版本信息
 * 
 * 获取优化评估器的版本信息。
 * 
 * @param major 输出参数，主版本号
 * @param minor 输出参数，次版本号
 * @param patch 输出参数，修订号
 */
void F_get_optimization_evaluator_version(int* major, int* minor, int* patch);

/**
 * @brief 获取优化评估器版本字符串
 * 
 * 获取优化评估器的版本字符串。
 * 
 * @return const char* 版本字符串
 */
const char* F_get_optimization_evaluator_version_string(void);

#endif /* CN_OPTIMIZATION_EVALUATOR_H */
