/**
 * @file CN_pool_operations.c
 * @brief 对象池分配器操作函数
 * 
 * 实现了对象池分配器的核心操作函数，包括分配、释放、重新分配等。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-12
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_pool_core.h"
#include "../utils/CN_pool_utils.h"
#include "../../../CN_memory_interface.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 对象池分配器分配函数
 */
void* pool_allocate(Stru_MemoryAllocatorInterface_t* allocator, 
                   size_t size, size_t alignment)
{
    Stru_PoolAllocatorData_t* pool_data = (Stru_PoolAllocatorData_t*)allocator->private_data;
    
    // 检查请求的大小是否匹配对象大小
    if (size != pool_data->object_size)
    {
        // 如果大小不匹配，使用父分配器
        if (pool_data->parent_allocator != NULL)
        {
            return pool_data->parent_allocator->allocate(pool_data->parent_allocator, size, alignment);
        }
        return NULL;
    }
    
    // 检查对齐要求是否满足
    if (alignment > 0)
    {
        // 检查池中对象是否满足对齐要求
        if (pool_data->aligned_object_size % alignment != 0)
        {
            // 如果池中对象的对齐不满足请求的对齐要求，使用父分配器
            if (pool_data->parent_allocator != NULL)
            {
                return pool_data->parent_allocator->allocate(pool_data->parent_allocator, size, alignment);
            }
            return NULL;
        }
    }
    
    // 检查是否有空闲对象
    if (pool_data->free_count == 0)
    {
        pool_data->allocation_failures++;
        return NULL;
    }
    
    // 从空闲链表中获取一个对象
    void* object = pool_data->free_list[--pool_data->free_count];
    pool_data->allocated_objects++;
    
    // 验证对象是否满足对齐要求（双重检查）
    if (alignment > 0 && ((uintptr_t)object % alignment != 0))
    {
        // 如果对象不满足对齐要求，将其放回空闲链表并使用父分配器
        pool_data->free_list[pool_data->free_count++] = object;
        pool_data->allocated_objects--;
        
        if (pool_data->parent_allocator != NULL)
        {
            return pool_data->parent_allocator->allocate(pool_data->parent_allocator, size, alignment);
        }
        return NULL;
    }
    
    return object;
}

/**
 * @brief 对象池分配器重新分配函数
 */
void* pool_reallocate(Stru_MemoryAllocatorInterface_t* allocator,
                     void* ptr, size_t new_size)
{
    Stru_PoolAllocatorData_t* pool_data = (Stru_PoolAllocatorData_t*)allocator->private_data;
    
    // 检查指针是否在对象池范围内
    if (F_is_pointer_in_pool_range(pool_data, ptr))
    {
        // 对象池不支持重新分配固定大小的对象
        return NULL;
    }
    
    // 如果不在对象池范围内，使用父分配器
    if (pool_data->parent_allocator != NULL)
    {
        return pool_data->parent_allocator->reallocate(pool_data->parent_allocator, ptr, new_size);
    }
    
    // 如果没有父分配器，返回NULL表示不支持
    return NULL;
}

/**
 * @brief 对象池分配器释放函数
 */
void pool_deallocate(Stru_MemoryAllocatorInterface_t* allocator,
                    void* ptr)
{
    Stru_PoolAllocatorData_t* pool_data = (Stru_PoolAllocatorData_t*)allocator->private_data;
    
    // 检查指针是否在对象池范围内
    if (!F_is_pointer_in_pool_range(pool_data, ptr))
    {
        // 如果不在对象池范围内，使用父分配器
        if (pool_data->parent_allocator != NULL)
        {
            pool_data->parent_allocator->deallocate(pool_data->parent_allocator, ptr);
        }
        return;
    }
    
    // 将对象放回空闲链表
    if (pool_data->free_count < pool_data->pool_size)
    {
        pool_data->free_list[pool_data->free_count++] = ptr;
        pool_data->allocated_objects--;
    }
}

/**
 * @brief 对象池分配器获取统计信息函数
 */
void pool_get_statistics(Stru_MemoryAllocatorInterface_t* allocator,
                        size_t* total_allocated, size_t* total_freed,
                        size_t* current_usage, size_t* allocation_count)
{
    Stru_PoolAllocatorData_t* pool_data = (Stru_PoolAllocatorData_t*)allocator->private_data;
    
    if (total_allocated != NULL)
    {
        *total_allocated = pool_data->allocated_objects * pool_data->aligned_object_size;
    }
    
    if (total_freed != NULL)
    {
        *total_freed = (pool_data->pool_size - pool_data->allocated_objects) * pool_data->aligned_object_size;
    }
    
    if (current_usage != NULL)
    {
        *current_usage = pool_data->allocated_objects * pool_data->aligned_object_size;
    }
    
    if (allocation_count != NULL)
    {
        *allocation_count = pool_data->allocated_objects;
    }
}

/**
 * @brief 对象池分配器验证函数
 */
bool pool_validate(Stru_MemoryAllocatorInterface_t* allocator,
                  void* ptr)
{
    Stru_PoolAllocatorData_t* pool_data = (Stru_PoolAllocatorData_t*)allocator->private_data;
    
    // 检查指针是否在对象池范围内
    if (!F_is_pointer_in_pool_range(pool_data, ptr))
    {
        return false;
    }
    
    // 检查指针是否对齐到对象大小
    return F_is_pointer_aligned_to_object(pool_data, ptr);
}

/**
 * @brief 对象池分配器清理函数
 */
void pool_cleanup(Stru_MemoryAllocatorInterface_t* allocator)
{
    Stru_PoolAllocatorData_t* pool_data = (Stru_PoolAllocatorData_t*)allocator->private_data;
    
    if (pool_data != NULL)
    {
        // 释放空闲链表
        if (pool_data->free_list != NULL)
        {
            if (pool_data->parent_allocator != NULL)
            {
                pool_data->parent_allocator->deallocate(pool_data->parent_allocator, pool_data->free_list);
            }
            else
            {
                free(pool_data->free_list);
            }
        }
        
        // 释放内存块（正确处理对齐内存）
        if (pool_data->memory_block != NULL)
        {
            if (pool_data->parent_allocator != NULL)
            {
                pool_data->parent_allocator->deallocate(pool_data->parent_allocator, pool_data->memory_block);
            }
            else
            {
                F_free_aligned_memory(pool_data->memory_block);
            }
        }
        
        // 释放池数据本身
        if (pool_data->parent_allocator != NULL)
        {
            pool_data->parent_allocator->deallocate(pool_data->parent_allocator, pool_data);
        }
        else
        {
            free(pool_data);
        }
        
        allocator->private_data = NULL;
    }
}

#ifdef __cplusplus
}
#endif
