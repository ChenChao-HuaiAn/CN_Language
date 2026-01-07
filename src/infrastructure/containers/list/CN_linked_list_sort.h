/**
 * @file CN_linked_list_sort.h
 * @brief 链表排序模块头文件
 * 
 * 提供链表排序算法的接口声明。
 * 支持多种排序算法，包括冒泡排序、插入排序和归并排序。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_LINKED_LIST_SORT_H
#define CN_LINKED_LIST_SORT_H

#include "CN_linked_list.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 排序算法类型枚举
 */
typedef enum Eum_SortAlgorithm_t
{
    Eum_SORT_BUBBLE,        /**< 冒泡排序 */
    Eum_SORT_INSERTION,     /**< 插入排序 */
    Eum_SORT_MERGE,         /**< 归并排序 */
    Eum_SORT_QUICK,         /**< 快速排序 */
    Eum_SORT_SELECTION      /**< 选择排序 */
} Eum_SortAlgorithm_t;

/**
 * @brief 排序方向枚举
 */
typedef enum Eum_SortDirection_t
{
    Eum_SORT_ASCENDING,     /**< 升序 */
    Eum_SORT_DESCENDING     /**< 降序 */
} Eum_SortDirection_t;

/**
 * @brief 排序链表
 * 
 * 使用指定的排序算法对链表进行排序。
 * 
 * @param list 链表指针
 * @param compare 比较函数，返回负数表示a<b，0表示a==b，正数表示a>b
 * @param algorithm 排序算法
 * @param direction 排序方向
 * @return 排序成功返回true，失败返回false
 */
bool F_linked_list_sort(Stru_LinkedList_t* list,
                       int (*compare)(void*, void*),
                       Eum_SortAlgorithm_t algorithm,
                       Eum_SortDirection_t direction);

/**
 * @brief 冒泡排序链表
 * 
 * 使用冒泡排序算法对链表进行排序。
 * 
 * @param list 链表指针
 * @param compare 比较函数
 * @param ascending 是否升序排序
 * @return 排序成功返回true，失败返回false
 */
bool F_linked_list_bubble_sort(Stru_LinkedList_t* list,
                              int (*compare)(void*, void*),
                              bool ascending);

/**
 * @brief 插入排序链表
 * 
 * 使用插入排序算法对链表进行排序。
 * 
 * @param list 链表指针
 * @param compare 比较函数
 * @param ascending 是否升序排序
 * @return 排序成功返回true，失败返回false
 */
bool F_linked_list_insertion_sort(Stru_LinkedList_t* list,
                                 int (*compare)(void*, void*),
                                 bool ascending);

/**
 * @brief 归并排序链表
 * 
 * 使用归并排序算法对链表进行排序。
 * 
 * @param list 链表指针
 * @param compare 比较函数
 * @param ascending 是否升序排序
 * @return 排序成功返回true，失败返回false
 */
bool F_linked_list_merge_sort(Stru_LinkedList_t* list,
                             int (*compare)(void*, void*),
                             bool ascending);

/**
 * @brief 检查链表是否已排序
 * 
 * 检查链表是否按照指定的顺序排序。
 * 
 * @param list 链表指针
 * @param compare 比较函数
 * @param ascending 是否检查升序
 * @return 已排序返回true，未排序返回false
 */
bool F_linked_list_is_sorted(Stru_LinkedList_t* list,
                            int (*compare)(void*, void*),
                            bool ascending);

/**
 * @brief 反转链表
 * 
 * 反转链表中元素的顺序。
 * 
 * @param list 链表指针
 * @return 反转成功返回true，失败返回false
 */
bool F_linked_list_reverse(Stru_LinkedList_t* list);

#ifdef __cplusplus
}
#endif

#endif // CN_LINKED_LIST_SORT_H
