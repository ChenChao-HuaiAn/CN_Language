/******************************************************************************
 * 文件名: CN_physical_allocator.c
 * 功能: CN_Language物理内存分配器实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建物理内存分配器，支持页框分配
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_physical_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief 物理内存分配器内部结构
 * 
 * 使用位图跟踪页面分配状态，支持快速分配和释放。
 */
struct Stru_CN_PhysicalAllocator_t
{
    Stru_CN_PhysicalConfig_t config;      /**< 分配器配置 */
    Stru_CN_PhysicalStats_t stats;        /**< 统计信息 */
    
    // 内存范围信息
    uintptr_t memory_start;               /**< 物理内存起始地址 */
    uintptr_t memory_end;                 /**< 物理内存结束地址 */
    size_t total_pages;                   /**< 总页面数量 */
    
    // 位图管理
    uint8_t* bitmap;                      /**< 页面分配位图（1=已分配，0=空闲） */
    size_t bitmap_size;                   /**< 位图大小（字节） */
    
    // 调试和跟踪
    bool debug_enabled;                   /**< 调试模式是否启用 */
    CN_PhysicalDebugCallback_t debug_callback; /**< 调试回调函数 */
    void* debug_user_data;                /**< 调试回调用户数据 */
    
    // 分配跟踪（如果启用）
    Stru_CN_PhysicalAllocationInfo_t* allocations; /**< 分配信息数组 */
    size_t max_allocations;               /**< 最大跟踪分配数 */
    size_t current_allocations;           /**< 当前跟踪分配数 */
    
    // 内部状态
    size_t next_search_index;             /**< 下一次搜索的起始索引（优化） */
    bool initialized;                     /**< 分配器是否已初始化 */
};

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 输出调试信息
 */
static void debug_log(Stru_CN_PhysicalAllocator_t* allocator, const char* format, ...)
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
static bool validate_allocator(Stru_CN_PhysicalAllocator_t* allocator)
{
    if (allocator == NULL || !allocator->initialized)
    {
        return false;
    }
    
    // 验证内存范围
    if (allocator->memory_start >= allocator->memory_end)
    {
        return false;
    }
    
    // 验证页面大小
    if (allocator->config.page_size == 0)
    {
        return false;
    }
    
    return true;
}

/**
 * @brief 地址转换为页面索引
 */
static size_t address_to_page_index(Stru_CN_PhysicalAllocator_t* allocator, uintptr_t address)
{
    if (address < allocator->memory_start || address >= allocator->memory_end)
    {
        return (size_t)-1;
    }
    
    return (address - allocator->memory_start) / allocator->config.page_size;
}

/**
 * @brief 页面索引转换为地址
 */
static uintptr_t page_index_to_address(Stru_CN_PhysicalAllocator_t* allocator, size_t page_index)
{
    if (page_index >= allocator->total_pages)
    {
        return 0;
    }
    
    return allocator->memory_start + (page_index * allocator->config.page_size);
}

/**
 * @brief 设置位图中的位
 */
