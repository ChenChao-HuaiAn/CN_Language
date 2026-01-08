/**
 * @file CN_memory_debug_tools_stacktrace.h
 * @brief 调用栈跟踪模块
 * 
 * 负责调用栈跟踪和调试信息收集。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_DEBUG_TOOLS_STACKTRACE_H
#define CN_MEMORY_DEBUG_TOOLS_STACKTRACE_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 调用栈跟踪选项
 */
typedef struct Stru_StackTraceOptions_t
{
    size_t max_depth;            ///< 最大跟踪深度
    bool show_function_names;    ///< 显示函数名
    bool show_file_names;        ///< 显示文件名
    bool show_line_numbers;      ///< 显示行号
    bool show_addresses;         ///< 显示地址
} Stru_StackTraceOptions_t;

/**
 * @brief 获取调用栈跟踪
 * 
 * 获取当前调用栈跟踪信息。
 * 
 * @param options 跟踪选项
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return size_t 实际写入的字符数（不包括终止符）
 */
size_t F_get_stack_trace(const Stru_StackTraceOptions_t* options,
                        char* buffer, size_t buffer_size);

/**
 * @brief 获取调用栈跟踪地址数组
 * 
 * 获取调用栈地址数组。
 * 
 * @param addresses 地址数组
 * @param max_depth 最大深度
 * @return size_t 实际获取的地址数量
 */
size_t F_get_stack_trace_addresses(void** addresses, size_t max_depth);

/**
 * @brief 获取默认调用栈跟踪选项
 * 
 * 获取默认的调用栈跟踪选项。
 * 
 * @return Stru_StackTraceOptions_t 默认选项
 */
Stru_StackTraceOptions_t F_get_default_stacktrace_options(void);

#ifdef __cplusplus
}
#endif

#endif // CN_MEMORY_DEBUG_TOOLS_STACKTRACE_H
