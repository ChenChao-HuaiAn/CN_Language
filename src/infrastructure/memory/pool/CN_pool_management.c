/******************************************************************************
 * 文件名: CN_pool_management.c
 * 功能: CN_Language对象池分配器管理功能实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 从CN_pool_allocator.c拆分，创建管理模块
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_pool_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

// ============================================================================
// 公共接口实现 - 查询和管理功能
// ============================================================================

bool CN_pool_get_config(Stru_CN_PoolAllocator_t* pool, Stru_CN_PoolConfig_t* config)
{
    if (pool == NULL || !pool->initialized || config == NULL)
    {
        return false;
    }
    
    *config = pool->config;
    return true;
}

bool CN_pool_get_stats(Stru_CN_PoolAllocator_t* pool, Stru_CN_PoolStats_t* stats)
{
    if (pool == NULL || !pool->initialized || stats == NULL)
    {
        return false;
    }
    
    *stats = pool->stats;
    return true;
}

void CN_pool_reset_stats(Stru_CN_PoolAllocator_t* pool)
{
    if (pool == NULL || !pool->initialized)
    {
        return;
    }
    
    // 保留基本配置信息，重置计数统计
    pool->stats.total_allocations = 0;
    pool->stats.total_deallocations = 0;
    pool->stats.pool_expansions = 0;
    pool->stats.allocation_failures = 0;
    pool->stats.peak_memory_usage = pool->stats.memory_usage;
    
    pool_debug_log(pool, "重置对象池统计信息");
}

bool CN_pool_contains(Stru_CN_PoolAllocator_t* pool, const void* ptr)
{
    if (pool == NULL || !pool->initialized || ptr == NULL)
    {
        return false;
    }
    
    // 检查空闲链表
    void** current = (void**)&pool->free_list;
    while (*current != NULL)
    {
        if (*current == ptr)
        {
            return true;  // 在空闲链表中
        }
        current = (void**)*current;
    }
    
    // 检查所有内存块
    Stru_CN_PoolBlock_t* block = pool->first_block;
    while (block != NULL)
    {
        char* block_start = (char*)block->memory;
        char* block_end = block_start + block->capacity * pool->config.object_size;
        char* obj_ptr = (char*)ptr;
        
        if (obj_ptr >= block_start && obj_ptr < block_end)
        {
            // 检查是否是对齐的地址
            size_t offset = obj_ptr - block_start;
            if (offset % pool->config.object_size == 0)
            {
                size_t obj_idx = offset / pool->config.object_size;
                if (obj_idx < block->capacity)
                {
                    // 检查位图确认是否已分配
                    size_t byte_idx = obj_idx / 8;
                    int bit_idx = obj_idx % 8;
                    return (block->usage_bitmap[byte_idx] & (1 << bit_idx)) != 0;
                }
            }
        }
        block = block->next;
    }
    
    return false;
}

float CN_pool_utilization(Stru_CN_PoolAllocator_t* pool)
{
    if (pool == NULL || !pool->initialized || pool->stats.total_objects == 0)
    {
        return -1.0f;
    }
    
    return (float)pool->stats.allocated_objects / (float)pool->stats.total_objects;
}

bool CN_pool_prealloc(Stru_CN_PoolAllocator_t* pool, size_t count)
{
    if (pool == NULL || !pool->initialized || count == 0)
    {
        return false;
    }
    
    // 计算需要扩展多少
    size_t needed = count;
    if (pool->stats.free_objects < needed)
    {
        needed -= pool->stats.free_objects;
        
        // 计算需要的新块数量
        size_t objects_per_block = pool->config.expand_increment;
        size_t blocks_needed = (needed + objects_per_block - 1) / objects_per_block;
        
        for (size_t i = 0; i < blocks_needed; i++)
        {
            if (!expand_pool(pool))
            {
                return false;
            }
        }
    }
    
    pool_debug_log(pool, "预分配 %zu 个对象完成", count);
    return true;
}

bool CN_pool_clear(Stru_CN_PoolAllocator_t* pool)
{
    if (pool == NULL || !pool->initialized)
    {
        return false;
    }
    
    // 清空空闲链表
    pool->free_list = NULL;
    
    // 重置所有块的位图
    Stru_CN_PoolBlock_t* block = pool->first_block;
    while (block != NULL)
    {
        size_t bitmap_size = (block->capacity + 7) / 8;
        memset(block->usage_bitmap, 0, bitmap_size);
        block->used_count = 0;
        block = block->next;
    }
    
    // 更新统计信息
    pool->stats.free_objects = pool->stats.total_objects;
    pool->stats.allocated_objects = 0;
    
    pool_debug_log(pool, "清空对象池，所有对象标记为空闲");
    return true;
}

bool CN_pool_shrink(Stru_CN_PoolAllocator_t* pool)
{
    if (pool == NULL || !pool->initialized)
    {
        return false;
    }
    
    // 对象池当前设计不支持收缩
    // 可以扩展此功能来释放完全空闲的块
    pool_debug_log(pool, "对象池收缩功能暂未实现");
    return false;
}
