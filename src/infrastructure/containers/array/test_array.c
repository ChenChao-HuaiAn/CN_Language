/******************************************************************************
 * 文件名: test_array.c
 * 功能: CN_array模块测试程序
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，测试CN_array模块功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_array.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

// 测试函数声明
void test_basic_operations(void);
void test_custom_type_array(void);
void test_array_operations(void);
void test_search_and_sort(void);
void test_iterator(void);

int main(void)
{
    printf("=== CN_array 模块测试 ===\n\n");
    
    printf("1. 测试基本操作:\n");
    test_basic_operations();
    
    printf("\n2. 测试自定义类型数组:\n");
    test_custom_type_array();
    
    printf("\n3. 测试数组操作:\n");
    test_array_operations();
    
    printf("\n4. 测试搜索和排序:\n");
    test_search_and_sort();
    
    printf("\n5. 测试迭代器:\n");
    test_iterator();
    
    printf("\n=== 所有测试完成 ===\n");
    return 0;
}

// 测试基本操作
void test_basic_operations(void)
{
    // 创建整数数组
    Stru_CN_Array_t* array = CN_array_create(Eum_ARRAY_ELEMENT_INT32, sizeof(int), 5);
    
    if (array == NULL)
    {
        printf("  错误: 无法创建数组\n");
        return;
    }
    
    printf("  创建数组成功\n");
    printf("  初始长度: %llu, 容量: %llu\n", 
           (unsigned long long)CN_array_length(array), 
           (unsigned long long)CN_array_capacity(array));
    
    // 添加元素
    for (int i = 1; i <= 10; i++)
    {
        if (CN_array_append(array, &i))
        {
            printf("  添加元素 %d 成功\n", i);
        }
        else
        {
            printf("  添加元素 %d 失败\n", i);
        }
    }
    
    printf("  最终长度: %llu, 容量: %llu\n", 
           (unsigned long long)CN_array_length(array), 
           (unsigned long long)CN_array_capacity(array));
    
    // 访问元素
    for (size_t i = 0; i < CN_array_length(array); i++)
    {
        int* value = (int*)CN_array_get(array, i);
        if (value != NULL)
        {
            printf("  array[%llu] = %d\n", (unsigned long long)i, *value);
        }
    }
    
    // 修改元素
    int new_value = 100;
    if (CN_array_set(array, 5, &new_value))
    {
        printf("  修改索引5的元素为100成功\n");
    }
    
    // 删除元素
    if (CN_array_remove(array, 0))
    {
        printf("  删除第一个元素成功\n");
    }
    
    printf("  删除后长度: %llu\n", (unsigned long long)CN_array_length(array));
    
    // 清理
    CN_array_destroy(array);
    printf("  数组销毁成功\n");
}

// 自定义类型
typedef struct
{
    int id;
    char name[20];
    float score;
} Student;

int compare_students(const void* a, const void* b)
{
    const Student* s1 = (const Student*)a;
    const Student* s2 = (const Student*)b;
    return s1->id - s2->id;
}

void* copy_student(const void* src)
{
    const Student* s = (const Student*)src;
    Student* copy = malloc(sizeof(Student));
    if (copy != NULL)
    {
        copy->id = s->id;
        strcpy(copy->name, s->name);
        copy->score = s->score;
    }
    return copy;
}

void free_student(void* element)
{
    // 对于Student结构体，没有需要特别释放的资源
    // 这里只是演示自定义释放函数
    (void)element;
}

// 测试自定义类型数组
void test_custom_type_array(void)
{
    // 创建自定义类型数组
    Stru_CN_Array_t* students = CN_array_create_custom(
        sizeof(Student), 3, compare_students, free_student, copy_student);
    
    if (students == NULL)
    {
        printf("  错误: 无法创建自定义类型数组\n");
        return;
    }
    
    printf("  创建自定义类型数组成功\n");
    
    // 添加学生
    Student s1 = {101, "张三", 85.5};
    Student s2 = {102, "李四", 92.0};
    Student s3 = {103, "王五", 78.5};
    
    CN_array_append(students, &s1);
    CN_array_append(students, &s2);
    CN_array_append(students, &s3);
    
    printf("  添加了3个学生\n");
    
    // 查找学生
    Student search_key = {102, "", 0};
    size_t index = CN_array_find(students, &search_key);
    if (index != SIZE_MAX)
    {
        Student* found = (Student*)CN_array_get(students, index);
        printf("  找到学生: ID=%d, 姓名=%s, 分数=%.1f\n", 
               found->id, found->name, found->score);
    }
    
    // 复制数组
    Stru_CN_Array_t* copy = CN_array_copy(students);
    if (copy != NULL)
    {
        printf("  数组复制成功，副本长度: %llu\n", (unsigned long long)CN_array_length(copy));
        CN_array_destroy(copy);
    }
    
    CN_array_destroy(students);
    printf("  自定义类型数组销毁成功\n");
}

// 测试数组操作
void test_array_operations(void)
{
    // 创建两个数组
    Stru_CN_Array_t* array1 = CN_array_create(Eum_ARRAY_ELEMENT_INT32, sizeof(int), 0);
    Stru_CN_Array_t* array2 = CN_array_create(Eum_ARRAY_ELEMENT_INT32, sizeof(int), 0);
    
    if (array1 == NULL || array2 == NULL)
    {
        printf("  错误: 无法创建数组\n");
        return;
    }
    
    // 填充数据
    for (int i = 1; i <= 5; i++)
    {
        CN_array_append(array1, &i);
    }
    
    for (int i = 6; i <= 10; i++)
    {
        CN_array_append(array2, &i);
    }
    
    printf("  数组1长度: %llu, 数组2长度: %llu\n", 
           (unsigned long long)CN_array_length(array1), 
           (unsigned long long)CN_array_length(array2));
    
    // 连接数组
    Stru_CN_Array_t* concatenated = CN_array_concat(array1, array2);
    if (concatenated != NULL)
    {
        printf("  数组连接成功，结果长度: %llu\n", (unsigned long long)CN_array_length(concatenated));
        
        // 获取子数组
        Stru_CN_Array_t* slice = CN_array_slice(concatenated, 3, 7);
        if (slice != NULL)
        {
            printf("  获取子数组成功，切片长度: %llu\n", (unsigned long long)CN_array_length(slice));
            CN_array_destroy(slice);
        }
        
        CN_array_destroy(concatenated);
    }
    
    // 测试交换元素
    if (CN_array_length(array1) >= 2)
    {
        CN_array_swap(array1, 0, 1);
        printf("  交换数组1的前两个元素成功\n");
    }
    
    // 测试填充
    int fill_value = 99;
    if (CN_array_fill(array1, &fill_value, 3))
    {
        printf("  填充数组1的前3个元素为99成功\n");
    }
    
    CN_array_destroy(array1);
    CN_array_destroy(array2);
    printf("  数组操作测试完成\n");
}

// 测试搜索和排序
void test_search_and_sort(void)
{
    Stru_CN_Array_t* array = CN_array_create(Eum_ARRAY_ELEMENT_INT32, sizeof(int), 0);
    
    if (array == NULL)
    {
        printf("  错误: 无法创建数组\n");
        return;
    }
    
    // 添加无序数据
    int values[] = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
    for (int i = 0; i < 10; i++)
    {
        CN_array_append(array, &values[i]);
    }
    
    printf("  原始数组: ");
    for (size_t i = 0; i < CN_array_length(array); i++)
    {
        int* val = (int*)CN_array_get(array, i);
        printf("%d ", *val);
    }
    printf("\n");
    
    // 搜索元素
    int search_val = 7;
    size_t found_index = CN_array_find(array, &search_val);
    if (found_index != SIZE_MAX)
    {
        printf("  找到元素 %d 在索引 %llu\n", search_val, (unsigned long long)found_index);
    }
    
    // 排序
    CN_array_sort(array);
    printf("  排序后数组: ");
    for (size_t i = 0; i < CN_array_length(array); i++)
    {
        int* val = (int*)CN_array_get(array, i);
        printf("%d ", *val);
    }
    printf("\n");
    
    // 反转
    CN_array_reverse(array);
    printf("  反转后数组: ");
    for (size_t i = 0; i < CN_array_length(array); i++)
    {
        int* val = (int*)CN_array_get(array, i);
        printf("%d ", *val);
    }
    printf("\n");
    
    CN_array_destroy(array);
    printf("  搜索和排序测试完成\n");
}

// 测试迭代器
void test_iterator(void)
{
    Stru_CN_Array_t* array = CN_array_create(Eum_ARRAY_ELEMENT_INT32, sizeof(int), 0);
    
    if (array == NULL)
    {
        printf("  错误: 无法创建数组\n");
        return;
    }
    
    // 添加数据
    for (int i = 1; i <= 5; i++)
    {
        CN_array_append(array, &i);
    }
    
    printf("  使用迭代器遍历数组:\n");
    
    // 创建迭代器
    Stru_CN_ArrayIterator_t* iter = CN_array_iterator_create(array);
    if (iter == NULL)
    {
        printf("  错误: 无法创建迭代器\n");
        CN_array_destroy(array);
        return;
    }
    
    // 遍历
    while (CN_array_iterator_has_next(iter))
    {
        int* value = (int*)CN_array_iterator_next(iter);
        if (value != NULL)
        {
            printf("    元素: %d\n", *value);
        }
    }
    
    // 重置迭代器
    CN_array_iterator_reset(iter);
    printf("  重置迭代器后第一个元素: ");
    int* first = (int*)CN_array_iterator_current(iter);
    if (first != NULL)
    {
        printf("%d\n", *first);
    }
    
    CN_array_iterator_destroy(iter);
    CN_array_destroy(array);
    printf("  迭代器测试完成\n");
}
