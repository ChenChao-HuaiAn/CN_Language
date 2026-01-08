/**
 * @file CN_context_allocation.c
 * @brief 内存上下文分配模块
 * 
 * 实现了内存上下文的分配记录管理和内存分配功能。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#include "../CN_context_core.h"
#include "../CN_context_struct.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 平台特定的对齐分配
#ifdef _WIN32
#include <malloc.h>
#define ALIGNED_ALLOC(alignment, size) _aligned_malloc(size, alignment)
#define ALIGNED_FREE(ptr) _aligned_free(ptr)
#else
// C11标准aligned_alloc，但需要检查可用性
#define ALIGNED_ALLOC(alignment, size) aligned_alloc(alignment, size)
#define ALIGNED_FREE(ptr) free(ptr)
#endif

// ============================================================================
// 分配记录管理函数
// ============================================================================

/**
 * @brief 创建分配记录
 */
Stru_AllocationRecord_t* F_create_allocation_record(
    void* address, size_t size, size_t alignment, const char* file, int line)
{
    Stru_AllocationRecord_t* record = 
        (Stru_AllocationRecord_t*)malloc(sizeof(Stru_AllocationRecord_t));
    
    if (record == NULL)
    {
        return NULL;
    }
    
    record->address = address;
    record->size = size;
    record->alignment = alignment;
    record->timestamp = (uint64_t)time(NULL) * 1000;
    record->file = file;
    record->line = line;
    record->next = NULL;
    
    return record;
}

/**
 * @brief 释放分配记录
 */
void F_free_allocation_record(Stru_AllocationRecord_t* record)
{
    if (record != NULL)
    {
        free(record);
    }
}

/**
 * @brief 添加分配记录到上下文
 */
void F_add_allocation_to_context(Stru_MemoryContext_t* context,
                                Stru_AllocationRecord_t* record)
{
    if (context == NULL || record == NULL)
    {
        return;
    }
    
    // 添加到链表头部
    record->next = context->allocations;
    context->allocations = record;
    
    // 更新统计信息
    context->total_allocated += record->size;
    context->current_usage += record->size;
    context->allocation_count++;
    
    // 更新峰值使用量
    if (context->current_usage > context->peak_allocated)
    {
        context->peak_allocated = context->current_usage;
    }
}

/**
 * @brief 从上下文中移除分配记录
 */
Stru_AllocationRecord_t* F_remove_allocation_from_context(
    Stru_MemoryContext_t* context, void* address)
{
    if (context == NULL || address == NULL)
    {
        return NULL;
    }
    
    Stru_AllocationRecord_t* prev = NULL;
    Stru_AllocationRecord_t* current = context->allocations;
    
    while (current != NULL)
    {
        if (current->address == address)
        {
            // 从链表中移除
            if (prev == NULL)
            {
                context->allocations = current->next;
            }
            else
            {
                prev->next = current->next;
            }
            
            // 更新统计信息
            context->current_usage -= current->size;
            context->total_allocated -= current->size;
            
            // 断开链表连接
            current->next = NULL;
            return current;
        }
        
        prev = current;
        current = current->next;
    }
    
    return NULL; // 未找到
}

// ============================================================================
// 内存分配函数实现
// ============================================================================

/**
 * @brief 在指定上下文中分配内存
 */
