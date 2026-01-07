/**
 * @file test_linked_list_edge.c
 * @brief 链表边界条件测试
 * 
 * 测试链表的边界条件和异常情况处理。
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
#include "../../../../src/infrastructure/containers/list/CN_linked_list.h"
#include "test_linked_list.h"

/**
 * @brief 测试空链表操作
 */
void test_empty_list_operations(void)
{
    printf("测试: 空链表操作\n");
    
    // 创建空链表
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    assert(F_linked_list_is_empty(list) == true);
    assert(F_linked_list_length(list) == 0);
    
    // 测试在空链表上执行各种操作
    int dummy_value = 42;
    
    // 删除操作应该失败
    int removed_value;
    bool success = F_linked_list_remove_first(list, &removed_value);
    assert(success == false);
    
    success = F_linked_list_remove_last(list, &removed_value);
    assert(success == false);
    
    success = F_linked_list_remove_at(list, 0, &removed_value);
    assert(success == false);
    
    // 获取操作应该返回NULL
    void* element = F_linked_list_get(list, 0);
    assert(element == NULL);
    
    element = F_linked_list_get(list, 10);
    assert(element == NULL);
    
    // 查找操作应该返回-1
    int64_t position = F_linked_list_find(list, &dummy_value, NULL);
    assert(position == -1);
    
    // 遍历操作应该不执行任何操作
    int callback_called = 0;
    
    F_linked_list_foreach(list, NULL, &callback_called);
    assert(callback_called == 0);
    
    // 清空操作应该成功但不改变状态
    F_linked_list_clear(list);
    assert(F_linked_list_is_empty(list) == true);
    assert(F_linked_list_length(list) == 0);
    
    // 迭代器测试
    Stru_LinkedListIterator_t* iter = F_create_linked_list_iterator(list);
    assert(iter != NULL);
    assert(F_linked_list_iterator_is_valid(iter) == false);
    
    success = F_linked_list_iterator_remove(iter, &removed_value);
    assert(success == false);
    
    F_destroy_linked_list_iterator(iter);
    F_destroy_linked_list(list);
    
    printf("✓ 空链表操作测试通过\n");
}

/**
 * @brief 测试单元素链表操作
 */
void test_single_element_list(void)
{
    printf("测试: 单元素链表操作\n");
    
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    int value = 42;
    bool success = F_linked_list_append(list, &value);
    assert(success == true);
    assert(F_linked_list_length(list) == 1);
    assert(F_linked_list_is_empty(list) == false);
    
    // 测试获取元素
    int* retrieved = (int*)F_linked_list_get(list, 0);
    assert(retrieved != NULL);
    assert(*retrieved == 42);
    
    retrieved = (int*)F_linked_list_get(list, 1);
    assert(retrieved == NULL);
    
    // 测试删除头部（也是尾部）
    int removed_value;
    success = F_linked_list_remove_first(list, &removed_value);
    assert(success == true);
    assert(removed_value == 42);
    assert(F_linked_list_length(list) == 0);
    assert(F_linked_list_is_empty(list) == true);
    
    // 重新添加元素
    success = F_linked_list_append(list, &value);
    assert(success == true);
    
    // 测试删除尾部（也是头部）
    success = F_linked_list_remove_last(list, &removed_value);
    assert(success == true);
    assert(removed_value == 42);
    assert(F_linked_list_length(list) == 0);
    
    // 重新添加元素
    success = F_linked_list_append(list, &value);
    assert(success == true);
    
    // 测试删除指定位置
    success = F_linked_list_remove_at(list, 0, &removed_value);
    assert(success == true);
    assert(removed_value == 42);
    assert(F_linked_list_length(list) == 0);
    
    // 测试越界删除
    success = F_linked_list_remove_at(list, 0, &removed_value);
    assert(success == false);
    
    success = F_linked_list_remove_at(list, 10, &removed_value);
    assert(success == false);
    
    // 测试迭代器
    success = F_linked_list_append(list, &value);
    assert(success == true);
    
    Stru_LinkedListIterator_t* iter = F_create_linked_list_iterator(list);
    assert(iter != NULL);
    assert(F_linked_list_iterator_is_valid(iter) == true);
    
    // 迭代器删除
    success = F_linked_list_iterator_remove(iter, &removed_value);
    assert(success == true);
    assert(removed_value == 42);
    assert(F_linked_list_iterator_is_valid(iter) == false);
    
    F_destroy_linked_list_iterator(iter);
    F_destroy_linked_list(list);
    
    printf("✓ 单元素链表操作测试通过\n");
}

/**
 * @brief 测试内存分配失败情况
 */
