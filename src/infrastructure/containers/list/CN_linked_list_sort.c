/**
 * @file CN_linked_list_sort.c
 * @brief 链表排序模块实现文件
 * 
 * 实现链表排序算法的核心功能。
 * 包括冒泡排序、插入排序、归并排序等算法。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_linked_list_sort.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief 内部函数：交换两个节点的数据
 * 
 * @param node1 第一个节点
 * @param node2 第二个节点
 * @param item_size 数据大小
 */
static void swap_node_data(Stru_ListNode_t* node1, Stru_ListNode_t* node2, size_t item_size)
{
    if (node1 == NULL || node2 == NULL || node1->data == NULL || node2->data == NULL)
    {
        return;
    }
    
    // 使用临时缓冲区交换数据
    void* temp = malloc(item_size);
    if (temp == NULL)
    {
        return;
    }
    
    memcpy(temp, node1->data, item_size);
    memcpy(node1->data, node2->data, item_size);
    memcpy(node2->data, temp, item_size);
    
    free(temp);
}

/**
 * @brief 内部函数：比较两个元素
 * 
 * @param a 第一个元素
 * @param b 第二个元素
 * @param compare 比较函数
 * @param ascending 是否升序
 * @return 比较结果
 */
static int compare_elements(void* a, void* b, int (*compare)(void*, void*), bool ascending)
{
    int result = compare(a, b);
    return ascending ? result : -result;
}

bool F_linked_list_bubble_sort(Stru_LinkedList_t* list,
                              int (*compare)(void*, void*),
                              bool ascending)
{
    if (list == NULL || compare == NULL || list->length < 2)
    {
        return false;
    }
    
    bool swapped;
    size_t i, j;
    
    for (i = 0; i < list->length - 1; i++)
    {
        swapped = false;
        Stru_ListNode_t* current = list->head;
        
        for (j = 0; j < list->length - i - 1; j++)
        {
            if (current == NULL || current->next == NULL)
            {
                break;
            }
            
            Stru_ListNode_t* next = current->next;
            
            // 比较当前节点和下一个节点
            if (compare_elements(current->data, next->data, compare, ascending) > 0)
            {
                swap_node_data(current, next, list->item_size);
                swapped = true;
            }
            
            current = current->next;
        }
        
        // 如果没有交换，说明已经排序完成
        if (!swapped)
        {
            break;
        }
    }
    
    return true;
}

bool F_linked_list_insertion_sort(Stru_LinkedList_t* list,
                                 int (*compare)(void*, void*),
                                 bool ascending)
{
    if (list == NULL || compare == NULL || list->length < 2)
    {
        return false;
    }
    
    if (list->head == NULL)
    {
        return false;
    }
    
    Stru_ListNode_t* sorted = NULL;
    Stru_ListNode_t* current = list->head;
    
    while (current != NULL)
    {
        Stru_ListNode_t* next = current->next;
        
        // 在已排序部分中找到插入位置
        if (sorted == NULL || 
            compare_elements(current->data, sorted->data, compare, ascending) <= 0)
        {
            // 插入到已排序部分的头部
            current->next = sorted;
            sorted = current;
        }
        else
        {
            // 在已排序部分中查找插入位置
            Stru_ListNode_t* search = sorted;
            while (search->next != NULL && 
                   compare_elements(current->data, search->next->data, compare, ascending) > 0)
            {
                search = search->next;
            }
            
            // 插入节点
            current->next = search->next;
            search->next = current;
        }
        
        current = next;
    }
    
    // 更新链表头
    list->head = sorted;
    
    // 更新链表尾
    Stru_ListNode_t* tail = sorted;
    while (tail != NULL && tail->next != NULL)
    {
        tail = tail->next;
    }
    list->tail = tail;
    
    return true;
}

/**
 * @brief 内部函数：分割链表
 * 
 * @param source 源链表头
 * @param front_ref 前部分链表头引用
 * @param back_ref 后部分链表头引用
 */
static void split_list(Stru_ListNode_t* source,
                      Stru_ListNode_t** front_ref,
                      Stru_ListNode_t** back_ref)
{
    if (source == NULL || source->next == NULL)
    {
        *front_ref = source;
        *back_ref = NULL;
        return;
    }
    
    Stru_ListNode_t* slow = source;
    Stru_ListNode_t* fast = source->next;
    
    // 快慢指针找到中点
    while (fast != NULL)
    {
        fast = fast->next;
        if (fast != NULL)
        {
            slow = slow->next;
            fast = fast->next;
        }
    }
    
    *front_ref = source;
    *back_ref = slow->next;
    slow->next = NULL;
}

