/**
 * @file CN_memory_leak_detection.c
 * @brief 内存泄漏检测模块实现
 * 
 * 实现了内存泄漏检测功能，包括：
 * - 跟踪所有内存分配
 * - 检测未释放的内存块
 * - 生成泄漏报告
 * - 支持调用栈跟踪
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#include "CN_memory_leak_detection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * @brief 内存分配记录
 */
typedef struct Stru_AllocationRecord_t
{
    void* ptr;                      ///< 分配的内存指针
    size_t size;                    ///< 分配的大小
    const char* file;               ///< 分配所在的源文件
    int line;                       ///< 分配所在的行号
    time_t timestamp;               ///< 分配时间戳
    struct Stru_AllocationRecord_t* next; ///< 下一个记录
} Stru_AllocationRecord_t;

/**
 * @brief 泄漏检测器私有数据
 */
typedef struct Stru_LeakDetectorData_t
{
    Stru_AllocationRecord_t* allocations; ///< 分配记录链表
    size_t total_allocations;             ///< 总分配次数
    size_t total_deallocations;           ///< 总释放次数
    size_t current_allocations;           ///< 当前活动分配数
    size_t peak_allocations;              ///< 峰值活动分配数
    size_t total_allocated_bytes;         ///< 总分配字节数
    size_t total_freed_bytes;             ///< 总释放字节数
    bool enabled;                         ///< 是否启用泄漏检测
    bool track_stack_trace;               ///< 是否跟踪调用栈
    size_t stack_trace_depth;             ///< 调用栈跟踪深度
} Stru_LeakDetectorData_t;

/**
 * @brief 创建泄漏检测器
 */
Stru_LeakDetectorContext_t* F_create_leak_detector(bool enable_stack_trace, size_t max_stack_depth)
{
    Stru_LeakDetectorData_t* data = 
        (Stru_LeakDetectorData_t*)malloc(sizeof(Stru_LeakDetectorData_t));
    
    if (data == NULL)
    {
        return NULL;
    }
    
    // 初始化数据
    data->allocations = NULL;
    data->total_allocations = 0;
    data->total_deallocations = 0;
    data->current_allocations = 0;
    data->peak_allocations = 0;
    data->total_allocated_bytes = 0;
    data->total_freed_bytes = 0;
    data->enabled = true;
    data->track_stack_trace = enable_stack_trace;
    data->stack_trace_depth = max_stack_depth;
    
    return (Stru_LeakDetectorContext_t*)data;
}

/**
 * @brief 销毁泄漏检测器
 */
void F_destroy_leak_detector(Stru_LeakDetectorContext_t* detector)
{
    if (detector == NULL)
    {
        return;
    }
    
    Stru_LeakDetectorData_t* data = (Stru_LeakDetectorData_t*)detector;
    
    if (data != NULL)
    {
        // 清理所有分配记录
        F_clear_leak_records(detector);
        free(data);
    }
}

/**
 * @brief 记录内存分配
 */
void F_record_allocation(Stru_LeakDetectorContext_t* detector,
                        void* address, size_t size,
                        const char* file, size_t line,
                        const char* function)
{
    if (detector == NULL || address == NULL)
    {
        return;
    }
    
    Stru_LeakDetectorData_t* data = (Stru_LeakDetectorData_t*)detector;
    
    if (!data->enabled)
    {
        return;
    }
    
    // 创建新的分配记录
    Stru_AllocationRecord_t* record = 
        (Stru_AllocationRecord_t*)malloc(sizeof(Stru_AllocationRecord_t));
    
    if (record == NULL)
    {
        return;
    }
    
    // 填充记录信息
    record->ptr = address;
    record->size = size;
    record->file = file;
    record->line = (int)line;
    record->timestamp = time(NULL);
    record->next = data->allocations;
    
    // 添加到链表头部
    data->allocations = record;
    
    // 更新统计信息
    data->total_allocations++;
    data->current_allocations++;
    data->total_allocated_bytes += size;
    
    if (data->current_allocations > data->peak_allocations)
    {
        data->peak_allocations = data->current_allocations;
    }
}

/**
 * @brief 记录内存释放
 */
void F_record_deallocation(Stru_LeakDetectorContext_t* detector, void* address)
{
    if (detector == NULL || address == NULL)
    {
        return;
    }
    
    Stru_LeakDetectorData_t* data = (Stru_LeakDetectorData_t*)detector;
    
    if (!data->enabled)
    {
        return;
    }
    
    // 在链表中查找对应的分配记录
    Stru_AllocationRecord_t* current = data->allocations;
    Stru_AllocationRecord_t* previous = NULL;
    
    while (current != NULL)
    {
        if (current->ptr == address)
        {
            // 从链表中移除记录
            if (previous == NULL)
            {
                data->allocations = current->next;
            }
            else
            {
                previous->next = current->next;
            }
            
            // 更新统计信息
            data->total_deallocations++;
            data->current_allocations--;
            data->total_freed_bytes += current->size;
            
            // 释放记录内存
            free(current);
            return;
        }
        
        previous = current;
        current = current->next;
    }
}

