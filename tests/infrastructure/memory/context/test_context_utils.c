/**
 * @file test_context_utils.c
 * @brief 内存上下文模块测试辅助函数
 * 
 * 内存上下文模块的测试辅助函数实现，提供测试框架支持。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_context.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief 打印测试模块头部信息
 * 
 * @param module_name 模块名称
 */
void test_context_print_header(const char* module_name)
{
    printf("\n");
    printf("========================================\n");
    printf("    内存上下文模块 - %s测试\n", module_name);
    printf("========================================\n");
    printf("\n");
}

/**
 * @brief 打印单个测试结果
 * 
 * @param test_name 测试名称
 * @param passed 是否通过
 */
void test_context_print_result(const char* test_name, bool passed)
{
    if (passed) {
        printf("  ✅ %s\n", test_name);
    } else {
        printf("  ❌ %s\n", test_name);
    }
}

/**
 * @brief 打印测试总结
 * 
 * @param result 测试结果
 */
void test_context_print_summary(const TestContextResult* result)
{
    printf("\n");
    printf("测试总结:\n");
    printf("  ├─ 模块名称:      %s\n", result->module_name);
    printf("  ├─ 总测试数:      %d\n", result->total_tests);
    printf("  ├─ 通过测试:      %d\n", result->passed_tests);
    printf("  ├─ 失败测试:      %d\n", result->failed_tests);
    
    if (result->total_tests > 0) {
        double pass_rate = (double)result->passed_tests / result->total_tests * 100;
        printf("  └─ 通过率:        %.1f%%\n", pass_rate);
    } else {
        printf("  └─ 通过率:        N/A\n");
    }
    
    printf("\n");
    
    if (result->failed_tests == 0) {
        printf("✅ 所有测试通过！\n");
    } else {
        printf("❌ 有 %d 个测试失败\n", result->failed_tests);
    }
    printf("\n");
}

/**
 * @brief 运行测试套件
 * 
 * @param tests 测试用例数组
 * @param test_count 测试用例数量
 * @param suite_name 套件名称
 * @return bool 是否所有测试都通过
 */
bool run_test_suite(const TestCase* tests, int test_count, const char* suite_name)
{
    TestContextResult result = {0};
    result.module_name = suite_name;
    result.total_tests = 0;
    
    printf("运行测试套件: %s\n", suite_name);
    printf("================\n\n");
    
    for (int i = 0; i < test_count; i++) {
        if (!tests[i].enabled) {
            continue;
        }
        
        result.total_tests++;
        printf("测试: %s\n", tests[i].test_name);
        
        bool test_passed = tests[i].test_func();
        test_context_print_result(tests[i].test_name, test_passed);
        
        if (test_passed) {
            result.passed_tests++;
        } else {
            result.failed_tests++;
        }
        
        printf("\n");
    }
    
    test_context_print_summary(&result);
    
    return result.failed_tests == 0;
}
