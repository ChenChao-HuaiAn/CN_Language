/**
 * @file CN_context_statistics.c
 * @brief 内存上下文统计模块
 * 
 * 实现了内存上下文的统计信息查询功能。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#include "CN_context_core.h"
#include "CN_context_struct.h"

// ============================================================================
// 内存管理函数实现
// ============================================================================

/**
 * @brief 获取内存块的实际分配大小
 */
size_t F_context_get_allocated_size(Stru_MemoryContext_t* context,
                                   void* ptr)
{
    if (context == NULL || ptr == NULL)
    {
        return 0;
    }
    
    // 查找分配记录
    Stru_AllocationRecord_t* record = context->allocations;
    while (record != NULL)
    {
        if (record->address == ptr)
        {
            return record->size;
        }
        record = record->next;
    }
    
    return 0; // 未找到
}

/**
 * @brief 获取上下文的总分配内存
 */
size_t F_context_get_total_allocated(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return 0;
    }
    
    return context->total_allocated;
}

/**
 * @brief 获取上下文的峰值分配内存
 */
size_t F_context_get_peak_allocated(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return 0;
    }
    
    return context->peak_allocated;
}

/**
 * @brief 获取上下文的当前使用内存
 */
size_t F_context_get_current_usage(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return 0;
    }
    
    return context->current_usage;
}

/**
 * @brief 获取上下文的分配次数
 */
size_t F_context_get_allocation_count(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return 0;
    }
    
    return context->allocation_count;
}

/**
 * @brief 启用或禁用上下文统计
 */
void F_context_enable_statistics(Stru_MemoryContext_t* context, bool enable)
{
    if (context == NULL || !context->is_valid)
    {
        return;
    }
    
    context->enable_statistics = enable;
}

/**
 * @brief 检查是否启用了上下文统计
 */
bool F_context_is_statistics_enabled(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return false;
    }
    
    return context->enable_statistics;
}

/**
 * @brief 启用或禁用上下文调试
 */
void F_context_enable_debugging(Stru_MemoryContext_t* context, bool enable)
{
    if (context == NULL || !context->is_valid)
    {
        return;
    }
    
    context->enable_debugging = enable;
}

/**
 * @brief 检查是否启用了上下文调试
 */
bool F_context_is_debugging_enabled(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return false;
    }
    
    return context->enable_debugging;
}

/**
 * @brief 获取上下文的引用计数
 */
size_t F_context_get_reference_count(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return 0;
    }
    
    return context->reference_count;
}

/**
 * @brief 增加上下文的引用计数
 */
void F_context_increment_reference(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return;
    }
    
    context->reference_count++;
}

/**
 * @brief 减少上下文的引用计数
 */
void F_context_decrement_reference(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return;
    }
    
    if (context->reference_count > 0)
    {
        context->reference_count--;
    }
}
