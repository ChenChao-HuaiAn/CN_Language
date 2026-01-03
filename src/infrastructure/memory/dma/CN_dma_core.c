/******************************************************************************
 * 文件名: CN_dma_core.c
 * 功能: CN_Language DMA内存分配器核心实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建DMA内存分配器核心实现，支持DMA内存区域管理
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_dma_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// ============================================================================
// 内部辅助函数实现
// ============================================================================

/**
 * @brief 输出调试信息
 */
void CN_dma_debug_log(Stru_CN_DmaAllocator_t* allocator, const char* format, ...)
{
    if (allocator->debug_enabled && allocator->debug_callback != NULL)
    {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        allocator->debug_callback(buffer, allocator->debug_user_data);
    }
}

/**
 * @brief 验证分配器是否已初始化
 */
bool CN_dma_validate_allocator(Stru_CN_DmaAllocator_t* allocator)
{
    if (allocator == NULL || !allocator->initialized)
    {
        return false;
    }
    
    // 验证内存范围
    if (allocator->dma_region_start >= allocator->dma_region_end)
    {
        return false;
    }
    
    // 验证页面大小
    if (allocator->config.page_size == 0)
    {
        return false;
    }
    
    // 验证缓存行大小
    if (allocator->config.cache_line_size == 0)
    {
        return false;
    }
    
    return true;
}

/**
 * @brief 地址转换为页面索引
 */
size_t CN_dma_address_to_page_index(Stru_CN_DmaAllocator_t* allocator, uintptr_t address)
{
    if (address < allocator->dma_region_start || address >= allocator->dma_region_end)
    {
        return (size_t)-1;
    }
    
    return (address - allocator->dma_region_start) / allocator->config.page_size;
}

/**
 * @brief 页面索引转换为地址
 */
uintptr_t CN_dma_page_index_to_address(Stru_CN_DmaAllocator_t* allocator, size_t page_index)
{
    if (page_index >= allocator->total_pages)
    {
        return 0;
    }
    
    return allocator->dma_region_start + (page_index * allocator->config.page_size);
}

// ============================================================================
// 核心分配器函数实现
// ============================================================================

Stru_CN_DmaAllocator_t* CN_dma_create(const Stru_CN_DmaConfig_t* config)
{
    if (config == NULL)
    {
        return NULL;
    }
    
    // 验证配置
    if (config->dma_region_start >= config->dma_region_end)
    {
        return NULL;
    }
    
    if (config->page_size == 0)
    {
        return NULL;
    }
    
    if (config->cache_line_size == 0)
    {
        return NULL;
    }
    
    // 分配分配器结构
    Stru_CN_DmaAllocator_t* allocator = (Stru_CN_DmaAllocator_t*)malloc(sizeof(Stru_CN_DmaAllocator_t));
    if (allocator == NULL)
    {
        return NULL;
    }
    
    // 初始化分配器
    memset(allocator, 0, sizeof(Stru_CN_DmaAllocator_t));
    allocator->config = *config;
    allocator->dma_region_start = config->dma_region_start;
    allocator->dma_region_end = config->dma_region_end;
    allocator->cache_line_size = config->cache_line_size;
    
    // 计算总页面数量
    size_t memory_size = config->dma_region_end - config->dma_region_start;
    allocator->total_pages = memory_size / config->page_size;
    
    // 分配位图
    allocator->bitmap_size = (allocator->total_pages + 7) / 8; // 每个页面1位
    allocator->bitmap = (uint8_t*)calloc(allocator->bitmap_size, 1);
    if (allocator->bitmap == NULL)
    {
        free(allocator);
        return NULL;
    }
    
    // 初始化统计信息
    memset(&allocator->stats, 0, sizeof(allocator->stats));
    allocator->stats.total_pages = allocator->total_pages;
    allocator->stats.free_pages = allocator->total_pages;
    
    // 初始化分配跟踪（如果启用）
    if (config->track_allocations)
    {
        allocator->max_allocations = 1024; // 默认最大跟踪1024个分配
        allocator->allocations = (Stru_CN_DmaAllocationInfo_t*)malloc(
            allocator->max_allocations * sizeof(Stru_CN_DmaAllocationInfo_t));
        
        if (allocator->allocations == NULL)
        {
            // 分配失败，但分配器仍然可用
            allocator->config.track_allocations = false;
        }
        else
        {
            memset(allocator->allocations, 0, 
                   allocator->max_allocations * sizeof(Stru_CN_DmaAllocationInfo_t));
        }
    }
    
    // 初始化内部状态
    allocator->next_search_index = 0;
    allocator->debug_enabled = config->enable_debug;
    allocator->initialized = true;
    allocator->next_handle = 1; // 从1开始，0表示无效句柄
    allocator->cache_coherent = CN_dma_determine_cache_coherence(config->default_attribute);
    
    CN_dma_debug_log(allocator, "DMA内存分配器已创建：%s", config->name);
    CN_dma_debug_log(allocator, "DMA区域：0x%llx - 0x%llx", 
              (unsigned long long)config->dma_region_start,
              (unsigned long long)config->dma_region_end);
    CN_dma_debug_log(allocator, "页面大小：%zu 字节，缓存行大小：%zu 字节", 
              config->page_size, config->cache_line_size);
    CN_dma_debug_log(allocator, "总页面数：%zu，默认内存属性：%d", 
              allocator->total_pages, config->default_attribute);
    
    return allocator;
}

