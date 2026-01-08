/**
 * @file test_runner_main.c
 * @brief CN_Language项目总测试运行器
 * 
 * 这是一个模块化的测试框架，可以灵活选择要测试的模块。
 * 支持命令行参数选择特定模块或运行所有测试。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

/* 测试模块定义 */
typedef struct {
    const char* module_name;      // 模块名称
    const char* module_path;      // 模块路径
    bool (*run_tests)(void);      // 运行测试的函数指针
    bool enabled;                 // 是否启用
} TestModule;

/* 测试结果统计 */
typedef struct {
    int total_modules;           // 总模块数
    int modules_passed;          // 通过的模块数
    int modules_failed;          // 失败的模块数
    int total_tests;             // 总测试数（如果支持）
    int tests_passed;            // 通过的测试数
    int tests_failed;            // 失败的测试数
    double total_time;           // 总运行时间（秒）
} TestStatistics;

/* 函数声明 */
void print_usage(const char* program_name);
void print_banner(void);
void print_statistics(const TestStatistics* stats);
bool parse_arguments(int argc, char* argv[], TestModule* modules, int module_count);
void run_test_module(TestModule* module, TestStatistics* stats);

/* 外部测试函数声明（各模块提供） */
// 基础设施层 - 容器模块
// extern bool test_dynamic_array_all(void);
// extern bool test_hash_table_all(void);
// extern bool test_linked_list_all(void);
extern bool test_queue_all(void);  // 队列模块
extern bool test_stack_all(void);  // 栈模块
extern bool test_string_all(void); // 字符串模块

// 基础设施层 - 内存模块
extern bool test_allocators_all(void); // 内存分配器模块
extern bool test_context_all(void);    // 内存上下文模块

// 核心层 - 编译器模块
// extern bool test_lexer_all(void);
// extern bool test_parser_all(void);
// extern bool test_ast_all(void);

// 应用层 - 工具模块
// extern bool test_cli_all(void);
// extern bool test_repl_all(void);

/**
 * @brief 主函数
 */
