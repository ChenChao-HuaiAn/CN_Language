/**
 * @file test_queue.h
 * @brief 队列模块测试头文件
 * 
 * 声明队列模块的所有测试函数。
 * 遵循模块化测试设计，测试文件和main()函数分离。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef TEST_QUEUE_H
#define TEST_QUEUE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 核心模块测试 ==================== */

/**
 * @brief 测试队列创建和销毁功能
 * @return 测试通过返回true，失败返回false
 */
bool test_queue_core_create_and_destroy(void);

/**
 * @brief 测试队列基本操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_queue_core_basic_operations(void);

/**
 * @brief 测试队列查询操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_queue_core_query_operations(void);

/**
 * @brief 测试队列管理功能
 * @return 测试通过返回true，失败返回false
 */
bool test_queue_core_management_operations(void);

/**
 * @brief 测试队列错误处理功能
 * @return 测试通过返回true，失败返回false
 */
bool test_queue_core_error_handling(void);

/* ==================== 迭代器模块测试 ==================== */

/**
 * @brief 测试迭代器创建和销毁功能
 * @return 测试通过返回true，失败返回false
 */
bool test_queue_iterator_create_and_destroy(void);

/**
 * @brief 测试迭代器遍历功能
 * @return 测试通过返回true，失败返回false
 */
bool test_queue_iterator_traversal(void);

/**
 * @brief 测试迭代器状态查询功能
 * @return 测试通过返回true，失败返回false
 */
bool test_queue_iterator_state_queries(void);

/**
 * @brief 测试迭代器重置功能
 * @return 测试通过返回true，失败返回false
 */
bool test_queue_iterator_reset(void);

/* ==================== 工具模块测试 ==================== */

/**
 * @brief 测试队列复制功能
 * @return 测试通过返回true，失败返回false
 */
bool test_queue_utils_copy(void);

/**
 * @brief 测试队列比较功能
 * @return 测试通过返回true，失败返回false
 */
bool test_queue_utils_equals(void);

/**
 * @brief 测试批量操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_queue_utils_batch_operations(void);

/**
 * @brief 测试队列转换功能
 * @return 测试通过返回true，失败返回false
 */
bool test_queue_utils_conversion(void);

/**
 * @brief 测试查找功能
 * @return 测试通过返回true，失败返回false
 */
bool test_queue_utils_search(void);

/* ==================== 综合测试 ==================== */

/**
 * @brief 测试队列边界情况
 * @return 测试通过返回true，失败返回false
 */
bool test_queue_edge_cases(void);

/**
 * @brief 测试队列性能特征
 * @return 测试通过返回true，失败返回false
 */
bool test_queue_performance(void);

/**
 * @brief 测试队列模块集成
 * @return 测试通过返回true，失败返回false
 */
bool test_queue_integration(void);

/* ==================== 测试运行器 ==================== */

/**
 * @brief 运行所有队列测试
 * @return 通过的测试数量
 */
int run_all_queue_tests(void);

/**
 * @brief 运行核心模块测试
 * @return 通过的测试数量
 */
int run_queue_core_tests(void);

/**
 * @brief 运行迭代器模块测试
 * @return 通过的测试数量
 */
int run_queue_iterator_tests(void);

/**
 * @brief 运行工具模块测试
 * @return 通过的测试数量
 */
int run_queue_utils_tests(void);

/**
 * @brief 运行所有队列测试（总测试运行器接口）
 * @return 所有测试通过返回true，否则返回false
 */
bool test_queue_all(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_QUEUE_H
