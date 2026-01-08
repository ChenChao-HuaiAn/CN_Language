/**
 * @file test_debug_tools.h
 * @brief 调试工具模块测试头文件
 * 
 * 本文件定义了调试工具模块的测试函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef TEST_DEBUG_TOOLS_H
#define TEST_DEBUG_TOOLS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 运行所有调试工具测试
 * 
 * 运行调试工具模块的所有测试。
 * 
 * @return bool 所有测试通过返回true，否则返回false
 */
bool test_debug_tools_module_all(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_DEBUG_TOOLS_H
