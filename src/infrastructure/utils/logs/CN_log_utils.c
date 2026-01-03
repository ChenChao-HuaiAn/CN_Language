/******************************************************************************
 * 文件名: CN_log_utils.c
 * 功能: CN_Language日志系统工具函数
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，从CN_log_core.c中提取工具函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_log_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

// ============================================================================
// 互斥锁管理函数
// ============================================================================

/**
 * @brief 初始化互斥锁
 */
bool CN_log_mutex_init(void)
{
    // 在Windows上使用CriticalSection，在POSIX上使用pthread_mutex_t
    // 这里简化实现，实际项目中需要平台特定代码
    // 暂时返回true表示成功
    return true;
}

/**
 * @brief 销毁互斥锁
 */
void CN_log_mutex_destroy(void)
{
    // 平台特定的互斥锁销毁代码
}

/**
 * @brief 锁定互斥锁
 */
void CN_log_mutex_lock(void)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    if (state->config.thread_safe && state->mutex)
    {
        // 平台特定的互斥锁锁定代码
    }
}

/**
 * @brief 解锁互斥锁
 */
void CN_log_mutex_unlock(void)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    if (state->config.thread_safe && state->mutex)
    {
        // 平台特定的互斥锁解锁代码
    }
}

// ============================================================================
// 时间戳和格式化函数
// ============================================================================

/**
 * @brief 获取当前时间戳（毫秒）
 */
long long CN_log_get_timestamp(void)
{
    // 简化实现，实际项目中需要平台特定代码
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

/**
 * @brief 格式化日志级别为字符串
 */
const char* CN_log_level_to_string(Eum_CN_LogLevel_t level)
{
    switch (level)
    {
        case Eum_LOG_LEVEL_TRACE: return "TRACE";
        case Eum_LOG_LEVEL_DEBUG: return "DEBUG";
        case Eum_LOG_LEVEL_INFO:  return "INFO";
        case Eum_LOG_LEVEL_WARN:  return "WARN";
        case Eum_LOG_LEVEL_ERROR: return "ERROR";
        case Eum_LOG_LEVEL_FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

/**
 * @brief 获取日志级别颜色代码（控制台）
 */
const char* CN_log_get_level_color(Eum_CN_LogLevel_t level)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (!state->config.enable_colors)
        return "";
    
    switch (level)
    {
        case Eum_LOG_LEVEL_TRACE: return "\033[90m";  // 灰色
        case Eum_LOG_LEVEL_DEBUG: return "\033[36m";  // 青色
        case Eum_LOG_LEVEL_INFO:  return "\033[32m";  // 绿色
        case Eum_LOG_LEVEL_WARN:  return "\033[33m";  // 黄色
        case Eum_LOG_LEVEL_ERROR: return "\033[31m";  // 红色
        case Eum_LOG_LEVEL_FATAL: return "\033[1;31m"; // 粗体红色
        default: return "\033[0m"; // 重置
    }
}

// ============================================================================
// 消息管理函数
// ============================================================================

/**
 * @brief 创建日志消息
 */
Stru_CN_LogMessage_t* CN_log_create_message(
    Eum_CN_LogLevel_t level,
    const char* file,
    int line,
    const char* function,
    const char* format,
    va_list args)
{
    // 第一次调用vsnprintf获取所需长度
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    
    if (needed < 0)
        return NULL;
    
    // 分配内存：结构体大小 + 消息字符串大小 + 终止符
    size_t message_size = (size_t)needed + 1;
    size_t total_size = sizeof(Stru_CN_LogMessage_t) + message_size;
    
    Stru_CN_LogMessage_t* message = (Stru_CN_LogMessage_t*)malloc(total_size);
    if (!message)
        return NULL;
    
    // 填充消息结构
    message->level = level;
    message->timestamp = CN_log_get_timestamp();
    message->file = file;
    message->line = line;
    message->function = function;
    
    // 格式化消息
    vsnprintf(message->message, message_size, format, args);
    
    return message;
}

/**
 * @brief 释放日志消息
 */
void CN_log_free_message(Stru_CN_LogMessage_t* message)
{
    if (message)
        free(message);
}

// ============================================================================
// 消息处理函数
// ============================================================================

/**
 * @brief 同步处理日志消息
 */
void CN_log_process_message_sync(Stru_CN_LogMessage_t* message)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (!state->initialized || !message || !state->current_formatter)
        return;
    
    // 格式化消息
    char buffer[CN_LOG_DEFAULT_BUFFER_SIZE];
    
    // 由于消息已经格式化，我们需要直接使用格式化后的字符串
    // 这里简化处理，实际格式化器应该能够处理已格式化的消息
    size_t formatted_length = 0;
    
    // 简单实现：直接复制消息
    if (state->config.show_timestamp && state->config.show_source_location)
    {
        formatted_length = snprintf(buffer, sizeof(buffer),
            "[%lld] [%s:%d] [%s] %s: %s",
            message->timestamp,
            message->file, message->line, message->function,
            CN_log_level_to_string(message->level),
            message->message);
    }
    else if (state->config.show_timestamp)
    {
        formatted_length = snprintf(buffer, sizeof(buffer),
            "[%lld] [%s] %s",
            message->timestamp,
            CN_log_level_to_string(message->level),
            message->message);
    }
    else
    {
        formatted_length = snprintf(buffer, sizeof(buffer),
            "[%s] %s",
            CN_log_level_to_string(message->level),
            message->message);
    }
    
    if (formatted_length == 0 || formatted_length >= sizeof(buffer))
        return;
    
    // 发送到所有输出处理器
    for (size_t i = 0; i < state->output_count; i++)
    {
        if (state->outputs[i].handler && state->outputs[i].initialized)
        {
            if (!state->outputs[i].handler->write(buffer, formatted_length, message->level))
                state->failed_writes++;
        }
    }
}
