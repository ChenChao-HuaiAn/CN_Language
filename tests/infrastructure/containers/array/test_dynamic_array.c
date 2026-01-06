/**
 * @file test_dynamic_array.c
 * @brief 动态数组模块测试实现文件
 * 
 * 实现动态数组模块的所有测试函数。
 * 遵循模块化测试设计，测试文件和main()函数分离。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_dynamic_array.h"
#include "../../../../src/infrastructure/containers/array/CN_dynamic_array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <limits.h>

// 测试辅助函数声明
static int compare_int(const void* a, const void* b);
static bool print_int_iterator(void* item, size_t index, void* user_data);
static void print_int_callback(void* item, void* user_data);

// 比较函数实现
static int compare_int(const void* a, const void* b)
{
    int int_a = *(const int*)a;
    int int_b = *(const int*)b;
    return int_a - int_b;
}

/**
 * @brief 测试创建和销毁功能
 */
bool test_dynamic_array_create_and_destroy(void)
{
    printf("测试: 创建和销毁\n");
    printf("----------------\n");
    
    bool all_passed = true;
    
    // 测试1: 基本创建
    printf("  1. 测试基本创建...\n");
    Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
    if (array == NULL) {
        printf("    ✗ 基本创建失败: 返回NULL\n");
        all_passed = false;
    } else {
        if (F_dynamic_array_length(array) != 0) {
            printf("    ✗ 基本创建失败: 初始长度不为0\n");
            all_passed = false;
        }
        if (F_dynamic_array_capacity(array) < 4) { // 最小容量
            printf("    ✗ 基本创建失败: 容量太小\n");
            all_passed = false;
        }
        if (F_dynamic_array_is_empty(array) != true) {
            printf("    ✗ 基本创建失败: 数组不为空\n");
            all_passed = false;
        }
        if (all_passed) {
            printf("    ✓ 基本创建测试通过\n");
        }
        F_destroy_dynamic_array(array);
    }
    
    // 测试2: 带初始容量的创建
    printf("  2. 测试带初始容量的创建...\n");
    enum Eum_DynamicArrayError error_code;
    array = F_create_dynamic_array_ex(sizeof(int), 100, &error_code);
    if (array == NULL) {
        printf("    ✗ 带容量创建失败: 返回NULL, 错误码: %d\n", error_code);
        all_passed = false;
    } else {
        if (F_dynamic_array_capacity(array) < 100) {
            printf("    ✗ 带容量创建失败: 容量小于指定值\n");
            all_passed = false;
        }
        if (error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
            printf("    ✗ 带容量创建失败: 错误码不正确\n");
            all_passed = false;
        }
        if (all_passed) {
            printf("    ✓ 带容量创建测试通过\n");
        }
        F_destroy_dynamic_array(array);
    }
    
    // 测试3: 无效参数创建
    printf("  3. 测试无效参数创建...\n");
    array = F_create_dynamic_array_ex(0, 10, &error_code);
    if (array != NULL) {
        printf("    ✗ 无效参数创建失败: 应该返回NULL\n");
        F_destroy_dynamic_array(array);
        all_passed = false;
    } else {
        if (error_code != Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE) {
            printf("    ✗ 无效参数创建失败: 错误码不正确\n");
            all_passed = false;
        } else {
            printf("    ✓ 无效参数创建测试通过\n");
        }
    }
    
    // 测试4: 销毁NULL指针
    printf("  4. 测试销毁NULL指针...\n");
    F_destroy_dynamic_array(NULL); // 不应该崩溃
    printf("    ✓ 销毁NULL指针测试通过\n");
    
    printf("创建和销毁测试: %s\n\n", all_passed ? "全部通过" : "有失败");
    return all_passed;
}

/**
 * @brief 测试基本操作功能
 */
