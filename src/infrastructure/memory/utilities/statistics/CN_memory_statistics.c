/**
 * @file CN_memory_statistics.c
 * @brief 内存统计工具实现
 * 
 * 实现了内存使用统计和分析功能，包括内存使用统计、碎片分析和性能监控。
 * 帮助开发者了解内存使用情况，优化内存管理策略。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_memory_statistics.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

// 简单的时间获取函数，避免平台特定的时钟定义
static uint64_t get_current_time_ns_simple(void)
{
#ifdef _WIN32
    // Windows平台 - 使用GetTickCount64，精度较低但简单可靠
    return (uint64_t)GetTickCount64() * 1000000ULL; // 转换为纳秒
#else
    // Unix-like平台 - 使用clock()函数，跨平台兼容
    clock_t clk = clock();
    return (uint64_t)clk * (1000000000ULL / CLOCKS_PER_SEC);
#endif
}

/**
 * @brief 内存统计接口实现结构体
 */
typedef struct MemoryStatisticsImpl_t
{
    Stru_MemoryStatisticsInterface_t iface;  /* 接口定义 */
    
    // 统计信息
    Stru_MemoryStatistics_t stats;
    Stru_MemoryFragmentation_t frag;
    Stru_MemoryPerformance_t perf;
    
    // 时间跟踪
    uint64_t start_time_ns;
    size_t last_alloc_size;
    uint64_t last_alloc_time_ns;
    
} MemoryStatisticsImpl;

/**
 * @brief 获取当前时间（纳秒）
 */
static uint64_t get_current_time_ns(void)
{
    // 使用简单的时间获取函数
    return get_current_time_ns_simple();
}

/**
 * @brief 获取内存统计信息实现
 */
static void get_statistics_impl(Stru_MemoryStatistics_t* out_stats)
{
    if (out_stats == NULL)
    {
        return;
    }
    
    // 这里应该从实际的分配器获取统计信息
    // 目前返回默认值
    *out_stats = (Stru_MemoryStatistics_t){
        .total_allocated = 0,
        .total_freed = 0,
        .current_usage = 0,
        .peak_usage = 0,
        .allocation_count = 0,
        .free_count = 0,
        .failed_allocations = 0,
        .average_allocation = 0.0,
        .largest_allocation = 0,
        .smallest_allocation = 0
    };
}

/**
 * @brief 获取内存碎片信息实现
 */
static void get_fragmentation_impl(Stru_MemoryFragmentation_t* out_frag)
{
    if (out_frag == NULL)
    {
        return;
    }
    
    // 这里应该从实际的分配器获取碎片信息
    // 目前返回默认值
    *out_frag = (Stru_MemoryFragmentation_t){
        .total_fragmentation = 0,
        .external_fragmentation = 0,
        .internal_fragmentation = 0,
        .fragmentation_percentage = 0.0,
        .free_blocks = 0,
        .used_blocks = 0,
        .largest_free_block = 0,
        .smallest_free_block = 0
    };
}

/**
 * @brief 获取内存性能信息实现
 */
static void get_performance_impl(Stru_MemoryPerformance_t* out_perf)
{
    if (out_perf == NULL)
    {
        return;
    }
    
    // 这里应该从实际的分配器获取性能信息
    // 目前返回默认值
    *out_perf = (Stru_MemoryPerformance_t){
        .total_alloc_time_ns = 0,
        .total_free_time_ns = 0,
        .peak_alloc_time_ns = 0,
        .peak_free_time_ns = 0,
        .average_alloc_time_ns = 0.0,
        .average_free_time_ns = 0.0,
        .allocations_per_second = 0,
        .frees_per_second = 0
    };
}

/**
 * @brief 重置统计信息实现
 */
static void reset_statistics_impl(void)
{
    // 重置所有统计信息
    // 在实际实现中，应该重置分配器的统计信息
}

