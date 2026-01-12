/**
 * @file CN_pool_allocator_main.c
 * @brief 对象池分配器主接口函数
 * 
 * 实现了对象池分配器的主接口函数，包括创建、统计和扩展功能。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-12
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_pool_allocator.h"
#include "core/CN_pool_core.h"
#include "utils/CN_pool_utils.h"
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// 声明操作函数（在CN_pool_operations.c中定义）
extern void* pool_allocate(Stru_MemoryAllocatorInterface_t* allocator, 
                          size_t size, size_t alignment);
extern void* pool_reallocate(Stru_MemoryAllocatorInterface_t* allocator,
                            void* ptr, size_t new_size);
extern void pool_deallocate(Stru_MemoryAllocatorInterface_t* allocator,
                           void* ptr);
extern void pool_get_statistics(Stru_MemoryAllocatorInterface_t* allocator,
                               size_t* total_allocated, size_t* total_freed,
                               size_t* current_usage, size_t* allocation_count);
extern bool pool_validate(Stru_MemoryAllocatorInterface_t* allocator,
                         void* ptr);
extern void pool_cleanup(Stru_MemoryAllocatorInterface_t* allocator);

/**
 * @brief 创建对象池分配器
 */
Stru_MemoryAllocatorInterface_t* F_create_pool_allocator(
    size_t object_size, size_t pool_size, size_t alignment,
    Stru_MemoryAllocatorInterface_t* parent_allocator)
{
    // 验证参数
    if (object_size == 0 || pool_size == 0)
    {
        return NULL;
    }
    
    // 计算对齐后的大小
    size_t aligned_object_size = F_calculate_aligned_size(object_size, alignment);
    
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
    pool_data->aligned_object_size = aligned_object_size;
    pool_data->pool_size = pool_size;
    pool_data->parent_allocator = parent_allocator;
    
    // 分配内存块（使用对齐后的大小）
    size_t total_memory = aligned_object_size * pool_size;
    if (parent_allocator != NULL)
    {
        pool_data->memory_block = parent_allocator->allocate(parent_allocator, total_memory, alignment);
    }
    else
    {
        pool_data->memory_block = F_allocate_aligned_memory(total_memory, alignment);
        if (pool_data->memory_block == NULL)
        {
            pool_data->memory_block = malloc(total_memory);
        }
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
            F_free_aligned_memory(pool_data->memory_block);
            free(pool_data);
        }
        return NULL;
    }
    
    // 初始化空闲链表（使用对齐后的大小）
    pool_data->free_count = pool_size;
    for (size_t i = 0; i < pool_size; i++)
    {
        pool_data->free_list[i] = (char*)pool_data->memory_block + i * aligned_object_size;
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
            F_free_aligned_memory(pool_data->memory_block);
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
    
    // 检查是否有已分配的对象
    // 如果有已分配的对象，我们不能扩展，因为无法更新已分配对象的指针
    if (pool_data->allocated_objects > 0) {
        return false;
    }
    
    // 计算新的总大小
    size_t new_pool_size = pool_data->pool_size + additional_objects;
    
    // 重新分配内存块（使用对齐后的大小）
    size_t new_total_memory = pool_data->aligned_object_size * new_pool_size;
    void* new_memory_block;
    
    if (pool_data->parent_allocator != NULL)
    {
        new_memory_block = pool_data->parent_allocator->reallocate(
            pool_data->parent_allocator, pool_data->memory_block, new_total_memory);
    }
    else
    {
        // 对于对齐的内存，我们不能简单地使用realloc
        // 需要分配新的对齐内存并复制数据
        new_memory_block = F_allocate_aligned_memory(new_total_memory, 16); // 使用默认对齐
        
        if (new_memory_block == NULL)
        {
            new_memory_block = malloc(new_total_memory);
        }
        
        if (new_memory_block != NULL && pool_data->memory_block != NULL)
        {
            // 复制旧数据
            size_t old_total_memory = pool_data->aligned_object_size * pool_data->pool_size;
            memcpy(new_memory_block, pool_data->memory_block, old_total_memory);
            
            // 释放旧内存
            F_free_aligned_memory(pool_data->memory_block);
        }
    }
    
    if (new_memory_block == NULL)
    {
        return false;
    }
    
    // 更新内存块指针
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
    
    // 重新初始化整个空闲链表（因为所有对象都是空闲的）
    pool_data->free_count = new_pool_size;
    for (size_t i = 0; i < new_pool_size; i++)
    {
        pool_data->free_list[i] = (char*)pool_data->memory_block + i * pool_data->aligned_object_size;
    }
    
    // 更新池大小
    pool_data->pool_size = new_pool_size;
    
    return true;
}

#ifdef __cplusplus
}
#endif