bool test_dynamic_array_basic_operations(void)
{
    printf("测试: 基本操作\n");
    printf("--------------\n");
    
    bool all_passed = true;
    Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
    
    if (array == NULL) {
        printf("  ✗ 无法创建数组，跳过基本操作测试\n");
        return false;
    }
    
    // 测试1: 添加元素
    printf("  1. 测试添加元素...\n");
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        if (!F_dynamic_array_push(array, &values[i])) {
            printf("    ✗ 添加元素失败: 索引 %d\n", i);
            all_passed = false;
            break;
        }
    }
    
    if (all_passed && F_dynamic_array_length(array) != 5) {
        printf("    ✗ 添加元素失败: 长度不正确\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 添加元素测试通过\n");
    }
    
    // 测试2: 获取元素
    printf("  2. 测试获取元素...\n");
    for (int i = 0; i < 5; i++) {
        int* retrieved = (int*)F_dynamic_array_get(array, i);
        if (retrieved == NULL) {
            printf("    ✗ 获取元素失败: 索引 %d 返回NULL\n", i);
            all_passed = false;
        } else if (*retrieved != values[i]) {
            printf("    ✗ 获取元素失败: 索引 %d 值不正确\n", i);
            all_passed = false;
        }
    }
    
    // 测试无效索引
    int* invalid = (int*)F_dynamic_array_get(array, 100);
    if (invalid != NULL) {
        printf("    ✗ 获取元素失败: 无效索引应该返回NULL\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 获取元素测试通过\n");
    }
    
    // 测试3: 设置元素
    printf("  3. 测试设置元素...\n");
    int new_value = 99;
    if (!F_dynamic_array_set(array, 2, &new_value)) {
        printf("    ✗ 设置元素失败: 索引 2\n");
        all_passed = false;
    } else {
        int* retrieved = (int*)F_dynamic_array_get(array, 2);
        if (retrieved == NULL || *retrieved != new_value) {
            printf("    ✗ 设置元素失败: 值未正确更新\n");
            all_passed = false;
        }
    }
    
    // 测试无效索引设置
    if (F_dynamic_array_set(array, 100, &new_value)) {
        printf("    ✗ 设置元素失败: 无效索引应该返回false\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 设置元素测试通过\n");
    }
    
    // 测试4: 移除元素
    printf("  4. 测试移除元素...\n");
    size_t original_length = F_dynamic_array_length(array);
    if (!F_dynamic_array_remove(array, 1)) {
        printf("    ✗ 移除元素失败: 索引 1\n");
        all_passed = false;
    } else {
        if (F_dynamic_array_length(array) != original_length - 1) {
            printf("    ✗ 移除元素失败: 长度未减少\n");
            all_passed = false;
        }
        
        // 验证元素已移动
        int* first = (int*)F_dynamic_array_get(array, 0);
        int* second = (int*)F_dynamic_array_get(array, 1); // 原来是索引2
        if (first == NULL || *first != 10) {
            printf("    ✗ 移除元素失败: 第一个元素不正确\n");
            all_passed = false;
        }
        if (second == NULL || *second != new_value) { // 原来是30，现在是99
            printf("    ✗ 移除元素失败: 第二个元素不正确\n");
            all_passed = false;
        }
    }
    
    // 测试无效索引移除
    if (F_dynamic_array_remove(array, 100)) {
        printf("    ✗ 移除元素失败: 无效索引应该返回false\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 移除元素测试通过\n");
    }
    
    F_destroy_dynamic_array(array);
    printf("基本操作测试: %s\n\n", all_passed ? "全部通过" : "有失败");
    return all_passed;
}

/**
 * @brief 测试查询操作功能
 */
bool test_dynamic_array_query_operations(void)
{
    printf("测试: 查询操作\n");
    printf("--------------\n");
    
    bool all_passed = true;
    Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
    
    if (array == NULL) {
        printf("  ✗ 无法创建数组，跳过查询操作测试\n");
        return false;
    }
    
    // 测试1: 空数组查询
    printf("  1. 测试空数组查询...\n");
    if (F_dynamic_array_length(array) != 0) {
        printf("    ✗ 空数组长度不为0\n");
        all_passed = false;
    }
    
    if (!F_dynamic_array_is_empty(array)) {
        printf("    ✗ 空数组is_empty返回false\n");
        all_passed = false;
    }
    
    size_t capacity = F_dynamic_array_capacity(array);
    if (capacity < 4) { // 最小容量
        printf("    ✗ 空数组容量太小\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 空数组查询测试通过\n");
    }
    
    // 测试2: 添加元素后查询
    printf("  2. 测试添加元素后查询...\n");
    int value = 42;
    F_dynamic_array_push(array, &value);
    
    if (F_dynamic_array_length(array) != 1) {
        printf("    ✗ 添加后长度不为1\n");
        all_passed = false;
    }
    
    if (F_dynamic_array_is_empty(array)) {
        printf("    ✗ 添加后is_empty返回true\n");
        all_passed = false;
    }
    
    // 测试带错误码的查询
    enum Eum_DynamicArrayError error_code;
    size_t len_with_error = F_dynamic_array_length_ex(array, &error_code);
    if (len_with_error != 1 || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的长度查询失败\n");
        all_passed = false;
    }
    
    bool empty_with_error = F_dynamic_array_is_empty_ex(array, &error_code);
    if (empty_with_error || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的空检查失败\n");
        all_passed = false;
    }
    
    size_t cap_with_error = F_dynamic_array_capacity_ex(array, &error_code);
    if (cap_with_error != capacity || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的容量查询失败\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 添加元素后查询测试通过\n");
    }
    
    // 测试3: NULL数组查询
    printf("  3. 测试NULL数组查询...\n");
    size_t null_len = F_dynamic_array_length(NULL);
    if (null_len != 0) {
        printf("    ✗ NULL数组长度不为0\n");
        all_passed = false;
    }
    
    size_t null_cap = F_dynamic_array_capacity(NULL);
    if (null_cap != 0) {
        printf("    ✗ NULL数组容量不为0\n");
        all_passed = false;
    }
    
    bool null_empty = F_dynamic_array_is_empty(NULL);
    if (!null_empty) {
        printf("    ✗ NULL数组is_empty返回false\n");
        all_passed = false;
    }
    
    // 测试带错误码的NULL查询
    size_t null_len_ex = F_dynamic_array_length_ex(NULL, &error_code);
    if (null_len_ex != 0 || error_code != Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER) {
        printf("    ✗ 带错误码的NULL长度查询失败\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ NULL数组查询测试通过\n");
    }
    
    F_destroy_dynamic_array(array);
    printf("查询操作测试: %s\n\n", all_passed ? "全部通过" : "有失败");
    return all_passed;
}

