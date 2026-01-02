/******************************************************************************
 * 文件名: CN_linked_list.h
 * 功能: CN_Language链表容器
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，定义链表接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_LINKED_LIST_H
#define CN_LINKED_LIST_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief 链表节点结构体（不透明类型）
 */
typedef struct Stru_CN_ListNode_t Stru_CN_ListNode_t;

/**
 * @brief 链表结构体（不透明类型）
 */
typedef struct Stru_CN_LinkedList_t Stru_CN_LinkedList_t;

/**
 * @brief 链表迭代器结构体
 */
typedef struct Stru_CN_LinkedListIterator_t
{
    Stru_CN_LinkedList_t* list;        /**< 关联的链表 */
    Stru_CN_ListNode_t* current_node;  /**< 当前节点 */
    size_t current_index;              /**< 当前索引 */
} Stru_CN_LinkedListIterator_t;

/**
 * @brief 链表类型枚举
 */
typedef enum Eum_CN_LinkedListType_t
{
    Eum_LINKED_LIST_SINGLY = 0,    /**< 单向链表 */
    Eum_LINKED_LIST_DOUBLY = 1     /**< 双向链表 */
} Eum_CN_LinkedListType_t;

/**
 * @brief 链表节点释放函数类型
 * 
 * @param element 要释放的元素
 */
typedef void (*CN_LinkedListFreeFunc)(void* element);

/**
 * @brief 链表节点复制函数类型
 * 
 * @param src 源元素
 * @return 新分配的元素副本
 */
typedef void* (*CN_LinkedListCopyFunc)(const void* src);

/**
 * @brief 链表节点比较函数类型
 * 
 * @param elem1 第一个元素
 * @param elem2 第二个元素
 * @return 比较结果：<0表示elem1<elem2，0表示相等，>0表示elem1>elem2
 */
typedef int (*CN_LinkedListCompareFunc)(const void* elem1, const void* elem2);

// ============================================================================
// 链表创建和销毁
// ============================================================================

/**
 * @brief 创建链表
 * 
 * @param list_type 链表类型（单向或双向）
 * @param element_size 元素大小（字节）
 * @return 新创建的链表，失败返回NULL
 */
Stru_CN_LinkedList_t* CN_linked_list_create(Eum_CN_LinkedListType_t list_type,
                                            size_t element_size);

/**
 * @brief 创建带自定义函数的链表
 * 
 * @param list_type 链表类型（单向或双向）
 * @param element_size 元素大小（字节）
 * @param compare_func 比较函数（可为NULL）
 * @param free_func 释放函数（可为NULL）
 * @param copy_func 复制函数（可为NULL）
 * @return 新创建的链表，失败返回NULL
 */
Stru_CN_LinkedList_t* CN_linked_list_create_custom(
    Eum_CN_LinkedListType_t list_type,
    size_t element_size,
    CN_LinkedListCompareFunc compare_func,
    CN_LinkedListFreeFunc free_func,
    CN_LinkedListCopyFunc copy_func);

/**
 * @brief 销毁链表
 * 
 * @param list 要销毁的链表
 */
void CN_linked_list_destroy(Stru_CN_LinkedList_t* list);

/**
 * @brief 清空链表（移除所有元素）
 * 
 * @param list 要清空的链表
 */
void CN_linked_list_clear(Stru_CN_LinkedList_t* list);

// ============================================================================
// 链表属性查询
// ============================================================================

/**
 * @brief 获取链表长度
 * 
 * @param list 链表
 * @return 链表长度（元素数量）
 */
size_t CN_linked_list_length(const Stru_CN_LinkedList_t* list);

/**
 * @brief 检查链表是否为空
 * 
 * @param list 链表
 * @return 如果链表为空返回true，否则返回false
 */
bool CN_linked_list_is_empty(const Stru_CN_LinkedList_t* list);

/**
 * @brief 获取链表类型
 * 
 * @param list 链表
 * @return 链表类型
 */
Eum_CN_LinkedListType_t CN_linked_list_type(const Stru_CN_LinkedList_t* list);

/**
 * @brief 获取元素大小
 * 
 * @param list 链表
 * @return 元素大小（字节）
 */
size_t CN_linked_list_element_size(const Stru_CN_LinkedList_t* list);

// ============================================================================
// 元素访问和修改
// ============================================================================

/**
 * @brief 获取第一个元素
 * 
 * @param list 链表
 * @return 第一个元素的指针，链表为空返回NULL
 */
void* CN_linked_list_first(const Stru_CN_LinkedList_t* list);

/**
 * @brief 获取最后一个元素
 * 
 * @param list 链表
 * @return 最后一个元素的指针，链表为空返回NULL
 */
void* CN_linked_list_last(const Stru_CN_LinkedList_t* list);

/**
 * @brief 获取指定位置的元素
 * 
 * @param list 链表
 * @param index 元素索引（0-based）
 * @return 元素的指针，索引无效返回NULL
 */
void* CN_linked_list_get(const Stru_CN_LinkedList_t* list, size_t index);

/**
 * @brief 设置指定位置的元素值
 * 
 * @param list 链表
 * @param index 元素索引
 * @param element 元素值指针
 * @return 设置成功返回true，失败返回false
 */
bool CN_linked_list_set(Stru_CN_LinkedList_t* list, size_t index, 
                        const void* element);

/**
 * @brief 在链表头部添加元素
 * 
 * @param list 链表
 * @param element 要添加的元素
 * @return 添加成功返回true，失败返回false
 */
bool CN_linked_list_prepend(Stru_CN_LinkedList_t* list, const void* element);

/**
 * @brief 在链表尾部添加元素
 * 
 * @param list 链表
 * @param element 要添加的元素
 * @return 添加成功返回true，失败返回false
 */
