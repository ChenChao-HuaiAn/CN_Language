/**
 * @file CN_memory_debug_core.h
 * @brief 内存调试器核心接口函数声明
 * 
 * 本文件声明了内存调试器的所有核心接口函数。
 * 这些函数实现了Stru_MemoryDebuggerInterface_t接口。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_DEBUG_CORE_H
#define CN_MEMORY_DEBUG_CORE_H

#include "../interface/CN_memory_debug_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 核心接口函数声明
// ============================================================================

/**
 * @brief 初始化内存调试器
 */
bool debug_initialize(Stru_MemoryDebuggerInterface_t* debugger);

/**
 * @brief 清理内存调试器资源
 */
void debug_cleanup(Stru_MemoryDebuggerInterface_t* debugger);

/**
 * @brief 启用或禁用内存监控
 */
void debug_enable_monitoring(Stru_MemoryDebuggerInterface_t* debugger, bool enable);

/**
 * @brief 检查内存监控是否启用
 */
bool debug_is_monitoring_enabled(Stru_MemoryDebuggerInterface_t* debugger);

/**
 * @brief 获取内存泄漏数量
 */
size_t debug_get_leak_count(Stru_MemoryDebuggerInterface_t* debugger);

/**
 * @brief 报告内存泄漏
 */
void debug_report_leaks(Stru_MemoryDebuggerInterface_t* debugger);

/**
 * @brief 清除泄漏记录
 */
void debug_clear_leak_records(Stru_MemoryDebuggerInterface_t* debugger);

/**
 * @brief 启用或禁用缓冲区溢出检查
 */
void debug_enable_overflow_check(Stru_MemoryDebuggerInterface_t* debugger, bool enable);

/**
 * @brief 启用或禁用双重释放检查
 */
void debug_enable_double_free_check(Stru_MemoryDebuggerInterface_t* debugger, bool enable);

/**
 * @brief 启用或禁用未初始化内存检查
 */
void debug_enable_uninitialized_check(Stru_MemoryDebuggerInterface_t* debugger, bool enable);

/**
 * @brief 验证指针有效性
 */
bool debug_validate_pointer(Stru_MemoryDebuggerInterface_t* debugger, const void* ptr);

/**
 * @brief 验证内存范围有效性
 */
bool debug_validate_memory_range(Stru_MemoryDebuggerInterface_t* debugger, 
                               const void* ptr, size_t size);

/**
 * @brief 检查所有活动内存分配
 */
void debug_check_all_allocations(Stru_MemoryDebuggerInterface_t* debugger);

/**
 * @brief 转储内存信息
 */
void debug_dump_memory_info(Stru_MemoryDebuggerInterface_t* debugger);

/**
 * @brief 转储分配统计信息
 */
void debug_dump_allocation_stats(Stru_MemoryDebuggerInterface_t* debugger);

/**
 * @brief 设置调试输出函数
 */
void debug_set_debug_output(Stru_MemoryDebuggerInterface_t* debugger, 
                          void (*output_func)(const char*));

/**
 * @brief 启用或禁用调用栈跟踪
 */
void debug_enable_stack_trace(Stru_MemoryDebuggerInterface_t* debugger, bool enable);

/**
 * @brief 获取调用栈跟踪深度
 */
size_t debug_get_stack_trace_depth(Stru_MemoryDebuggerInterface_t* debugger);

/**
 * @brief 设置调用栈跟踪深度
 */
void debug_set_stack_trace_depth(Stru_MemoryDebuggerInterface_t* debugger, size_t depth);

/**
 * @brief 获取总分配次数
 */
uint64_t debug_get_total_allocations(Stru_MemoryDebuggerInterface_t* debugger);

/**
 * @brief 获取总释放次数
 */
uint64_t debug_get_total_deallocations(Stru_MemoryDebuggerInterface_t* debugger);

/**
 * @brief 获取峰值内存使用量
 */
size_t debug_get_peak_memory_usage(Stru_MemoryDebuggerInterface_t* debugger);

/**
 * @brief 获取当前内存使用量
 */
size_t debug_get_current_memory_usage(Stru_MemoryDebuggerInterface_t* debugger);

#ifdef __cplusplus
}
#endif

#endif // CN_MEMORY_DEBUG_CORE_H