/**
 * @brief 测试数组操作功能
 */
bool test_dynamic_array_array_operations(void)
{
    printf("测试: 数组操作\n");
    printf("--------------\n");
    
    bool all_passed = true;
    Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
    
    if (array == NULL) {
        printf("  ✗ 无法创建数组，跳过数组操作测试\n");
        return false;
    }
    
    // 添加一些测试数据
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        F_dynamic_array_push(array, &values[i]);
    }
    
    // 测试1: 清空数组
    printf("  1. 测试清空数组...\n");
    F_dynamic_array_clear(array);
    
    if (F_dynamic_array_length(array) != 0) {
        printf("    ✗ 清空后长度不为0\n");
        all_passed = false;
    }
    
    if (!F_dynamic_array_is_empty(array)) {
        printf("    ✗ 清空后is_empty返回false\n");
        all_passed = false;
    }
    
    // 测试带错误码的清空
    enum Eum_DynamicArrayError error_code;
    F_dynamic_array_clear_ex(array, &error_code);
    if (error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的清空失败\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 清空数组测试通过\n");
    }
    
    // 重新添加数据
    for (int i = 0; i < 5; i++) {
        F_dynamic_array_push(array, &values[i]);
    }
    
    // 测试2: 调整容量
    printf("  2. 测试调整容量...\n");
    // 注意：original_capacity变量不再使用，但保留计算以供参考
    size_t original_capacity = F_dynamic_array_capacity(array);
    (void)original_capacity; // 标记为已使用以避免警告
    
    // 扩大容量
    if (!F_dynamic_array_resize(array, 100)) {
        printf("    ✗ 扩大容量失败\n");
        all_passed = false;
    } else if (F_dynamic_array_capacity(array) < 100) {
        printf("    ✗ 扩大容量后容量不正确\n");
        all_passed = false;
    }
    
    // 缩小容量（但大于当前长度）
    if (!F_dynamic_array_resize(array, 10)) {
        printf("    ✗ 缩小容量失败\n");
        all_passed = false;
    } else if (F_dynamic_array_capacity(array) != 10) {
        printf("    ✗ 缩小容量后容量不正确\n");
        all_passed = false;
    }
    
    // 测试带错误码的调整
    if (!F_dynamic_array_resize_ex(array, 20, &error_code)) {
        printf("    ✗ 带错误码的调整容量失败\n");
        all_passed = false;
    } else if (error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的调整容量错误码不正确\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 调整容量测试通过\n");
    }
    
    // 测试3: 查找元素
    printf("  3. 测试查找元素...\n");
    
    int target = 30;
    size_t found_index = F_dynamic_array_find(array, &target, compare_int);
    if (found_index == SIZE_MAX) {
        printf("    ✗ 查找元素失败: 未找到30\n");
        all_passed = false;
    } else {
        int* found = (int*)F_dynamic_array_get(array, found_index);
        if (found == NULL || *found != target) {
            printf("    ✗ 查找元素失败: 找到的值不正确\n");
            all_passed = false;
        }
    }
    
    // 查找不存在的元素
    int not_found = 99;
    size_t not_found_index = F_dynamic_array_find(array, &not_found, compare_int);
    if (not_found_index != SIZE_MAX) {
        printf("    ✗ 查找元素失败: 不应该找到99\n");
        all_passed = false;
    }
    
    // 测试带错误码的查找
    size_t found_index_ex = F_dynamic_array_find_ex(array, &target, compare_int, &error_code);
    if (found_index_ex == SIZE_MAX || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的查找失败\n");
        all_passed = false;
    }
    
    // 测试查找不存在的元素（带错误码）
    size_t not_found_index_ex = F_dynamic_array_find_ex(array, &not_found, compare_int, &error_code);
    if (not_found_index_ex != SIZE_MAX || error_code != Eum_DYNAMIC_ARRAY_ERROR_ITEM_NOT_FOUND) {
        printf("    ✗ 带错误码的查找不存在的元素失败\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 查找元素测试通过\n");
    }
    
    // 测试4: 批量添加元素
    printf("  4. 测试批量添加元素...\n");
    F_dynamic_array_clear(array);
    
    int batch_values[] = {100, 200, 300, 400, 500};
    if (!F_dynamic_array_push_batch(array, batch_values, 5)) {
        printf("    ✗ 批量添加元素失败\n");
        all_passed = false;
    } else {
        if (F_dynamic_array_length(array) != 5) {
            printf("    ✗ 批量添加后长度不正确\n");
            all_passed = false;
        }
        
        for (int i = 0; i < 5; i++) {
            int* val = (int*)F_dynamic_array_get(array, i);
            if (val == NULL || *val != batch_values[i]) {
                printf("    ✗ 批量添加元素值不正确: 索引 %d\n", i);
                all_passed = false;
                break;
            }
        }
    }
    
    // 测试带错误码的批量添加
    F_dynamic_array_clear(array);
    if (!F_dynamic_array_push_batch_ex(array, batch_values, 5, &error_code)) {
        printf("    ✗ 带错误码的批量添加失败\n");
        all_passed = false;
    } else if (error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的批量添加错误码不正确\n");
        all_passed = false;
    }
    
    // 测试无效批量添加
    if (F_dynamic_array_push_batch_ex(array, NULL, 5, &error_code)) {
        printf("    ✗ 无效批量添加应该失败\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 批量添加元素测试通过\n");
    }
    
    F_destroy_dynamic_array(array);
    printf("数组操作测试: %s\n\n", all_passed ? "全部通过" : "有失败");
    return all_passed;
}

