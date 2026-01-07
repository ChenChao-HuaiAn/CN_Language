/**
 * @file CN_linked_list_iterator.c
 * @brief 链表迭代器模块实现文件
 * 
 * 实现链表迭代器的核心功能。
 * 包括创建、销毁、遍历、插入、删除等操作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_linked_list_iterator.h"
#include <stdlib.h>
#include <string.h>

Stru_LinkedListIterator_t* F_create_linked_list_iterator(Stru_LinkedList_t* list)
{
    if (list == NULL)
    {
        return NULL;
    }
    
    // 分配迭代器结构体
    Stru_LinkedListIterator_t* iterator = 
        (Stru_LinkedListIterator_t*)malloc(sizeof(Stru_LinkedListIterator_t));
    if (iterator == NULL)
    {
        return NULL;
    }
    
    // 初始化字段
    iterator->list = list;
    iterator->current = list->head;
    iterator->position = 0;
    iterator->is_valid = (list->head != NULL);
    
    return iterator;
}

void F_destroy_linked_list_iterator(Stru_LinkedListIterator_t* iterator)
{
    if (iterator == NULL)
    {
        return;
    }
    
    free(iterator);
}

bool F_linked_list_iterator_reset(Stru_LinkedListIterator_t* iterator)
{
    if (iterator == NULL || iterator->list == NULL)
    {
        return false;
    }
    
    iterator->current = iterator->list->head;
    iterator->position = 0;
    iterator->is_valid = (iterator->list->head != NULL);
    
    return true;
}

bool F_linked_list_iterator_has_next(Stru_LinkedListIterator_t* iterator)
{
    if (iterator == NULL || !iterator->is_valid)
    {
        return false;
    }
    
    return iterator->current != NULL && iterator->current->next != NULL;
}

void* F_linked_list_iterator_next(Stru_LinkedListIterator_t* iterator)
{
    if (iterator == NULL || !iterator->is_valid || iterator->current == NULL)
    {
        return NULL;
    }
    
    // 移动到下一个节点
    iterator->current = iterator->current->next;
    
    if (iterator->current != NULL)
    {
        iterator->position++;
        return iterator->current->data;
    }
    else
    {
        iterator->is_valid = false;
        return NULL;
    }
}

void* F_linked_list_iterator_current(Stru_LinkedListIterator_t* iterator)
{
    if (iterator == NULL || !iterator->is_valid || iterator->current == NULL)
    {
        return NULL;
    }
    
    return iterator->current->data;
}

int64_t F_linked_list_iterator_position(Stru_LinkedListIterator_t* iterator)
{
    if (iterator == NULL || !iterator->is_valid)
    {
        return -1;
    }
    
    return (int64_t)iterator->position;
}

bool F_linked_list_iterator_is_valid(Stru_LinkedListIterator_t* iterator)
{
    if (iterator == NULL)
    {
        return false;
    }
    
    return iterator->is_valid;
}

bool F_linked_list_iterator_insert(Stru_LinkedListIterator_t* iterator, void* data)
{
    if (iterator == NULL || iterator->list == NULL || data == NULL)
    {
        return false;
    }
    
    // 如果迭代器无效或指向头部，在头部插入
    if (!iterator->is_valid || iterator->position == 0)
    {
        bool result = F_linked_list_prepend(iterator->list, data);
        if (result)
        {
            // 在头部插入后，迭代器位置应该增加1
            // 因为新节点插入在当前位置之前
            iterator->position++;
        }
        return result;
    }
    
    // 创建新节点
    Stru_ListNode_t* new_node = malloc(sizeof(Stru_ListNode_t));
    if (new_node == NULL)
    {
        return false;
    }
    
    new_node->data = malloc(iterator->list->item_size);
    if (new_node->data == NULL)
    {
        free(new_node);
        return false;
    }
    
    memcpy(new_node->data, data, iterator->list->item_size);
    
    // 找到当前节点的前一个节点
    Stru_ListNode_t* prev = iterator->list->head;
    for (size_t i = 0; i < iterator->position - 1 && prev != NULL; i++)
    {
        prev = prev->next;
    }
    
    if (prev == NULL)
    {
        free(new_node->data);
        free(new_node);
        return false;
    }
    
    // 插入新节点
    new_node->next = prev->next;
    prev->next = new_node;
    
    // 如果插入在尾部，更新尾节点
    if (new_node->next == NULL)
    {
        iterator->list->tail = new_node;
    }
    
    iterator->list->length++;
    iterator->position++; // 位置后移，因为插入了新节点
    
    return true;
}

bool F_linked_list_iterator_remove(Stru_LinkedListIterator_t* iterator, void* out_data)
{
    if (iterator == NULL || iterator->list == NULL || !iterator->is_valid || 
        iterator->current == NULL)
    {
        return false;
    }
    
    // 复制数据到输出参数（如果提供）
    if (out_data != NULL && iterator->current->data != NULL)
    {
        memcpy(out_data, iterator->current->data, iterator->list->item_size);
    }
    
    // 保存要移除的节点
    Stru_ListNode_t* node_to_remove = iterator->current;
    
    // 找到前一个节点
    Stru_ListNode_t* prev = NULL;
    if (iterator->position > 0)
    {
        prev = iterator->list->head;
        for (size_t i = 0; i < iterator->position - 1 && prev != NULL; i++)
        {
            prev = prev->next;
        }
    }
    
    // 更新链表连接
    if (prev == NULL)
    {
        // 移除的是头节点
        iterator->list->head = node_to_remove->next;
        if (iterator->list->head == NULL)
        {
            iterator->list->tail = NULL;
        }
    }
    else
    {
        prev->next = node_to_remove->next;
        if (prev->next == NULL)
        {
            iterator->list->tail = prev;
        }
    }
    
    // 更新迭代器状态
    iterator->current = node_to_remove->next;
    if (iterator->current == NULL)
    {
        iterator->is_valid = false;
    }
    
    // 释放节点内存
    if (node_to_remove->data != NULL)
    {
        free(node_to_remove->data);
    }
    free(node_to_remove);
    
    iterator->list->length--;
    
    return true;
}

Stru_LinkedListIterator_t* F_linked_list_find_iterator(Stru_LinkedList_t* list, 
                                                      void* data, 
                                                      int (*compare)(void*, void*))
{
    if (list == NULL || data == NULL || compare == NULL)
    {
        return NULL;
    }
    
    // 创建迭代器
    Stru_LinkedListIterator_t* iterator = F_create_linked_list_iterator(list);
    if (iterator == NULL)
    {
        return NULL;
    }
    
    // 遍历查找
    while (iterator->is_valid && iterator->current != NULL)
    {
        if (iterator->current->data != NULL && 
            compare(iterator->current->data, data) == 0)
        {
            return iterator;
        }
        
        // 移动到下一个节点
        iterator->current = iterator->current->next;
        if (iterator->current != NULL)
        {
            iterator->position++;
        }
        else
        {
            iterator->is_valid = false;
        }
    }
    
    // 未找到，销毁迭代器
    F_destroy_linked_list_iterator(iterator);
    return NULL;
}
