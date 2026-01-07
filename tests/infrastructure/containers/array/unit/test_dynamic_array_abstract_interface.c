/**
 * @file test_dynamic_array_abstract_interface.c
 * @brief 动态数组抽象接口测试
 * 
 * 测试动态数组的抽象接口功能。
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
 * @brief 测试抽象接口功能
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_abstract_interface(void)
{
    printf("测试: 抽象接口\n");
    printf("--------------\n");
    
    TestResults results;
    init_test_results(&results);
    
    // 测试1: 获取默认接口
    printf("  1. 测试获取默认接口...\n");
    bool test_passed = true;
    
    const Stru_DynamicArrayInterface_t* interface = F_get_default_dynamic_array_interface();
    if (interface == NULL) {
        printf("    ✗ 获取默认接口失败: 返回NULL\n");
        test_passed = false;
    } else {
        // 检查接口函数指针是否有效
        if (interface->create == NULL) {
            printf("    ✗ 接口create函数为NULL\n");
            test_passed = false;
        }
        if (interface->destroy == NULL) {
            printf("    ✗ 接口destroy函数为NULL\n");
            test_passed = false;
        }
        if (interface->push == NULL) {
            printf("    ✗ 接口push函数为NULL\n");
            test_passed = false;
        }
        if (interface->get == NULL) {
            printf("    ✗ 接口get函数为NULL\n");
            test_passed = false;
        }
        if (interface->set == NULL) {
            printf("    ✗ 接口set函数为NULL\n");
            test_passed = false;
        }
        if (interface->remove == NULL) {
            printf("    ✗ 接口remove函数为NULL\n");
            test_passed = false;
        }
        if (interface->length == NULL) {
            printf("    ✗ 接口length函数为NULL\n");
            test_passed = false;
        }
        if (interface->capacity == NULL) {
            printf("    ✗ 接口capacity函数为NULL\n");
            test_passed = false;
        }
        if (interface->is_empty == NULL) {
            printf("    ✗ 接口is_empty函数为NULL\n");
            test_passed = false;
        }
        if (interface->clear == NULL) {
            printf("    ✗ 接口clear函数为NULL\n");
            test_passed = false;
        }
    }
    
    if (test_passed) {
        printf("    ✓ 获取默认接口测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试2: 使用接口创建和操作数组
    printf("  2. 测试使用接口操作数组...\n");
    test_passed = true;
    
    if (interface != NULL) {
        // 创建数组句柄
        enum Eum_DynamicArrayError error_code;
        void* array_handle = interface->create(sizeof(int), 10, &error_code);
        
        if (array_handle == NULL) {
            printf("    ✗ 接口创建失败: 错误码 %d\n", error_code);
            test_passed = false;
        } else {
            // 测试添加元素
            int value = 42;
            if (!interface->push(array_handle, &value, &error_code)) {
                printf("    ✗ 接口添加元素失败: 错误码 %d\n", error_code);
                test_passed = false;
            }
            
            // 测试获取长度
            size_t length = interface->length(array_handle, &error_code);
            if (error_code != Eum_DYNAMIC_ARRAY_SUCCESS || length != 1) {
                printf("    ✗ 接口获取长度失败: 长度=%zu, 错误码=%d\n", length, error_code);
                test_passed = false;
            }
            
            // 测试获取元素
            void* retrieved = interface->get(array_handle, 0, &error_code);
            if (retrieved == NULL || error_code != Eum_DYNAMIC_ARRAY_SUCCESS) {
                printf("    ✗ 接口获取元素失败: 错误码 %d\n", error_code);
                test_passed = false;
            } else if (*(int*)retrieved != value) {
                printf("    ✗ 接口获取元素值不正确\n");
                test_passed = false;
            }
            
            // 测试设置元素
            int new_value = 99;
            if (!interface->set(array_handle, 0, &new_value, &error_code)) {
                printf("    ✗ 接口设置元素失败: 错误码 %d\n", error_code);
                test_passed = false;
            }
            
            // 测试移除元素
            if (!interface->remove(array_handle, 0, &error_code)) {
                printf("    ✗ 接口移除元素失败: 错误码 %d\n", error_code);
                test_passed = false;
            }
            
            // 测试清空数组
            if (!interface->clear(array_handle, &error_code)) {
                printf("    ✗ 接口清空数组失败: 错误码 %d\n", error_code);
                test_passed = false;
            }
            
            // 销毁数组
            interface->destroy(array_handle, &error_code);
        }
    }
    
    if (test_passed) {
        printf("    ✓ 接口操作测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    // 测试3: 接口错误处理
    printf("  3. 测试接口错误处理...\n");
    test_passed = true;
    
    if (interface != NULL) {
        // 测试无效参数
        enum Eum_DynamicArrayError error_code;
        
        // 测试无效大小
        void* invalid_handle = interface->create(0, 10, &error_code);
        if (invalid_handle != NULL) {
            printf("    ✗ 无效大小应该失败\n");
            test_passed = false;
            interface->destroy(invalid_handle, NULL);
        } else if (error_code != Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE) {
            printf("    ✗ 错误码不正确: 期望 %d, 实际 %d\n", 
                   Eum_DYNAMIC_ARRAY_ERROR_INVALID_SIZE, error_code);
            test_passed = false;
        }
        
        // 测试NULL错误码参数
        void* array_with_null_error = interface->create(sizeof(int), 10, NULL);
        if (array_with_null_error == NULL) {
            printf("    ✗ 创建数组失败，即使错误码为NULL\n");
            test_passed = false;
        } else {
            interface->destroy(array_with_null_error, NULL);
        }
    }
    
    if (test_passed) {
        printf("    ✓ 接口错误处理测试通过\n");
    }
    update_test_results(&results, test_passed);
    
    print_test_results(&results, "抽象接口");
    return results.failed_tests == 0;
}
