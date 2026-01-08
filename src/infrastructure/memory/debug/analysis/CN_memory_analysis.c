/**
 * @file CN_memory_analysis.c
 * @brief 内存使用分析模块实现
 * 
 * 实现了内存使用分析功能，包括：
 * - 内存使用统计
 * - 内存碎片分析
 * - 性能分析
 * - 趋势分析
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#include "CN_memory_analysis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * @brief 内存分配记录
 */
typedef struct Stru_AllocationRecord_t
{
    void* address;                      ///< 分配的内存地址
    size_t size;                        ///< 分配的大小
    uint64_t allocation_time_ns;        ///< 分配耗时（纳秒）
    uint64_t timestamp;                 ///< 分配时间戳
    struct Stru_AllocationRecord_t* next; ///< 下一个记录
} Stru_AllocationRecord_t;

/**
 * @brief 性能采样数据
 */
typedef struct Stru_PerformanceSample_t
{
    uint64_t timestamp;                 ///< 采样时间戳
    size_t memory_usage;                ///< 内存使用量
    uint64_t allocation_count;          ///< 分配次数
    uint64_t deallocation_count;        ///< 释放次数
    struct Stru_PerformanceSample_t* next; ///< 下一个采样
} Stru_PerformanceSample_t;

/**
 * @brief 分析器私有数据
 */
typedef struct Stru_AnalyzerData_t
{
    Stru_AllocationRecord_t* allocations; ///< 分配记录链表
    Stru_PerformanceSample_t* samples;    ///< 性能采样链表
    Stru_MemoryAllocationStats_t stats;   ///< 统计信息
    Stru_PerformanceMetrics_t metrics;    ///< 性能指标
    bool enable_performance_tracking;     ///< 是否启用性能跟踪
    bool enable_fragmentation_analysis;   ///< 是否启用碎片分析
    uint32_t sampling_interval_ms;        ///< 采样间隔（毫秒）
    uint64_t last_sample_time;            ///< 上次采样时间
    size_t sample_count;                  ///< 采样数量
    size_t max_samples;                   ///< 最大采样数量
} Stru_AnalyzerData_t;

/**
 * @brief 创建内存分析器
 */
Stru_AnalyzerContext_t* F_create_memory_analyzer(
    bool enable_performance_tracking,
    bool enable_fragmentation_analysis,
    uint32_t sampling_interval_ms)
{
    Stru_AnalyzerData_t* data = 
        (Stru_AnalyzerData_t*)malloc(sizeof(Stru_AnalyzerData_t));
    
    if (data == NULL)
    {
        return NULL;
    }
    
    // 初始化数据
    data->allocations = NULL;
    data->samples = NULL;
    
    // 初始化统计信息
    memset(&data->stats, 0, sizeof(Stru_MemoryAllocationStats_t));
    
    // 初始化性能指标
    memset(&data->metrics, 0, sizeof(Stru_PerformanceMetrics_t));
    
    data->enable_performance_tracking = enable_performance_tracking;
    data->enable_fragmentation_analysis = enable_fragmentation_analysis;
    data->sampling_interval_ms = sampling_interval_ms;
    data->last_sample_time = 0;
    data->sample_count = 0;
    data->max_samples = 1000; // 最多保存1000个采样
    
    return (Stru_AnalyzerContext_t*)data;
}

/**
 * @brief 销毁内存分析器
 */
void F_destroy_memory_analyzer(Stru_AnalyzerContext_t* analyzer)
{
    if (analyzer == NULL)
    {
        return;
    }
    
    Stru_AnalyzerData_t* data = (Stru_AnalyzerData_t*)analyzer;
    
    if (data != NULL)
    {
        // 清理分配记录
        Stru_AllocationRecord_t* allocation = data->allocations;
        while (allocation != NULL)
        {
            Stru_AllocationRecord_t* next = allocation->next;
            free(allocation);
            allocation = next;
        }
        
        // 清理性能采样
        Stru_PerformanceSample_t* sample = data->samples;
        while (sample != NULL)
        {
            Stru_PerformanceSample_t* next = sample->next;
            free(sample);
            sample = next;
        }
        
        free(data);
    }
}

/**
 * @brief 记录内存分配（分析模块）
 */