static void bitmap_set(Stru_CN_PhysicalAllocator_t* allocator, size_t page_index, bool allocated)
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
static bool bitmap_get(Stru_CN_PhysicalAllocator_t* allocator, size_t page_index)
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
static size_t find_free_pages(Stru_CN_PhysicalAllocator_t* allocator, size_t page_count)
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
        
        if (!bitmap_get(allocator, current_index))
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
static size_t find_free_pages_aligned(Stru_CN_PhysicalAllocator_t* allocator, 
                                      size_t page_count, size_t alignment_pages)
{
    if (page_count == 0 || page_count > allocator->total_pages)
    {
        return (size_t)-1;
    }
    
    // 简化实现：先找到连续页面，然后检查对齐
    // 实际实现应该更高效
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
            if (bitmap_get(allocator, current_index + j))
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

/**
 * @brief 添加分配跟踪记录
 */
static void add_allocation_tracking(Stru_CN_PhysicalAllocator_t* allocator,
                                   uintptr_t address, size_t page_count,
                                   const char* file, int line, const char* purpose)
{
    if (!allocator->config.track_allocations || allocator->allocations == NULL)
    {
        return;
    }
    
    if (allocator->current_allocations >= allocator->max_allocations)
    {
        // 分配跟踪数组已满，忽略新记录
        debug_log(allocator, "警告：分配跟踪数组已满，无法跟踪新分配");
        return;
    }
    
    Stru_CN_PhysicalAllocationInfo_t* info = &allocator->allocations[allocator->current_allocations];
    info->physical_address = address;
    info->page_count = page_count;
    info->file = file;
    info->line = line;
    info->timestamp = 0; // 实际实现应该使用时间戳
    info->purpose = purpose;
    
    allocator->current_allocations++;
}

/**
 * @brief 移除分配跟踪记录
 */
static void remove_allocation_tracking(Stru_CN_PhysicalAllocator_t* allocator,
                                      uintptr_t address)
{
    if (!allocator->config.track_allocations || allocator->allocations == NULL)
    {
        return;
    }
    
    for (size_t i = 0; i < allocator->current_allocations; i++)
    {
        if (allocator->allocations[i].physical_address == address)
        {
            // 将最后一个元素移动到当前位置
            if (i < allocator->current_allocations - 1)
            {
                allocator->allocations[i] = allocator->allocations[allocator->current_allocations - 1];
            }
            allocator->current_allocations--;
            break;
        }
    }
}

// ============================================================================
// 公共接口实现
// ============================================================================

Stru_CN_PhysicalAllocator_t* CN_physical_create(const Stru_CN_PhysicalConfig_t* config)
{
    if (config == NULL)
    {
        return NULL;
    }
    
    // 验证配置
    if (config->memory_start >= config->memory_end)
    {
        return NULL;
    }
    
    if (config->page_size == 0)
    {
        return NULL;
    }
    
    // 分配分配器结构
    Stru_CN_PhysicalAllocator_t* allocator = (Stru_CN_PhysicalAllocator_t*)malloc(sizeof(Stru_CN_PhysicalAllocator_t));
    if (allocator == NULL)
    {
        return NULL;
    }
    
    // 初始化分配器
    memset(allocator, 0, sizeof(Stru_CN_PhysicalAllocator_t));
    allocator->config = *config;
    allocator->memory_start = config->memory_start;
    allocator->memory_end = config->memory_end;
    
    // 计算总页面数量
    size_t memory_size = config->memory_end - config->memory_start;
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
    
    // 设置保留页面
    if (config->reserved_pages > 0)
    {
        size_t pages_to_reserve = config->reserved_pages;
        if (pages_to_reserve > allocator->total_pages)
        {
            pages_to_reserve = allocator->total_pages;
        }
        
        for (size_t i = 0; i < pages_to_reserve; i++)
        {
            bitmap_set(allocator, i, true);
        }
        
        allocator->stats.reserved_pages = pages_to_reserve;
        allocator->stats.free_pages -= pages_to_reserve;
        allocator->stats.used_pages = pages_to_reserve;
    }
    
    // 初始化分配跟踪（如果启用）
    if (config->track_allocations)
    {
        allocator->max_allocations = 1024; // 默认最大跟踪1024个分配
        allocator->allocations = (Stru_CN_PhysicalAllocationInfo_t*)malloc(
            allocator->max_allocations * sizeof(Stru_CN_PhysicalAllocationInfo_t));
        
        if (allocator->allocations == NULL)
        {
            // 分配失败，但分配器仍然可用
            allocator->config.track_allocations = false;
        }
        else
        {
            memset(allocator->allocations, 0, 
                   allocator->max_allocations * sizeof(Stru_CN_PhysicalAllocationInfo_t));
        }
    }
    
    // 初始化内部状态
    allocator->next_search_index = config->reserved_pages; // 从保留页面之后开始搜索
    allocator->debug_enabled = config->enable_debug;
    allocator->initialized = true;
    
    debug_log(allocator, "物理内存分配器已创建：%s", config->name);
    debug_log(allocator, "内存范围：0x%llx - 0x%llx", 
              (unsigned long long)config->memory_start,
              (unsigned long long)config->memory_end);
    debug_log(allocator, "页面大小：%zu 字节，总页面数：%zu", 
              config->page_size, allocator->total_pages);
    
    return allocator;
}

void CN_physical_destroy(Stru_CN_PhysicalAllocator_t* allocator)
{
    if (allocator == NULL)
    {
        return;
    }
    
    debug_log(allocator, "销毁物理内存分配器：%s", allocator->config.name);
    
    // 检查内存泄漏
    if (allocator->stats.used_pages > allocator->stats.reserved_pages)
    {
        debug_log(allocator, "警告：检测到内存泄漏，%zu 页面未释放",
                  allocator->stats.used_pages - allocator->stats.reserved_pages);
        
        if (allocator->config.track_allocations && allocator->allocations != NULL)
        {
            CN_physical_dump_leaks(allocator);
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

uintptr_t CN_physical_alloc_pages(Stru_CN_PhysicalAllocator_t* allocator, 
                                  size_t page_count, 
                                  const char* file, int line, 
                                  const char* purpose)
{
    if (!validate_allocator(allocator))
    {
        return 0;
    }
    
    if (page_count == 0 || page_count > allocator->stats.free_pages)
    {
        allocator->stats.allocation_failures++;
        debug_log(allocator, "分配失败：请求 %zu 页面，但只有 %zu 空闲页面",
                  page_count, allocator->stats.free_pages);
        return 0;
    }
    
    // 查找连续空闲页面
    size_t start_page = find_free_pages(allocator, page_count);
    if (start_page == (size_t)-1)
    {
        allocator->stats.allocation_failures++;
        debug_log(allocator, "分配失败：找不到 %zu 个连续空闲页面", page_count);
        return 0;
    }
    
    // 标记页面为已分配
    for (size_t i = 0; i < page_count; i++)
    {
        bitmap_set(allocator, start_page + i, true);
    }
    
    // 更新统计信息
    allocator->stats.free_pages -= page_count;
    allocator->stats.used_pages += page_count;
    allocator->stats.allocation_count++;
    
    // 计算物理地址
    uintptr_t address = page_index_to_address(allocator, start_page);
    
    // 添加分配跟踪
    add_allocation_tracking(allocator, address, page_count, file, line, purpose);
    
    // 清零页面（如果配置要求）
    if (allocator->config.zero_on_alloc && address != 0)
    {
        // 注意：在实际操作系统中，需要将物理地址映射到虚拟地址才能访问
        // 这里只是记录需求，实际实现需要操作系统支持
        debug_log(allocator, "页面清零已请求，但需要虚拟地址映射");
    }
    
    debug_log(allocator, "分配 %zu 页面，起始地址：0x%llx，目的：%s",
              page_count, (unsigned long long)address, 
              purpose ? purpose : "未指定");
    
    return address;
}

uintptr_t CN_physical_alloc_pages_aligned(Stru_CN_PhysicalAllocator_t* allocator, 
                                          size_t page_count, 
                                          size_t alignment,
                                          const char* file, int line, 
                                          const char* purpose)
{
    if (!validate_allocator(allocator))
    {
        return 0;
    }
    
    // 验证对齐要求
    if (alignment == 0 || (alignment % allocator->config.page_size) != 0)
    {
        debug_log(allocator, "对齐要求无效：%zu，必须是页面大小(%zu)的倍数",
                  alignment, allocator->config.page_size);
        return 0;
    }
    
    if (page_count == 0 || page_count > allocator->stats.free_pages)
    {
        allocator->stats.allocation_failures++;
        debug_log(allocator, "分配失败：请求 %zu 页面，但只有 %zu 空闲页面",
                  page_count, allocator->stats.free_pages);
        return 0;
    }
    
    // 计算对齐页面数量
    size_t alignment_pages = alignment / allocator->config.page_size;
    
    // 查找对齐的连续空闲页面
    size_t start_page = find_free_pages_aligned(allocator, page_count, alignment_pages);
    if (start_page == (size_t)-1)
    {
        allocator->stats.allocation_failures++;
        debug_log(allocator, "分配失败：找不到 %zu 个对齐的连续空闲页面", page_count);
        return 0;
    }
    
    // 标记页面为已分配
    for (size_t i = 0; i < page_count; i++)
    {
        bitmap_set(allocator, start_page + i, true);
    }
    
    // 更新统计信息
    allocator->stats.free_pages -= page_count;
    allocator->stats.used_pages += page_count;
    allocator->stats.allocation_count++;
    
    // 计算物理地址
    uintptr_t address = page_index_to_address(allocator, start_page);
    
    // 添加分配跟踪
    add_allocation_tracking(allocator, address, page_count, file, line, purpose);
    
    debug_log(allocator, "分配 %zu 对齐页面（对齐：%zu），起始地址：0x%llx，目的：%s",
              page_count, alignment, (unsigned long long)address, 
              purpose ? purpose : "未指定");
    
    return address;
}

uintptr_t CN_physical_alloc_pages_at(Stru_CN_PhysicalAllocator_t* allocator, 
                                     uintptr_t physical_address,
                                     size_t page_count, 
                                     const char* file, int line, 
                                     const char* purpose)
{
    if (!validate_allocator(allocator))
    {
        return 0;
    }
    
    // 验证地址是否页面对齐
    if (!CN_physical_is_page_aligned(physical_address, allocator->config.page_size))
    {
        debug_log(allocator, "地址 0x%llx 不是页面对齐的（页面大小：%zu）",
                  (unsigned long long)physical_address, allocator->config.page_size);
        return 0;
    }
    
    // 转换为页面索引
    size_t start_page = address_to_page_index(allocator, physical_address);
    if (start_page == (size_t)-1)
    {
        debug_log(allocator, "地址 0x%llx 不在分配器内存范围内",
                  (unsigned long long)physical_address);
        return 0;
    }
    
    // 检查是否有足够的连续页面
    if (start_page + page_count > allocator->total_pages)
    {
        debug_log(allocator, "请求的页面范围超出内存边界");
        return 0;
    }
    
    // 检查所有页面是否都空闲
    for (size_t i = 0; i < page_count; i++)
    {
        if (bitmap_get(allocator, start_page + i))
        {
            debug_log(allocator, "页面 %zu 已分配，无法分配特定地址",
                      start_page + i);
            return 0;
        }
    }
    
    // 标记页面为已分配
    for (size_t i = 0; i < page_count; i++)
    {
        bitmap_set(allocator, start_page + i, true);
    }
    
    // 更新统计信息
    allocator->stats.free_pages -= page_count;
    allocator->stats.used_pages += page_count;
    allocator->stats.allocation_count++;
    
    // 添加分配跟踪
    add_allocation_tracking(allocator, physical_address, page_count, file, line, purpose);
    
    debug_log(allocator, "在特定地址分配 %zu 页面，地址：0x%llx，目的：%s",
              page_count, (unsigned long long)physical_address, 
              purpose ? purpose : "未指定");
    
    return physical_address;
}

void CN_physical_free_pages(Stru_CN_PhysicalAllocator_t* allocator, 
                            uintptr_t physical_address,
                            size_t page_count, 
                            const char* file, int line)
{
    // 标记未使用的参数以避免编译器警告
    (void)file;
    (void)line;
    
    if (!validate_allocator(allocator))
    {
        return;
    }
    
    if (physical_address == 0 || page_count == 0)
    {
        return;
    }
    
    // 验证地址是否页面对齐
    if (!CN_physical_is_page_aligned(physical_address, allocator->config.page_size))
    {
        debug_log(allocator, "释放失败：地址 0x%llx 不是页面对齐的",
                  (unsigned long long)physical_address);
        return;
    }
    
    // 转换为页面索引
    size_t start_page = address_to_page_index(allocator, physical_address);
    if (start_page == (size_t)-1)
    {
        debug_log(allocator, "释放失败：地址 0x%llx 不在分配器内存范围内",
                  (unsigned long long)physical_address);
        return;
    }
    
    // 检查页面范围
    if (start_page + page_count > allocator->total_pages)
    {
        debug_log(allocator, "释放失败：页面范围超出内存边界");
        return;
    }
    
    // 标记页面为未分配
    for (size_t i = 0; i < page_count; i++)
    {
        bitmap_set(allocator, start_page + i, false);
    }
    
    // 更新统计信息
    allocator->stats.free_pages += page_count;
    allocator->stats.used_pages -= page_count;
    allocator->stats.free_count++;
    
    // 移除分配跟踪
    remove_allocation_tracking(allocator, physical_address);
    
    debug_log(allocator, "释放 %zu 页面，起始地址：0x%llx",
              page_count, (unsigned long long)physical_address);
}

bool CN_physical_get_config(Stru_CN_PhysicalAllocator_t* allocator, 
                            Stru_CN_PhysicalConfig_t* config)
{
    if (!validate_allocator(allocator) || config == NULL)
    {
        return false;
    }
    
    *config = allocator->config;
    return true;
}

bool CN_physical_get_stats(Stru_CN_PhysicalAllocator_t* allocator, 
                           Stru_CN_PhysicalStats_t* stats)
{
    if (!validate_allocator(allocator) || stats == NULL)
    {
        return false;
    }
    
    *stats = allocator->stats;
    
    // 计算碎片化程度（简化实现）
    if (allocator->stats.total_pages > 0)
    {
        size_t free_pages = allocator->stats.free_pages;
        size_t largest_block = CN_physical_largest_free_block(allocator);
        
        if (free_pages > 0 && largest_block > 0)
        {
            // 碎片化 = 1 - (最大连续块 / 总空闲页面)
            double fragmentation = 1.0 - ((double)largest_block / (double)free_pages);
            allocator->stats.fragmentation = (size_t)(fragmentation * 100.0);
        }
        else
        {
            allocator->stats.fragmentation = 0;
        }
        
        stats->fragmentation = allocator->stats.fragmentation;
        stats->largest_free_block = largest_block;
    }
    
    return true;
}

void CN_physical_reset_stats(Stru_CN_PhysicalAllocator_t* allocator)
{
    if (!validate_allocator(allocator))
    {
        return;
    }
    
    // 重置计数统计，但保持页面统计
    allocator->stats.allocation_count = 0;
    allocator->stats.free_count = 0;
    allocator->stats.allocation_failures = 0;
    allocator->stats.fragmentation = 0;
    
    debug_log(allocator, "统计信息已重置");
}

bool CN_physical_is_allocated(Stru_CN_PhysicalAllocator_t* allocator, 
                              uintptr_t physical_address)
{
    if (!validate_allocator(allocator))
    {
        return false;
    }
    
    size_t page_index = address_to_page_index(allocator, physical_address);
    if (page_index == (size_t)-1)
    {
        return false;
    }
    
    return bitmap_get(allocator, page_index);
}

size_t CN_physical_total_pages(Stru_CN_PhysicalAllocator_t* allocator)
{
    if (!validate_allocator(allocator))
    {
        return 0;
    }
    
    return allocator->stats.total_pages;
}

size_t CN_physical_get_free_pages(Stru_CN_PhysicalAllocator_t* allocator)
{
    if (!validate_allocator(allocator))
    {
        return 0;
    }
    
    return allocator->stats.free_pages;
}

size_t CN_physical_used_pages(Stru_CN_PhysicalAllocator_t* allocator)
{
    if (!validate_allocator(allocator))
    {
        return 0;
    }
    
    return allocator->stats.used_pages;
}

size_t CN_physical_largest_free_block(Stru_CN_PhysicalAllocator_t* allocator)
{
    if (!validate_allocator(allocator))
    {
        return 0;
    }
    
    size_t largest_block = 0;
    size_t current_block = 0;
    
    for (size_t i = 0; i < allocator->total_pages; i++)
    {
        if (!bitmap_get(allocator, i))
        {
            current_block++;
            if (current_block > largest_block)
            {
                largest_block = current_block;
            }
        }
        else
        {
            current_block = 0;
        }
    }
    
    allocator->stats.largest_free_block = largest_block;
    return largest_block;
}

void CN_physical_enable_debug(Stru_CN_PhysicalAllocator_t* allocator, bool enable)
{
    if (!validate_allocator(allocator))
    {
        return;
    }
    
    allocator->debug_enabled = enable;
    allocator->config.enable_debug = enable;
    
    debug_log(allocator, "调试模式 %s", enable ? "启用" : "禁用");
}

bool CN_physical_validate(Stru_CN_PhysicalAllocator_t* allocator)
{
    if (!validate_allocator(allocator))
    {
        return false;
    }
    
    // 验证位图一致性
    size_t bitmap_count = 0;
    for (size_t i = 0; i < allocator->total_pages; i++)
    {
        if (bitmap_get(allocator, i))
        {
            bitmap_count++;
        }
    }
    
    // 验证统计信息一致性
    if (bitmap_count != allocator->stats.used_pages)
    {
        debug_log(allocator, "验证失败：位图计数(%zu) != 使用页面统计(%zu)",
                  bitmap_count, allocator->stats.used_pages);
        return false;
    }
    
    if (allocator->stats.total_pages != allocator->total_pages)
    {
        debug_log(allocator, "验证失败：总页面统计(%zu) != 实际总页面(%zu)",
                  allocator->stats.total_pages, allocator->total_pages);
        return false;
    }
    
    if (allocator->stats.free_pages + allocator->stats.used_pages != allocator->stats.total_pages)
    {
        debug_log(allocator, "验证失败：空闲(%zu) + 使用(%zu) != 总(%zu)",
                  allocator->stats.free_pages, allocator->stats.used_pages, 
                  allocator->stats.total_pages);
        return false;
    }
    
    debug_log(allocator, "分配器验证通过");
    return true;
}

void CN_physical_dump(Stru_CN_PhysicalAllocator_t* allocator)
{
    if (!validate_allocator(allocator))
    {
        return;
    }
    
    printf("=== 物理内存分配器状态：%s ===\n", allocator->config.name);
    printf("内存范围: 0x%llx - 0x%llx\n", 
           (unsigned long long)allocator->memory_start,
           (unsigned long long)allocator->memory_end);
    printf("页面大小: %zu 字节\n", allocator->config.page_size);
    printf("总页面数: %zu\n", allocator->stats.total_pages);
    printf("空闲页面: %zu\n", allocator->stats.free_pages);
    printf("使用页面: %zu\n", allocator->stats.used_pages);
    printf("保留页面: %zu\n", allocator->stats.reserved_pages);
    printf("分配次数: %zu\n", allocator->stats.allocation_count);
    printf("释放次数: %zu\n", allocator->stats.free_count);
    printf("分配失败: %zu\n", allocator->stats.allocation_failures);
    printf("碎片化程度: %zu%%\n", allocator->stats.fragmentation);
    printf("最大连续空闲块: %zu 页面\n", allocator->stats.largest_free_block);
    printf("调试模式: %s\n", allocator->debug_enabled ? "启用" : "禁用");
    printf("分配跟踪: %s (%zu/%zu)\n", 
           allocator->config.track_allocations ? "启用" : "禁用",
           allocator->current_allocations, allocator->max_allocations);
    printf("==========================================\n");
}

void CN_physical_dump_leaks(Stru_CN_PhysicalAllocator_t* allocator)
{
    if (!validate_allocator(allocator) || !allocator->config.track_allocations)
    {
        return;
    }
    
    if (allocator->current_allocations == 0)
    {
        printf("未检测到内存泄漏\n");
        return;
    }
    
    printf("=== 检测到内存泄漏：%zu 个未释放分配 ===\n", allocator->current_allocations);
    for (size_t i = 0; i < allocator->current_allocations; i++)
    {
        Stru_CN_PhysicalAllocationInfo_t* info = &allocator->allocations[i];
        printf("泄漏 #%zu:\n", i + 1);
        printf("  地址: 0x%llx\n", (unsigned long long)info->physical_address);
        printf("  页面数: %zu\n", info->page_count);
        printf("  大小: %zu 字节\n", info->page_count * allocator->config.page_size);
        if (info->file)
        {
            printf("  位置: %s:%d\n", info->file, info->line);
        }
        if (info->purpose)
        {
            printf("  目的: %s\n", info->purpose);
        }
        printf("\n");
    }
    printf("==========================================\n");
}

void CN_physical_set_debug_callback(Stru_CN_PhysicalAllocator_t* allocator, 
                                    CN_PhysicalDebugCallback_t callback, 
                                    void* user_data)
{
    if (!validate_allocator(allocator))
    {
        return;
    }
    
    allocator->debug_callback = callback;
    allocator->debug_user_data = user_data;
    
    debug_log(allocator, "调试回调已设置");
}
