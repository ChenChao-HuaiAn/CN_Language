/******************************************************************************
 * 文件名: CN_queue_api.c
 * 功能: CN_Language队列容器公共API实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现队列公共API
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_queue.h"
#include "CN_queue_internal.h"
#include "../../memory/CN_memory.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// 队列创建和销毁
// ============================================================================

/**
 * @brief 创建队列
 */
Stru_CN_Queue_t* CN_queue_create(Eum_CN_QueueImplementation_t implementation,
                                 size_t element_size, 
                                 size_t initial_capacity,
                                 Eum_CN_QueueThreadSafety_t thread_safety)
{
    return CN_queue_create_custom(implementation, element_size, initial_capacity,
                                  thread_safety, NULL, NULL);
}

/**
 * @brief 创建带自定义函数的队列
 */
Stru_CN_Queue_t* CN_queue_create_custom(
    Eum_CN_QueueImplementation_t implementation,
    size_t element_size,
    size_t initial_capacity,
    Eum_CN_QueueThreadSafety_t thread_safety,
    CN_QueueFreeFunc free_func,
    CN_QueueCopyFunc copy_func)
{
    if (element_size == 0)
    {
        return NULL;
    }
    
    // 根据实现类型创建队列
    switch (implementation)
    {
        case Eum_QUEUE_IMPLEMENTATION_ARRAY:
            if (initial_capacity == 0)
            {
                initial_capacity = 16; // 默认初始容量
            }
            return CN_queue_internal_create_array(element_size, initial_capacity,
                                                  thread_safety, free_func, copy_func);
            
        case Eum_QUEUE_IMPLEMENTATION_LIST:
            return CN_queue_internal_create_list(element_size, thread_safety,
                                                 free_func, copy_func);
            
        case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
            if (initial_capacity == 0)
            {
                initial_capacity = 16; // 默认初始容量
            }
            return CN_queue_internal_create_circular(element_size, initial_capacity,
                                                     thread_safety, free_func, copy_func);
            
        default:
            return NULL;
    }
}

/**
 * @brief 销毁队列
 */
void CN_queue_destroy(Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return;
    }
    
    // 根据实现类型销毁队列
    switch (queue->implementation)
    {
        case Eum_QUEUE_IMPLEMENTATION_ARRAY:
            CN_queue_internal_destroy_array(queue);
            break;
            
        case Eum_QUEUE_IMPLEMENTATION_LIST:
            CN_queue_internal_destroy_list(queue);
            break;
            
        case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
            CN_queue_internal_destroy_circular(queue);
            break;
            
        default:
            break;
    }
    
    // 释放队列结构本身
    cn_free(queue);
}

/**
 * @brief 清空队列（移除所有元素）
 */
void CN_queue_clear(Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return;
    }
    
    CN_queue_internal_lock(queue);
    
    // 根据实现类型清空队列
    switch (queue->implementation)
    {
        case Eum_QUEUE_IMPLEMENTATION_ARRAY:
            // 对于数组实现，只需重置大小和索引
            if (queue->free_func != NULL)
            {
                for (size_t i = 0; i < queue->size; i++)
                {
                    void* element = CN_queue_internal_array_get_element(queue, i);
                    if (element != NULL)
                    {
                        queue->free_func(element);
                    }
                }
            }
            queue->impl.array.head_index = 0;
            queue->impl.array.tail_index = 0;
            break;
            
        case Eum_QUEUE_IMPLEMENTATION_LIST:
            // 对于链表实现，需要释放所有节点
            CN_queue_internal_destroy_list(queue);
            // 重新初始化链表
            queue->impl.list.head = NULL;
            queue->impl.list.tail = NULL;
            break;
            
        case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
            // 对于循环数组实现，重置索引和标志
            if (queue->free_func != NULL)
            {
                for (size_t i = 0; i < queue->size; i++)
                {
                    void* element = CN_queue_internal_circular_get_element(queue, i);
                    if (element != NULL)
                    {
                        queue->free_func(element);
                    }
                }
            }
            queue->impl.circular.head_index = 0;
            queue->impl.circular.tail_index = 0;
            queue->impl.circular.is_full = false;
            break;
            
        default:
            break;
    }
    
    queue->size = 0;
    CN_queue_internal_unlock(queue);
}

