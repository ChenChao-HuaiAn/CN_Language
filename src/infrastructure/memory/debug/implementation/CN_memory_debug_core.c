/**
 * @file CN_memory_debug_core.c
 * @brief 内存调试器核心接口函数实现
 * 
 * 本文件实现了内存调试器的核心接口函数。
 * 这些函数提供了内存调试的基本功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#include "CN_memory_debug_private.h"
#include "../leak_detection/CN_memory_leak_detection.h"
#include "../error_detection/CN_memory_error_detection.h"
#include "../analysis/CN_memory_analysis.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 接口函数实现
// ============================================================================

/**
 * @brief 初始化内存调试器
 */
bool debug_initialize(Stru_MemoryDebuggerInterface_t* debugger)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return false;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    // 初始化子模块
    if (data->leak_detector != NULL)
    {
        // 泄漏检测器没有单独的初始化函数，创建时已初始化
        debug_output(data, "泄漏检测器已初始化");
    }
    
    if (data->error_detector != NULL)
    {
        // 错误检测器没有单独的初始化函数，创建时已初始化
        debug_output(data, "错误检测器已初始化");
    }
    
    if (data->analyzer != NULL)
    {
        // 分析器没有单独的初始化函数，创建时已初始化
        debug_output(data, "内存分析器已初始化");
    }
    
    // 初始化统计信息
    data->total_allocations = 0;
    data->total_deallocations = 0;
    data->peak_memory_usage = 0;
    data->current_memory_usage = 0;
    
    // 设置默认输出函数
    if (data->output_func == NULL)
    {
        data->output_func = default_output_func;
    }
    
    debug_output(data, "内存调试器初始化成功");
    return true;
}

/**
 * @brief 清理内存调试器资源
 */
void debug_cleanup(Stru_MemoryDebuggerInterface_t* debugger)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    debug_output(data, "清理内存调试器资源");
    
    // 清理子模块
    if (data->leak_detector != NULL)
    {
        F_destroy_leak_detector(data->leak_detector);
        data->leak_detector = NULL;
    }
    
    if (data->error_detector != NULL)
    {
        F_destroy_error_detector(data->error_detector);
        data->error_detector = NULL;
    }
    
    if (data->analyzer != NULL)
    {
        F_destroy_memory_analyzer(data->analyzer);
        data->analyzer = NULL;
    }
    
    // 清理私有数据
    free(data);
    debugger->private_data = NULL;
}

/**
 * @brief 启用或禁用内存监控
 */
void debug_enable_monitoring(Stru_MemoryDebuggerInterface_t* debugger, bool enable)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    data->monitoring_enabled = enable;
    debug_output(data, "内存监控%s", enable ? "启用" : "禁用");
}

/**
 * @brief 检查内存监控是否启用
 */
bool debug_is_monitoring_enabled(Stru_MemoryDebuggerInterface_t* debugger)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return false;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    return data->monitoring_enabled;
}

/**
 * @brief 获取内存泄漏数量
 */
size_t debug_get_leak_count(Stru_MemoryDebuggerInterface_t* debugger)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return 0;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    if (data->leak_detector == NULL)
    {
        return 0;
    }
    
    return F_check_leaks(data->leak_detector);
}

/**
 * @brief 报告内存泄漏
 */
void debug_report_leaks(Stru_MemoryDebuggerInterface_t* debugger)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    if (data->leak_detector == NULL)
    {
        debug_output(data, "警告：泄漏检测器未启用");
        return;
    }
    
    char report_buffer[4096];
    F_generate_leak_report(data->leak_detector, report_buffer, sizeof(report_buffer));
    debug_output(data, "%s", report_buffer);
}

/**
 * @brief 清除泄漏记录
 */
void debug_clear_leak_records(Stru_MemoryDebuggerInterface_t* debugger)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    if (data->leak_detector == NULL)
    {
        return;
    }
    
    F_clear_leak_records(data->leak_detector);
    debug_output(data, "泄漏记录已清除");
}

/**
 * @brief 启用或禁用缓冲区溢出检查
 */
void debug_enable_overflow_check(Stru_MemoryDebuggerInterface_t* debugger, bool enable)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    data->overflow_check_enabled = enable;
    
    // 注意：子模块没有直接的启用/禁用函数，需要在创建时配置
    debug_output(data, "缓冲区溢出检查%s", enable ? "启用" : "禁用");
}

/**
 * @brief 启用或禁用双重释放检查
 */
void debug_enable_double_free_check(Stru_MemoryDebuggerInterface_t* debugger, bool enable)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    data->double_free_check_enabled = enable;
    
    // 注意：子模块没有直接的启用/禁用函数，需要在创建时配置
    debug_output(data, "双重释放检查%s", enable ? "启用" : "禁用");
}

/**
 * @brief 启用或禁用未初始化内存检查
 */
void debug_enable_uninitialized_check(Stru_MemoryDebuggerInterface_t* debugger, bool enable)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    data->uninitialized_check_enabled = enable;
    
    // 注意：子模块没有直接的启用/禁用函数，需要在创建时配置
    debug_output(data, "未初始化内存检查%s", enable ? "启用" : "禁用");
}

/**
 * @brief 验证指针有效性
 */
