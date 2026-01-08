/**
 * @file CN_memory_debug_tools_core.h
 * @brief 内存调试工具核心管理模块
 * 
 * 负责内存调试工具模块的初始化和全局状态管理。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_DEBUG_TOOLS_CORE_H
#define CN_MEMORY_DEBUG_TOOLS_CORE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 调试输出回调函数类型
 */
typedef void (*DebugOutputCallback)(const char* message);

/**
 * @brief 初始化调试工具核心模块
 * 
 * 初始化调试工具核心模块，设置默认状态。
 * 
 * @return bool 初始化成功返回true，否则返回false
 */
bool F_initialize_debug_tools_core(void);

/**
 * @brief 清理调试工具核心模块
 * 
 * 清理调试工具核心模块，释放相关资源。
 */
void F_cleanup_debug_tools_core(void);

/**
 * @brief 设置调试输出回调
 * 
 * 设置调试信息输出回调函数。
 * 
 * @param callback 回调函数
 */
void F_set_debug_output_callback_core(DebugOutputCallback callback);

/**
 * @brief 启用详细调试输出
 * 
 * 启用或禁用详细的调试输出。
 * 
 * @param enable 是否启用
 */
void F_enable_verbose_debugging_core(bool enable);

/**
 * @brief 设置调试输出文件
 * 
 * 设置调试输出文件。
 * 
 * @param filename 文件名
 * @return bool 设置成功返回true，否则返回false
 */
bool F_set_debug_output_file_core(const char* filename);

/**
 * @brief 检查是否已初始化
 * 
 * 检查调试工具核心模块是否已初始化。
 * 
 * @return bool 已初始化返回true，否则返回false
 */
bool F_is_debug_tools_initialized(void);

/**
 * @brief 检查是否已初始化（别名，用于子模块）
 * 
 * 检查调试工具核心模块是否已初始化。
 * 
 * @return bool 已初始化返回true，否则返回false
 */
static inline bool F_check_debug_tools_initialized(void)
{
    return F_is_debug_tools_initialized();
}

/**
 * @brief 检查是否启用详细调试输出
 * 
 * 检查是否启用了详细调试输出。
 * 
 * @return bool 启用返回true，否则返回false
 */
bool F_is_verbose_debugging_enabled(void);

/**
 * @brief 输出调试信息
 * 
 * 输出调试信息到配置的输出目标。
 * 
 * @param format 格式字符串
 * @param ... 可变参数
 */
void F_debug_output(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif // CN_MEMORY_DEBUG_TOOLS_CORE_H