// ============================================================================
// 队列属性查询
// ============================================================================

/**
 * @brief 获取队列大小（元素数量）
 */
size_t CN_queue_size(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return 0;
    }
    
    return queue->size;
}

/**
 * @brief 检查队列是否为空
 */
bool CN_queue_is_empty(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return true;
    }
    
    return queue->size == 0;
}

/**
 * @brief 检查队列是否已满（仅对固定容量队列有效）
 */
bool CN_queue_is_full(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return false;
    }
    
    switch (queue->implementation)
    {
        case Eum_QUEUE_IMPLEMENTATION_ARRAY:
            return queue->size >= queue->impl.array.capacity;
            
        case Eum_QUEUE_IMPLEMENTATION_LIST:
            return false; // 链表实现没有固定容量限制
            
        case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
            return CN_queue_internal_circular_is_full(queue);
            
        default:
            return false;
    }
}

/**
 * @brief 获取队列容量（仅对数组实现有效）
 */
size_t CN_queue_capacity(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return 0;
    }
    
    switch (queue->implementation)
    {
        case Eum_QUEUE_IMPLEMENTATION_ARRAY:
            return queue->impl.array.capacity;
            
        case Eum_QUEUE_IMPLEMENTATION_LIST:
            return SIZE_MAX; // 链表实现理论上无限容量
            
        case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
            return queue->impl.circular.capacity;
            
        default:
            return 0;
    }
}

/**
 * @brief 获取元素大小
 */
size_t CN_queue_element_size(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return 0;
    }
    
    return queue->element_size;
}

/**
 * @brief 获取队列实现类型
 */
Eum_CN_QueueImplementation_t CN_queue_implementation(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return Eum_QUEUE_IMPLEMENTATION_ARRAY; // 默认值
    }
    
    return queue->implementation;
}

// ============================================================================
// 队列操作（核心功能）
// ============================================================================

/**
 * @brief 入队（在队尾添加元素）
 */
bool CN_queue_enqueue(Stru_CN_Queue_t* queue, const void* element)
{
    if (queue == NULL || element == NULL)
    {
        return false;
    }
    
    CN_queue_internal_lock(queue);
    
    bool result;
    switch (queue->implementation)
    {
        case Eum_QUEUE_IMPLEMENTATION_ARRAY:
            result = CN_queue_internal_array_enqueue(queue, element);
            break;
            
        case Eum_QUEUE_IMPLEMENTATION_LIST:
            result = CN_queue_internal_list_enqueue(queue, element);
            break;
            
        case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
            result = CN_queue_internal_circular_enqueue(queue, element);
            break;
            
        default:
            result = false;
            break;
    }
    
    CN_queue_internal_unlock(queue);
    return result;
}

/**
 * @brief 出队（移除并返回队头元素）
 */
bool CN_queue_dequeue(Stru_CN_Queue_t* queue, void* element)
{
    if (queue == NULL)
    {
        return false;
    }
    
    CN_queue_internal_lock(queue);
    
    bool result;
    switch (queue->implementation)
    {
        case Eum_QUEUE_IMPLEMENTATION_ARRAY:
            result = CN_queue_internal_array_dequeue(queue, element);
            break;
            
        case Eum_QUEUE_IMPLEMENTATION_LIST:
            result = CN_queue_internal_list_dequeue(queue, element);
            break;
            
        case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
            result = CN_queue_internal_circular_dequeue(queue, element);
            break;
            
        default:
            result = false;
            break;
    }
    
    CN_queue_internal_unlock(queue);
    return result;
}

