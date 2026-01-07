/**
 * @file CN_queue_iterator.c
 * @brief 队列迭代器模块实现文件
 * 
 * 实现队列迭代器的功能。
 * 包括创建、销毁、遍历、重置等操作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_queue_iterator.h"
#include <stdlib.h>
#include <string.h>

Stru_QueueIterator_t* F_create_queue_iterator(Stru_Queue_t* queue)
{
    if (queue == NULL)
    {
        return NULL;
    }
    
    Stru_QueueIterator_t* iterator = (Stru_QueueIterator_t*)malloc(sizeof(Stru_QueueIterator_t));
    if (iterator == NULL)
    {
        return NULL;
    }
    
    iterator->queue = queue;
    iterator->current_index = queue->front;
    iterator->visited_count = 0;
    
    return iterator;
}

void F_destroy_queue_iterator(Stru_QueueIterator_t* iterator)
{
    if (iterator == NULL)
    {
        return;
    }
    
    free(iterator);
}

bool F_queue_iterator_has_next(Stru_QueueIterator_t* iterator)
{
    if (iterator == NULL || iterator->queue == NULL)
    {
        return false;
    }
    
    return iterator->visited_count < iterator->queue->size;
}

bool F_queue_iterator_next(Stru_QueueIterator_t* iterator, void* out_item)
{
    if (iterator == NULL || iterator->queue == NULL || 
        iterator->visited_count >= iterator->queue->size)
    {
        return false;
    }
    
    // 获取当前元素
    void* current_item = iterator->queue->items[iterator->current_index];
    
    // 复制数据到输出参数（如果提供）
    if (out_item != NULL && current_item != NULL)
    {
        memcpy(out_item, current_item, iterator->queue->item_size);
    }
    
    // 移动到下一个位置
    iterator->current_index = (iterator->current_index + 1) % iterator->queue->capacity;
    iterator->visited_count++;
    
    return true;
}

void F_queue_iterator_reset(Stru_QueueIterator_t* iterator)
{
    if (iterator == NULL || iterator->queue == NULL)
    {
        return;
    }
    
    iterator->current_index = iterator->queue->front;
    iterator->visited_count = 0;
}

void* F_queue_iterator_current(Stru_QueueIterator_t* iterator)
{
    if (iterator == NULL || iterator->queue == NULL || 
        iterator->visited_count >= iterator->queue->size)
    {
        return NULL;
    }
    
    return iterator->queue->items[iterator->current_index];
}

size_t F_queue_iterator_visited_count(Stru_QueueIterator_t* iterator)
{
    if (iterator == NULL)
    {
        return 0;
    }
    
    return iterator->visited_count;
}

size_t F_queue_iterator_remaining_count(Stru_QueueIterator_t* iterator)
{
    if (iterator == NULL || iterator->queue == NULL)
    {
        return 0;
    }
    
    return iterator->queue->size - iterator->visited_count;
}
