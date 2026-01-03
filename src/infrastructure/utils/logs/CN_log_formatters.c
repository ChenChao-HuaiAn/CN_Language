/******************************************************************************
 * 文件名: CN_log_formatters.c
 * 功能: CN_Language日志系统格式化器实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现日志格式化器
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_log_internal.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 格式化时间戳为可读字符串
 * 
 * @param timestamp 时间戳（毫秒）
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 格式化后的字符串长度
 */
static size_t format_timestamp(long long timestamp, char* buffer, size_t buffer_size)
{
    if (timestamp <= 0 || !buffer || buffer_size == 0)
        return 0;
    
    time_t seconds = timestamp / 1000;
    int milliseconds = timestamp % 1000;
    
    struct tm* timeinfo = localtime(&seconds);
    if (!timeinfo)
        return 0;
    
    return snprintf(buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                    timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
                    milliseconds);
}

/**
 * @brief 获取线程ID（简化实现）
 * 
 * @return 线程ID字符串
 */
static const char* get_thread_id(void)
{
    // 简化实现，实际项目中需要平台特定代码
    static char thread_id[16] = "main";
    return thread_id;
}

// ============================================================================
// 默认格式化器实现
// ============================================================================

static size_t default_formatter_format(char* buffer, size_t buffer_size,
                                       Eum_CN_LogLevel_t level,
                                       long long timestamp,
                                       const char* file,
                                       int line,
                                       const char* function,
                                       const char* format,
                                       va_list args)
{
    if (!buffer || buffer_size == 0)
        return 0;
    
    size_t offset = 0;
    
    // 时间戳
    if (timestamp > 0)
    {
        char time_buffer[64];
        size_t time_len = format_timestamp(timestamp, time_buffer, sizeof(time_buffer));
        if (time_len > 0 && time_len < sizeof(time_buffer))
        {
            offset += snprintf(buffer + offset, buffer_size - offset, "[%s] ", time_buffer);
        }
    }
    
    // 线程ID（可选）
    // 这里暂时省略，实际可根据配置添加
    
    // 日志级别
    const char* level_str = CN_log_level_to_string(level);
    const char* color_code = CN_log_get_level_color(level);
    const char* reset_code = "\033[0m";
    
    if (color_code[0] != '\0')
    {
        offset += snprintf(buffer + offset, buffer_size - offset, "%s[%s]%s ",
                          color_code, level_str, reset_code);
    }
    else
    {
        offset += snprintf(buffer + offset, buffer_size - offset, "[%s] ", level_str);
    }
    
    // 源文件位置（可选）
    if (file && file[0] != '\0' && line > 0 && function && function[0] != '\0')
    {
        // 提取文件名（去掉路径）
        const char* filename = strrchr(file, '/');
        if (!filename)
            filename = strrchr(file, '\\');
        if (filename)
            filename++;
        else
            filename = file;
        
        offset += snprintf(buffer + offset, buffer_size - offset, "[%s:%d %s] ",
                          filename, line, function);
    }
    
    // 消息内容
    if (format && format[0] != '\0')
    {
        offset += vsnprintf(buffer + offset, buffer_size - offset, format, args);
    }
    
    // 确保以换行符结束
    if (offset < buffer_size - 1)
    {
        buffer[offset] = '\n';
        buffer[offset + 1] = '\0';
        offset++;
    }
    else if (buffer_size > 1)
    {
        buffer[buffer_size - 2] = '\n';
        buffer[buffer_size - 1] = '\0';
        offset = buffer_size - 1;
    }
    
    return offset;
}

static const char* default_formatter_get_name(void)
{
    return "default";
}

const Stru_CN_LogFormatterInterface_t CN_log_default_formatter = {
    .format = default_formatter_format,
    .get_name = default_formatter_get_name
};

// ============================================================================
// 简单格式化器实现
// ============================================================================

