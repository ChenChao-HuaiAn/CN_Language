/**
 * @file test_runner.c
 * @brief 哈希表模块测试运行器
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * 
 * @copyright MIT许可证 - 详见项目根目录LICENSE文件
 * 
 * 测试运行器文件，包含main()函数，用于运行哈希表模块的所有测试。
 * 遵循项目架构规范，测试文件和main()函数分离，便于集成到更大的测试框架。
 */

#include <stdio.h>
#include <stdlib.h>
#include "test_hash_table.h"

/**
 * @brief 主函数 - 运行所有哈希表测试
 * 
 * @return int 程序退出码：0表示所有测试通过，非0表示有测试失败
 * 
 * @note 函数会运行所有10个测试函数，并输出详细的测试结果统计。
 *       遵循模块化测试设计原则，测试文件和main()函数分离。
 */
int main(void)
{
    printf("=== 哈希表模块测试套件 ===\n\n");
    
    // 运行所有测试
    int passed = 0;
    int failed = 0;
    
    // 测试1: 创建和销毁功能
    printf("[1/10] test_hash_table_create_and_destroy... ");
    if (test_hash_table_create_and_destroy())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 测试2: 基本操作功能
    printf("[2/10] test_hash_table_basic_operations... ");
    if (test_hash_table_basic_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 测试3: 查询操作功能
    printf("[3/10] test_hash_table_query_operations... ");
    if (test_hash_table_query_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 测试4: 哈希表操作功能
    printf("[4/10] test_hash_table_table_operations... ");
    if (test_hash_table_table_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 测试5: 条目操作功能
    printf("[5/10] test_hash_table_entry_operations... ");
    if (test_hash_table_entry_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 测试6: 工具函数功能
    printf("[6/10] test_hash_table_utils_functions... ");
    if (test_hash_table_utils_functions())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 测试7: 抽象接口功能
    printf("[7/10] test_hash_table_abstract_interface... ");
    if (test_hash_table_abstract_interface())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 测试8: 错误处理功能
    printf("[8/10] test_hash_table_error_handling... ");
    if (test_hash_table_error_handling())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 测试9: 边界情况
    printf("[9/10] test_hash_table_edge_cases... ");
    if (test_hash_table_edge_cases())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 测试10: 性能特征
    printf("[10/10] test_hash_table_performance... ");
    if (test_hash_table_performance())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 输出测试结果统计
    printf("\n=== 测试结果 ===\n");
    printf("总测试数: %d\n", passed + failed);
    printf("通过: %d\n", passed);
    printf("失败: %d\n", failed);
    
    if (failed == 0)
    {
        printf("通过率: 100.00%%\n");
        printf("\n所有测试通过！哈希表模块功能正常。\n");
        return 0;
    }
    else
    {
        printf("通过率: %.2f%%\n", (float)passed / (passed + failed) * 100.0f);
        printf("\n有测试失败，请检查相关功能。\n");
        return 1;
    }
}
