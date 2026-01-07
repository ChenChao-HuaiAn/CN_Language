/**
 * @file test_stack.h
 * @brief 栈模块测试头文件
 * 
 * 提供栈模块测试的公共定义和函数声明。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef TEST_STACK_H
#define TEST_STACK_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 测试宏定义 */
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("❌ 测试失败: %s (文件: %s, 行: %d)\n", \
                   message, __FILE__, __LINE__); \
            return false; \
        } \
    } while (0)

#define TEST_PASS(message) \
    do { \
        printf("✅ %s\n", message); \
    } while (0)

#define TEST_START(name) \
    do { \
        printf("开始测试: %s\n", name); \
    } while (0)

#define TEST_END(name) \
    do { \
        printf("完成测试: %s\n\n", name); \
    } while (0)

/* 测试函数声明 - 核心模块 */
bool test_stack_create_destroy(void);
bool test_stack_basic_operations(void);
bool test_stack_edge_cases(void);
bool test_stack_memory_management(void);
bool test_stack_error_handling(void);
bool test_stack_performance(void);

/* 测试函数声明 - 迭代器模块 */
bool test_stack_iterator_create_destroy(void);
bool test_stack_iterator_traversal(void);
bool test_stack_iterator_directions(void);
bool test_stack_iterator_edge_cases(void);
bool test_stack_iterator_error_handling(void);

/* 测试函数声明 - 工具模块 */
bool test_stack_utils_batch_operations(void);
bool test_stack_utils_copy_functions(void);
bool test_stack_utils_comparison(void);
bool test_stack_utils_find_functions(void);
bool test_stack_utils_stack_operations(void);
bool test_stack_utils_conversion(void);
bool test_stack_utils_error_handling(void);

/* 测试运行器函数声明 */
bool test_stack_core_all(void);
bool test_stack_iterator_all(void);
bool test_stack_utils_all(void);

#ifdef __cplusplus
}
#endif

#endif /* TEST_STACK_H */
