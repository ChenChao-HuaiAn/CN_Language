/******************************************************************************
 * 文件名: CN_linked_list.c
 * 功能: CN_Language链表容器实现 - 创建、销毁和基本操作
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现链表创建、销毁和基本操作
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_linked_list_internal.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 内部辅助函数实现
// ============================================================================

/**
 * @brief 创建新节点
 */
Stru_CN_ListNode_t* CN_linked_list_internal_create_node(const void* element, 
                                                        size_t element_size)
{
    if (element == NULL || element_size == 0)
    {
        return NULL;
    }
    
    Stru_CN_ListNode_t* node = (Stru_CN_ListNode_t*)malloc(sizeof(Stru_CN_ListNode_t));
    if (node == NULL)
    {
        return NULL;
    }
    
    node->data = malloc(element_size);
    if (node->data == NULL)
    {
        free(node);
        return NULL;
    }
    
    memcpy(node->data, element, element_size);
    node->next = NULL;
    node->prev = NULL;
    
    return node;
}

/**
 * @brief 销毁节点
 */
void CN_linked_list_internal_destroy_node(Stru_CN_ListNode_t* node,
                                          CN_LinkedListFreeFunc free_func)
{
    if (node == NULL)
    {
        return;
    }
    
    if (node->data != NULL)
    {
        if (free_func != NULL)
        {
            free_func(node->data);
        }
        else
        {
            free(node->data);
        }
    }
    
    free(node);
}

/**
 * @brief 复制节点
 */
Stru_CN_ListNode_t* CN_linked_list_internal_copy_node(const Stru_CN_ListNode_t* node,
                                                      CN_LinkedListCopyFunc copy_func,
                                                      size_t element_size)
{
    if (node == NULL || node->data == NULL || element_size == 0)
    {
        return NULL;
    }
    
    Stru_CN_ListNode_t* new_node = (Stru_CN_ListNode_t*)malloc(sizeof(Stru_CN_ListNode_t));
    if (new_node == NULL)
    {
        return NULL;
    }
    
    if (copy_func != NULL)
    {
        new_node->data = copy_func(node->data);
    }
    else
    {
        new_node->data = malloc(element_size);
        if (new_node->data != NULL)
        {
            memcpy(new_node->data, node->data, element_size);
        }
    }
    
    if (new_node->data == NULL)
    {
        free(new_node);
        return NULL;
    }
    
    new_node->next = NULL;
    new_node->prev = NULL;
    
    return new_node;
}

/**
 * @brief 获取指定位置的节点
 */
Stru_CN_ListNode_t* CN_linked_list_internal_get_node(
    const Stru_CN_LinkedList_t* list, size_t index)
{
    if (list == NULL || index >= list->length)
    {
        return NULL;
    }
    
    // 根据索引位置选择遍历方向（从头或从尾）
    if (index < list->length / 2)
    {
        // 从头开始正向遍历
        Stru_CN_ListNode_t* current = list->head;
        for (size_t i = 0; i < index && current != NULL; i++)
        {
            current = current->next;
        }
        return current;
    }
    else
    {
        // 从尾开始反向遍历（仅双向链表）
        if (list->list_type == Eum_LINKED_LIST_DOUBLY)
        {
            Stru_CN_ListNode_t* current = list->tail;
            for (size_t i = list->length - 1; i > index && current != NULL; i--)
            {
                current = current->prev;
            }
            return current;
        }
        else
        {
            // 单向链表只能从头遍历
            Stru_CN_ListNode_t* current = list->head;
            for (size_t i = 0; i < index && current != NULL; i++)
            {
                current = current->next;
            }
            return current;
        }
    }
}

/**
 * @brief 在节点后插入新节点
 */
bool CN_linked_list_internal_insert_after(Stru_CN_LinkedList_t* list,
                                          Stru_CN_ListNode_t* prev_node,
                                          Stru_CN_ListNode_t* new_node)
{
    if (list == NULL || new_node == NULL)
    {
        return false;
    }
    
    if (prev_node == NULL)
    {
        // 在头部插入
        new_node->next = list->head;
        new_node->prev = NULL;
        
        if (list->head != NULL)
        {
            if (list->list_type == Eum_LINKED_LIST_DOUBLY)
            {
                list->head->prev = new_node;
            }
        }
        
        list->head = new_node;
        
        if (list->tail == NULL)
        {
            list->tail = new_node;
        }
    }
    else
    {
        // 在prev_node后插入
        new_node->next = prev_node->next;
        new_node->prev = prev_node;
        
        if (prev_node->next != NULL)
        {
            if (list->list_type == Eum_LINKED_LIST_DOUBLY)
            {
                prev_node->next->prev = new_node;
            }
        }
        
        prev_node->next = new_node;
        
        if (prev_node == list->tail)
        {
            list->tail = new_node;
        }
    }
    
    list->length++;
    return true;
}