/**
 * @brief 查看队头元素（不移除）
 */
void* CN_queue_front(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return NULL;
    }
    
    // 注意：这里不锁定，因为front是只读操作
    // 对于线程安全队列，调用者应该使用CN_queue_front_safe
    
    switch (queue->implementation)
    {
        case Eum_QUEUE_IMPLEMENTATION_ARRAY:
            return CN_queue_internal_array_front(queue);
            
        case Eum_QUEUE_IMPLEMENTATION_LIST:
            return CN_queue_internal_list_front(queue);
            
        case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
            return CN_queue_internal_circular_front(queue);
            
        default:
            return NULL;
    }
}

/**
 * @brief 查看队尾元素（不移除）
 */
void* CN_queue_rear(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return NULL;
    }
    
    // 注意：这里不锁定，因为rear是只读操作
    // 对于线程安全队列，调用者应该使用线程安全版本
    
    switch (queue->implementation)
    {
        case Eum_QUEUE_IMPLEMENTATION_ARRAY:
            return CN_queue_internal_array_rear(queue);
            
        case Eum_QUEUE_IMPLEMENTATION_LIST:
            return CN_queue_internal_list_rear(queue);
            
        case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
            return CN_queue_internal_circular_rear(queue);
            
        default:
            return NULL;
    }
}

/**
 * @brief 查看队列中指定位置的元素
 */
void* CN_queue_get(const Stru_CN_Queue_t* queue, size_t index)
{
    if (queue == NULL || index >= queue->size)
    {
        return NULL;
    }
    
    return CN_queue_internal_get_element(queue, index);
}

// ============================================================================
// 队列操作（高级功能）
// ============================================================================

/**
 * @brief 确保队列有足够容量（仅对数组实现有效）
 */
bool CN_queue_ensure_capacity(Stru_CN_Queue_t* queue, size_t min_capacity)
{
    if (queue == NULL)
    {
        return false;
    }
    
    CN_queue_internal_lock(queue);
    
    bool result;
    switch (queue->implementation)
    {
        case Eum_QUEUE_IMPLEMENTATION_ARRAY:
            result = CN_queue_internal_array_ensure_capacity(queue, min_capacity);
            break;
            
        case Eum_QUEUE_IMPLEMENTATION_LIST:
            result = true; // 链表实现不需要容量管理
            break;
            
        case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
            result = CN_queue_internal_circular_ensure_capacity(queue, min_capacity);
            break;
            
        default:
            result = false;
            break;
    }
    
    CN_queue_internal_unlock(queue);
    return result;
}

/**
 * @brief 缩小队列容量以匹配大小（仅对数组实现有效）
 */
bool CN_queue_shrink_to_fit(Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return false;
    }
    
    CN_queue_internal_lock(queue);
    
    bool result = false;
    
    switch (queue->implementation)
    {
        case Eum_QUEUE_IMPLEMENTATION_ARRAY:
            if (queue->size < queue->impl.array.capacity)
            {
                result = CN_queue_internal_array_ensure_capacity(queue, queue->size);
            }
            else
            {
                result = true; // 已经匹配
            }
            break;
            
        case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
            if (queue->size < queue->impl.circular.capacity)
            {
                result = CN_queue_internal_circular_ensure_capacity(queue, queue->size);
            }
            else
            {
                result = true; // 已经匹配
            }
            break;
            
        default:
            result = true; // 链表实现不需要此操作
            break;
    }
    
    CN_queue_internal_unlock(queue);
    return result;
}

/**
 * @brief 复制队列
 */
