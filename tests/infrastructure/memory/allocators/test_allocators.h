/**
 * @file test_allocators.h
 * @brief 内存分配器模块测试头文件
 * 
 * 定义了内存分配器模块的测试接口和辅助函数。
 * 遵循项目测试框架规范。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef TEST_ALLOCATORS_H
#define TEST_ALLOCATORS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 测试函数声明 */
bool test_system_allocator_all(void);
bool test_debug_allocator_all(void);
bool test_pool_allocator_all(void);
bool test_region_allocator_all(void);
bool test_allocator_factory_all(void);
bool test_allocator_config_all(void);

/* 辅助函数声明 */
void test_allocator_basic_operations(void* (*create_allocator)(void));
void test_allocator_statistics(void* (*create_allocator)(void));
void test_allocator_validation(void* (*create_allocator)(void));
void test_allocator_cleanup(void* (*create_allocator)(void));

/* 测试用例计数 */
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
} TestCaseCounter;

/* 测试用例计数器函数 */
void test_case_init(TestCaseCounter* counter);
void test_case_begin(const char* test_name, TestCaseCounter* counter);
void test_case_end(bool passed, TestCaseCounter* counter);
void test_case_summary(const TestCaseCounter* counter);

#ifdef __cplusplus
}
#endif

#endif /* TEST_ALLOCATORS_H */
