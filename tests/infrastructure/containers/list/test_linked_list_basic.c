/**
 * @file test_linked_list_basic.c
 * @brief 链表基础功能测试
 * 
 * 测试链表的基本操作，包括创建、销毁、插入、删除、查询等。
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
 * @brief 测试创建和销毁链表
 */
void test_create_destroy(void)
{
    printf("测试: 创建和销毁链表\n");
    
    // 测试正常创建
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    assert(list->length == 0);
    assert(list->head == NULL);
    assert(list->tail == NULL);
    assert(list->item_size == sizeof(int));
    
    // 测试销毁
    F_destroy_linked_list(list);
    
    // 测试无效参数
    Stru_LinkedList_t* null_list = F_create_linked_list(0);
    assert(null_list == NULL);
    
    printf("✓ 创建和销毁测试通过\n");
}

/**
 * @brief 测试插入操作
 */
void test_insert_operations(void)
{
    printf("测试: 插入操作\n");
    
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    // 测试尾部插入
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++)
    {
        bool success = F_linked_list_append(list, &values[i]);
        assert(success == true);
        assert(list->length == (size_t)(i + 1));
    }
    
    // 验证插入结果
    assert(list->length == 5);
    int* first = (int*)F_linked_list_get(list, 0);
    int* last = (int*)F_linked_list_get(list, 4);
    assert(first != NULL && *first == 10);
    assert(last != NULL && *last == 50);
    
    // 测试头部插入
    int head_value = 5;
    bool success = F_linked_list_prepend(list, &head_value);
    assert(success == true);
    assert(list->length == 6);
    
    first = (int*)F_linked_list_get(list, 0);
    assert(first != NULL && *first == 5);
    
    // 测试中间插入
    int middle_value = 25;
    success = F_linked_list_insert(list, &middle_value, 3);
    assert(success == true);
    assert(list->length == 7);
    
    int* middle = (int*)F_linked_list_get(list, 3);
    assert(middle != NULL && *middle == 25);
    
    // 测试无效插入
    success = F_linked_list_insert(NULL, &middle_value, 0);
    assert(success == false);
    
    success = F_linked_list_insert(list, NULL, 0);
    assert(success == false);
    
    // 测试越界插入（应该在尾部插入）
    int out_of_bounds_value = 100;
    success = F_linked_list_insert(list, &out_of_bounds_value, 100);
    assert(success == true);
    assert(list->length == 8);
    
    last = (int*)F_linked_list_get(list, 7);
    assert(last != NULL && *last == 100);
    
    F_destroy_linked_list(list);
    printf("✓ 插入操作测试通过\n");
}

/**
 * @brief 测试删除操作
 */
void test_remove_operations(void)
{
    printf("测试: 删除操作\n");
    
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    // 准备测试数据
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    // 测试头部删除
    int removed_value;
    bool success = F_linked_list_remove_first(list, &removed_value);
    assert(success == true);
    assert(removed_value == 10);
    assert(list->length == 4);
    
    // 验证头部已更新
    int* first = (int*)F_linked_list_get(list, 0);
    assert(first != NULL && *first == 20);
    
    // 测试尾部删除
    success = F_linked_list_remove_last(list, &removed_value);
    assert(success == true);
    assert(removed_value == 50);
    assert(list->length == 3);
    
    // 验证尾部已更新
    int* last = (int*)F_linked_list_get(list, 2);
    assert(last != NULL && *last == 40);
    
    // 测试中间删除
    success = F_linked_list_remove_at(list, 1, &removed_value);
    assert(success == true);
    assert(removed_value == 30);
    assert(list->length == 2);
    
    // 验证中间删除结果
    int* remaining1 = (int*)F_linked_list_get(list, 0);
    int* remaining2 = (int*)F_linked_list_get(list, 1);
    assert(remaining1 != NULL && *remaining1 == 20);
    assert(remaining2 != NULL && *remaining2 == 40);
    
    // 测试无效删除
    success = F_linked_list_remove_first(NULL, &removed_value);
    assert(success == false);
    
    success = F_linked_list_remove_at(list, 10, &removed_value);
    assert(success == false);
    
    // 测试空链表删除
    F_linked_list_clear(list);
    success = F_linked_list_remove_first(list, &removed_value);
    assert(success == false);
    
    F_destroy_linked_list(list);
    printf("✓ 删除操作测试通过\n");
}

