/**
 * @file CN_queue_utils.c
 * @brief 队列工具模块实现文件
 * 
 * 实现队列的高级工具功能。
 * 包括队列复制、比较、批量操作等高级功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_queue_utils.h"
#include <stdlib.h>
#include <string.h>

Stru_Queue_t* F_queue_copy(Stru_Queue_t* src_queue)
{
    if (src_queue == NULL)
    {
        return NULL;
    }
    
    // 创建新队列
    Stru_Queue_t* new_queue = F_create_queue(src_queue->item_size);
    if (new_queue == NULL)
    {
        return NULL;
    }
    
    // 调整容量
    if (!F_queue_reserve(new_queue, src_queue->size))
    {
        F_destroy_queue(new_queue);
        return NULL;
    }
    
    // 复制所有元素
    for (size_t i = 0; i < src_queue->size; i++)
    {
        size_t index = (src_queue->front + i) % src_queue->capacity;
        void* src_item = src_queue->items[index];
        
        if (src_item != NULL)
        {
            // 分配新元素内存并复制数据
            void* new_item = malloc(src_queue->item_size);
            if (new_item == NULL)
            {
                F_destroy_queue(new_queue);
                return NULL;
            }
            
            memcpy(new_item, src_item, src_queue->item_size);
            new_queue->items[i] = new_item;
        }
        else
        {
            new_queue->items[i] = NULL;
        }
    }
    
    // 更新新队列状态
    new_queue->size = src_queue->size;
    new_queue->rear = src_queue->size;
    
    return new_queue;
}

bool F_queue_equals(Stru_Queue_t* queue1, Stru_Queue_t* queue2, 
                   int (*compare_func)(const void*, const void*))
{
    if (queue1 == NULL || queue2 == NULL)
    {
        return false;
    }
    
    // 检查基本属性
    if (queue1->item_size != queue2->item_size || queue1->size != queue2->size)
    {
        return false;
    }
    
    // 如果两个队列都为空，则相等
    if (queue1->size == 0 && queue2->size == 0)
    {
        return true;
    }
    
    // 比较每个元素
    for (size_t i = 0; i < queue1->size; i++)
    {
        size_t index1 = (queue1->front + i) % queue1->capacity;
        size_t index2 = (queue2->front + i) % queue2->capacity;
        
        void* item1 = queue1->items[index1];
        void* item2 = queue2->items[index2];
        
        // 如果两个元素都为NULL，则相等
        if (item1 == NULL && item2 == NULL)
        {
            continue;
        }
        
        // 如果一个为NULL另一个不为NULL，则不相等
        if (item1 == NULL || item2 == NULL)
        {
            return false;
        }
        
        // 比较元素内容
        if (compare_func != NULL)
        {
            if (compare_func(item1, item2) != 0)
            {
                return false;
            }
        }
        else
        {
            if (memcmp(item1, item2, queue1->item_size) != 0)
            {
                return false;
            }
        }
    }
    
    return true;
}

size_t F_queue_enqueue_batch(Stru_Queue_t* queue, void** items, size_t count)
{
    if (queue == NULL || items == NULL || count == 0)
    {
        return 0;
    }
    
    size_t success_count = 0;
    
    for (size_t i = 0; i < count; i++)
    {
        if (F_queue_enqueue(queue, items[i]))
        {
            success_count++;
        }
        else
        {
            // 如果入队失败，停止批量操作
            break;
        }
    }
    
    return success_count;
}

size_t F_queue_dequeue_batch(Stru_Queue_t* queue, void** out_items, size_t max_count)
{
    if (queue == NULL || max_count == 0)
    {
        return 0;
    }
    
    size_t dequeue_count = 0;
    
    for (size_t i = 0; i < max_count; i++)
    {
        void* out_item = (out_items != NULL) ? out_items[i] : NULL;
        
        if (!F_queue_dequeue(queue, out_item))
        {
            // 如果出队失败（队列为空），停止批量操作
            break;
        }
        
        dequeue_count++;
    }
    
    return dequeue_count;
}

size_t F_queue_to_array(Stru_Queue_t* queue, void** out_array)
{
    if (queue == NULL || out_array == NULL || queue->size == 0)
    {
        *out_array = NULL;
        return 0;
    }
    
    // 分配数组内存
    void* array = malloc(queue->size * queue->item_size);
    if (array == NULL)
    {
        *out_array = NULL;
        return 0;
    }
    
    // 复制元素到数组
    for (size_t i = 0; i < queue->size; i++)
    {
        size_t index = (queue->front + i) % queue->capacity;
        void* item = queue->items[index];
        
        if (item != NULL)
        {
            void* dest = (char*)array + i * queue->item_size;
            memcpy(dest, item, queue->item_size);
        }
    }
    
    *out_array = array;
    return queue->size;
}

Stru_Queue_t* F_queue_from_array(void** items, size_t count, size_t item_size)
{
    if (items == NULL || count == 0 || item_size == 0)
    {
        return NULL;
    }
    
    // 创建新队列
    Stru_Queue_t* queue = F_create_queue(item_size);
    if (queue == NULL)
    {
        return NULL;
    }
    
    // 调整容量
    if (!F_queue_reserve(queue, count))
    {
        F_destroy_queue(queue);
        return NULL;
    }
    
    // 批量入队
    size_t success_count = F_queue_enqueue_batch(queue, items, count);
    
    if (success_count != count)
    {
        // 如果部分元素入队失败，销毁队列
        F_destroy_queue(queue);
        return NULL;
    }
    
    return queue;
}

int F_queue_find(Stru_Queue_t* queue, void* item, 
                int (*compare_func)(const void*, const void*))
{
    if (queue == NULL || item == NULL || queue->size == 0)
    {
        return -1;
    }
    
    for (size_t i = 0; i < queue->size; i++)
    {
        size_t index = (queue->front + i) % queue->capacity;
        void* queue_item = queue->items[index];
        
        if (queue_item != NULL)
        {
            bool equal = false;
            
            if (compare_func != NULL)
            {
                equal = (compare_func(queue_item, item) == 0);
            }
            else
            {
                equal = (memcmp(queue_item, item, queue->item_size) == 0);
            }
            
            if (equal)
            {
                return (int)i;
            }
        }
    }
    
    return -1;
}

bool F_queue_contains(Stru_Queue_t* queue, void* item, 
                     int (*compare_func)(const void*, const void*))
{
    return F_queue_find(queue, item, compare_func) >= 0;
}
