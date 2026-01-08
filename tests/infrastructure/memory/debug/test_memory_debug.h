/**
 * @file test_memory_debug.h
 * @brief 内存调试模块测试头文件
 * 
 * 本文件定义了内存调试模块的测试函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef TEST_MEMORY_DEBUG_H
#define TEST_MEMORY_DEBUG_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 运行所有内存调试模块测试
 * 
 * 运行内存调试模块的所有测试。
 * 
 * @return bool 所有测试通过返回true，否则返回false
 */
bool test_memory_debug_all(void);

/**
 * @brief 测试内存调试器接口
 * 
 * 测试内存调试器接口的基本功能。
 * 
 * @return bool 测试通过返回true，否则返回false
 */
bool test_memory_debug_interface(void);

/**
 * @brief 测试内存泄漏检测
 * 
 * 测试内存泄漏检测功能。
 * 
 * @return bool 测试通过返回true，否则返回false
 */
bool test_memory_leak_detection(void);

/**
 * @brief 测试内存错误检测
 * 
 * 测试内存错误检测功能。
 * 
 * @return bool 测试通过返回true，否则返回false
 */
bool test_memory_error_detection(void);

/**
 * @brief 测试内存分析功能
 * 
 * 测试内存分析功能。
 * 
 * @return bool 测试通过返回true，否则返回false
 */
bool test_memory_analysis(void);

/**
 * @brief 测试内存调试工具
 * 
 * 测试内存调试工具功能。
 * 
 * @return bool 测试通过返回true，否则返回false
 */
bool test_memory_debug_tools(void);

/**
 * @brief 测试内存调试器工厂
 * 
 * 测试内存调试器工厂功能。
 * 
 * @return bool 测试通过返回true，否则返回false
 */
bool test_memory_debug_factory(void);

/**
 * @brief 测试向后兼容性
 * 
 * 测试向后兼容性功能。
 * 
 * @return bool 测试通过返回true，否则返回false
 */
bool test_backward_compatibility(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_MEMORY_DEBUG_H