void* F_context_allocate(Stru_MemoryContext_t* context,
                        size_t size, size_t alignment)
{
    // 确定目标上下文
    Stru_MemoryContext_t* target_context = context;
    if (target_context == NULL)
    {
        target_context = F_context_get_current();
    }
    
    if (target_context == NULL || !target_context->is_valid || size == 0)
    {
        return NULL;
    }
    
    // 执行分配
    void* memory = NULL;
    
    if (target_context->allocate_func != NULL)
    {
        memory = target_context->allocate_func(target_context->allocator_data,
                                              size, alignment);
    }
    else
    {
        // 使用系统分配器
        if (alignment == 0 || alignment <= sizeof(void*))
        {
            memory = malloc(size);
        }
        else
        {
            // 对齐分配（使用平台特定的对齐分配函数）
            // 对齐分配要求size是alignment的倍数
            size_t aligned_size = ((size + alignment - 1) / alignment) * alignment;
            memory = ALIGNED_ALLOC(alignment, aligned_size);
        }
    }
    
    if (memory == NULL)
    {
        return NULL;
    }
    
    // 记录分配（如果启用了统计）
    if (target_context->enable_statistics)
    {
        Stru_AllocationRecord_t* record = F_create_allocation_record(
            memory, size, alignment, __FILE__, __LINE__);
        
        if (record != NULL)
        {
            F_add_allocation_to_context(target_context, record);
        }
    }
    
    return memory;
}

/**
 * @brief 在指定上下文中重新分配内存
 */
void* F_context_reallocate(Stru_MemoryContext_t* context,
                          void* ptr, size_t new_size)
{
    // 简化实现：分配新内存，复制数据，释放旧内存
    if (ptr == NULL)
    {
        return F_context_allocate(context, new_size, 0);
    }
    
    if (new_size == 0)
    {
        F_context_deallocate(context, ptr);
        return NULL;
    }
    
    // 查找旧分配记录以获取大小信息
    Stru_MemoryContext_t* target_context = context;
    if (target_context == NULL)
    {
        target_context = F_context_get_current();
    }
    
    size_t old_size = 0;
    if (target_context != NULL && target_context->enable_statistics)
    {
        Stru_AllocationRecord_t* record = target_context->allocations;
        while (record != NULL)
        {
            if (record->address == ptr)
            {
                old_size = record->size;
                break;
            }
            record = record->next;
        }
    }
    
    // 分配新内存
    void* new_memory = F_context_allocate(context, new_size, 0);
    if (new_memory == NULL)
    {
        return NULL;
    }
    
    // 复制数据
    if (old_size > 0)
    {
        size_t copy_size = (old_size < new_size) ? old_size : new_size;
        memcpy(new_memory, ptr, copy_size);
    }
    
    // 释放旧内存
    F_context_deallocate(context, ptr);
    
    return new_memory;
}

/**
 * @brief 释放指定上下文中的内存
 */
void F_context_deallocate(Stru_MemoryContext_t* context, void* ptr)
{
    if (ptr == NULL)
    {
        return;
    }
    
    // 确定目标上下文
    Stru_MemoryContext_t* target_context = context;
    if (target_context == NULL)
    {
        target_context = F_context_get_current();
    }
    
    if (target_context == NULL || !target_context->is_valid)
    {
        return;
    }
    
    // 查找并移除分配记录
    Stru_AllocationRecord_t* record = NULL;
    if (target_context->enable_statistics)
    {
        record = F_remove_allocation_from_context(target_context, ptr);
    }
    
    // 执行释放
    if (target_context->deallocate_func != NULL)
    {
        target_context->deallocate_func(target_context->allocator_data, ptr);
    }
    else
    {
        // 检查是否是对齐分配
        bool is_aligned = false;
        
        if (record != NULL)
        {
            // 如果找到了记录，直接从记录中获取对齐信息
            if (record->alignment > sizeof(void*))
            {
                is_aligned = true;
            }
        }
        else if (target_context->enable_statistics)
        {
            // 如果没有记录但启用了统计，遍历链表查找
            Stru_AllocationRecord_t* temp = target_context->allocations;
            while (temp != NULL)
            {
                if (temp->address == ptr && temp->alignment > sizeof(void*))
                {
                    is_aligned = true;
                    break;
                }
                temp = temp->next;
            }
        }
        
        if (is_aligned)
        {
            // 对于对齐分配的内存，使用ALIGNED_FREE释放
            ALIGNED_FREE(ptr);
        }
        else
        {
            free(ptr);
        }
    }
    
    // 释放分配记录
    if (record != NULL)
    {
        F_free_allocation_record(record);
    }
}
