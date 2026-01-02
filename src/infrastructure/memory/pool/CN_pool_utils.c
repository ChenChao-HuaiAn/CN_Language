/******************************************************************************
 * 文件名: CN_pool_utils.c
 * 功能: CN_Language对象池分配器工具函数实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 从CN_pool_allocator.c拆分，创建工具模块
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_pool_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

// ============================================================================
// 公共接口实现 - 调试和验证功能
// ============================================================================

bool CN_pool_validate(Stru_CN_PoolAllocator_t* pool)
{
    if (pool == NULL || !pool->initialized)
    {
        return false;
    }
    
    bool valid = true;
    size_t total_allocated_from_blocks = 0;
    size_t total_capacity = 0;
    
    // 验证所有块
    Stru_CN_PoolBlock_t* block = pool->first_block;
    while (block != NULL)
    {
        // 验证块指针
        if (block->memory == NULL || block->usage_bitmap == NULL)
        {
            pool_debug_log(pool, "验证失败: 块 %p 有NULL指针", block);
            valid = false;
        }
        
        // 验证容量
        if (block->capacity == 0)
        {
            pool_debug_log(pool, "验证失败: 块 %p 容量为0", block);
            valid = false;
        }
        
        // 验证使用计数
        size_t actual_used = 0;
        size_t bitmap_size = (block->capacity + 7) / 8;
        for (size_t i = 0; i < bitmap_size; i++)
        {
            unsigned char byte = block->usage_bitmap[i];
            for (int j = 0; j < 8; j++)
            {
                size_t obj_idx = i * 8 + j;
                if (obj_idx < block->capacity)
                {
                    if (byte & (1 << j))
                    {
                        actual_used++;
                    }
                }
            }
        }
        
        if (actual_used != block->used_count)
        {
            pool_debug_log(pool, "验证失败: 块 %p 使用计数不匹配 (位图: %zu, 记录: %zu)", 
                          block, actual_used, block->used_count);
            valid = false;
        }
        
        total_allocated_from_blocks += block->used_count;
        total_capacity += block->capacity;
        
        block = block->next;
    }
    
    // 验证空闲链表
    size_t free_list_count = 0;
    void** current = (void**)&pool->free_list;
    while (*current != NULL)
    {
        free_list_count++;
        current = (void**)*current;
    }
    
    // 验证统计信息一致性
    // 注意：空闲链表中的对象在块的位图中是空闲的
    // 所以块中统计的已分配对象应该等于池统计的已分配对象
    if (total_allocated_from_blocks != pool->stats.allocated_objects)
    {
        pool_debug_log(pool, "验证失败: 分配对象计数不匹配 (块统计: %zu, 池统计: %zu)", 
                      total_allocated_from_blocks, pool->stats.allocated_objects);
        valid = false;
    }
    
    // 验证总对象数
    if (total_capacity != pool->stats.total_objects)
    {
        pool_debug_log(pool, "验证失败: 总对象数不匹配 (块容量: %zu, 池统计: %zu)", 
                      total_capacity, pool->stats.total_objects);
        valid = false;
    }
    
    // 验证空闲对象计数
    // 空闲对象 = 总对象 - 已分配对象
    size_t calculated_free_objects = pool->stats.total_objects - pool->stats.allocated_objects;
    if (calculated_free_objects != pool->stats.free_objects)
    {
        pool_debug_log(pool, "验证失败: 空闲对象计数不匹配 (计算: %zu, 池统计: %zu)", 
                      calculated_free_objects, pool->stats.free_objects);
        valid = false;
    }
    
    // 验证空闲链表计数不超过总空闲对象
    if (free_list_count > calculated_free_objects)
    {
        pool_debug_log(pool, "验证失败: 空闲链表计数 (%zu) 超过总空闲对象 (%zu)", 
                      free_list_count, calculated_free_objects);
        valid = false;
    }
    
    if (valid)
    {
        pool_debug_log(pool, "对象池验证通过");
    }
    else
    {
        pool_debug_log(pool, "对象池验证失败");
    }
    
    return valid;
}

void CN_pool_dump(Stru_CN_PoolAllocator_t* pool)
{
    if (pool == NULL || !pool->initialized)
    {
        printf("对象池未初始化或为NULL\n");
        return;
    }
    
    printf("=== 对象池状态转储 ===\n");
    printf("配置信息:\n");
    printf("  对象大小: %zu 字节\n", pool->config.object_size);
    printf("  初始容量: %zu 对象\n", pool->config.initial_capacity);
    printf("  最大容量: %zu 对象\n", pool->config.max_capacity);
    printf("  自动扩展: %s\n", pool->config.auto_expand ? "是" : "否");
    printf("  扩展增量: %zu 对象\n", pool->config.expand_increment);
    printf("  线程安全: %s\n", pool->config.thread_safe ? "是" : "否");
    printf("  分配清零: %s\n", pool->config.zero_on_alloc ? "是" : "否");
    printf("  释放清零: %s\n", pool->config.zero_on_free ? "是" : "否");
    
    printf("\n统计信息:\n");
    printf("  总对象数: %zu\n", pool->stats.total_objects);
    printf("  空闲对象: %zu\n", pool->stats.free_objects);
    printf("  分配对象: %zu\n", pool->stats.allocated_objects);
    printf("  总分配次数: %zu\n", pool->stats.total_allocations);
    printf("  总释放次数: %zu\n", pool->stats.total_deallocations);
    printf("  池扩展次数: %zu\n", pool->stats.pool_expansions);
    printf("  内存使用量: %zu 字节\n", pool->stats.memory_usage);
    printf("  峰值内存使用: %zu 字节\n", pool->stats.peak_memory_usage);
    printf("  分配失败次数: %zu\n", pool->stats.allocation_failures);
    printf("  使用率: %.2f%%\n", CN_pool_utilization(pool) * 100.0f);
    
    printf("\n内存块信息 (%zu 个块):\n", pool->block_count);
    Stru_CN_PoolBlock_t* block = pool->first_block;
    size_t block_index = 0;
    while (block != NULL)
    {
        printf("  块 #%zu: 容量=%zu, 已用=%zu, 使用率=%.1f%%\n",
               block_index, block->capacity, block->used_count,
               (block->capacity > 0) ? (100.0f * block->used_count / block->capacity) : 0.0f);
        block = block->next;
        block_index++;
    }
    
    printf("========================\n");
}

void CN_pool_set_debug_callback(Stru_CN_PoolAllocator_t* pool, 
                                CN_PoolDebugCallback_t callback, 
                                void* user_data)
{
    if (pool == NULL || !pool->initialized)
    {
        return;
    }
    
    pool->debug_callback = callback;
    pool->debug_user_data = user_data;
    
    if (callback != NULL)
    {
        pool_debug_log(pool, "调试回调已设置");
    }
}
