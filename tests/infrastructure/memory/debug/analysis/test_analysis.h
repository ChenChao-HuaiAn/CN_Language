/**
 * @file test_analysis.h
 * @brief 分析模块测试头文件
 * 
 * 本文件定义了分析模块的测试函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef TEST_ANALYSIS_H
#define TEST_ANALYSIS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 运行所有分析测试
 * 
 * 运行分析模块的所有测试。
 * 
 * @return bool 所有测试通过返回true，否则返回false
 */
bool test_analysis_all(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_ANALYSIS_H
