/******************************************************************************
 * 文件名: test_linked_list.c
 * 功能: CN_linked_list模块测试程序
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，测试CN_linked_list模块功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "../../../src/infrastructure/containers/linked_list/CN_linked_list.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

// 测试函数声明
void test_singly_linked_list_basic(void);
void test_doubly_linked_list_basic(void);
void test_linked_list_operations(void);
void test_linked_list_iterator(void);
void test_custom_functions(void);
void test_edge_cases(void);

int main(void)
{
    printf("=== CN_linked_list 模块测试 ===\n\n");
    
    printf("1. 测试单向链表基本操作:\n");
    test_singly_linked_list_basic();
    
    printf("\n2. 测试双向链表基本操作:\n");
    test_doubly_linked_list_basic();
    
    printf("\n3. 测试链表高级操作:\n");
    test_linked_list_operations();
    
    printf("\n4. 测试链表迭代器:\n");
    test_linked_list_iterator();
    
    printf("\n5. 测试自定义函数:\n");
    test_custom_functions();
    
    printf("\n6. 测试边界情况:\n");
    test_edge_cases();
    
    printf("\n=== 所有测试完成 ===\n");
    return 0;
}

// 测试单向链表基本操作
void test_singly_linked_list_basic(void)
{
    printf("  创建单向链表...\n");
    Stru_CN_LinkedList_t* list = CN_linked_list_create(Eum_LINKED_LIST_SINGLY, sizeof(int));
    
    if (list == NULL)
    {
        printf("  错误: 无法创建单向链表\n");
        return;
    }
    
    printf("  单向链表创建成功\n");
    printf("  初始长度: %zu\n", CN_linked_list_length(list));
    printf("  链表类型: %s\n", 
           CN_linked_list_type(list) == Eum_LINKED_LIST_SINGLY ? "单向链表" : "双向链表");
    
    // 测试添加元素
    printf("  测试添加元素...\n");
    for (int i = 1; i <= 5; i++)
    {
        if (CN_linked_list_append(list, &i))
        {
            printf("    添加元素 %d 成功\n", i);
        }
        else
        {
            printf("    添加元素 %d 失败\n", i);
        }
    }
    
    printf("  链表长度: %zu\n", CN_linked_list_length(list));
    
    // 测试访问元素
    printf("  测试访问元素...\n");
    for (size_t i = 0; i < CN_linked_list_length(list); i++)
    {
        int* value = (int*)CN_linked_list_get(list, i);
        if (value != NULL)
        {
            printf("    list[%zu] = %d\n", i, *value);
        }
    }
    
    // 测试头部添加
    printf("  测试头部添加元素...\n");
    int head_value = 0;
    if (CN_linked_list_prepend(list, &head_value))
    {
        printf("    在头部添加元素 0 成功\n");
    }
    
    // 测试中间插入
    printf("  测试中间插入元素...\n");
    int middle_value = 99;
    if (CN_linked_list_insert(list, 3, &middle_value))
    {
        printf("    在索引3插入元素 99 成功\n");
    }
    
    // 测试获取第一个和最后一个元素
    int* first = (int*)CN_linked_list_first(list);
    int* last = (int*)CN_linked_list_last(list);
    if (first != NULL && last != NULL)
    {
        printf("    第一个元素: %d, 最后一个元素: %d\n", *first, *last);
    }
    
    // 测试移除元素
    printf("  测试移除元素...\n");
    int removed_value;
    if (CN_linked_list_remove_first(list, &removed_value))
    {
        printf("    移除第一个元素: %d\n", removed_value);
    }
    
    if (CN_linked_list_remove_last(list, &removed_value))
    {
        printf("    移除最后一个元素: %d\n", removed_value);
    }
    
    if (CN_linked_list_remove(list, 2, &removed_value))
    {
        printf("    移除索引2的元素: %d\n", removed_value);
    }
    
    printf("  最终链表长度: %zu\n", CN_linked_list_length(list));
    
    // 清理
    CN_linked_list_destroy(list);
    printf("  单向链表销毁成功\n");
}

// 测试双向链表基本操作
void test_doubly_linked_list_basic(void)
{
    printf("  创建双向链表...\n");
    Stru_CN_LinkedList_t* list = CN_linked_list_create(Eum_LINKED_LIST_DOUBLY, sizeof(double));
    
    if (list == NULL)
    {
        printf("  错误: 无法创建双向链表\n");
        return;
    }
    
    printf("  双向链表创建成功\n");
    printf("  链表类型: %s\n", 
           CN_linked_list_type(list) == Eum_LINKED_LIST_SINGLY ? "单向链表" : "双向链表");
    
    // 测试添加元素
    printf("  测试添加元素...\n");
    for (double i = 1.0; i <= 5.0; i += 1.0)
    {
        if (CN_linked_list_append(list, &i))
        {
            printf("    添加元素 %.1f 成功\n", i);
        }
    }
    
    // 测试双向遍历
    printf("  测试双向访问...\n");
    printf("    正向遍历: ");
    for (size_t i = 0; i < CN_linked_list_length(list); i++)
    {
        double* value = (double*)CN_linked_list_get(list, i);
        if (value != NULL)
        {
            printf("%.1f ", *value);
        }
    }
    printf("\n");
    
    // 测试设置元素
    printf("  测试修改元素...\n");
    double new_value = 99.9;
    if (CN_linked_list_set(list, 2, &new_value))
    {
        printf("    修改索引2的元素为99.9成功\n");
    }
    
    // 测试查找元素
    printf("  测试查找元素...\n");
    double search_value = 3.0;
    size_t index = CN_linked_list_find(list, &search_value);
    if (index != SIZE_MAX)
    {
        printf("    找到元素 %.1f 在索引 %zu\n", search_value, index);
    }
    
    // 测试包含检查
    if (CN_linked_list_contains(list, &search_value))
    {
        printf("    链表包含元素 %.1f\n", search_value);
    }
    
    // 清理
    CN_linked_list_destroy(list);
    printf("  双向链表销毁成功\n");
}

// 测试链表高级操作
void test_linked_list_operations(void)
{
    printf("  创建两个链表进行高级操作测试...\n");
    
    // 创建第一个链表
    Stru_CN_LinkedList_t* list1 = CN_linked_list_create(Eum_LINKED_LIST_SINGLY, sizeof(int));
    Stru_CN_LinkedList_t* list2 = CN_linked_list_create(Eum_LINKED_LIST_SINGLY, sizeof(int));
    
    if (list1 == NULL || list2 == NULL)
    {
        printf("  错误: 无法创建链表\n");
        return;
    }
    
    // 填充数据
    for (int i = 1; i <= 3; i++)
    {
        CN_linked_list_append(list1, &i);
    }
    
    for (int i = 4; i <= 6; i++)
    {
        CN_linked_list_append(list2, &i);
    }
    
    printf("  链表1长度: %zu, 链表2长度: %zu\n", 
           CN_linked_list_length(list1), CN_linked_list_length(list2));
    
    // 测试复制链表
    printf("  测试复制链表...\n");
    Stru_CN_LinkedList_t* copy = CN_linked_list_copy(list1);
    if (copy != NULL)
    {
        printf("    链表复制成功，副本长度: %zu\n", CN_linked_list_length(copy));
        
        // 测试链表相等
        if (CN_linked_list_equal(list1, copy))
        {
            printf("    链表1和副本相等\n");
        }
        
        CN_linked_list_destroy(copy);
    }
    
    // 测试连接链表
    printf("  测试连接链表...\n");
    Stru_CN_LinkedList_t* concatenated = CN_linked_list_concat(list1, list2);
    if (concatenated != NULL)
    {
        printf("    链表连接成功，结果长度: %zu\n", CN_linked_list_length(concatenated));
        
        printf("    连接后的链表: ");
        for (size_t i = 0; i < CN_linked_list_length(concatenated); i++)
        {
            int* value = (int*)CN_linked_list_get(concatenated, i);
            if (value != NULL)
            {
                printf("%d ", *value);
            }
        }
        printf("\n");
        
        CN_linked_list_destroy(concatenated);
    }
    
    // 测试反转链表
    printf("  测试反转链表...\n");
    printf("    原始链表1: ");
    for (size_t i = 0; i < CN_linked_list_length(list1); i++)
    {
        int* value = (int*)CN_linked_list_get(list1, i);
        if (value != NULL)
        {
            printf("%d ", *value);
        }
    }
    printf("\n");
    
    if (CN_linked_list_reverse(list1))
    {
        printf("    链表反转成功\n");
        printf("    反转后链表1: ");
        for (size_t i = 0; i < CN_linked_list_length(list1); i++)
        {
            int* value = (int*)CN_linked_list_get(list1, i);
            if (value != NULL)
            {
                printf("%d ", *value);
            }
        }
        printf("\n");
    }
    
    // 测试清空链表
    printf("  测试清空链表...\n");
    CN_linked_list_clear(list1);
    printf("    清空后链表1长度: %zu\n", CN_linked_list_length(list1));
    
    // 清理
    CN_linked_list_destroy(list1);
    CN_linked_list_destroy(list2);
    printf("  链表高级操作测试完成\n");
}

// 测试链表迭代器
void test_linked_list_iterator(void)
{
    printf("  创建链表测试迭代器...\n");
    Stru_CN_LinkedList_t* list = CN_linked_list_create(Eum_LINKED_LIST_DOUBLY, sizeof(int));
    
    if (list == NULL)
    {
        printf("  错误: 无法创建链表\n");
        return;
    }
    
    // 填充数据
    for (int i = 1; i <= 5; i++)
    {
        CN_linked_list_append(list, &i);
    }
    
    printf("  测试正向迭代器...\n");
    Stru_CN_LinkedListIterator_t* iter = CN_linked_list_iterator_create(list);
    if (iter == NULL)
    {
        printf("  错误: 无法创建迭代器\n");
        CN_linked_list_destroy(list);
        return;
    }
    
    printf("    正向遍历: ");
    while (CN_linked_list_iterator_has_next(iter))
    {
        int* value = (int*)CN_linked_list_iterator_next(iter);
        if (value != NULL)
        {
            printf("%d ", *value);
        }
    }
    printf("\n");
    
    // 测试获取当前元素
    printf("    当前元素: ");
    int* current = (int*)CN_linked_list_iterator_current(iter);
    if (current != NULL)
    {
        printf("%d\n", *current);
    }
    else
    {
        printf("无（已到末尾）\n");
    }
    
    // 测试重置迭代器
    printf("  测试重置迭代器...\n");
    CN_linked_list_iterator_reset(iter);
    printf("    重置后当前元素: ");
    current = (int*)CN_linked_list_iterator_current(iter);
    if (current != NULL)
    {
        printf("%d\n", *current);
    }
    
    // 测试双向链表的反向迭代
    printf("  测试反向迭代器（仅双向链表）...\n");
    if (CN_linked_list_type(list) == Eum_LINKED_LIST_DOUBLY)
    {
        // 先移动到末尾
        while (CN_linked_list_iterator_has_next(iter))
        {
            CN_linked_list_iterator_next(iter);
        }
        
        printf("    反向遍历: ");
        while (CN_linked_list_iterator_has_prev(iter))
        {
            int* value = (int*)CN_linked_list_iterator_prev(iter);
            if (value != NULL)
            {
                printf("%d ", *value);
            }
        }
        printf("\n");
    }
    else
    {
        printf("    单向链表不支持反向迭代\n");
    }
    
    CN_linked_list_iterator_destroy(iter);
    CN_linked_list_destroy(list);
    printf("  链表迭代器测试完成\n");
}

// 自定义类型和函数
typedef struct
{
    int id;
    char name[32];
    float salary;
} Employee;

int compare_employees(const void* a, const void* b)
{
    const Employee* e1 = (const Employee*)a;
    const Employee* e2 = (const Employee*)b;
    return e1->id - e2->id;
}

void* copy_employee(const void* src)
{
    const Employee* e = (const Employee*)src;
    Employee* copy = malloc(sizeof(Employee));
    if (copy != NULL)
    {
        copy->id = e->id;
        strcpy(copy->name, e->name);
        copy->salary = e->salary;
    }
    return copy;
}

void free_employee(void* element)
{
    // 这里可以释放Employee内部的动态内存（如果有的话）
    // 当前Employee没有动态内存，所以只是释放结构体本身
    free(element);
}

// 测试自定义函数
void test_custom_functions(void)
{
    printf("  创建带自定义函数的链表...\n");
    
    Stru_CN_LinkedList_t* list = CN_linked_list_create_custom(
        Eum_LINKED_LIST_SINGLY,
        sizeof(Employee),
        compare_employees,
        free_employee,
        copy_employee);
    
    if (list == NULL)
    {
        printf("  错误: 无法创建自定义链表\n");
        return;
    }
    
    printf("  自定义链表创建成功\n");
    
    // 添加员工
    Employee e1 = {1001, "张三", 5000.0};
    Employee e2 = {1002, "李四", 6000.0};
    Employee e3 = {1003, "王五", 5500.0};
    
    CN_linked_list_append(list, &e1);
    CN_linked_list_append(list, &e2);
    CN_linked_list_append(list, &e3);
    
    printf("  添加了3名员工\n");
    
    // 查找员工
    Employee search_key = {1002, "", 0};
    size_t index = CN_linked_list_find(list, &search_key);
    if (index != SIZE_MAX)
    {
        Employee* found = (Employee*)CN_linked_list_get(list, index);
        printf("  找到员工: ID=%d, 姓名=%s, 工资=%.1f\n", 
               found->id, found->name, found->salary);
    }
    
    // 测试自定义比较函数
    printf("  测试自定义比较函数...\n");
    Employee e4 = {1001, "张三", 5000.0};
    if (CN_linked_list_contains(list, &e4))
    {
        printf("  链表包含员工ID 1001\n");
    }
    
    // 测试复制（会使用自定义复制函数）
    printf("  测试链表复制（使用自定义复制函数）...\n");
    Stru_CN_LinkedList_t* copy = CN_linked_list_copy(list);
    if (copy != NULL)
    {
        printf("  链表复制成功，副本长度: %zu\n", CN_linked_list_length(copy));
        
        // 比较两个链表
        if (CN_linked_list_equal(list, copy))
        {
            printf("  链表和副本相等\n");
        }
        
        CN_linked_list_destroy(copy);
    }
    
    CN_linked_list_destroy(list);
    printf("  自定义函数测试完成\n");
}

// 测试边界情况
void test_edge_cases(void)
{
    printf("  测试边界情况...\n");
    
    // 测试空链表
    printf("  测试空链表操作...\n");
    Stru_CN_LinkedList_t* empty_list = CN_linked_list_create(Eum_LINKED_LIST_SINGLY, sizeof(int));
    
    if (empty_list == NULL)
    {
        printf("  错误: 无法创建空链表\n");
        return;
    }
    
    printf("    空链表长度: %zu\n", CN_linked_list_length(empty_list));
    printf("    空链表是否为空: %s\n", CN_linked_list_is_empty(empty_list) ? "是" : "否");
    
    // 测试无效访问
    int* value = (int*)CN_linked_list_get(empty_list, 0);
    if (value == NULL)
    {
        printf("    获取空链表元素返回NULL（正确）\n");
    }
    
    // 测试无效索引
    value = (int*)CN_linked_list_get(empty_list, 10);
    if (value == NULL)
    {
        printf("    获取无效索引返回NULL（正确）\n");
    }
    
    // 测试无效移除
    int removed;
    if (!CN_linked_list_remove_first(empty_list, &removed))
    {
        printf("    从空链表移除元素失败（正确）\n");
    }
    
    if (!CN_linked_list_remove_last(empty_list, &removed))
    {
        printf("    从空链表移除最后一个元素失败（正确）\n");
    }
    
    // 测试无效插入
    int test_value = 42;
    if (!CN_linked_list_insert(empty_list, 10, &test_value))
    {
        printf("    在无效位置插入元素失败（正确）\n");
    }
    
    // 测试在空链表中插入（应该能工作）
    if (CN_linked_list_insert(empty_list, 0, &test_value))
    {
        printf("    在空链表位置0插入元素成功\n");
        printf("    插入后链表长度: %zu\n", CN_linked_list_length(empty_list));
    }
    
    // 测试NULL参数
    printf("  测试NULL参数处理...\n");
    Stru_CN_LinkedList_t* null_list = NULL;
    
    printf("    CN_linked_list_length(NULL) = %zu\n", CN_linked_list_length(null_list));
    printf("    CN_linked_list_is_empty(NULL) = %s\n", 
           CN_linked_list_is_empty(null_list) ? "true" : "false");
    
    // 测试零大小元素
    printf("  测试零大小元素链表...\n");
    Stru_CN_LinkedList_t* zero_size_list = CN_linked_list_create(Eum_LINKED_LIST_SINGLY, 0);
    if (zero_size_list == NULL)
    {
        printf("    创建零大小元素链表失败（正确）\n");
    }
    
    // 测试大链表
    printf("  测试大链表操作...\n");
    Stru_CN_LinkedList_t* large_list = CN_linked_list_create(Eum_LINKED_LIST_SINGLY, sizeof(int));
    if (large_list != NULL)
    {
        int count = 1000;
        for (int i = 0; i < count; i++)
        {
            CN_linked_list_append(large_list, &i);
        }
        
        printf("    创建了包含%d个元素的大链表\n", count);
        printf("    大链表长度: %zu\n", CN_linked_list_length(large_list));
        
        // 测试随机访问
        size_t random_index = 500;
        int* random_value = (int*)CN_linked_list_get(large_list, random_index);
        if (random_value != NULL)
        {
            printf("    随机访问索引%zu的元素: %d\n", random_index, *random_value);
        }
        
        CN_linked_list_destroy(large_list);
        printf("    大链表销毁成功\n");
    }
    
    // 清理
    CN_linked_list_destroy(empty_list);
    printf("  边界情况测试完成\n");
}
