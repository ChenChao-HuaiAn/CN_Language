/**
 * @file test_string.h
 * @brief 字符串模块测试头文件
 * 
 * 提供字符串模块测试的公共定义和函数声明。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef TEST_STRING_H
#define TEST_STRING_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

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

#define TEST_PASS() \
    do { \
        printf("✅ 测试通过\n"); \
    } while (0)

#define TEST_START(name) \
    do { \
        printf("开始测试: %s\n", name); \
    } while (0)

#define TEST_END() \
    do { \
        printf("完成测试\n\n"); \
    } while (0)

/* 测试函数声明 - 核心模块 */
bool test_string_create_destroy(void);
bool test_string_basic_properties(void);
bool test_string_memory_management(void);
bool test_string_edge_cases(void);
bool test_string_error_handling(void);

/* 测试函数声明 - 操作模块 */
bool test_string_append_operations(void);
bool test_string_copy_assign_operations(void);
bool test_string_comparison_operations(void);
bool test_string_substring_operations(void);
bool test_string_operations_edge_cases(void);

/* 测试函数声明 - 搜索模块 */
bool test_string_find_operations(void);
bool test_string_replace_operations(void);
bool test_string_pattern_matching(void);
bool test_string_search_edge_cases(void);
bool test_string_search_error_handling(void);

/* 测试函数声明 - 转换模块 */
bool test_string_case_conversion(void);
bool test_string_whitespace_handling(void);
bool test_string_encoding_conversion(void);
bool test_string_transform_edge_cases(void);
bool test_string_transform_error_handling(void);

/* 测试函数声明 - 工具模块 */
bool test_string_utf8_processing(void);
bool test_string_character_classification(void);
bool test_string_formatting(void);
bool test_string_parsing(void);
bool test_string_splitting(void);
bool test_string_iterator(void);
bool test_string_utils_edge_cases(void);
bool test_string_utils_error_handling(void);

/* 测试运行器函数声明 */
bool test_string_core_all(void);
bool test_string_operations_all(void);
bool test_string_search_all(void);
bool test_string_transform_all(void);
bool test_string_utils_all(void);

#ifdef __cplusplus
}
#endif

#endif /* TEST_STRING_H */
