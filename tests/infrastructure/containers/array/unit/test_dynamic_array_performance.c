/**
 * @file test_dynamic_array_performance.c
 * @brief 动态数组性能测试
 * 
 * 测试动态数组的性能特性。
 * 遵循模块化测试设计，每个测试功能在单独的文件中。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_dynamic_array_common.h"
#include "../../../../../src/infrastructure/containers/array/CN_dynamic_array.h"
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

/**
 * @brief 性能测试：批量添加元素
 * @param array 动态数组指针
 * @param count 要添加的元素数量
 * @return 添加操作耗时（秒）
 */
static double test_batch_add_performance(Stru_DynamicArray_t* array, int count)
{
    clock_t start = clock();
    
    int value = 42;
    for (int i = 0; i < count; i++) {
        F_dynamic_array_push(array, &value);
    }
    
    clock_t end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}

/**
 * @brief 性能测试：随机访问元素
 * @param array 动态数组指针
 * @param count 访问次数
 * @return 访问操作耗时（秒）
 */
static double test_random_access_performance(Stru_DynamicArray_t* array, int count)
{
    size_t length = F_dynamic_array_length(array);
    if (length == 0) return 0.0;
    
    clock_t start = clock();
    
    for (int i = 0; i < count; i++) {
        size_t index = rand() % length;
        F_dynamic_array_get(array, index);
    }
    
    clock_t end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}

/**
 * @brief 性能测试：遍历所有元素
 * @param array 动态数组指针
 * @return 遍历操作耗时（秒）
 */
static double test_iteration_performance(Stru_DynamicArray_t* array)
{
    size_t length = F_dynamic_array_length(array);
    
    clock_t start = clock();
    
    for (size_t i = 0; i < length; i++) {
        F_dynamic_array_get(array, i);
    }
    
    clock_t end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}

/**
 * @brief 测试性能功能
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_performance(void)
{
    printf("测试: 性能测试\n");
    printf("--------------\n");
    
    TestResults results;
    init_test_results(&results);
    
    // 初始化随机数种子
    srand((unsigned int)time(NULL));
    
    // 测试1: 小规模性能测试
    printf("  1. 测试小规模性能...\n");
    bool test_passed = true;
    
    Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
    if (array == NULL) {
        printf("    ✗ 无法创建数组，跳过性能测试\n");
        update_test_results(&results, false);
        return results.failed_tests == 0;
    }
    
    // 测试批量添加1000个元素
    double batch_time = test_batch_add_performance(array, 1000);
    printf("    - 批量添加1000个元素: %.6f 秒\n", batch_time);
    
    if (batch_time > 0.1) { // 如果超过0.1秒，可能有问题
        printf("    ✗ 批量添加性能太慢\n");
        test_passed = false;
    }
    
    // 测试随机访问10000次
    double access_time = test_random_access_performance(array, 10000);
    printf("    - 随机访问10000次: %.6f 秒\n", access_time);
    
    if (access_time > 0.05) { // 如果超过0.05秒，可能有问题
        printf("    ✗ 随机访问性能太慢\n");
        test_passed = false;
    }
    
    // 测试遍历所有元素
    double iteration_time = test_iteration_performance(array);
    printf("    - 遍历1000个元素: %.6f 秒\n", iteration_time);
    
    if (iteration_time > 0.01) { // 如果超过0.01秒，可能有问题
        printf("    ✗ 遍历性能太慢\n");
        test_passed = false;
    }
    
    F_destroy_dynamic_array(array);
    
    if (test_passed) {
        printf("    ✓ 小规模性能测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试2: 中等规模性能测试
    printf("  2. 测试中等规模性能...\n");
    test_passed = true;
    
    array = F_create_dynamic_array(sizeof(int));
    
    // 测试批量添加10000个元素
    batch_time = test_batch_add_performance(array, 10000);
    printf("    - 批量添加10000个元素: %.6f 秒\n", batch_time);
    
    // 期望时间大约是1000个元素的10倍左右
    if (batch_time > 1.0) { // 如果超过1秒，可能有问题
        printf("    ✗ 中等规模批量添加性能太慢\n");
        test_passed = false;
    }
    
    // 测试随机访问100000次
    access_time = test_random_access_performance(array, 100000);
    printf("    - 随机访问100000次: %.6f 秒\n", access_time);
    
    if (access_time > 0.5) { // 如果超过0.5秒，可能有问题
        printf("    ✗ 中等规模随机访问性能太慢\n");
        test_passed = false;
    }
    
    F_destroy_dynamic_array(array);
    
    if (test_passed) {
        printf("    ✓ 中等规模性能测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试3: 内存使用性能
    printf("  3. 测试内存使用性能...\n");
    test_passed = true;
    
    // 测试创建和销毁大量小数组
    clock_t memory_start = clock();
    
    for (int i = 0; i < 1000; i++) {
        Stru_DynamicArray_t* temp_array = F_create_dynamic_array(sizeof(int));
        if (temp_array == NULL) {
            printf("    ✗ 创建数组失败: 迭代 %d\n", i);
            test_passed = false;
            break;
        }
        
        // 添加一些元素
        int value = i;
        for (int j = 0; j < 10; j++) {
            F_dynamic_array_push(temp_array, &value);
        }
        
        F_destroy_dynamic_array(temp_array);
    }
    
    clock_t memory_end = clock();
    double memory_time = (double)(memory_end - memory_start) / CLOCKS_PER_SEC;
    printf("    - 创建/销毁1000个小数组: %.6f 秒\n", memory_time);
    
    if (memory_time > 2.0) { // 如果超过2秒，可能有问题
        printf("    ✗ 内存管理性能太慢\n");
        test_passed = false;
    }
    
    if (test_passed) {
        printf("    ✓ 内存使用性能测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试4: 自动扩容性能
    printf("  4. 测试自动扩容性能...\n");
    test_passed = true;
    
    array = F_create_dynamic_array_ex(sizeof(int), 10, NULL); // 小初始容量
    
    clock_t resize_start = clock();
    
    // 添加大量元素，触发多次自动扩容
    for (int i = 0; i < 100000; i++) {
        F_dynamic_array_push(array, &i);
    }
    
    clock_t resize_end = clock();
    double resize_time = (double)(resize_end - resize_start) / CLOCKS_PER_SEC;
    printf("    - 自动扩容添加100000个元素: %.6f 秒\n", resize_time);
    
    if (resize_time > 0.5) { // 如果超过0.5秒，可能有问题
        printf("    ✗ 自动扩容性能太慢\n");
        test_passed = false;
    }
    
    // 检查最终容量
    size_t final_capacity = F_dynamic_array_capacity(array);
    size_t final_length = F_dynamic_array_length(array);
    printf("    - 最终长度: %zu, 最终容量: %zu\n", final_length, final_capacity);
    
    if (final_capacity < final_length) {
        printf("    ✗ 容量小于长度\n");
        test_passed = false;
    }
    
    F_destroy_dynamic_array(array);
    
    if (test_passed) {
        printf("    ✓ 自动扩容性能测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    print_test_results(&results, "性能测试");
    return results.failed_tests == 0;
}
