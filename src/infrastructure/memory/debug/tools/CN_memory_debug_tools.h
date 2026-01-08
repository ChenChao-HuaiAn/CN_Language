/**
 * @file CN_memory_debug_tools.h
 * @brief 内存调试工具模块 - 主接口文件
 * 
 * 本模块提供了各种内存调试工具，采用模块化设计：
 * - 核心管理模块 (core/) - 初始化和全局状态管理
 * - 内存转储模块 (dump/) - 内存内容转储和格式化输出
 * - 内存验证模块 (validation/) - 内存验证和模式检查
 * - 调用栈跟踪模块 (stacktrace/) - 调用栈跟踪功能
 * - 工具函数模块 (utils/) - 辅助工具函数
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_DEBUG_TOOLS_H
#define CN_MEMORY_DEBUG_TOOLS_H

// 包含所有子模块的头文件
#include "core/CN_memory_debug_tools_core.h"
#include "dump/CN_memory_debug_tools_dump.h"
#include "validation/CN_memory_debug_tools_validation.h"
#include "stacktrace/CN_memory_debug_tools_stacktrace.h"
#include "utils/CN_memory_debug_tools_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 向后兼容接口（保持原有函数名）
// ============================================================================

/**
 * @brief 初始化调试工具（向后兼容）
 * 
 * 初始化调试工具模块。
 * 
 * @return bool 初始化成功返回true，否则返回false
 */
static inline bool F_initialize_debug_tools(void)
{
    return F_initialize_debug_tools_core();
}

/**
 * @brief 清理调试工具（向后兼容）
 * 
 * 清理调试工具模块，释放相关资源。
 */
static inline void F_cleanup_debug_tools(void)
{
    F_cleanup_debug_tools_core();
}

/**
 * @brief 设置调试输出回调（向后兼容）
 * 
 * 设置调试信息输出回调函数。
 * 
 * @param callback 回调函数
 */
static inline void F_set_debug_output_callback(DebugOutputCallback callback)
{
    F_set_debug_output_callback_core(callback);
}

/**
 * @brief 启用详细调试输出（向后兼容）
 * 
 * 启用或禁用详细的调试输出。
 * 
 * @param enable 是否启用
 */
static inline void F_enable_verbose_debugging(bool enable)
{
    F_enable_verbose_debugging_core(enable);
}

/**
 * @brief 设置调试输出文件（向后兼容）
 * 
 * 设置调试输出文件。
 * 
 * @param filename 文件名
 * @return bool 设置成功返回true，否则返回false
 */
static inline bool F_set_debug_output_file(const char* filename)
{
    return F_set_debug_output_file_core(filename);
}


// ============================================================================
// 类型定义（从子模块重新导出）
// ============================================================================

typedef Stru_MemoryDumpOptions_t Stru_MemoryDumpOptions_t;
typedef Stru_StackTraceOptions_t Stru_StackTraceOptions_t;
typedef DebugOutputCallback DebugOutputCallback;

#ifdef __cplusplus
}
#endif

#endif // CN_MEMORY_DEBUG_TOOLS_H
