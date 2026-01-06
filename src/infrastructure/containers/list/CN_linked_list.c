/**
 * @file CN_linked_list.c
 * @brief 链表模块实现文件
 * 
 * 实现链表数据结构的核心功能。
 * 包括创建、销毁、插入、删除、遍历等操作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_linked_list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief 内部函数：创建新节点
 * 
 * 分配并初始化一个新的链表节点。
 * 
 * @param data 节点数据指针
 * @param item_size 数据大小
 * @return 指向新节点的指针，失败返回NULL
 */
static Stru_ListNode_t* create_node(void* data, size_t item_size)
{
    if (data == NULL)
    {
        return NULL;
    }
    
    // 分配节点结构体
    Stru_ListNode_t* node = (Stru_ListNode_t*)malloc(sizeof(Stru_ListNode_t));
    if (node == NULL)
    {
        return NULL;
    }
    
    // 分配节点数据内存
    node->data = malloc(item_size);
    if (node->data == NULL)
    {
        free(node);
        return NULL;
    }
    
    // 复制数据
    memcpy(node->data, data, item_size);
    node->next = NULL;
    
    return node;
}

/**
 * @brief 内部函数：释放节点
 * 
 * 释放节点占用的内存。
 * 
 * @param node 要释放的节点指针
 */
static void free_node(Stru_ListNode_t* node)
{
    if (node == NULL)
    {
        return;
    }
    
    if (node->data != NULL)
    {
        free(node->data);
        node->data = NULL;
    }
    
    free(node);
}

/**
 * @brief 内部函数：获取指定位置的节点
 * 
 * 遍历链表获取指定位置的节点。
 * 
 * @param list 链表指针
 * @param position 节点位置（0-based）
 * @return 指向节点的指针，位置无效返回NULL
 */
static Stru_ListNode_t* get_node_at(Stru_LinkedList_t* list, size_t position)
{
    if (list == NULL || position >= list->length)
    {
        return NULL;
    }
    
    Stru_ListNode_t* current = list->head;
    for (size_t i = 0; i < position && current != NULL; i++)
    {
        current = current->next;
    }
    
    return current;
}

Stru_LinkedList_t* F_create_linked_list(size_t item_size)
{
    if (item_size == 0)
    {
        return NULL;
    }
    
    // 分配链表结构体
    Stru_LinkedList_t* list = (Stru_LinkedList_t*)malloc(sizeof(Stru_LinkedList_t));
    if (list == NULL)
    {
        return NULL;
    }
    
    // 初始化字段
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
    list->item_size = item_size;
    
    return list;
}

void F_destroy_linked_list(Stru_LinkedList_t* list)
{
    if (list == NULL)
    {
        return;
    }
    
    // 释放所有节点
    Stru_ListNode_t* current = list->head;
    while (current != NULL)
    {
        Stru_ListNode_t* next = current->next;
        free_node(current);
        current = next;
    }
    
    // 释放链表结构体
    free(list);
}

bool F_linked_list_prepend(Stru_LinkedList_t* list, void* data)
{
    if (list == NULL || data == NULL)
    {
        return false;
    }
    
    // 创建新节点
    Stru_ListNode_t* new_node = create_node(data, list->item_size);
    if (new_node == NULL)
    {
        return false;
    }
    
    // 插入到头部
    new_node->next = list->head;
    list->head = new_node;
    
    // 如果链表为空，更新尾节点
    if (list->tail == NULL)
    {
        list->tail = new_node;
    }
    
    list->length++;
    return true;
}

bool F_linked_list_append(Stru_LinkedList_t* list, void* data)
{
    if (list == NULL || data == NULL)
    {
        return false;
    }
    
    // 创建新节点
    Stru_ListNode_t* new_node = create_node(data, list->item_size);
    if (new_node == NULL)
    {
        return false;
    }
    
    // 插入到尾部
    if (list->tail == NULL)
    {
        // 链表为空
        list->head = new_node;
        list->tail = new_node;
    }
    else
    {
        list->tail->next = new_node;
        list->tail = new_node;
    }
    
    list->length++;
    return true;
}

bool F_linked_list_insert(Stru_LinkedList_t* list, void* data, size_t position)
{
    if (list == NULL || data == NULL)
    {
        return false;
    }
    
    // 特殊处理头部插入
    if (position == 0)
    {
        return F_linked_list_prepend(list, data);
    }
    
    // 特殊处理尾部插入
    if (position >= list->length)
    {
        return F_linked_list_append(list, data);
    }
    
    // 创建新节点
    Stru_ListNode_t* new_node = create_node(data, list->item_size);
    if (new_node == NULL)
    {
        return false;
    }
    
    // 找到插入位置的前一个节点
    Stru_ListNode_t* prev = get_node_at(list, position - 1);
    if (prev == NULL)
    {
        free_node(new_node);
        return false;
    }
    
    // 插入节点
    new_node->next = prev->next;
    prev->next = new_node;
    
    list->length++;
    return true;
}

