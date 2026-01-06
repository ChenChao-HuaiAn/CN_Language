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

#ifdef __cplusplus
}
#endif

#endif // CN_QUEUE_H
