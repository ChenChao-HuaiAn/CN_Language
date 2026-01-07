/**
 * @file test_dynamic_array_edge_cases.c
 * @brief 动态数组边界情况测试
 * 
 * 测试动态数组的边界情况和极端情况。
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
#include <limits.h>

/**
 * @brief 测试边界情况功能
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_edge_cases(void)
{
    printf("测试: 边界情况\n");
    printf("--------------\n");
    
    TestResults results;
    init_test_results(&results);
    
    // 测试1: 超大容量
    printf("  1. 测试超大容量...\n");
    bool test_passed = true;
    
    enum Eum_DynamicArrayError error_code;
    Stru_DynamicArray_t* large_array = F_create_dynamic_array_ex(sizeof(int), 1000000, &error_code);
    
    if (large_array == NULL) {
        printf("    ✗ 创建超大容量数组失败: 错误码 %d\n", error_code);
        test_passed = false;
    } else {
        // 添加大量元素
        int value = 42;
        for (int i = 0; i < 10000; i++) {
            if (!F_dynamic_array_push(large_array, &value)) {
                printf("    ✗ 添加大量元素失败: 索引 %d\n", i);
                test_passed = false;
                break;
            }
        }
        
        if (test_passed && F_dynamic_array_length(large_array) != 10000) {
            printf("    ✗ 大量元素长度不正确\n");
            test_passed = false;
        }
        
        F_destroy_dynamic_array(large_array);
    }
    
    if (test_passed) {
        printf("    ✓ 超大容量测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试2: 极小容量
    printf("  2. 测试极小容量...\n");
    test_passed = true;
    
    Stru_DynamicArray_t* tiny_array = F_create_dynamic_array_ex(sizeof(int), 1, &error_code);
    
    if (tiny_array == NULL) {
        printf("    ✗ 创建极小容量数组失败: 错误码 %d\n", error_code);
        test_passed = false;
    } else {
        // 测试自动扩容
        int values[] = {1, 2, 3, 4, 5};
        for (int i = 0; i < 5; i++) {
            if (!F_dynamic_array_push(tiny_array, &values[i])) {
                printf("    ✗ 极小容量自动扩容失败: 索引 %d\n", i);
                test_passed = false;
                break;
            }
        }
        
        if (test_passed && F_dynamic_array_length(tiny_array) != 5) {
            printf("    ✗ 极小容量长度不正确\n");
            test_passed = false;
        }
        
        F_destroy_dynamic_array(tiny_array);
    }
    
    if (test_passed) {
        printf("    ✓ 极小容量测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试3: 零大小元素
    printf("  3. 测试零大小元素...\n");
    test_passed = true;
    
    Stru_DynamicArray_t* zero_size_array = F_create_dynamic_array_ex(0, 10, &error_code);
    
    if (zero_size_array != NULL) {
        printf("    ✗ 零大小元素应该失败\n");
        F_destroy_dynamic_array(zero_size_array);
        test_passed = false;
    } else if (error_code != Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE) {
        printf("    ✗ 错误码不正确: 期望 %d, 实际 %d\n", 
               Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE, error_code);
        test_passed = false;
    }
    
    if (test_passed) {
        printf("    ✓ 零大小元素测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试4: 超大元素大小
    printf("  4. 测试超大元素大小...\n");
    test_passed = true;
    
    // 测试一个合理的超大元素大小
    // 使用SIZE_MAX / 1024，这仍然是一个非常大的值
    // 注意：在某些实现中，这可能不会立即失败，因为内存分配可能是惰性的
    size_t huge_size = SIZE_MAX / 1024;
    printf("    - 尝试创建元素大小为%zu的数组...\n", huge_size);
    Stru_DynamicArray_t* huge_element_array = F_create_dynamic_array_ex(huge_size, 1, &error_code);
    
    if (huge_element_array != NULL) {
        printf("    - 警告：超大元素大小(%zu)创建成功（可能是惰性分配）\n", huge_size);
        printf("    - 返回的数组指针: %p, 错误码: %d\n", (void*)huge_element_array, error_code);
        
        // 尝试添加一个元素（这应该失败，因为无法分配实际内存）
        char test_element = 'A';
        bool push_result = F_dynamic_array_push(huge_element_array, &test_element);
        if (push_result) {
            printf("    ✗ 超大元素大小数组不应该能够添加元素\n");
            test_passed = false;
        } else {
            printf("    - 添加元素失败，符合预期\n");
            printf("    ✓ 超大元素大小测试通过（创建成功但操作失败）\n");
        }
        
        F_destroy_dynamic_array(huge_element_array);
    } else {
        // 创建失败，符合预期
        printf("    - 创建失败，符合预期\n");
        printf("    - 实际错误码: %d\n", error_code);
        printf("    ✓ 超大元素大小测试通过\n");
    }
    
    update_test_results(&results, test_passed);
    
    // 测试5: 边界索引操作
    printf("  5. 测试边界索引操作...\n");
    test_passed = true;
    
    Stru_DynamicArray_t* array = F_create_dynamic_array(sizeof(int));
    if (array == NULL) {
        printf("    ✗ 无法创建数组，跳过边界索引测试\n");
        update_test_results(&results, false);
    } else {
        // 添加一些元素
        int values[] = {10, 20, 30, 40, 50};
        for (int i = 0; i < 5; i++) {
            F_dynamic_array_push(array, &values[i]);
        }
        
        // 测试第一个元素
        int* first = (int*)F_dynamic_array_get(array, 0);
        if (first == NULL || *first != 10) {
            printf("    ✗ 第一个元素不正确\n");
            test_passed = false;
        }
        
        // 测试最后一个元素
        int* last = (int*)F_dynamic_array_get(array, 4);
        if (last == NULL || *last != 50) {
            printf("    ✗ 最后一个元素不正确\n");
            test_passed = false;
        }
        
        // 测试刚好超出边界的索引
        int* out_of_bounds = (int*)F_dynamic_array_get(array, 5);
        if (out_of_bounds != NULL) {
            printf("    ✗ 超出边界索引应该返回NULL\n");
            test_passed = false;
        }
        
        // 测试SIZE_MAX索引
        int* max_index = (int*)F_dynamic_array_get(array, SIZE_MAX);
        if (max_index != NULL) {
            printf("    ✗ SIZE_MAX索引应该返回NULL\n");
            test_passed = false;
        }
        
        // 测试边界设置
        int new_first = 99;
        if (!F_dynamic_array_set(array, 0, &new_first)) {
            printf("    ✗ 设置第一个元素失败\n");
            test_passed = false;
        } else {
            int* updated_first = (int*)F_dynamic_array_get(array, 0);
            if (updated_first == NULL || *updated_first != new_first) {
                printf("    ✗ 第一个元素未正确更新\n");
                test_passed = false;
            }
        }
        
        // 测试边界移除
        if (!F_dynamic_array_remove(array, 4)) {
            printf("    ✗ 移除最后一个元素失败\n");
            test_passed = false;
        } else if (F_dynamic_array_length(array) != 4) {
            printf("    ✗ 移除后长度不正确\n");
            test_passed = false;
        }
        
        F_destroy_dynamic_array(array);
        
        if (test_passed) {
            printf("    ✓ 边界索引操作测试通过\n");
        }
        update_test_results(&results, test_passed);
    }
    
    // 测试6: 重复创建和销毁
    printf("  6. 测试重复创建和销毁...\n");
    test_passed = true;
    
    for (int i = 0; i < 100; i++) {
        Stru_DynamicArray_t* temp_array = F_create_dynamic_array(sizeof(int));
        if (temp_array == NULL) {
            printf("    ✗ 重复创建失败: 迭代 %d\n", i);
            test_passed = false;
            break;
        }
        
        int value = i;
        if (!F_dynamic_array_push(temp_array, &value)) {
            printf("    ✗ 重复添加元素失败: 迭代 %d\n", i);
            F_destroy_dynamic_array(temp_array);
            test_passed = false;
            break;
        }
        
        F_destroy_dynamic_array(temp_array);
    }
    
    if (test_passed) {
        printf("    ✓ 重复创建和销毁测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    print_test_results(&results, "边界情况");
    return results.failed_tests == 0;
}
