/******************************************************************************
 * 文件名: CN_arena_operations.c
 * 功能: CN_Language区域分配器批量操作和调试实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建区域分配器批量操作和调试实现
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_arena_allocator.h"
#include "CN_arena_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// 批量操作接口实现
// ============================================================================

bool CN_arena_reserve(Stru_CN_ArenaAllocator_t* arena, size_t size)
{
    if (arena == NULL || size == 0)
    {
        return false;
    }
    
    // 检查当前剩余空间是否足够
    size_t remaining = CN_arena_remaining(arena);
    if (remaining >= size)
    {
        return true;  // 已有足够空间
    }
    
    // 计算需要额外分配的空间
    size_t needed = size - remaining;
    
    // 扩展区域
    return CN_arena_expand(arena, needed);
}

bool CN_arena_expand(Stru_CN_ArenaAllocator_t* arena, size_t additional_size)
{
    if (arena == NULL || additional_size == 0)
    {
        return false;
    }
    
    // 线程安全：加锁
    
    // 计算新大小
    size_t new_size = arena->current_block_size + additional_size;
    
    // 检查最大大小限制
    if (arena->config.max_size > 0 && new_size > arena->config.max_size)
    {
        // 线程安全：解锁
        return false;
    }
    
    // 重新分配内存
    void* new_memory = realloc(arena->memory, new_size);
    if (new_memory == NULL)
    {
        // 线程安全：解锁
        return false;
    }
    
    // 如果扩展部分需要清零
    if (arena->config.zero_on_alloc)
    {
        memset((char*)new_memory + arena->current_block_size, 0, additional_size);
    }
    
    // 更新状态
    arena->memory = new_memory;
    arena->current_block_size = new_size;
    arena->total_allocated_size += additional_size;
    
    // 更新统计信息
    if (arena->config.enable_statistics)
    {
        arena->stats.arena_expansions++;
        
        if (arena->stats.peak_usage < arena->total_allocated_size)
        {
            arena->stats.peak_usage = arena->total_allocated_size;
        }
    }
    
    // 调用调试回调
    if (arena->debug_callback != NULL)
    {
        char message[256];
        snprintf(message, sizeof(message), 
                "区域已扩展: 原大小=%zu, 新增=%zu, 新大小=%zu",
                arena->current_block_size - additional_size,
                additional_size, arena->current_block_size);
        arena->debug_callback(message, arena->debug_user_data);
    }
    
    // 线程安全：解锁
    return true;
}

bool CN_arena_shrink(Stru_CN_ArenaAllocator_t* arena)
{
    if (arena == NULL || arena->memory == NULL)
    {
        return false;
    }
    
    // 线程安全：加锁
    
    // 计算建议的新大小（当前使用量加上一些额外空间）
    size_t suggested_size = arena->current_offset;
    
    // 至少保留初始大小
    if (suggested_size < arena->config.initial_size)
    {
        suggested_size = arena->config.initial_size;
    }
    
    // 对齐到扩展增量
    if (arena->config.expand_increment > 0)
    {
        suggested_size = ((suggested_size + arena->config.expand_increment - 1) / 
                         arena->config.expand_increment) * arena->config.expand_increment;
    }
    
    // 如果新大小不小于当前大小，不需要收缩
    if (suggested_size >= arena->current_block_size)
    {
        // 线程安全：解锁
        return true;
    }
    
    // 重新分配内存
    void* new_memory = realloc(arena->memory, suggested_size);
    if (new_memory == NULL)
    {
        // 收缩失败，但原内存仍然有效
        // 线程安全：解锁
        return false;
    }
    
    // 计算释放的大小
    size_t freed_size = arena->current_block_size - suggested_size;
    
    // 更新状态
    arena->memory = new_memory;
    arena->current_block_size = suggested_size;
    arena->total_allocated_size -= freed_size;
    
    // 调用调试回调
    if (arena->debug_callback != NULL)
    {
        char message[256];
        snprintf(message, sizeof(message), 
                "区域已收缩: 原大小=%zu, 新大小=%zu, 释放=%zu",
                arena->current_block_size + freed_size,
                arena->current_block_size, freed_size);
        arena->debug_callback(message, arena->debug_user_data);
    }
    
    // 线程安全：解锁
    return true;
}

// ============================================================================
// 调试接口实现
// ============================================================================

bool CN_arena_validate(Stru_CN_ArenaAllocator_t* arena)
{
    if (arena == NULL)
    {
        return false;
    }
    
    // 基本验证
    if (arena->memory == NULL && arena->current_offset > 0)
    {
        return false;  // 有偏移量但没有内存
    }
    
    if (arena->current_offset > arena->current_block_size)
    {
        return false;  // 偏移量超出块大小
    }
    
    if (arena->config.max_size > 0 && arena->current_block_size > arena->config.max_size)
    {
        return false;  // 块大小超出最大限制
    }
    
    // 验证对齐值
    if (!(arena->config.alignment > 0 && 
          (arena->config.alignment & (arena->config.alignment - 1)) == 0))
    {
        return false;  // 对齐值无效
    }
    
    // 统计信息验证（如果启用）
    if (arena->config.enable_statistics)
    {
        if (arena->stats.current_usage > arena->stats.peak_usage)
        {
            return false;  // 当前使用量不应大于峰值
        }
        
        if (arena->stats.current_usage > arena->current_block_size)
        {
            return false;  // 当前使用量不应大于块大小
        }
    }
    
    return true;
}

void CN_arena_dump(Stru_CN_ArenaAllocator_t* arena)
{
    if (arena == NULL)
    {
        printf("区域分配器: NULL\n");
        return;
    }
    
    printf("=== 区域分配器状态转储 ===\n");
    printf("内存地址: %p\n", arena->memory);
    printf("当前偏移量: %zu 字节\n", arena->current_offset);
    printf("当前块大小: %zu 字节\n", arena->current_block_size);
    printf("总分配大小: %zu 字节\n", arena->total_allocated_size);
    printf("剩余空间: %zu 字节\n", CN_arena_remaining(arena));
    printf("使用率: %.2f%%\n", CN_arena_utilization(arena) * 100.0f);
    
    printf("\n配置信息:\n");
    printf("  初始大小: %zu 字节\n", arena->config.initial_size);
    printf("  最大大小: %zu 字节\n", arena->config.max_size);
    printf("  对齐要求: %zu 字节\n", arena->config.alignment);
    printf("  自动扩展: %s\n", arena->config.auto_expand ? "是" : "否");
    printf("  扩展增量: %zu 字节\n", arena->config.expand_increment);
    printf("  分配清零: %s\n", arena->config.zero_on_alloc ? "是" : "否");
    printf("  线程安全: %s\n", arena->config.thread_safe ? "是" : "否");
    printf("  启用统计: %s\n", arena->config.enable_statistics ? "是" : "否");
    
    if (arena->config.enable_statistics)
    {
        printf("\n统计信息:\n");
        printf("  总分配字节数: %zu\n", arena->stats.total_allocated);
        printf("  总释放字节数: %zu\n", arena->stats.total_freed);
        printf("  当前使用字节数: %zu\n", arena->stats.current_usage);
        printf("  峰值使用字节数: %zu\n", arena->stats.peak_usage);
        printf("  分配次数: %zu\n", arena->stats.allocation_count);
        printf("  区域扩展次数: %zu\n", arena->stats.arena_expansions);
        printf("  区域重置次数: %zu\n", arena->stats.arena_resets);
        printf("  分配失败次数: %zu\n", arena->stats.allocation_failures);
        printf("  浪费空间: %zu 字节\n", arena->stats.wasted_space);
    }
    
    printf("===========================\n");
}

void CN_arena_set_debug_callback(Stru_CN_ArenaAllocator_t* arena, 
                                 CN_ArenaDebugCallback_t callback, 
                                 void* user_data)
{
    if (arena == NULL)
    {
        return;
    }
    
    arena->debug_callback = callback;
    arena->debug_user_data = user_data;
}