/**
 * @brief 检查内存泄漏
 */
size_t F_check_leaks(Stru_LeakDetectorContext_t* detector)
{
    if (detector == NULL)
    {
        return 0;
    }
    
    Stru_LeakDetectorData_t* data = (Stru_LeakDetectorData_t*)detector;
    
    if (!data->enabled)
    {
        return 0;
    }
    
    // 计算链表中的记录数量
    size_t count = 0;
    Stru_AllocationRecord_t* current = data->allocations;
    
    while (current != NULL)
    {
        count++;
        current = current->next;
    }
    
    return count;
}

/**
 * @brief 生成泄漏报告
 */
void F_generate_leak_report(Stru_LeakDetectorContext_t* detector,
                           char* report_buffer, size_t buffer_size)
{
    if (detector == NULL || report_buffer == NULL || buffer_size == 0)
    {
        return;
    }
    
    Stru_LeakDetectorData_t* data = (Stru_LeakDetectorData_t*)detector;
    
    if (!data->enabled)
    {
        snprintf(report_buffer, buffer_size, "泄漏检测已禁用\n");
        return;
    }
    
    size_t leak_count = F_check_leaks(detector);
    
    if (leak_count == 0)
    {
        snprintf(report_buffer, buffer_size, "未检测到内存泄漏\n");
        return;
    }
    
    // 生成泄漏报告
    char* ptr = report_buffer;
    size_t remaining = buffer_size;
    
    int written = snprintf(ptr, remaining, 
                          "内存泄漏报告（共 %zu 个泄漏）:\n"
                          "========================================\n",
                          leak_count);
    
    if (written > 0 && (size_t)written < remaining)
    {
        ptr += written;
        remaining -= written;
    }
    
    // 遍历所有泄漏记录
    Stru_AllocationRecord_t* current = data->allocations;
    size_t index = 1;
    size_t total_leaked_bytes = 0;
    
    while (current != NULL && remaining > 0)
    {
        // 格式化时间戳
        char time_str[64];
        struct tm* tm_info = localtime(&current->timestamp);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        
        // 添加泄漏记录
        written = snprintf(ptr, remaining,
                          "泄漏 #%zu:\n"
                          "  地址: %p\n"
                          "  大小: %zu 字节\n"
                          "  位置: %s:%d\n"
                          "  时间: %s\n"
                          "  -------------------------\n",
                          index,
                          current->ptr,
                          current->size,
                          current->file ? current->file : "未知",
                          current->line,
                          time_str);
        
        if (written > 0 && (size_t)written < remaining)
        {
            ptr += written;
            remaining -= written;
            total_leaked_bytes += current->size;
        }
        
        current = current->next;
        index++;
    }
    
    // 添加总结信息
    if (remaining > 0)
    {
        snprintf(ptr, remaining,
                "\n总结:\n"
                "  总泄漏数量: %zu\n"
                "  总泄漏字节数: %zu\n"
                "  当前活动分配: %zu\n"
                "  峰值活动分配: %zu\n",
                leak_count,
                total_leaked_bytes,
                data->current_allocations,
                data->peak_allocations);
    }
}

/**
 * @brief 清除泄漏记录
 */
void F_clear_leak_records(Stru_LeakDetectorContext_t* detector)
{
    if (detector == NULL)
    {
        return;
    }
    
    Stru_LeakDetectorData_t* data = (Stru_LeakDetectorData_t*)detector;
    
    // 释放所有分配记录
    Stru_AllocationRecord_t* current = data->allocations;
    
    while (current != NULL)
    {
        Stru_AllocationRecord_t* next = current->next;
        free(current);
        current = next;
    }
    
    data->allocations = NULL;
    data->current_allocations = 0;
    // 注意：不清除总分配/释放次数，这些是历史统计
}

/**
 * @brief 获取活动分配数量
 */
size_t F_get_active_allocations(Stru_LeakDetectorContext_t* detector)
{
    if (detector == NULL)
    {
        return 0;
    }
    
    Stru_LeakDetectorData_t* data = (Stru_LeakDetectorData_t*)detector;
    return data->current_allocations;
}

/**
 * @brief 获取总分配内存
 */
size_t F_get_total_allocated_memory(Stru_LeakDetectorContext_t* detector)
{
    if (detector == NULL)
    {
        return 0;
    }
    
    Stru_LeakDetectorData_t* data = (Stru_LeakDetectorData_t*)detector;
    return data->total_allocated_bytes;
}