Stru_CN_Queue_t* CN_queue_copy(const Stru_CN_Queue_t* src)
{
    if (src == NULL)
    {
        return NULL;
    }
    
    // 根据实现类型复制队列
    switch (src->implementation)
    {
        case Eum_QUEUE_IMPLEMENTATION_ARRAY:
            return CN_queue_internal_array_copy(src);
            
        case Eum_QUEUE_IMPLEMENTATION_LIST:
            return CN_queue_internal_list_copy(src);
            
        case Eum_QUEUE_IMPLEMENTATION_CIRCULAR:
            return CN_queue_internal_circular_copy(src);
            
        default:
            return NULL;
    }
}

/**
 * @brief 反转队列中元素的顺序
 */
bool CN_queue_reverse(Stru_CN_Queue_t* queue)
{
    if (queue == NULL || queue->size <= 1)
    {
        return true; // 空队列或单元素队列已经反转
    }
    
    CN_queue_internal_lock(queue);
    
    bool result = false;
    
    // 临时数组用于存储元素
    void* temp_array = cn_malloc(queue->element_size * queue->size, "CN_queue_reverse_temp");
    if (temp_array == NULL)
    {
        CN_queue_internal_unlock(queue);
        return false;
    }
    
    // 将队列元素复制到临时数组（从队头到队尾）
    for (size_t i = 0; i < queue->size; i++)
    {
        void* src = CN_queue_get(queue, i);
        if (src == NULL)
        {
            cn_free(temp_array);
            CN_queue_internal_unlock(queue);
            return false;
        }
        
        void* dest = (char*)temp_array + (i * queue->element_size);
        memcpy(dest, src, queue->element_size);
    }
    
    // 清空队列
    CN_queue_clear(queue);
    
    // 将元素按相反顺序入队
    for (size_t i = queue->size; i > 0; i--)
    {
        void* src = (char*)temp_array + ((i - 1) * queue->element_size);
        if (!CN_queue_enqueue(queue, src))
        {
            cn_free(temp_array);
            CN_queue_internal_unlock(queue);
            return false;
        }
    }
    
    cn_free(temp_array);
    CN_queue_internal_unlock(queue);
    return true;
}

// ============================================================================
// 循环队列特定操作
// ============================================================================

/**
 * @brief 检查是否为循环队列
 */
bool CN_queue_is_circular(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return false;
    }
    
    return queue->implementation == Eum_QUEUE_IMPLEMENTATION_CIRCULAR;
}

/**
 * @brief 获取循环队列的头部索引（仅对循环队列有效）
 */
size_t CN_queue_head_index(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL || queue->implementation != Eum_QUEUE_IMPLEMENTATION_CIRCULAR)
    {
        return 0;
    }
    
    return queue->impl.circular.head_index;
}

/**
 * @brief 获取循环队列的尾部索引（仅对循环队列有效）
 */
size_t CN_queue_tail_index(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL || queue->implementation != Eum_QUEUE_IMPLEMENTATION_CIRCULAR)
    {
        return 0;
    }
    
    return queue->impl.circular.tail_index;
}

/**
 * @brief 循环队列的剩余空间（仅对循环队列有效）
 */
size_t CN_queue_remaining_space(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL || queue->implementation != Eum_QUEUE_IMPLEMENTATION_CIRCULAR)
    {
        return 0;
    }
    
    return CN_queue_internal_circular_remaining_space(queue);
}

// ============================================================================
// 线程安全操作（仅对线程安全队列有效）
// ============================================================================

/**
 * @brief 线程安全地入队
 */
bool CN_queue_enqueue_safe(Stru_CN_Queue_t* queue, const void* element)
{
    // 对于线程安全队列，CN_queue_enqueue已经线程安全
    return CN_queue_enqueue(queue, element);
}

/**
 * @brief 线程安全地出队
 */
bool CN_queue_dequeue_safe(Stru_CN_Queue_t* queue, void* element)
{
    // 对于线程安全队列，CN_queue_dequeue已经线程安全
    return CN_queue_dequeue(queue, element);
}

/**
 * @brief 线程安全地查看队头元素
 */
