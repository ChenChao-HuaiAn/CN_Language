/******************************************************************************
 * 文件名: CN_linked_list_internal.h
 * 功能: CN_Language链表容器内部结构定义
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，定义链表内部结构
 * 版权: MIT许可证
 * 
 * 注意: 此文件仅供CN_linked_list模块内部使用，外部模块不应包含此文件
 ******************************************************************************/

#ifndef CN_LINKED_LIST_INTERNAL_H
#define CN_LINKED_LIST_INTERNAL_H

#include "CN_linked_list.h"

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief 链表节点结构体（完整定义，仅供内部使用）
 */
struct Stru_CN_ListNode_t
{
    void* data;                     /**< 节点数据指针 */
    Stru_CN_ListNode_t* next;       /**< 指向下一个节点的指针 */
    Stru_CN_ListNode_t* prev;       /**< 指向前一个节点的指针（仅双向链表） */
};

/**
 * @brief 链表结构体（完整定义，仅供内部使用）
 */
struct Stru_CN_LinkedList_t
{
    Stru_CN_ListNode_t* head;       /**< 链表头节点 */
    Stru_CN_ListNode_t* tail;       /**< 链表尾节点 */
    size_t length;                  /**< 链表长度（节点数量） */
    size_t element_size;            /**< 元素大小（字节） */
    Eum_CN_LinkedListType_t list_type; /**< 链表类型（单向/双向） */
    CN_LinkedListCompareFunc compare_func; /**< 比较函数 */
    CN_LinkedListFreeFunc free_func; /**< 释放函数 */
    CN_LinkedListCopyFunc copy_func; /**< 复制函数 */
};

// ============================================================================
// 内部辅助函数声明
// ============================================================================

/**
 * @brief 创建新节点
 * 
 * @param element 元素数据指针
 * @param element_size 元素大小
 * @return 新创建的节点，失败返回NULL
 */
Stru_CN_ListNode_t* CN_linked_list_internal_create_node(const void* element, 
                                                        size_t element_size);

/**
 * @brief 销毁节点
 * 
 * @param node 要销毁的节点
 * @param free_func 释放函数（可为NULL）
 */
void CN_linked_list_internal_destroy_node(Stru_CN_ListNode_t* node,
                                          CN_LinkedListFreeFunc free_func);

/**
 * @brief 复制节点
 * 
 * @param node 要复制的节点
 * @param copy_func 复制函数（可为NULL）
 * @param element_size 元素大小
 * @return 新创建的节点副本，失败返回NULL
 */
Stru_CN_ListNode_t* CN_linked_list_internal_copy_node(const Stru_CN_ListNode_t* node,
                                                      CN_LinkedListCopyFunc copy_func,
                                                      size_t element_size);

/**
 * @brief 获取指定位置的节点
 * 
 * @param list 链表
 * @param index 节点索引（0-based）
 * @return 节点指针，索引无效返回NULL
 */
Stru_CN_ListNode_t* CN_linked_list_internal_get_node(
    const Stru_CN_LinkedList_t* list, size_t index);

/**
 * @brief 在节点后插入新节点
 * 
 * @param list 链表
 * @param prev_node 前一个节点（如果为NULL，则在头部插入）
 * @param new_node 要插入的新节点
 * @return 插入成功返回true，失败返回false
 */
bool CN_linked_list_internal_insert_after(Stru_CN_LinkedList_t* list,
                                          Stru_CN_ListNode_t* prev_node,
                                          Stru_CN_ListNode_t* new_node);

/**
 * @brief 移除节点
 * 
 * @param list 链表
 * @param node 要移除的节点
 * @param element 输出参数，接收移除的元素值（可为NULL）
 * @return 移除成功返回true，失败返回false
 */
bool CN_linked_list_internal_remove_node(Stru_CN_LinkedList_t* list,
                                         Stru_CN_ListNode_t* node,
                                         void* element);

/**
 * @brief 获取链表比较函数
 * 
 * @param list 链表
 * @return 比较函数指针
 */
CN_LinkedListCompareFunc CN_linked_list_internal_get_compare_func(
    const Stru_CN_LinkedList_t* list);

/**
 * @brief 获取链表释放函数
 * 
 * @param list 链表
 * @return 释放函数指针
 */
CN_LinkedListFreeFunc CN_linked_list_internal_get_free_func(
    const Stru_CN_LinkedList_t* list);

/**
 * @brief 获取链表复制函数
 * 
 * @param list 链表
 * @return 复制函数指针
 */
CN_LinkedListCopyFunc CN_linked_list_internal_get_copy_func(
    const Stru_CN_LinkedList_t* list);

/**
 * @brief 检查索引是否有效
 * 
 * @param list 链表
 * @param index 要检查的索引
 * @return 索引有效返回true，否则返回false
 */
bool CN_linked_list_internal_is_index_valid(const Stru_CN_LinkedList_t* list,
                                            size_t index);

#endif // CN_LINKED_LIST_INTERNAL_H