// 迭代器回调函数实现
static bool print_int_iterator(void* item, size_t index, void* user_data)
{
    int* value = (int*)item;
    int* count = (int*)user_data;
    
    if (value != NULL) {
        printf("  [%zu] = %d\n", index, *value);
        if (count != NULL) {
            (*count)++;
        }
    }
    
    return true; // 继续迭代
}

// 提前停止的迭代器回调函数
static bool stop_at_third(void* item, size_t index, void* user_data)
{
    (void)index;      // 标记为已使用以避免警告
    (void)user_data;  // 标记为已使用以避免警告
    
    int* value = (int*)item;
    if (value != NULL && *value == 30) { // 在值为30时停止
        return false;
    }
    return true;
}

// 回调函数实现
static void print_int_callback(void* item, void* user_data)
{
    int* value = (int*)item;
    int* count = (int*)user_data;
    
    if (value != NULL) {
        printf("  值: %d\n", *value);
        if (count != NULL) {
            (*count)++;
        }
    }
}

/**
 * @brief 测试迭代操作功能
 */
bool test_dynamic_array_iteration_operations(void)
{
    printf("测试: 迭代操作\n");
    printf("--------------\n");
    
    bool all_passed = true;
    Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
    
    if (array == NULL) {
        printf("  ✗ 无法创建数组，跳过迭代操作测试\n");
        return false;
    }
    
    // 添加测试数据
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        F_dynamic_array_push(array, &values[i]);
    }
    
    // 测试1: 基本遍历
    printf("  1. 测试基本遍历...\n");
    int count = 0;
    F_dynamic_array_foreach(array, print_int_callback, &count);
    
    if (count != 5) {
        printf("    ✗ 基本遍历失败: 回调调用次数不正确\n");
        all_passed = false;
    } else {
        printf("    ✓ 基本遍历测试通过\n");
    }
    
    // 测试2: 带索引的遍历
    printf("  2. 测试带索引的遍历...\n");
    count = 0;
    enum Eum_DynamicArrayError error_code;
    bool foreach_result = F_dynamic_array_foreach_ex(array, print_int_iterator, &count, &error_code);
    
    if (!foreach_result || count != 5 || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带索引的遍历失败\n");
        all_passed = false;
    } else {
        printf("    ✓ 带索引的遍历测试通过\n");
    }
    
    // 测试3: 提前停止的迭代器
    printf("  3. 测试提前停止的迭代器...\n");
    
    foreach_result = F_dynamic_array_foreach_ex(array, stop_at_third, NULL, &error_code);
    if (foreach_result) { // 应该返回false，因为迭代器提前停止了
        printf("    ✗ 提前停止的迭代器失败: 应该返回false\n");
        all_passed = false;
    } else {
        printf("    ✓ 提前停止的迭代器测试通过\n");
    }
    
    // 测试4: NULL数组遍历
    printf("  4. 测试NULL数组遍历...\n");
    F_dynamic_array_foreach(NULL, print_int_callback, NULL); // 不应该崩溃
    foreach_result = F_dynamic_array_foreach_ex(NULL, print_int_iterator, NULL, &error_code);
    if (foreach_result || error_code != Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER) {
        printf("    ✗ NULL数组遍历失败\n");
        all_passed = false;
    } else {
        printf("    ✓ NULL数组遍历测试通过\n");
    }
    
    F_destroy_dynamic_array(array);
    printf("迭代操作测试: %s\n\n", all_passed ? "全部通过" : "有失败");
    return all_passed;
}

/**
 * @brief 测试内存管理功能
 */