/**
 * @brief 生成统计报告实现
 */
static bool generate_report_impl(char* buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0)
    {
        return false;
    }
    
    Stru_MemoryStatistics_t stats;
    Stru_MemoryFragmentation_t frag;
    Stru_MemoryPerformance_t perf;
    
    get_statistics_impl(&stats);
    get_fragmentation_impl(&frag);
    get_performance_impl(&perf);
    
    int written = snprintf(buffer, buffer_size,
        "内存统计报告\n"
        "============\n\n"
        "使用统计:\n"
        "  总分配: %zu 字节\n"
        "  总释放: %zu 字节\n"
        "  当前使用: %zu 字节\n"
        "  峰值使用: %zu 字节\n"
        "  分配次数: %zu\n"
        "  释放次数: %zu\n"
        "  分配失败: %zu\n\n"
        "碎片分析:\n"
        "  总碎片: %zu 字节\n"
        "  外部碎片: %zu 字节\n"
        "  内部碎片: %zu 字节\n"
        "  碎片率: %.2f%%\n"
        "  空闲块: %zu\n"
        "  使用块: %zu\n\n"
        "性能监控:\n"
        "  平均分配时间: %.2f ns\n"
        "  平均释放时间: %.2f ns\n"
        "  分配/秒: %zu\n"
        "  释放/秒: %zu\n",
        stats.total_allocated, stats.total_freed, stats.current_usage,
        stats.peak_usage, stats.allocation_count, stats.free_count,
        stats.failed_allocations,
        frag.total_fragmentation, frag.external_fragmentation,
        frag.internal_fragmentation, frag.fragmentation_percentage,
        frag.free_blocks, frag.used_blocks,
        perf.average_alloc_time_ns, perf.average_free_time_ns,
        perf.allocations_per_second, perf.frees_per_second);
    
    return written > 0 && (size_t)written < buffer_size;
}

/**
 * @brief 创建内存统计接口实例
 */
Stru_MemoryStatisticsInterface_t* F_create_memory_statistics(void)
{
    MemoryStatisticsImpl* impl = (MemoryStatisticsImpl*)malloc(sizeof(MemoryStatisticsImpl));
    if (impl == NULL)
    {
        return NULL;
    }
    
    // 初始化统计信息
    memset(&impl->stats, 0, sizeof(impl->stats));
    memset(&impl->frag, 0, sizeof(impl->frag));
    memset(&impl->perf, 0, sizeof(impl->perf));
    
    impl->start_time_ns = get_current_time_ns();
    impl->last_alloc_size = 0;
    impl->last_alloc_time_ns = 0;
    
    // 初始化接口函数指针
    impl->iface.get_statistics = get_statistics_impl;
    impl->iface.get_fragmentation = get_fragmentation_impl;
    impl->iface.get_performance = get_performance_impl;
    impl->iface.reset_statistics = reset_statistics_impl;
    impl->iface.generate_report = generate_report_impl;
    impl->iface.private_data = impl;
    
    return (Stru_MemoryStatisticsInterface_t*)impl;
}

/**
 * @brief 全局内存统计接口实例
 */
static Stru_MemoryStatisticsInterface_t* g_global_memory_statistics = NULL;

/**
 * @brief 获取全局内存统计接口实例
 */
Stru_MemoryStatisticsInterface_t* F_get_global_memory_statistics(void)
{
    if (g_global_memory_statistics == NULL)
    {
        g_global_memory_statistics = F_create_memory_statistics();
    }
    return g_global_memory_statistics;
}

/**
 * @brief 销毁内存统计接口实例
 */
void F_destroy_memory_statistics(Stru_MemoryStatisticsInterface_t* statistics)
{
    if (statistics != NULL)
    {
        free(statistics);
        
        // 如果是全局实例，重置指针
        if (statistics == g_global_memory_statistics)
        {
            g_global_memory_statistics = NULL;
        }
    }
}
