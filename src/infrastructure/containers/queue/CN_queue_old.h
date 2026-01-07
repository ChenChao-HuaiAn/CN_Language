/**
 * @file CN_queue.h
 * @brief 队列模块头文件
 * 
 * 提供队列数据结构的定义和接口声明。
 * 队列是一种先进先出（FIFO）的数据结构。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_QUEUE_H
#define CN_QUEUE_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 队列结构体
 * 
 * 基于循环数组实现的队列数据结构。
 */
typedef struct Stru_Queue_t
{
    void** items;           /**< 元素指针数组 */
    size_t capacity;        /**< 当前分配的容量 */
    size_t front;           /**< 队头位置 */
    size_t rear;            /**< 队尾位置（下一个空闲位置） */
    size_t size;            /**< 队列中元素数量 */
    size_t item_size;       /**< 每个元素的大小（字节） */
} Stru_Queue_t;

/**
 * @brief 创建队列
 * 
 * 分配并初始化一个新的队列。
 * 
 * @param item_size 每个元素的大小（字节）
 * @return 指向新创建的队列的指针，失败返回NULL
 */
Stru_Queue_t* F_create_queue(size_t item_size);

/**
 * @brief 销毁队列
 * 
 * 释放队列占用的所有内存。
 * 
 * @param queue 要销毁的队列指针
 * 
 * @note 如果queue为NULL，函数不执行任何操作
 */
void F_destroy_queue(Stru_Queue_t* queue);

/**
 * @brief 入队
 * 
 * 将元素加入队尾。
 * 
 * @param queue 队列指针
 * @param item 要加入的元素指针
 * @return 入队成功返回true，失败返回false
 */
bool F_queue_enqueue(Stru_Queue_t* queue, void* item);

/**
 * @brief 出队
 * 
 * 从队头移除元素。
 * 
 * @param queue 队列指针
 * @param out_item 输出参数，用于接收移除的元素（可为NULL）
 * @return 出队成功返回true，队列为空返回false
 */
bool F_queue_dequeue(Stru_Queue_t* queue, void* out_item);

/**
 * @brief 查看队头元素
 * 
 * 查看队头元素但不移除。
 * 
 * @param queue 队列指针
 * @return 指向队头元素的指针，队列为空返回NULL
 */
void* F_queue_peek(Stru_Queue_t* queue);

/**
 * @brief 获取队列大小
 * 
 * 返回队列中元素的数量。
 * 
 * @param queue 队列指针
 * @return 队列大小，如果queue为NULL返回0
 */
size_t F_queue_size(Stru_Queue_t* queue);

/**
 * @brief 检查队列是否为空
 * 
 * 检查队列是否不包含任何元素。
 * 
 * @param queue 队列指针
 * @return 队列为空返回true，否则返回false
 */
bool F_queue_is_empty(Stru_Queue_t* queue);

/**
 * @brief 检查队列是否已满
 * 
 * 检查队列是否已满。
 * 
 * @param queue 队列指针
 * @return 队列已满返回true，否则返回false
 */
bool F_queue_is_full(Stru_Queue_t* queue);

/**
 * @brief 清空队列
 * 
 * 移除队列中的所有元素，但不释放队列本身。
 * 
 * @param queue 队列指针
 * 
 * @note 清空后队列大小变为0
 */
void F_queue_clear(Stru_Queue_t* queue);

/**
 * @brief 队列迭代器结构体
 * 
 * 用于遍历队列中的元素。
 */
typedef struct Stru_QueueIterator_t
{
    Stru_Queue_t* queue;    /**< 要遍历的队列 */
    size_t current_index;   /**< 当前遍历位置 */
    size_t visited_count;   /**< 已访问元素数量 */
} Stru_QueueIterator_t;

/**
 * @brief 创建队列迭代器
 * 
 * 创建一个新的队列迭代器，用于遍历队列中的元素。
 * 
 * @param queue 要遍历的队列
 * @return 指向新创建的迭代器的指针，失败返回NULL
 */
Stru_QueueIterator_t* F_create_queue_iterator(Stru_Queue_t* queue);

/**
 * @brief 销毁队列迭代器
 * 
 * 释放迭代器占用的内存。
 * 
 * @param iterator 要销毁的迭代器指针
 */
void F_destroy_queue_iterator(Stru_QueueIterator_t* iterator);

/**
 * @brief 检查迭代器是否有下一个元素
 * 
 * 检查迭代器是否还有未遍历的元素。
 * 
 * @param iterator 迭代器指针
 * @return 还有下一个元素返回true，否则返回false
 */
bool F_queue_iterator_has_next(Stru_QueueIterator_t* iterator);

/**
 * @brief 获取迭代器的下一个元素
 * 
 * 获取迭代器的下一个元素，并将迭代器移动到下一个位置。
 * 
 * @param iterator 迭代器指针
 * @param out_item 输出参数，用于接收元素（可为NULL）
 * @return 成功获取元素返回true，没有更多元素返回false
 */
bool F_queue_iterator_next(Stru_QueueIterator_t* iterator, void* out_item);

/**
 * @brief 重置队列迭代器
 * 
 * 将迭代器重置到队列的开始位置。
 * 
 * @param iterator 要重置的迭代器指针
 */
void F_queue_iterator_reset(Stru_QueueIterator_t* iterator);

/**
 * @brief 复制队列
 * 
 * 创建一个队列的深拷贝。
 * 
 * @param src_queue 源队列指针
 * @return 指向新创建的队列副本的指针，失败返回NULL
 */
Stru_Queue_t* F_queue_copy(Stru_Queue_t* src_queue);

/**
 * @brief 比较两个队列是否相等
 * 
 * 比较两个队列的内容是否完全相同。
 * 
 * @param queue1 第一个队列指针
 * @param queue2 第二个队列指针
 * @param compare_func 元素比较函数，如果为NULL则使用memcmp
 * @return 队列相等返回true，否则返回false
 */
bool F_queue_equals(Stru_Queue_t* queue1, Stru_Queue_t* queue2, 
                   int (*compare_func)(const void*, const void*));

/**
 * @brief 批量入队
 * 
 * 将多个元素一次性加入队列。
 * 
 * @param queue 队列指针
 * @param items 元素数组指针
 * @param count 元素数量
 * @return 成功入队的元素数量
 */
size_t F_queue_enqueue_batch(Stru_Queue_t* queue, void** items, size_t count);

/**
 * @brief 批量出队
 * 
 * 一次性从队列中移除多个元素。
 * 
 * @param queue 队列指针
 * @param out_items 输出数组指针（可为NULL）
 * @param max_count 最多出队的元素数量
 * @return 实际出队的元素数量
 */
size_t F_queue_dequeue_batch(Stru_Queue_t* queue, void** out_items, size_t max_count);

/**
 * @brief 获取队列容量
 * 
 * 返回队列当前分配的容量。
 * 
 * @param queue 队列指针
 * @return 队列容量，如果queue为NULL返回0
 */
size_t F_queue_capacity(Stru_Queue_t* queue);

/**
 * @brief 调整队列容量
 * 
 * 调整队列的容量。如果新容量小于当前大小，会失败。
 * 
 * @param queue 队列指针
 * @param new_capacity 新的容量
 * @return 调整成功返回true，失败返回false
 */
bool F_queue_reserve(Stru_Queue_t* queue, size_t new_capacity);

#ifdef __cplusplus
}
#endif

#endif // CN_QUEUE_H
