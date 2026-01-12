/******************************************************************************
 * 文件名: CN_loop_utils.h
 * 功能: CN_Language 循环优化器工具模块
 * 
 * 提供循环优化器的工具函数，包括循环分析辅助函数、数学计算、
 * 字符串转换、内存管理等。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_LOOP_UTILS_H
#define CN_LOOP_UTILS_H

#include "../../CN_optimizer_interface.h"
#include "../CN_loop_optimizer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 计算平均循环深度
 * 
 * @param loop_infos 循环信息数组
 * @return double 平均循环深度，失败返回0.0
 */
double F_calculate_average_loop_depth(Stru_DynamicArray_t* loop_infos);

/**
 * @brief 计算可优化循环比例
 * 
 * @param loop_infos 循环信息数组
 * @return double 可优化循环比例 (0.0-100.0)，失败返回0.0
 */
double F_calculate_optimizable_loop_ratio(Stru_DynamicArray_t* loop_infos);

/**
 * @brief 检查向量化安全性
 * 
 * @param ast AST根节点
 * @param loop_infos 循环信息数组
 * @return bool 安全返回true，不安全返回false
 */
bool F_check_vectorization_safety(Stru_AstNode_t* ast, Stru_DynamicArray_t* loop_infos);

/**
 * @brief 检查并行化安全性
 * 
 * @param ast AST根节点
 * @param loop_infos 循环信息数组
 * @return bool 安全返回true，不安全返回false
 */
bool F_check_parallelization_safety(Stru_AstNode_t* ast, Stru_DynamicArray_t* loop_infos);

/**
 * @brief 计算循环迭代次数估计
 * 
 * @param loop_info 循环信息
 * @return size_t 估计的迭代次数，无法估计返回0
 */
size_t F_estimate_loop_iterations(Stru_LoopAnalysisInfo_t* loop_info);

/**
 * @brief 检查循环是否可展开
 * 
 * @param loop_info 循环信息
 * @param max_unroll_factor 最大展开因子
 * @return bool 可展开返回true，不可展开返回false
 */
bool F_can_loop_be_unrolled(Stru_LoopAnalysisInfo_t* loop_info, size_t max_unroll_factor);

/**
 * @brief 检查循环是否可向量化
 * 
 * @param loop_info 循环信息
 * @param vector_width 向量宽度
 * @return bool 可向量化返回true，不可向量化返回false
 */
bool F_can_loop_be_vectorized(Stru_LoopAnalysisInfo_t* loop_info, size_t vector_width);

/**
 * @brief 检查循环是否可并行化
 * 
 * @param loop_info 循环信息
 * @return bool 可并行化返回true，不可并行化返回false
 */
bool F_can_loop_be_parallelized(Stru_LoopAnalysisInfo_t* loop_info);

/**
 * @brief 计算循环优化收益估计
 * 
 * @param loop_info 循环信息
 * @param optimization_type 优化类型
 * @return double 估计的性能提升比例 (0.0-1.0)
 */
double F_estimate_loop_optimization_gain(Stru_LoopAnalysisInfo_t* loop_info, Eum_LoopOptimizationType optimization_type);

/**
 * @brief 创建循环优化报告
 * 
 * @param loop_infos 循环信息数组
 * @param config 优化配置
 * @return char* 优化报告字符串，调用者负责释放
 */
char* F_create_loop_optimization_report(Stru_DynamicArray_t* loop_infos, Stru_LoopOptimizationConfig_t* config);

/**
 * @brief 验证循环优化结果
 * 
 * @param original_ast 原始AST
 * @param optimized_ast 优化后AST
 * @param loop_info 循环信息
 * @return bool 验证通过返回true，失败返回false
 */
bool F_validate_loop_optimization_result(Stru_AstNode_t* original_ast, Stru_AstNode_t* optimized_ast, Stru_LoopAnalysisInfo_t* loop_info);

/**
 * @brief 计算循环复杂度
 * 
 * @param loop_info 循环信息
 * @return int 循环复杂度分数
 */
int F_calculate_loop_complexity(Stru_LoopAnalysisInfo_t* loop_info);

#ifdef __cplusplus
}
#endif

#endif /* CN_LOOP_UTILS_H */
