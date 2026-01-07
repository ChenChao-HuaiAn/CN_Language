/**
 * @file test_stack_runner.c
 * @brief 栈模块测试运行器
 * 
 * 提供栈模块的测试运行器，用于集成到总测试框架中。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_stack.h"

/**
 * @brief 运行栈模块的所有测试
 * 
 * 运行栈模块的所有子模块测试。
 * 
 * @return 所有测试通过返回true，否则返回false
 */
bool test_stack_all(void)
{
    printf("运行栈模块测试...\n");
    printf("==================\n\n");
    
    bool all_passed = true;
    
    // 运行核心模块测试
    printf("1. 栈核心模块测试:\n");
    if (test_stack_core_all()) {
        printf("   ✅ 通过\n");
    } else {
        printf("   ❌ 失败\n");
        all_passed = false;
    }
    printf("\n");
    
    // 运行迭代器模块测试
    printf("2. 栈迭代器模块测试:\n");
    if (test_stack_iterator_all()) {
        printf("   ✅ 通过\n");
    } else {
        printf("   ❌ 失败\n");
        all_passed = false;
    }
    printf("\n");
    
    // 运行工具模块测试
    printf("3. 栈工具模块测试:\n");
    if (test_stack_utils_all()) {
        printf("   ✅ 通过\n");
    } else {
        printf("   ❌ 失败\n");
        all_passed = false;
    }
    printf("\n");
    
    // 打印总结
    printf("栈模块测试总结:\n");
    printf("================\n");
    if (all_passed) {
        printf("✅ 所有测试通过！\n");
    } else {
        printf("❌ 有测试失败\n");
    }
    printf("\n");
    
    return all_passed;
}