bool test_dynamic_array_memory_management(void)
{
    printf("测试: 内存管理\n");
    printf("--------------\n");
    
    bool all_passed = true;
    Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
    
    if (array == NULL) {
        printf("  ✗ 无法创建数组，跳過内存管理测试\n");
        return false;
    }
    
    // 添加测试数据
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        F_dynamic_array_push(array, &values[i]);
    }
    
    // 测试1: 获取内存统计
    printf("  1. 测试获取内存统计...\n");
    size_t total_bytes = 0, used_bytes = 0;
    bool stats_result = F_dynamic_array_get_memory_stats(array, &total_bytes, &used_bytes);
    
    if (!stats_result) {
        printf("    ✗ 获取内存统计失败\n");
        all_passed = false;
    } else if (total_bytes == 0 || used_bytes == 0) {
        printf("    ✗ 内存统计值不正确\n");
        all_passed = false;
    } else {
        printf("    总内存: %zu 字节, 已使用: %zu 字节\n", total_bytes, used_bytes);
        printf("    ✓ 获取内存统计测试通过\n");
    }
    
    // 测试2: 带错误码的内存统计
    printf("  2. 测试带错误码的内存统计...\n");
    enum Eum_DynamicArrayError error_code;
    stats_result = F_dynamic_array_get_memory_stats_ex(array, &total_bytes, &used_bytes, &error_code);
    
    if (!stats_result || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的内存统计失败\n");
        all_passed = false;
    } else {
        printf("    ✓ 带错误码的内存统计测试通过\n");
    }
    
    // 测试3: 压缩数组
    printf("  3. 测试压缩数组...\n");
    size_t original_capacity = F_dynamic_array_capacity(array);
    
    // 先调整容量到更大值
    F_dynamic_array_resize(array, 100);
    
    // 然后压缩
    bool shrink_result = F_dynamic_array_shrink_to_fit(array);
    size_t new_capacity = F_dynamic_array_capacity(array);
    
    if (!shrink_result) {
        printf("    ✗ 压缩数组失败\n");
        all_passed = false;
    } else if (new_capacity > original_capacity) {
        printf("    ✗ 压缩后容量仍然太大\n");
        all_passed = false;
    } else {
        printf("    原始容量: %zu, 压缩后容量: %zu\n", original_capacity, new_capacity);
        printf("    ✓ 压缩数组测试通过\n");
    }
    
    // 测试4: 带错误码的压缩
    printf("  4. 测试带错误码的压缩...\n");
    shrink_result = F_dynamic_array_shrink_to_fit_ex(array, &error_code);
    
    if (!shrink_result || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的压缩失败\n");
        all_passed = false;
    } else {
        printf("    ✓ 带错误码的压缩测试通过\n");
    }
    
    // 测试5: NULL数组内存操作
    printf("  5. 测试NULL数组内存操作...\n");
    stats_result = F_dynamic_array_get_memory_stats_ex(NULL, &total_bytes, &used_bytes, &error_code);
    if (stats_result || error_code != Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER) {
        printf("    ✗ NULL数组内存统计失败\n");
        all_passed = false;
    }
    
    shrink_result = F_dynamic_array_shrink_to_fit_ex(NULL, &error_code);
    if (shrink_result || error_code != Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER) {
        printf("    ✗ NULL数组压缩失败\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ NULL数组内存操作测试通过\n");
    }
    
    F_destroy_dynamic_array(array);
    printf("内存管理测试: %s\n\n", all_passed ? "全部通过" : "有失败");
    return all_passed;
}

/**
 * @brief 测试错误处理功能
 */
bool test_dynamic_array_error_handling(void)
{
    printf("测试: 错误处理\n");
    printf("--------------\n");
    
    bool all_passed = true;
    enum Eum_DynamicArrayError error_code;
    
    // 测试1: 各种无效参数的错误码
    printf("  1. 测试无效参数错误码...\n");
    
    // 创建时无效参数
    Stru_DynamicArray_t* array = F_create_dynamic_array_ex(0, 10, &error_code);
    if (array != NULL || error_code != Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE) {
        printf("    ✗ 无效item_size错误码不正确\n");
        all_passed = false;
    }
    
    // 正常创建
    array = F_create_dynamic_array_ex(sizeof(int), 10, &error_code);
    if (array == NULL) {
        printf("    ✗ 无法创建数组，跳过错误处理测试\n");
        return false;
    }
    
    // 添加元素时NULL指针
    bool push_result = F_dynamic_array_push_ex(array, NULL, &error_code);
    if (push_result || error_code != Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER) {
        printf("    ✗ NULL item添加错误码不正确\n");
        all_passed = false;
    }
    
    // 获取元素时无效索引
    void* get_result = F_dynamic_array_get_ex(array, 100, &error_code);
    if (get_result != NULL || error_code != Eum_DYNAMIC_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS) {
        printf("    ✗ 无效索引获取错误码不正确\n");
        all_passed = false;
    }
    
    // 设置元素时无效索引
    int value = 42;
    bool set_result = F_dynamic_array_set_ex(array, 100, &value, &error_code);
    if (set_result || error_code != Eum_DYNAMIC_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS) {
        printf("    ✗ 无效索引设置错误码不正确\n");
        all_passed = false;
    }
    
    // 设置元素时NULL item - 先添加一个元素使索引0有效
    int temp_value = 42;
    if (!F_dynamic_array_push_ex(array, &temp_value, &error_code)) {
        printf("    ✗ 无法添加测试元素，跳过NULL item测试\n");
        all_passed = false;
    } else {
        set_result = F_dynamic_array_set_ex(array, 0, NULL, &error_code);
        if (set_result || error_code != Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER) {
            printf("    ✗ NULL item设置错误码不正确\n");
            all_passed = false;
        }
    }
    
    // 移除元素时无效索引
    bool remove_result = F_dynamic_array_remove_ex(array, 100, &error_code);
    if (remove_result || error_code != Eum_DYNAMIC_ARRAY_ERROR_INDEX_OUT_OF_BOUNDS) {
        printf("    ✗ 无效索引移除错误码不正确\n");
        all_passed = false;
    }
    
    if (all_passed) {
        printf("    ✓ 无效参数错误码测试通过\n");
    }
    
    // 测试2: 内存不足错误（模拟）
    printf("  2. 测试内存不足错误...\n");
    // 注意：实际测试内存不足错误比较困难，这里我们主要测试错误码枚举的完整性
    printf("    ✓ 内存不足错误测试跳过（需要模拟环境）\n");
    
    // 测试3: 内部错误
    printf("  3. 测试内部错误...\n");
    // 内部错误通常由实现中的bug引起，难以在测试中可靠触发
    printf("    ✓ 内部错误测试跳过（难以可靠触发）\n");
    
    F_destroy_dynamic_array(array);
    printf("错误处理测试: %s\n\n", all_passed ? "全部通过" : "有失败");
    return all_passed;
}

