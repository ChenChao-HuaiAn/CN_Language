/******************************************************************************
 * 文件名: CN_debug.c
 * 功能: CN_Language调试系统实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现基础调试系统
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 全局变量
static Eum_DebugLevel_t g_debug_level = Eum_DEBUG_LEVEL_NONE;
static FILE* g_debug_file = NULL;
static char* g_debug_filename = NULL;

/**
 * @brief 初始化调试系统
 * @param level 调试级别
 * @note 设置全局调试级别，控制调试信息的输出
 */
void F_debug_init(Eum_DebugLevel_t level)
{
    g_debug_level = level;
    
    // 如果之前有打开的文件，先关闭
    if (g_debug_file != NULL && g_debug_file != stdout && g_debug_file != stderr)
    {
        fclose(g_debug_file);
        g_debug_file = NULL;
    }
    
    // 释放之前的文件名
    if (g_debug_filename != NULL)
    {
        free(g_debug_filename);
        g_debug_filename = NULL;
    }
}

/**
 * @brief 设置调试输出文件
 * @param filename 输出文件名，NULL表示输出到标准错误
 * @note 可以将调试信息重定向到指定文件
 */
void F_debug_set_output_file(const char* filename)
{
    // 关闭当前文件
    if (g_debug_file != NULL && g_debug_file != stdout && g_debug_file != stderr)
    {
        fclose(g_debug_file);
        g_debug_file = NULL;
    }
    
    // 释放之前的文件名
    if (g_debug_filename != NULL)
    {
        free(g_debug_filename);
        g_debug_filename = NULL;
    }
    
    // 设置新的文件名
    if (filename != NULL)
    {
        g_debug_filename = strdup(filename);
        if (g_debug_filename != NULL)
        {
            g_debug_file = fopen(g_debug_filename, "a");
            if (g_debug_file == NULL)
            {
                // 如果无法打开文件，回退到标准错误输出
                g_debug_file = stderr;
            }
        }
        else
        {
            g_debug_file = stderr;
        }
    }
    else
    {
        g_debug_file = stderr;
    }
}

/**
 * @brief 将调试级别转换为字符串
 * @param level 调试级别
 * @return 对应的字符串表示
 */
const char* F_debug_level_to_string(Eum_DebugLevel_t level)
{
    switch (level)
    {
        case Eum_DEBUG_LEVEL_ERROR:
            return "ERROR";
        case Eum_DEBUG_LEVEL_WARN:
            return "WARN";
        case Eum_DEBUG_LEVEL_INFO:
            return "INFO";
        case Eum_DEBUG_LEVEL_DEBUG:
            return "DEBUG";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief 记录调试信息
 * @param level 调试级别
 * @param file 源文件名
 * @param line 行号
 * @param format 格式化字符串
 * @param ... 可变参数
 * @note 根据调试级别和全局设置决定是否输出调试信息
 */
void F_debug_log(Eum_DebugLevel_t level, const char* file, int line, 
                  const char* format, ...)
{
    // 检查是否应该输出此级别的调试信息
    if (level > g_debug_level || g_debug_level == Eum_DEBUG_LEVEL_NONE)
    {
        return;
    }
    
    // 确保有输出文件
    if (g_debug_file == NULL)
    {
        g_debug_file = stderr;
    }
    
    // 获取当前时间
    time_t current_time;
    time(&current_time);
    struct tm* time_info = localtime(&current_time);
    
    // 输出时间戳和调试级别
    fprintf(g_debug_file, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] ",
            time_info->tm_year + 1900,
            time_info->tm_mon + 1,
            time_info->tm_mday,
            time_info->tm_hour,
            time_info->tm_min,
            time_info->tm_sec,
            F_debug_level_to_string(level));
    
    // 输出文件名和行号
    const char* filename = strrchr(file, '/');
    if (filename == NULL)
    {
        filename = strrchr(file, '\\');
    }
    if (filename == NULL)
    {
        filename = file;
    }
    else
    {
        filename++; // 跳过路径分隔符
    }
    
    fprintf(g_debug_file, "[%s:%d] ", filename, line);
    
    // 输出格式化消息
    va_list args;
    va_start(args, format);
    vfprintf(g_debug_file, format, args);
    va_end(args);
    
    // 输出换行符
    fprintf(g_debug_file, "\n");
    
    // 刷新输出缓冲区
    fflush(g_debug_file);
}

/**
 * @brief 调试断言
 * @param condition 断言条件
 * @param file 源文件名
 * @param line 行号
 * @param message 错误消息
 * @note 当条件为假时输出错误信息并终止程序
 */
void F_debug_assert(int condition, const char* file, int line, 
                     const char* message)
{
    if (!condition)
    {
        // 输出断言失败信息
        if (g_debug_file == NULL)
        {
            g_debug_file = stderr;
        }
        
        const char* filename = strrchr(file, '/');
        if (filename == NULL)
        {
            filename = strrchr(file, '\\');
        }
        if (filename == NULL)
        {
            filename = file;
        }
        else
        {
            filename++; // 跳过路径分隔符
        }
        
        fprintf(g_debug_file, "ASSERTION FAILED: %s\n", message);
        fprintf(g_debug_file, "File: %s, Line: %d\n", filename, line);
        fflush(g_debug_file);
        
        // 终止程序
        abort();
    }
}