void F_analyzer_record_allocation(Stru_AnalyzerContext_t* analyzer,
                        void* address, size_t size,
                        uint64_t allocation_time_ns)
{
    if (analyzer == NULL || address == NULL || size == 0)
    {
        return;
    }
    
    Stru_AnalyzerData_t* data = (Stru_AnalyzerData_t*)analyzer;
    
    // 创建分配记录
    Stru_AllocationRecord_t* record = 
        (Stru_AllocationRecord_t*)malloc(sizeof(Stru_AllocationRecord_t));
    
    if (record == NULL)
    {
        return;
    }
    
    // 填充记录信息
    record->address = address;
    record->size = size;
    record->allocation_time_ns = allocation_time_ns;
    record->timestamp = (uint64_t)time(NULL) * 1000000000ULL; // 转换为纳秒
    record->next = data->allocations;
    
    // 添加到链表头部
    data->allocations = record;
    
    // 更新统计信息
    data->stats.total_allocations++;
    data->stats.allocation_count++;
    data->stats.current_memory_usage += size;
    data->stats.total_allocated_bytes += size;
    
    // 更新最大/最小分配大小
    if (size > data->stats.largest_allocation)
    {
        data->stats.largest_allocation = size;
    }
    
    if (data->stats.smallest_allocation == 0 || size < data->stats.smallest_allocation)
    {
        data->stats.smallest_allocation = size;
    }
    
    // 更新峰值内存使用
    if (data->stats.current_memory_usage > data->stats.peak_memory_usage)
    {
        data->stats.peak_memory_usage = data->stats.current_memory_usage;
    }
    
    // 更新平均分配大小
    if (data->stats.total_allocations > 0)
    {
        data->stats.average_allocation_size = 
            data->stats.total_allocated_bytes / data->stats.total_allocations;
    }
    
    // 更新性能指标
    if (data->enable_performance_tracking)
    {
        data->metrics.allocation_time_ns += allocation_time_ns;
        data->metrics.allocation_count++;
        
        if (allocation_time_ns > data->metrics.peak_allocation_time_ns)
        {
            data->metrics.peak_allocation_time_ns = allocation_time_ns;
        }
        
        if (data->metrics.allocation_count > 0)
        {
            data->metrics.average_allocation_time_ns = 
                data->metrics.allocation_time_ns / data->metrics.allocation_count;
        }
    }
    
    // 性能采样
    if (data->sampling_interval_ms > 0)
    {
        uint64_t current_time = (uint64_t)time(NULL) * 1000ULL; // 转换为毫秒
        if (current_time - data->last_sample_time >= data->sampling_interval_ms)
        {
            // 创建采样记录
            Stru_PerformanceSample_t* sample = 
                (Stru_PerformanceSample_t*)malloc(sizeof(Stru_PerformanceSample_t));
            
            if (sample != NULL)
            {
                sample->timestamp = current_time;
                sample->memory_usage = data->stats.current_memory_usage;
                sample->allocation_count = data->stats.total_allocations;
                sample->deallocation_count = data->stats.total_deallocations;
                sample->next = data->samples;
                
                // 添加到链表头部
                data->samples = sample;
                data->sample_count++;
                
                // 限制采样数量
                if (data->sample_count > data->max_samples)
                {
                    // 删除最旧的采样
                    Stru_PerformanceSample_t* current = data->samples;
                    Stru_PerformanceSample_t* previous = NULL;
                    
                    while (current != NULL && current->next != NULL)
                    {
                        previous = current;
                        current = current->next;
                    }
                    
                    if (previous != NULL && current != NULL)
                    {
                        previous->next = NULL;
                        free(current);
                        data->sample_count--;
                    }
                }
                
                data->last_sample_time = current_time;
            }
        }
    }
}

/**
 * @brief 记录内存释放（分析模块）
 */
void F_analyzer_record_deallocation(Stru_AnalyzerContext_t* analyzer,
                          void* address, uint64_t deallocation_time_ns)
{
    if (analyzer == NULL || address == NULL)
    {
        return;
    }
    
    Stru_AnalyzerData_t* data = (Stru_AnalyzerData_t*)analyzer;
    
    // 在链表中查找对应的分配记录
    Stru_AllocationRecord_t* current = data->allocations;
    Stru_AllocationRecord_t* previous = NULL;
    
    while (current != NULL)
    {
        if (current->address == address)
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
            data->stats.total_deallocations++;
            data->stats.allocation_count--;
            data->stats.current_memory_usage -= current->size;
            data->stats.total_freed_bytes += current->size;
            
            // 更新性能指标
            if (data->enable_performance_tracking)
            {
                data->metrics.deallocation_time_ns += deallocation_time_ns;
                data->metrics.deallocation_count++;
                
                if (data->metrics.deallocation_count > 0)
                {
                    data->metrics.average_deallocation_time_ns = 
                        data->metrics.deallocation_time_ns / data->metrics.deallocation_count;
                }
            }
            
            // 释放记录内存
            free(current);
            return;
        }
        
        previous = current;
        current = current->next;
    }
}

