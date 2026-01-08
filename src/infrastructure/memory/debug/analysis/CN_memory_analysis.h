/**
 * @file CN_memory_analysis.h
 * @brief 内存使用分析模块
 * 
 * 本模块提供了内存使用分析功能，包括：
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

#ifndef CN_MEMORY_ANALYSIS_H
#define CN_MEMORY_ANALYSIS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 内存分配统计
 */
typedef struct Stru_MemoryAllocationStats_t
{
    uint64_t total_allocations;      ///< 总分配次数
    uint64_t total_deallocations;    ///< 总释放次数
    size_t peak_memory_usage;        ///< 峰值内存使用量（字节）
    size_t current_memory_usage;     ///< 当前内存使用量（字节）
    size_t total_allocated_bytes;    ///< 总分配字节数
    size_t total_freed_bytes;        ///< 总释放字节数
    size_t allocation_count;         ///< 当前活动分配数量
    size_t largest_allocation;       ///< 最大单次分配大小
    size_t smallest_allocation;      ///< 最小单次分配大小
    size_t average_allocation_size;  ///< 平均分配大小
} Stru_MemoryAllocationStats_t;

/**
 * @brief 内存碎片信息
 */
typedef struct Stru_MemoryFragmentationInfo_t
{
    size_t total_memory;             ///< 总内存
    size_t used_memory;              ///< 已使用内存
    size_t free_memory;              ///< 空闲内存
    size_t largest_free_block;       ///< 最大空闲块
    size_t free_block_count;         ///< 空闲块数量
    size_t used_block_count;         ///< 已使用块数量
    float fragmentation_percentage;  ///< 碎片化百分比
} Stru_MemoryFragmentationInfo_t;

/**
 * @brief 性能指标
 */
typedef struct Stru_PerformanceMetrics_t
{
    uint64_t allocation_time_ns;     ///< 分配总时间（纳秒）
    uint64_t deallocation_time_ns;   ///< 释放总时间（纳秒）
    uint64_t allocation_count;       ///< 分配次数
    uint64_t deallocation_count;     ///< 释放次数
    uint64_t average_allocation_time_ns;  ///< 平均分配时间（纳秒）
    uint64_t average_deallocation_time_ns; ///< 平均释放时间（纳秒）
    uint64_t peak_allocation_time_ns;     ///< 峰值分配时间（纳秒）
} Stru_PerformanceMetrics_t;

/**
 * @brief 分析器上下文
 * 
 * 分析器的私有数据。
 */
typedef struct Stru_AnalyzerContext_t Stru_AnalyzerContext_t;

/**
 * @brief 创建内存分析器
 * 
 * 创建一个新的内存分析器实例。
 * 
 * @param enable_performance_tracking 是否启用性能跟踪
 * @param enable_fragmentation_analysis 是否启用碎片分析
 * @param sampling_interval_ms 采样间隔（毫秒，0表示禁用采样）
 * @return Stru_AnalyzerContext_t* 分析器上下文，失败返回NULL
 */
Stru_AnalyzerContext_t* F_create_memory_analyzer(
    bool enable_performance_tracking,
    bool enable_fragmentation_analysis,
    uint32_t sampling_interval_ms);

/**
 * @brief 销毁内存分析器
 * 
 * 销毁内存分析器实例，释放相关资源。
 * 
 * @param analyzer 要销毁的分析器
 */
void F_destroy_memory_analyzer(Stru_AnalyzerContext_t* analyzer);

/**
 * @brief 记录内存分配（分析模块）
 * 
 * 记录一次内存分配操作。
 * 
 * @param analyzer 分析器
 * @param address 分配的内存地址
 * @param size 分配的大小
 * @param allocation_time_ns 分配耗时（纳秒）
 */
void F_analyzer_record_allocation(Stru_AnalyzerContext_t* analyzer,
                        void* address, size_t size,
                        uint64_t allocation_time_ns);

/**
 * @brief 记录内存释放（分析模块）
 * 
 * 记录一次内存释放操作。
 * 
 * @param analyzer 分析器
 * @param address 释放的内存地址
 * @param deallocation_time_ns 释放耗时（纳秒）
 */
void F_analyzer_record_deallocation(Stru_AnalyzerContext_t* analyzer,
                          void* address, uint64_t deallocation_time_ns);

/**
 * @brief 获取内存分配统计
 * 
 * 获取当前的内存分配统计信息。
 * 
 * @param analyzer 分析器
 * @param stats 输出参数：统计信息
 */
void F_get_allocation_stats(Stru_AnalyzerContext_t* analyzer,
                           Stru_MemoryAllocationStats_t* stats);

/**
 * @brief 获取内存碎片信息
 * 
 * 获取当前的内存碎片信息。
 * 
 * @param analyzer 分析器
 * @param fragmentation_info 输出参数：碎片信息
 * @return bool 获取成功返回true，否则返回false
 */
bool F_get_fragmentation_info(Stru_AnalyzerContext_t* analyzer,
                             Stru_MemoryFragmentationInfo_t* fragmentation_info);

/**
 * @brief 获取性能指标
 * 
 * 获取当前的性能指标。
 * 
 * @param analyzer 分析器
 * @param metrics 输出参数：性能指标
 */
void F_get_performance_metrics(Stru_AnalyzerContext_t* analyzer,
                              Stru_PerformanceMetrics_t* metrics);

/**
 * @brief 生成内存使用报告
 * 
 * 生成详细的内存使用报告。
 * 
 * @param analyzer 分析器
 * @param report_buffer 报告缓冲区
 * @param buffer_size 缓冲区大小
 */
void F_generate_memory_report(Stru_AnalyzerContext_t* analyzer,
                             char* report_buffer, size_t buffer_size);

/**
 * @brief 生成性能报告
 * 
 * 生成详细的性能报告。
 * 
 * @param analyzer 分析器
 * @param report_buffer 报告缓冲区
 * @param buffer_size 缓冲区大小
 */
void F_generate_performance_report(Stru_AnalyzerContext_t* analyzer,
                                  char* report_buffer, size_t buffer_size);

/**
 * @brief 生成碎片分析报告
 * 
 * 生成详细的碎片分析报告。
 * 
 * @param analyzer 分析器
 * @param report_buffer 报告缓冲区
 * @param buffer_size 缓冲区大小
 */
void F_generate_fragmentation_report(Stru_AnalyzerContext_t* analyzer,
                                    char* report_buffer, size_t buffer_size);

/**
 * @brief 重置统计信息
 * 
 * 重置所有统计信息，重新开始统计。
 * 
 * @param analyzer 分析器
 */
void F_reset_statistics(Stru_AnalyzerContext_t* analyzer);

/**
 * @brief 开始性能采样
 * 
 * 开始性能采样。
 * 
 * @param analyzer 分析器
 */
void F_start_performance_sampling(Stru_AnalyzerContext_t* analyzer);

/**
 * @brief 停止性能采样
 * 
 * 停止性能采样。
 * 
 * @param analyzer 分析器
 */
void F_stop_performance_sampling(Stru_AnalyzerContext_t* analyzer);

/**
 * @brief 获取采样数据数量
 * 
 * 获取当前采样的数据数量。
 * 
 * @param analyzer 分析器
 * @return size_t 采样数据数量
 */
size_t F_get_sample_count(Stru_AnalyzerContext_t* analyzer);

#ifdef __cplusplus
}
#endif

#endif // CN_MEMORY_ANALYSIS_H
