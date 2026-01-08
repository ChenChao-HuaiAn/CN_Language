/**
 * @file test_context.h
 * @brief 内存上下文模块测试头文件
 * 
 * 内存上下文模块的测试头文件，包含测试函数声明和测试辅助函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef TEST_CONTEXT_H
#define TEST_CONTEXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* 测试结果结构 */
typedef struct {
    int total_tests;      // 总测试数
    int passed_tests;     // 通过的测试数
    int failed_tests;     // 失败的测试数
    const char* module_name; // 模块名称
} TestContextResult;

/* 测试函数类型 */
typedef bool (*TestFunction)(void);

/* 测试用例结构 */
typedef struct {
    const char* test_name;    // 测试名称
    TestFunction test_func;   // 测试函数
    bool enabled;            // 是否启用
} TestCase;

/* 测试辅助函数 */
void test_context_print_header(const char* module_name);
void test_context_print_result(const char* test_name, bool passed);
void test_context_print_summary(const TestContextResult* result);

/* 测试运行函数 */
bool run_test_suite(const TestCase* tests, int test_count, const char* suite_name);

/* 测试断言宏 */
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf("❌ 断言失败: %s\n", #condition); \
            printf("    文件: %s, 行: %d\n", __FILE__, __LINE__); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("❌ 断言失败: %s != %s\n", #expected, #actual); \
            printf("    期望值: %lld, 实际值: %lld\n", (long long)(expected), (long long)(actual)); \
            printf("    文件: %s, 行: %d\n", __FILE__, __LINE__); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            printf("❌ 断言失败: %s 为 NULL\n", #ptr); \
            printf("    文件: %s, 行: %d\n", __FILE__, __LINE__); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            printf("❌ 断言失败: %s 不为 NULL\n", #ptr); \
            printf("    实际值: %p\n", (void*)(ptr)); \
            printf("    文件: %s, 行: %d\n", __FILE__, __LINE__); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_STRING_EQUAL(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf("❌ 字符串断言失败: %s != %s\n", #expected, #actual); \
            printf("    期望值: \"%s\", 实际值: \"%s\"\n", (expected), (actual)); \
            printf("    文件: %s, 行: %d\n", __FILE__, __LINE__); \
            return false; \
        } \
    } while (0)

#endif /* TEST_CONTEXT_H */
