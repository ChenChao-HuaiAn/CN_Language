/**
 * @file CN_stack.h
 * @brief 栈模块主头文件
 * 
 * 提供栈数据结构的完整接口声明。
 * 这是栈模块的主要入口点，包含所有子模块的功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_STACK_H
#define CN_STACK_H

#include <stddef.h>
#include <stdbool.h>

/* 包含所有子模块的头文件 */
#include "stack_core/CN_stack_core.h"
#include "stack_iterator/CN_stack_iterator.h"
#include "stack_utils/CN_stack_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 栈模块版本信息
 * 
 * 提供栈模块的版本信息，便于版本管理和兼容性检查。
 */
#define CN_STACK_VERSION_MAJOR 2
#define CN_STACK_VERSION_MINOR 0
#define CN_STACK_VERSION_PATCH 0
#define CN_STACK_VERSION_STRING "2.0.0"

/**
 * @brief 获取栈模块版本
 * 
 * 返回栈模块的版本信息。
 * 
 * @param major 输出参数，主版本号
 * @param minor 输出参数，次版本号
 * @param patch 输出参数，修订号
 */
static inline void F_stack_get_version(int* major, int* minor, int* patch)
{
    if (major != NULL) *major = CN_STACK_VERSION_MAJOR;
    if (minor != NULL) *minor = CN_STACK_VERSION_MINOR;
    if (patch != NULL) *patch = CN_STACK_VERSION_PATCH;
}

/**
 * @brief 获取栈模块版本字符串
 * 
 * 返回栈模块的版本字符串。
 * 
 * @return 版本字符串
 */
static inline const char* F_stack_get_version_string(void)
{
    return CN_STACK_VERSION_STRING;
}

/**
 * @brief 栈模块初始化
 * 
 * 初始化栈模块。当前版本不需要特殊初始化，保留此接口用于未来扩展。
 * 
 * @return 初始化成功返回true，失败返回false
 */
static inline bool F_stack_module_init(void)
{
    // 当前版本不需要特殊初始化
    return true;
}

/**
 * @brief 栈模块清理
 * 
 * 清理栈模块。当前版本不需要特殊清理，保留此接口用于未来扩展。
 */
static inline void F_stack_module_cleanup(void)
{
    // 当前版本不需要特殊清理
}

#ifdef __cplusplus
}
#endif

#endif // CN_STACK_H