void* CN_queue_front_safe(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return NULL;
    }
    
    // 锁定队列以进行线程安全的只读访问
    CN_queue_internal_lock((Stru_CN_Queue_t*)queue);
    void* result = CN_queue_front(queue);
    CN_queue_internal_unlock((Stru_CN_Queue_t*)queue);
    
    return result;
}

/**
 * @brief 获取队列的互斥锁（用于手动同步）
 */
bool CN_queue_lock(Stru_CN_Queue_t* queue)
{
    if (queue == NULL || queue->thread_safety != Eum_QUEUE_THREAD_SAFE)
    {
        return false;
    }
    
    CN_queue_internal_lock(queue);
    return true;
}

/**
 * @brief 释放队列的互斥锁
 */
void CN_queue_unlock(Stru_CN_Queue_t* queue)
{
    if (queue == NULL || queue->thread_safety != Eum_QUEUE_THREAD_SAFE)
    {
        return;
    }
    
    CN_queue_internal_unlock(queue);
}

// ============================================================================
// 迭代器支持
// ============================================================================

/**
 * @brief 创建队列迭代器（从队头开始）
 */
Stru_CN_QueueIterator_t* CN_queue_iterator_create(Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return NULL;
    }
    
    Stru_CN_QueueIterator_t* iterator = (Stru_CN_QueueIterator_t*)cn_malloc(
        sizeof(Stru_CN_QueueIterator_t), "CN_queue_iterator");
    if (iterator == NULL)
    {
        return NULL;
    }
    
    iterator->queue = queue;
    iterator->current_index = 0; // 从队头开始
    iterator->current_element = CN_queue_get(queue, 0);
    
    return iterator;
}

/**
 * @brief 销毁迭代器
 */
void CN_queue_iterator_destroy(Stru_CN_QueueIterator_t* iterator)
{
    if (iterator == NULL)
    {
        return;
    }
    
    cn_free(iterator);
}

/**
 * @brief 重置迭代器到队头
 */
void CN_queue_iterator_reset(Stru_CN_QueueIterator_t* iterator)
{
    if (iterator == NULL || iterator->queue == NULL)
    {
        return;
    }
    
    iterator->current_index = 0;
    iterator->current_element = CN_queue_get(iterator->queue, 0);
}

/**
 * @brief 检查迭代器是否有下一个元素（向队尾方向）
 */
bool CN_queue_iterator_has_next(const Stru_CN_QueueIterator_t* iterator)
{
    if (iterator == NULL || iterator->queue == NULL)
    {
        return false;
    }
    
    return iterator->current_index < CN_queue_size(iterator->queue);
}

/**
 * @brief 获取下一个元素（向队尾方向）
 */
void* CN_queue_iterator_next(Stru_CN_QueueIterator_t* iterator)
{
    if (iterator == NULL || iterator->queue == NULL)
    {
        return NULL;
    }
    
    if (!CN_queue_iterator_has_next(iterator))
    {
        return NULL;
    }
    
    // 获取当前元素
    void* current = iterator->current_element;
    
    // 移动到下一个元素
    iterator->current_index++;
    iterator->current_element = CN_queue_get(iterator->queue, iterator->current_index);
    
    return current;
}

/**
 * @brief 获取当前元素
 */
void* CN_queue_iterator_current(const Stru_CN_QueueIterator_t* iterator)
{
    if (iterator == NULL)
    {
        return NULL;
    }
    
    return iterator->current_element;
}

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 比较两个队列是否相等
 */
