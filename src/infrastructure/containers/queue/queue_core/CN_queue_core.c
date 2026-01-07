/**
 * @file CN_queue_core.c
 * @brief 队列核心模块实现文件
 * 
 * 实现队列数据结构的核心功能。
 * 包括创建、销毁、入队、出队、查看等基本操作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_queue_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief 内部函数：确保队列有足够容量
 * 
 * 检查队列是否有足够容量容纳新元素，如果没有则扩容。
 * 
 * @param queue 队列指针
 * @return 容量足够或扩容成功返回true，失败返回false
 */
static bool ensure_capacity(Stru_Queue_t* queue)
{
    if (queue == NULL)
    {
        return false;
    }
    
    // 如果队列已满，需要扩容
    if (queue->size >= queue->capacity)
    {
        size_t new_capacity = queue->capacity * CN_QUEUE_GROWTH_FACTOR;
        if (new_capacity < CN_QUEUE_INITIAL_CAPACITY)
        {
            new_capacity = CN_QUEUE_INITIAL_CAPACITY;
        }
        
        // 分配新数组
        void** new_items = (void**)malloc(new_capacity * sizeof(void*));
        if (new_items == NULL)
        {
            return false;
        }
        
        // 复制元素到新数组（保持顺序）
        if (queue->size > 0)
        {
            if (queue->front < queue->rear)
            {
                // 连续存储的情况
                memcpy(new_items, &queue->items[queue->front], queue->size * sizeof(void*));
            }
            else
            {
                // 环绕存储的情况
                size_t first_part = queue->capacity - queue->front;
                memcpy(new_items, &queue->items[queue->front], first_part * sizeof(void*));
                memcpy(&new_items[first_part], queue->items, queue->rear * sizeof(void*));
            }
        }
        
        // 释放旧数组
        free(queue->items);
        
        // 更新队列状态
        queue->items = new_items;
        queue->capacity = new_capacity;
        queue->front = 0;
        queue->rear = queue->size;
    }
    
    return true;
}

Stru_Queue_t* F_create_queue(size_t item_size)
{
    if (item_size == 0)
    {
        return NULL;
    }
    
    // 分配队列结构体
    Stru_Queue_t* queue = (Stru_Queue_t*)malloc(sizeof(Stru_Queue_t));
    if (queue == NULL)
    {
        return NULL;
    }
    
    // 初始化字段
    queue->capacity = CN_QUEUE_INITIAL_CAPACITY;
    queue->front = 0;
    queue->rear = 0;
    queue->size = 0;
    queue->item_size = item_size;
    
    // 分配初始元素数组
    queue->items = (void**)malloc(CN_QUEUE_INITIAL_CAPACITY * sizeof(void*));
    if (queue->items == NULL)
    {
        free(queue);
        return NULL;
    }
    
    // 初始化指针为NULL
    for (size_t i = 0; i < queue->capacity; i++)
    {
        queue->items[i] = NULL;
    }
    
    return queue;
}

void F_destroy_queue(Stru_Queue_t* queue)
{
    if (queue == NULL)
    {
        return;
    }
    
    // 释放所有元素
    for (size_t i = 0; i < queue->size; i++)
    {
        size_t index = (queue->front + i) % queue->capacity;
        if (queue->items[index] != NULL)
        {
            free(queue->items[index]);
            queue->items[index] = NULL;
        }
    }
    
    // 释放元素数组
    if (queue->items != NULL)
    {
        free(queue->items);
        queue->items = NULL;
    }
    
    // 释放队列结构体
    free(queue);
}

bool F_queue_enqueue(Stru_Queue_t* queue, void* item)
{
    if (queue == NULL || item == NULL)
    {
        return false;
    }
    
    // 确保有足够容量
    if (!ensure_capacity(queue))
    {
        return false;
    }
    
    // 分配新元素内存并复制数据
    void* new_item = malloc(queue->item_size);
    if (new_item == NULL)
    {
        return false;
    }
    
    memcpy(new_item, item, queue->item_size);
    queue->items[queue->rear] = new_item;
    
    // 更新队尾位置
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->size++;
    
    return true;
}

bool F_queue_dequeue(Stru_Queue_t* queue, void* out_item)
{
    if (queue == NULL || queue->size == 0)
    {
        return false;
    }
    
    // 获取队头元素
    void* front_item = queue->items[queue->front];
    
    // 复制数据到输出参数（如果提供）
    if (out_item != NULL && front_item != NULL)
    {
        memcpy(out_item, front_item, queue->item_size);
    }
    
    // 释放队头元素
    if (front_item != NULL)
    {
        free(front_item);
    }
    
    queue->items[queue->front] = NULL;
    
    // 更新队头位置
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;
    
    return true;
}

void* F_queue_peek(Stru_Queue_t* queue)
{
    if (queue == NULL || queue->size == 0)
    {
        return NULL;
    }
    
    return queue->items[queue->front];
}

size_t F_queue_size(Stru_Queue_t* queue)
{
    if (queue == NULL)
    {
        return 0;
    }
    
    return queue->size;
}

bool F_queue_is_empty(Stru_Queue_t* queue)
{
    if (queue == NULL)
    {
        return true;
    }
    
    return queue->size == 0;
}

bool F_queue_is_full(Stru_Queue_t* queue)
{
    if (queue == NULL)
    {
        return false;
    }
    
    return queue->size >= queue->capacity;
}

void F_queue_clear(Stru_Queue_t* queue)
{
    if (queue == NULL)
    {
        return;
    }
    
    // 释放所有元素但不释放数组
    for (size_t i = 0; i < queue->size; i++)
    {
        size_t index = (queue->front + i) % queue->capacity;
        if (queue->items[index] != NULL)
        {
            free(queue->items[index]);
            queue->items[index] = NULL;
        }
    }
    
    // 重置队列状态
    queue->front = 0;
    queue->rear = 0;
    queue->size = 0;
}

size_t F_queue_capacity(Stru_Queue_t* queue)
{
    if (queue == NULL)
    {
        return 0;
    }
    
    return queue->capacity;
}

bool F_queue_reserve(Stru_Queue_t* queue, size_t new_capacity)
{
    if (queue == NULL || new_capacity < queue->size)
    {
        return false;
    }
    
    if (new_capacity <= queue->capacity)
    {
        return true; // 不需要调整
    }
    
    // 分配新数组
    void** new_items = (void**)malloc(new_capacity * sizeof(void*));
    if (new_items == NULL)
    {
        return false;
    }
    
    // 复制元素到新数组（保持顺序）
    if (queue->size > 0)
    {
        if (queue->front < queue->rear)
        {
            // 连续存储的情况
            memcpy(new_items, &queue->items[queue->front], queue->size * sizeof(void*));
        }
        else
        {
            // 环绕存储的情况
            size_t first_part = queue->capacity - queue->front;
            memcpy(new_items, &queue->items[queue->front], first_part * sizeof(void*));
            memcpy(&new_items[first_part], queue->items, queue->rear * sizeof(void*));
        }
    }
    
    // 初始化新位置为NULL
    for (size_t i = queue->size; i < new_capacity; i++)
    {
        new_items[i] = NULL;
    }
    
    // 释放旧数组
    free(queue->items);
    
    // 更新队列状态
    queue->items = new_items;
    queue->capacity = new_capacity;
    queue->front = 0;
    queue->rear = queue->size;
    
    return true;
}
