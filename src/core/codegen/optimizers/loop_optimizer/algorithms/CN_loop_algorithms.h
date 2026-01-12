/******************************************************************************
 * 文件名: CN_loop_algorithms.h
 * 功能: CN_Language 循环优化算法模块
 * 
 * 提供循环优化算法的实现，包括循环不变代码外提、循环展开、
 * 循环融合、循环向量化等优化算法。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_LOOP_ALGORITHMS_H
#define CN_LOOP_ALGORITHMS_H

#include "../../CN_optimizer_interface.h"
#include "../CN_loop_optimizer.h"

/* ============================================
 * 循环不变代码外提算法
 * ============================================ */

/**
 * @brief 应用循环不变代码外提算法实现
 * 
 * 将循环中不变的计算移到循环外部。
 * 
 * @param ast AST根节点
 * @param loop_info 循环分析信息
 * @return bool 优化是否成功应用
 */
bool F_apply_loop_invariant_code_motion_impl(Stru_AstNode_t* ast, Stru_LoopAnalysisInfo_t* loop_info);

/**
 * @brief 识别循环不变表达式
 * 
 * 识别循环中的不变表达式。
 * 
 * @param loop_info 循环分析信息
 * @param ast 循环AST节点
 * @return Stru_DynamicArray_t* 不变表达式数组
 */
Stru_DynamicArray_t* F_identify_invariant_expressions(Stru_LoopAnalysisInfo_t* loop_info, Stru_AstNode_t* ast);

/**
 * @brief 外提循环不变表达式
 * 
 * 将循环不变表达式移到循环外部。
 * 
 * @param ast 循环AST节点
 * @param invariant_expressions 不变表达式数组
 * @return bool 外提是否成功
 */
bool F_hoist_invariant_expressions(Stru_AstNode_t* ast, Stru_DynamicArray_t* invariant_expressions);

/* ============================================
 * 循环展开算法
 * ============================================ */

/**
 * @brief 应用循环展开算法实现
 * 
 * 将循环体复制多次，减少循环控制开销。
 * 
 * @param ast AST根节点
 * @param loop_info 循环分析信息
 * @param unroll_factor 展开因子
 * @return bool 优化是否成功应用
 */
bool F_apply_loop_unrolling_impl(Stru_AstNode_t* ast, Stru_LoopAnalysisInfo_t* loop_info, size_t unroll_factor);

/**
 * @brief 应用循环展开优化（接口适配）
 * 
 * 适配优化器接口的循环展开优化。
 * 
 * @param ast AST根节点
 * @param loop_infos 循环分析信息数组
 * @param config 循环优化配置
 * @return bool 优化是否成功应用
 */
bool F_apply_loop_unrolling_optimization(Stru_AstNode_t* ast, Stru_DynamicArray_t* loop_infos, Stru_LoopOptimizationConfig_t* config);

/**
 * @brief 检查循环是否可展开
 * 
 * 检查循环是否满足展开条件。
 * 
 * @param loop_info 循环分析信息
 * @param config 循环优化配置
 * @return bool 是否可展开
 */
bool F_can_unroll_loop(Stru_LoopAnalysisInfo_t* loop_info, Stru_LoopOptimizationConfig_t* config);

/**
 * @brief 计算最佳展开因子
 * 
 * 根据循环特性和配置计算最佳展开因子。
 * 
 * @param loop_info 循环分析信息
 * @param config 循环优化配置
 * @return size_t 最佳展开因子
 */
size_t F_calculate_optimal_unroll_factor(Stru_LoopAnalysisInfo_t* loop_info, Stru_LoopOptimizationConfig_t* config);

/* ============================================
 * 循环融合算法
 * ============================================ */

/**
 * @brief 应用循环融合算法实现
 * 
 * 将多个相邻的循环合并为一个循环。
 * 
 * @param ast AST根节点
 * @param loop_infos 循环分析信息数组
 * @param loop_count 循环数量
 * @return bool 优化是否成功应用
 */
bool F_apply_loop_fusion_impl(Stru_AstNode_t* ast, Stru_LoopAnalysisInfo_t** loop_infos, size_t loop_count);

/**
 * @brief 检查循环是否可融合
 * 
 * 检查多个循环是否满足融合条件。
 * 
 * @param loop_infos 循环分析信息数组
 * @param loop_count 循环数量
 * @return bool 是否可融合
 */
bool F_can_fuse_loops(Stru_LoopAnalysisInfo_t** loop_infos, size_t loop_count);