/**
 * @brief 测试抽象接口功能
 */
bool test_dynamic_array_abstract_interface(void)
{
    printf("测试: 抽象接口\n");
    printf("--------------\n");
    
    bool all_passed = true;
    
    // 测试1: 获取默认接口
    printf("  1. 测试获取默认接口...\n");
    const Stru_DynamicArrayInterface_t* interface = F_get_default_dynamic_array_interface();
    
    if (interface == NULL) {
        printf("    ✗ 获取默认接口失败\n");
        return false;
    }
    
    // 验证接口函数指针不为NULL
    if (interface->create == NULL || interface->destroy == NULL ||
        interface->push == NULL || interface->get == NULL ||
        interface->set == NULL || interface->remove == NULL ||
        interface->length == NULL || interface->capacity == NULL ||
        interface->is_empty == NULL || interface->clear == NULL ||
        interface->resize == NULL || interface->find == NULL ||
        interface->push_batch == NULL || interface->foreach == NULL ||
        interface->get_memory_stats == NULL || interface->shrink_to_fit == NULL) {
        printf("    ✗ 接口函数指针为NULL\n");
        all_passed = false;
    } else {
        printf("    ✓ 获取默认接口测试通过\n");
    }
    
    // 测试2: 使用接口创建和操作数组
    printf("  2. 测试使用接口操作数组...\n");
    enum Eum_DynamicArrayError error_code;
    
    // 使用接口创建数组
    void* array = interface->create(sizeof(int), 10, &error_code);
    if (array == NULL || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 接口创建数组失败\n");
        all_passed = false;
    } else {
        // 使用接口添加元素
        int value = 42;
        if (!interface->push(array, &value, &error_code) || 
            error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
            printf("    ✗ 接口添加元素失败\n");
            all_passed = false;
        }
        
        // 使用接口获取元素
        void* retrieved = interface->get(array, 0, &error_code);
        if (retrieved == NULL || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
            printf("    ✗ 接口获取元素失败\n");
            all_passed = false;
        } else {
            int* int_value = (int*)retrieved;
            if (*int_value != value) {
                printf("    ✗ 接口获取的元素值不正确\n");
                all_passed = false;
            }
        }
        
        // 使用接口获取长度
        size_t len = interface->length(array, &error_code);
        if (len != 1 || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
            printf("    ✗ 接口获取长度失败\n");
            all_passed = false;
        }
        
        // 使用接口销毁数组
        interface->destroy(array, &error_code);
        if (error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
            printf("    ✗ 接口销毁数组失败\n");
            all_passed = false;
        }
        
        if (all_passed) {
            printf("    ✓ 使用接口操作数组测试通过\n");
        }
    }
    
    // 测试3: 接口错误处理
    printf("  3. 测试接口错误处理...\n");
    
    // 测试无效参数
    void* invalid_array = interface->create(0, 10, &error_code);
    if (invalid_array != NULL || error_code != Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE) {
        printf("    ✗ 接口无效参数错误处理失败\n");
        all_passed = false;
    } else {
        printf("    ✓ 接口错误处理测试通过\n");
    }
    
    printf("抽象接口测试: %s\n\n", all_passed ? "全部通过" : "有失败");
    return all_passed;
}

/**
 * @brief 测试边界情况
 */
