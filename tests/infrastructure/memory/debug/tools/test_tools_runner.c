/**
 * @file test_tools_runner.c
 * @brief 内存调试工具模块测试运行器
 * 
 * 本文件是内存调试工具模块的测试运行器，负责组织和运行所有子模块的测试。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#include <stdio.h>
#include <stdbool.h>

// 声明各子模块测试函数
bool test_debug_tools_core_all(void);
bool test_debug_tools_dump_all(void);
bool test_debug_tools_validation_all(void);
bool test_debug_tools_stacktrace_all(void);
bool test_debug_tools_utils_all(void);
bool test_debug_tools_integration_all(void);

/**
 * @brief 运行所有内存调试工具模块测试
 * @return 所有测试通过返回true，否则返回false
 */
bool test_debug_tools_module_all(void)
{
    printf("\n");
    printf("========================================\n");
    printf("内存调试工具模块测试\n");
    printf("========================================\n");
    
    bool all_passed = true;
    int passed_count = 0;
    int total_count = 0;
    
    // 测试核心管理模块
    printf("\n1. 核心管理模块测试:\n");
    printf("   -----------------\n");
    if (test_debug_tools_core_all()) {
        passed_count++;
        printf("   ✅ 核心管理模块测试通过\n");
    } else {
        all_passed = false;
        printf("   ❌ 核心管理模块测试失败\n");
    }
    total_count++;
    
    // 测试内存转储模块
    printf("\n2. 内存转储模块测试:\n");
    printf("   -----------------\n");
    if (test_debug_tools_dump_all()) {
        passed_count++;
        printf("   ✅ 内存转储模块测试通过\n");
    } else {
        all_passed = false;
        printf("   ❌ 内存转储模块测试失败\n");
    }
    total_count++;
    
    // 测试内存验证模块
    printf("\n3. 内存验证模块测试:\n");
    printf("   -----------------\n");
    if (test_debug_tools_validation_all()) {
        passed_count++;
        printf("   ✅ 内存验证模块测试通过\n");
    } else {
        all_passed = false;
        printf("   ❌ 内存验证模块测试失败\n");
    }
    total_count++;
    
    // 测试调用栈跟踪模块
    printf("\n4. 调用栈跟踪模块测试:\n");
    printf("   -------------------\n");
    if (test_debug_tools_stacktrace_all()) {
        passed_count++;
        printf("   ✅ 调用栈跟踪模块测试通过\n");
    } else {
        all_passed = false;
        printf("   ❌ 调用栈跟踪模块测试失败\n");
    }
    total_count++;
    
    // 测试工具函数模块
    printf("\n5. 工具函数模块测试:\n");
    printf("   -----------------\n");
    if (test_debug_tools_utils_all()) {
        passed_count++;
        printf("   ✅ 工具函数模块测试通过\n");
    } else {
        all_passed = false;
        printf("   ❌ 工具函数模块测试失败\n");
    }
    total_count++;
    
    // 测试集成功能
    printf("\n6. 集成功能测试:\n");
    printf("   -------------\n");
    if (test_debug_tools_integration_all()) {
        passed_count++;
        printf("   ✅ 集成功能测试通过\n");
    } else {
        all_passed = false;
        printf("   ❌ 集成功能测试失败\n");
    }
    total_count++;
    
    // 输出测试总结
    printf("\n========================================\n");
    printf("测试总结:\n");
    printf("  总测试数: %d\n", total_count);
    printf("  通过数: %d\n", passed_count);
    printf("  失败数: %d\n", total_count - passed_count);
    
    if (all_passed) {
        printf("  ✅ 所有内存调试工具模块测试通过\n");
    } else {
        printf("  ❌ 部分内存调试工具模块测试失败\n");
    }
    printf("========================================\n");
    
    return all_passed;
}

/**
 * @brief 主测试函数（供外部调用）
 * @return 所有测试通过返回true，否则返回false
 */
bool test_memory_debug_tools_module(void)
{
    return test_debug_tools_module_all();
}