bool CN_linked_list_append(Stru_CN_LinkedList_t* list, const void* element);

/**
 * @brief 在指定位置插入元素
 * 
 * @param list 链表
 * @param index 插入位置
 * @param element 要插入的元素
 * @return 插入成功返回true，失败返回false
 */
bool CN_linked_list_insert(Stru_CN_LinkedList_t* list, size_t index, 
                           const void* element);

/**
 * @brief 移除链表头部元素
 * 
 * @param list 链表
 * @param element 输出参数，接收移除的元素值（可为NULL）
 * @return 移除成功返回true，失败返回false
 */
bool CN_linked_list_remove_first(Stru_CN_LinkedList_t* list, void* element);

/**
 * @brief 移除链表尾部元素
 * 
 * @param list 链表
 * @param element 输出参数，接收移除的元素值（可为NULL）
 * @return 移除成功返回true，失败返回false
 */
bool CN_linked_list_remove_last(Stru_CN_LinkedList_t* list, void* element);

/**
 * @brief 移除指定位置的元素
 * 
 * @param list 链表
 * @param index 要移除的元素索引
 * @param element 输出参数，接收移除的元素值（可为NULL）
 * @return 移除成功返回true，失败返回false
 */
bool CN_linked_list_remove(Stru_CN_LinkedList_t* list, size_t index, 
                           void* element);

// ============================================================================
// 链表操作
// ============================================================================

/**
 * @brief 复制链表
 * 
 * @param src 源链表
 * @return 新创建的链表副本，失败返回NULL
 */
Stru_CN_LinkedList_t* CN_linked_list_copy(const Stru_CN_LinkedList_t* src);

/**
 * @brief 连接两个链表
 * 
 * @param list1 第一个链表
 * @param list2 第二个链表
 * @return 新创建的连接后的链表，失败返回NULL
 */
Stru_CN_LinkedList_t* CN_linked_list_concat(const Stru_CN_LinkedList_t* list1,
                                            const Stru_CN_LinkedList_t* list2);

/**
 * @brief 反转链表
 * 
 * @param list 链表
 * @return 反转成功返回true，失败返回false
 */
bool CN_linked_list_reverse(Stru_CN_LinkedList_t* list);

/**
 * @brief 查找元素
 * 
 * @param list 链表
 * @param element 要查找的元素
 * @return 元素索引，未找到返回SIZE_MAX
 */
size_t CN_linked_list_find(const Stru_CN_LinkedList_t* list, const void* element);

/**
 * @brief 查找元素（使用自定义比较函数）
 * 
 * @param list 链表
 * @param element 要查找的元素
 * @param compare_func 比较函数
 * @return 元素索引，未找到返回SIZE_MAX
 */
size_t CN_linked_list_find_custom(const Stru_CN_LinkedList_t* list, 
                                  const void* element,
                                  CN_LinkedListCompareFunc compare_func);

/**
 * @brief 检查链表是否包含元素
 * 
 * @param list 链表
 * @param element 要检查的元素
 * @return 如果包含返回true，否则返回false
 */
bool CN_linked_list_contains(const Stru_CN_LinkedList_t* list, const void* element);

// ============================================================================
// 迭代器支持
// ============================================================================

/**
 * @brief 创建链表迭代器
 * 
 * @param list 链表
 * @return 新创建的迭代器，失败返回NULL
 */
Stru_CN_LinkedListIterator_t* CN_linked_list_iterator_create(
    Stru_CN_LinkedList_t* list);

/**
 * @brief 销毁迭代器
 * 
 * @param iterator 要销毁的迭代器
 */
void CN_linked_list_iterator_destroy(Stru_CN_LinkedListIterator_t* iterator);

/**
 * @brief 重置迭代器到开始位置
 * 
 * @param iterator 迭代器
 */
void CN_linked_list_iterator_reset(Stru_CN_LinkedListIterator_t* iterator);

/**
 * @brief 检查迭代器是否有下一个元素
 * 
 * @param iterator 迭代器
 * @return 如果有下一个元素返回true，否则返回false
 */
bool CN_linked_list_iterator_has_next(const Stru_CN_LinkedListIterator_t* iterator);

/**
 * @brief 获取下一个元素
 * 
 * @param iterator 迭代器
 * @return 下一个元素的指针，没有更多元素返回NULL
 */
void* CN_linked_list_iterator_next(Stru_CN_LinkedListIterator_t* iterator);

/**
 * @brief 获取当前元素
 * 
 * @param iterator 迭代器
 * @return 当前元素的指针
 */
void* CN_linked_list_iterator_current(const Stru_CN_LinkedListIterator_t* iterator);

/**
 * @brief 检查迭代器是否有前一个元素（仅双向链表）
 * 
 * @param iterator 迭代器
 * @return 如果有前一个元素返回true，否则返回false
 */
bool CN_linked_list_iterator_has_prev(const Stru_CN_LinkedListIterator_t* iterator);

/**
 * @brief 获取前一个元素（仅双向链表）
 * 
 * @param iterator 迭代器
 * @return 前一个元素的指针，没有更多元素返回NULL
 */
void* CN_linked_list_iterator_prev(Stru_CN_LinkedListIterator_t* iterator);

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 比较两个链表
 * 
 * @param list1 第一个链表
 * @param list2 第二个链表
 * @return 如果链表相等返回true，否则返回false
 */
bool CN_linked_list_equal(const Stru_CN_LinkedList_t* list1, 
                          const Stru_CN_LinkedList_t* list2);

/**
 * @brief 转储链表信息到控制台（调试用）
 * 
 * @param list 链表
 */
void CN_linked_list_dump(const Stru_CN_LinkedList_t* list);

#endif // CN_LINKED_LIST_H
