/******************************************************************************
 * 文件名: CN_queue_circular.c
 * 功能: CN_Language队列容器 - 循环数组实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现循环数组队列
 * 版权: MIT许可证
 * 
 * 注意: 此文件实现循环数组队列，遵循单一职责原则
 ******************************************************************************/

#include "CN_queue_internal.h"
#include "../../memory/CN_memory.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief 创建循环数组实现的队列
 */
Stru_CN_Queue_t* CN_queue_internal_create_circular(
    size_t element_size,
    size_t initial_capacity,
    Eum_CN_QueueThreadSafety_t thread_safety,
    CN_QueueFreeFunc free_func,
    CN_QueueCopyFunc copy_func)
{
    if (element_size == 0 || initial_capacity == 0)
    {
        return NULL;
    }
    
    Stru_CN_Queue_t* queue = (Stru_CN_Queue_t*)cn_malloc(sizeof(Stru_CN_Queue_t), "CN_queue_circular");
    if (queue == NULL)
    {
        return NULL;
    }
    
    // 初始化通用字段
    queue->implementation = Eum_QUEUE_IMPLEMENTATION_CIRCULAR;
    queue->element_size = element_size;
    queue->size = 0;
    queue->thread_safety = thread_safety;
    queue->free_func = free_func;
    queue->copy_func = copy_func;
    
    // 初始化循环数组特定字段
    queue->impl.circular.capacity = initial_capacity;
    queue->impl.circular.head_index = 0;
    queue->impl.circular.tail_index = 0;
    queue->impl.circular.is_full = false;
    
    // 分配数据数组
    size_t data_size = element_size * initial_capacity;
    queue->impl.circular.data = cn_malloc(data_size, "CN_queue_circular_data");
    if (queue->impl.circular.data == NULL)
    {
        cn_free(queue);
        return NULL;
    }
    
    // 初始化锁
    if (!CN_queue_internal_init_lock(queue))
    {
        cn_free(queue->impl.circular.data);
        cn_free(queue);
        return NULL;
    }
    
    return queue;
}

/**
 * @brief 销毁循环数组实现的队列
 */
void CN_queue_internal_destroy_circular(Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return;
    }
    
    CN_queue_internal_lock(queue);
    
    // 如果有释放函数，释放所有元素
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
    
    // 释放数据数组
    if (queue->impl.circular.data != NULL)
    {
        cn_free(queue->impl.circular.data);
        queue->impl.circular.data = NULL;
    }
    
    CN_queue_internal_unlock(queue);
    CN_queue_internal_destroy_lock(queue);
}

/**
 * @brief 循环数组实现的入队操作
 */
bool CN_queue_internal_circular_enqueue(Stru_CN_Queue_t* queue, const void* element)
{
    if (queue == NULL || element == NULL)
    {
        return false;
    }
    
    // 检查队列是否已满
    if (CN_queue_internal_circular_is_full(queue))
    {
        // 需要扩容
        size_t new_capacity = queue->impl.circular.capacity * 2;
        if (!CN_queue_internal_circular_ensure_capacity(queue, new_capacity))
        {
            return false;
        }
    }
    
    // 复制元素到队尾
    void* dest = (char*)queue->impl.circular.data + (queue->impl.circular.tail_index * queue->element_size);
    memcpy(dest, element, queue->element_size);
    
    // 更新队尾索引
    queue->impl.circular.tail_index++;
    if (queue->impl.circular.tail_index >= queue->impl.circular.capacity)
    {
        queue->impl.circular.tail_index = 0;
    }
    
    // 检查队列是否已满
    if (queue->impl.circular.tail_index == queue->impl.circular.head_index)
    {
        queue->impl.circular.is_full = true;
    }
    
    queue->size++;
    
    return true;
}

/**
 * @brief 循环数组实现的出队操作
 */
