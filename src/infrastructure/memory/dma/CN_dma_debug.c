/******************************************************************************
 * 文件名: CN_dma_debug.c
 * 功能: CN_Language DMA内存分配器调试和统计功能
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建DMA内存分配器调试和统计功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_dma_internal.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// 调试和统计函数实现
// ============================================================================

/**
 * @brief 获取分配器统计信息
 */
bool CN_dma_get_stats_internal(Stru_CN_DmaAllocator_t* allocator,
                              Stru_CN_DmaStats_t* stats)
{
    if (!CN_dma_validate_allocator(allocator) || stats == NULL)
    {
        return false;
    }
    
    // 计算碎片化程度
    if (allocator->stats.total_pages > 0)
    {
        // 简化实现：基于最大连续空闲块计算碎片化
        size_t largest_free = CN_dma_largest_free_block_internal(allocator);
        if (largest_free > 0)
        {
            allocator->stats.fragmentation = 100 - (largest_free * 100 / allocator->stats.free_pages);
        }
        else
        {
            allocator->stats.fragmentation = 100;
        }
    }
    
    *stats = allocator->stats;
    return true;
}

/**
 * @brief 重置分配器统计信息
 */
void CN_dma_reset_stats_internal(Stru_CN_DmaAllocator_t* allocator)
{
    if (!CN_dma_validate_allocator(allocator))
    {
        return;
    }
    
    memset(&allocator->stats, 0, sizeof(allocator->stats));
    allocator->stats.total_pages = allocator->total_pages;
    allocator->stats.free_pages = allocator->total_pages;
    
    CN_dma_debug_log(allocator, "统计信息已重置");
}

/**
 * @brief 计算最大连续空闲块
 */
size_t CN_dma_largest_free_block_internal(Stru_CN_DmaAllocator_t* allocator)
{
    if (!CN_dma_validate_allocator(allocator))
    {
        return 0;
    }
    
    size_t largest = 0;
    size_t current = 0;
    
    for (size_t i = 0; i < allocator->total_pages; i++)
    {
        if (!CN_dma_bitmap_get(allocator, i))
        {
            current++;
            if (current > largest)
            {
                largest = current;
            }
        }
        else
        {
            current = 0;
        }
    }
    
    allocator->stats.largest_free_block = largest;
    return largest;
}

/**
 * @brief 转储分配器状态
 */
void CN_dma_dump_internal(Stru_CN_DmaAllocator_t* allocator)
{
    if (!CN_dma_validate_allocator(allocator))
    {
        printf("无效的DMA内存分配器\n");
        return;
    }
    
    printf("=== DMA内存分配器状态转储 ===\n");
    printf("名称: %s\n", allocator->config.name);
    printf("DMA区域: 0x%llx - 0x%llx\n", 
           (unsigned long long)allocator->dma_region_start,
           (unsigned long long)allocator->dma_region_end);
    printf("页面大小: %zu 字节\n", allocator->config.page_size);
    printf("缓存行大小: %zu 字节\n", allocator->config.cache_line_size);
    printf("总页面数: %zu\n", allocator->total_pages);
    printf("空闲页面: %zu\n", allocator->stats.free_pages);
    printf("已用页面: %zu\n", allocator->stats.used_pages);
    printf("分配次数: %zu\n", allocator->stats.allocation_count);
    printf("释放次数: %zu\n", allocator->stats.free_count);
    printf("分配失败次数: %zu\n", allocator->stats.allocation_failures);
    printf("最大连续空闲块: %zu 页面\n", allocator->stats.largest_free_block);
    printf("碎片化程度: %zu%%\n", allocator->stats.fragmentation);
    printf("调试模式: %s\n", allocator->debug_enabled ? "启用" : "禁用");
    printf("跟踪分配: %s\n", allocator->config.track_allocations ? "启用" : "禁用");
    printf("当前跟踪分配数: %zu\n", allocator->current_allocations);
    printf("=============================\n");
}

/**
 * @brief 转储内存泄漏信息
 */