void test_memory_allocation_failure(void)
{
    printf("测试: 内存分配失败情况\n");
    
    // 测试创建链表时内存分配失败
    // 注：这里我们无法直接模拟malloc失败，但可以测试item_size为0的情况
    Stru_LinkedList_t* list = F_create_linked_list(0);
    assert(list == NULL);
    
    // 创建正常链表
    list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    // 测试插入时内存分配失败
    // 注：这里我们无法直接模拟malloc失败，但可以测试无效参数
    bool success = F_linked_list_append(list, NULL);
    assert(success == false);
    
    success = F_linked_list_prepend(list, NULL);
    assert(success == false);
    
    success = F_linked_list_insert(list, NULL, 0);
    assert(success == false);
    
    // 测试迭代器创建失败
    Stru_LinkedListIterator_t* iter = F_create_linked_list_iterator(NULL);
    assert(iter == NULL);
    
    // 清理
    F_destroy_linked_list(list);
    
    printf("✓ 内存分配失败情况测试通过\n");
}

/**
 * @brief 测试边界位置操作
 */
void test_boundary_positions(void)
{
    printf("测试: 边界位置操作\n");
    
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    // 添加一些元素
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    // 测试边界位置插入
    int head_value = 5;
    bool success = F_linked_list_insert(list, &head_value, 0);
    assert(success == true);
    assert(F_linked_list_length(list) == 6);
    
    int* first = (int*)F_linked_list_get(list, 0);
    assert(first != NULL && *first == 5);
    
    int tail_value = 60;
    success = F_linked_list_insert(list, &tail_value, 6); // 等于长度，应该在尾部插入
    assert(success == true);
    assert(F_linked_list_length(list) == 7);
    
    int* last = (int*)F_linked_list_get(list, 6);
    assert(last != NULL && *last == 60);
    
    // 测试越界插入（大于长度）
    int out_of_bounds_value = 100;
    success = F_linked_list_insert(list, &out_of_bounds_value, 100);
    assert(success == true); // 应该在尾部插入
    assert(F_linked_list_length(list) == 8);
    
    last = (int*)F_linked_list_get(list, 7);
    assert(last != NULL && *last == 100);
    
    // 测试边界位置删除
    int removed_value;
    success = F_linked_list_remove_at(list, 0, &removed_value);
    assert(success == true);
    assert(removed_value == 5);
    assert(F_linked_list_length(list) == 7);
    
    success = F_linked_list_remove_at(list, 6, &removed_value); // 删除最后一个
    assert(success == true);
    assert(removed_value == 100);
    assert(F_linked_list_length(list) == 6);
    
    // 测试越界删除
    success = F_linked_list_remove_at(list, 10, &removed_value);
    assert(success == false);
    
    // 测试迭代器边界
    Stru_LinkedListIterator_t* iter = F_create_linked_list_iterator(list);
    assert(iter != NULL);
    
    // 移动到最后一个元素
    while (F_linked_list_iterator_has_next(iter))
    {
        F_linked_list_iterator_next(iter);
    }
    
    // 在最后一个元素后插入
    int insert_at_end = 99;
    success = F_linked_list_iterator_insert(iter, &insert_at_end);
    assert(success == true); // 应该在尾部插入
    
    F_destroy_linked_list_iterator(iter);
    F_destroy_linked_list(list);
    
    printf("✓ 边界位置操作测试通过\n");
}

/**
 * @brief 测试重复操作
 */
void test_repeated_operations(void)
{
    printf("测试: 重复操作\n");
    
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    // 重复添加和删除
    const int REPEAT_COUNT = 100;
    for (int i = 0; i < REPEAT_COUNT; i++)
    {
        bool success = F_linked_list_append(list, &i);
        assert(success == true);
        assert(F_linked_list_length(list) == (size_t)(i + 1));
    }
    
    // 验证所有元素
    for (int i = 0; i < REPEAT_COUNT; i++)
    {
        int* value = (int*)F_linked_list_get(list, i);
        assert(value != NULL);
        assert(*value == i);
    }
    
    // 重复删除
    for (int i = 0; i < REPEAT_COUNT; i++)
    {
        int removed_value;
        bool success = F_linked_list_remove_first(list, &removed_value);
        assert(success == true);
        assert(removed_value == i);
        assert(F_linked_list_length(list) == (size_t)(REPEAT_COUNT - i - 1));
    }
    
    assert(F_linked_list_is_empty(list) == true);
    
    // 重复插入和删除混合操作
    for (int i = 0; i < 50; i++)
    {
        bool success = F_linked_list_append(list, &i);
        assert(success == true);
        
        if (i % 2 == 0)
        {
            int removed_value;
            success = F_linked_list_remove_first(list, &removed_value);
            assert(success == true);
            assert(removed_value == i / 2);
        }
    }
    
    // 清空链表
    F_linked_list_clear(list);
    assert(F_linked_list_is_empty(list) == true);
    
    F_destroy_linked_list(list);
    
    printf("✓ 重复操作测试通过\n");
}

/**
 * @brief 测试无效参数处理
 */
