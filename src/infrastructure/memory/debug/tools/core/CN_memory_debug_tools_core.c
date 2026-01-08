/**
 * @file CN_memory_debug_tools_core.c
 * @brief 内存调试工具核心管理模块实现
 * 
 * 实现了内存调试工具核心管理功能，包括：
 * - 模块初始化和清理
 * - 全局状态管理
 * - 调试输出控制
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#include "CN_memory_debug_tools_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// ============================================================================
// 私有数据结构和全局变量
// ============================================================================

/**
 * @brief 调试工具模块全局状态
 */
typedef struct Stru_DebugToolsGlobalState_t
{
    bool initialized;                     ///< 是否已初始化
    DebugOutputCallback output_callback;  ///< 输出回调函数
    FILE* output_file;                    ///< 输出文件
    bool verbose_debugging;               ///< 是否启用详细调试输出
} Stru_DebugToolsGlobalState_t;

static Stru_DebugToolsGlobalState_t g_debug_tools_state = {
    .initialized = false,
    .output_callback = NULL,
    .output_file = NULL,
    .verbose_debugging = false
};

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 默认输出函数
 */
static void default_output_func(const char* message)
{
    if (message != NULL)
    {
        printf("%s\n", message);
    }
}

/**
 * @brief 输出调试信息
 */
static void debug_output(const char* format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // 使用回调函数输出
    if (g_debug_tools_state.output_callback != NULL)
    {
        g_debug_tools_state.output_callback(buffer);
    }
    else
    {
        default_output_func(buffer);
    }
    
    // 输出到文件
    if (g_debug_tools_state.output_file != NULL)
    {
        fprintf(g_debug_tools_state.output_file, "%s\n", buffer);
        fflush(g_debug_tools_state.output_file);
    }
}

// ============================================================================
// 公共函数实现
// ============================================================================

/**
 * @brief 初始化调试工具核心模块
 */
bool F_initialize_debug_tools_core(void)
{
    if (g_debug_tools_state.initialized)
    {
        debug_output("调试工具核心模块已初始化");
        return true;
    }
    
    // 初始化全局状态
    g_debug_tools_state.initialized = true;
    g_debug_tools_state.output_callback = NULL;
    g_debug_tools_state.output_file = NULL;
    g_debug_tools_state.verbose_debugging = false;
    
    debug_output("调试工具核心模块初始化成功");
    return true;
}

/**
 * @brief 清理调试工具核心模块
 */
void F_cleanup_debug_tools_core(void)
{
    if (!g_debug_tools_state.initialized)
    {
        return;
    }
    
    // 关闭输出文件
    if (g_debug_tools_state.output_file != NULL)
    {
        fclose(g_debug_tools_state.output_file);
        g_debug_tools_state.output_file = NULL;
    }
    
    // 重置状态
    g_debug_tools_state.initialized = false;
    g_debug_tools_state.output_callback = NULL;
    g_debug_tools_state.verbose_debugging = false;
    
    debug_output("调试工具核心模块清理完成");
}

/**
 * @brief 设置调试输出回调
 */
void F_set_debug_output_callback_core(DebugOutputCallback callback)
{
    if (!g_debug_tools_state.initialized)
    {
        debug_output("警告：调试工具核心模块未初始化");
        return;
    }
    
    g_debug_tools_state.output_callback = callback;
    debug_output("调试输出回调已设置");
}

/**
 * @brief 启用详细调试输出
 */
void F_enable_verbose_debugging_core(bool enable)
{
    if (!g_debug_tools_state.initialized)
    {
        debug_output("警告：调试工具核心模块未初始化");
        return;
    }
    
    g_debug_tools_state.verbose_debugging = enable;
    debug_output("详细调试输出%s", enable ? "启用" : "禁用");
}

/**
 * @brief 设置调试输出文件
 */
bool F_set_debug_output_file_core(const char* filename)
{
    if (!g_debug_tools_state.initialized || filename == NULL)
    {
        debug_output("错误：调试工具核心模块未初始化或文件名为空");
        return false;
    }
    
    // 关闭现有文件
    if (g_debug_tools_state.output_file != NULL)
    {
        fclose(g_debug_tools_state.output_file);
        g_debug_tools_state.output_file = NULL;
    }
    
    // 打开新文件
    FILE* file = fopen(filename, "a");
    if (file == NULL)
    {
        debug_output("无法打开调试输出文件: %s", filename);
        return false;
    }
    
    g_debug_tools_state.output_file = file;
    debug_output("调试输出文件已设置为: %s", filename);
    return true;
}

/**
 * @brief 检查是否已初始化
 */
bool F_is_debug_tools_initialized(void)
{
    return g_debug_tools_state.initialized;
}

/**
 * @brief 获取全局状态（内部使用）
 */
Stru_DebugToolsGlobalState_t* F_get_debug_tools_global_state(void)
{
    return &g_debug_tools_state;
}

/**
 * @brief 获取调试输出函数（内部使用）
 */
void F_debug_output_internal(const char* format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    debug_output("%s", buffer);
}

/**
 * @brief 检查是否启用详细调试输出
 */
bool F_is_verbose_debugging_enabled(void)
{
    return g_debug_tools_state.verbose_debugging;
}

/**
 * @brief 输出调试信息
 */
void F_debug_output(const char* format, ...)
{
    if (!g_debug_tools_state.initialized)
    {
        return;
    }
    
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    debug_output("%s", buffer);
}
