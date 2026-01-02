/******************************************************************************
 * 文件名: CN_queue.c
 * 功能: CN_Language队列容器主实现 - 锁管理和通用函数
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 重构文件，只保留锁管理和通用函数
 * 版权: MIT许可证
 * 
 * 注意: 此文件只包含队列的锁管理和通用函数，具体实现已分离到单独文件
 ******************************************************************************/

#include "CN_queue_internal.h"
#include "../../memory/CN_memory.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// 内部辅助函数实现（锁管理）
// ============================================================================

/**
 * @brief 初始化队列锁
 */
bool CN_queue_internal_init_lock(Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return false;
    }
    
    if (queue->thread_safety == Eum_QUEUE_THREAD_SAFE)
    {
#ifdef _WIN32
        InitializeCriticalSection(&queue->lock);
#else
        if (pthread_mutex_init(&queue->lock, NULL) != 0)
        {
            return false;
        }
#endif
        queue->lock_initialized = true;
    }
    else
    {
        queue->lock_initialized = false;
    }
    
    return true;
}

/**
 * @brief 销毁队列锁
 */
void CN_queue_internal_destroy_lock(Stru_CN_Queue_t* queue)
{
    if (queue == NULL || !queue->lock_initialized)
    {
        return;
    }
    
    if (queue->thread_safety == Eum_QUEUE_THREAD_SAFE)
    {
#ifdef _WIN32
        DeleteCriticalSection(&queue->lock);
#else
        pthread_mutex_destroy(&queue->lock);
#endif
    }
    
    queue->lock_initialized = false;
}

/**
 * @brief 锁定队列（线程安全）
 */
void CN_queue_internal_lock(Stru_CN_Queue_t* queue)
{
    if (queue == NULL || !queue->lock_initialized)
    {
        return;
    }
    
    if (queue->thread_safety == Eum_QUEUE_THREAD_SAFE)
    {
#ifdef _WIN32
        EnterCriticalSection(&queue->lock);
#else
        pthread_mutex_lock(&queue->lock);
#endif
    }
}

/**
 * @brief 解锁队列（线程安全）
 */
void CN_queue_internal_unlock(Stru_CN_Queue_t* queue)
{
    if (queue == NULL || !queue->lock_initialized)
    {
        return;
    }
    
    if (queue->thread_safety == Eum_QUEUE_THREAD_SAFE)
    {
#ifdef _WIN32
        LeaveCriticalSection(&queue->lock);
#else
        pthread_mutex_unlock(&queue->lock);
#endif
    }
}

// ============================================================================
// 通用函数实现
// ============================================================================

/**
 * @brief 获取队列元素指针（通用）
 */
void* CN_queue_internal_get_element(const Stru_CN_Queue_t* queue, size_t index)
{
    if (queue == NULL || index >= queue->size)
    {
        return NULL;
    }
    
    switch (queue->implementation)
    {
        case Eum_QUEUE_IMPLEMENTATION_ARRAY:
            return CN_queue_internal_array_get_element(queue, index);
            
        case Eum_QUEUE_IMPLEMENTATION_LIST:
            return CN_queue_internal_list_get_element(queue, index);
            
        case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
            return CN_queue_internal_circular_get_element(queue, index);
            
        default:
            return NULL;
    }
}

/**
 * @brief 批量入队多个元素（通用）
 */
size_t CN_queue_internal_enqueue_batch(Stru_CN_Queue_t* queue, const void* elements, size_t count)
{
    if (queue == NULL || elements == NULL || count == 0)
    {
        return 0;
    }
    
    CN_queue_internal_lock(queue);
    
    size_t enqueued = 0;
    const char* element_ptr = (const char*)elements;
    
    for (size_t i = 0; i < count; i++)
    {
        const void* element = element_ptr + (i * queue->element_size);
        
        bool success;
        switch (queue->implementation)
        {
            case Eum_QUEUE_IMPLEMENTATION_ARRAY:
                success = CN_queue_internal_array_enqueue(queue, element);
                break;
                
            case Eum_QUEUE_IMPLEMENTATION_LIST:
                success = CN_queue_internal_list_enqueue(queue, element);
                break;
                
            case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
                success = CN_queue_internal_circular_enqueue(queue, element);
                break;
                
            default:
                success = false;
                break;
        }
        
        if (!success)
        {
            break;
        }
        
        enqueued++;
    }
    
    CN_queue_internal_unlock(queue);
    return enqueued;
}

/**
 * @brief 批量出队多个元素（通用）
 */
size_t CN_queue_internal_dequeue_batch(Stru_CN_Queue_t* queue, void* elements, size_t max_count)
{
    if (queue == NULL || max_count == 0)
    {
        return 0;
    }
    
    CN_queue_internal_lock(queue);
    
    size_t dequeued = 0;
    char* element_ptr = (char*)elements;
    
    for (size_t i = 0; i < max_count && queue->size > 0; i++)
    {
        void* element = (element_ptr != NULL) ? (element_ptr + (i * queue->element_size)) : NULL;
        
        bool success;
        switch (queue->implementation)
        {
            case Eum_QUEUE_IMPLEMENTATION_ARRAY:
                success = CN_queue_internal_array_dequeue(queue, element);
                break;
                
            case Eum_QUEUE_IMPLEMENTATION_LIST:
                success = CN_queue_internal_list_dequeue(queue, element);
                break;
                
            case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
                success = CN_queue_internal_circular_dequeue(queue, element);
                break;
                
            default:
                success = false;
                break;
        }
        
        if (!success)
        {
            break;
        }
        
        dequeued++;
    }
    
    CN_queue_internal_unlock(queue);
    return dequeued;
}
