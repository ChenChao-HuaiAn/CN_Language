/**
 * @file test_queue_main.c
 * @brief 队列模块测试运行器主文件
 * 
 * 实现队列模块的测试运行器和main函数。
 * 遵循模块化测试设计，测试文件和main()函数分离。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ==================== 测试运行器实现 ==================== */

int run_all_queue_tests(void)
{
    int passed = 0;
    int total = 0;
    
    printf("=== 队列模块测试套件 ===\n\n");
    
    /* ==================== 核心模块测试 ==================== */
    printf("--- 核心模块测试 ---\n");
    
    total++; printf("[%d/%d] ", total, 17);
    if (test_queue_core_create_and_destroy()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    total++; printf("[%d/%d] ", total, 17);
    if (test_queue_core_basic_operations()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    total++; printf("[%d/%d] ", total, 17);
    if (test_queue_core_query_operations()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    total++; printf("[%d/%d] ", total, 17);
    if (test_queue_core_management_operations()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    total++; printf("[%d/%d] ", total, 17);
    if (test_queue_core_error_handling()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    /* ==================== 迭代器模块测试 ==================== */
    printf("\n--- 迭代器模块测试 ---\n");
    
    total++; printf("[%d/%d] ", total, 17);
    if (test_queue_iterator_create_and_destroy()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    total++; printf("[%d/%d] ", total, 17);
    if (test_queue_iterator_traversal()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    total++; printf("[%d/%d] ", total, 17);
    if (test_queue_iterator_state_queries()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    total++; printf("[%d/%d] ", total, 17);
    if (test_queue_iterator_reset()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    /* ==================== 工具模块测试 ==================== */
    printf("\n--- 工具模块测试 ---\n");
    
    total++; printf("[%d/%d] ", total, 17);
    if (test_queue_utils_copy()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    total++; printf("[%d/%d] ", total, 17);
    if (test_queue_utils_equals()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    total++; printf("[%d/%d] ", total, 17);
    if (test_queue_utils_batch_operations()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    total++; printf("[%d/%d] ", total, 17);
    if (test_queue_utils_conversion()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    total++; printf("[%d/%d] ", total, 17);
    if (test_queue_utils_search()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    /* ==================== 综合测试 ==================== */
    printf("\n--- 综合测试 ---\n");
    
    total++; printf("[%d/%d] ", total, 17);
    if (test_queue_edge_cases()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    total++; printf("[%d/%d] ", total, 17);
    if (test_queue_performance()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    total++; printf("[%d/%d] ", total, 17);
    if (test_queue_integration()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    /* ==================== 测试结果 ==================== */
    printf("\n=== 测试结果 ===\n");
    printf("总测试数: %d\n", total);
    printf("通过: %d\n", passed);
    printf("失败: %d\n", total - passed);
    
    if (passed == total)
    {
        printf("通过率: 100.00%%\n");
        printf("\n所有测试通过！队列模块功能正常。\n");
    }
    else
    {
        printf("通过率: %.2f%%\n", (float)passed / total * 100.0f);
        printf("\n有测试失败，请检查相关功能。\n");
    }
    
    return passed;
}

int run_queue_core_tests(void)
{
    int passed = 0;
    int total = 5;
    
    printf("=== 队列核心模块测试 ===\n\n");
    
    printf("[1/%d] ", total);
    if (test_queue_core_create_and_destroy()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    printf("[2/%d] ", total);
    if (test_queue_core_basic_operations()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    printf("[3/%d] ", total);
    if (test_queue_core_query_operations()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    printf("[4/%d] ", total);
    if (test_queue_core_management_operations()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    printf("[5/%d] ", total);
    if (test_queue_core_error_handling()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    printf("\n核心模块测试结果: %d/%d 通过\n", passed, total);
    return passed;
}

int run_queue_iterator_tests(void)
{
    int passed = 0;
    int total = 4;
    
    printf("=== 队列迭代器模块测试 ===\n\n");
    
    printf("[1/%d] ", total);
    if (test_queue_iterator_create_and_destroy()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    printf("[2/%d] ", total);
    if (test_queue_iterator_traversal()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    printf("[3/%d] ", total);
    if (test_queue_iterator_state_queries()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    printf("[4/%d] ", total);
    if (test_queue_iterator_reset()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    printf("\n迭代器模块测试结果: %d/%d 通过\n", passed, total);
    return passed;
}

int run_queue_utils_tests(void)
{
    int passed = 0;
    int total = 5;
    
    printf("=== 队列工具模块测试 ===\n\n");
    
    printf("[1/%d] ", total);
    if (test_queue_utils_copy()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    printf("[2/%d] ", total);
    if (test_queue_utils_equals()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    printf("[3/%d] ", total);
    if (test_queue_utils_batch_operations()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    printf("[4/%d] ", total);
    if (test_queue_utils_conversion()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    printf("[5/%d] ", total);
    if (test_queue_utils_search()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
    
    printf("\n工具模块测试结果: %d/%d 通过\n", passed, total);
    return passed;
}

/* ==================== 总测试运行器接口 ==================== */

/**
 * @brief 运行所有队列测试（总测试运行器接口）
 * @return 所有测试通过返回true，否则返回false
 */
bool test_queue_all(void)
{
    int passed = run_all_queue_tests();
    return (passed == 17); // 总共有17个测试
}

/* ==================== 主函数 ==================== */

int main(int argc, char* argv[])
{
    printf("CN_Language 队列模块测试程序\n");
    printf("版本: 1.0.0\n");
    printf("构建日期: %s %s\n", __DATE__, __TIME__);
    printf("\n");
    
    // 检查命令行参数
    if (argc > 1)
    {
        char* test_type = argv[1];
        
        if (strcmp(test_type, "core") == 0)
        {
            printf("运行核心模块测试...\n\n");
            int result = run_queue_core_tests();
            return (result == 5) ? 0 : 1;
        }
        else if (strcmp(test_type, "iterator") == 0)
        {
            printf("运行迭代器模块测试...\n\n");
            int result = run_queue_iterator_tests();
            return (result == 4) ? 0 : 1;
        }
        else if (strcmp(test_type, "utils") == 0)
        {
            printf("运行工具模块测试...\n\n");
            int result = run_queue_utils_tests();
            return (result == 5) ? 0 : 1;
        }
        else if (strcmp(test_type, "integration") == 0)
        {
            printf("运行集成测试...\n\n");
            // 运行集成测试
            int passed = 0;
            int total = 3;
            
            printf("=== 队列集成测试 ===\n\n");
            
            printf("[1/%d] ", total);
            if (test_queue_edge_cases()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
            
            printf("[2/%d] ", total);
            if (test_queue_performance()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
            
            printf("[3/%d] ", total);
            if (test_queue_integration()) { printf("通过\n"); passed++; } else { printf("失败\n"); }
            
            printf("\n集成测试结果: %d/%d 通过\n", passed, total);
            return (passed == total) ? 0 : 1;
        }
        else if (strcmp(test_type, "help") == 0 || strcmp(test_type, "-h") == 0 || strcmp(test_type, "--help") == 0)
        {
            printf("用法: test_queue [测试类型]\n");
            printf("\n测试类型:\n");
            printf("  core       运行核心模块测试\n");
            printf("  iterator   运行迭代器模块测试\n");
            printf("  utils      运行工具模块测试\n");
            printf("  integration 运行集成测试\n");
            printf("  all        运行所有测试（默认）\n");
            printf("  help       显示此帮助信息\n");
            printf("\n示例:\n");
            printf("  test_queue core\n");
            printf("  test_queue iterator\n");
            printf("  test_queue all\n");
            return 0;
        }
    }
    
    // 默认运行所有测试
    printf("运行所有队列模块测试...\n\n");
    int result = run_all_queue_tests();
    
    if (result == 17)
    {
        printf("\n所有测试通过！队列模块功能正常。\n");
        return 0;
    }
    else
    {
        printf("\n有测试失败，请检查相关功能。\n");
        return 1;
    }
}