bool debug_validate_pointer(Stru_MemoryDebuggerInterface_t* debugger, const void* ptr)
{
    if (debugger == NULL || debugger->private_data == NULL || ptr == NULL)
    {
        return false;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    if (data->error_detector == NULL)
    {
        return true; // 如果未启用错误检测，假设指针有效
    }
    
    return F_validate_pointer(data->error_detector, ptr);
}

/**
 * @brief 验证内存范围有效性
 */
bool debug_validate_memory_range(Stru_MemoryDebuggerInterface_t* debugger, 
                               const void* ptr, size_t size)
{
    if (debugger == NULL || debugger->private_data == NULL || ptr == NULL || size == 0)
    {
        return false;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    if (data->error_detector == NULL)
    {
        return true; // 如果未启用错误检测，假设内存范围有效
    }
    
    return F_validate_memory_range(data->error_detector, ptr, size);
}

/**
 * @brief 检查所有活动内存分配
 */
void debug_check_all_allocations(Stru_MemoryDebuggerInterface_t* debugger)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    debug_output(data, "检查所有活动内存分配...");
    
    if (data->leak_detector != NULL)
    {
        size_t leak_count = F_check_leaks(data->leak_detector);
        if (leak_count > 0)
        {
            debug_output(data, "发现 %zu 个内存泄漏", leak_count);
            debug_report_leaks(debugger);
        }
        else
        {
            debug_output(data, "未发现内存泄漏");
        }
    }
    
    if (data->error_detector != NULL)
    {
        size_t error_count = F_check_all_allocations(data->error_detector);
        if (error_count > 0)
        {
            debug_output(data, "发现 %zu 个内存错误", error_count);
        }
        else
        {
            debug_output(data, "未发现内存错误");
        }
    }
}

/**
 * @brief 转储内存信息
 */
void debug_dump_memory_info(Stru_MemoryDebuggerInterface_t* debugger)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    debug_output(data, "=== 内存信息转储 ===");
    debug_output(data, "监控状态: %s", data->monitoring_enabled ? "启用" : "禁用");
    debug_output(data, "缓冲区溢出检查: %s", data->overflow_check_enabled ? "启用" : "禁用");
    debug_output(data, "双重释放检查: %s", data->double_free_check_enabled ? "启用" : "禁用");
    debug_output(data, "未初始化内存检查: %s", data->uninitialized_check_enabled ? "启用" : "禁用");
    debug_output(data, "调用栈跟踪: %s (深度: %zu)", 
                data->stack_trace_enabled ? "启用" : "禁用", 
                data->stack_trace_depth);
    
    // 转泄漏信息
    if (data->leak_detector != NULL)
    {
        size_t leak_count = F_check_leaks(data->leak_detector);
        debug_output(data, "内存泄漏数量: %zu", leak_count);
        size_t active_allocations = F_get_active_allocations(data->leak_detector);
        debug_output(data, "活动分配数量: %zu", active_allocations);
        size_t total_allocated = F_get_total_allocated_memory(data->leak_detector);
        debug_output(data, "总分配内存: %zu 字节", total_allocated);
    }
    
    // 转统计信息
    debug_output(data, "总分配次数: %llu", (unsigned long long)data->total_allocations);
    debug_output(data, "总释放次数: %llu", (unsigned long long)data->total_deallocations);
    debug_output(data, "当前内存使用: %zu 字节", data->current_memory_usage);
    debug_output(data, "峰值内存使用: %zu 字节", data->peak_memory_usage);
    
    debug_output(data, "=== 转储完成 ===");
}

/**
 * @brief 转储分配统计信息
 */
void debug_dump_allocation_stats(Stru_MemoryDebuggerInterface_t* debugger)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    if (data->analyzer == NULL)
    {
        debug_output(data, "警告：内存分析器未启用");
        return;
    }
    
    char report_buffer[4096];
    F_generate_memory_report(data->analyzer, report_buffer, sizeof(report_buffer));
    debug_output(data, "%s", report_buffer);
}

/**
 * @brief 设置调试输出函数
 */
void debug_set_debug_output(Stru_MemoryDebuggerInterface_t* debugger, 
                          void (*output_func)(const char*))
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    data->output_func = (output_func != NULL) ? output_func : default_output_func;
}

/**
 * @brief 启用或禁用调用栈跟踪
 */
void debug_enable_stack_trace(Stru_MemoryDebuggerInterface_t* debugger, bool enable)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    data->stack_trace_enabled = enable;
    debug_output(data, "调用栈跟踪%s", enable ? "启用" : "禁用");
}

/**
 * @brief 获取调用栈跟踪深度
 */
size_t debug_get_stack_trace_depth(Stru_MemoryDebuggerInterface_t* debugger)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return 0;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    return data->stack_trace_depth;
}

/**
 * @brief 设置调用栈跟踪深度
 */
void debug_set_stack_trace_depth(Stru_MemoryDebuggerInterface_t* debugger, size_t depth)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    data->stack_trace_depth = depth;
    debug_output(data, "调用栈跟踪深度设置为: %zu", depth);
}

/**
 * @brief 获取总分配次数
 */
uint64_t debug_get_total_allocations(Stru_MemoryDebuggerInterface_t* debugger)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return 0;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    return data->total_allocations;
}

/**
 * @brief 获取总释放次数
 */
uint64_t debug_get_total_deallocations(Stru_MemoryDebuggerInterface_t* debugger)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return 0;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    return data->total_deallocations;
}

/**
 * @brief 获取峰值内存使用量
 */
size_t debug_get_peak_memory_usage(Stru_MemoryDebuggerInterface_t* debugger)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return 0;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    return data->peak_memory_usage;
}

/**
 * @brief 获取当前内存使用量
 */
size_t debug_get_current_memory_usage(Stru_MemoryDebuggerInterface_t* debugger)
{
    if (debugger == NULL || debugger->private_data == NULL)
    {
        return 0;
    }
    
    Stru_MemoryDebuggerPrivateData_t* data = get_private_data(debugger);
    
    return data->current_memory_usage;
}
