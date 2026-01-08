/**
 * @file CN_memory_debug_private.h
 * @brief 内存调试器私有数据结构和辅助函数声明
 * 
 * 本文件定义了内存调试器的私有数据结构和内部辅助函数。
 * 这些定义仅供内存调试器内部使用，不对外暴露。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_DEBUG_PRIVATE_H
#define CN_MEMORY_DEBUG_PRIVATE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 包含接口定义
#include "../interface/CN_memory_debug_interface.h"

// 前向声明子模块类型
typedef struct Stru_LeakDetectorContext_t Stru_LeakDetectorContext_t;
typedef struct Stru_ErrorDetectorContext_t Stru_ErrorDetectorContext_t;
typedef struct Stru_AnalyzerContext_t Stru_AnalyzerContext_t;

/**
 * @brief 内存调试器私有数据结构
 * 
 * 包含内存调试器的所有私有数据和状态信息。
 */
typedef struct Stru_MemoryDebuggerPrivateData_t
{
    // 子模块实例
    Stru_LeakDetectorContext_t* leak_detector;      ///< 泄漏检测器实例
    Stru_ErrorDetectorContext_t* error_detector;    ///< 错误检测器实例
    Stru_AnalyzerContext_t* analyzer;               ///< 内存分析器实例
    
    // 配置选项
    bool monitoring_enabled;                        ///< 监控是否启用
    bool overflow_check_enabled;                    ///< 缓冲区溢出检查是否启用
    bool double_free_check_enabled;                 ///< 双重释放检查是否启用
    bool uninitialized_check_enabled;               ///< 未初始化内存检查是否启用
    bool stack_trace_enabled;                       ///< 调用栈跟踪是否启用
    size_t stack_trace_depth;                       ///< 调用栈跟踪深度
    
    // 统计信息
    uint64_t total_allocations;                     ///< 总分配次数
    uint64_t total_deallocations;                   ///< 总释放次数
    size_t peak_memory_usage;                       ///< 峰值内存使用量
    size_t current_memory_usage;                    ///< 当前内存使用量
    
    // 输出函数
    void (*output_func)(const char* message);       ///< 调试输出函数
} Stru_MemoryDebuggerPrivateData_t;

/**
 * @brief 获取私有数据
 * 
 * 从接口实例中获取私有数据指针。
 * 
 * @param debugger 内存调试器接口实例
 * @return Stru_MemoryDebuggerPrivateData_t* 私有数据指针
 */
static inline Stru_MemoryDebuggerPrivateData_t* get_private_data(
    Stru_MemoryDebuggerInterface_t* debugger)
{
    if (debugger == NULL)
    {
        return NULL;
    }
    return (Stru_MemoryDebuggerPrivateData_t*)debugger->private_data;
}

/**
 * @brief 默认输出函数
 * 
 * 当用户未提供输出函数时使用的默认输出函数。
 * 
 * @param message 要输出的消息
 */
void default_output_func(const char* message);

/**
 * @brief 输出调试信息
 * 
 * 使用配置的输出函数输出调试信息。
 * 
 * @param data 私有数据
 * @param format 格式化字符串
 * @param ... 格式化参数
 */
void debug_output(Stru_MemoryDebuggerPrivateData_t* data, const char* format, ...);

#endif // CN_MEMORY_DEBUG_PRIVATE_H
