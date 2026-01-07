/**
 * @file test_dynamic_array_all.h
 * @brief 动态数组模块所有测试头文件
 * 
 * 声明动态数组模块的所有测试函数。
 * 遵循模块化测试设计，每个测试功能在单独的文件中。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef TEST_DYNAMIC_ARRAY_ALL_H
#define TEST_DYNAMIC_ARRAY_ALL_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 测试函数声明
bool test_dynamic_array_create_and_destroy(void);
bool test_dynamic_array_basic_operations(void);
bool test_dynamic_array_query_operations(void);
bool test_dynamic_array_array_operations(void);
bool test_dynamic_array_iteration_operations(void);
bool test_dynamic_array_memory_management(void);
bool test_dynamic_array_error_handling(void);
bool test_dynamic_array_abstract_interface(void);
bool test_dynamic_array_edge_cases(void);
bool test_dynamic_array_performance(void);

// 运行所有测试
int run_all_dynamic_array_tests(void);

// 统一测试接口（供总测试运行器使用）
bool test_dynamic_array_all(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_DYNAMIC_ARRAY_ALL_H
