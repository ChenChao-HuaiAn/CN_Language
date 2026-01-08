/**
 * @file test_error_detection.h
 * @brief 错误检测模块测试头文件
 * 
 * 本文件定义了错误检测模块的测试函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef TEST_ERROR_DETECTION_H
#define TEST_ERROR_DETECTION_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 运行所有错误检测测试
 * 
 * 运行错误检测模块的所有测试。
 * 
 * @return bool 所有测试通过返回true，否则返回false
 */
bool test_error_detection_all(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_ERROR_DETECTION_H
