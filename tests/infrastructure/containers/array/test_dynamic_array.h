/**
 * @file test_dynamic_array.h
 * @brief 动态数组模块测试头文件
 * 
 * 声明动态数组模块的所有测试函数。
 * 遵循模块化测试设计，测试文件和main()函数分离。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef TEST_DYNAMIC_ARRAY_H
#define TEST_DYNAMIC_ARRAY_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 测试创建和销毁功能
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_create_and_destroy(void);

/**
 * @brief 测试基本操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_basic_operations(void);

/**
 * @brief 测试查询操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_query_operations(void);

/**
 * @brief 测试数组操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_array_operations(void);

/**
 * @brief 测试迭代操作功能
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_iteration_operations(void);

/**
 * @brief 测试内存管理功能
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_memory_management(void);

/**
 * @brief 测试错误处理功能
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_error_handling(void);

/**
 * @brief 测试抽象接口功能
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_abstract_interface(void);

/**
 * @brief 测试边界情况
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_edge_cases(void);

/**
 * @brief 测试性能特征
 * @return 测试通过返回true，失败返回false
 */
bool test_dynamic_array_performance(void);

/**
 * @brief 运行所有动态数组测试
 * @return 通过的测试数量
 */
int run_all_dynamic_array_tests(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_DYNAMIC_ARRAY_H
