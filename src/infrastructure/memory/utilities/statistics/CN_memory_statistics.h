/**
 * @file CN_memory_statistics.h
 * @brief 内存统计工具接口
 * 
 * 提供内存使用统计和分析功能，包括内存使用统计、碎片分析和性能监控。
 * 帮助开发者了解内存使用情况，优化内存管理策略。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_STATISTICS_H
#define CN_MEMORY_STATISTICS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 内存统计信息结构体
 */
typedef struct
{
    size_t total_allocated;      ///< 总分配字节数
    size_t total_freed;          ///< 总释放字节数
    size_t current_usage;        ///< 当前使用字节数
    size_t peak_usage;           ///< 峰值使用字节数
    size_t allocation_count;     ///< 分配次数
    size_t free_count;           ///< 释放次数
    size_t failed_allocations;   ///< 分配失败次数
    double average_allocation;   ///< 平均分配大小
    size_t largest_allocation;   ///< 最大单次分配大小
    size_t smallest_allocation;  ///< 最小单次分配大小
} Stru_MemoryStatistics_t;

/**
 * @brief 内存碎片信息结构体
 */
typedef struct
{
    size_t total_fragmentation;      ///< 总碎片大小
    size_t external_fragmentation;   ///< 外部碎片大小
    size_t internal_fragmentation;   ///< 内部碎片大小
    double fragmentation_percentage; ///< 碎片百分比
    size_t free_blocks;              ///< 空闲块数量
    size_t used_blocks;              ///< 使用块数量
    size_t largest_free_block;       ///< 最大空闲块大小
    size_t smallest_free_block;      ///< 最小空闲块大小
} Stru_MemoryFragmentation_t;

/**
 * @brief 内存性能信息结构体
 */
typedef struct
{
    uint64_t total_alloc_time_ns;    ///< 总分配时间（纳秒）
    uint64_t total_free_time_ns;     ///< 总释放时间（纳秒）
    uint64_t peak_alloc_time_ns;     ///< 单次分配最长时间
    uint64_t peak_free_time_ns;      ///< 单次释放最长时间
    double average_alloc_time_ns;    ///< 平均分配时间
    double average_free_time_ns;     ///< 平均释放时间
    size_t allocations_per_second;   ///< 每秒分配次数
    size_t frees_per_second;         ///< 每秒释放次数
} Stru_MemoryPerformance_t;

/**
 * @brief 内存统计接口结构体
 * 
 * 定义了内存统计的核心功能，支持内存使用统计、碎片分析和性能监控。
 */
typedef struct Stru_MemoryStatisticsInterface_t Stru_MemoryStatisticsInterface_t;

/**
 * @brief 内存统计接口
 */
struct Stru_MemoryStatisticsInterface_t
{
    /**
     * @brief 获取内存统计信息
     * 
     * @param stats 输出参数：内存统计信息
     */
    void (*get_statistics)(Stru_MemoryStatistics_t* stats);
    
    /**
     * @brief 获取内存碎片信息
     * 
     * @param frag 输出参数：内存碎片信息
     */
    void (*get_fragmentation)(Stru_MemoryFragmentation_t* frag);
    
    /**
     * @brief 获取内存性能信息
     * 
     * @param perf 输出参数：内存性能信息
     */
    void (*get_performance)(Stru_MemoryPerformance_t* perf);
    
    /**
     * @brief 重置统计信息
     * 
     * 重置所有统计计数器。
     */
    void (*reset_statistics)(void);
    
    /**
     * @brief 生成统计报告
     * 
     * 生成格式化的统计报告字符串。
     * 
     * @param buffer 输出缓冲区
     * @param buffer_size 缓冲区大小
     * @return bool 成功返回true，否则返回false
     */
    bool (*generate_report)(char* buffer, size_t buffer_size);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

/**
 * @brief 创建内存统计接口实例
 * 
 * 创建默认的内存统计接口实例。
 * 
 * @return Stru_MemoryStatisticsInterface_t* 内存统计接口指针
 */
Stru_MemoryStatisticsInterface_t* F_create_memory_statistics(void);

/**
 * @brief 获取全局内存统计接口实例
 * 
 * 返回全局的内存统计接口单例实例。
 * 如果尚未创建，则创建新的实例。
 * 
 * @return Stru_MemoryStatisticsInterface_t* 全局内存统计接口实例
 */
Stru_MemoryStatisticsInterface_t* F_get_global_memory_statistics(void);

/**
 * @brief 销毁内存统计接口实例
 * 
 * 释放内存统计接口实例占用的资源。
 * 
 * @param statistics 要销毁的内存统计接口
 */
void F_destroy_memory_statistics(Stru_MemoryStatisticsInterface_t* statistics);

#ifdef __cplusplus
}
#endif

#endif // CN_MEMORY_STATISTICS_H
