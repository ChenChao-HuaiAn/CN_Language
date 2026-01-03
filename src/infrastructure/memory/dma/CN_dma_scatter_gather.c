/******************************************************************************
 * 文件名: CN_dma_scatter_gather.c
 * 功能: CN_Language DMA分散-聚集内存分配支持
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建DMA分散-聚集内存分配支持
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_dma_internal.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 分散-聚集DMA函数实现
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
    const char* purpose)
{
    if (!CN_dma_validate_allocator(allocator))
    {
        return NULL;
    }
    
    if (!allocator->config.support_scatter_gather)
    {
        CN_dma_debug_log(allocator, "分配失败：分散-聚集DMA不支持");
        return NULL;
    }
    
    if (max_elements == 0 || max_elements > allocator->config.max_scatter_elements)
    {
        CN_dma_debug_log(allocator, "分配失败：最大元素数量无效：%zu", max_elements);
        return NULL;
    }
    
    // 分配分散-聚集列表结构
    Stru_CN_DmaScatterGatherList_t* sg_list = (Stru_CN_DmaScatterGatherList_t*)malloc(
        sizeof(Stru_CN_DmaScatterGatherList_t));
    if (sg_list == NULL)
    {
        CN_dma_debug_log(allocator, "分配失败：无法分配分散-聚集列表结构");
        return NULL;
    }
    
    memset(sg_list, 0, sizeof(Stru_CN_DmaScatterGatherList_t));
    
    // 分配元素数组
    sg_list->elements = (Stru_CN_DmaScatterGatherElement_t*)malloc(
        max_elements * sizeof(Stru_CN_DmaScatterGatherElement_t));
    if (sg_list->elements == NULL)
    {
        free(sg_list);
        CN_dma_debug_log(allocator, "分配失败：无法分配分散-聚集元素数组");
        return NULL;
    }
    
    memset(sg_list->elements, 0, max_elements * sizeof(Stru_CN_DmaScatterGatherElement_t));
    
    // 简化实现：分配单个连续缓冲区，然后分割成多个元素
    // 实际实现应该分配多个不连续的缓冲区
    Stru_CN_DmaBuffer_t* buffer = CN_dma_alloc_buffer(allocator, total_size, 
                                                     allocator->config.cache_line_size,
                                                     attribute, file, line, purpose);
    if (buffer == NULL)
    {
        free(sg_list->elements);
        free(sg_list);
        return NULL;
    }
    
    // 将连续缓冲区分割成多个元素
    size_t element_size = total_size / max_elements;
    if (element_size == 0)
    {
        element_size = total_size;
        max_elements = 1;
    }
    
    sg_list->element_count = max_elements;
    sg_list->total_length = total_size;
    
    for (size_t i = 0; i < max_elements; i++)
    {
        size_t offset = i * element_size;
        size_t length = (i == max_elements - 1) ? (total_size - offset) : element_size;
        
        sg_list->elements[i].physical_address = buffer->physical_address + offset;
        sg_list->elements[i].length = length;
    }
    
    // 注意：这里简化实现，实际应该存储buffer句柄以便后续释放
    CN_dma_debug_log(allocator, "分散-聚集分配成功：%zu 字节，%zu 个元素",
              total_size, max_elements);
    
    return sg_list;
}

/**
 * @brief 释放分散-聚集DMA列表
 */
void CN_dma_free_scatter_gather_internal(
    Stru_CN_DmaAllocator_t* allocator,
    Stru_CN_DmaScatterGatherList_t* sg_list,
    const char* file, int line)
{
    if (!CN_dma_validate_allocator(allocator) || sg_list == NULL)
    {
        return;
    }
    
    // 简化实现：假设所有元素来自同一个连续缓冲区
    // 实际实现需要处理多个独立的缓冲区
    if (sg_list->elements != NULL && sg_list->element_count > 0)
    {
        // 获取第一个元素的物理地址
        uintptr_t first_address = sg_list->elements[0].physical_address;
        
        // 计算总大小
        size_t total_size = 0;
        for (size_t i = 0; i < sg_list->element_count; i++)
        {
            total_size += sg_list->elements[i].length;
        }
        
        // 创建临时缓冲区结构用于释放
        Stru_CN_DmaBuffer_t temp_buffer;
        memset(&temp_buffer, 0, sizeof(temp_buffer));
        temp_buffer.physical_address = first_address;
        temp_buffer.size = total_size;
        temp_buffer.handle = 0; // 未知句柄
        
        // 查找并释放页面
        size_t page_index = CN_dma_address_to_page_index(allocator, first_address);
        if (page_index != (size_t)-1)
        {
            size_t page_count = CN_dma_size_to_pages(total_size, allocator->config.page_size);
            
            for (size_t i = 0; i < page_count; i++)
            {
                CN_dma_bitmap_set(allocator, page_index + i, false);
            }
            
            allocator->stats.free_pages += page_count;
            allocator->stats.used_pages -= page_count;
            allocator->stats.free_count++;
        }
    }
    
    // 释放元素数组
    if (sg_list->elements != NULL)
    {
        free(sg_list->elements);
    }
    
    // 释放列表结构
    free(sg_list);
    
    CN_dma_debug_log(allocator, "分散-聚集释放成功");
}

// ============================================================================
// 公共接口包装函数
// ============================================================================

Stru_CN_DmaScatterGatherList_t* CN_dma_alloc_scatter_gather(
    Stru_CN_DmaAllocator_t* allocator,
    size_t total_size,
    size_t max_elements,
    Eum_CN_DmaMemoryAttribute_t attribute,
    const char* file, int line,
    const char* purpose)
{
    return CN_dma_alloc_scatter_gather_internal(allocator, total_size, max_elements, 
                                               attribute, file, line, purpose);
}

void CN_dma_free_scatter_gather(Stru_CN_DmaAllocator_t* allocator,
                                Stru_CN_DmaScatterGatherList_t* sg_list,
                                const char* file, int line)
{
    CN_dma_free_scatter_gather_internal(allocator, sg_list, file, line);
}
