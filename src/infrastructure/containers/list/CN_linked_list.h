/**
 * @file CN_linked_list.h
 * @brief 链表模块头文件
 * 
 * 提供链表数据结构的定义和接口声明。
 * 链表支持高效的插入和删除操作，特别适合频繁修改的场景。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_LINKED_LIST_H
#define CN_LINKED_LIST_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 链表节点结构体
 * 
 * 链表的基本单元，包含数据和指向下一个节点的指针。
 */
typedef struct Stru_ListNode_t
{
    void* data;                 /**< 节点数据指针 */
    struct Stru_ListNode_t* next; /**< 指向下一个节点的指针 */
} Stru_ListNode_t;

/**
 * @brief 链表结构体
 * 
 * 单向链表数据结构，支持在头部和尾部高效插入。
 */
typedef struct Stru_LinkedList_t
{
    Stru_ListNode_t* head;      /**< 链表头节点 */
    Stru_ListNode_t* tail;      /**< 链表尾节点 */
    size_t length;              /**< 链表长度 */
    size_t item_size;           /**< 每个元素的大小（字节） */
} Stru_LinkedList_t;

/**
 * @brief 创建链表
 * 
 * 分配并初始化一个新的链表。
 * 
 * @param item_size 每个元素的大小（字节）
 * @return 指向新创建的链表的指针，失败返回NULL
 */
Stru_LinkedList_t* F_create_linked_list(size_t item_size);

/**
 * @brief 销毁链表
 * 
 * 释放链表占用的所有内存。
 * 
 * @param list 要销毁的链表指针
 * 
 * @note 如果list为NULL，函数不执行任何操作
 */
void F_destroy_linked_list(Stru_LinkedList_t* list);

/**
 * @brief 在链表头部插入元素
 * 
 * 在链表的头部插入一个新元素。
 * 
 * @param list 链表指针
 * @param data 要插入的数据指针
 * @return 插入成功返回true，失败返回false
 */
bool F_linked_list_prepend(Stru_LinkedList_t* list, void* data);

/**
 * @brief 在链表尾部插入元素
 * 
 * 在链表的尾部插入一个新元素。
 * 
 * @param list 链表指针
 * @param data 要插入的数据指针
 * @return 插入成功返回true，失败返回false
 */
bool F_linked_list_append(Stru_LinkedList_t* list, void* data);

/**
 * @brief 在指定位置插入元素
 * 
 * 在链表的指定位置插入一个新元素。
 * 
 * @param list 链表指针
 * @param data 要插入的数据指针
 * @param position 插入位置（0-based）
 * @return 插入成功返回true，失败返回false
 * 
 * @note 如果position等于链表长度，则在尾部插入
 */
bool F_linked_list_insert(Stru_LinkedList_t* list, void* data, size_t position);

/**
 * @brief 从链表头部移除元素
 * 
 * 移除链表的头部元素。
 * 
 * @param list 链表指针
 * @param out_data 输出参数，用于接收移除的数据（可为NULL）
 * @return 移除成功返回true，链表为空返回false
 */
bool F_linked_list_remove_first(Stru_LinkedList_t* list, void* out_data);

/**
 * @brief 从链表尾部移除元素
 * 
 * 移除链表的尾部元素。
 * 
 * @param list 链表指针
 * @param out_data 输出参数，用于接收移除的数据（可为NULL）
 * @return 移除成功返回true，链表为空返回false
 */
bool F_linked_list_remove_last(Stru_LinkedList_t* list, void* out_data);

/**
 * @brief 从指定位置移除元素
 * 
 * 移除链表中指定位置的元素。
 * 
 * @param list 链表指针
 * @param position 要移除的元素位置（0-based）
 * @param out_data 输出参数，用于接收移除的数据（可为NULL）
 * @return 移除成功返回true，位置无效返回false
 */
bool F_linked_list_remove_at(Stru_LinkedList_t* list, size_t position, void* out_data);

/**
 * @brief 获取链表长度
 * 
 * 返回链表中当前元素的数量。
 * 
 * @param list 链表指针
 * @return 链表长度，如果list为NULL返回0
 */
size_t F_linked_list_length(Stru_LinkedList_t* list);

/**
 * @brief 获取链表元素
 * 
 * 获取链表中指定位置的元素。
 * 
 * @param list 链表指针
 * @param position 元素位置（0-based）
 * @return 指向元素的指针，位置无效返回NULL
 */
void* F_linked_list_get(Stru_LinkedList_t* list, size_t position);

/**
 * @brief 查找元素位置
 * 
 * 查找指定元素在链表中的位置。
 * 
 * @param list 链表指针
 * @param data 要查找的数据指针
 * @param compare 比较函数，返回0表示相等
 * @return 元素位置（0-based），未找到返回-1
 */
int64_t F_linked_list_find(Stru_LinkedList_t* list, void* data, 
                          int (*compare)(void*, void*));

/**
 * @brief 清空链表
 * 
 * 移除链表中的所有元素，但不释放链表本身。
 * 
 * @param list 链表指针
 * 
 * @note 清空后链表长度变为0
 */
void F_linked_list_clear(Stru_LinkedList_t* list);

/**
 * @brief 遍历链表
 * 
 * 对链表中的每个元素执行指定的操作。
 * 
 * @param list 链表指针
 * @param callback 回调函数，接收元素指针和用户数据
 * @param user_data 传递给回调函数的用户数据
 */
void F_linked_list_foreach(Stru_LinkedList_t* list,
                          void (*callback)(void* data, void* user_data),
                          void* user_data);

/**
 * @brief 检查链表是否为空
 * 
 * 检查链表是否不包含任何元素。
 * 
 * @param list 链表指针
 * @return 链表为空返回true，否则返回false
 */
bool F_linked_list_is_empty(Stru_LinkedList_t* list);

/**
 * @brief 链表迭代器接口
 * 
 * 为了方便使用，这里包含迭代器接口的声明。
 * 详细实现见CN_linked_list_iterator.h
 */
#include "CN_linked_list_iterator.h"

/**
 * @brief 链表排序接口
 * 
 * 为了方便使用，这里包含排序接口的声明。
 * 详细实现见CN_linked_list_sort.h
 */
#include "CN_linked_list_sort.h"

#ifdef __cplusplus
}
#endif

#endif // CN_LINKED_LIST_H