void CN_dma_dump_leaks_internal(Stru_CN_DmaAllocator_t* allocator)
{
    if (!CN_dma_validate_allocator(allocator))
    {
        printf("无效的DMA内存分配器\n");
        return;
    }
    
    if (!allocator->config.track_allocations || allocator->allocations == NULL)
    {
        printf("分配跟踪未启用\n");
        return;
    }
    
    if (allocator->current_allocations == 0)
    {
        printf("未检测到内存泄漏\n");
        return;
    }
    
    printf("=== 内存泄漏检测 ===\n");
    printf("检测到 %zu 个未释放的分配：\n", allocator->current_allocations);
    
    for (size_t i = 0; i < allocator->current_allocations; i++)
    {
        Stru_CN_DmaAllocationInfo_t* info = &allocator->allocations[i];
        printf("泄漏 #%zu:\n", i + 1);
        printf("  物理地址: 0x%llx\n", (unsigned long long)info->physical_address);
        printf("  虚拟地址: %p\n", info->virtual_address);
        printf("  大小: %zu 字节\n", info->size);
        printf("  对齐: %zu 字节\n", info->alignment);
        printf("  内存属性: %d\n", info->attribute);
        printf("  分配位置: %s:%d\n", info->file ? info->file : "未知", info->line);
        printf("  分配目的: %s\n", info->purpose ? info->purpose : "未知");
        printf("  分配句柄: %u\n", info->handle);
        printf("\n");
    }
    
    printf("========================\n");
}

// ============================================================================
// 公共接口包装函数
// ============================================================================

bool CN_dma_get_config(Stru_CN_DmaAllocator_t* allocator,
                       Stru_CN_DmaConfig_t* config)
{
    if (!CN_dma_validate_allocator(allocator) || config == NULL)
    {
        return false;
    }
    
    *config = allocator->config;
    return true;
}

bool CN_dma_get_stats(Stru_CN_DmaAllocator_t* allocator,
                      Stru_CN_DmaStats_t* stats)
{
    return CN_dma_get_stats_internal(allocator, stats);
}

void CN_dma_reset_stats(Stru_CN_DmaAllocator_t* allocator)
{
    CN_dma_reset_stats_internal(allocator);
}

size_t CN_dma_total_pages(Stru_CN_DmaAllocator_t* allocator)
{
    if (!CN_dma_validate_allocator(allocator))
    {
        return 0;
    }
    
    return allocator->total_pages;
}

size_t CN_dma_get_free_pages(Stru_CN_DmaAllocator_t* allocator)
{
    if (!CN_dma_validate_allocator(allocator))
    {
        return 0;
    }
    
    return allocator->stats.free_pages;
}

size_t CN_dma_used_pages(Stru_CN_DmaAllocator_t* allocator)
{
    if (!CN_dma_validate_allocator(allocator))
    {
        return 0;
    }
    
    return allocator->stats.used_pages;
}

size_t CN_dma_largest_free_block(Stru_CN_DmaAllocator_t* allocator)
{
    return CN_dma_largest_free_block_internal(allocator);
}

void CN_dma_enable_debug(Stru_CN_DmaAllocator_t* allocator, bool enable)
{
    if (!CN_dma_validate_allocator(allocator))
    {
        return;
    }
    
    allocator->debug_enabled = enable;
    CN_dma_debug_log(allocator, "调试模式%s", enable ? "启用" : "禁用");
}

bool CN_dma_validate(Stru_CN_DmaAllocator_t* allocator)
{
    return CN_dma_validate_allocator(allocator);
}

void CN_dma_dump(Stru_CN_DmaAllocator_t* allocator)
{
    CN_dma_dump_internal(allocator);
}

void CN_dma_dump_leaks(Stru_CN_DmaAllocator_t* allocator)
{
    CN_dma_dump_leaks_internal(allocator);
}

void CN_dma_set_debug_callback(Stru_CN_DmaAllocator_t* allocator,
                               CN_DmaDebugCallback_t callback,
                               void* user_data)
{
    if (!CN_dma_validate_allocator(allocator))
    {
        return;
    }
    
    allocator->debug_callback = callback;
    allocator->debug_user_data = user_data;
    
    if (callback != NULL)
    {
        CN_dma_debug_log(allocator, "调试回调函数已设置");
    }
    else
    {
        CN_dma_debug_log(allocator, "调试回调函数已清除");
    }
}

Stru_CN_DmaBuffer_t* CN_dma_alloc_contiguous(Stru_CN_DmaAllocator_t* allocator,
                                             size_t size,
                                             size_t alignment,
                                             Eum_CN_DmaMemoryAttribute_t attribute,
                                             const char* file, int line,
                                             const char* purpose)
{
    // 对于DMA分配器，所有分配都是物理连续的
    // 所以直接调用CN_dma_alloc_buffer
    return CN_dma_alloc_buffer(allocator, size, alignment, attribute, file, line, purpose);
}