/**
 * @brief 测试查询操作
 */
void test_query_operations(void)
{
    printf("测试: 查询操作\n");
    
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    // 准备测试数据
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    // 测试长度查询
    size_t length = F_linked_list_length(list);
    assert(length == 5);
    
    length = F_linked_list_length(NULL);
    assert(length == 0);
    
    // 测试元素获取
    for (int i = 0; i < 5; i++)
    {
        int* value = (int*)F_linked_list_get(list, i);
        assert(value != NULL);
        assert(*value == values[i]);
    }
    
    // 测试越界获取
    int* out_of_bounds = (int*)F_linked_list_get(list, 10);
    assert(out_of_bounds == NULL);
    
    out_of_bounds = (int*)F_linked_list_get(NULL, 0);
    assert(out_of_bounds == NULL);
    
    // 测试空检查
    bool empty = F_linked_list_is_empty(list);
    assert(empty == false);
    
    Stru_LinkedList_t* empty_list = F_create_linked_list(sizeof(int));
    empty = F_linked_list_is_empty(empty_list);
    assert(empty == true);
    
    empty = F_linked_list_is_empty(NULL);
    assert(empty == true);
    
    F_destroy_linked_list(empty_list);
    F_destroy_linked_list(list);
    printf("✓ 查询操作测试通过\n");
}

/**
 * @brief 测试查找操作
 */
int compare_int(void* a, void* b)
{
    int* ia = (int*)a;
    int* ib = (int*)b;
    return (*ia == *ib) ? 0 : (*ia > *ib ? 1 : -1);
}

void test_find_operation(void)
{
    printf("测试: 查找操作\n");
    
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    // 准备测试数据
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    // 测试查找存在的元素
    int search_value = 30;
    int64_t position = F_linked_list_find(list, &search_value, compare_int);
    assert(position == 2);
    
    // 测试查找不存在的元素
    search_value = 99;
    position = F_linked_list_find(list, &search_value, compare_int);
    assert(position == -1);
    
    // 测试无效参数
    position = F_linked_list_find(NULL, &search_value, compare_int);
    assert(position == -1);
    
    position = F_linked_list_find(list, NULL, compare_int);
    assert(position == -1);
    
    position = F_linked_list_find(list, &search_value, NULL);
    assert(position == -1);
    
    F_destroy_linked_list(list);
    printf("✓ 查找操作测试通过\n");
}

/**
 * @brief 测试清空和遍历操作
 */
void print_callback(void* data, void* user_data)
{
    int* value = (int*)data;
    int* count = (int*)user_data;
    (*count)++;
    
    // 验证数据有效性
    assert(value != NULL);
    assert(*value >= 10 && *value <= 50);
}

void test_clear_and_foreach(void)
{
    printf("测试: 清空和遍历操作\n");
    
    Stru_LinkedList_t* list = F_create_linked_list(sizeof(int));
    assert(list != NULL);
    
    // 准备测试数据
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++)
    {
        F_linked_list_append(list, &values[i]);
    }
    
    // 测试遍历
    int callback_count = 0;
    F_linked_list_foreach(list, print_callback, &callback_count);
    assert(callback_count == 5);
    
    // 测试无效遍历参数
    F_linked_list_foreach(NULL, print_callback, &callback_count);
    F_linked_list_foreach(list, NULL, &callback_count);
    
    // 测试清空
    F_linked_list_clear(list);
    assert(list->length == 0);
    assert(list->head == NULL);
    assert(list->tail == NULL);
    
    // 清空后再次遍历
    callback_count = 0;
    F_linked_list_foreach(list, print_callback, &callback_count);
    assert(callback_count == 0);
    
    // 测试无效清空
    F_linked_list_clear(NULL);
    
    F_destroy_linked_list(list);
    printf("✓ 清空和遍历测试通过\n");
}

/**
 * @brief 链表基础功能测试主函数
 * 
 * @return true 测试通过
 * @return false 测试失败
 */
bool test_linked_list_basic_operations(void)
{
    printf("开始链表基础功能测试\n");
    printf("=====================\n");
    
    test_create_destroy();
    test_insert_operations();
    test_remove_operations();
    test_query_operations();
    test_find_operation();
    test_clear_and_foreach();
    
    printf("\n=====================\n");
    printf("所有基础功能测试通过！\n");
    
    return true;
}