bool CN_queue_equal(const Stru_CN_Queue_t* queue1, const Stru_CN_Queue_t* queue2)
{
    if (queue1 == NULL && queue2 == NULL)
    {
        return true;
    }
    
    if (queue1 == NULL || queue2 == NULL)
    {
        return false;
    }
    
    // 检查基本属性
    if (queue1->element_size != queue2->element_size ||
        queue1->size != queue2->size)
    {
        return false;
    }
    
    // 如果两个队列都为空，则相等
    if (queue1->size == 0)
    {
        return true;
    }
    
    // 比较每个元素
    for (size_t i = 0; i < queue1->size; i++)
    {
        void* elem1 = CN_queue_get(queue1, i);
        void* elem2 = CN_queue_get(queue2, i);
        
        if (elem1 == NULL || elem2 == NULL)
        {
            return false;
        }
        
        // 如果有自定义比较函数，使用它
        if (queue1->copy_func != NULL)
        {
            // 使用自定义比较逻辑（这里简化处理，实际可能需要更复杂的比较）
            // 注意：这里假设copy_func可以用于比较，实际可能需要专门的比较函数
            // 为了简化，我们使用memcmp
            if (memcmp(elem1, elem2, queue1->element_size) != 0)
            {
                return false;
            }
        }
        else
        {
            // 使用内存比较
            if (memcmp(elem1, elem2, queue1->element_size) != 0)
            {
                return false;
            }
        }
    }
    
    return true;
}

/**
 * @brief 转储队列信息到控制台（调试用）
 */
void CN_queue_dump(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        printf("Queue: NULL\n");
        return;
    }
    
    // 打印队列基本信息
    printf("Queue [size=%zu, capacity=%zu, element_size=%zu, implementation=%d, thread_safe=%d]:\n",
           queue->size, CN_queue_capacity(queue), queue->element_size,
           queue->implementation, queue->thread_safety);
    
    // 打印元素信息（从队头到队尾）
    for (size_t i = 0; i < queue->size; i++)
    {
        void* element = CN_queue_get(queue, i);
        if (element == NULL)
        {
            printf("  [%zu]: NULL\n", i);
        }
        else
        {
            // 简化表示：只显示内存地址
            printf("  [%zu]: 0x%p\n", i, element);
        }
    }
}

/**
 * @brief 将队列内容转换为数组（从队头到队尾）
 */
size_t CN_queue_to_array(const Stru_CN_Queue_t* queue, void* array, size_t max_elements)
{
    if (queue == NULL || array == NULL || max_elements == 0)
    {
        return 0;
    }
    
    size_t elements_to_copy = queue->size;
    if (elements_to_copy > max_elements)
    {
        elements_to_copy = max_elements;
    }
    
    // 复制元素（从队头到队尾）
    for (size_t i = 0; i < elements_to_copy; i++)
    {
        void* src = CN_queue_get(queue, i);
        if (src == NULL)
        {
            return i; // 返回已复制的元素数量
        }
        
        void* dest = (char*)array + (i * queue->element_size);
        memcpy(dest, src, queue->element_size);
    }
    
    return elements_to_copy;
}

/**
 * @brief 批量入队多个元素
 */
size_t CN_queue_enqueue_batch(Stru_CN_Queue_t* queue, const void* elements, size_t count)
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
        if (!CN_queue_enqueue(queue, element_ptr + (i * queue->element_size)))
        {
            break;
        }
        enqueued++;
    }
    
    CN_queue_internal_unlock(queue);
    return enqueued;
}

/**
 * @brief 批量出队多个元素
 */
size_t CN_queue_dequeue_batch(Stru_CN_Queue_t* queue, void* elements, size_t max_count)
{
    if (queue == NULL || max_count == 0)
    {
        return 0;
    }
    
    CN_queue_internal_lock(queue);
    
    size_t dequeued = 0;
    char* element_ptr = (char*)elements;
    
    for (size_t i = 0; i < max_count; i++)
    {
        if (elements != NULL)
        {
            if (!CN_queue_dequeue(queue, element_ptr + (i * queue->element_size)))
            {
                break;
            }
        }
        else
        {
            // 如果elements为NULL，只移除元素不返回
            if (!CN_queue_dequeue(queue, NULL))
            {
                break;
            }
        }
        dequeued++;
    }
    
    CN_queue_internal_unlock(queue);
    return dequeued;
}
