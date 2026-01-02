/******************************************************************************
 * 文件名: CN_linked_list_iterator.c
 * 功能: CN_Language链表容器实现 - 迭代器支持
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现链表迭代器
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_linked_list_internal.h"
#include <stdlib.h>

// ============================================================================
// 公共API实现 - 迭代器支持
// ============================================================================

/**
 * @brief 创建链表迭代器
 */
Stru_CN_LinkedListIterator_t* CN_linked_list_iterator_create(
    Stru_CN_LinkedList_t* list)
{
    if (list == NULL)
    {
        return NULL;
    }
    
    Stru_CN_LinkedListIterator_t* iterator = 
        (Stru_CN_LinkedListIterator_t*)malloc(sizeof(Stru_CN_LinkedListIterator_t));
    
    if (iterator == NULL)
    {
        return NULL;
    }
    
    iterator->list = list;
    iterator->current_node = list->head;
    iterator->current_index = 0;
    
    return iterator;
}

/**
 * @brief 销毁迭代器
 */
void CN_linked_list_iterator_destroy(Stru_CN_LinkedListIterator_t* iterator)
{
    if (iterator != NULL)
    {
        free(iterator);
    }
}

/**
 * @brief 重置迭代器到开始位置
 */
void CN_linked_list_iterator_reset(Stru_CN_LinkedListIterator_t* iterator)
{
    if (iterator != NULL && iterator->list != NULL)
    {
        iterator->current_node = iterator->list->head;
        iterator->current_index = 0;
    }
}

/**
 * @brief 检查迭代器是否有下一个元素
 */
bool CN_linked_list_iterator_has_next(const Stru_CN_LinkedListIterator_t* iterator)
{
    if (iterator == NULL || iterator->current_node == NULL)
    {
        return false;
    }
    
    return iterator->current_node->next != NULL;
}

/**
 * @brief 获取下一个元素
 */
void* CN_linked_list_iterator_next(Stru_CN_LinkedListIterator_t* iterator)
{
    if (iterator == NULL || iterator->current_node == NULL)
    {
        return NULL;
    }
    
    // 移动到下一个节点
    iterator->current_node = iterator->current_node->next;
    iterator->current_index++;
    
    // 返回当前节点的数据（如果存在）
    return iterator->current_node != NULL ? iterator->current_node->data : NULL;
}

/**
 * @brief 获取当前元素
 */
void* CN_linked_list_iterator_current(const Stru_CN_LinkedListIterator_t* iterator)
{
    if (iterator == NULL || iterator->current_node == NULL)
    {
        return NULL;
    }
    
    return iterator->current_node->data;
}

/**
 * @brief 检查迭代器是否有前一个元素（仅双向链表）
 */
bool CN_linked_list_iterator_has_prev(const Stru_CN_LinkedListIterator_t* iterator)
{
    if (iterator == NULL || iterator->current_node == NULL || iterator->list == NULL)
    {
        return false;
    }
    
    // 仅双向链表支持前向遍历
    if (iterator->list->list_type != Eum_LINKED_LIST_DOUBLY)
    {
        return false;
    }
    
    return iterator->current_node->prev != NULL;
}

/**
 * @brief 获取前一个元素（仅双向链表）
 */
void* CN_linked_list_iterator_prev(Stru_CN_LinkedListIterator_t* iterator)
{
    if (iterator == NULL || iterator->current_node == NULL || iterator->list == NULL)
    {
        return NULL;
    }
    
    // 仅双向链表支持前向遍历
    if (iterator->list->list_type != Eum_LINKED_LIST_DOUBLY)
    {
        return NULL;
    }
    
    // 移动到前一个节点
    iterator->current_node = iterator->current_node->prev;
    
    // 更新索引（如果可能）
    if (iterator->current_node != NULL && iterator->current_index > 0)
    {
        iterator->current_index--;
    }
    
    // 返回当前节点的数据（如果存在）
    return iterator->current_node != NULL ? iterator->current_node->data : NULL;
}
