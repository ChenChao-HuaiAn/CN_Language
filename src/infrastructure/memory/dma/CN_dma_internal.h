/******************************************************************************
 * 文件名: CN_dma_internal.h
 * 功能: CN_Language DMA内存分配器内部接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建DMA内存分配器内部接口，支持模块化拆分
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_DMA_INTERNAL_H
#define CN_DMA_INTERNAL_H

#include "CN_dma_allocator.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// DMA内存分配器内部结构声明
// ============================================================================

/**
 * @brief DMA内存分配器内部结构
 * 
 * 使用位图跟踪页面分配状态，支持DMA特定功能。
 */
struct Stru_CN_DmaAllocator_t
{
    Stru_CN_DmaConfig_t config;          /**< 分配器配置 */
    Stru_CN_DmaStats_t stats;            /**< 统计信息 */
    
    // 内存范围信息
    uintptr_t dma_region_start;          /**< DMA区域起始地址 */
    uintptr_t dma_region_end;            /**< DMA区域结束地址 */
    size_t total_pages;                  /**< 总页面数量 */
    
    // 位图管理
    uint8_t* bitmap;                     /**< 页面分配位图（1=已分配，0=空闲） */
    size_t bitmap_size;                  /**< 位图大小（字节） */
    
    // 调试和跟踪
    bool debug_enabled;                  /**< 调试模式是否启用 */
    CN_DmaDebugCallback_t debug_callback; /**< 调试回调函数 */
    void* debug_user_data;               /**< 调试回调用户数据 */
    
    // 分配跟踪（如果启用）
    Stru_CN_DmaAllocationInfo_t* allocations; /**< 分配信息数组 */
    size_t max_allocations;              /**< 最大跟踪分配数 */
    size_t current_allocations;          /**< 当前跟踪分配数 */
    
    // 内部状态
    size_t next_search_index;            /**< 下一次搜索的起始索引（优化） */
    bool initialized;                    /**< 分配器是否已初始化 */
    uint32_t next_handle;                /**< 下一个分配句柄 */
    
    // DMA特定状态
    bool cache_coherent;                 /**< 是否支持缓存一致性 */
    size_t cache_line_size;              /**< 缓存行大小 */
};

// ============================================================================
// 内部辅助函数声明
// ============================================================================

/**
 * @brief 输出调试信息
 */
void CN_dma_debug_log(Stru_CN_DmaAllocator_t* allocator, const char* format, ...);

/**
 * @brief 验证分配器是否已初始化
 */
bool CN_dma_validate_allocator(Stru_CN_DmaAllocator_t* allocator);

/**
 * @brief 地址转换为页面索引
 */
size_t CN_dma_address_to_page_index(Stru_CN_DmaAllocator_t* allocator, uintptr_t address);

/**
 * @brief 页面索引转换为地址
 */
uintptr_t CN_dma_page_index_to_address(Stru_CN_DmaAllocator_t* allocator, size_t page_index);

// ============================================================================
// 位图管理函数声明
// ============================================================================

/**
 * @brief 设置位图中的位
 */
void CN_dma_bitmap_set(Stru_CN_DmaAllocator_t* allocator, size_t page_index, bool allocated);

/**
 * @brief 获取位图中的位
 */
bool CN_dma_bitmap_get(Stru_CN_DmaAllocator_t* allocator, size_t page_index);

/**
 * @brief 查找连续空闲页面
 */
size_t CN_dma_find_free_pages(Stru_CN_DmaAllocator_t* allocator, size_t page_count);

/**
 * @brief 查找对齐的连续空闲页面
 */
size_t CN_dma_find_free_pages_aligned(Stru_CN_DmaAllocator_t* allocator, 
                                      size_t page_count, size_t alignment_pages);

// ============================================================================
// 分配跟踪函数声明
// ============================================================================

/**
 * @brief 添加分配跟踪记录
 */
void CN_dma_add_allocation_tracking(Stru_CN_DmaAllocator_t* allocator,
                                   Stru_CN_DmaBuffer_t* buffer,
                                   const char* file, int line, const char* purpose);

/**
 * @brief 移除分配跟踪记录
 */
void CN_dma_remove_allocation_tracking(Stru_CN_DmaAllocator_t* allocator,
                                      uint32_t handle);

// ============================================================================
// 缓存一致性函数声明
// ============================================================================

/**
 * @brief 根据内存属性设置缓存一致性标志
 */
bool CN_dma_determine_cache_coherence(Eum_CN_DmaMemoryAttribute_t attribute);

// ============================================================================
// 分散-聚集DMA函数声明
// ============================================================================

/**
 * @brief 分配分散-聚集DMA列表
 */
Stru_CN_DmaScatterGatherList_t* CN_dma_alloc_scatter_gather_internal(
    Stru_CN_DmaAllocator_t* allocator,
    size_t total_size,
    size_t max_elements,
    Eum_CN_DmaMemoryAttribute_t attribute,
    const char* file, int line,
    const char* purpose);

/**
 * @brief 释放分散-聚集DMA列表
 */
void CN_dma_free_scatter_gather_internal(
    Stru_CN_DmaAllocator_t* allocator,
    Stru_CN_DmaScatterGatherList_t* sg_list,
    const char* file, int line);

// ============================================================================
// 调试和统计函数声明
// ============================================================================

/**
 * @brief 获取分配器统计信息
 */
bool CN_dma_get_stats_internal(Stru_CN_DmaAllocator_t* allocator,
                              Stru_CN_DmaStats_t* stats);

/**
 * @brief 重置分配器统计信息
 */
void CN_dma_reset_stats_internal(Stru_CN_DmaAllocator_t* allocator);

/**
 * @brief 计算最大连续空闲块
 */
size_t CN_dma_largest_free_block_internal(Stru_CN_DmaAllocator_t* allocator);

/**
 * @brief 转储分配器状态
 */
void CN_dma_dump_internal(Stru_CN_DmaAllocator_t* allocator);

/**
 * @brief 转储内存泄漏信息
 */
void CN_dma_dump_leaks_internal(Stru_CN_DmaAllocator_t* allocator);

#ifdef __cplusplus
}
#endif

#endif // CN_DMA_INTERNAL_H
