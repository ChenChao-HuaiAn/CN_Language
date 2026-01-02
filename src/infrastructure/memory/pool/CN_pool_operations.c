/******************************************************************************
 * 文件名: CN_pool_operations.c
 * 功能: CN_Language对象池分配器操作实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 从CN_pool_allocator.c拆分，创建操作模块
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_pool_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

// ============================================================================
// 公共接口实现 - 分配和释放操作
// ============================================================================

void* CN_pool_alloc(Stru_CN_PoolAllocator_t* pool)
{
    if (pool == NULL || !pool->initialized)
    {
        return NULL;
    }
    
    // 首先尝试从空闲链表中获取
    if (pool->free_list != NULL)
    {
        void* obj = pool->free_list;
        // 从空闲链表中移除（使用嵌入链表技术）
        pool->free_list = *(void**)obj;
        
        // 更新统计信息
        pool->stats.allocated_objects++;
        pool->stats.free_objects--;
        pool->stats.total_allocations++;
        
        // 如果需要，清零内存
        if (pool->config.zero_on_alloc)
        {
            memset(obj, 0, pool->config.object_size);
        }
        
        pool_debug_log(pool, "从空闲链表分配对象: %p", obj);
        return obj;
    }
    
    // 遍历所有块，寻找空闲位置
    struct Stru_CN_PoolBlock_t* block = pool->first_block;
    while (block != NULL)
    {
        if (block->used_count < block->capacity)
        {
            void* obj = allocate_from_block(block, pool->config.object_size);
            if (obj != NULL)
            {
                // 更新统计信息
                pool->stats.allocated_objects++;
                pool->stats.free_objects--;
                pool->stats.total_allocations++;
                
                // 如果需要，清零内存
                if (pool->config.zero_on_alloc)
                {
                    memset(obj, 0, pool->config.object_size);
                }
                
                pool_debug_log(pool, "从块分配对象: %p (块使用率: %zu/%zu)", 
                               obj, block->used_count, block->capacity);
                return obj;
            }
        }
        block = block->next;
    }
    
    // 所有块都满了，尝试扩展
    if (pool->config.auto_expand)
    {
        if (expand_pool(pool))
        {
            // 递归调用，从新扩展的块中分配
            return CN_pool_alloc(pool);
        }
    }
    
    // 分配失败
    pool->stats.allocation_failures++;
    pool_debug_log(pool, "对象分配失败: 池已满且无法扩展");
    return NULL;
}

bool CN_pool_free(Stru_CN_PoolAllocator_t* pool, void* ptr)
{
    if (pool == NULL || !pool->initialized || ptr == NULL)
    {
        return false;
    }
    
    // 首先检查是否在空闲链表中（避免重复释放）
    void** current = (void**)&pool->free_list;
    while (*current != NULL)
    {
        if (*current == ptr)
        {
            pool_debug_log(pool, "重复释放对象: %p", ptr);
            return false;  // 对象已在空闲链表中
        }
        current = (void**)*current;
    }
    
    // 遍历所有块，查找对象
    struct Stru_CN_PoolBlock_t* block = pool->first_block;
    while (block != NULL)
    {
        if (free_in_block(block, ptr, pool->config.object_size))
        {
            // 将对象添加到空闲链表（使用嵌入链表技术）
            *(void**)ptr = pool->free_list;
            pool->free_list = ptr;
            
            // 更新统计信息
            pool->stats.allocated_objects--;
            pool->stats.free_objects++;
            pool->stats.total_deallocations++;
            
            // 如果需要，清零内存
            if (pool->config.zero_on_free)
            {
                memset(ptr, 0, pool->config.object_size);
            }
            
            pool_debug_log(pool, "释放对象到空闲链表: %p", ptr);
            return true;
        }
        block = block->next;
    }
    
    // 对象不属于这个池
    pool_debug_log(pool, "释放失败: 对象 %p 不属于此对象池", ptr);
    return false;
}

void* CN_pool_realloc(Stru_CN_PoolAllocator_t* pool, void* ptr, size_t new_size)
{
    if (pool == NULL || !pool->initialized)
    {
        return NULL;
    }
    
    // 对象池不支持真正的重新分配
    // 如果新大小等于对象大小，返回原指针
    // 否则分配新对象，复制数据，释放原对象
    
    if (new_size == pool->config.object_size)
    {
        return ptr;  // 大小相同，直接返回
    }
    
    if (ptr == NULL)
    {
        // 相当于分配新对象
        return CN_pool_alloc(pool);
    }
    
    // 分配新对象
    void* new_ptr = CN_pool_alloc(pool);
    if (new_ptr == NULL)
    {
        return NULL;
    }
    
    // 复制数据（取较小的大小）
    size_t copy_size = (new_size < pool->config.object_size) ? new_size : pool->config.object_size;
    memcpy(new_ptr, ptr, copy_size);
    
    // 释放原对象
    CN_pool_free(pool, ptr);
    
    pool_debug_log(pool, "重新分配对象: %p -> %p (大小: %zu -> %zu)", 
                   ptr, new_ptr, pool->config.object_size, new_size);
    
    return new_ptr;
}