bool F_linked_list_remove_first(Stru_LinkedList_t* list, void* out_data)
{
    if (list == NULL || list->head == NULL)
    {
        return false;
    }
    
    Stru_ListNode_t* node_to_remove = list->head;
    
    // 复制数据到输出参数（如果提供）
    if (out_data != NULL && node_to_remove->data != NULL)
    {
        memcpy(out_data, node_to_remove->data, list->item_size);
    }
    
    // 更新头节点
    list->head = node_to_remove->next;
    
    // 如果移除后链表为空，更新尾节点
    if (list->head == NULL)
    {
        list->tail = NULL;
    }
    
    // 释放节点
    free_node(node_to_remove);
    list->length--;
    
    return true;
}

bool F_linked_list_remove_last(Stru_LinkedList_t* list, void* out_data)
{
    if (list == NULL || list->head == NULL)
    {
        return false;
    }
    
    // 如果只有一个节点
    if (list->head == list->tail)
    {
        return F_linked_list_remove_first(list, out_data);
    }
    
    // 找到倒数第二个节点
    Stru_ListNode_t* current = list->head;
    while (current != NULL && current->next != list->tail)
    {
        current = current->next;
    }
    
    if (current == NULL)
    {
        return false;
    }
    
    Stru_ListNode_t* node_to_remove = list->tail;
    
    // 复制数据到输出参数（如果提供）
    if (out_data != NULL && node_to_remove->data != NULL)
    {
        memcpy(out_data, node_to_remove->data, list->item_size);
    }
    
    // 更新尾节点
    current->next = NULL;
    list->tail = current;
    
    // 释放节点
    free_node(node_to_remove);
    list->length--;
    
    return true;
}

bool F_linked_list_remove_at(Stru_LinkedList_t* list, size_t position, void* out_data)
{
    if (list == NULL || position >= list->length)
    {
        return false;
    }
    
    // 特殊处理头部移除
    if (position == 0)
    {
        return F_linked_list_remove_first(list, out_data);
    }
    
    // 特殊处理尾部移除
    if (position == list->length - 1)
    {
        return F_linked_list_remove_last(list, out_data);
    }
    
    // 找到要移除节点的前一个节点
    Stru_ListNode_t* prev = get_node_at(list, position - 1);
    if (prev == NULL || prev->next == NULL)
    {
        return false;
    }
    
    Stru_ListNode_t* node_to_remove = prev->next;
    
    // 复制数据到输出参数（如果提供）
    if (out_data != NULL && node_to_remove->data != NULL)
    {
        memcpy(out_data, node_to_remove->data, list->item_size);
    }
    
    // 从链表中移除节点
    prev->next = node_to_remove->next;
    
    // 释放节点
    free_node(node_to_remove);
    list->length--;
    
    return true;
}

size_t F_linked_list_length(Stru_LinkedList_t* list)
{
    if (list == NULL)
    {
        return 0;
    }
    
    return list->length;
}

void* F_linked_list_get(Stru_LinkedList_t* list, size_t position)
{
    Stru_ListNode_t* node = get_node_at(list, position);
    if (node == NULL)
    {
        return NULL;
    }
    
    return node->data;
}

int64_t F_linked_list_find(Stru_LinkedList_t* list, void* data, 
                          int (*compare)(void*, void*))
{
    if (list == NULL || data == NULL || compare == NULL)
    {
        return -1;
    }
    
    Stru_ListNode_t* current = list->head;
    size_t position = 0;
    
    while (current != NULL)
    {
        if (current->data != NULL && compare(current->data, data) == 0)
        {
            return (int64_t)position;
        }
        
        current = current->next;
        position++;
    }
    
    return -1;
}

void F_linked_list_clear(Stru_LinkedList_t* list)
{
    if (list == NULL)
    {
        return;
    }
    
    // 释放所有节点
    Stru_ListNode_t* current = list->head;
    while (current != NULL)
    {
        Stru_ListNode_t* next = current->next;
        free_node(current);
        current = next;
    }
    
    // 重置链表状态
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
}

void F_linked_list_foreach(Stru_LinkedList_t* list,
                          void (*callback)(void* data, void* user_data),
                          void* user_data)
{
    if (list == NULL || callback == NULL)
    {
        return;
    }
    
    Stru_ListNode_t* current = list->head;
    while (current != NULL)
    {
        callback(current->data, user_data);
        current = current->next;
    }
}

bool F_linked_list_is_empty(Stru_LinkedList_t* list)
{
    if (list == NULL)
    {
        return true;
    }
    
    return list->length == 0;
}
