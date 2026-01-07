/**
 * @file test_linked_list_sort.c
 * @brief 链表排序功能测试
 * 
 * 测试链表排序算法的基本操作，包括各种排序算法和工具函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "../../../../src/infrastructure/containers/list/CN_linked_list.h"
#include "test_linked_list.h"

/**
 * @brief 比较函数：升序比较整数
 */
int compare_int_ascending(void* a, void* b)
{
    int* ia = (int*)a;
    int* ib = (int*)b;
    return (*ia - *ib);
}

/**
 * @brief 比较函数：降序比较整数
 */
int compare_int_descending(void* a, void* b)
{
    int* ia = (int*)a;
    int* ib = (int*)b;
    return (*ib - *ia); // 反转顺序实现降序
}

/**
 * @brief 验证链表是否按指定顺序排序
 */
void verify_list_sorted(Stru_LinkedList_t* list, int (*compare)(void*, void*), bool ascending)
{
    if (list == NULL || list->length < 2)
    {
        return;
    }
    
    for (size_t i = 0; i < list->length - 1; i++)
    {
        int* current = (int*)F_linked_list_get(list, i);
        int* next = (int*)F_linked_list_get(list, i + 1);
        
        assert(current != NULL);
        assert(next != NULL);
        
        int cmp = compare(current, next);
        
        if (ascending)
        {
            assert(cmp <= 0); // 升序：当前元素 <= 下一个元素
        }
        else
        {
            assert(cmp >= 0); // 降序：当前元素 >= 下一个元素
        }
    }
}

/**
 * @brief 测试冒泡排序
 */