void CN_dma_destroy(Stru_CN_DmaAllocator_t* allocator)
{
    if (allocator == NULL)
    {
        return;
    }
    
    CN_dma_debug_log(allocator, "销毁DMA内存分配器：%s", allocator->config.name);
    
    // 检查内存泄漏
    if (allocator->stats.used_pages > 0)
    {
        CN_dma_debug_log(allocator, "警告：检测到内存泄漏，%zu 页面未释放",
                  allocator->stats.used_pages);
        
        if (allocator->config.track_allocations && allocator->allocations != NULL)
        {
            CN_dma_dump_leaks(allocator);
        }
    }
    
    // 释放分配跟踪数组
    if (allocator->allocations != NULL)
    {
        free(allocator->allocations);
    }
    
    // 释放位图
    if (allocator->bitmap != NULL)
    {
        free(allocator->bitmap);
    }
    
    // 释放分配器结构
    free(allocator);
}

Stru_CN_DmaBuffer_t* CN_dma_alloc_buffer(Stru_CN_DmaAllocator_t* allocator,
                                         size_t size,
                                         size_t alignment,
                                         Eum_CN_DmaMemoryAttribute_t attribute,
                                         const char* file, int line,
                                         const char* purpose)
{
    if (!CN_dma_validate_allocator(allocator))
    {
        return NULL;
    }
    
    // 验证对齐要求
    if (alignment == 0 || (alignment % allocator->config.cache_line_size) != 0)
    {
        CN_dma_debug_log(allocator, "对齐要求无效：%zu，必须是缓存行大小(%zu)的倍数",
                  alignment, allocator->config.cache_line_size);
        return NULL;
    }
    
    // 计算需要的页面数量
    size_t page_count = CN_dma_size_to_pages(size, allocator->config.page_size);
    if (page_count == 0 || page_count > allocator->stats.free_pages)
    {
        allocator->stats.allocation_failures++;
        CN_dma_debug_log(allocator, "分配失败：请求 %zu 字节（%zu 页面），但只有 %zu 空闲页面",
                  size, page_count, allocator->stats.free_pages);
        return NULL;
    }
    
    // 计算对齐页面数量
    size_t alignment_pages = CN_dma_size_to_pages(alignment, allocator->config.page_size);
    
    // 查找对齐的连续空闲页面
    size_t start_page = CN_dma_find_free_pages_aligned(allocator, page_count, alignment_pages);
    if (start_page == (size_t)-1)
    {
        allocator->stats.allocation_failures++;
        CN_dma_debug_log(allocator, "分配失败：找不到 %zu 个对齐的连续空闲页面", page_count);
        return NULL;
    }
    
    // 标记页面为已分配
    for (size_t i = 0; i < page_count; i++)
    {
        CN_dma_bitmap_set(allocator, start_page + i, true);
    }
    
    // 更新统计信息
    allocator->stats.free_pages -= page_count;
    allocator->stats.used_pages += page_count;
    allocator->stats.allocation_count++;
    
    // 计算物理地址和虚拟地址
    uintptr_t physical_address = CN_dma_page_index_to_address(allocator, start_page);
    void* virtual_address = (void*)physical_address; // 简化：假设物理地址直接映射
    
    // 分配缓冲区结构
    Stru_CN_DmaBuffer_t* buffer = (Stru_CN_DmaBuffer_t*)malloc(sizeof(Stru_CN_DmaBuffer_t));
    if (buffer == NULL)
    {
        // 分配失败，回滚页面分配
        for (size_t i = 0; i < page_count; i++)
        {
            CN_dma_bitmap_set(allocator, start_page + i, false);
        }
        allocator->stats.free_pages += page_count;
        allocator->stats.used_pages -= page_count;
        allocator->stats.allocation_count--;
        allocator->stats.allocation_failures++;
        
        CN_dma_debug_log(allocator, "分配失败：无法分配缓冲区结构");
        return NULL;
    }
    
    // 初始化缓冲区
    memset(buffer, 0, sizeof(Stru_CN_DmaBuffer_t));
    buffer->physical_address = physical_address;
    buffer->virtual_address = virtual_address;
    buffer->size = page_count * allocator->config.page_size;
    buffer->alignment = alignment;
    buffer->attribute = attribute;
    buffer->direction = Eum_DMA_DIRECTION_NONE;
    buffer->is_coherent = CN_dma_determine_cache_coherence(attribute);
    buffer->is_contiguous = true;
    buffer->handle = allocator->next_handle++;
    
    // 如果配置要求，清零内存
    if (allocator->config.zero_on_alloc && virtual_address != NULL)
    {
        memset(virtual_address, 0, buffer->size);
    }
    
    // 添加分配跟踪
    CN_dma_add_allocation_tracking(allocator, buffer, file, line, purpose);
    
    CN_dma_debug_log(allocator, "分配成功：%zu 字节（%zu 页面），物理地址：0x%llx，句柄：%u",
              buffer->size, page_count, (unsigned long long)physical_address, buffer->handle);
    
    return buffer;
}

