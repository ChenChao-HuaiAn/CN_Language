/**
 * @file CN_utils_math_error.c
 * @brief 数学函数和错误处理函数实现
 * 
 * 实现了数学函数和错误处理框架，提供统一的错误管理和日志功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_utils_interface.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

// ============================================================================
// 数学函数实现
// ============================================================================

int cn_abs(int x)
{
    return (x < 0) ? -x : x;
}

long cn_labs(long x)
{
    return (x < 0) ? -x : x;
}

long long cn_llabs(long long x)
{
    return (x < 0) ? -x : x;
}

int cn_div(int numer, int denom, int* rem)
{
    if (denom == 0)
    {
        if (rem != NULL)
        {
            *rem = 0;
        }
        return 0;
    }
    
    int quotient = numer / denom;
    
    if (rem != NULL)
    {
        *rem = numer % denom;
    }
    
    return quotient;
}

int cn_min(int a, int b)
{
    return (a < b) ? a : b;
}

int cn_max(int a, int b)
{
    return (a > b) ? a : b;
}

int cn_clamp(int value, int min, int max)
{
    if (value < min)
    {
        return min;
    }
    
    if (value > max)
    {
        return max;
    }
    
    return value;
}

// ============================================================================
// 错误处理函数实现
// ============================================================================

// 线程局部错误信息
#ifdef _WIN32
#include <windows.h>
static __declspec(thread) Stru_ErrorInfo_t* g_thread_error = NULL;
#else
#include <pthread.h>
static __thread Stru_ErrorInfo_t* g_thread_error = NULL;
#endif

// 默认日志级别
static Eum_LogLevel_t g_log_level = Eum_LOG_LEVEL_INFO;

// 日志回调函数
static void (*g_log_callback)(Eum_LogLevel_t level,
                             const char* file, int line,
                             const char* message) = NULL;

void cn_set_error(Eum_ErrorCode_t code, const char* message,
                  const char* file, int line, Stru_ErrorInfo_t* cause)
{
    // 清除现有错误
    cn_clear_error();
    
    // 创建新错误
    g_thread_error = cn_create_error(code, message, file, line, cause);
}

Stru_ErrorInfo_t* cn_get_error(void)
{
    return g_thread_error;
}

void cn_clear_error(void)
{
    if (g_thread_error != NULL)
    {
        cn_destroy_error(g_thread_error);
        g_thread_error = NULL;
    }
}

Stru_ErrorInfo_t* cn_create_error(Eum_ErrorCode_t code, const char* message,
                                  const char* file, int line, Stru_ErrorInfo_t* cause)
{
    Stru_ErrorInfo_t* error = (Stru_ErrorInfo_t*)malloc(sizeof(Stru_ErrorInfo_t));
    
    if (error == NULL)
    {
        return NULL;
    }
    
    error->code = code;
    
    // 复制消息
    if (message != NULL)
    {
        size_t len = strlen(message);
        char* msg_copy = (char*)malloc(len + 1);
        
        if (msg_copy != NULL)
        {
            strcpy(msg_copy, message);
            error->message = msg_copy;
        }
        else
        {
            error->message = "Memory allocation failed for error message";
        }
    }
    else
    {
        error->message = "Unknown error";
    }
    
    // 复制文件名
    if (file != NULL)
    {
        size_t len = strlen(file);
        char* file_copy = (char*)malloc(len + 1);
        
        if (file_copy != NULL)
        {
            strcpy(file_copy, file);
            error->file = file_copy;
        }
        else
        {
            error->file = "unknown";
        }
    }
    else
    {
        error->file = "unknown";
    }
    
    error->line = line;
    error->cause = cause;
    
    return error;
}

void cn_destroy_error(Stru_ErrorInfo_t* error)
{
    if (error == NULL)
    {
        return;
    }
    
    // 递归销毁原因错误
    if (error->cause != NULL)
    {
        cn_destroy_error(error->cause);
    }
    
    // 释放消息字符串
    if (error->message != NULL && 
        error->message != "Memory allocation failed for error message" &&
        error->message != "Unknown error")
    {
        free((void*)error->message);
    }
    
    // 释放文件名字符串
    if (error->file != NULL && 
        error->file != "unknown")
    {
        free((void*)error->file);
    }
    
    free(error);
}

// ============================================================================
// 日志函数实现
// ============================================================================

void cn_log(Eum_LogLevel_t level, const char* file, int line,
            const char* format, ...)
{
    // 检查日志级别
    if (level < g_log_level)
    {
        return;
    }
    
    // 格式化消息
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // 调用日志回调或使用默认输出
    if (g_log_callback != NULL)
    {
        g_log_callback(level, file, line, buffer);
    }
    else
    {
        // 默认输出到stderr
        const char* level_str = "UNKNOWN";
        switch (level)
        {
            case Eum_LOG_LEVEL_TRACE:   level_str = "TRACE"; break;
            case Eum_LOG_LEVEL_DEBUG:   level_str = "DEBUG"; break;
            case Eum_LOG_LEVEL_INFO:    level_str = "INFO"; break;
            case Eum_LOG_LEVEL_WARNING: level_str = "WARNING"; break;
            case Eum_LOG_LEVEL_ERROR:   level_str = "ERROR"; break;
            case Eum_LOG_LEVEL_FATAL:   level_str = "FATAL"; break;
        }
        
        fprintf(stderr, "[%s] %s:%d: %s\n", level_str, file, line, buffer);
    }
}

void cn_set_log_level(Eum_LogLevel_t level)
{
    g_log_level = level;
}

void cn_set_log_callback(void (*callback)(Eum_LogLevel_t level,
                                          const char* file, int line,
                                          const char* message))
{
    g_log_callback = callback;
}