static size_t simple_formatter_format(char* buffer, size_t buffer_size,
                                      Eum_CN_LogLevel_t level,
                                      long long timestamp,
                                      const char* file,
                                      int line,
                                      const char* function,
                                      const char* format,
                                      va_list args)
{
    if (!buffer || buffer_size == 0)
        return 0;
    
    size_t offset = 0;
    
    // 只显示级别和消息
    const char* level_str = CN_log_level_to_string(level);
    offset += snprintf(buffer + offset, buffer_size - offset, "[%s] ", level_str);
    
    // 消息内容
    if (format && format[0] != '\0')
    {
        offset += vsnprintf(buffer + offset, buffer_size - offset, format, args);
    }
    
    // 确保以换行符结束
    if (offset < buffer_size - 1)
    {
        buffer[offset] = '\n';
        buffer[offset + 1] = '\0';
        offset++;
    }
    
    return offset;
}

static const char* simple_formatter_get_name(void)
{
    return "simple";
}

const Stru_CN_LogFormatterInterface_t CN_log_simple_formatter = {
    .format = simple_formatter_format,
    .get_name = simple_formatter_get_name
};

// ============================================================================
// JSON格式化器实现
// ============================================================================

static size_t json_formatter_format(char* buffer, size_t buffer_size,
                                    Eum_CN_LogLevel_t level,
                                    long long timestamp,
                                    const char* file,
                                    int line,
                                    const char* function,
                                    const char* format,
                                    va_list args)
{
    if (!buffer || buffer_size == 0)
        return 0;
    
    // 先格式化消息内容
    char message_buffer[512];
    vsnprintf(message_buffer, sizeof(message_buffer), format, args);
    
    // 转义特殊字符（简化实现）
    char escaped_buffer[1024];
    size_t escaped_len = 0;
    for (size_t i = 0; message_buffer[i] != '\0' && escaped_len < sizeof(escaped_buffer) - 1; i++)
    {
        switch (message_buffer[i])
        {
            case '"': 
                if (escaped_len + 2 < sizeof(escaped_buffer))
                {
                    escaped_buffer[escaped_len++] = '\\';
                    escaped_buffer[escaped_len++] = '"';
                }
                break;
            case '\\':
                if (escaped_len + 2 < sizeof(escaped_buffer))
                {
                    escaped_buffer[escaped_len++] = '\\';
                    escaped_buffer[escaped_len++] = '\\';
                }
                break;
            case '\n':
                if (escaped_len + 2 < sizeof(escaped_buffer))
                {
                    escaped_buffer[escaped_len++] = '\\';
                    escaped_buffer[escaped_len++] = 'n';
                }
                break;
            case '\t':
                if (escaped_len + 2 < sizeof(escaped_buffer))
                {
                    escaped_buffer[escaped_len++] = '\\';
                    escaped_buffer[escaped_len++] = 't';
                }
                break;
            default:
                escaped_buffer[escaped_len++] = message_buffer[i];
                break;
        }
    }
    escaped_buffer[escaped_len] = '\0';
    
    // 格式化时间戳
    char time_buffer[64];
    format_timestamp(timestamp, time_buffer, sizeof(time_buffer));
    
    // 提取文件名
    const char* filename = file;
    if (file)
    {
        const char* slash = strrchr(file, '/');
        if (!slash)
            slash = strrchr(file, '\\');
        if (slash)
            filename = slash + 1;
    }
    
    // 生成JSON
    const char* level_str = CN_log_level_to_string(level);
    size_t offset = snprintf(buffer, buffer_size,
        "{\"timestamp\":\"%s\",\"level\":\"%s\",\"file\":\"%s\",\"line\":%d,\"function\":\"%s\",\"message\":\"%s\"}\n",
        time_buffer, level_str, filename ? filename : "", line, function ? function : "", escaped_buffer);
    
    return offset;
}

static const char* json_formatter_get_name(void)
{
    return "json";
}

const Stru_CN_LogFormatterInterface_t CN_log_json_formatter = {
    .format = json_formatter_format,
    .get_name = json_formatter_get_name
};
