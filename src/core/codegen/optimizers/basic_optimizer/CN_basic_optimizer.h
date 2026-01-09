/**
 * @file CN_basic_optimizer.h
 * @brief CN_Language 基础优化器实现
 * 
 * 提供基础优化功能的实现，包括常量折叠、死代码消除等。
 * 遵循项目架构规范，实现优化器插件接口。
 * 
 * @author CN_Language开发团队
 * @date 2026-01-09
 * @version 1.0.0
 */

#ifndef CN_BASIC_OPTIMIZER_H
#define CN_BASIC_OPTIMIZER_H

#include "../../CN_optimizer_interface.h"

/**
 * @brief 创建基础优化器插件接口实例
 * 
 * 创建并返回一个新的基础优化器插件接口实例。
 * 调用者负责在不再使用时调用destroy函数。
 * 
 * @return Stru_OptimizerPluginInterface_t* 新创建的基础优化器插件接口实例
 */
Stru_OptimizerPluginInterface_t* F_create_basic_optimizer_interface(void);

/**
 * @brief 获取基础优化器版本信息
 * 
 * 返回基础优化器的版本信息。
 * 
 * @param major 输出参数，主版本号
 * @param minor 输出参数，次版本号
 * @param patch 输出参数，修订号
 */
void F_get_basic_optimizer_version(int* major, int* minor, int* patch);

/**
 * @brief 获取基础优化器版本字符串
 * 
 * 返回基础优化器的版本字符串。
 * 
 * @return 版本字符串
 */
const char* F_get_basic_optimizer_version_string(void);

/**
 * @brief 检查是否支持特定优化类型
 * 
 * 检查基础优化器是否支持特定的优化类型。
 * 
 * @param optimization_type 优化类型
 * @return bool 是否支持
 */
bool F_basic_optimizer_supports_optimization(Eum_OptimizationType optimization_type);

/**
 * @brief 获取基础优化器支持的优化类型
 * 
 * 获取基础优化器支持的所有优化类型。
 * 
 * @param count 输出参数，优化类型数量
 * @return Eum_OptimizationType* 优化类型数组，调用者不应修改
 */
const Eum_OptimizationType* F_get_basic_optimizer_supported_optimizations(size_t* count);

#endif // CN_BASIC_OPTIMIZER_H
