/**
 * @file CN_memory_debug_new.c
 * @brief 内存调试器模块化实现（薄包装层）
 * 
 * 这是重构后的内存调试器实现，作为薄包装层调用模块化实现。
 * 保持向后兼容性，同时遵守单一职责原则。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.1.0
 * @copyright MIT License
 */

#include "CN_memory_debug.h"
#include "interface/CN_memory_debug_factory.h"
#include "implementation/CN_memory_debug_private.h"
#include "implementation/CN_memory_debug_core.h"
#include "leak_detection/CN_memory_leak_detection.h"
#include "error_detection/CN_memory_error_detection.h"
#include "analysis/CN_memory_analysis.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 工厂函数实现（调用模块化实现）
// ============================================================================

/**
 * @brief 创建默认内存调试器
 */
Stru_MemoryDebuggerInterface_t* F_create_default_memory_debugger(void)
{
    return F_create_configured_memory_debugger(
        true,   // enable_leak_detection
        true,   // enable_overflow_check
        true,   // enable_double_free_check
        4       // stack_trace_depth
    );
}

/**
 * @brief 创建具有特定配置的内存调试器
 */
Stru_MemoryDebuggerInterface_t* F_create_configured_memory_debugger(
    bool enable_leak_detection,
    bool enable_overflow_check,
    bool enable_double_free_check,
    size_t stack_trace_depth)
{
    // 创建接口实例
    Stru_MemoryDebuggerInterface_t* debugger = 
        (Stru_MemoryDebuggerInterface_t*)malloc(sizeof(Stru_MemoryDebuggerInterface_t));
    
    if (debugger == NULL)
    {
        return NULL;
    }
    
    // 创建私有数据
    Stru_MemoryDebuggerPrivateData_t* data = 
        (Stru_MemoryDebuggerPrivateData_t*)malloc(sizeof(Stru_MemoryDebuggerPrivateData_t));
    
    if (data == NULL)
    {
        free(debugger);
        return NULL;
    }
    
    // 初始化私有数据
    memset(data, 0, sizeof(Stru_MemoryDebuggerPrivateData_t));
    
    // 创建子模块
    if (enable_leak_detection)
    {
        data->leak_detector = F_create_leak_detector(
            stack_trace_depth > 0,  // enable_stack_trace
            stack_trace_depth       // max_stack_depth
        );
    }
    
    if (enable_overflow_check || enable_double_free_check)
    {
        data->error_detector = F_create_error_detector(
            enable_overflow_check,   // enable_overflow_check
            enable_double_free_check, // enable_double_free_check
            false,                   // enable_uninitialized_check (默认禁用)
            16                       // guard_zone_size
        );
    }
    
    // 总是创建分析器
    data->analyzer = F_create_memory_analyzer(
        true,   // enable_performance_tracking
        true,   // enable_fragmentation_analysis
        1000    // sampling_interval_ms
    );
    
    // 设置配置
    data->monitoring_enabled = true;
    data->overflow_check_enabled = enable_overflow_check;
    data->double_free_check_enabled = enable_double_free_check;
    data->uninitialized_check_enabled = false;
    data->stack_trace_enabled = (stack_trace_depth > 0);
    data->stack_trace_depth = stack_trace_depth;
    
    // 设置接口函数（指向模块化实现）
    debugger->initialize = debug_initialize;
    debugger->cleanup = debug_cleanup;
    debugger->enable_monitoring = debug_enable_monitoring;
    debugger->is_monitoring_enabled = debug_is_monitoring_enabled;
    debugger->get_leak_count = debug_get_leak_count;
    debugger->report_leaks = debug_report_leaks;
    debugger->clear_leak_records = debug_clear_leak_records;
    debugger->enable_overflow_check = debug_enable_overflow_check;
    debugger->enable_double_free_check = debug_enable_double_free_check;
    debugger->enable_uninitialized_check = debug_enable_uninitialized_check;
    debugger->validate_pointer = debug_validate_pointer;
    debugger->validate_memory_range = debug_validate_memory_range;
    debugger->check_all_allocations = debug_check_all_allocations;
    debugger->dump_memory_info = debug_dump_memory_info;
    debugger->dump_allocation_stats = debug_dump_allocation_stats;
    debugger->set_debug_output = debug_set_debug_output;
    debugger->enable_stack_trace = debug_enable_stack_trace;
    debugger->get_stack_trace_depth = debug_get_stack_trace_depth;
    debugger->set_stack_trace_depth = debug_set_stack_trace_depth;
    debugger->get_total_allocations = debug_get_total_allocations;
    debugger->get_total_deallocations = debug_get_total_deallocations;
    debugger->get_peak_memory_usage = debug_get_peak_memory_usage;
    debugger->get_current_memory_usage = debug_get_current_memory_usage;
    
    // 设置私有数据
    debugger->private_data = data;
    
    // 初始化调试器
    if (!debug_initialize(debugger))
    {
        debug_cleanup(debugger);
        free(debugger);
        return NULL;
    }
    
    return debugger;
}