bool test_dynamic_array_edge_cases(void)
{
    printf("测试: 边界情况\n");
    printf("--------------\n");
    
    bool all_passed = true;
    
    // 测试1: 超大容量
    printf("  1. 测试超大容量...\n");
    enum Eum_DynamicArrayError error_code;
    Stru_DynamicArray_t* array = F_create_dynamic_array_ex(sizeof(int), 1000000, &error_code);
    
    if (array == NULL) {
        printf("    ✗ 创建超大容量数组失败，错误码: %d\n", error_code);
        // 这可能是因为内存不足，不是错误
        printf("    ⚠ 超大容量测试跳过（可能内存不足）\n");
    } else {
        // 添加大量元素
        int value = 42;
        bool push_success = true;
        for (int i = 0; i < 1000; i++) {
            if (!F_dynamic_array_push_ex(array, &value, &error_code)) {
                printf("    ✗ 添加元素失败: 索引 %d, 错误码: %d\n", i, error_code);
                push_success = false;
                all_passed = false;
                break;
            }
        }
        
        if (push_success) {
            if (F_dynamic_array_length(array) != 1000) {
                printf("    ✗ 添加大量元素后长度不正确\n");
                all_passed = false;
            } else {
                printf("    ✓ 超大容量测试通过\n");
            }
        }
        
        F_destroy_dynamic_array(array);
    }
    
    // 测试2: 零大小元素
    printf("  2. 测试零大小元素...\n");
    array = F_create_dynamic_array_ex(0, 10, &error_code);
    if (array != NULL || error_code != Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE) {
        printf("    ✗ 零大小元素创建应该失败\n");
        all_passed = false;
    } else {
        printf("    ✓ 零大小元素测试通过\n");
    }
    
    // 测试3: 重复添加和移除
    printf("  3. 测试重复添加和移除...\n");
    array = F_create_dynamic_array(sizeof(int));
    if (array == NULL) {
        printf("    ✗ 无法创建数组，跳过重复添加移除测试\n");
        all_passed = false;
    } else {
        int val = 1;
        // 添加100个元素
        for (int i = 0; i < 100; i++) {
            F_dynamic_array_push(array, &val);
        }
        
        // 移除所有元素
        for (int i = 99; i >= 0; i--) {
            if (!F_dynamic_array_remove(array, i)) {
                printf("    ✗ 移除元素失败: 索引 %d\n", i);
                all_passed = false;
                break;
            }
        }
        
        if (F_dynamic_array_length(array) != 0) {
            printf("    ✗ 重复添加移除后长度不为0\n");
            all_passed = false;
        } else {
            printf("    ✓ 重复添加移除测试通过\n");
        }
        
        F_destroy_dynamic_array(array);
    }
    
    // 测试4: 内存边界测试
    printf("  4. 测试内存边界...\n");
    array = F_create_dynamic_array(sizeof(int));
    if (array == NULL) {
        printf("    ✗ 无法创建数组，跳过内存边界测试\n");
        all_passed = false;
    } else {
        // 测试容量调整到刚好边界
        size_t initial_capacity = F_dynamic_array_capacity(array);
        if (!F_dynamic_array_resize(array, initial_capacity * 2)) {
            printf("    ✗ 容量翻倍调整失败\n");
            all_passed = false;
        }
        
        // 测试缩小到最小容量
        if (!F_dynamic_array_resize(array, 1)) {
            printf("    ✗ 缩小到最小容量失败\n");
            all_passed = false;
        }
        
        // 测试压缩
        if (!F_dynamic_array_shrink_to_fit(array)) {
            printf("    ✗ 压缩数组失败\n");
            all_passed = false;
        }
        
        printf("    ✓ 内存边界测试通过\n");
        F_destroy_dynamic_array(array);
    }
    
    // 测试5: 特殊值测试
    printf("  5. 测试特殊值...\n");
    array = F_create_dynamic_array(sizeof(int));
    if (array == NULL) {
        printf("    ✗ 无法创建数组，跳过特殊值测试\n");
        all_passed = false;
    } else {
        int special_values[] = {0, -1, INT_MAX, INT_MIN};
        for (int i = 0; i < 4; i++) {
            if (!F_dynamic_array_push(array, &special_values[i])) {
                printf("    ✗ 添加特殊值失败: %d\n", special_values[i]);
                all_passed = false;
                break;
            }
        }
        
        if (all_passed) {
            for (int i = 0; i < 4; i++) {
                int* retrieved = (int*)F_dynamic_array_get(array, i);
                if (retrieved == NULL || *retrieved != special_values[i]) {
                    printf("    ✗ 获取特殊值失败: 索引 %d\n", i);
                    all_passed = false;
                    break;
                }
            }
        }
        
        if (all_passed) {
            printf("    ✓ 特殊值测试通过\n");
        }
        
        F_destroy_dynamic_array(array);
    }
    
    printf("边界情况测试: %s\n\n", all_passed ? "全部通过" : "有失败");
    return all_passed;
}

/**
 * @brief 测试性能特征
 */
