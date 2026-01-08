/**
 * @file CN_memory_debug_private.c
 * @brief 内存调试器私有辅助函数实现
 * 
 * 本文件实现了内存调试器的私有辅助函数。
 * 这些函数仅供内存调试器内部使用，不对外暴露。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#include "CN_memory_debug_private.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// ============================================================================
// 辅助函数实现
// ============================================================================

/**
 * @brief 默认输出函数
 */
void default_output_func(const char* message)
{
    if (message != NULL)
    {
        printf("%s\n", message);
    }
}

/**
 * @brief 输出调试信息
 */
void debug_output(Stru_MemoryDebuggerPrivateData_t* data, const char* format, ...)
{
    if (data == NULL || data->output_func == NULL)
    {
        return;
    }
    
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    data->output_func(buffer);
}
