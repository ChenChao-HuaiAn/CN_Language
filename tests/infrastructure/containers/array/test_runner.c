/**
 * @file test_runner.c
 * @brief 动态数组模块测试运行器
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * 
 * @copyright MIT许可证 - 详见项目根目录LICENSE文件
 * 
 * 测试运行器文件，包含main()函数，用于运行动态数组模块的所有测试。
 * 遵循项目架构规范，测试文件和main()函数分离，便于集成到更大的测试框架。
 */

#include <stdio.h>
#include <stdlib.h>
#include "test_dynamic_array.h"

/**
 * @brief 主函数 - 运行所有动态数组测试
 * 
 * @return int 程序退出码：0表示所有测试通过，非0表示有测试失败
 * 
 * @note 函数会运行所有10个测试函数，并输出详细的测试结果统计。
 *       遵循模块化测试设计原则，测试文件和main()函数分离。
 */
int main(void)
{
    printf("=== 动态数组模块测试套件 ===\n\n");
    
    // 运行所有测试
    int passed = 0;
    int failed = 0;
    
    // 测试1: 创建和销毁功能
    printf("[1/10] test_dynamic_array_create_and_destroy... ");
    if (test_dynamic_array_create_and_destroy())
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
    printf("[2/10] test_dynamic_array_basic_operations... ");
    if (test_dynamic_array_basic_operations())
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
    printf("[3/10] test_dynamic_array_query_operations... ");
    if (test_dynamic_array_query_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 测试4: 数组操作功能
    printf("[4/10] test_dynamic_array_array_operations... ");
    if (test_dynamic_array_array_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 测试5: 迭代操作功能
    printf("[5/10] test_dynamic_array_iteration_operations... ");
    if (test_dynamic_array_iteration_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 测试6: 内存管理功能
    printf("[6/10] test_dynamic_array_memory_management... ");
    if (test_dynamic_array_memory_management())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 测试7: 错误处理功能
    printf("[7/10] test_dynamic_array_error_handling... ");
    if (test_dynamic_array_error_handling())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 测试8: 抽象接口功能
    printf("[8/10] test_dynamic_array_abstract_interface... ");
    if (test_dynamic_array_abstract_interface())
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
    printf("[9/10] test_dynamic_array_edge_cases... ");
    if (test_dynamic_array_edge_cases())
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
    printf("[10/10] test_dynamic_array_performance... ");
    if (test_dynamic_array_performance())
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
        printf("\n所有测试通过！动态数组模块功能正常。\n");
        return 0;
    }
    else
    {
        printf("通过率: %.2f%%\n", (float)passed / (passed + failed) * 100.0f);
        printf("\n有测试失败，请检查相关功能。\n");
        return 1;
    }
}

/**
 * @brief 替代方案：使用run_all_dynamic_array_tests函数
 * 
 * 也可以使用test_dynamic_array.c中提供的run_all_dynamic_array_tests函数，
 * 该函数已经实现了测试运行和统计功能。
 * 
 * 示例代码：
 * 
 * int main(void)
 * {
 *     return run_all_dynamic_array_tests();
 * }
 * 
 * 当前实现提供了更详细的测试输出，便于调试和问题定位。
 */