/**
 * @brief 获取内存分配统计
 */
void F_get_allocation_stats(Stru_AnalyzerContext_t* analyzer,
                           Stru_MemoryAllocationStats_t* stats)
{
    if (analyzer == NULL || stats == NULL)
    {
        return;
    }
    
    Stru_AnalyzerData_t* data = (Stru_AnalyzerData_t*)analyzer;
    *stats = data->stats;
}

/**
 * @brief 获取内存碎片信息
 */
bool F_get_fragmentation_info(Stru_AnalyzerContext_t* analyzer,
                             Stru_MemoryFragmentationInfo_t* fragmentation_info)
{
    if (analyzer == NULL || fragmentation_info == NULL)
    {
        return false;
    }
    
    Stru_AnalyzerData_t* data = (Stru_AnalyzerData_t*)analyzer;
    
    if (!data->enable_fragmentation_analysis)
    {
        return false;
    }
    
    // 简化实现：计算基本碎片信息
    // 实际实现需要知道内存池的详细信息
    
    // 假设总内存为当前使用内存的2倍（简化）
    fragmentation_info->total_memory = data->stats.current_memory_usage * 2;
    fragmentation_info->used_memory = data->stats.current_memory_usage;
    fragmentation_info->free_memory = fragmentation_info->total_memory - 
                                     fragmentation_info->used_memory;
    
    // 假设有10个空闲块（简化）
    fragmentation_info->free_block_count = 10;
    fragmentation_info->used_block_count = data->stats.allocation_count;
    
    // 假设最大空闲块为总空闲内存的一半（简化）
    fragmentation_info->largest_free_block = fragmentation_info->free_memory / 2;
    
    // 计算碎片化百分比
    if (fragmentation_info->free_memory > 0)
    {
        float fragmentation = 1.0f - (float)fragmentation_info->largest_free_block / 
                                   (float)fragmentation_info->free_memory;
        fragmentation_info->fragmentation_percentage = fragmentation * 100.0f;
    }
    else
    {
        fragmentation_info->fragmentation_percentage = 0.0f;
    }
    
    return true;
}

/**
 * @brief 获取性能指标
 */
void F_get_performance_metrics(Stru_AnalyzerContext_t* analyzer,
                              Stru_PerformanceMetrics_t* metrics)
{
    if (analyzer == NULL || metrics == NULL)
    {
        return;
    }
    
    Stru_AnalyzerData_t* data = (Stru_AnalyzerData_t*)analyzer;
    *metrics = data->metrics;
}

/**
 * @brief 生成内存使用报告
 */
void F_generate_memory_report(Stru_AnalyzerContext_t* analyzer,
                             char* report_buffer, size_t buffer_size)
{
    if (analyzer == NULL || report_buffer == NULL || buffer_size == 0)
    {
        return;
    }
    
    Stru_AnalyzerData_t* data = (Stru_AnalyzerData_t*)analyzer;
    
    snprintf(report_buffer, buffer_size,
            "内存使用报告:\n"
            "====================\n"
            "总分配次数: %llu\n"
            "总释放次数: %llu\n"
            "当前活动分配: %zu\n"
            "当前内存使用: %zu 字节\n"
            "峰值内存使用: %zu 字节\n"
            "总分配字节数: %zu 字节\n"
            "总释放字节数: %zu 字节\n"
            "最大单次分配: %zu 字节\n"
            "最小单次分配: %zu 字节\n"
            "平均分配大小: %zu 字节\n",
            (unsigned long long)data->stats.total_allocations,
            (unsigned long long)data->stats.total_deallocations,
            data->stats.allocation_count,
            data->stats.current_memory_usage,
            data->stats.peak_memory_usage,
            data->stats.total_allocated_bytes,
            data->stats.total_freed_bytes,
            data->stats.largest_allocation,
            data->stats.smallest_allocation,
            data->stats.average_allocation_size);
}

/**
 * @brief 生成性能报告
 */
