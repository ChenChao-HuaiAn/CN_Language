/******************************************************************************
 * 文件名: CN_arena_core.c
 * 功能: CN_Language区域分配器核心实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建完整的区域分配器核心实现
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_arena_allocator.h"
#include "CN_arena_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 验证对齐值是否有效（必须是2的幂）
 * 
 * @param alignment 对齐值
 * @return 如果有效返回true，否则返回false
 */
static bool is_valid_alignment(size_t alignment)
{
    return alignment > 0 && (alignment & (alignment - 1)) == 0;
}

/**
 * @brief 计算对齐后的分配大小
 * 
 * @param size 原始大小
 * @param alignment 对齐要求
 * @return 对齐后的大小
 */
static size_t calculate_aligned_size(size_t size, size_t alignment)
{
    if (alignment <= 1)
    {
        return size;
    }
    
    return ((size + alignment - 1) / alignment) * alignment;
}

/**
 * @brief 分配新的内存块
 * 
 * @param arena 区域分配器
 * @param required_size 需要的大小
 * @return 分配成功返回true，失败返回false
 */
static bool allocate_new_block(Stru_CN_ArenaAllocator_t* arena, size_t required_size)
{
    // 计算新块大小
    size_t new_block_size = arena->current_block_size;
    
    if (new_block_size == 0)
    {
        // 第一个块
        new_block_size = arena->config.initial_size;
    }
    
    // 确保块大小足够
    while (new_block_size < required_size)
    {
        if (!arena->config.auto_expand)
        {
            return false;
        }
        
        new_block_size += arena->config.expand_increment;
        
        // 检查最大大小限制
        if (arena->config.max_size > 0 && new_block_size > arena->config.max_size)
        {
            return false;
        }
    }
    
    // 分配内存块
    void* new_memory = malloc(new_block_size);
    if (new_memory == NULL)
    {
        return false;
    }
    
    // 如果配置要求，清零内存
    if (arena->config.zero_on_alloc)
    {
        memset(new_memory, 0, new_block_size);
    }
    
    // 更新区域状态
    arena->memory = new_memory;
    arena->current_offset = 0;
    arena->current_block_size = new_block_size;
    arena->total_allocated_size += new_block_size;
    
    // 更新统计信息
    if (arena->config.enable_statistics)
    {
        arena->stats.arena_expansions++;
        
        if (arena->stats.peak_usage < arena->total_allocated_size)
        {
            arena->stats.peak_usage = arena->total_allocated_size;
        }
    }
    
    return true;
}

// ============================================================================
// 公共接口实现
// ============================================================================

Stru_CN_ArenaAllocator_t* CN_arena_create(const Stru_CN_ArenaConfig_t* config)
{
    // 验证配置
    if (config == NULL)
    {
        return NULL;
    }
    
    // 验证对齐值
    if (!is_valid_alignment(config->alignment))
    {
        return NULL;
    }
    
    // 分配区域分配器结构
    Stru_CN_ArenaAllocator_t* arena = (Stru_CN_ArenaAllocator_t*)malloc(sizeof(Stru_CN_ArenaAllocator_t));
    if (arena == NULL)
    {
        return NULL;
    }
    
    // 初始化结构
    memset(arena, 0, sizeof(Stru_CN_ArenaAllocator_t));
    
    // 复制配置
    arena->config = *config;
    
    // 初始化锁（如果启用线程安全）
    // 注意：这里简化实现，实际需要平台特定的锁实现
    arena->lock = NULL;
    
    // 初始化调试回调
    arena->debug_callback = NULL;
    arena->debug_user_data = NULL;
    
    // 初始化统计信息
    if (arena->config.enable_statistics)
    {
        memset(&arena->stats, 0, sizeof(Stru_CN_ArenaStats_t));
    }
    
    // 分配初始内存块
    if (arena->config.initial_size > 0)
    {
        if (!allocate_new_block(arena, arena->config.initial_size))
        {
            free(arena);
            return NULL;
        }
    }
    
    return arena;
}

void CN_arena_destroy(Stru_CN_ArenaAllocator_t* arena)
{
    if (arena == NULL)
    {
        return;
    }
    
    // 释放所有内存块
    if (arena->memory != NULL)
    {
        free(arena->memory);
        arena->memory = NULL;
    }
    
    // 释放锁（如果存在）
    if (arena->lock != NULL)
    {
        // 注意：这里简化实现，实际需要平台特定的锁销毁
        free(arena->lock);
    }
    
    // 释放区域分配器结构
    free(arena);
}

