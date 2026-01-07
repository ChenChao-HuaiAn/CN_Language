/**
 * @file test_linked_list_iterator.c
 * @brief 链表迭代器功能测试
 * 
 * 测试链表迭代器的基本操作，包括创建、销毁、遍历、插入、删除等。
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
 * @brief 测试创建和销毁迭代器
 */
void test_iterator_create_destroy(void)
{
    printf("测试: 创建和销毁迭代器\n");
    
    // 创建链表
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    // 测试创建空链表迭代器
    Stru_LinkedListIterator_t* iter = F_create_linked_list_iterator(list);
    assert(iter != NULL);
    assert(iter->list == list);
    assert(iter->current == NULL);
    assert(iter->position == 0);
    assert(iter->is_valid == false);
    
    F_destroy_linked_list_iterator(iter);
    
    // 添加数据后创建迭代器
    int values[] = {10, 20, 30};
    for (int i = 0; i < 3; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    iter = F_create_linked_list_iterator(list);
    assert(iter != NULL);
    assert(iter->list == list);
    assert(iter->current == list->head);
    assert(iter->position == 0);
    assert(iter->is_valid == true);
    
    // 测试无效参数
    Stru_LinkedListIterator_t* null_iter = F_create_linked_list_iterator(NULL);
    assert(null_iter == NULL);
    
    F_destroy_linked_list_iterator(iter);
    F_destroy_linked_list(list);
    
    printf("✓ 创建和销毁迭代器测试通过\n");
}

/**
 * @brief 测试迭代器遍历操作
 */
void test_iterator_traversal(void)
{
    printf("测试: 迭代器遍历操作\n");
    
    // 创建链表并添加数据
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    // 创建迭代器
    Stru_LinkedListIterator_t* iter = F_create_linked_list_iterator(list);
    assert(iter != NULL);
    
    // 测试初始状态
    assert(F_linked_list_iterator_is_valid(iter) == true);
    assert(F_linked_list_iterator_position(iter) == 0);
    
    int* current = (int*)F_linked_list_iterator_current(iter);
    assert(current != NULL && *current == 10);
    
    // 测试遍历所有元素
    int count = 0;
    while (F_linked_list_iterator_is_valid(iter))
    {
        current = (int*)F_linked_list_iterator_current(iter);
        assert(current != NULL);
        assert(*current == values[count]);
        
        // 测试has_next
        if (count < 4)
        {
            assert(F_linked_list_iterator_has_next(iter) == true);
        }
        else
        {
            assert(F_linked_list_iterator_has_next(iter) == false);
        }
        
        // 移动到下一个元素
        void* next = F_linked_list_iterator_next(iter);
        if (count < 4)
        {
            assert(next != NULL);
            assert(*(int*)next == values[count + 1]);
        }
        else
        {
            assert(next == NULL);
            assert(F_linked_list_iterator_is_valid(iter) == false);
        }
        
        count++;
    }
    
    assert(count == 5);
    
    // 测试重置迭代器
    bool success = F_linked_list_iterator_reset(iter);
    assert(success == true);
    assert(F_linked_list_iterator_is_valid(iter) == true);
    assert(F_linked_list_iterator_position(iter) == 0);
    
    current = (int*)F_linked_list_iterator_current(iter);
    assert(current != NULL && *current == 10);
    
    // 测试无效重置
    success = F_linked_list_iterator_reset(NULL);
    assert(success == false);
    
    F_destroy_linked_list_iterator(iter);
    F_destroy_linked_list(list);
    
    printf("✓ 迭代器遍历操作测试通过\n");
}

/**
 * @brief 测试迭代器插入操作
 */
void test_iterator_insert(void)
{
    printf("测试: 迭代器插入操作\n");
    
    // 创建链表并添加数据
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    int values[] = {10, 20, 30};
    for (int i = 0; i < 3; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    // 创建迭代器
    Stru_LinkedListIterator_t* iter = F_create_linked_list_iterator(list);
    assert(iter != NULL);
    
    // 测试在头部插入（迭代器在位置0）
    int head_value = 5;
    bool success = F_linked_list_iterator_insert(iter, &head_value);
    assert(success == true);
    assert(list->length == 4);
    assert(F_linked_list_iterator_position(iter) == 1); // 位置后移
    
    // 验证插入结果
    int* first = (int*)F_linked_list_get(list, 0);
    assert(first != NULL && *first == 5);
    
    // 重置迭代器并移动到位置2
    F_linked_list_iterator_reset(iter);
    F_linked_list_iterator_next(iter); // 位置1
    F_linked_list_iterator_next(iter); // 位置2
    
    // 测试在中间插入
    int middle_value = 25;
    success = F_linked_list_iterator_insert(iter, &middle_value);
    assert(success == true);
    assert(list->length == 5);
    assert(F_linked_list_iterator_position(iter) == 3); // 位置后移
    
    // 验证插入结果
    int* middle = (int*)F_linked_list_get(list, 2);
    assert(middle != NULL && *middle == 25);
    
    // 测试无效插入
    success = F_linked_list_iterator_insert(NULL, &middle_value);
    assert(success == false);
    
    success = F_linked_list_iterator_insert(iter, NULL);
    assert(success == false);
    
    // 测试在无效迭代器上插入
    Stru_LinkedList_t* empty_list = F_create_linked_list(sizeof(int));
    Stru_LinkedListIterator_t* empty_iter = F_create_linked_list_iterator(empty_list);
    
    int test_value = 100;
    success = F_linked_list_iterator_insert(empty_iter, &test_value);
    assert(success == true); // 应该在头部插入
    assert(empty_list->length == 1);
    
    F_destroy_linked_list_iterator(empty_iter);
    F_destroy_linked_list(empty_list);
    F_destroy_linked_list_iterator(iter);
    F_destroy_linked_list(list);
    
    printf("✓ 迭代器插入操作测试通过\n");
}

/**
 * @brief 测试迭代器删除操作
 */
void test_iterator_remove(void)
{
    printf("测试: 迭代器删除操作\n");
    
    // 创建链表并添加数据
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    // 创建迭代器并移动到位置2（值30）
    Stru_LinkedListIterator_t* iter = F_create_linked_list_iterator(list);
    assert(iter != NULL);
    
    F_linked_list_iterator_next(iter); // 位置1
    F_linked_list_iterator_next(iter); // 位置2
    
    // 测试删除当前元素
    int removed_value;
    bool success = F_linked_list_iterator_remove(iter, &removed_value);
    assert(success == true);
    assert(removed_value == 30);
    assert(list->length == 4);
    
    // 验证删除后迭代器状态
    assert(F_linked_list_iterator_is_valid(iter) == true);
    int* current = (int*)F_linked_list_iterator_current(iter);
    assert(current != NULL && *current == 40);
    assert(F_linked_list_iterator_position(iter) == 2); // 位置不变，但元素变了
    
    // 验证链表内容
    int* elem0 = (int*)F_linked_list_get(list, 0);
    int* elem1 = (int*)F_linked_list_get(list, 1);
    int* elem2 = (int*)F_linked_list_get(list, 2);
    int* elem3 = (int*)F_linked_list_get(list, 3);
    
    assert(elem0 != NULL && *elem0 == 10);
    assert(elem1 != NULL && *elem1 == 20);
    assert(elem2 != NULL && *elem2 == 40);
    assert(elem3 != NULL && *elem3 == 50);
    
    // 测试删除头部元素
    F_linked_list_iterator_reset(iter);
    success = F_linked_list_iterator_remove(iter, &removed_value);
    assert(success == true);
    assert(removed_value == 10);
    assert(list->length == 3);
    
    // 验证头部已更新
    current = (int*)F_linked_list_iterator_current(iter);
    assert(current != NULL && *current == 20);
    
    // 测试删除尾部元素
    F_linked_list_iterator_reset(iter);
    F_linked_list_iterator_next(iter); // 位置1
    F_linked_list_iterator_next(iter); // 位置2（现在是最后一个元素）
    
    success = F_linked_list_iterator_remove(iter, &removed_value);
    assert(success == true);
    assert(removed_value == 50);
    assert(list->length == 2);
    assert(F_linked_list_iterator_is_valid(iter) == false); // 删除尾部后迭代器无效
    
    // 测试无效删除
    success = F_linked_list_iterator_remove(NULL, &removed_value);
    assert(success == false);
    
    // 测试在无效迭代器上删除
    F_linked_list_iterator_reset(iter);
    success = F_linked_list_iterator_remove(iter, &removed_value);
    assert(success == true); // 可以删除，现在是头部
    
    // 测试空链表删除
    F_linked_list_clear(list);
    F_linked_list_iterator_reset(iter);
    success = F_linked_list_iterator_remove(iter, &removed_value);
    assert(success == false); // 空链表不能删除
    
    F_destroy_linked_list_iterator(iter);
    F_destroy_linked_list(list);
    
    printf("✓ 迭代器删除操作测试通过\n");
}

/**
 * @brief 测试查找迭代器功能
 */
static int compare_int_iterator(void* a, void* b)
{
    int* ia = (int*)a;
    int* ib = (int*)b;
    return (*ia == *ib) ? 0 : (*ia > *ib ? 1 : -1);
}

void test_find_iterator(void)
{
    printf("测试: 查找迭代器功能\n");
    
    // 创建链表并添加数据
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    // 测试查找存在的元素
    int search_value = 30;
    Stru_LinkedListIterator_t* iter = F_linked_list_find_iterator(list, &search_value, compare_int_iterator);
    assert(iter != NULL);
    assert(F_linked_list_iterator_is_valid(iter) == true);
    assert(F_linked_list_iterator_position(iter) == 2);
    
    int* current = (int*)F_linked_list_iterator_current(iter);
    assert(current != NULL && *current == 30);
    
    F_destroy_linked_list_iterator(iter);
    
    // 测试查找不存在的元素
    search_value = 99;
    iter = F_linked_list_find_iterator(list, &search_value, compare_int_iterator);
    assert(iter == NULL);
    
    // 测试查找头部元素
    search_value = 10;
    iter = F_linked_list_find_iterator(list, &search_value, compare_int_iterator);
    assert(iter != NULL);
    assert(F_linked_list_iterator_position(iter) == 0);
    
    F_destroy_linked_list_iterator(iter);
    
    // 测试查找尾部元素
    search_value = 50;
    iter = F_linked_list_find_iterator(list, &search_value, compare_int_iterator);
    assert(iter != NULL);
    assert(F_linked_list_iterator_position(iter) == 4);
    
    F_destroy_linked_list_iterator(iter);
    
    // 测试无效参数
    iter = F_linked_list_find_iterator(NULL, &search_value, compare_int_iterator);
    assert(iter == NULL);
    
    iter = F_linked_list_find_iterator(list, NULL, compare_int_iterator);
    assert(iter == NULL);
    
    iter = F_linked_list_find_iterator(list, &search_value, NULL);
    assert(iter == NULL);
    
    F_destroy_linked_list(list);
    
    printf("✓ 查找迭代器功能测试通过\n");
}

/**
 * @brief 链表迭代器功能测试主函数
 * 
 * @return true 测试通过
 * @return false 测试失败
 */
bool test_linked_list_iterator_operations(void)
{
    printf("开始链表迭代器功能测试\n");
    printf("=======================\n");
    
    test_iterator_create_destroy();
    test_iterator_traversal();
    test_iterator_insert();
    test_iterator_remove();
    test_find_iterator();
    
    printf("\n=======================\n");
    printf("所有迭代器功能测试通过！\n");
    
    return true;
}
