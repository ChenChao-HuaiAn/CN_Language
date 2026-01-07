/**
 * @file test_allocators_runner.c
 * @brief 内存分配器模块测试运行器
 * 
 * 内存分配器模块的测试运行器，整合所有子模块测试，
 * 提供统一的测试接口供总测试运行器调用。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_allocators.h"
#include "../../../../src/infrastructure/memory/CN_memory_interface.h"
#include <stdio.h>
#include <string.h>

/* 外部测试函数声明 */
extern bool test_system_allocator_all(void);
extern bool test_debug_allocator_all(void);
extern bool test_pool_allocator_all(void);
extern bool test_region_allocator_all(void);
extern bool test_allocator_factory_all(void);
extern bool test_allocator_config_all(void);

/**
 * @brief 运行内存分配器模块的所有测试
 * 
 * 此函数被总测试运行器调用，运行内存分配器模块的所有子模块测试。
 * 返回true表示所有测试通过，false表示有测试失败。
 * 
 * @return bool 测试结果
 */
bool test_allocators_all(void)
{
    printf("\n");
    printf("========================================\n");
    printf("    内存分配器模块 - 完整测试套件\n");
    printf("========================================\n");
    printf("\n");
    
    bool all_passed = true;
    int modules_passed = 0;
    int modules_total = 6;
    
    /* 运行系统分配器测试 */
    printf("1. 系统分配器测试\n");
    printf("------------------\n");
    bool system_passed = test_system_allocator_all();
    all_passed = system_passed && all_passed;
    modules_passed += system_passed ? 1 : 0;
    
    /* 运行调试分配器测试 */
    printf("\n2. 调试分配器测试\n");
    printf("------------------\n");
    bool debug_passed = test_debug_allocator_all();
    all_passed = debug_passed && all_passed;
    modules_passed += debug_passed ? 1 : 0;
    
    /* 运行对象池分配器测试 */
    printf("\n3. 对象池分配器测试\n");
    printf("--------------------\n");
    bool pool_passed = test_pool_allocator_all();
    all_passed = pool_passed && all_passed;
    modules_passed += pool_passed ? 1 : 0;
    
    /* 运行区域分配器测试 */
    printf("\n4. 区域分配器测试\n");
    printf("------------------\n");
    bool region_passed = test_region_allocator_all();
    all_passed = region_passed && all_passed;
    modules_passed += region_passed ? 1 : 0;
    
    /* 运行分配器配置测试 */
    printf("\n5. 分配器配置测试\n");
    printf("------------------\n");
    bool config_passed = test_allocator_config_all();
    all_passed = config_passed && all_passed;
    modules_passed += config_passed ? 1 : 0;
    
    /* 运行分配器工厂测试 */
    printf("\n6. 分配器工厂测试\n");
    printf("------------------\n");
    bool factory_passed = test_allocator_factory_all();
    all_passed = factory_passed && all_passed;
    modules_passed += factory_passed ? 1 : 0;
    
    /* 打印总体结果 */
    printf("\n========================================\n");
    printf("内存分配器模块测试总结\n");
    printf("========================================\n");
    printf("总模块数: %d\n", modules_total);
    printf("通过模块: %d\n", modules_passed);
    printf("失败模块: %d\n", modules_total - modules_passed);
    printf("通过率: %.1f%%\n", (float)modules_passed / modules_total * 100);
    printf("\n");
    
    if (all_passed) {
        printf("✅ 内存分配器模块所有测试通过！\n");
    } else {
        printf("❌ 内存分配器模块有测试失败\n");
    }
    printf("\n");
    
    return all_passed;
}

/**
 * @brief 运行内存分配器模块的特定子模块测试
 * 
 * 根据模块名运行特定的子模块测试。
 * 用于命令行参数选择测试。
 * 
 * @param module_name 模块名
 * @return bool 测试结果
 */
bool test_allocators_module(const char* module_name)
{
    if (module_name == NULL) {
        return false;
    }
    
    printf("运行内存分配器模块测试: %s\n", module_name);
    printf("================================\n\n");
    
    if (strcmp(module_name, "system") == 0) {
        return test_system_allocator_all();
    } else if (strcmp(module_name, "debug") == 0) {
        return test_debug_allocator_all();
    } else if (strcmp(module_name, "pool") == 0) {
        return test_pool_allocator_all();
    } else if (strcmp(module_name, "region") == 0) {
        return test_region_allocator_all();
    } else if (strcmp(module_name, "config") == 0) {
        return test_allocator_config_all();
    } else if (strcmp(module_name, "factory") == 0) {
        return test_allocator_factory_all();
    } else if (strcmp(module_name, "all") == 0) {
        return test_allocators_all();
    } else {
        printf("错误: 未知的内存分配器模块名 '%s'\n", module_name);
        printf("可用的模块: system, debug, pool, region, config, factory, all\n");
        return false;
    }
}

/**
 * @brief 主函数（独立测试运行器）
 * 
 * 当此文件被单独编译时，可以作为独立的测试运行器使用。
 * 支持命令行参数选择测试模块。
 */
#ifdef TEST_ALLOCATORS_STANDALONE
int main(int argc, char* argv[])
{
    printf("\n");
    printf("========================================\n");
    printf("    内存分配器模块 - 独立测试运行器\n");
    printf("========================================\n");
    printf("\n");
    
    if (argc == 1) {
        // 没有参数，运行所有测试
        return test_allocators_all() ? 0 : 1;
    } else if (argc == 2) {
        // 一个参数，运行指定模块
        bool result = test_allocators_module(argv[1]);
        return result ? 0 : 1;
    } else {
        printf("使用说明: %s [模块名]\n", argv[0]);
        printf("\n模块名:\n");
        printf("  system       系统分配器测试\n");
        printf("  debug        调试分配器测试\n");
        printf("  pool         对象池分配器测试\n");
        printf("  region       区域分配器测试\n");
        printf("  config       分配器配置测试\n");
        printf("  factory      分配器工厂测试\n");
        printf("  all          所有模块测试（默认）\n");
        printf("\n示例: %s system\n", argv[0]);
        return 1;
    }
}
#endif