void* CN_arena_alloc(Stru_CN_ArenaAllocator_t* arena, size_t size)
{
    return CN_arena_alloc_aligned(arena, size, arena->config.alignment);
}

void* CN_arena_alloc_aligned(Stru_CN_ArenaAllocator_t* arena, size_t size, size_t alignment)
{
    if (arena == NULL || size == 0)
    {
        return NULL;
    }
    
    // 验证对齐值
    if (!is_valid_alignment(alignment))
    {
        return NULL;
    }
    
    // 线程安全：加锁（如果启用）
    // 这里简化实现
    
    // 计算对齐后的分配大小
    size_t aligned_size = calculate_aligned_size(size, alignment);
    
    // 计算对齐填充
    uintptr_t current_addr = (uintptr_t)arena->memory + arena->current_offset;
    size_t padding = CN_arena_padding_size(current_addr, alignment);
    
    // 计算总需要大小
    size_t total_needed = aligned_size + padding;
    
    // 检查是否有足够空间
    if (arena->memory == NULL || 
        arena->current_offset + total_needed > arena->current_block_size)
    {
        // 需要新块
        if (!allocate_new_block(arena, total_needed))
        {
            // 更新统计信息
            if (arena->config.enable_statistics)
            {
                arena->stats.allocation_failures++;
            }
            
            // 线程安全：解锁
            return NULL;
        }
        
        // 重新计算（新块从0开始）
        current_addr = (uintptr_t)arena->memory;
        padding = CN_arena_padding_size(current_addr, alignment);
    }
    
    // 计算分配地址
    void* allocated_ptr = (void*)(current_addr + padding);
    
    // 更新偏移量
    arena->current_offset += total_needed;
    
    // 如果配置要求，清零内存
    if (arena->config.zero_on_alloc)
    {
        memset(allocated_ptr, 0, size);
    }
    
    // 更新统计信息
    if (arena->config.enable_statistics)
    {
        arena->stats.total_allocated += size;
        arena->stats.current_usage += total_needed;
        arena->stats.allocation_count++;
        arena->stats.wasted_space += padding;
        
        if (arena->stats.current_usage > arena->stats.peak_usage)
        {
            arena->stats.peak_usage = arena->stats.current_usage;
        }
    }
    
    // 线程安全：解锁
    
    return allocated_ptr;
}

void CN_arena_reset(Stru_CN_ArenaAllocator_t* arena, bool zero_memory)
{
    if (arena == NULL || arena->memory == NULL)
    {
        return;
    }
    
    // 线程安全：加锁
    
    // 如果要求，清零内存
    if (zero_memory)
    {
        memset(arena->memory, 0, arena->current_offset);
    }
    
    // 更新统计信息
    if (arena->config.enable_statistics)
    {
        arena->stats.total_freed += arena->stats.current_usage;
        arena->stats.current_usage = 0;
        arena->stats.arena_resets++;
    }
    
    // 重置偏移量
    arena->current_offset = 0;
    
    // 线程安全：解锁
}

bool CN_arena_get_config(Stru_CN_ArenaAllocator_t* arena, Stru_CN_ArenaConfig_t* config)
{
    if (arena == NULL || config == NULL)
    {
        return false;
    }
    
    *config = arena->config;
    return true;
}

bool CN_arena_get_stats(Stru_CN_ArenaAllocator_t* arena, Stru_CN_ArenaStats_t* stats)
{
    if (arena == NULL || stats == NULL || !arena->config.enable_statistics)
    {
        return false;
    }
    
    *stats = arena->stats;
    return true;
}

void CN_arena_reset_stats(Stru_CN_ArenaAllocator_t* arena)
{
    if (arena == NULL || !arena->config.enable_statistics)
    {
        return;
    }
    
    memset(&arena->stats, 0, sizeof(Stru_CN_ArenaStats_t));
}

float CN_arena_utilization(Stru_CN_ArenaAllocator_t* arena)
{
    if (arena == NULL || arena->current_block_size == 0)
    {
        return -1.0f;
    }
    
    return (float)arena->current_offset / (float)arena->current_block_size;
}

size_t CN_arena_remaining(Stru_CN_ArenaAllocator_t* arena)
{
    if (arena == NULL || arena->memory == NULL)
    {
        return 0;
    }
    
    return arena->current_block_size - arena->current_offset;
}

size_t CN_arena_total_size(Stru_CN_ArenaAllocator_t* arena)
{
    if (arena == NULL)
    {
        return 0;
    }
    
    return arena->current_block_size;
}
