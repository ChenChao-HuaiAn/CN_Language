/**
 * @file test_dynamic_array_array_operations.c
 * @brief 动态数组数组操作测试
 * 
 * 测试动态数组的数组操作功能（清空、调整容量、查找、批量添加等）。
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
#include <stdint.h>

/**
 * @brief 测试数组操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_array_operations(void)
{
    printf("测试: 数组操作\n");
    printf("--------------\n");
    
    TestResults results;
    init_test_results(&results);
    
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
    bool test_passed = true;
    F_dynamic_array_clear(array);
    
    if (F_dynamic_array_length(array) != 0) {
        printf("    ✗ 清空后长度不为0\n");
        test_passed = false;
    }
    
    if (!F_dynamic_array_is_empty(array)) {
        printf("    ✗ 清空后is_empty返回false\n");
        test_passed = false;
    }
    
    // 测试带错误码的清空
    enum Eum_DynamicArrayError error_code;
    F_dynamic_array_clear_ex(array, &error_code);
    if (error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的清空失败\n");
        test_passed = false;
    }
    
    if (test_passed) {
        printf("    ✓ 清空数组测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 重新添加数据
    for (int i = 0; i < 5; i++) {
        F_dynamic_array_push(array, &values[i]);
    }
    
    // 测试2: 调整容量
    printf("  2. 测试调整容量...\n");
    test_passed = true;
    
    // 扩大容量
    if (!F_dynamic_array_resize(array, 100)) {
        printf("    ✗ 扩大容量失败\n");
        test_passed = false;
    } else if (F_dynamic_array_capacity(array) < 100) {
        printf("    ✗ 扩大容量后容量不正确\n");
        test_passed = false;
    }
    
    // 缩小容量（但大于当前长度）
    if (!F_dynamic_array_resize(array, 10)) {
        printf("    ✗ 缩小容量失败\n");
        test_passed = false;
    } else if (F_dynamic_array_capacity(array) != 10) {
        printf("    ✗ 缩小容量后容量不正确\n");
        test_passed = false;
    }
    
    // 测试带错误码的调整
    if (!F_dynamic_array_resize_ex(array, 20, &error_code)) {
        printf("    ✗ 带错误码的调整容量失败\n");
        test_passed = false;
    } else if (error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的调整容量错误码不正确\n");
        test_passed = false;
    }
    
    if (test_passed) {
        printf("    ✓ 调整容量测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试3: 查找元素
    printf("  3. 测试查找元素...\n");
    test_passed = true;
    
    int target = 30;
    size_t found_index = F_dynamic_array_find(array, &target, compare_int);
    if (found_index == SIZE_MAX) {
        printf("    ✗ 查找元素失败: 未找到30\n");
        test_passed = false;
    } else {
        int* found = (int*)F_dynamic_array_get(array, found_index);
        if (found == NULL || *found != target) {
            printf("    ✗ 查找元素失败: 找到的值不正确\n");
            test_passed = false;
        }
    }
    
    // 查找不存在的元素
    int not_found = 99;
    size_t not_found_index = F_dynamic_array_find(array, &not_found, compare_int);
    if (not_found_index != SIZE_MAX) {
        printf("    ✗ 查找元素失败: 不应该找到99\n");
        test_passed = false;
    }
    
    // 测试带错误码的查找
    size_t found_index_ex = F_dynamic_array_find_ex(array, &target, compare_int, &error_code);
    if (found_index_ex == SIZE_MAX || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的查找失败\n");
        test_passed = false;
    }
    
    // 测试查找不存在的元素（带错误码）
    size_t not_found_index_ex = F_dynamic_array_find_ex(array, &not_found, compare_int, &error_code);
    if (not_found_index_ex != SIZE_MAX || error_code != Eum_DYNAMIC_ARRAY_ERROR_ITEM_NOT_FOUND) {
        printf("    ✗ 带错误码的查找不存在的元素失败\n");
        test_passed = false;
    }
    
    if (test_passed) {
        printf("    ✓ 查找元素测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试4: 批量添加元素
    printf("  4. 测试批量添加元素...\n");
    test_passed = true;
    F_dynamic_array_clear(array);
    
    int batch_values[] = {100, 200, 300, 400, 500};
    if (!F_dynamic_array_push_batch(array, batch_values, 5)) {
        printf("    ✗ 批量添加元素失败\n");
        test_passed = false;
    } else {
        if (F_dynamic_array_length(array) != 5) {
            printf("    ✗ 批量添加后长度不正确\n");
            test_passed = false;
        }
        
        for (int i = 0; i < 5; i++) {
            int* val = (int*)F_dynamic_array_get(array, i);
            if (val == NULL || *val != batch_values[i]) {
                printf("    ✗ 批量添加元素值不正确: 索引 %d\n", i);
                test_passed = false;
                break;
            }
        }
    }
    
    // 测试带错误码的批量添加
    F_dynamic_array_clear(array);
    if (!F_dynamic_array_push_batch_ex(array, batch_values, 5, &error_code)) {
        printf("    ✗ 带错误码的批量添加失败\n");
        test_passed = false;
    } else if (error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的批量添加错误码不正确\n");
        test_passed = false;
    }
    
    // 测试无效批量添加
    if (F_dynamic_array_push_batch_ex(array, NULL, 5, &error_code)) {
        printf("    ✗ 无效批量添加应该失败\n");
        test_passed = false;
    }
    
    if (test_passed) {
        printf("    ✓ 批量添加元素测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    F_destroy_dynamic_array(array);
    print_test_results(&results, "数组操作");
    return results.failed_tests == 0;
}
