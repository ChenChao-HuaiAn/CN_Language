/**
 * @file CN_pool_allocator.c
 * @brief 对象池分配器实现
 * 
 * 实现了固定大小对象的高效内存分配器，通过对象池减少内存碎片。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_pool_allocator.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 对象池分配器私有数据结构
 */
struct Stru_PoolAllocatorData_t
{
    size_t object_size;                         ///< 每个对象的大小（字节）
    size_t pool_size;                           ///< 池中对象的总数
    size_t allocated_objects;                   ///< 已分配的对象数
    size_t allocation_failures;                 ///< 分配失败次数
    void* memory_block;                         ///< 内存块指针
    void** free_list;                           ///< 空闲对象链表
    size_t free_count;                          ///< 空闲对象数量
    Stru_MemoryAllocatorInterface_t* parent_allocator; ///< 父分配器
};

/**
 * @brief 对象池分配器分配函数
 */
static void* pool_allocate(Stru_MemoryAllocatorInterface_t* allocator, 
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
    
    // 忽略对齐参数，对象池内部保证对齐
    (void)alignment;
    
    // 检查是否有空闲对象
    if (pool_data->free_count == 0)
    {
        pool_data->allocation_failures++;
        return NULL;
    }
    
    // 从空闲链表中获取一个对象
    void* object = pool_data->free_list[--pool_data->free_count];
    pool_data->allocated_objects++;
    
    return object;
}

/**
 * @brief 对象池分配器重新分配函数
 */
