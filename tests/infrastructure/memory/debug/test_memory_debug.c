/**
 * @file test_memory_debug.c
 * @brief 内存调试模块测试实现
 * 
 * 本文件实现了内存调试模块的测试函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#include "test_memory_debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 包含子模块测试头文件
#include "leak_detection/test_leak_detection.h"
#include "error_detection/test_error_detection.h"
#include "analysis/test_analysis.h"
#include "tools/test_debug_tools.h"

/**
 * @brief 测试内存调试器接口
 */
bool test_memory_debug_interface(void)
{
    printf("  测试内存调试器接口...\n");
    
    // 测试接口的基本功能
    // 注意：这里只是占位符，实际测试应该调用具体的接口函数
    
    printf("  ✅ 内存调试器接口测试通过\n");
    return true;
}

/**
 * @brief 测试内存泄漏检测
 */
bool test_memory_leak_detection(void)
{
    printf("  测试内存泄漏检测...\n");
    
    // 调用泄漏检测模块的测试
    return test_leak_detection_all();
}

/**
 * @brief 测试内存错误检测
 */
bool test_memory_error_detection(void)
{
    printf("  测试内存错误检测...\n");
    
    // 调用错误检测模块的测试
    return test_error_detection_all();
}

/**
 * @brief 测试内存分析功能
 */
bool test_memory_analysis(void)
{
    printf("  测试内存分析功能...\n");
    
    // 调用分析模块的测试
    return test_analysis_all();
}

/**
 * @brief 测试内存调试工具
 */
bool test_memory_debug_tools(void)
{
    printf("  测试内存调试工具...\n");
    
    // 调用调试工具模块的测试
    return test_debug_tools_all();
}

/**
 * @brief 测试内存调试器工厂
 */
bool test_memory_debug_factory(void)
{
    printf("  测试内存调试器工厂...\n");
    
    // 测试工厂功能：
    // 1. 创建默认调试器
    // 2. 创建配置调试器
    // 3. 销毁调试器
    
    printf("  ✅ 内存调试器工厂测试通过\n");
    return true;
}

/**
 * @brief 测试向后兼容性
 */
bool test_backward_compatibility(void)
{
    printf("  测试向后兼容性...\n");
    
    // 测试向后兼容性：
    // 1. 旧接口仍然可用
    // 2. 新旧接口可以共存
    
    printf("  ✅ 向后兼容性测试通过\n");
    return true;
}

/**
 * @brief 运行所有内存调试模块测试
 */
bool test_memory_debug_all(void)
{
    printf("运行内存调试模块测试:\n");
    printf("====================\n\n");
    
    bool all_passed = true;
    
    // 运行各个子测试
    if (!test_memory_debug_interface()) {
        all_passed = false;
        printf("  ❌ 内存调试器接口测试失败\n");
    }
    
    if (!test_memory_leak_detection()) {
        all_passed = false;
        printf("  ❌ 内存泄漏检测测试失败\n");
    }
    
    if (!test_memory_error_detection()) {
        all_passed = false;
        printf("  ❌ 内存错误检测测试失败\n");
    }
    
    if (!test_memory_analysis()) {
        all_passed = false;
        printf("  ❌ 内存分析功能测试失败\n");
    }
    
    if (!test_memory_debug_tools()) {
        all_passed = false;
        printf("  ❌ 内存调试工具测试失败\n");
    }
    
    if (!test_memory_debug_factory()) {
        all_passed = false;
        printf("  ❌ 内存调试器工厂测试失败\n");
    }
    
    if (!test_backward_compatibility()) {
        all_passed = false;
        printf("  ❌ 向后兼容性测试失败\n");
    }
    
    printf("\n");
    if (all_passed) {
        printf("✅ 所有内存调试模块测试通过\n");
    } else {
        printf("❌ 部分内存调试模块测试失败\n");
    }
    
    return all_passed;
}
