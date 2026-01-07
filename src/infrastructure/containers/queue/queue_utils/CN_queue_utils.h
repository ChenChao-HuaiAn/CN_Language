/**
 * @file CN_queue_utils.h
 * @brief 队列工具模块头文件
 * 
 * 提供队列的高级工具功能。
 * 包括队列复制、比较、批量操作等高级功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_QUEUE_UTILS_H
#define CN_QUEUE_UTILS_H

#include "../queue_core/CN_queue_core.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

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
 * @brief 将队列转换为数组
 * 
 * 将队列中的所有元素复制到一个新分配的数组中。
 * 
 * @param queue 队列指针
 * @param out_array 输出参数，指向分配的数组指针
 * @return 数组中的元素数量，失败返回0
 */
size_t F_queue_to_array(Stru_Queue_t* queue, void** out_array);

/**
 * @brief 从数组创建队列
 * 
 * 从数组创建一个新的队列。
 * 
 * @param items 元素数组指针
 * @param count 元素数量
 * @param item_size 每个元素的大小（字节）
 * @return 指向新创建的队列的指针，失败返回NULL
 */
Stru_Queue_t* F_queue_from_array(void** items, size_t count, size_t item_size);

/**
 * @brief 查找队列中的元素
 * 
 * 在队列中查找指定的元素。
 * 
 * @param queue 队列指针
 * @param item 要查找的元素指针
 * @param compare_func 元素比较函数，如果为NULL则使用memcmp
 * @return 找到的元素位置（从0开始），未找到返回-1
 */
int F_queue_find(Stru_Queue_t* queue, void* item, 
                int (*compare_func)(const void*, const void*));

/**
 * @brief 检查队列是否包含元素
 * 
 * 检查队列是否包含指定的元素。
 * 
 * @param queue 队列指针
 * @param item 要检查的元素指针
 * @param compare_func 元素比较函数，如果为NULL则使用memcmp
 * @return 包含返回true，否则返回false
 */
bool F_queue_contains(Stru_Queue_t* queue, void* item, 
                     int (*compare_func)(const void*, const void*));

#ifdef __cplusplus
}
#endif

#endif // CN_QUEUE_UTILS_H
