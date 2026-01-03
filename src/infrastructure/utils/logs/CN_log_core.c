/******************************************************************************
 * 文件名: CN_log_core.c
 * 功能: CN_Language日志系统核心实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 重构文件，提取工具函数和异步函数到单独文件
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_log_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// ============================================================================
// 静态全局状态
// ============================================================================

static Stru_CN_LogSystemState_t g_log_state = {
    .initialized = false,
    .enabled = true,
    .formatter_count = 0,
    .output_count = 0,
    .async_thread_running = false,
    .mutex = NULL,
    .total_logs = 0,
    .filtered_logs = 0,
    .failed_writes = 0,
    .current_formatter = NULL
};

// ============================================================================
// 默认配置
// ============================================================================

static const Stru_CN_LogConfig_t g_default_config = {
    .default_level = Eum_LOG_LEVEL_INFO,
    .thread_safe = true,
    .async_mode = false,
    .async_queue_size = CN_LOG_DEFAULT_ASYNC_QUEUE_SIZE,
    .enable_colors = true,
    .show_timestamp = true,
    .show_source_location = true,
    .show_thread_id = false,
    .default_formatter = "default"
};

// ============================================================================
// 状态管理函数
// ============================================================================

/**
 * @brief 获取日志系统状态（单例）
 */
Stru_CN_LogSystemState_t* CN_log_get_state(void)
{
    return &g_log_state;
}

// ============================================================================
// 公共API实现
// ============================================================================

bool CN_log_init(const Stru_CN_LogConfig_t* config)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (state->initialized)
        return true;
    
    // 设置配置
    if (config)
        state->config = *config;
    else
        state->config = g_default_config;
    
    // 初始化互斥锁
    if (state->config.thread_safe)
    {
        if (!CN_log_mutex_init())
            return false;
    }
    
    // 初始化异步队列
    if (state->config.async_mode && state->config.async_queue_size > 0)
    {
        if (!CN_log_async_queue_init(&state->async_queue, state->config.async_queue_size))
            return false;
        
        // 启动异步线程（简化实现，实际需要线程创建代码）
        state->async_thread_running = true;
    }
    
    // 注册默认格式化器
    CN_log_register_formatter("default", &CN_log_default_formatter);
    CN_log_register_formatter("simple", &CN_log_simple_formatter);
    CN_log_register_formatter("json", &CN_log_json_formatter);
    
    // 设置默认格式化器
    CN_log_set_formatter(state->config.default_formatter);
    
    // 注册默认输出处理器
    CN_log_set_output_handler(Eum_LOG_OUTPUT_CONSOLE, &CN_log_console_output_handler);
    CN_log_set_output_handler(Eum_LOG_OUTPUT_FILE, &CN_log_file_output_handler);
    
    state->initialized = true;
    state->enabled = true;
    
    return true;
}

void CN_log_shutdown(void)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (!state->initialized)
        return;
    
    CN_log_mutex_lock();
    
    // 停止异步线程
    if (state->async_thread_running)
    {
        state->async_thread_running = false;
        // 等待线程结束（简化实现）
    }
    
    // 销毁异步队列
    if (state->config.async_mode)
        CN_log_async_queue_destroy(&state->async_queue);
    
    // 关闭所有输出处理器
    for (size_t i = 0; i < state->output_count; i++)
    {
        if (state->outputs[i].handler && state->outputs[i].initialized)
            state->outputs[i].handler->shutdown();
    }
    
    // 销毁互斥锁
    if (state->config.thread_safe)
        CN_log_mutex_destroy();
    
    // 重置状态
    state->initialized = false;
    state->enabled = false;
    state->formatter_count = 0;
    state->output_count = 0;
    state->current_formatter = NULL;
    
    CN_log_mutex_unlock();
}

