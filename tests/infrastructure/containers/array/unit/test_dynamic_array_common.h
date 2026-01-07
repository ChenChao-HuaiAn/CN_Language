/**
 * @file test_dynamic_array_common.h
 * @brief 动态数组模块测试公共头文件
 * 
 * 包含动态数组测试的公共声明和辅助函数。
 * 遵循模块化测试设计，每个测试功能在单独的文件中。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef TEST_DYNAMIC_ARRAY_COMMON_H
#define TEST_DYNAMIC_ARRAY_COMMON_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 测试辅助函数声明
int compare_int(const void* a, const void* b);
bool print_int_iterator(void* item, size_t index, void* user_data);
void print_int_callback(void* item, void* user_data);
bool stop_at_third(void* item, size_t index, void* user_data);

// 测试结果统计
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
} TestResults;

// 初始化测试结果
void init_test_results(TestResults* results);

// 更新测试结果
void update_test_results(TestResults* results, bool test_passed);

// 打印测试结果
void print_test_results(const TestResults* results, const char* test_name);

#ifdef __cplusplus
}
#endif

#endif // TEST_DYNAMIC_ARRAY_COMMON_H