void CN_dma_free_buffer(Stru_CN_DmaAllocator_t* allocator,
                        Stru_CN_DmaBuffer_t* buffer,
                        const char* file, int line)
{
    if (!CN_dma_validate_allocator(allocator) || buffer == NULL)
    {
        return;
    }
    
    // 查找页面索引
    size_t page_index = CN_dma_address_to_page_index(allocator, buffer->physical_address);
    if (page_index == (size_t)-1)
    {
        CN_dma_debug_log(allocator, "释放失败：无效的物理地址：0x%llx",
                  (unsigned long long)buffer->physical_address);
        return;
    }
    
    // 计算页面数量
    size_t page_count = CN_dma_size_to_pages(buffer->size, allocator->config.page_size);
    
    // 验证这些页面确实已分配
    for (size_t i = 0; i < page_count; i++)
    {
        if (!CN_dma_bitmap_get(allocator, page_index + i))
        {
            CN_dma_debug_log(allocator, "释放失败：页面 %zu 未分配", page_index + i);
            return;
        }
    }
    
    // 标记页面为空闲
    for (size_t i = 0; i < page_count; i++)
    {
        CN_dma_bitmap_set(allocator, page_index + i, false);
    }
    
    // 更新统计信息
    allocator->stats.free_pages += page_count;
    allocator->stats.used_pages -= page_count;
    allocator->stats.free_count++;
    
    // 移除分配跟踪
    CN_dma_remove_allocation_tracking(allocator, buffer->handle);
    
    CN_dma_debug_log(allocator, "释放成功：%zu 字节（%zu 页面），物理地址：0x%llx，句柄：%u",
              buffer->size, page_count, (unsigned long long)buffer->physical_address, buffer->handle);
    
    // 释放缓冲区结构
    free(buffer);
}
