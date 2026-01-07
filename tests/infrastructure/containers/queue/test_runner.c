/**
 * @file test_runner.c
 * @brief 队列模块测试运行器
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * 
 * @copyright MIT许可证 - 详见项目根目录LICENSE文件
 * 
 * 测试运行器文件，包含main()函数，用于运行队列模块的所有测试。
 * 遵循项目架构规范，测试文件和main()函数分离，便于集成到更大的测试框架。
 */

#include <stdio.h>
#include <stdlib.h>
#include "test_queue.h"

/**
 * @brief 运行所有队列测试
 * 
 * @return int 通过的测试数量
 */
int run_all_queue_tests(void)
{
    int passed = 0;
    int total = 0;
    
    printf("=== 队列模块测试套件 ===\n\n");
    
    /* ==================== 核心模块测试 ==================== */
    printf("--- 核心模块测试 ---\n");
    
    total++;
    printf("[%d/%d] test_queue_core_create_and_destroy... ", total, 17);
    if (test_queue_core_create_and_destroy())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    total++;
    printf("[%d/%d] test_queue_core_basic_operations... ", total, 17);
    if (test_queue_core_basic_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    total++;
    printf("[%d/%d] test_queue_core_query_operations... ", total, 17);
    if (test_queue_core_query_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    total++;
    printf("[%d/%d] test_queue_core_management_operations... ", total, 17);
    if (test_queue_core_management_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    total++;
    printf("[%d/%d] test_queue_core_error_handling... ", total, 17);
    if (test_queue_core_error_handling())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    /* ==================== 迭代器模块测试 ==================== */
    printf("\n--- 迭代器模块测试 ---\n");
    
    total++;
    printf("[%d/%d] test_queue_iterator_create_and_destroy... ", total, 17);
    if (test_queue_iterator_create_and_destroy())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    total++;
    printf("[%d/%d] test_queue_iterator_traversal... ", total, 17);
    if (test_queue_iterator_traversal())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    /* ==================== 工具模块测试 ==================== */
    printf("\n--- 工具模块测试 ---\n");
    
    total++;
    printf("[%d/%d] test_queue_utils_copy... ", total, 17);
    if (test_queue_utils_copy())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    total++;
    printf("[%d/%d] test_queue_utils_batch_operations... ", total, 17);
    if (test_queue_utils_batch_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    /* ==================== 综合测试 ==================== */
    printf("\n--- 综合测试 ---\n");
    
    total++;
    printf("[%d/%d] test_queue_edge_cases... ", total, 17);
    if (test_queue_edge_cases())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
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

/**
 * @brief 运行核心模块测试
 * 
 * @return int 通过的测试数量
 */
int run_queue_core_tests(void)
{
    int passed = 0;
    int total = 5;
    
    printf("=== 队列核心模块测试 ===\n\n");
    
    printf("[1/%d] test_queue_core_create_and_destroy... ", total);
    if (test_queue_core_create_and_destroy())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    printf("[2/%d] test_queue_core_basic_operations... ", total);
    if (test_queue_core_basic_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    printf("[3/%d] test_queue_core_query_operations... ", total);
    if (test_queue_core_query_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    printf("[4/%d] test_queue_core_management_operations... ", total);
    if (test_queue_core_management_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    printf("[5/%d] test_queue_core_error_handling... ", total);
    if (test_queue_core_error_handling())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    printf("\n核心模块测试结果: %d/%d 通过\n", passed, total);
    return passed;
}

/**
 * @brief 运行迭代器模块测试
 * 
 * @return int 通过的测试数量
 */
int run_queue_iterator_tests(void)
{
    int passed = 0;
    int total = 2;
    
    printf("=== 队列迭代器模块测试 ===\n\n");
    
    printf("[1/%d] test_queue_iterator_create_and_destroy... ", total);
    if (test_queue_iterator_create_and_destroy())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    printf("[2/%d] test_queue_iterator_traversal... ", total);
    if (test_queue_iterator_traversal())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    printf("\n迭代器模块测试结果: %d/%d 通过\n", passed, total);
    return passed;
}

/**
 * @brief 运行工具模块测试
 * 
 * @return int 通过的测试数量
 */
int run_queue_utils_tests(void)
{
    int passed = 0;
    int total = 2;
    
    printf("=== 队列工具模块测试 ===\n\n");
    
    printf("[1/%d] test_queue_utils_copy... ", total);
    if (test_queue_utils_copy())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    printf("[2/%d] test_queue_utils_batch_operations... ", total);
    if (test_queue_utils_batch_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
    }
    
    printf("\n工具模块测试结果: %d/%d 通过\n", passed, total);
    return passed;
}

/**
 * @brief 主函数 - 运行所有队列测试
 * 
 * @return int 程序退出码：0表示所有测试通过，非0表示有测试失败
 */
int main(void)
{
    int passed = run_all_queue_tests();
    int total = 17; // 当前实现的测试数量
    
    return (passed == total) ? 0 : 1;
}
