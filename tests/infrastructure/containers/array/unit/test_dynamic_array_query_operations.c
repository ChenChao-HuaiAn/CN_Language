/**
 * @file test_dynamic_array_query_operations.c
 * @brief 动态数组查询操作测试
 * 
 * 测试动态数组的查询操作功能（长度、容量、是否为空等）。
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
 * @brief 测试查询操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_query_operations(void)
{
    printf("测试: 查询操作\n");
    printf("--------------\n");
    
    TestResults results;
    init_test_results(&results);
    
    Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
    
    if (array == NULL) {
        printf("  ✗ 无法创建数组，跳过查询操作测试\n");
        return false;
    }
    
    // 测试1: 空数组查询
    printf("  1. 测试空数组查询...\n");
    bool test_passed = true;
    
    if (F_dynamic_array_length(array) != 0) {
        printf("    ✗ 空数组长度不为0\n");
        test_passed = false;
    }
    
    if (!F_dynamic_array_is_empty(array)) {
        printf("    ✗ 空数组is_empty返回false\n");
        test_passed = false;
    }
    
    size_t capacity = F_dynamic_array_capacity(array);
    if (capacity < 4) { // 最小容量
        printf("    ✗ 空数组容量太小\n");
        test_passed = false;
    }
    
    if (test_passed) {
        printf("    ✓ 空数组查询测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试2: 添加元素后查询
    printf("  2. 测试添加元素后查询...\n");
    test_passed = true;
    int value = 42;
    F_dynamic_array_push(array, &value);
    
    if (F_dynamic_array_length(array) != 1) {
        printf("    ✗ 添加后长度不为1\n");
        test_passed = false;
    }
    
    if (F_dynamic_array_is_empty(array)) {
        printf("    ✗ 添加后is_empty返回true\n");
        test_passed = false;
    }
    
    // 测试带错误码的查询
    enum Eum_DynamicArrayError error_code;
    size_t len_with_error = F_dynamic_array_length_ex(array, &error_code);
    if (len_with_error != 1 || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的长度查询失败\n");
        test_passed = false;
    }
    
    bool empty_with_error = F_dynamic_array_is_empty_ex(array, &error_code);
    if (empty_with_error || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的空检查失败\n");
        test_passed = false;
    }
    
    size_t cap_with_error = F_dynamic_array_capacity_ex(array, &error_code);
    if (cap_with_error != capacity || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
        printf("    ✗ 带错误码的容量查询失败\n");
        test_passed = false;
    }
    
    if (test_passed) {
        printf("    ✓ 添加元素后查询测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试3: NULL数组查询
    printf("  3. 测试NULL数组查询...\n");
    test_passed = true;
    
    size_t null_len = F_dynamic_array_length(NULL);
    if (null_len != 0) {
        printf("    ✗ NULL数组长度不为0\n");
        test_passed = false;
    }
    
    size_t null_cap = F_dynamic_array_capacity(NULL);
    if (null_cap != 0) {
        printf("    ✗ NULL数组容量不为0\n");
        test_passed = false;
    }
    
    bool null_empty = F_dynamic_array_is_empty(NULL);
    if (!null_empty) {
        printf("    ✗ NULL数组is_empty返回false\n");
        test_passed = false;
    }
    
    // 测试带错误码的NULL查询
    size_t null_len_ex = F_dynamic_array_length_ex(NULL, &error_code);
    if (null_len_ex != 0 || error_code != Eum_DYNAMIC_ARRAY_ERROR_NULL_POINTER) {
        printf("    ✗ 带错误码的NULL长度查询失败\n");
        test_passed = false;
    }
    
    if (test_passed) {
        printf("    ✓ NULL数组查询测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    F_destroy_dynamic_array(array);
    print_test_results(&results, "查询操作");
    return results.failed_tests == 0;
}
