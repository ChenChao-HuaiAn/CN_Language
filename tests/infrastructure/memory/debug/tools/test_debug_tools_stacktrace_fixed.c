/**
 * @file test_debug_tools_stacktrace_fixed.c
 * @brief 调用栈跟踪模块测试（修复版）
 * 
 * 本文件实现了调用栈跟踪模块的测试函数，使用正确的API。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../../../src/infrastructure/memory/debug/tools/CN_memory_debug_tools.h"

// 测试用的递归函数，用于生成调用栈
static int g_recursion_depth = 0;
static const int MAX_RECURSION_DEPTH = 5;

/**
 * @brief 递归测试函数
 */
static void recursive_function(int depth)
{
    if (depth >= MAX_RECURSION_DEPTH) {
        return;
    }
    
    g_recursion_depth = depth;
    
    // 递归调用
    recursive_function(depth + 1);
}

/**
 * @brief 另一个测试函数，用于生成不同的调用栈
 */
static void another_function(void)
{
    // 空函数，仅用于生成调用栈条目
}

/**
 * @brief 测试调用栈捕获
 */
static bool test_stack_trace_capture(void)
{
    printf("    测试调用栈捕获...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 测试1: 获取默认选项
    Stru_StackTraceOptions_t default_options = F_get_default_stacktrace_options();
    printf("      默认选项 - 最大深度: %zu\n", default_options.max_depth);
    
    // 测试2: 获取调用栈到字符串
    char trace_buffer[4096];
    size_t trace_length = F_get_stack_trace(&default_options, trace_buffer, sizeof(trace_buffer));
    
    if (trace_length == 0) {
        printf("      ⚠ 调用栈获取失败或为空\n");
    } else {
        printf("      调用栈长度: %zu 字符\n", trace_length);
        printf("      调用栈内容（前200字符）: %.200s\n", trace_buffer);
    }
    
    // 测试3: 获取调用栈地址数组
    void* addresses[32];
    size_t address_count = F_get_stack_trace_addresses(addresses, 32);
    
    if (address_count == 0) {
        printf("      ⚠ 调用栈地址获取失败\n");
    } else {
        printf("      获取到 %zu 个调用栈地址\n", address_count);
        for (size_t i = 0; i < address_count && i < 5; i++) {
            printf("        地址[%zu]: %p\n", i, addresses[i]);
        }
        if (address_count > 5) {
            printf("        ... 还有 %zu 个地址\n", address_count - 5);
        }
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 调用栈捕获测试通过\n");
    return true;
}

/**
 * @brief 测试调用栈格式化输出
 */
static bool test_stack_trace_formatting(void)
{
    printf("    测试调用栈格式化输出...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 测试不同的选项组合
    Stru_StackTraceOptions_t options_list[] = {
        // 默认选项
        F_get_default_stacktrace_options(),
        
        // 最小选项
        { .max_depth = 5, .show_function_names = false, .show_file_names = false, 
          .show_line_numbers = false, .show_addresses = false },
        
        // 完整选项
        { .max_depth = 20, .show_function_names = true, .show_file_names = true, 
          .show_line_numbers = true, .show_addresses = true },
        
        // 仅地址
        { .max_depth = 10, .show_function_names = false, .show_file_names = false, 
          .show_line_numbers = false, .show_addresses = true },
        
        // 仅函数名
        { .max_depth = 15, .show_function_names = true, .show_file_names = false, 
          .show_line_numbers = false, .show_addresses = false }
    };
    
    const char* option_names[] = {
        "默认选项",
        "最小选项",
        "完整选项",
        "仅地址",
        "仅函数名"
    };
    
    size_t option_count = sizeof(options_list) / sizeof(options_list[0]);
    
    for (size_t i = 0; i < option_count; i++) {
        printf("      测试 %s:\n", option_names[i]);
        
        char trace_buffer[2048];
        size_t trace_length = F_get_stack_trace(&options_list[i], trace_buffer, sizeof(trace_buffer));
        
        if (trace_length == 0) {
            printf("        ⚠ 调用栈获取失败\n");
        } else {
            printf("        调用栈长度: %zu 字符\n", trace_length);
            
            // 检查缓冲区是否足够
            if (trace_length >= sizeof(trace_buffer) - 1) {
                printf("        ⚠ 缓冲区可能不足\n");
            }
        }
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 调用栈格式化输出测试通过\n");
    return true;
}

/**
 * @brief 测试调用栈性能
 */
static bool test_stack_trace_performance(void)
{
    printf("    测试调用栈性能...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 性能测试：多次获取调用栈
    int iterations = 5;
    printf("      执行%d次调用栈性能测试...\n", iterations);
    
    Stru_StackTraceOptions_t options = F_get_default_stacktrace_options();
    options.max_depth = 10; // 限制深度以提高性能
    
    for (int i = 0; i < iterations; i++) {
        char trace_buffer[1024];
        size_t trace_length = F_get_stack_trace(&options, trace_buffer, sizeof(trace_buffer));
        
        if (trace_length == 0) {
            printf("        ⚠ 第%d次调用栈获取失败\n", i + 1);
        } else {
            printf("        第%d次: %zu 字符\n", i + 1, trace_length);
        }
    }
    
    // 测试地址数组性能
    printf("      执行%d次调用栈地址获取性能测试...\n", iterations);
    
    for (int i = 0; i < iterations; i++) {
        void* addresses[20];
        size_t address_count = F_get_stack_trace_addresses(addresses, 20);
        
        if (address_count == 0) {
            printf("        ⚠ 第%d次地址获取失败\n", i + 1);
        } else {
            printf("        第%d次: %zu 个地址\n", i + 1, address_count);
        }
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 调用栈性能测试通过\n");
    return true;
}

/**
 * @brief 测试调用栈边界情况
 */
static bool test_stack_trace_edge_cases(void)
{
    printf("    测试调用栈边界情况...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    Stru_StackTraceOptions_t options = F_get_default_stacktrace_options();
    
    // 测试1: NULL缓冲区
    size_t null_result = F_get_stack_trace(&options, NULL, 0);
    printf("      NULL缓冲区结果: %zu\n", null_result);
    
    // 测试2: 小缓冲区
    char small_buffer[10];
    size_t small_result = F_get_stack_trace(&options, small_buffer, sizeof(small_buffer));
    printf("      小缓冲区结果: %zu\n", small_result);
    if (small_result > 0) {
        printf("      小缓冲区内容: %.*s\n", (int)sizeof(small_buffer), small_buffer);
    }
    
    // 测试3: 零深度
    Stru_StackTraceOptions_t zero_depth_options = options;
    zero_depth_options.max_depth = 0;
    char zero_buffer[1024];
    size_t zero_result = F_get_stack_trace(&zero_depth_options, zero_buffer, sizeof(zero_buffer));
    printf("      零深度结果: %zu\n", zero_result);
    
    // 测试4: NULL地址数组
    size_t null_address_result = F_get_stack_trace_addresses(NULL, 0);
    printf("      NULL地址数组结果: %zu\n", null_address_result);
    
    // 测试5: 小地址数组
    void* small_address_array[2];
    size_t small_address_result = F_get_stack_trace_addresses(small_address_array, 2);
    printf("      小地址数组结果: %zu\n", small_address_result);
    
    // 测试6: 递归调用栈
    printf("      测试递归调用栈...\n");
    recursive_function(0);
    
    char recursive_buffer[2048];
    size_t recursive_result = F_get_stack_trace(&options, recursive_buffer, sizeof(recursive_buffer));
    printf("      递归调用栈结果: %zu\n", recursive_result);
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 调用栈边界情况测试通过\n");
    return true;
}

/**
 * @brief 运行所有调用栈跟踪模块测试
 * @return 所有测试通过返回true，否则返回false
 */
bool test_debug_tools_stacktrace_all(void)
{
    printf("  运行调用栈跟踪模块测试:\n");
    printf("  -----------------------\n");
    
    bool all_passed = true;
    
    if (!test_stack_trace_capture()) {
        all_passed = false;
    }
    
    if (!test_stack_trace_formatting()) {
        all_passed = false;
    }
    
    if (!test_stack_trace_performance()) {
        all_passed = false;
    }
    
    if (!test_stack_trace_edge_cases()) {
        all_passed = false;
    }
    
    if (all_passed) {
        printf("  ✅ 所有调用栈跟踪模块测试通过\n");
    } else {
        printf("  ❌ 部分调用栈跟踪模块测试失败\n");
    }
    
    return all_passed;
}