bool CN_queue_internal_circular_dequeue(Stru_CN_Queue_t* queue, void* element)
{
    if (queue == NULL || CN_queue_internal_circular_is_empty(queue))
    {
        return false;
    }
    
    // 获取队头元素
    void* src = (char*)queue->impl.circular.data + (queue->impl.circular.head_index * queue->element_size);
    
    // 如果提供了输出参数，复制元素值
    if (element != NULL)
    {
        memcpy(element, src, queue->element_size);
    }
    
    // 如果有释放函数，释放元素
    if (queue->free_func != NULL)
    {
        queue->free_func(src);
    }
    
    // 更新队头索引
    queue->impl.circular.head_index++;
    if (queue->impl.circular.head_index >= queue->impl.circular.capacity)
    {
        queue->impl.circular.head_index = 0;
    }
    
    // 队列不再满
    queue->impl.circular.is_full = false;
    
    queue->size--;
    
    return true;
}

/**
 * @brief 循环数组实现的查看队头操作
 */
void* CN_queue_internal_circular_front(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL || CN_queue_internal_circular_is_empty(queue))
    {
        return NULL;
    }
    
    return (char*)queue->impl.circular.data + (queue->impl.circular.head_index * queue->element_size);
}

/**
 * @brief 循环数组实现的查看队尾操作
 */
void* CN_queue_internal_circular_rear(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL || CN_queue_internal_circular_is_empty(queue))
    {
        return NULL;
    }
    
    // 计算队尾索引（考虑回绕）
    size_t rear_index;
    if (queue->impl.circular.tail_index == 0)
    {
        rear_index = queue->impl.circular.capacity - 1;
    }
    else
    {
        rear_index = queue->impl.circular.tail_index - 1;
    }
    
    return (char*)queue->impl.circular.data + (rear_index * queue->element_size);
}

/**
 * @brief 确保循环数组队列有足够容量
 */
bool CN_queue_internal_circular_ensure_capacity(Stru_CN_Queue_t* queue, size_t min_capacity)
{
    if (queue == NULL || min_capacity <= queue->impl.circular.capacity)
    {
        return true;
    }
    
    // 计算新的容量（至少翻倍）
    size_t new_capacity = queue->impl.circular.capacity;
    while (new_capacity < min_capacity)
    {
        new_capacity *= 2;
    }
    
    // 分配新的数据数组
    size_t new_data_size = queue->element_size * new_capacity;
    void* new_data = cn_malloc(new_data_size, "CN_queue_circular_new_data");
    if (new_data == NULL)
    {
        return false;
    }
    
    // 复制现有元素
    if (queue->size > 0)
    {
        if (!CN_queue_internal_circular_is_full(queue) && 
            queue->impl.circular.head_index < queue->impl.circular.tail_index)
        {
            // 连续存储，一次复制
            void* src = (char*)queue->impl.circular.data + (queue->impl.circular.head_index * queue->element_size);
            memcpy(new_data, src, queue->size * queue->element_size);
        }
        else
        {
            // 回绕存储或队列已满，分两次复制
            size_t first_part = queue->impl.circular.capacity - queue->impl.circular.head_index;
            if (first_part > queue->size)
            {
                first_part = queue->size;
            }
            
            void* src_first = (char*)queue->impl.circular.data + (queue->impl.circular.head_index * queue->element_size);
            memcpy(new_data, src_first, first_part * queue->element_size);
            
            if (first_part < queue->size)
            {
                size_t second_part = queue->size - first_part;
                void* src_second = queue->impl.circular.data;
                void* dst_second = (char*)new_data + (first_part * queue->element_size);
                memcpy(dst_second, src_second, second_part * queue->element_size);
            }
        }
    }
    
    // 释放旧数据
    cn_free(queue->impl.circular.data);
    
    // 更新队列结构
    queue->impl.circular.data = new_data;
    queue->impl.circular.capacity = new_capacity;
    queue->impl.circular.head_index = 0;
    queue->impl.circular.tail_index = queue->size;
    queue->impl.circular.is_full = (queue->size == new_capacity);
    
    return true;
}

