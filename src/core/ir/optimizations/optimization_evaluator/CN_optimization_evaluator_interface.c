/******************************************************************************
 * 文件名: CN_optimization_evaluator_interface.c
 * 功能: CN_Language 优化效果评估器接口模块
 
 * 实现优化效果评估器的接口函数，连接各个模块。
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

// ============================================================================
// 外部函数声明
// ============================================================================

// 从主模块导入
extern const char* get_name(void);
extern const char* get_version(void);
extern void destroy_evaluation_result(Stru_OptimizationEvaluationResult_t* result);
extern void destroy_interface(void);

// 从评估模块导入
extern Stru_OptimizationEvaluationResult_t* evaluate_optimization(
    void* original_ir,
    void* optimized_ir,
    Eum_IrOptimizationType opt_type);
extern Stru_OptimizationEvaluationResult_t* evaluate_optimization_level(
    void* original_ir,
    void* optimized_ir,
    Eum_IrOptimizationLevel level);

// 从基准测试模块导入
extern bool run_benchmark(Stru_OptimizationBenchmark_t* benchmark);
extern Stru_OptimizationBenchmark_t* create_benchmark(const char* name,
                                                     void* original_ir,
                                                     void* optimized_ir);
extern void destroy_benchmark(Stru_OptimizationBenchmark_t* benchmark);

// 从报告模块导入
extern char* generate_evaluation_report(const Stru_OptimizationEvaluationResult_t* result);
extern bool save_report_to_file(const Stru_OptimizationEvaluationResult_t* result,
                               const char* filename);

// 从分析模块导入
extern bool analyze_optimization_trends(Stru_OptimizationEvaluationResult_t** results,
                                       size_t count,
                                       Stru_OptimizationMetrics_t* trends);

// ============================================================================
// 接口函数实现（这些函数只是转发到实际实现）
// ============================================================================

// 注意：这些函数已经在各个模块中实现，这里只是确保它们可以被接口访问。
// 实际上，接口结构体中的函数指针直接指向这些函数。

// 这个文件的主要目的是确保所有需要的函数都有声明，
// 并且可以被其他模块正确链接。