int main(int argc, char* argv[])
{
    print_banner();
    
    /* 初始化测试模块列表 */
    TestModule modules[] = {
        // 基础设施层 - 容器模块
        // {"dynamic_array", "infrastructure/containers/array", test_dynamic_array_all, true},
        // {"hash_table",    "infrastructure/containers/hash",  test_hash_table_all,    true},
        // {"linked_list",   "infrastructure/containers/list",  test_linked_list_all,   true},
        {"queue",         "infrastructure/containers/queue", test_queue_all,         true},
        {"stack",         "infrastructure/containers/stack", test_stack_all,         true},
        {"string",        "infrastructure/containers/string", test_string_all,       true},
        
        // 基础设施层 - 内存模块
        {"allocators",    "infrastructure/memory/allocators", test_allocators_all,   true},
        {"context",       "infrastructure/memory/context",    test_context_all,      true},
        
        // 核心层 - 编译器模块
        // {"lexer",         "core/lexer",                     test_lexer_all,         false},
        // {"parser",        "core/parser",                    test_parser_all,        false},
        // {"ast",           "core/ast",                       test_ast_all,           false},
        
        // 应用层 - 工具模块
        // {"cli",           "application/cli",                test_cli_all,           false},
        // {"repl",          "application/repl",               test_repl_all,          false},
    };
    
    int module_count = sizeof(modules) / sizeof(modules[0]);
    
    /* 解析命令行参数 */
    if (!parse_arguments(argc, argv, modules, module_count)) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    /* 初始化统计信息 */
    TestStatistics stats = {0};
    stats.total_modules = module_count;
    
    /* 运行测试 */
    printf("\n开始运行测试...\n");
    printf("================\n\n");
    
    clock_t start_time = clock();
    
    for (int i = 0; i < module_count; i++) {
        if (modules[i].enabled) {
            run_test_module(&modules[i], &stats);
        }
    }
    
    clock_t end_time = clock();
    stats.total_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    /* 打印统计信息 */
    printf("\n测试完成！\n");
    printf("==========\n\n");
    print_statistics(&stats);
    
    return (stats.modules_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/**
 * @brief 打印使用说明
 */
void print_usage(const char* program_name)
{
    printf("使用说明: %s [选项] [模块名...]\n\n", program_name);
    printf("选项:\n");
    printf("  --all             运行所有模块的测试\n");
    printf("  --list            列出所有可用的测试模块\n");
    printf("  --help            显示此帮助信息\n");
    printf("\n模块名:\n");
    printf("  可以指定一个或多个要测试的模块名\n");
    printf("  例如: %s dynamic_array hash_table\n", program_name);
    printf("\n如果不指定任何参数，默认运行所有启用的模块测试。\n");
}

/**
 * @brief 打印横幅
 */
void print_banner(void)
{
    printf("\n");
    printf("========================================\n");
    printf("    CN_Language项目 - 模块化测试框架\n");
    printf("========================================\n");
    printf("\n");
}

/**
 * @brief 打印统计信息
 */
void print_statistics(const TestStatistics* stats)
{
    printf("测试统计:\n");
    printf("  ├─ 总模块数:      %d\n", stats->total_modules);
    printf("  ├─ 测试模块数:    %d\n", stats->modules_passed + stats->modules_failed);
    printf("  ├─ 模块通过:      %d\n", stats->modules_passed);
    printf("  ├─ 模块失败:      %d\n", stats->modules_failed);
    
    if (stats->total_tests > 0) {
        printf("  ├─ 总测试数:      %d\n", stats->total_tests);
        printf("  ├─ 测试通过:      %d\n", stats->tests_passed);
        printf("  ├─ 测试失败:      %d\n", stats->tests_failed);
        double pass_rate = (double)stats->tests_passed / stats->total_tests * 100;
        printf("  ├─ 通过率:        %.1f%%\n", pass_rate);
    }
    
    printf("  └─ 总运行时间:    %.3f 秒\n", stats->total_time);
    printf("\n");
    
    if (stats->modules_failed == 0) {
        printf("✅ 所有测试通过！\n");
    } else {
        printf("❌ 有 %d 个模块测试失败\n", stats->modules_failed);
    }
    printf("\n");
}

/**
 * @brief 解析命令行参数
 */
bool parse_arguments(int argc, char* argv[], TestModule* modules, int module_count)
{
    // 如果没有参数，使用默认设置
    if (argc == 1) {
        return true;
    }
    
    // 处理选项
    bool run_all = false;
    bool list_modules = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--all") == 0) {
            run_all = true;
        } else if (strcmp(argv[i], "--list") == 0) {
            list_modules = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            return false; // 显示帮助信息
        }
    }
    
    // 列出模块
    if (list_modules) {
        printf("可用的测试模块:\n");
        printf("================\n\n");
        
        for (int i = 0; i < module_count; i++) {
            printf("  %-20s - %s\n", 
                   modules[i].module_name, 
                   modules[i].module_path);
        }
        printf("\n");
        exit(EXIT_SUCCESS);
    }
    
    // 如果指定了--all，启用所有模块
    if (run_all) {
        for (int i = 0; i < module_count; i++) {
            modules[i].enabled = true;
        }
        return true;
    }
    
    // 否则，根据模块名启用特定模块
    // 首先禁用所有模块
    for (int i = 0; i < module_count; i++) {
        modules[i].enabled = false;
    }
    
    // 然后启用指定的模块
    bool found_valid_module = false;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            continue; // 跳过选项
        }
        
        bool found = false;
        for (int j = 0; j < module_count; j++) {
            if (strcmp(argv[i], modules[j].module_name) == 0) {
                modules[j].enabled = true;
                found = true;
                found_valid_module = true;
                break;
            }
        }
        
        if (!found) {
            printf("错误: 未知的模块名 '%s'\n", argv[i]);
            return false;
        }
    }
    
    // 如果没有指定任何模块，启用所有默认启用的模块
    if (!found_valid_module) {
        // 保持默认的enabled设置
    }
    
    return true;
}

/**
 * @brief 运行单个测试模块
 */
void run_test_module(TestModule* module, TestStatistics* stats)
{
    printf("测试模块: %s\n", module->module_name);
    printf("路径: %s\n", module->module_path);
    printf("状态: ");
    
    clock_t module_start = clock();
    bool result = module->run_tests();
    clock_t module_end = clock();
    
    double module_time = (double)(module_end - module_start) / CLOCKS_PER_SEC;
    
    if (result) {
        printf("✅ 通过 (%.3f 秒)\n", module_time);
        stats->modules_passed++;
    } else {
        printf("❌ 失败 (%.3f 秒)\n", module_time);
        stats->modules_failed++;
    }
    
    printf("\n");
}
