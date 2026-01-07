/**
 * @file test_runner_unit.c
 * @brief 动态数组模块单元测试运行器
 * 
 * 运行动态数组模块的所有单元测试。
 * 遵循模块化测试设计，每个测试功能在单独的文件中。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_dynamic_array_all.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief 运行所有动态数组测试
 */
int run_all_dynamic_array_tests(void)
{
    printf("========================================\n");
    printf("开始运行动态数组模块单元测试\n");
    printf("========================================\n\n");
    
    int passed_count = 0;
    int total_count = 10; // 总测试函数数量
    
    // 运行所有测试函数
    if (test_dynamic_array_create_and_destroy()) passed_count++;
    if (test_dynamic_array_basic_operations()) passed_count++;
    if (test_dynamic_array_query_operations()) passed_count++;
    if (test_dynamic_array_array_operations()) passed_count++;
    if (test_dynamic_array_iteration_operations()) passed_count++;
    if (test_dynamic_array_memory_management()) passed_count++;
    if (test_dynamic_array_error_handling()) passed_count++;
    if (test_dynamic_array_abstract_interface()) passed_count++;
    if (test_dynamic_array_edge_cases()) passed_count++;
    if (test_dynamic_array_performance()) passed_count++;
    
    printf("========================================\n");
    printf("动态数组模块单元测试完成\n");
    printf("通过: %d/%d\n", passed_count, total_count);
    printf("========================================\n");
    
    return passed_count;
}

/**
 * @brief 统一测试接口（供总测试运行器使用）
 * @return 所有测试通过返回true，否则返回false
 */
bool test_dynamic_array_all(void)
{
    int passed = run_all_dynamic_array_tests();
    return (passed == 10); // 所有10个测试都通过
}

/**
 * @brief 主函数
 */
int main(void)
{
    int passed = run_all_dynamic_array_tests();
    
    if (passed == 10) {
        printf("\n所有测试通过！\n");
        return EXIT_SUCCESS;
    } else {
        printf("\n有测试失败！\n");
        return EXIT_FAILURE;
    }
}
