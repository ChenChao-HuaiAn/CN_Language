/**
 * @file test_hash_table.h
 * @brief 哈希表模块测试头文件
 * 
 * 声明哈希表模块的所有测试函数。
 * 遵循模块化测试设计，测试文件和main()函数分离。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef TEST_HASH_TABLE_H
#define TEST_HASH_TABLE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 测试创建和销毁功能
 * @return 测试通过返回true，失败返回false
 */
bool test_hash_table_create_and_destroy(void);

/**
 * @brief 测试基本操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_hash_table_basic_operations(void);

/**
 * @brief 测试查询操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_hash_table_query_operations(void);

/**
 * @brief 测试哈希表操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_hash_table_table_operations(void);

/**
 * @brief 测试条目操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_hash_table_entry_operations(void);

/**
 * @brief 测试工具函数功能
 * @return 测试通过返回true，失败返回false
 */
bool test_hash_table_utils_functions(void);

/**
 * @brief 测试抽象接口功能
 * @return 测试通过返回true，失败返回false
 */
bool test_hash_table_abstract_interface(void);

/**
 * @brief 测试错误处理功能
 * @return 测试通过返回true，失败返回false
 */
bool test_hash_table_error_handling(void);

/**
 * @brief 测试边界情况
 * @return 测试通过返回true，失败返回false
 */
bool test_hash_table_edge_cases(void);

/**
 * @brief 测试性能特征
 * @return 测试通过返回true，失败返回false
 */
bool test_hash_table_performance(void);

/**
 * @brief 运行所有哈希表测试
 * @return 通过的测试数量
 */
int run_all_hash_table_tests(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_HASH_TABLE_H