static void* pool_reallocate(Stru_MemoryAllocatorInterface_t* allocator,
                            void* ptr, size_t new_size)
{
    Stru_PoolAllocatorData_t* pool_data = (Stru_PoolAllocatorData_t*)allocator->private_data;
    
    // 对象池不支持重新分配，使用父分配器
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
static void pool_deallocate(Stru_MemoryAllocatorInterface_t* allocator,
                           void* ptr)
{
    Stru_PoolAllocatorData_t* pool_data = (Stru_PoolAllocatorData_t*)allocator->private_data;
    
    // 检查指针是否在对象池范围内
    if (ptr < pool_data->memory_block || 
        ptr >= (char*)pool_data->memory_block + pool_data->pool_size * pool_data->object_size)
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
static void pool_get_statistics(Stru_MemoryAllocatorInterface_t* allocator,
                               size_t* total_allocated, size_t* total_freed,
                               size_t* current_usage, size_t* allocation_count)
{
    Stru_PoolAllocatorData_t* pool_data = (Stru_PoolAllocatorData_t*)allocator->private_data;
    
    if (total_allocated != NULL)
    {
        *total_allocated = pool_data->allocated_objects * pool_data->object_size;
    }
    
    if (total_freed != NULL)
    {
        *total_freed = (pool_data->pool_size - pool_data->allocated_objects) * pool_data->object_size;
    }
    
    if (current_usage != NULL)
    {
        *current_usage = pool_data->allocated_objects * pool_data->object_size;
    }
    
    if (allocation_count != NULL)
    {
        *allocation_count = pool_data->allocated_objects;
    }
}

/**
 * @brief 对象池分配器验证函数
 */
static bool pool_validate(Stru_MemoryAllocatorInterface_t* allocator,
                         void* ptr)
{
    Stru_PoolAllocatorData_t* pool_data = (Stru_PoolAllocatorData_t*)allocator->private_data;
    
    // 检查指针是否在对象池范围内
    if (ptr < pool_data->memory_block || 
        ptr >= (char*)pool_data->memory_block + pool_data->pool_size * pool_data->object_size)
    {
        return false;
    }
    
    // 检查指针是否对齐到对象大小
    size_t offset = (char*)ptr - (char*)pool_data->memory_block;
    if (offset % pool_data->object_size != 0)
    {
        return false;
    }
    
    return true;
}

/**
 * @brief 对象池分配器清理函数
 */
static void pool_cleanup(Stru_MemoryAllocatorInterface_t* allocator)
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
        
        // 释放内存块
        if (pool_data->memory_block != NULL)
        {
            if (pool_data->parent_allocator != NULL)
            {
                pool_data->parent_allocator->deallocate(pool_data->parent_allocator, pool_data->memory_block);
            }
            else
            {
                free(pool_data->memory_block);
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

/**
 * @brief 创建对象池分配器
 */
Stru_MemoryAllocatorInterface_t* F_create_pool_allocator(
    size_t object_size, size_t pool_size,
    Stru_MemoryAllocatorInterface_t* parent_allocator)
{
    // 验证参数
    if (object_size == 0 || pool_size == 0)
    {
        return NULL;
    }
    
    // 分配池数据
    Stru_PoolAllocatorData_t* pool_data;
    if (parent_allocator != NULL)
    {
        pool_data = (Stru_PoolAllocatorData_t*)parent_allocator->allocate(
            parent_allocator, sizeof(Stru_PoolAllocatorData_t), 0);
    }
    else
    {
        pool_data = (Stru_PoolAllocatorData_t*)malloc(sizeof(Stru_PoolAllocatorData_t));
    }
    
    if (pool_data == NULL)
    {
        return NULL;
    }
    
    // 初始化池数据
    memset(pool_data, 0, sizeof(Stru_PoolAllocatorData_t));
    pool_data->object_size = object_size;
    pool_data->pool_size = pool_size;
    pool_data->parent_allocator = parent_allocator;
    
    // 分配内存块
    size_t total_memory = object_size * pool_size;
    if (parent_allocator != NULL)
    {
        pool_data->memory_block = parent_allocator->allocate(parent_allocator, total_memory, 0);
    }
    else
    {
        pool_data->memory_block = malloc(total_memory);
    }
    
    if (pool_data->memory_block == NULL)
    {
        if (parent_allocator != NULL)
        {
            parent_allocator->deallocate(parent_allocator, pool_data);
        }
        else
        {
            free(pool_data);
        }
        return NULL;
    }
    
    // 分配空闲链表
    if (parent_allocator != NULL)
    {
        pool_data->free_list = (void**)parent_allocator->allocate(
            parent_allocator, pool_size * sizeof(void*), 0);
    }
    else
    {
        pool_data->free_list = (void**)malloc(pool_size * sizeof(void*));
    }
    
    if (pool_data->free_list == NULL)
    {
        if (parent_allocator != NULL)
        {
            parent_allocator->deallocate(parent_allocator, pool_data->memory_block);
            parent_allocator->deallocate(parent_allocator, pool_data);
        }
        else
        {
            free(pool_data->memory_block);
            free(pool_data);
        }
        return NULL;
    }
    
    // 初始化空闲链表
    pool_data->free_count = pool_size;
    for (size_t i = 0; i < pool_size; i++)
    {
        pool_data->free_list[i] = (char*)pool_data->memory_block + i * object_size;
    }
    
    // 创建分配器接口
    Stru_MemoryAllocatorInterface_t* allocator;
    if (parent_allocator != NULL)
    {
        allocator = (Stru_MemoryAllocatorInterface_t*)parent_allocator->allocate(
            parent_allocator, sizeof(Stru_MemoryAllocatorInterface_t), 0);
    }
    else
    {
        allocator = (Stru_MemoryAllocatorInterface_t*)malloc(sizeof(Stru_MemoryAllocatorInterface_t));
    }
    
    if (allocator == NULL)
    {
        if (parent_allocator != NULL)
        {
            parent_allocator->deallocate(parent_allocator, pool_data->free_list);
            parent_allocator->deallocate(parent_allocator, pool_data->memory_block);
            parent_allocator->deallocate(parent_allocator, pool_data);
        }
        else
        {
            free(pool_data->free_list);
            free(pool_data->memory_block);
            free(pool_data);
        }
        return NULL;
    }
    
    // 初始化分配器接口
    allocator->allocate = pool_allocate;
    allocator->reallocate = pool_reallocate;
    allocator->deallocate = pool_deallocate;
    allocator->get_statistics = pool_get_statistics;
    allocator->validate = pool_validate;
    allocator->cleanup = pool_cleanup;
    allocator->private_data = pool_data;
    
    return allocator;
}

/**
 * @brief 获取对象池分配器的统计信息
 */
void F_get_pool_allocator_statistics(
    Stru_MemoryAllocatorInterface_t* allocator,
    size_t* total_objects, size_t* allocated_objects,
    size_t* available_objects, size_t* allocation_failures)
{
    if (allocator == NULL || allocator->private_data == NULL)
    {
        return;
    }
    
    Stru_PoolAllocatorData_t* pool_data = (Stru_PoolAllocatorData_t*)allocator->private_data;
    
    if (total_objects != NULL)
    {
        *total_objects = pool_data->pool_size;
    }
    
    if (allocated_objects != NULL)
    {
        *allocated_objects = pool_data->allocated_objects;
    }
    
    if (available_objects != NULL)
    {
        *available_objects = pool_data->free_count;
    }
    
    if (allocation_failures != NULL)
    {
        *allocation_failures = pool_data->allocation_failures;
    }
}

/**
 * @brief 扩展对象池容量
 */
bool F_expand_pool_allocator(
    Stru_MemoryAllocatorInterface_t* allocator,
    size_t additional_objects)
{
    if (allocator == NULL || allocator->private_data == NULL || additional_objects == 0)
    {
        return false;
    }
    
    Stru_PoolAllocatorData_t* pool_data = (Stru_PoolAllocatorData_t*)allocator->private_data;
    
    // 计算新的总大小
    size_t new_pool_size = pool_data->pool_size + additional_objects;
    
    // 重新分配内存块
    size_t new_total_memory = pool_data->object_size * new_pool_size;
    void* new_memory_block;
    
    if (pool_data->parent_allocator != NULL)
    {
        new_memory_block = pool_data->parent_allocator->reallocate(
            pool_data->parent_allocator, pool_data->memory_block, new_total_memory);
    }
    else
    {
        new_memory_block = realloc(pool_data->memory_block, new_total_memory);
    }
    
    if (new_memory_block == NULL)
    {
        return false;
    }
    
    pool_data->memory_block = new_memory_block;
    
    // 重新分配空闲链表
    void** new_free_list;
    if (pool_data->parent_allocator != NULL)
    {
        new_free_list = (void**)pool_data->parent_allocator->reallocate(
            pool_data->parent_allocator, pool_data->free_list, new_pool_size * sizeof(void*));
    }
    else
    {
        new_free_list = (void**)realloc(pool_data->free_list, new_pool_size * sizeof(void*));
    }
    
    if (new_free_list == NULL)
    {
        return false;
    }
    
    pool_data->free_list = new_free_list;
    
    // 添加新对象到空闲链表
    for (size_t i = pool_data->pool_size; i < new_pool_size; i++)
    {
        pool_data->free_list[pool_data->free_count++] = 
            (char*)pool_data->memory_block + i * pool_data->object_size;
    }
    
    // 更新池大小
    pool_data->pool_size = new_pool_size;
    
    return true;
}

#ifdef __cplusplus
}
#endif