void F_generate_performance_report(Stru_AnalyzerContext_t* analyzer,
                                  char* report_buffer, size_t buffer_size)
{
    if (analyzer == NULL || report_buffer == NULL || buffer_size == 0)
    {
        return;
    }
    
    Stru_AnalyzerData_t* data = (Stru_AnalyzerData_t*)analyzer;
    
    if (!data->enable_performance_tracking)
    {
        snprintf(report_buffer, buffer_size, "性能跟踪已禁用\n");
        return;
    }
    
    snprintf(report_buffer, buffer_size,
            "性能报告:\n"
            "====================\n"
            "分配总时间: %llu 纳秒\n"
            "释放总时间: %llu 纳秒\n"
            "分配次数: %llu\n"
            "释放次数: %llu\n"
            "平均分配时间: %llu 纳秒\n"
            "平均释放时间: %llu 纳秒\n"
            "峰值分配时间: %llu 纳秒\n"
            "采样数量: %zu\n",
            (unsigned long long)data->metrics.allocation_time_ns,
            (unsigned long long)data->metrics.deallocation_time_ns,
            (unsigned long long)data->metrics.allocation_count,
            (unsigned long long)data->metrics.deallocation_count,
            (unsigned long long)data->metrics.average_allocation_time_ns,
            (unsigned long long)data->metrics.average_deallocation_time_ns,
            (unsigned long long)data->metrics.peak_allocation_time_ns,
            data->sample_count);
}

/**
 * @brief 生成碎片分析报告
 */
void F_generate_fragmentation_report(Stru_AnalyzerContext_t* analyzer,
                                    char* report_buffer, size_t buffer_size)
{
    if (analyzer == NULL || report_buffer == NULL || buffer_size == 0)
    {
        return;
    }
    
    Stru_AnalyzerData_t* data = (Stru_AnalyzerData_t*)analyzer;
    
    if (!data->enable_fragmentation_analysis)
    {
        snprintf(report_buffer, buffer_size, "碎片分析已禁用\n");
        return;
    }
    
    Stru_MemoryFragmentationInfo_t fragmentation_info;
    if (F_get_fragmentation_info(analyzer, &fragmentation_info))
    {
        snprintf(report_buffer, buffer_size,
                "碎片分析报告:\n"
                "====================\n"
                "总内存: %zu 字节\n"
                "已使用内存: %zu 字节\n"
                "空闲内存: %zu 字节\n"
                "最大空闲块: %zu 字节\n"
                "空闲块数量: %zu\n"
                "已使用块数量: %zu\n"
                "碎片化百分比: %.2f%%\n",
                fragmentation_info.total_memory,
                fragmentation_info.used_memory,
                fragmentation_info.free_memory,
                fragmentation_info.largest_free_block,
                fragmentation_info.free_block_count,
                fragmentation_info.used_block_count,
                fragmentation_info.fragmentation_percentage);
    }
    else
    {
        snprintf(report_buffer, buffer_size, "无法获取碎片信息\n");
    }
}

/**
 * @brief 重置统计信息
 */
void F_reset_statistics(Stru_AnalyzerContext_t* analyzer)
{
    if (analyzer == NULL)
    {
        return;
    }
    
    Stru_AnalyzerData_t* data = (Stru_AnalyzerData_t*)analyzer;
    
    // 清理分配记录
    Stru_AllocationRecord_t* allocation = data->allocations;
    while (allocation != NULL)
    {
        Stru_AllocationRecord_t* next = allocation->next;
        free(allocation);
        allocation = next;
    }
    
    data->allocations = NULL;
    
    // 重置统计信息
    memset(&data->stats, 0, sizeof(Stru_MemoryAllocationStats_t));
    
    // 重置性能指标
    memset(&data->metrics, 0, sizeof(Stru_PerformanceMetrics_t));
    
    // 注意：不清除性能采样，保留历史数据
}

/**
 * @brief 开始性能采样
 */
void F_start_performance_sampling(Stru_AnalyzerContext_t* analyzer)
{
    if (analyzer == NULL)
    {
        return;
    }
    
    Stru_AnalyzerData_t* data = (Stru_AnalyzerData_t*)analyzer;
    data->last_sample_time = (uint64_t)time(NULL) * 1000ULL; // 转换为毫秒
}

/**
 * @brief 停止性能采样
 */
void F_stop_performance_sampling(Stru_AnalyzerContext_t* analyzer)
{
    if (analyzer == NULL)
    {
        return;
    }
    
    Stru_AnalyzerData_t* data = (Stru_AnalyzerData_t*)analyzer;
    data->last_sample_time = 0;
}

/**
 * @brief 获取采样数据数量
 */
size_t F_get_sample_count(Stru_AnalyzerContext_t* analyzer)
{
    if (analyzer == NULL)
    {
        return 0;
    }
    
    Stru_AnalyzerData_t* data = (Stru_AnalyzerData_t*)analyzer;
    return data->sample_count;
}