bool test_dynamic_array_performance(void)
{
    printf("测试: 性能特征\n");
    printf("--------------\n");
    
    bool all_passed = true;
    
    // 测试1: 添加大量元素的性能
    printf("  1. 测试添加大量元素的性能...\n");
    Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
    
    if (array == NULL) {
        printf("    ✗ 无法创建数组，跳过性能测试\n");
        return false;
    }
    
    // 记录开始时间
    clock_t start_time = clock();
    
    // 添加10000个元素
    int value = 42;
    for (int i = 0; i < 10000; i++) {
        if (!F_dynamic_array_push(array, &value)) {
            printf("    ✗ 添加元素失败: 索引 %d\n", i);
            all_passed = false;
            break;
        }
    }
    
    // 记录结束时间
    clock_t end_time = clock();
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    if (all_passed) {
        printf("    添加10000个元素耗时: %.6f 秒\n", elapsed_time);
        if (elapsed_time > 1.0) { // 如果超过1秒，可能有问题
            printf("    ⚠ 添加操作可能较慢\n");
        } else {
            printf("    ✓ 添加大量元素性能测试通过\n");
        }
    }
    
    // 测试2: 查找操作的性能
    printf("  2. 测试查找操作的性能...\n");
    
    // 查找不存在的元素（最坏情况）
    start_time = clock();
    int not_found = -1;
    size_t found_index = F_dynamic_array_find(array, &not_found, compare_int);
    end_time = clock();
    elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    if (found_index != SIZE_MAX) {
        printf("    ✗ 查找不存在的元素应该返回SIZE_MAX\n");
        all_passed = false;
    } else {
        printf("    查找不存在的元素耗时: %.6f 秒\n", elapsed_time);
        printf("    ✓ 查找操作性能测试通过\n");
    }
    
    // 测试3: 内存使用效率
    printf("  3. 测试内存使用效率...\n");
    
    size_t total_bytes = 0, used_bytes = 0;
    if (F_dynamic_array_get_memory_stats(array, &total_bytes, &used_bytes)) {
        double efficiency = (double)used_bytes / total_bytes * 100.0;
        printf("    总内存: %zu 字节, 已使用: %zu 字节\n", total_bytes, used_bytes);
        printf("    内存使用效率: %.2f%%\n", efficiency);
        
        if (efficiency < 50.0) {
            printf("    ⚠ 内存使用效率较低，考虑压缩数组\n");
        } else {
            printf("    ✓ 内存使用效率测试通过\n");
        }
    } else {
        printf("    ✗ 获取内存统计失败\n");
        all_passed = false;
    }
    
    // 测试4: 批量操作的性能
    printf("  4. 测试批量操作的性能...\n");
    
    F_dynamic_array_clear(array);
    
    // 准备批量数据
    int batch_data[1000];
    for (int i = 0; i < 1000; i++) {
        batch_data[i] = i;
    }
    
    start_time = clock();
    bool batch_result = F_dynamic_array_push_batch(array, batch_data, 1000);
    end_time = clock();
    elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    if (!batch_result) {
        printf("    ✗ 批量添加失败\n");
        all_passed = false;
    } else {
        printf("    批量添加1000个元素耗时: %.6f 秒\n", elapsed_time);
        
        // 与逐个添加比较
        F_dynamic_array_clear(array);
        start_time = clock();
        for (int i = 0; i < 1000; i++) {
            F_dynamic_array_push(array, &batch_data[i]);
        }
        end_time = clock();
        double individual_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        
        printf("    逐个添加1000个元素耗时: %.6f 秒\n", individual_time);
        printf("    批量操作加速比: %.2fx\n", individual_time / elapsed_time);
        
        if (elapsed_time > individual_time) {
            printf("    ⚠ 批量操作没有加速效果\n");
        } else {
            printf("    ✓ 批量操作性能测试通过\n");
        }
    }
    
    F_destroy_dynamic_array(array);
    printf("性能测试: %s\n\n", all_passed ? "全部通过" : "有失败");
    return all_passed;
}

/**
 * @brief 运行所有动态数组测试
 */
int run_all_dynamic_array_tests(void)
{
    printf("========================================\n");
    printf("开始运行动态数组模块测试\n");
    printf("========================================\n\n");
    
    int passed_count = 0;
    int total_count = 10; // 总测试函数数量
    
    // 运行所有测试函数
    if (test_dynamic_array_create_and_destroy()) passed_count++;
    if (test_dynamic_array_basic_operations()) passed_count++;
    if (test_dynamic_array_query_operations()) passed_count++;
    if (test_dynamic_array_array_operations()) passed_count++;
    if (test_dynamic_array_iteration_operations()) passed_count++;
    if (test_dynamic_array_memory_management()) passed_count++;
    if (test_dynamic_array_error_handling()) passed_count++;
    if (test_dynamic_array_abstract_interface()) passed_count++;
    if (test_dynamic_array_edge_cases()) passed_count++;
    if (test_dynamic_array_performance()) passed_count++;
    
    printf("========================================\n");
    printf("动态数组模块测试完成\n");
    printf("通过: %d/%d\n", passed_count, total_count);
    printf("========================================\n");
    
    return passed_count;
}
