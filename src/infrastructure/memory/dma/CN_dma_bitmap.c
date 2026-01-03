/******************************************************************************
 * 文件名: CN_dma_bitmap.c
 * 功能: CN_Language DMA内存分配器位图管理
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建DMA内存分配器位图管理，支持页面分配跟踪
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_dma_internal.h"
#include <string.h>

// ============================================================================
// 位图管理函数实现
// ============================================================================

/**
 * @brief 设置位图中的位
 */
void CN_dma_bitmap_set(Stru_CN_DmaAllocator_t* allocator, size_t page_index, bool allocated)
{
    if (page_index >= allocator->total_pages)
    {
        return;
    }
    
    size_t byte_index = page_index / 8;
    size_t bit_index = page_index % 8;
    
    if (allocated)
    {
        allocator->bitmap[byte_index] |= (1 << bit_index);
    }
    else
    {
        allocator->bitmap[byte_index] &= ~(1 << bit_index);
    }
}

/**
 * @brief 获取位图中的位
 */
bool CN_dma_bitmap_get(Stru_CN_DmaAllocator_t* allocator, size_t page_index)
{
    if (page_index >= allocator->total_pages)
    {
        return true; // 越界视为已分配
    }
    
    size_t byte_index = page_index / 8;
    size_t bit_index = page_index % 8;
    
    return (allocator->bitmap[byte_index] & (1 << bit_index)) != 0;
}

/**
 * @brief 查找连续空闲页面
 */
size_t CN_dma_find_free_pages(Stru_CN_DmaAllocator_t* allocator, size_t page_count)
{
    if (page_count == 0 || page_count > allocator->total_pages)
    {
        return (size_t)-1;
    }
    
    size_t start_index = allocator->next_search_index;
    size_t consecutive_free = 0;
    
    // 从next_search_index开始搜索，循环回到开头
    for (size_t i = 0; i < allocator->total_pages * 2; i++)
    {
        size_t current_index = (start_index + i) % allocator->total_pages;
        
        if (!CN_dma_bitmap_get(allocator, current_index))
        {
            consecutive_free++;
            if (consecutive_free == page_count)
            {
                size_t found_index = current_index - page_count + 1;
                allocator->next_search_index = (found_index + page_count) % allocator->total_pages;
                return found_index;
            }
        }
        else
        {
            consecutive_free = 0;
        }
    }
    
    return (size_t)-1;
}

/**
 * @brief 查找对齐的连续空闲页面
 */
size_t CN_dma_find_free_pages_aligned(Stru_CN_DmaAllocator_t* allocator, 
                                      size_t page_count, size_t alignment_pages)
{
    if (page_count == 0 || page_count > allocator->total_pages)
    {
        return (size_t)-1;
    }
    
    size_t start_index = allocator->next_search_index;
    
    for (size_t i = 0; i < allocator->total_pages; i++)
    {
        size_t current_index = (start_index + i) % allocator->total_pages;
        
        // 检查对齐
        if (current_index % alignment_pages != 0)
        {
            continue;
        }
        
        // 检查是否有足够的连续空闲页面
        bool all_free = true;
        for (size_t j = 0; j < page_count; j++)
        {
            if (CN_dma_bitmap_get(allocator, current_index + j))
            {
                all_free = false;
                break;
            }
        }
        
        if (all_free && (current_index + page_count) <= allocator->total_pages)
        {
            allocator->next_search_index = (current_index + page_count) % allocator->total_pages;
            return current_index;
        }
    }
    
    return (size_t)-1;
}