void test_invalid_parameters(void)
{
    printf("测试: 无效参数处理\n");
    
    // 测试所有函数对NULL参数的处理
    
    // F_create_linked_list - 已测试item_size=0
    
    // F_destroy_linked_list - 应该处理NULL
    F_destroy_linked_list(NULL);
    
    // F_linked_list_prepend
    int value = 42;
    bool success = F_linked_list_prepend(NULL, &value);
    assert(success == false);
    
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    success = F_linked_list_prepend(list, NULL);
    assert(success == false);
    
    // F_linked_list_append
    success = F_linked_list_append(NULL, &value);
    assert(success == false);
    
    success = F_linked_list_append(list, NULL);
    assert(success == false);
    
    // F_linked_list_insert
    success = F_linked_list_insert(NULL, &value, 0);
    assert(success == false);
    
    success = F_linked_list_insert(list, NULL, 0);
    assert(success == false);
    
    // F_linked_list_remove_first
    int removed_value;
    success = F_linked_list_remove_first(NULL, &removed_value);
    assert(success == false);
    
    // F_linked_list_remove_last
    success = F_linked_list_remove_last(NULL, &removed_value);
    assert(success == false);
    
    // F_linked_list_remove_at
    success = F_linked_list_remove_at(NULL, 0, &removed_value);
    assert(success == false);
    
    // F_linked_list_length
    size_t length = F_linked_list_length(NULL);
    assert(length == 0);
    
    // F_linked_list_get
    void* element = F_linked_list_get(NULL, 0);
    assert(element == NULL);
    
    // F_linked_list_find
    int64_t position = F_linked_list_find(NULL, &value, NULL);
    assert(position == -1);
    
    position = F_linked_list_find(list, NULL, NULL);
    assert(position == -1);
    
    position = F_linked_list_find(list, &value, NULL);
    assert(position == -1);
    
    // F_linked_list_clear
    F_linked_list_clear(NULL);
    
    // F_linked_list_foreach
    F_linked_list_foreach(NULL, NULL, NULL);
    F_linked_list_foreach(list, NULL, NULL);
    
    // F_linked_list_is_empty
    bool empty = F_linked_list_is_empty(NULL);
    assert(empty == true);
    
    // 迭代器函数
    // F_create_linked_list_iterator - 已测试
    // F_destroy_linked_list_iterator - 应该处理NULL
    F_destroy_linked_list_iterator(NULL);
    
    // F_linked_list_iterator_reset
    success = F_linked_list_iterator_reset(NULL);
    assert(success == false);
    
    // F_linked_list_iterator_has_next
    bool has_next = F_linked_list_iterator_has_next(NULL);
    assert(has_next == false);
    
    // F_linked_list_iterator_next
    void* next = F_linked_list_iterator_next(NULL);
    assert(next == NULL);
    
    // F_linked_list_iterator_current
    void* current = F_linked_list_iterator_current(NULL);
    assert(current == NULL);
    
    // F_linked_list_iterator_position
    int64_t pos = F_linked_list_iterator_position(NULL);
    assert(pos == -1);
    
    // F_linked_list_iterator_is_valid
    bool valid = F_linked_list_iterator_is_valid(NULL);
    assert(valid == false);
    
    // F_linked_list_iterator_insert
    success = F_linked_list_iterator_insert(NULL, &value);
    assert(success == false);
    
    // F_linked_list_iterator_remove
    success = F_linked_list_iterator_remove(NULL, &removed_value);
    assert(success == false);
    
    // F_linked_list_find_iterator
    Stru_LinkedListIterator_t* iter = F_linked_list_find_iterator(NULL, &value, NULL);
    assert(iter == NULL);
    
    // 排序函数
    // F_linked_list_sort
    success = F_linked_list_sort(NULL, NULL, Eum_SORT_BUBBLE, Eum_SORT_ASCENDING);
    assert(success == false);
    
    // F_linked_list_bubble_sort
    success = F_linked_list_bubble_sort(NULL, NULL, true);
    assert(success == false);
    
    // F_linked_list_insertion_sort
    success = F_linked_list_insertion_sort(NULL, NULL, true);
    assert(success == false);
    
    // F_linked_list_merge_sort
    success = F_linked_list_merge_sort(NULL, NULL, true);
    assert(success == false);
    
    // F_linked_list_is_sorted
    bool is_sorted = F_linked_list_is_sorted(NULL, NULL, true);
    assert(is_sorted == true);
    
    // F_linked_list_reverse
    success = F_linked_list_reverse(NULL);
    assert(success == false);
    
    F_destroy_linked_list(list);
    
    printf("✓ 无效参数处理测试通过\n");
}

/**
 * @brief 链表边界条件测试主函数
 * 
 * @return true 测试通过
 * @return false 测试失败
 */
bool test_linked_list_edge_cases(void)
{
    printf("开始链表边界条件测试\n");
    printf("=====================\n");
    
    test_empty_list_operations();
    test_single_element_list();
    test_memory_allocation_failure();
    test_boundary_positions();
    test_repeated_operations();
    test_invalid_parameters();
    
    printf("\n=====================\n");
    printf("所有边界条件测试通过！\n");
    
    return true;
}