/**
 * @brief 移除节点
 */
bool CN_linked_list_internal_remove_node(Stru_CN_LinkedList_t* list,
                                         Stru_CN_ListNode_t* node,
                                         void* element)
{
    if (list == NULL || node == NULL)
    {
        return false;
    }
    
    // 复制元素值（如果需要）
    if (element != NULL && node->data != NULL)
    {
        memcpy(element, node->data, list->element_size);
    }
    
    // 更新前一个节点的next指针
    if (node->prev != NULL)
    {
        node->prev->next = node->next;
    }
    else
    {
        // 移除的是头节点
        list->head = node->next;
    }
    
    // 更新后一个节点的prev指针（仅双向链表）
    if (node->next != NULL)
    {
        if (list->list_type == Eum_LINKED_LIST_DOUBLY)
        {
            node->next->prev = node->prev;
        }
    }
    else
    {
        // 移除的是尾节点
        list->tail = node->prev;
    }
    
    // 销毁节点
    CN_linked_list_internal_destroy_node(node, list->free_func);
    list->length--;
    
    return true;
}

/**
 * @brief 获取链表比较函数
 */
CN_LinkedListCompareFunc CN_linked_list_internal_get_compare_func(
    const Stru_CN_LinkedList_t* list)
{
    return list != NULL ? list->compare_func : NULL;
}

/**
 * @brief 获取链表释放函数
 */
CN_LinkedListFreeFunc CN_linked_list_internal_get_free_func(
    const Stru_CN_LinkedList_t* list)
{
    return list != NULL ? list->free_func : NULL;
}

/**
 * @brief 获取链表复制函数
 */
CN_LinkedListCopyFunc CN_linked_list_internal_get_copy_func(
    const Stru_CN_LinkedList_t* list)
{
    return list != NULL ? list->copy_func : NULL;
}

/**
 * @brief 检查索引是否有效
 */
bool CN_linked_list_internal_is_index_valid(const Stru_CN_LinkedList_t* list,
                                            size_t index)
{
    return list != NULL && index < list->length;
}

// ============================================================================
// 公共API实现 - 创建和销毁
// ============================================================================

/**
 * @brief 创建链表
 */
Stru_CN_LinkedList_t* CN_linked_list_create(Eum_CN_LinkedListType_t list_type,
                                            size_t element_size)
{
    return CN_linked_list_create_custom(list_type, element_size, NULL, NULL, NULL);
}

/**
 * @brief 创建带自定义函数的链表
 */
Stru_CN_LinkedList_t* CN_linked_list_create_custom(
    Eum_CN_LinkedListType_t list_type,
    size_t element_size,
    CN_LinkedListCompareFunc compare_func,
    CN_LinkedListFreeFunc free_func,
    CN_LinkedListCopyFunc copy_func)
{
    if (element_size == 0)
    {
        return NULL;
    }
    
    Stru_CN_LinkedList_t* list = (Stru_CN_LinkedList_t*)malloc(sizeof(Stru_CN_LinkedList_t));
    if (list == NULL)
    {
        return NULL;
    }
    
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
    list->element_size = element_size;
    list->list_type = list_type;
    list->compare_func = compare_func;
    list->free_func = free_func;
    list->copy_func = copy_func;
    
    return list;
}

/**
 * @brief 销毁链表
 */
void CN_linked_list_destroy(Stru_CN_LinkedList_t* list)
{
    if (list == NULL)
    {
        return;
    }
    
    CN_linked_list_clear(list);
    free(list);
}

/**
 * @brief 清空链表
 */
void CN_linked_list_clear(Stru_CN_LinkedList_t* list)
{
    if (list == NULL)
    {
        return;
    }
    
    Stru_CN_ListNode_t* current = list->head;
    while (current != NULL)
    {
        Stru_CN_ListNode_t* next = current->next;
        CN_linked_list_internal_destroy_node(current, list->free_func);
        current = next;
    }
    
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
}

// ============================================================================
// 公共API实现 - 属性查询
// ============================================================================

/**
 * @brief 获取链表长度
 */
size_t CN_linked_list_length(const Stru_CN_LinkedList_t* list)
{
    return list != NULL ? list->length : 0;
}

/**
 * @brief 检查链表是否为空
 */
bool CN_linked_list_is_empty(const Stru_CN_LinkedList_t* list)
{
    return list == NULL || list->length == 0;
}

/**
 * @brief 获取链表类型
 */
Eum_CN_LinkedListType_t CN_linked_list_type(const Stru_CN_LinkedList_t* list)
{
    return list != NULL ? list->list_type : Eum_LINKED_LIST_SINGLY;
}

/**
 * @brief 获取元素大小
 */
size_t CN_linked_list_element_size(const Stru_CN_LinkedList_t* list)
{
    return list != NULL ? list->element_size : 0;
}
