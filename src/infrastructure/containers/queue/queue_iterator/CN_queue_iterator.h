/**
 * @file CN_queue_iterator.h
 * @brief 队列迭代器模块头文件
 * 
 * 提供队列迭代器的定义和接口声明。
 * 迭代器用于遍历队列中的元素，支持顺序访问。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_QUEUE_ITERATOR_H
#define CN_QUEUE_ITERATOR_H

#include "../queue_core/CN_queue_core.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

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
 * @brief 获取迭代器当前元素
 * 
 * 获取迭代器当前指向的元素，但不移动迭代器。
 * 
 * @param iterator 迭代器指针
 * @return 指向当前元素的指针，没有当前元素返回NULL
 */
void* F_queue_iterator_current(Stru_QueueIterator_t* iterator);

/**
 * @brief 获取已访问元素数量
 * 
 * 返回迭代器已经访问过的元素数量。
 * 
 * @param iterator 迭代器指针
 * @return 已访问元素数量，如果iterator为NULL返回0
 */
size_t F_queue_iterator_visited_count(Stru_QueueIterator_t* iterator);

/**
 * @brief 获取剩余元素数量
 * 
 * 返回迭代器还未访问的元素数量。
 * 
 * @param iterator 迭代器指针
 * @return 剩余元素数量，如果iterator为NULL返回0
 */
size_t F_queue_iterator_remaining_count(Stru_QueueIterator_t* iterator);

#ifdef __cplusplus
}
#endif

#endif // CN_QUEUE_ITERATOR_H
