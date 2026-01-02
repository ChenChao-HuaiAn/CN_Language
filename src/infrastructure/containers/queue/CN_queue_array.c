/******************************************************************************
 * 文件名: CN_queue_array.c
 * 功能: CN_Language队列容器 - 数组实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现数组队列
 * 版权: MIT许可证
 * 
 * 注意: 此文件实现数组队列，遵循单一职责原则
 ******************************************************************************/

#include "CN_queue_internal.h"
#include "../../memory/CN_memory.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief 创建数组实现的队列
 */
Stru_CN_Queue_t* CN_queue_internal_create_array(
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
    
    Stru_CN_Queue_t* queue = (Stru_CN_Queue_t*)cn_malloc(sizeof(Stru_CN_Queue_t), "CN_queue_array");
    if (queue == NULL)
    {
        return NULL;
    }
    
    // 初始化通用字段
    queue->implementation = Eum_QUEUE_IMPLEMENTATION_ARRAY;
    queue->element_size = element_size;
    queue->size = 0;
    queue->thread_safety = thread_safety;
    queue->free_func = free_func;
    queue->copy_func = copy_func;
    
    // 初始化数组特定字段
    queue->impl.array.capacity = initial_capacity;
    queue->impl.array.head_index = 0;
    queue->impl.array.tail_index = 0;
    
    // 分配数据数组
    size_t data_size = element_size * initial_capacity;
    queue->impl.array.data = cn_malloc(data_size, "CN_queue_array_data");
    if (queue->impl.array.data == NULL)
    {
        cn_free(queue);
        return NULL;
    }
    
    // 初始化锁
    if (!CN_queue_internal_init_lock(queue))
    {
        cn_free(queue->impl.array.data);
        cn_free(queue);
        return NULL;
    }
    
    return queue;
}

/**
 * @brief 销毁数组实现的队列
 */
void CN_queue_internal_destroy_array(Stru_CN_Queue_t* queue)
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
            void* element = CN_queue_internal_array_get_element(queue, i);
            if (element != NULL)
            {
                queue->free_func(element);
            }
        }
    }
    
    // 释放数据数组
    if (queue->impl.array.data != NULL)
    {
        cn_free(queue->impl.array.data);
        queue->impl.array.data = NULL;
    }
    
    CN_queue_internal_unlock(queue);
    CN_queue_internal_destroy_lock(queue);
}

/**
 * @brief 数组实现的入队操作
 */
bool CN_queue_internal_array_enqueue(Stru_CN_Queue_t* queue, const void* element)
{
    if (queue == NULL || element == NULL)
    {
        return false;
    }
    
    // 检查是否需要扩容
    if (queue->size >= queue->impl.array.capacity)
    {
        size_t new_capacity = queue->impl.array.capacity * 2;
        if (!CN_queue_internal_array_ensure_capacity(queue, new_capacity))
        {
            return false;
        }
    }
    
    // 复制元素到队尾
    void* dest = (char*)queue->impl.array.data + (queue->impl.array.tail_index * queue->element_size);
    memcpy(dest, element, queue->element_size);
    
    // 更新队尾索引
    queue->impl.array.tail_index++;
    if (queue->impl.array.tail_index >= queue->impl.array.capacity)
    {
        // 如果队尾到达数组末尾，需要移动元素或扩容
        // 这里我们选择移动元素以保持连续性
        if (queue->impl.array.head_index > 0)
        {
            // 移动元素到数组开头
            size_t move_count = queue->size;
            if (move_count > 0)
            {
                void* src = (char*)queue->impl.array.data + (queue->impl.array.head_index * queue->element_size);
                void* dst = queue->impl.array.data;
                memmove(dst, src, move_count * queue->element_size);
            }
            queue->impl.array.tail_index = move_count;
            queue->impl.array.head_index = 0;
        }
        else
        {
            // 需要扩容
            size_t new_capacity = queue->impl.array.capacity * 2;
            if (!CN_queue_internal_array_ensure_capacity(queue, new_capacity))
            {
                return false;
            }
        }
    }
    
    queue->size++;
    
    return true;
}

/**
 * @brief 数组实现的出队操作
 */
bool CN_queue_internal_array_dequeue(Stru_CN_Queue_t* queue, void* element)
{
    if (queue == NULL || queue->size == 0)
    {
        return false;
    }
    
    // 获取队头元素
    void* src = (char*)queue->impl.array.data + (queue->impl.array.head_index * queue->element_size);
    
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
    queue->impl.array.head_index++;
    if (queue->impl.array.head_index >= queue->impl.array.capacity)
    {
        queue->impl.array.head_index = 0;
    }
    
    queue->size--;
    
    return true;
}

