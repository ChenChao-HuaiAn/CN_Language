/******************************************************************************
 * 文件名: CN_linked_list_operations.c
 * 功能: CN_Language链表容器实现 - 元素访问和修改操作
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现链表元素访问和修改操作
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_linked_list_internal.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 公共API实现 - 元素访问
// ============================================================================

/**
 * @brief 获取第一个元素
 */
void* CN_linked_list_first(const Stru_CN_LinkedList_t* list)
{
    if (list == NULL || list->head == NULL)
    {
        return NULL;
    }
    
    return list->head->data;
}

/**
 * @brief 获取最后一个元素
 */
void* CN_linked_list_last(const Stru_CN_LinkedList_t* list)
{
    if (list == NULL || list->tail == NULL)
    {
        return NULL;
    }
    
    return list->tail->data;
}

/**
 * @brief 获取指定位置的元素
 */
void* CN_linked_list_get(const Stru_CN_LinkedList_t* list, size_t index)
{
    Stru_CN_ListNode_t* node = CN_linked_list_internal_get_node(list, index);
    return node != NULL ? node->data : NULL;
}

/**
 * @brief 设置指定位置的元素值
 */
bool CN_linked_list_set(Stru_CN_LinkedList_t* list, size_t index, 
                        const void* element)
{
    if (list == NULL || element == NULL)
    {
        return false;
    }
    
    Stru_CN_ListNode_t* node = CN_linked_list_internal_get_node(list, index);
    if (node == NULL)
    {
        return false;
    }
    
    // 如果存在释放函数，先释放旧数据
    if (list->free_func != NULL && node->data != NULL)
    {
        list->free_func(node->data);
        node->data = NULL;
    }
    else if (node->data != NULL)
    {
        free(node->data);
        node->data = NULL;
    }
    
    // 分配新内存并复制数据
    node->data = malloc(list->element_size);
    if (node->data == NULL)
    {
        return false;
    }
    
    memcpy(node->data, element, list->element_size);
    return true;
}

// ============================================================================
// 公共API实现 - 元素添加
// ============================================================================

/**
 * @brief 在链表头部添加元素
 */
bool CN_linked_list_prepend(Stru_CN_LinkedList_t* list, const void* element)
{
    if (list == NULL || element == NULL)
    {
        return false;
    }
    
    Stru_CN_ListNode_t* new_node = CN_linked_list_internal_create_node(
        element, list->element_size);
    
    if (new_node == NULL)
    {
        return false;
    }
    
    return CN_linked_list_internal_insert_after(list, NULL, new_node);
}

/**
 * @brief 在链表尾部添加元素
 */
bool CN_linked_list_append(Stru_CN_LinkedList_t* list, const void* element)
{
    if (list == NULL || element == NULL)
    {
        return false;
    }
    
    Stru_CN_ListNode_t* new_node = CN_linked_list_internal_create_node(
        element, list->element_size);
    
    if (new_node == NULL)
    {
        return false;
    }
    
    // 如果链表为空，在头部插入
    if (list->head == NULL)
    {
        return CN_linked_list_internal_insert_after(list, NULL, new_node);
    }
    
    // 否则在尾部插入
    return CN_linked_list_internal_insert_after(list, list->tail, new_node);
}

/**
 * @brief 在指定位置插入元素
 */
bool CN_linked_list_insert(Stru_CN_LinkedList_t* list, size_t index, 
                           const void* element)
{
    if (list == NULL || element == NULL)
    {
        return false;
    }
    
    // 处理边界情况
    if (index == 0)
    {
        return CN_linked_list_prepend(list, element);
    }
    
    if (index >= list->length)
    {
        return CN_linked_list_append(list, element);
    }
    
    // 获取前一个节点
    Stru_CN_ListNode_t* prev_node = CN_linked_list_internal_get_node(list, index - 1);
    if (prev_node == NULL)
    {
        return false;
    }
    
    Stru_CN_ListNode_t* new_node = CN_linked_list_internal_create_node(
        element, list->element_size);
    
    if (new_node == NULL)
    {
        return false;
    }
    
    return CN_linked_list_internal_insert_after(list, prev_node, new_node);
}

// ============================================================================
// 公共API实现 - 元素移除
// ============================================================================

/**
 * @brief 移除链表头部元素
 */
bool CN_linked_list_remove_first(Stru_CN_LinkedList_t* list, void* element)
{
    if (list == NULL || list->head == NULL)
    {
        return false;
    }
    
    return CN_linked_list_internal_remove_node(list, list->head, element);
}

/**
 * @brief 移除链表尾部元素
 */
bool CN_linked_list_remove_last(Stru_CN_LinkedList_t* list, void* element)
{
    if (list == NULL || list->tail == NULL)
    {
        return false;
    }
    
    return CN_linked_list_internal_remove_node(list, list->tail, element);
}

/**
 * @brief 移除指定位置的元素
 */
bool CN_linked_list_remove(Stru_CN_LinkedList_t* list, size_t index, 
                           void* element)
{
    if (list == NULL)
    {
        return false;
    }
    
    // 处理边界情况
    if (index == 0)
    {
        return CN_linked_list_remove_first(list, element);
    }
    
    if (index == list->length - 1)
    {
        return CN_linked_list_remove_last(list, element);
    }
    
    // 获取要移除的节点
    Stru_CN_ListNode_t* node = CN_linked_list_internal_get_node(list, index);
    if (node == NULL)
    {
        return false;
    }
    
    return CN_linked_list_internal_remove_node(list, node, element);
}

