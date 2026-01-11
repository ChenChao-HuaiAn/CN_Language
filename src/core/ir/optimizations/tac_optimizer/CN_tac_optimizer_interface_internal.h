/******************************************************************************
 * 文件名: CN_tac_optimizer_interface_internal.h
 * 功能: CN_Language 三地址码优化器接口内部头文件
 
 * 定义TAC优化器接口的内部函数和数据结构。
 * 仅供TAC优化器接口模块内部使用。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_TAC_OPTIMIZER_INTERFACE_INTERNAL_H
#define CN_TAC_OPTIMIZER_INTERFACE_INTERNAL_H

#include "../CN_tac_optimizer.h"
#include "../../../../infrastructure/memory/utilities/CN_memory_utilities.h"
#include "../../../../infrastructure/containers/string/CN_string.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

// ============================================================================
// 内部函数声明
// ============================================================================

/**
 * @brief 创建默认优化结果
 * 
 * 创建并初始化一个默认的优化结果结构体。
 * 
 * @return Stru_IrOptimizationResult_t* 优化结果
 */
Stru_IrOptimizationResult_t* F_create_default_optimization_result(void);

/**
 * @brief 复制TAC数据
 * 
 * 复制TAC数据到新的实例。
 * 
 * @param tac_data 原始TAC数据
 * @return Stru_TacData_t* 复制的TAC数据
 */
Stru_TacData_t* F_copy_tac_data(const Stru_TacData_t* tac_data);

/**
 * @brief 应用优化到TAC数据
 * 
 * 根据配置应用优化到TAC数据。
 * 
 * @param tac_data TAC数据
 * @param config 优化配置
 * @param optimized_tac 输出参数，优化后的TAC数据
 * @return bool 成功返回true，失败返回false
 */
bool F_apply_optimizations_to_tac(Stru_TacData_t* tac_data,
                                 const Stru_IrOptimizationConfig_t* config,
                                 Stru_TacData_t** optimized_tac);

/**
 * @brief 计算优化改进比例
 * 
 * 计算优化前后的改进比例。
 * 
 * @param original_count 原始指令数
 * @param optimized_count 优化后指令数
 * @param removed_instructions 移除的指令数
 * @param added_instructions 添加的指令数
 * @return double 改进比例
 */
double F_calculate_improvement_ratio(size_t original_count,
                                    size_t optimized_count,
                                    size_t removed_instructions,
                                    size_t added_instructions);

/**
 * @brief 处理优化结果
 * 
 * 处理优化结果，释放中间资源。
 * 
 * @param result 优化结果
 * @param optimized_tac 优化后的TAC数据
 */
void F_handle_optimization_result(Stru_IrOptimizationResult_t* result,
                                 Stru_TacData_t* optimized_tac);

#endif /* CN_TAC_OPTIMIZER_INTERFACE_INTERNAL_H */
