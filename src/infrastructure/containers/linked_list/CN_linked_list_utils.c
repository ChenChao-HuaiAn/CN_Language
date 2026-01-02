/******************************************************************************
 * 文件名: CN_linked_list_utils.c
 * 功能: CN_Language链表容器实现 - 工具函数
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现链表工具函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_linked_list_internal.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// 公共API实现 - 工具函数
// ============================================================================

/**
 * @brief 比较两个链表
 */
bool CN_linked_list_equal(const Stru_CN_LinkedList_t* list1, 
                          const Stru_CN_LinkedList_t* list2)
{
    if (list1 == NULL && list2 == NULL)
    {
        return true;
    }
    
    if (list1 == NULL || list2 == NULL)
    {
        return false;
    }
    
    // 检查基本属性
    if (list1->length != list2->length ||
        list1->element_size != list2->element_size ||
        list1->list_type != list2->list_type)
    {
        return false;
    }
    
    // 如果两个链表都有比较函数，使用它们
    CN_LinkedListCompareFunc compare_func = list1->compare_func;
    if (compare_func == NULL)
    {
        compare_func = list2->compare_func;
    }
    
    // 如果没有比较函数，使用memcmp
    if (compare_func == NULL)
    {
        compare_func = (CN_LinkedListCompareFunc)memcmp;
    }
    
    // 比较所有元素
    Stru_CN_ListNode_t* node1 = list1->head;
    Stru_CN_ListNode_t* node2 = list2->head;
    
    while (node1 != NULL && node2 != NULL)
    {
        if (compare_func(node1->data, node2->data) != 0)
        {
            return false;
        }
        
        node1 = node1->next;
        node2 = node2->next;
    }
    
    return true;
}

/**
 * @brief 转储链表信息到控制台（调试用）
 */
void CN_linked_list_dump(const Stru_CN_LinkedList_t* list)
{
    if (list == NULL)
    {
        printf("链表: NULL\n");
        return;
    }
    
    printf("链表信息:\n");
    printf("  类型: %s\n", 
           list->list_type == Eum_LINKED_LIST_SINGLY ? "单向链表" : "双向链表");
    printf("  长度: %zu\n", list->length);
    printf("  元素大小: %zu 字节\n", list->element_size);
    printf("  头节点: %p\n", (void*)list->head);
    printf("  尾节点: %p\n", (void*)list->tail);
    
    if (list->compare_func != NULL)
    {
        printf("  比较函数: 已设置\n");
    }
    
    if (list->free_func != NULL)
    {
        printf("  释放函数: 已设置\n");
    }
    
    if (list->copy_func != NULL)
    {
        printf("  复制函数: 已设置\n");
    }
    
    printf("  元素列表:\n");
    
    Stru_CN_ListNode_t* current = list->head;
    size_t index = 0;
    
    while (current != NULL)
    {
        printf("    [%zu] 节点: %p, 数据: %p", 
               index, (void*)current, current->data);
        
        if (current->prev != NULL)
        {
            printf(", 前驱: %p", (void*)current->prev);
        }
        
        if (current->next != NULL)
        {
            printf(", 后继: %p", (void*)current->next);
        }
        
        printf("\n");
        
        // 尝试打印元素值（仅适用于基本类型）
        if (current->data != NULL)
        {
            printf("        值: ");
            
            // 根据元素大小尝试解释数据
            if (list->element_size == sizeof(int))
            {
                printf("%d", *(int*)current->data);
            }
            else if (list->element_size == sizeof(double))
            {
                printf("%f", *(double*)current->data);
            }
            else if (list->element_size == sizeof(char))
            {
                printf("'%c'", *(char*)current->data);
            }
            else if (list->element_size == sizeof(char*))
            {
                printf("\"%s\"", *(char**)current->data);
            }
            else
            {
                printf("(无法显示，大小: %zu 字节)", list->element_size);
            }
            
            printf("\n");
        }
        
        current = current->next;
        index++;
    }
    
    printf("\n");
}