void CN_log_core(Eum_CN_LogLevel_t level,
                 const char* file,
                 int line,
                 const char* function,
                 const char* format,
                 ...)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (!state->initialized || !state->enabled)
        return;
    
    // 检查日志级别
    if (level < state->config.default_level)
    {
        state->filtered_logs++;
        return;
    }
    
    state->total_logs++;
    
    CN_log_mutex_lock();
    
    // 创建日志消息
    va_list args;
    va_start(args, format);
    Stru_CN_LogMessage_t* message = CN_log_create_message(level, file, line, function, format, args);
    va_end(args);
    
    if (!message)
    {
        state->failed_writes++;
        CN_log_mutex_unlock();
        return;
    }
    
    // 处理消息
    if (state->config.async_mode)
    {
        if (!CN_log_process_message_async(message))
        {
            CN_log_free_message(message);
            state->failed_writes++;
        }
    }
    else
    {
        CN_log_process_message_sync(message);
        CN_log_free_message(message);
    }
    
    CN_log_mutex_unlock();
}

void CN_log(Eum_CN_LogLevel_t level, const char* format, ...)
{
    // 简化接口，不包含源文件信息
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (!state->initialized || !state->enabled)
        return;
    
    if (level < state->config.default_level)
    {
        state->filtered_logs++;
        return;
    }
    
    state->total_logs++;
    
    CN_log_mutex_lock();
    
    va_list args;
    va_start(args, format);
    
    // 创建简单消息（无源文件信息）
    Stru_CN_LogMessage_t* message = CN_log_create_message(
        level, "", 0, "", format, args);
    
    va_end(args);
    
    if (!message)
    {
        state->failed_writes++;
        CN_log_mutex_unlock();
        return;
    }
    
    // 处理消息
    if (state->config.async_mode)
    {
        if (!CN_log_process_message_async(message))
        {
            CN_log_free_message(message);
            state->failed_writes++;
        }
    }
    else
    {
        CN_log_process_message_sync(message);
        CN_log_free_message(message);
    }
    
    CN_log_mutex_unlock();
}

// ============================================================================
// 配置管理函数实现
// ============================================================================

Stru_CN_LogConfig_t CN_log_get_config(void)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    return state->config;
}

bool CN_log_set_config(const Stru_CN_LogConfig_t* config)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (!state->initialized || !config)
        return false;
    
    CN_log_mutex_lock();
    state->config = *config;
    CN_log_mutex_unlock();
    
    return true;
}

// ============================================================================
// 其他公共API实现
// ============================================================================

void CN_log_set_level(Eum_CN_LogLevel_t level)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (!state->initialized)
        return;
    
    CN_log_mutex_lock();
    state->config.default_level = level;
    CN_log_mutex_unlock();
}

Eum_CN_LogLevel_t CN_log_get_level(void)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    return state->initialized ? state->config.default_level : Eum_LOG_LEVEL_INFO;
}

void CN_log_set_enabled(bool enabled)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (!state->initialized)
        return;
    
    CN_log_mutex_lock();
    state->enabled = enabled;
    CN_log_mutex_unlock();
}

bool CN_log_is_enabled(void)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    return state->initialized && state->enabled;
}

void CN_log_flush_all(void)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (!state->initialized)
        return;
    
    CN_log_mutex_lock();
    
    for (size_t i = 0; i < state->output_count; i++)
    {
        if (state->outputs[i].handler && state->outputs[i].initialized)
            state->outputs[i].handler->flush();
    }
    
    CN_log_mutex_unlock();
}

void CN_log_get_stats(size_t* total_logs, size_t* filtered_logs, size_t* failed_writes)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (!state->initialized)
    {
        if (total_logs) *total_logs = 0;
        if (filtered_logs) *filtered_logs = 0;
        if (failed_writes) *failed_writes = 0;
        return;
    }
    
    CN_log_mutex_lock();
    
    if (total_logs) *total_logs = state->total_logs;
    if (filtered_logs) *filtered_logs = state->filtered_logs;
    if (failed_writes) *failed_writes = state->failed_writes;
    
    CN_log_mutex_unlock();
}
