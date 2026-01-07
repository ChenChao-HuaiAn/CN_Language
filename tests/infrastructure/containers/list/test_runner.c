/**
 * @file test_runner.c
 * @brief 链表模块测试运行器
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * 
 * @copyright MIT许可证 - 详见项目根目录LICENSE文件
 * 
 * 测试运行器文件，包含main()函数，用于运行链表模块的所有测试。
 * 遵循项目架构规范，测试文件和main()函数分离，便于集成到更大的测试框架。
 */

#include <stdio.h>
#include <stdlib.h>
#include "test_linked_list.h"

/**
 * @brief 主函数 - 运行所有链表测试
 * 
 * @return int 程序退出码：0表示所有测试通过，非0表示有测试失败
 * 
 * @note 函数会运行所有4个测试函数，并输出详细的测试结果统计。
 *       遵循模块化测试设计原则，测试文件和main()函数分离。
 */
int main(void)
{
    printf("=== 链表模块测试套件 ===\n\n");
    
    // 运行所有测试
    int passed = 0;
    int failed = 0;
    
    // 测试1: 基础功能测试
    printf("[1/4] test_linked_list_basic_operations... ");
    if (test_linked_list_basic_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 测试2: 迭代器功能测试
    printf("[2/4] test_linked_list_iterator_operations... ");
    if (test_linked_list_iterator_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 测试3: 排序功能测试
    printf("[3/4] test_linked_list_sort_operations... ");
    if (test_linked_list_sort_operations())
    {
        printf("通过\n");
        passed++;
    }
    else
    {
        printf("失败\n");
        failed++;
    }
    
    // 测试4: 边界条件测试
    printf("[4/4] test_linked_list_edge_cases... ");
    if (test_linked_list_edge_cases())
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
        printf("\n所有测试通过！链表模块功能正常。\n");
        return 0;
    }
    else
    {
        printf("通过率: %.2f%%\n", (float)passed / (passed + failed) * 100.0f);
        printf("\n有测试失败，请检查相关功能。\n");
        return 1;
    }
}