/**
 * @brief 复制循环数组队列
 */
Stru_CN_Queue_t* CN_queue_internal_circular_copy(const Stru_CN_Queue_t* src)
{
    if (src == NULL)
    {
        return NULL;
    }
    
    // 创建新队列
    Stru_CN_Queue_t* dst = CN_queue_internal_create_circular(
        src->element_size,
        src->impl.circular.capacity,
        src->thread_safety,
        src->free_func,
        src->copy_func);
    
    if (dst == NULL)
    {
        return NULL;
    }
    
    // 复制元素
    if (src->size > 0)
    {
        // 复制所有元素
        for (size_t i = 0; i < src->size; i++)
        {
            void* src_element = CN_queue_internal_circular_get_element(src, i);
            if (src_element == NULL)
            {
                CN_queue_internal_destroy_circular(dst);
                return NULL;
            }
            
            if (!CN_queue_internal_circular_enqueue(dst, src_element))
            {
                CN_queue_internal_destroy_circular(dst);
                return NULL;
            }
        }
    }
    
    return dst;
}

/**
 * @brief 获取循环数组队列元素指针
 */
void* CN_queue_internal_circular_get_element(const Stru_CN_Queue_t* queue, size_t index)
{
    if (queue == NULL || index >= queue->size)
    {
        return NULL;
    }
    
    // 计算物理索引（考虑回绕）
    size_t physical_index = queue->impl.circular.head_index + index;
    if (physical_index >= queue->impl.circular.capacity)
    {
        physical_index -= queue->impl.circular.capacity;
    }
    
    return (char*)queue->impl.circular.data + (physical_index * queue->element_size);
}

/**
 * @brief 计算循环数组索引（向前移动）
 */
size_t CN_queue_internal_circular_next_index(const Stru_CN_Queue_t* queue, size_t current_index, size_t steps)
{
    if (queue == NULL)
    {
        return 0;
    }
    
    size_t new_index = current_index + steps;
    if (new_index >= queue->impl.circular.capacity)
    {
        new_index -= queue->impl.circular.capacity;
    }
    
    return new_index;
}

/**
 * @brief 计算循环数组索引（向后移动）
 */
size_t CN_queue_internal_circular_prev_index(const Stru_CN_Queue_t* queue, size_t current_index, size_t steps)
{
    if (queue == NULL)
    {
        return 0;
    }
    
    if (steps > current_index)
    {
        steps -= current_index;
        return queue->impl.circular.capacity - steps;
    }
    else
    {
        return current_index - steps;
    }
}

/**
 * @brief 检查循环数组队列是否为空
 */
bool CN_queue_internal_circular_is_empty(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return true;
    }
    
    return (!queue->impl.circular.is_full && 
            queue->impl.circular.head_index == queue->impl.circular.tail_index);
}

/**
 * @brief 检查循环数组队列是否已满
 */
bool CN_queue_internal_circular_is_full(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return false;
    }
    
    return queue->impl.circular.is_full;
}

/**
 * @brief 获取循环数组队列的剩余空间
 */
size_t CN_queue_internal_circular_remaining_space(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return 0;
    }
    
    if (CN_queue_internal_circular_is_full(queue))
    {
        return 0;
    }
    
    if (queue->impl.circular.head_index <= queue->impl.circular.tail_index)
    {
        return queue->impl.circular.capacity - (queue->impl.circular.tail_index - queue->impl.circular.head_index);
    }
    else
    {
        return queue->impl.circular.head_index - queue->impl.circular.tail_index;
    }
}

/**
 * @brief 清空循环数组队列
 */
void CN_queue_internal_circular_clear(Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return;
    }
    
    CN_queue_internal_lock(queue);
    
    // 如果有释放函数，释放所有元素
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
    
    // 重置索引和标志
    queue->impl.circular.head_index = 0;
    queue->impl.circular.tail_index = 0;
    queue->impl.circular.is_full = false;
    queue->size = 0;
    
    CN_queue_internal_unlock(queue);
}