// ============================================================================
// 公共API实现 - 链表操作
// ============================================================================

/**
 * @brief 复制链表
 */
Stru_CN_LinkedList_t* CN_linked_list_copy(const Stru_CN_LinkedList_t* src)
{
    if (src == NULL)
    {
        return NULL;
    }
    
    // 创建新链表
    Stru_CN_LinkedList_t* dest = CN_linked_list_create_custom(
        src->list_type,
        src->element_size,
        src->compare_func,
        src->free_func,
        src->copy_func);
    
    if (dest == NULL)
    {
        return NULL;
    }
    
    // 复制所有节点
    Stru_CN_ListNode_t* current = src->head;
    while (current != NULL)
    {
        Stru_CN_ListNode_t* new_node = CN_linked_list_internal_copy_node(
            current, src->copy_func, src->element_size);
        
        if (new_node == NULL)
        {
            CN_linked_list_destroy(dest);
            return NULL;
        }
        
        // 将新节点添加到目标链表
        if (dest->tail == NULL)
        {
            // 第一个节点
            dest->head = new_node;
            dest->tail = new_node;
        }
        else
        {
            // 在尾部添加
            dest->tail->next = new_node;
            if (dest->list_type == Eum_LINKED_LIST_DOUBLY)
            {
                new_node->prev = dest->tail;
            }
            dest->tail = new_node;
        }
        
        dest->length++;
        current = current->next;
    }
    
    return dest;
}

/**
 * @brief 连接两个链表
 */
Stru_CN_LinkedList_t* CN_linked_list_concat(const Stru_CN_LinkedList_t* list1,
                                            const Stru_CN_LinkedList_t* list2)
{
    if (list1 == NULL && list2 == NULL)
    {
        return NULL;
    }
    
    if (list1 == NULL)
    {
        return CN_linked_list_copy(list2);
    }
    
    if (list2 == NULL)
    {
        return CN_linked_list_copy(list1);
    }
    
    // 检查链表类型和元素大小是否兼容
    if (list1->list_type != list2->list_type || 
        list1->element_size != list2->element_size ||
        list1->compare_func != list2->compare_func ||
        list1->free_func != list2->free_func ||
        list1->copy_func != list2->copy_func)
    {
        return NULL;
    }
    
    // 复制第一个链表
    Stru_CN_LinkedList_t* result = CN_linked_list_copy(list1);
    if (result == NULL)
    {
        return NULL;
    }
    
    // 复制第二个链表的所有节点并添加到结果链表
    Stru_CN_ListNode_t* current = list2->head;
    while (current != NULL)
    {
        Stru_CN_ListNode_t* new_node = CN_linked_list_internal_copy_node(
            current, list2->copy_func, list2->element_size);
        
        if (new_node == NULL)
        {
            CN_linked_list_destroy(result);
            return NULL;
        }
        
        // 在尾部添加新节点
        if (result->tail == NULL)
        {
            result->head = new_node;
            result->tail = new_node;
        }
        else
        {
            result->tail->next = new_node;
            if (result->list_type == Eum_LINKED_LIST_DOUBLY)
            {
                new_node->prev = result->tail;
            }
            result->tail = new_node;
        }
        
        result->length++;
        current = current->next;
    }
    
    return result;
}

/**
 * @brief 反转链表
 */
bool CN_linked_list_reverse(Stru_CN_LinkedList_t* list)
{
    if (list == NULL || list->length <= 1)
    {
        return true;
    }
    
    Stru_CN_ListNode_t* prev = NULL;
    Stru_CN_ListNode_t* current = list->head;
    Stru_CN_ListNode_t* next = NULL;
    
    // 交换头尾指针
    Stru_CN_ListNode_t* old_head = list->head;
    list->head = list->tail;
    list->tail = old_head;
    
    // 反转所有节点的指针
    while (current != NULL)
    {
        next = current->next;
        
        // 反转当前节点的指针
        current->next = prev;
        if (list->list_type == Eum_LINKED_LIST_DOUBLY)
        {
            current->prev = next;
        }
        
        prev = current;
        current = next;
    }
    
    return true;
}

/**
 * @brief 查找元素
 */
size_t CN_linked_list_find(const Stru_CN_LinkedList_t* list, const void* element)
{
    return CN_linked_list_find_custom(list, element, list->compare_func);
}

/**
 * @brief 查找元素（使用自定义比较函数）
 */
size_t CN_linked_list_find_custom(const Stru_CN_LinkedList_t* list, 
                                  const void* element,
                                  CN_LinkedListCompareFunc compare_func)
{
    if (list == NULL || element == NULL)
    {
        return SIZE_MAX;
    }
    
    // 如果没有比较函数，使用memcmp
    if (compare_func == NULL)
    {
        compare_func = (CN_LinkedListCompareFunc)memcmp;
    }
    
    Stru_CN_ListNode_t* current = list->head;
    size_t index = 0;
    
    while (current != NULL)
    {
        if (compare_func(current->data, element) == 0)
        {
            return index;
        }
        
        current = current->next;
        index++;
    }
    
    return SIZE_MAX;
}

/**
 * @brief 检查链表是否包含元素
 */
bool CN_linked_list_contains(const Stru_CN_LinkedList_t* list, const void* element)
{
    return CN_linked_list_find(list, element) != SIZE_MAX;
}
