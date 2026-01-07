/**
 * @file CN_queue.h
 * @brief 队列模块主头文件
 * 
 * 提供队列数据结构的完整接口声明。
 * 这是队列模块的主要入口点，包含所有子模块的功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_QUEUE_H
#define CN_QUEUE_H

/* 包含所有子模块的头文件 */
#include "queue_core/CN_queue_core.h"
#include "queue_iterator/CN_queue_iterator.h"
#include "queue_utils/CN_queue_utils.h"

/**
 * @brief 队列模块版本信息
 * 
 * 提供队列模块的版本信息，便于版本管理和兼容性检查。
 */
#define CN_QUEUE_VERSION_MAJOR 2
#define CN_QUEUE_VERSION_MINOR 0
#define CN_QUEUE_VERSION_PATCH 0
#define CN_QUEUE_VERSION_STRING "2.0.0"

/**
 * @brief 获取队列模块版本
 * 
 * 返回队列模块的版本信息。
 * 
 * @param major 输出参数，主版本号
 * @param minor 输出参数，次版本号
 * @param patch 输出参数，修订号
 */
static inline void F_queue_get_version(int* major, int* minor, int* patch)
{
    if (major != NULL) *major = CN_QUEUE_VERSION_MAJOR;
    if (minor != NULL) *minor = CN_QUEUE_VERSION_MINOR;
    if (patch != NULL) *patch = CN_QUEUE_VERSION_PATCH;
}

/**
 * @brief 获取队列模块版本字符串
 * 
 * 返回队列模块的版本字符串。
 * 
 * @return 版本字符串
 */
static inline const char* F_queue_get_version_string(void)
{
    return CN_QUEUE_VERSION_STRING;
}

/**
 * @brief 队列模块初始化
 * 
 * 初始化队列模块。当前版本不需要特殊初始化，保留此接口用于未来扩展。
 * 
 * @return 初始化成功返回true，失败返回false
 */
static inline bool F_queue_module_init(void)
{
    // 当前版本不需要特殊初始化
    return true;
}

/**
 * @brief 队列模块清理
 * 
 * 清理队列模块。当前版本不需要特殊清理，保留此接口用于未来扩展。
 */
static inline void F_queue_module_cleanup(void)
{
    // 当前版本不需要特殊清理
}

#endif // CN_QUEUE_H
