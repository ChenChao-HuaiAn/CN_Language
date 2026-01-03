/******************************************************************************
 * 文件名: CN_error_context.c
 * 功能: CN_Language错误上下文实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现错误上下文功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_error_context.h"
#include "CN_error.h"  // 添加CN_error.h以获取CN_error_is_recoverable等函数声明
#include "CN_error_codes.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

/**
 * @brief 获取当前时间戳（毫秒）
 * 
 * @return 当前时间戳
 */
static uint64_t get_current_timestamp(void)
{
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    // 转换为毫秒（从1601年1月1日到现在的100纳秒间隔数）
    return (uli.QuadPart / 10000) - 11644473600000ULL;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

/**
 * @brief 获取当前线程ID
 * 
 * @return 线程ID
 */
static uint64_t get_current_thread_id(void)
{
#ifdef _WIN32
    return GetCurrentThreadId();
#else
    return (uint64_t)pthread_self();
#endif
}

/**
 * @brief 创建错误上下文
 */
Stru_CN_ErrorContext_t CN_error_create_context(
    Eum_CN_ErrorCode_t error_code,
    const char* message,
    const char* filename,
    size_t line,
    size_t column,
    const char* function,
    const char* module)
{
    Stru_CN_ErrorContext_t context;
    
    context.error_code = error_code;
    context.message = message;
    context.filename = filename;
    context.line = line;
    context.column = column;
    context.function = function;
    context.module = module;
    context.timestamp = get_current_timestamp();
    context.thread_id = get_current_thread_id();
    context.recoverable = CN_error_is_recoverable(error_code);
    context.user_data = NULL;
    context.user_data_size = 0;
    
    return context;
}

/**
 * @brief 创建简单的错误上下文（只有错误码和消息）
 */
Stru_CN_ErrorContext_t CN_error_create_simple_context(
    Eum_CN_ErrorCode_t error_code,
    const char* message)
{
    return CN_error_create_context(
        error_code,
        message,
        NULL,
        0,
        0,
        NULL,
        NULL);
}

/**
 * @brief 创建带位置信息的错误上下文
 */
Stru_CN_ErrorContext_t CN_error_create_position_context(
    Eum_CN_ErrorCode_t error_code,
    const char* message,
    const char* filename,
    size_t line,
    size_t column)
{
    return CN_error_create_context(
        error_code,
        message,
        filename,
        line,
        column,
        NULL,
        NULL);
}

/**
 * @brief 格式化错误上下文为字符串
 */
size_t CN_error_format_context(
    const Stru_CN_ErrorContext_t* context,
    char* buffer,
    size_t buffer_size)
{
    if (!context || !buffer || buffer_size == 0)
    {
        return 0;
    }
    
    const char* error_desc = CN_error_get_description(context->error_code);
    const char* actual_message = context->message ? context->message : error_desc;
    
    // 格式化时间
    char time_buf[64];
    time_t rawtime = context->timestamp / 1000;
    struct tm* timeinfo = localtime(&rawtime);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    int written = 0;
    
    if (context->filename && context->line > 0)
    {
        if (context->column > 0)
        {
            written = snprintf(buffer, buffer_size,
                "[%s] [线程: %llu] [%s:%zu:%zu] 错误 %04X: %s",
                time_buf,
                (unsigned long long)context->thread_id,
                context->filename,
                context->line,
                context->column,
                context->error_code,
                actual_message);
        }
        else
        {
            written = snprintf(buffer, buffer_size,
                "[%s] [线程: %llu] [%s:%zu] 错误 %04X: %s",
                time_buf,
                (unsigned long long)context->thread_id,
                context->filename,
                context->line,
                context->error_code,
                actual_message);
        }
    }
    else if (context->function)
    {
        written = snprintf(buffer, buffer_size,
            "[%s] [线程: %llu] [函数: %s] 错误 %04X: %s",
            time_buf,
            (unsigned long long)context->thread_id,
            context->function,
            context->error_code,
            actual_message);
    }
    else if (context->module)
    {
        written = snprintf(buffer, buffer_size,
            "[%s] [线程: %llu] [模块: %s] 错误 %04X: %s",
            time_buf,
            (unsigned long long)context->thread_id,
            context->module,
            context->error_code,
            actual_message);
    }
    else
    {
        written = snprintf(buffer, buffer_size,
            "[%s] [线程: %llu] 错误 %04X: %s",
            time_buf,
            (unsigned long long)context->thread_id,
            context->error_code,
            actual_message);
    }
    
    if (written < 0)
    {
        return 0;
    }
    
    if ((size_t)written >= buffer_size)
    {
        return buffer_size - 1;
    }
    
    return (size_t)written;
}

/**
 * @brief 获取错误上下文的默认消息
 */
const char* CN_error_get_default_message(
    const Stru_CN_ErrorContext_t* context)
{
    if (!context)
    {
        return "无效的错误上下文";
    }
    
    if (context->message)
    {
        return context->message;
    }
    
    return CN_error_get_description(context->error_code);
}

/**
 * @brief 检查错误上下文是否有效
 */
bool CN_error_is_context_valid(const Stru_CN_ErrorContext_t* context)
{
    if (!context)
    {
        return false;
    }
    
    // 检查错误码是否在有效范围内
    if (context->error_code > Eum_CN_ERROR_USER_6)
    {
        return false;
    }
    
    // 检查时间戳是否合理（不能是未来的时间，也不能太早）
    uint64_t current_time = get_current_timestamp();
    if (context->timestamp > current_time + 3600000) // 未来1小时
    {
        return false;
    }
    
    // 检查行号和列号
    if (context->line == 0 && context->column > 0)
    {
        return false; // 列号不为0但行号为0是无效的
    }
    
    return true;
}

/**
 * @brief 清空错误上下文
 */
void CN_error_clear_context(Stru_CN_ErrorContext_t* context)
{
    if (!context)
    {
        return;
    }
    
    context->error_code = Eum_CN_ERROR_SUCCESS;
    context->message = NULL;
    context->filename = NULL;
    context->line = 0;
    context->column = 0;
    context->function = NULL;
    context->module = NULL;
    context->timestamp = 0;
    context->thread_id = 0;
    context->recoverable = true;
    context->user_data = NULL;
    context->user_data_size = 0;
}

/**
 * @brief 复制错误上下文
 */
bool CN_error_copy_context(
    Stru_CN_ErrorContext_t* dest,
    const Stru_CN_ErrorContext_t* src)
{
    if (!dest || !src)
    {
        return false;
    }
    
    // 简单复制所有字段
    *dest = *src;
    
    // 注意：这里只复制指针，不复制指针指向的数据
    // 如果需要深拷贝，需要额外的处理
    
    return true;
}
