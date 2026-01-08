/**
 * @file CN_context_operations.c
 * @brief 内存上下文操作模块
 * 
 * 实现了内存上下文的各种操作功能，包括上下文切换和层次关系查询。
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
// 上下文操作函数实现
// ============================================================================

/**
 * @brief 获取当前线程的活动上下文
 */
Stru_MemoryContext_t* F_context_get_current(void)
{
    if (g_thread_current_context != NULL)
    {
        return g_thread_current_context;
    }
    
    // 如果没有设置线程本地上下文，返回根上下文
    return g_root_context;
}

/**
 * @brief 设置当前线程的活动上下文
 */
void F_context_set_current(Stru_MemoryContext_t* context)
{
    if (context != NULL && !context->is_valid)
    {
        return; // 无效上下文
    }
    
    g_thread_current_context = context;
}

/**
 * @brief 获取上下文的父上下文
 */
Stru_MemoryContext_t* F_context_get_parent(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return NULL;
    }
    
    return context->parent;
}

/**
 * @brief 获取上下文的第一个子上下文
 */
Stru_MemoryContext_t* F_context_get_first_child(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return NULL;
    }
    
    return context->first_child;
}

/**
 * @brief 获取上下文的下一个兄弟上下文
 */
Stru_MemoryContext_t* F_context_get_next_sibling(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return NULL;
    }
    
    return context->next_sibling;
}

/**
 * @brief 获取上下文名称
 */
const char* F_context_get_name(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return NULL;
    }
    
    return context->name;
}

/**
 * @brief 获取上下文唯一标识符
 */
uint64_t F_context_get_id(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return 0;
    }
    
    return context->id;
}

/**
 * @brief 检查上下文是否有效
 */
bool F_context_is_valid(Stru_MemoryContext_t* context)
{
    if (context == NULL)
    {
        return false;
    }
    
    return context->is_valid;
}

/**
 * @brief 获取上下文创建时间戳
 */
uint64_t F_context_get_creation_time(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return 0;
    }
    
    return context->creation_time;
}

/**
 * @brief 获取上下文深度（距离根上下文的层级数）
 */
size_t F_context_get_depth(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return 0;
    }
    
    return context->depth;
}
