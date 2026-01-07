/**
 * @file CN_memory_context.c
 * @brief 内存上下文管理实现
 * 
 * 实现了内存上下文管理功能，简化多个分配器的使用。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_memory_context.h"
#include "../allocators/system/CN_system_allocator.h"
#include "../allocators/debug/CN_debug_allocator.h"
#include <stdlib.h>

/**
 * @brief 创建完整的内存管理上下文
 */
Stru_MemoryContext_t* F_create_memory_context(bool use_debug_allocator)
{
    Stru_MemoryContext_t* context = 
        (Stru_MemoryContext_t*)malloc(sizeof(Stru_MemoryContext_t));
    
    if (context == NULL)
    {
        return NULL;
    }
    
    if (use_debug_allocator)
    {
        context->allocator = F_create_debug_allocator(NULL);
    }
    else
    {
        context->allocator = F_create_system_allocator();
    }
    
    context->debugger = NULL;
    context->private_data = NULL;
    
    return context;
}

/**
 * @brief 创建自定义内存管理上下文
 */
Stru_MemoryContext_t* F_create_custom_memory_context(
    Stru_MemoryAllocatorInterface_t* allocator,
    Stru_MemoryDebugInterface_t* debugger)
{
    Stru_MemoryContext_t* context = 
        (Stru_MemoryContext_t*)malloc(sizeof(Stru_MemoryContext_t));
    
    if (context == NULL)
    {
        return NULL;
    }
    
    if (allocator == NULL)
    {
        context->allocator = F_create_system_allocator();
    }
    else
    {
        context->allocator = allocator;
    }
    
    context->debugger = debugger;
    context->private_data = NULL;
    
    return context;
}

/**
 * @brief 销毁内存管理上下文
 */
void F_destroy_memory_context(Stru_MemoryContext_t* context)
{
    if (context != NULL)
    {
        if (context->allocator != NULL)
        {
            context->allocator->cleanup(context->allocator);
        }
        
        free(context);
    }
}

/**
 * @brief 从上下文获取分配器
 */
Stru_MemoryAllocatorInterface_t* F_get_context_allocator(
    Stru_MemoryContext_t* context)
{
    if (context == NULL || context->allocator == NULL)
    {
        return F_create_system_allocator();
    }
    
    return context->allocator;
}

/**
 * @brief 从上下文获取调试器
 */
Stru_MemoryDebugInterface_t* F_get_context_debugger(
    Stru_MemoryContext_t* context)
{
    if (context == NULL)
    {
        return NULL;
    }
    
    return context->debugger;
}

/**
 * @brief 设置上下文私有数据
 */
void F_set_context_private_data(Stru_MemoryContext_t* context,
                               void* private_data)
{
    if (context != NULL)
    {
        context->private_data = private_data;
    }
}

/**
 * @brief 获取上下文私有数据
 */
void* F_get_context_private_data(Stru_MemoryContext_t* context)
{
    if (context == NULL)
    {
        return NULL;
    }
    
    return context->private_data;
}