/**
 * @brief 内部函数：合并两个已排序链表
 * 
 * @param a 第一个链表头
 * @param b 第二个链表头
 * @param compare 比较函数
 * @param ascending 是否升序
 * @return 合并后的链表头
 */
static Stru_ListNode_t* merge_lists(Stru_ListNode_t* a,
                                   Stru_ListNode_t* b,
                                   int (*compare)(void*, void*),
                                   bool ascending)
{
    if (a == NULL)
    {
        return b;
    }
    if (b == NULL)
    {
        return a;
    }
    
    Stru_ListNode_t* result = NULL;
    
    // 选择较小的节点作为结果头
    if (compare_elements(a->data, b->data, compare, ascending) <= 0)
    {
        result = a;
        result->next = merge_lists(a->next, b, compare, ascending);
    }
    else
    {
        result = b;
        result->next = merge_lists(a, b->next, compare, ascending);
    }
    
    return result;
}

/**
 * @brief 内部函数：归并排序递归实现
 * 
 * @param head_ref 链表头引用
 * @param compare 比较函数
 * @param ascending 是否升序
 */
static void merge_sort_recursive(Stru_ListNode_t** head_ref,
                                int (*compare)(void*, void*),
                                bool ascending)
{
    Stru_ListNode_t* head = *head_ref;
    if (head == NULL || head->next == NULL)
    {
        return;
    }
    
    Stru_ListNode_t* a;
    Stru_ListNode_t* b;
    
    // 分割链表
    split_list(head, &a, &b);
    
    // 递归排序两个子链表
    merge_sort_recursive(&a, compare, ascending);
    merge_sort_recursive(&b, compare, ascending);
    
    // 合并已排序的子链表
    *head_ref = merge_lists(a, b, compare, ascending);
}

bool F_linked_list_merge_sort(Stru_LinkedList_t* list,
                             int (*compare)(void*, void*),
                             bool ascending)
{
    if (list == NULL || compare == NULL || list->length < 2)
    {
        return false;
    }
    
    // 执行归并排序
    merge_sort_recursive(&(list->head), compare, ascending);
    
    // 更新链表尾
    Stru_ListNode_t* current = list->head;
    while (current != NULL && current->next != NULL)
    {
        current = current->next;
    }
    list->tail = current;
    
    return true;
}

bool F_linked_list_sort(Stru_LinkedList_t* list,
                       int (*compare)(void*, void*),
                       Eum_SortAlgorithm_t algorithm,
                       Eum_SortDirection_t direction)
{
    if (list == NULL || compare == NULL)
    {
        return false;
    }
    
    bool ascending = (direction == Eum_SORT_ASCENDING);
    
    switch (algorithm)
    {
        case Eum_SORT_BUBBLE:
            return F_linked_list_bubble_sort(list, compare, ascending);
            
        case Eum_SORT_INSERTION:
            return F_linked_list_insertion_sort(list, compare, ascending);
            
        case Eum_SORT_MERGE:
            return F_linked_list_merge_sort(list, compare, ascending);
            
        case Eum_SORT_QUICK:
        case Eum_SORT_SELECTION:
            // 暂未实现，使用归并排序作为默认
            return F_linked_list_merge_sort(list, compare, ascending);
            
        default:
            return false;
    }
}

bool F_linked_list_is_sorted(Stru_LinkedList_t* list,
                            int (*compare)(void*, void*),
                            bool ascending)
{
    if (list == NULL || compare == NULL || list->length < 2)
    {
        return true;
    }
    
    Stru_ListNode_t* current = list->head;
    while (current != NULL && current->next != NULL)
    {
        void* current_data = current->data;
        void* next_data = current->next->data;
        
        if (current_data == NULL || next_data == NULL)
        {
            return false;
        }
        
        int cmp = compare(current_data, next_data);
        
        if (ascending && cmp > 0)
        {
            return false;
        }
        else if (!ascending && cmp < 0)
        {
            return false;
        }
        
        current = current->next;
    }
    
    return true;
}

bool F_linked_list_reverse(Stru_LinkedList_t* list)
{
    if (list == NULL || list->length < 2)
    {
        return false;
    }
    
    Stru_ListNode_t* prev = NULL;
    Stru_ListNode_t* current = list->head;
    Stru_ListNode_t* next = NULL;
    
    list->tail = list->head; // 原头节点将成为尾节点
    
    while (current != NULL)
    {
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }
    
    list->head = prev; // 新的头节点是原尾节点
    
    return true;
}
