/**
 * @file test_dynamic_array_common.c
 * @brief 动态数组模块测试公共实现文件
 * 
 * 实现动态数组测试的公共辅助函数。
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
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

// 比较函数实现
int compare_int(const void* a, const void* b)
{
    int int_a = *(const int*)a;
    int int_b = *(const int*)b;
    return int_a - int_b;
}

// 带索引的迭代器回调函数实现
bool print_int_iterator(void* item, size_t index, void* user_data)
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

// 回调函数实现
void print_int_callback(void* item, void* user_data)
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

// 提前停止的迭代器回调函数实现
bool stop_at_third(void* item, size_t index, void* user_data)
{
    (void)index;      // 标记为已使用以避免警告
    (void)user_data;  // 标记为已使用以避免警告
    
    int* value = (int*)item;
    if (value != NULL && *value == 30) { // 在值为30时停止
        return false;
    }
    return true;
}

// 初始化测试结果
void init_test_results(TestResults* results)
{
    if (results == NULL) {
        return;
    }
    
    results->total_tests = 0;
    results->passed_tests = 0;
    results->failed_tests = 0;
}

// 更新测试结果
void update_test_results(TestResults* results, bool test_passed)
{
    if (results == NULL) {
        return;
    }
    
    results->total_tests++;
    if (test_passed) {
        results->passed_tests++;
    } else {
        results->failed_tests++;
    }
}

// 打印测试结果
void print_test_results(const TestResults* results, const char* test_name)
{
    if (results == NULL || test_name == NULL) {
        return;
    }
    
    printf("\n%s测试结果:\n", test_name);
    printf("  总测试数: %d\n", results->total_tests);
    printf("  通过: %d\n", results->passed_tests);
    printf("  失败: %d\n", results->failed_tests);
    printf("  通过率: %.2f%%\n", 
           results->total_tests > 0 ? 
           (float)results->passed_tests / results->total_tests * 100.0f : 0.0f);
}
