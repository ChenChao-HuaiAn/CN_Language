/**
 * @file test_dynamic_array_iteration_operations.c
 * @brief 动态数组迭代操作测试
 * 
 * 测试动态数组的迭代操作功能（遍历、回调等）。
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
 * @brief 测试迭代操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_iteration_operations(void)
{
    printf("测试: 迭代操作\n");
    printf("--------------\n");
    
    TestResults results;
    init_test_results(&results);
    
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
    bool test_passed = true;
    int count = 0;
    F_dynamic_array_foreach(array, print_int_callback, &count);
    
    if (count != 5) {
        printf("    ✗ 基本遍历失败: 回调调用次数不正确\n");
        test_passed = false;
    } else {
        printf("    ✓ 基本遍历测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试2: 带索引的遍历
    printf("  2. 测试带索引的遍历...\n");
    test_passed = true;
    count = 0;
    enum Eum_DynamicArrayError error_code;
    bool foreach_result = F_dynamic_array_foreach_ex(array, print_int_iterator, &count, &error_code);
    
    if (!foreach_result || count != 5 || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带索引的遍历失败\n");
        test_passed = false;
    } else {
        printf("    ✓ 带索引的遍历测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试3: 提前停止的迭代器
    printf("  3. 测试提前停止的迭代器...\n");
    test_passed = true;
    
    foreach_result = F_dynamic_array_foreach_ex(array, stop_at_third, NULL, &error_code);
    if (foreach_result) { // 应该返回false，因为迭代器提前停止了
        printf("    ✗ 提前停止的迭代器失败: 应该返回false\n");
        test_passed = false;
    } else {
        printf("    ✓ 提前停止的迭代器测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试4: NULL数组遍历
    printf("  4. 测试NULL数组遍历...\n");
    test_passed = true;
    F_dynamic_array_foreach(NULL, print_int_callback, NULL); // 不应该崩溃
    foreach_result = F_dynamic_array_foreach_ex(NULL, print_int_iterator, NULL, &error_code);
    if (foreach_result || error_code != Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER) {
        printf("    ✗ NULL数组遍历失败\n");
        test_passed = false;
    } else {
        printf("    ✓ NULL数组遍历测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    F_destroy_dynamic_array(array);
    print_test_results(&results, "迭代操作");
    return results.failed_tests == 0;
}
