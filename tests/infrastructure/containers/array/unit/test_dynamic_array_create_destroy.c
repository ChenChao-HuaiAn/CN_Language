/**
 * @file test_dynamic_array_create_destroy.c
 * @brief 动态数组创建和销毁测试
 * 
 * 测试动态数组的创建和销毁功能。
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
 * @brief 测试创建和销毁功能
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_create_and_destroy(void)
{
    printf("测试: 创建和销毁\n");
    printf("----------------\n");
    
    TestResults results;
    init_test_results(&results);
    
    // 测试1: 基本创建
    printf("  1. 测试基本创建...\n");
    Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
    bool test_passed = false;
    
    if (array == NULL) {
        printf("    ✗ 基本创建失败: 返回NULL\n");
    } else {
        if (F_dynamic_array_length(array) != 0) {
            printf("    ✗ 基本创建失败: 初始长度不为0\n");
        } else if (F_dynamic_array_capacity(array) < 4) { // 最小容量
            printf("    ✗ 基本创建失败: 容量太小\n");
        } else if (F_dynamic_array_is_empty(array) != true) {
            printf("    ✗ 基本创建失败: 数组不为空\n");
        } else {
            printf("    ✓ 基本创建测试通过\n");
            test_passed = true;
        }
        F_destroy_dynamic_array(array);
    }
    update_test_results(&results, test_passed);
    
    // 测试2: 带初始容量的创建
    printf("  2. 测试带初始容量的创建...\n");
    enum Eum_DynamicArrayError error_code;
    array = F_create_dynamic_array_ex(sizeof(int), 100, &error_code);
    test_passed = false;
    
    if (array == NULL) {
        printf("    ✗ 带容量创建失败: 返回NULL, 错误码: %d\n", error_code);
    } else {
        if (F_dynamic_array_capacity(array) < 100) {
            printf("    ✗ 带容量创建失败: 容量小于指定值\n");
        } else if (error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
            printf("    ✗ 带容量创建失败: 错误码不正确\n");
        } else {
            printf("    ✓ 带容量创建测试通过\n");
            test_passed = true;
        }
        F_destroy_dynamic_array(array);
    }
    update_test_results(&results, test_passed);
    
    // 测试3: 无效参数创建
    printf("  3. 测试无效参数创建...\n");
    array = F_create_dynamic_array_ex(0, 10, &error_code);
    test_passed = false;
    
    if (array != NULL) {
        printf("    ✗ 无效参数创建失败: 应该返回NULL\n");
        F_destroy_dynamic_array(array);
    } else {
        if (error_code != Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE) {
            printf("    ✗ 无效参数创建失败: 错误码不正确\n");
        } else {
            printf("    ✓ 无效参数创建测试通过\n");
            test_passed = true;
        }
    }
    update_test_results(&results, test_passed);
    
    // 测试4: 销毁NULL指针
    printf("  4. 测试销毁NULL指针...\n");
    F_destroy_dynamic_array(NULL); // 不应该崩溃
    printf("    ✓ 销毁NULL指针测试通过\n");
    update_test_results(&results, true);
    
    print_test_results(&results, "创建和销毁");
    return results.failed_tests == 0;
}
