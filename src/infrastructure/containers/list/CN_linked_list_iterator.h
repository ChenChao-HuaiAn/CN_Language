/**
 * @file CN_linked_list_iterator.h
 * @brief 链表迭代器模块头文件
 * 
 * 提供链表迭代器的定义和接口声明。
 * 迭代器支持安全、高效的链表遍历。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_LINKED_LIST_ITERATOR_H
#define CN_LINKED_LIST_ITERATOR_H

#include "CN_linked_list.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 链表迭代器结构体
 * 
 * 用于遍历链表的迭代器，支持前向遍历。
 */
typedef struct Stru_LinkedListIterator_t
{
    Stru_LinkedList_t* list;        /**< 关联的链表 */
    Stru_ListNode_t* current;       /**< 当前节点 */
    size_t position;                /**< 当前位置 */
    bool is_valid;                  /**< 迭代器是否有效 */
} Stru_LinkedListIterator_t;

/**
 * @brief 创建链表迭代器
 * 
 * 创建并初始化一个新的链表迭代器。
 * 
 * @param list 要遍历的链表
 * @return 指向新创建的迭代器的指针，失败返回NULL
 */
Stru_LinkedListIterator_t* F_create_linked_list_iterator(Stru_LinkedList_t* list);

/**
 * @brief 销毁链表迭代器
 * 
 * 释放迭代器占用的内存。
 * 
 * @param iterator 要销毁的迭代器指针
 */
void F_destroy_linked_list_iterator(Stru_LinkedListIterator_t* iterator);

/**
 * @brief 重置迭代器到链表头部
 * 
 * 将迭代器重置到链表的开始位置。
 * 
 * @param iterator 迭代器指针
 * @return 重置成功返回true，失败返回false
 */
bool F_linked_list_iterator_reset(Stru_LinkedListIterator_t* iterator);

/**
 * @brief 检查是否有下一个元素
 * 
 * 检查迭代器当前位置之后是否还有元素。
 * 
 * @param iterator 迭代器指针
 * @return 有下一个元素返回true，否则返回false
 */
bool F_linked_list_iterator_has_next(Stru_LinkedListIterator_t* iterator);

/**
 * @brief 获取下一个元素
 * 
 * 移动到下一个元素并返回其数据。
 * 
 * @param iterator 迭代器指针
 * @return 指向下一个元素数据的指针，没有下一个元素返回NULL
 */
void* F_linked_list_iterator_next(Stru_LinkedListIterator_t* iterator);

/**
 * @brief 获取当前元素
 * 
 * 返回迭代器当前指向的元素数据。
 * 
 * @param iterator 迭代器指针
 * @return 指向当前元素数据的指针，迭代器无效返回NULL
 */
void* F_linked_list_iterator_current(Stru_LinkedListIterator_t* iterator);

/**
 * @brief 获取当前位置
 * 
 * 返回迭代器当前在链表中的位置。
 * 
 * @param iterator 迭代器指针
 * @return 当前位置（0-based），迭代器无效返回-1
 */
int64_t F_linked_list_iterator_position(Stru_LinkedListIterator_t* iterator);

/**
 * @brief 检查迭代器是否有效
 * 
 * 检查迭代器是否处于有效状态。
 * 
 * @param iterator 迭代器指针
 * @return 迭代器有效返回true，否则返回false
 */
bool F_linked_list_iterator_is_valid(Stru_LinkedListIterator_t* iterator);

/**
 * @brief 在当前位置插入元素
 * 
 * 在迭代器当前位置插入一个新元素。
 * 
 * @param iterator 迭代器指针
 * @param data 要插入的数据指针
 * @return 插入成功返回true，失败返回false
 */
bool F_linked_list_iterator_insert(Stru_LinkedListIterator_t* iterator, void* data);

/**
 * @brief 移除当前元素
 * 
 * 移除迭代器当前指向的元素。
 * 
 * @param iterator 迭代器指针
 * @param out_data 输出参数，用于接收移除的数据（可为NULL）
 * @return 移除成功返回true，失败返回false
 */
bool F_linked_list_iterator_remove(Stru_LinkedListIterator_t* iterator, void* out_data);

/**
 * @brief 查找元素并创建迭代器
 * 
 * 查找指定元素并创建指向该元素的迭代器。
 * 
 * @param list 链表指针
 * @param data 要查找的数据指针
 * @param compare 比较函数，返回0表示相等
 * @return 指向找到元素的迭代器，未找到返回NULL
 */
Stru_LinkedListIterator_t* F_linked_list_find_iterator(Stru_LinkedList_t* list, 
                                                      void* data, 
                                                      int (*compare)(void*, void*));

#ifdef __cplusplus
}
#endif

#endif // CN_LINKED_LIST_ITERATOR_H
