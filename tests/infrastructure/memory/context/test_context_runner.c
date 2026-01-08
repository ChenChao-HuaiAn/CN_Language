/**
 * @file test_context_runner.c
 * @brief 内存上下文模块测试运行器
 * 
 * 内存上下文模块的测试运行器，整合所有子模块测试，
 * 提供统一的测试接口供总测试运行器调用。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_context.h"
#include "../../../../src/infrastructure/memory/context/CN_memory_context.h"
#include <stdio.h>
#include <string.h>

/* 外部测试函数声明 */
extern bool test_context_management_all(void);
extern bool test_context_operations_all(void);
extern bool test_context_allocation_all(void);
extern bool test_context_statistics_all(void);

/**
 * @brief 运行内存上下文模块的所有测试
 * 
 * 此函数被总测试运行器调用，运行内存上下文模块的所有子模块测试。
 * 返回true表示所有测试通过，false表示有测试失败。
 * 
 * @return bool 测试结果
 */
bool test_context_all(void)
{
    printf("\n");
    printf("========================================\n");
    printf("    内存上下文模块 - 完整测试套件\n");
    printf("========================================\n");
    printf("\n");
    
    bool all_passed = true;
    int modules_passed = 0;
    int modules_total = 4;
    
    /* 运行上下文管理测试 */
    printf("1. 上下文管理测试\n");
    printf("------------------\n");
    bool management_passed = test_context_management_all();
    all_passed = management_passed && all_passed;
    modules_passed += management_passed ? 1 : 0;
    
    /* 运行上下文操作测试 */
    printf("\n2. 上下文操作测试\n");
    printf("------------------\n");
    bool operations_passed = test_context_operations_all();
    all_passed = operations_passed && all_passed;
    modules_passed += operations_passed ? 1 : 0;
    
    /* 运行内存分配测试 */
    printf("\n3. 内存分配测试\n");
    printf("----------------\n");
    bool allocation_passed = test_context_allocation_all();
    all_passed = allocation_passed && all_passed;
    modules_passed += allocation_passed ? 1 : 0;
    
    /* 运行内存统计测试 */
    printf("\n4. 内存统计测试\n");
    printf("----------------\n");
    bool statistics_passed = test_context_statistics_all();
    all_passed = statistics_passed && all_passed;
    modules_passed += statistics_passed ? 1 : 0;
    
    /* 打印总体结果 */
    printf("\n========================================\n");
    printf("内存上下文模块测试总结\n");
    printf("========================================\n");
    printf("总模块数: %d\n", modules_total);
    printf("通过模块: %d\n", modules_passed);
    printf("失败模块: %d\n", modules_total - modules_passed);
    printf("通过率: %.1f%%\n", (float)modules_passed / modules_total * 100);
    printf("\n");
    
    if (all_passed) {
        printf("✅ 内存上下文模块所有测试通过！\n");
    } else {
        printf("❌ 内存上下文模块有测试失败\n");
    }
    printf("\n");
    
    return all_passed;
}

/**
 * @brief 运行内存上下文模块的特定子模块测试
 * 
 * 根据模块名运行特定的子模块测试。
 * 用于命令行参数选择测试。
 * 
 * @param module_name 模块名
 * @return bool 测试结果
 */
bool test_context_module(const char* module_name)
{
    if (module_name == NULL) {
        return false;
    }
    
    printf("运行内存上下文模块测试: %s\n", module_name);
    printf("================================\n\n");
    
    if (strcmp(module_name, "management") == 0) {
        return test_context_management_all();
    } else if (strcmp(module_name, "operations") == 0) {
        return test_context_operations_all();
    } else if (strcmp(module_name, "allocation") == 0) {
        return test_context_allocation_all();
    } else if (strcmp(module_name, "statistics") == 0) {
        return test_context_statistics_all();
    } else if (strcmp(module_name, "all") == 0) {
        return test_context_all();
    } else {
        printf("错误: 未知的内存上下文模块名 '%s'\n", module_name);
        printf("可用的模块: management, operations, allocation, statistics, all\n");
        return false;
    }
}

/**
 * @brief 主函数（独立测试运行器）
 * 
 * 当此文件被单独编译时，可以作为独立的测试运行器使用。
 * 支持命令行参数选择测试模块。
 */
#ifdef TEST_CONTEXT_STANDALONE
int main(int argc, char* argv[])
{
    printf("\n");
    printf("========================================\n");
    printf("    内存上下文模块 - 独立测试运行器\n");
    printf("========================================\n");
    printf("\n");
    
    if (argc == 1) {
        // 没有参数，运行所有测试
        return test_context_all() ? 0 : 1;
    } else if (argc == 2) {
        // 一个参数，运行指定模块
        bool result = test_context_module(argv[1]);
        return result ? 0 : 1;
    } else {
        printf("使用说明: %s [模块名]\n", argv[0]);
        printf("\n模块名:\n");
        printf("  management    上下文管理测试\n");
        printf("  operations    上下文操作测试\n");
        printf("  allocation    内存分配测试\n");
        printf("  statistics    内存统计测试\n");
        printf("  all           所有模块测试（默认）\n");
        printf("\n示例: %s management\n", argv[0]);
        return 1;
    }
}
#endif