/**
 * @brief 数组实现的查看队头操作
 */
void* CN_queue_internal_array_front(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL || queue->size == 0)
    {
        return NULL;
    }
    
    return (char*)queue->impl.array.data + (queue->impl.array.head_index * queue->element_size);
}

/**
 * @brief 数组实现的查看队尾操作
 */
void* CN_queue_internal_array_rear(const Stru_CN_Queue_t* queue)
{
    if (queue == NULL || queue->size == 0)
    {
        return NULL;
    }
    
    // 计算队尾索引（考虑回绕）
    size_t rear_index;
    if (queue->impl.array.tail_index == 0)
    {
        rear_index = queue->impl.array.capacity - 1;
    }
    else
    {
        rear_index = queue->impl.array.tail_index - 1;
    }
    
    return (char*)queue->impl.array.data + (rear_index * queue->element_size);
}

/**
 * @brief 确保数组队列有足够容量
 */
bool CN_queue_internal_array_ensure_capacity(Stru_CN_Queue_t* queue, size_t min_capacity)
{
    if (queue == NULL || min_capacity <= queue->impl.array.capacity)
    {
        return true;
    }
    
    // 计算新的容量（至少翻倍）
    size_t new_capacity = queue->impl.array.capacity;
    while (new_capacity < min_capacity)
    {
        new_capacity *= 2;
    }
    
    // 分配新的数据数组
    size_t new_data_size = queue->element_size * new_capacity;
    void* new_data = cn_malloc(new_data_size, "CN_queue_array_new_data");
    if (new_data == NULL)
    {
        return false;
    }
    
    // 复制现有元素
    if (queue->size > 0)
    {
        // 如果队列有回绕，需要分两次复制
        if (queue->impl.array.head_index < queue->impl.array.tail_index)
        {
            // 连续存储，一次复制
            void* src = (char*)queue->impl.array.data + (queue->impl.array.head_index * queue->element_size);
            memcpy(new_data, src, queue->size * queue->element_size);
        }
        else if (queue->impl.array.head_index > queue->impl.array.tail_index)
        {
            // 回绕存储，分两次复制
            size_t first_part = queue->impl.array.capacity - queue->impl.array.head_index;
            void* src_first = (char*)queue->impl.array.data + (queue->impl.array.head_index * queue->element_size);
            memcpy(new_data, src_first, first_part * queue->element_size);
            
            void* src_second = queue->impl.array.data;
            void* dst_second = (char*)new_data + (first_part * queue->element_size);
            memcpy(dst_second, src_second, queue->impl.array.tail_index * queue->element_size);
        }
    }
    
    // 释放旧数据
    cn_free(queue->impl.array.data);
    
    // 更新队列结构
    queue->impl.array.data = new_data;
    queue->impl.array.capacity = new_capacity;
    queue->impl.array.head_index = 0;
    queue->impl.array.tail_index = queue->size;
    
    return true;
}

/**
 * @brief 复制数组队列
 */
Stru_CN_Queue_t* CN_queue_internal_array_copy(const Stru_CN_Queue_t* src)
{
    if (src == NULL)
    {
        return NULL;
    }
    
    // 创建新队列
    Stru_CN_Queue_t* dst = CN_queue_internal_create_array(
        src->element_size,
        src->impl.array.capacity,
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
            void* src_element = CN_queue_internal_array_get_element(src, i);
            if (src_element == NULL)
            {
                CN_queue_internal_destroy_array(dst);
                return NULL;
            }
            
            if (!CN_queue_internal_array_enqueue(dst, src_element))
            {
                CN_queue_internal_destroy_array(dst);
                return NULL;
            }
        }
    }
    
    return dst;
}

/**
 * @brief 获取数组队列元素指针
 */
void* CN_queue_internal_array_get_element(const Stru_CN_Queue_t* queue, size_t index)
{
    if (queue == NULL || index >= queue->size)
    {
        return NULL;
    }
    
    // 计算物理索引（考虑回绕）
    size_t physical_index = queue->impl.array.head_index + index;
    if (physical_index >= queue->impl.array.capacity)
    {
        physical_index -= queue->impl.array.capacity;
    }
    
    return (char*)queue->impl.array.data + (physical_index * queue->element_size);
}

/**
 * @brief 清空数组队列
 */
void CN_queue_internal_array_clear(Stru_CN_Queue_t* queue)
{
    if (queue == NULL)
    {
        return;
    }
    
    // 如果有释放函数，释放所有元素
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
    
    // 重置索引
    queue->impl.array.head_index = 0;
    queue->impl.array.tail_index = 0;
    queue->size = 0;
}