/**
 * @brief 融合循环体
 * 
 * 将多个循环体融合为一个循环体。
 * 
 * @param loop_asts 循环AST节点数组
 * @param loop_count 循环数量
 * @return Stru_AstNode_t* 融合后的循环AST节点
 */
Stru_AstNode_t* F_fuse_loop_bodies(Stru_AstNode_t** loop_asts, size_t loop_count);

/* ============================================
 * 循环向量化算法
 * ============================================ */

/**
 * @brief 应用循环向量化算法实现
 * 
 * 将循环转换为向量化操作。
 * 
 * @param ast AST根节点
 * @param loop_info 循环分析信息
 * @param vector_width 向量宽度
 * @return bool 优化是否成功应用
 */
bool F_apply_loop_vectorization_impl(Stru_AstNode_t* ast, Stru_LoopAnalysisInfo_t* loop_info, size_t vector_width);

/**
 * @brief 应用循环向量化优化（接口适配）
 * 
 * 适配优化器接口的循环向量化优化。
 * 
 * @param ast AST根节点
 * @param loop_infos 循环分析信息数组
 * @param config 循环优化配置
 * @return bool 优化是否成功应用
 */
bool F_apply_loop_vectorization_optimization(Stru_AstNode_t* ast, Stru_DynamicArray_t* loop_infos, Stru_LoopOptimizationConfig_t* config);

/**
 * @brief 检查循环是否可向量化
 * 
 * 检查循环是否满足向量化条件。
 * 
 * @param loop_info 循环分析信息
 * @param config 循环优化配置
 * @return bool 是否可向量化
 */
bool F_can_vectorize_loop(Stru_LoopAnalysisInfo_t* loop_info, Stru_LoopOptimizationConfig_t* config);

/**
 * @brief 计算最佳向量宽度
 * 
 * 根据循环特性和配置计算最佳向量宽度。
 * 
 * @param loop_info 循环分析信息
 * @param config 循环优化配置
 * @return size_t 最佳向量宽度
 */
size_t F_calculate_optimal_vector_width(Stru_LoopAnalysisInfo_t* loop_info, Stru_LoopOptimizationConfig_t* config);

/* ============================================
 * 循环并行化算法
 * ============================================ */

/**
 * @brief 应用循环并行化优化（接口适配）
 * 
 * 适配优化器接口的循环并行化优化。
 * 
 * @param ast AST根节点
 * @param loop_infos 循环分析信息数组
 * @param config 循环优化配置
 * @return bool 优化是否成功应用
 */
bool F_apply_loop_parallelization_optimization(Stru_AstNode_t* ast, Stru_DynamicArray_t* loop_infos, Stru_LoopOptimizationConfig_t* config);

/**
 * @brief 检查循环是否可并行化
 * 
 * 检查循环是否满足并行化条件。
 * 
 * @param loop_info 循环分析信息
 * @param config 循环优化配置
 * @return bool 是否可并行化
 */
bool F_can_parallelize_loop(Stru_LoopAnalysisInfo_t* loop_info, Stru_LoopOptimizationConfig_t* config);

/* ============================================
 * 内存访问优化算法
 * ============================================ */

/**
 * @brief 应用内存访问优化（接口适配）
 * 
 * 适配优化器接口的内存访问优化。
 * 
 * @param ast AST根节点
 * @param loop_infos 循环分析信息数组
 * @param config 循环优化配置
 * @return bool 优化是否成功应用
 */
bool F_apply_memory_access_optimization(Stru_AstNode_t* ast, Stru_DynamicArray_t* loop_infos, Stru_LoopOptimizationConfig_t* config);

/**
 * @brief 分析内存访问模式
 * 
 * 分析循环中的内存访问模式。
 * 
 * @param loop_info 循环分析信息
 * @param ast 循环AST节点
 * @return Stru_DynamicArray_t* 内存访问模式数组
 */
Stru_DynamicArray_t* F_analyze_memory_access_patterns(Stru_LoopAnalysisInfo_t* loop_info, Stru_AstNode_t* ast);

/**
 * @brief 优化内存访问
 * 
 * 优化循环中的内存访问。
 * 
 * @param ast 循环AST节点
 * @param memory_patterns 内存访问模式数组
 * @return bool 优化是否成功
 */
bool F_optimize_memory_access(Stru_AstNode_t* ast, Stru_DynamicArray_t* memory_patterns);

#endif /* CN_LOOP_ALGORITHMS_H */
