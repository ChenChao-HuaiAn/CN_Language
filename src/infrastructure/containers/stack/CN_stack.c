/******************************************************************************
 * 文件名: CN_stack.c
 * 功能: CN_Language栈容器主实现 - 锁管理和通用函数
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 重构文件，只保留锁管理和通用函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_stack_internal.h"
#include "../../memory/CN_memory.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// 内部辅助函数实现（锁管理）
// ============================================================================

/**
 * @brief 初始化栈锁
 */
bool CN_stack_internal_init_lock(Stru_CN_Stack_t* stack)
{
    if (stack == NULL)
    {
        return false;
    }
    
    if (stack->thread_safety == Eum_STACK_THREAD_SAFE)
    {
#ifdef _WIN32
        InitializeCriticalSection(&stack->lock);
#else
        if (pthread_mutex_init(&stack->lock, NULL) != 0)
        {
            return false;
        }
#endif
        stack->lock_initialized = true;
    }
    else
    {
        stack->lock_initialized = false;
    }
    
    return true;
}

/**
 * @brief 销毁栈锁
 */
void CN_stack_internal_destroy_lock(Stru_CN_Stack_t* stack)
{
    if (stack == NULL || !stack->lock_initialized)
    {
        return;
    }
    
    if (stack->thread_safety == Eum_STACK_THREAD_SAFE)
    {
#ifdef _WIN32
        DeleteCriticalSection(&stack->lock);
#else
        pthread_mutex_destroy(&stack->lock);
#endif
    }
    
    stack->lock_initialized = false;
}

/**
 * @brief 锁定栈（线程安全）
 */
void CN_stack_internal_lock(Stru_CN_Stack_t* stack)
{
    if (stack == NULL || !stack->lock_initialized)
    {
        return;
    }
    
    if (stack->thread_safety == Eum_STACK_THREAD_SAFE)
    {
#ifdef _WIN32
        EnterCriticalSection(&stack->lock);
#else
        pthread_mutex_lock(&stack->lock);
#endif
    }
}

/**
 * @brief 解锁栈（线程安全）
 */
void CN_stack_internal_unlock(Stru_CN_Stack_t* stack)
{
    if (stack == NULL || !stack->lock_initialized)
    {
        return;
    }
    
    if (stack->thread_safety == Eum_STACK_THREAD_SAFE)
    {
#ifdef _WIN32
        LeaveCriticalSection(&stack->lock);
#else
        pthread_mutex_unlock(&stack->lock);
#endif
    }
}

// ============================================================================
// 通用辅助函数实现
// ============================================================================

/**
 * @brief 获取栈元素指针（通用）
 */
void* CN_stack_internal_get_element(const Stru_CN_Stack_t* stack, size_t index)
{
    if (stack == NULL || index >= stack->size)
    {
        return NULL;
    }
    
    // 根据实现类型获取元素
    switch (stack->implementation)
    {
        case Eum_STACK_IMPLEMENTATION_ARRAY:
            return CN_stack_internal_array_get_element(stack, index);
            
        case Eum_STACK_IMPLEMENTATION_LIST:
            return CN_stack_internal_list_get_element(stack, index);
            
        case Eum_STACK_IMPLEMENTATION_CIRCULAR:
            return CN_stack_internal_circular_get_element(stack, index);
            
        default:
            return NULL;
    }
}

/**
 * @brief 计算循环数组索引
 */
size_t CN_stack_internal_circular_compute_index(const Stru_CN_Stack_t* stack, size_t logical_index)
{
    if (stack == NULL || stack->implementation != Eum_STACK_IMPLEMENTATION_CIRCULAR)
    {
        return 0;
    }
    
    // 计算物理索引（考虑回绕）
    size_t physical_index = stack->impl.circular.base_index + logical_index;
    if (physical_index >= stack->impl.circular.capacity)
    {
        physical_index -= stack->impl.circular.capacity;
    }
    
    return physical_index;
}
