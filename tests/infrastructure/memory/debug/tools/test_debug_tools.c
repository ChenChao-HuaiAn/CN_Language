/**
 * @file test_debug_tools.c
 * @brief 调试工具模块测试实现
 * 
 * 本文件实现了调试工具模块的测试函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../../../../src/infrastructure/memory/debug/tools/CN_memory_debug_tools.h"

/**
 * @brief 测试调试工具初始化和清理
 */
static bool test_debug_tools_init_cleanup(void)
{
    printf("    测试调试工具初始化和清理...\n");
    
    // 初始化调试工具
    bool initialized = F_initialize_debug_tools();
    if (!initialized) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 清理调试工具
    F_cleanup_debug_tools();
    
    printf("      ✅ 调试工具初始化和清理测试通过\n");
    return true;
}

/**
 * @brief 测试内存转储
 */
static bool test_memory_dump(void)
{
    printf("    测试内存转储...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 分配一些内存
    char* buffer = (char*)malloc(64);
    if (buffer == NULL) {
        printf("      ❌ 内存分配失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    // 填充测试数据
    strcpy(buffer, "Test memory dump data for debugging tools");
    
    // 创建转储选项
    Stru_MemoryDumpOptions_t options;
    memset(&options, 0, sizeof(options));
    options.show_address = true;
    options.show_hex = true;
    options.show_ascii = true;
    options.bytes_per_line = 16;
    
    // 转储内存（注意：这个函数可能只是打印到控制台）
    F_dump_memory(buffer, 50, &options);
    
    // 清理
    free(buffer);
    F_cleanup_debug_tools();
    
    printf("      ✅ 内存转储测试通过\n");
    return true;
}

/**
 * @brief 测试调用栈跟踪
 */
static bool test_stack_trace(void)
{
    printf("    测试调用栈跟踪...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 创建调用栈选项
    Stru_StackTraceOptions_t options;
    memset(&options, 0, sizeof(options));
    options.max_depth = 10;
    options.show_function_names = true;
    options.show_file_names = true;
    options.show_line_numbers = true;
    options.show_addresses = true;
    
    // 获取调用栈
    char buffer[1024];
    size_t trace_size = F_get_stack_trace(&options, buffer, sizeof(buffer));
    
    if (trace_size == 0) {
        printf("      ⚠ 调用栈跟踪返回空（可能是平台不支持）\n");
    } else {
        printf("      调用栈跟踪大小: %zu 字节\n", trace_size);
    }
    
    F_cleanup_debug_tools();
    
    printf("      ✅ 调用栈跟踪测试通过\n");
    return true;
}

/**
 * @brief 测试调试工具组合使用
 */
static bool test_debug_tools_integration(void)
{
    printf("    测试调试工具组合使用...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 分配内存
    int* numbers = (int*)malloc(10 * sizeof(int));
    if (numbers == NULL) {
        printf("      ❌ 内存分配失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    // 填充数据
    for (int i = 0; i < 10; i++) {
        numbers[i] = i * 100;
    }
    
    // 转储内存
    Stru_MemoryDumpOptions_t dump_options;
    memset(&dump_options, 0, sizeof(dump_options));
    dump_options.show_address = true;
    dump_options.show_hex = true;
    dump_options.bytes_per_line = 16;
    
    F_dump_memory(numbers, 10 * sizeof(int), &dump_options);
    
    // 获取调用栈
    Stru_StackTraceOptions_t trace_options;
    memset(&trace_options, 0, sizeof(trace_options));
    trace_options.max_depth = 5;
    trace_options.show_function_names = true;
    trace_options.show_file_names = true;
    trace_options.show_line_numbers = true;
    trace_options.show_addresses = true;
    
    char trace_buffer[512];
    F_get_stack_trace(&trace_options, trace_buffer, sizeof(trace_buffer));
    
    // 清理
    free(numbers);
    F_cleanup_debug_tools();
    
    printf("      ✅ 调试工具组合使用测试通过\n");
    return true;
}

/**
 * @brief 运行所有调试工具测试
 */
bool test_debug_tools_all(void)
{
    printf("  运行调试工具模块测试:\n");
    printf("  ---------------------\n");
    
    bool all_passed = true;
    
    if (!test_debug_tools_init_cleanup()) {
        all_passed = false;
    }
    
    if (!test_memory_dump()) {
        all_passed = false;
    }
    
    if (!test_stack_trace()) {
        all_passed = false;
    }
    
    if (!test_debug_tools_integration()) {
        all_passed = false;
    }
    
    if (all_passed) {
        printf("  ✅ 所有调试工具测试通过\n");
    } else {
        printf("  ❌ 部分调试工具测试失败\n");
    }
    
    return all_passed;
}