/**
 * @brief 销毁内存调试器
 */
void F_destroy_memory_debugger(Stru_MemoryDebuggerInterface_t* debugger)
{
    if (debugger == NULL)
    {
        return;
    }
    
    debug_cleanup(debugger);
    free(debugger);
}

/**
 * @brief 获取内存调试器接口（向后兼容）
 */
Stru_MemoryDebuggerInterface_t* F_get_memory_debugger_interface(void)
{
    static Stru_MemoryDebuggerInterface_t* default_debugger = NULL;
    
    if (default_debugger == NULL)
    {
        default_debugger = F_create_default_memory_debugger();
    }
    
    return default_debugger;
}

// ============================================================================
// 向后兼容函数（旧接口）
// ============================================================================

/**
 * @brief 创建内存调试器（旧接口）
 */
Stru_MemoryDebugInterface_t* F_create_memory_debugger(
    Stru_MemoryAllocatorInterface_t* allocator)
{
    // 创建新的调试器
    Stru_MemoryDebuggerInterface_t* new_debugger = F_create_default_memory_debugger();
    
    if (new_debugger == NULL)
    {
        return NULL;
    }
    
    // 将新的调试器转换为旧接口
    // 注意：这是一个简化实现，实际需要适配器模式
    Stru_MemoryDebugInterface_t* old_debugger = 
        (Stru_MemoryDebugInterface_t*)malloc(sizeof(Stru_MemoryDebugInterface_t));
    
    if (old_debugger == NULL)
    {
        F_destroy_memory_debugger(new_debugger);
        return NULL;
    }
    
    // 设置旧接口函数（简化实现）
    old_debugger->enable_debugging = NULL; // 暂时不实现
    old_debugger->check_leaks = NULL;      // 暂时不实现
    old_debugger->get_usage_report = NULL; // 暂时不实现
    old_debugger->set_allocation_failure_callback = NULL; // 暂时不实现
    old_debugger->dump_allocations = NULL; // 暂时不实现
    
    // 存储新调试器引用以便后续使用
    // 注意：这里需要存储引用，但旧接口没有private_data字段
    // 实际实现需要更复杂的适配器
    
    return old_debugger;
}

/**
 * @brief 销毁内存调试器（旧接口）
 */
void F_destroy_memory_debugger_old(Stru_MemoryDebugInterface_t* debugger)
{
    if (debugger != NULL)
    {
        free(debugger);
    }
}

/**
 * @brief 启用详细调试输出（旧接口）
 */
void F_enable_verbose_debugging_old(Stru_MemoryDebugInterface_t* debugger,
                                   bool enable)
{
    (void)debugger;
    (void)enable;
    // 简化实现：暂时不实现
}

/**
 * @brief 设置调试输出回调（旧接口）
 */
void F_set_debug_output_callback_old(Stru_MemoryDebugInterface_t* debugger,
                                    void (*callback)(const char* message))
{
    (void)debugger;
    (void)callback;
    // 简化实现：暂时不实现
}
