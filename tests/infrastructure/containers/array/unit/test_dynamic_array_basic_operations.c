/**
 * @file test_dynamic_array_basic_operations.c
 * @brief 动态数组基本操作测试
 * 
 * 测试动态数组的基本操作功能（添加、获取、设置、移除）。
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

/**
 * @brief 测试基本操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_basic_operations(void)
{
    printf("测试: 基本操作\n");
    printf("--------------\n");
    
    TestResults results;
    init_test_results(&results);
    
    Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
    
    if (array == NULL) {
        printf("  ✗ 无法创建数组，跳过基本操作测试\n");
        return false;
    }
    
    // 测试1: 添加元素
    printf("  1. 测试添加元素...\n");
    int values[] = {10, 20, 30, 40, 50};
    bool test_passed = true;
    
    for (int i = 0; i < 5; i++) {
        if (!F_dynamic_array_push(array, &values[i])) {
            printf("    ✗ 添加元素失败: 索引 %d\n", i);
            test_passed = false;
            break;
        }
    }
    
    if (test_passed && F_dynamic_array_length(array) != 5) {
        printf("    ✗ 添加元素失败: 长度不正确\n");
        test_passed = false;
    }
    
    if (test_passed) {
        printf("    ✓ 添加元素测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试2: 获取元素
    printf("  2. 测试获取元素...\n");
    test_passed = true;
    
    for (int i = 0; i < 5; i++) {
        int* retrieved = (int*)F_dynamic_array_get(array, i);
        if (retrieved == NULL) {
            printf("    ✗ 获取元素失败: 索引 %d 返回NULL\n", i);
            test_passed = false;
        } else if (*retrieved != values[i]) {
            printf("    ✗ 获取元素失败: 索引 %d 值不正确\n", i);
            test_passed = false;
        }
    }
    
    // 测试无效索引
    int* invalid = (int*)F_dynamic_array_get(array, 100);
    if (invalid != NULL) {
        printf("    ✗ 获取元素失败: 无效索引应该返回NULL\n");
        test_passed = false;
    }
    
    if (test_passed) {
        printf("    ✓ 获取元素测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试3: 设置元素
    printf("  3. 测试设置元素...\n");
    test_passed = true;
    int new_value = 99;
    
    if (!F_dynamic_array_set(array, 2, &new_value)) {
        printf("    ✗ 设置元素失败: 索引 2\n");
        test_passed = false;
    } else {
        int* retrieved = (int*)F_dynamic_array_get(array, 2);
        if (retrieved == NULL || *retrieved != new_value) {
            printf("    ✗ 设置元素失败: 值未正确更新\n");
            test_passed = false;
        }
    }
    
    // 测试无效索引设置
    if (F_dynamic_array_set(array, 100, &new_value)) {
        printf("    ✗ 设置元素失败: 无效索引应该返回false\n");
        test_passed = false;
    }
    
    if (test_passed) {
        printf("    ✓ 设置元素测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试4: 移除元素
    printf("  4. 测试移除元素...\n");
    test_passed = true;
    size_t original_length = F_dynamic_array_length(array);
    
    if (!F_dynamic_array_remove(array, 1)) {
        printf("    ✗ 移除元素失败: 索引 1\n");
        test_passed = false;
    } else {
        if (F_dynamic_array_length(array) != original_length - 1) {
            printf("    ✗ 移除元素失败: 长度未减少\n");
            test_passed = false;
        }
        
        // 验证元素已移动
        int* first = (int*)F_dynamic_array_get(array, 0);
        int* second = (int*)F_dynamic_array_get(array, 1); // 原来是索引2
        if (first == NULL || *first != 10) {
            printf("    ✗ 移除元素失败: 第一个元素不正确\n");
            test_passed = false;
        }
        if (second == NULL || *second != new_value) { // 原来是30，现在是99
            printf("    ✗ 移除元素失败: 第二个元素不正确\n");
            test_passed = false;
        }
    }
    
    // 测试无效索引移除
    if (F_dynamic_array_remove(array, 100)) {
        printf("    ✗ 移除元素失败: 无效索引应该返回false\n");
        test_passed = false;
    }
    
    if (test_passed) {
        printf("    ✓ 移除元素测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    F_destroy_dynamic_array(array);
    print_test_results(&results, "基本操作");
    return results.failed_tests == 0;
}
