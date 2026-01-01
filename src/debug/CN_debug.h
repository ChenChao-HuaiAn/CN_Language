/******************************************************************************
 * 文件名: CN_debug.h
 * 功能: CN_Language调试系统头文件
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现基础调试系统
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_DEBUG_H
#define CN_DEBUG_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

// 调试级别定义
typedef enum {
    CN_DEBUG_LEVEL_NONE = 0,
    CN_DEBUG_LEVEL_ERROR = 1,
    CN_DEBUG_LEVEL_WARN = 2,
    CN_DEBUG_LEVEL_INFO = 3,
    CN_DEBUG_LEVEL_DEBUG = 4
} CN_DebugLevel;

// 调试宏
#ifdef CN_DEBUG_MODE
    #define CN_DEBUG(level, format, ...) \
        cn_debug_log(level, __FILE__, __LINE__, format, ##__VA_ARGS__)
    #define CN_ASSERT(condition, message) \
        cn_debug_assert(condition, __FILE__, __LINE__, message)
#else
    #define CN_DEBUG(level, format, ...)
    #define CN_ASSERT(condition, message)
#endif

// 接口声明
void cn_debug_init(CN_DebugLevel level);
void cn_debug_set_output_file(const char* filename);
void cn_debug_log(CN_DebugLevel level, const char* file, int line, 
                  const char* format, ...);
void cn_debug_assert(int condition, const char* file, int line, 
                     const char* message);
const char* cn_debug_level_to_string(CN_DebugLevel level);

#endif // CN_DEBUG_H