void test_bubble_sort(void)
{
    printf("测试: 冒泡排序\n");
    
    // 测试升序排序
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    int values[] = {64, 34, 25, 12, 22, 11, 90};
    for (int i = 0; i < 7; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    bool success = F_linked_list_bubble_sort(list, compare_int_ascending, true);
    assert(success == true);
    
    verify_list_sorted(list, compare_int_ascending, true);
    
    // 测试降序排序
    F_linked_list_clear(list);
    for (int i = 0; i < 7; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    success = F_linked_list_bubble_sort(list, compare_int_ascending, false);
    assert(success == true);
    
    verify_list_sorted(list, compare_int_ascending, false);
    
    // 测试边界情况：空链表
    F_linked_list_clear(list);
    success = F_linked_list_bubble_sort(list, compare_int_ascending, true);
    assert(success == false); // 长度小于2返回false
    
    // 测试边界情况：单元素链表
    int single_value = 42;
    F_linked_list_append(list, &single_value);
    success = F_linked_list_bubble_sort(list, compare_int_ascending, true);
    assert(success == false); // 长度小于2返回false
    
    // 测试无效参数
    success = F_linked_list_bubble_sort(NULL, compare_int_ascending, true);
    assert(success == false);
    
    success = F_linked_list_bubble_sort(list, NULL, true);
    assert(success == false);
    
    F_destroy_linked_list(list);
    printf("✓ 冒泡排序测试通过\n");
}

/**
 * @brief 测试插入排序
 */
void test_insertion_sort(void)
{
    printf("测试: 插入排序\n");
    
    // 测试升序排序
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    int values[] = {12, 11, 13, 5, 6, 7};
    for (int i = 0; i < 6; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    bool success = F_linked_list_insertion_sort(list, compare_int_ascending, true);
    assert(success == true);
    
    verify_list_sorted(list, compare_int_ascending, true);
    
    // 验证具体顺序
    int* first = (int*)F_linked_list_get(list, 0);
    int* last = (int*)F_linked_list_get(list, 5);
    assert(first != NULL && *first == 5);
    assert(last != NULL && *last == 13);
    
    // 测试降序排序
    F_linked_list_clear(list);
    for (int i = 0; i < 6; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    success = F_linked_list_insertion_sort(list, compare_int_ascending, false);
    assert(success == true);
    
    verify_list_sorted(list, compare_int_ascending, false);
    
    // 验证具体顺序
    first = (int*)F_linked_list_get(list, 0);
    last = (int*)F_linked_list_get(list, 5);
    assert(first != NULL && *first == 13);
    assert(last != NULL && *last == 5);
    
    // 测试已排序链表
    F_linked_list_clear(list);
    int sorted_values[] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++)
    {
        F_linked_list_append(list, &sorted_values[i]);
    }
    
    success = F_linked_list_insertion_sort(list, compare_int_ascending, true);
    assert(success == true);
    verify_list_sorted(list, compare_int_ascending, true);
    
    F_destroy_linked_list(list);
    printf("✓ 插入排序测试通过\n");
}

/**
 * @brief 测试归并排序
 */
void test_merge_sort(void)
{
    printf("测试: 归并排序\n");
    
    // 测试升序排序
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    int values[] = {38, 27, 43, 3, 9, 82, 10};
    for (int i = 0; i < 7; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    bool success = F_linked_list_merge_sort(list, compare_int_ascending, true);
    assert(success == true);
    
    verify_list_sorted(list, compare_int_ascending, true);
    
    // 验证具体顺序
    int* first = (int*)F_linked_list_get(list, 0);
    int* last = (int*)F_linked_list_get(list, 6);
    assert(first != NULL && *first == 3);
    assert(last != NULL && *last == 82);
    
    // 测试降序排序
    F_linked_list_clear(list);
    for (int i = 0; i < 7; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    success = F_linked_list_merge_sort(list, compare_int_ascending, false);
    assert(success == true);
    
    verify_list_sorted(list, compare_int_ascending, false);
    
    // 验证具体顺序
    first = (int*)F_linked_list_get(list, 0);
    last = (int*)F_linked_list_get(list, 6);
    assert(first != NULL && *first == 82);
    assert(last != NULL && *last == 3);
    
    // 测试随机数据
    F_linked_list_clear(list);
    srand(time(NULL));
    
    const int RANDOM_COUNT = 100;
    int random_values[RANDOM_COUNT];
    
    for (int i = 0; i < RANDOM_COUNT; i++)
    {
        random_values[i] = rand() % 1000;
        F_linked_list_append(list, &random_values[i]);
    }
    
    success = F_linked_list_merge_sort(list, compare_int_ascending, true);
    assert(success == true);
    verify_list_sorted(list, compare_int_ascending, true);
    
    F_destroy_linked_list(list);
    printf("✓ 归并排序测试通过\n");
}

/**
 * @brief 测试通用排序接口
 */
void test_general_sort(void)
{
    printf("测试: 通用排序接口\n");
    
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    int values[] = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
    for (int i = 0; i < 10; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    // 测试冒泡排序算法
    bool success = F_linked_list_sort(list, compare_int_ascending, 
                                     Eum_SORT_BUBBLE, Eum_SORT_ASCENDING);
    assert(success == true);
    verify_list_sorted(list, compare_int_ascending, true);
    
    // 重置链表
    F_linked_list_clear(list);
    for (int i = 0; i < 10; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    // 测试插入排序算法
    success = F_linked_list_sort(list, compare_int_ascending,
                                Eum_SORT_INSERTION, Eum_SORT_DESCENDING);
    assert(success == true);
    verify_list_sorted(list, compare_int_ascending, false);
    
    // 重置链表
    F_linked_list_clear(list);
    for (int i = 0; i < 10; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    // 测试归并排序算法
    success = F_linked_list_sort(list, compare_int_ascending,
                                Eum_SORT_MERGE, Eum_SORT_ASCENDING);
    assert(success == true);
    verify_list_sorted(list, compare_int_ascending, true);
    
    // 测试未实现的算法（应该使用归并排序作为默认）
    F_linked_list_clear(list);
    for (int i = 0; i < 10; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    success = F_linked_list_sort(list, compare_int_ascending,
                                Eum_SORT_QUICK, Eum_SORT_ASCENDING);
    assert(success == true); // 应该成功，使用默认算法
    
    // 测试无效参数
    success = F_linked_list_sort(NULL, compare_int_ascending,
                                Eum_SORT_BUBBLE, Eum_SORT_ASCENDING);
    assert(success == false);
    
    success = F_linked_list_sort(list, NULL,
                                Eum_SORT_BUBBLE, Eum_SORT_ASCENDING);
    assert(success == false);
    
    F_destroy_linked_list(list);
    printf("✓ 通用排序接口测试通过\n");
}

/**
 * @brief 测试排序检查功能
 */
void test_is_sorted(void)
{
    printf("测试: 排序检查功能\n");
    
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    // 测试已排序链表（升序）
    int sorted_asc[] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++)
    {
        F_linked_list_append(list, &sorted_asc[i]);
    }
    
    bool is_sorted = F_linked_list_is_sorted(list, compare_int_ascending, true);
    assert(is_sorted == true);
    
    is_sorted = F_linked_list_is_sorted(list, compare_int_ascending, false);
    assert(is_sorted == false); // 不是降序
    
    // 测试已排序链表（降序）
    F_linked_list_clear(list);
    int sorted_desc[] = {5, 4, 3, 2, 1};
    for (int i = 0; i < 5; i++)
    {
        F_linked_list_append(list, &sorted_desc[i]);
    }
    
    is_sorted = F_linked_list_is_sorted(list, compare_int_ascending, false);
    assert(is_sorted == true);
    
    is_sorted = F_linked_list_is_sorted(list, compare_int_ascending, true);
    assert(is_sorted == false); // 不是升序
    
    // 测试未排序链表
    F_linked_list_clear(list);
    int unsorted[] = {3, 1, 4, 2, 5};
    for (int i = 0; i < 5; i++)
    {
        F_linked_list_append(list, &unsorted[i]);
    }
    
    is_sorted = F_linked_list_is_sorted(list, compare_int_ascending, true);
    assert(is_sorted == false);
    
    is_sorted = F_linked_list_is_sorted(list, compare_int_ascending, false);
    assert(is_sorted == false);
    
    // 测试边界情况
    F_linked_list_clear(list);
    is_sorted = F_linked_list_is_sorted(list, compare_int_ascending, true);
    assert(is_sorted == true); // 空链表视为已排序
    
    int single = 42;
    F_linked_list_append(list, &single);
    is_sorted = F_linked_list_is_sorted(list, compare_int_ascending, true);
    assert(is_sorted == true); // 单元素链表视为已排序
    
    // 测试无效参数
    is_sorted = F_linked_list_is_sorted(NULL, compare_int_ascending, true);
    assert(is_sorted == true); // NULL视为已排序
    
    is_sorted = F_linked_list_is_sorted(list, NULL, true);
    assert(is_sorted == true); // NULL比较函数视为已排序
    
    F_destroy_linked_list(list);
    printf("✓ 排序检查功能测试通过\n");
}

/**
 * @brief 测试反转功能
 */
void test_reverse(void)
{
    printf("测试: 反转功能\n");
    
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    // 测试正常反转
    int values[] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    bool success = F_linked_list_reverse(list);
    assert(success == true);
    
    // 验证反转结果
    for (int i = 0; i < 5; i++)
    {
        int* value = (int*)F_linked_list_get(list, i);
        assert(value != NULL);
        assert(*value == values[4 - i]);
    }
    
    // 验证头尾指针
    assert(list->head != NULL);
    assert(list->tail != NULL);
    
    int* head_value = (int*)list->head->data;
    int* tail_value = (int*)list->tail->data;
    assert(head_value != NULL && *head_value == 5);
    assert(tail_value != NULL && *tail_value == 1);
    
    // 测试边界情况：空链表
    F_linked_list_clear(list);
    success = F_linked_list_reverse(list);
    assert(success == false);
    
    // 测试边界情况：单元素链表
    int single = 42;
    F_linked_list_append(list, &single);
    success = F_linked_list_reverse(list);
    assert(success == false);
    
    // 测试无效参数
    success = F_linked_list_reverse(NULL);
    assert(success == false);
    
    F_destroy_linked_list(list);
    printf("✓ 反转功能测试通过\n");
}

/**
 * @brief 链表排序功能测试主函数
 * 
 * @return true 测试通过
 * @return false 测试失败
 */
bool test_linked_list_sort_operations(void)
{
    printf("开始链表排序功能测试\n");
    printf("=====================\n");
    
    test_bubble_sort();
    test_insertion_sort();
    test_merge_sort();
    test_general_sort();
    test_is_sorted();
    test_reverse();
    
    printf("\n=====================\n");
    printf("所有排序功能测试通过！\n");
    
    return true;
}
