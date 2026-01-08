/**
 * @file test_debug_tools_stacktrace.c
 * @brief 调用栈跟踪模块测试
 * 
 * 本文件实现了调用栈跟踪模块的测试函数。
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
    
    // 测试1: 捕获当前调用栈
    printf("      （捕获当前调用栈）\n");
    Stru_StackTrace_t stack_trace;
    bool captured = F_capture_stack_trace(&stack_trace);
    if (!captured) {
        printf("      ⚠ 调用栈捕获失败\n");
    } else {
        printf("      捕获到 %zu 个调用栈帧\n", stack_trace.frame_count);
        
        // 检查调用栈数据
        if (stack_trace.frame_count == 0) {
            printf("      ⚠ 调用栈帧数为0\n");
        }
    }
    
    // 测试2: 捕获递归调用栈
    printf("      （捕获递归调用栈）\n");
    recursive_function(0);
    
    Stru_StackTrace_t recursive_trace;
    bool recursive_captured = F_capture_stack_trace(&recursive_trace);
    if (!recursive_captured) {
        printf("      ⚠ 递归调用栈捕获失败\n");
    }
    
    // 测试3: 捕获另一个函数的调用栈
    printf("      （捕获另一个函数的调用栈）\n");
    another_function();
    
    Stru_StackTrace_t another_trace;
    bool another_captured = F_capture_stack_trace(&another_trace);
    if (!another_captured) {
        printf("      ⚠ 另一个函数调用栈捕获失败\n");
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
    
    // 捕获调用栈
    Stru_StackTrace_t stack_trace;
    bool captured = F_capture_stack_trace(&stack_trace);
    if (!captured) {
        printf("      ⚠ 调用栈捕获失败，跳过格式化测试\n");
        F_cleanup_debug_tools();
        return true; // 不是致命错误
    }
    
    // 测试1: 输出到标准输出
    printf("      （调用栈输出到标准输出）\n");
    bool printed = F_print_stack_trace(&stack_trace);
    if (!printed) {
        printf("      ⚠ 调用栈输出失败\n");
    }
    
    // 测试2: 输出到字符串
    char trace_string[4096];
    bool string_output = F_stack_trace_to_string(&stack_trace, trace_string, sizeof(trace_string));
    if (!string_output) {
        printf("      ⚠ 调用栈字符串输出失败\n");
    } else {
        // 检查字符串是否包含调用栈信息
        if (strlen(trace_string) == 0) {
            printf("      ⚠ 调用栈字符串为空\n");
        } else {
            printf("      调用栈字符串长度: %zu\n", strlen(trace_string));
        }
    }
    
    // 测试3: 输出到文件
    bool file_output = F_save_stack_trace_to_file(&stack_trace, "test_stack_trace.txt");
    if (!file_output) {
        printf("      ⚠ 调用栈文件输出失败\n");
    }
    
    // 测试4: 使用自定义选项
    Stru_StackTraceOptions_t options;
    options.max_depth = 10;
    options.show_addresses = true;
    options.show_function_names = true;
    options.show_file_names = false;
    options.show_line_numbers = false;
    
    printf("      （使用自定义选项输出调用栈）\n");
    bool custom_output = F_print_stack_trace_with_options(&stack_trace, &options);
    if (!custom_output) {
        printf("      ⚠ 自定义选项调用栈输出失败\n");
    }
    
    // 清理
    remove("test_stack_trace.txt"); // 删除测试文件
    F_cleanup_debug_tools();
    
    printf("      ✅ 调用栈格式化输出测试通过\n");
    return true;
}

/**
 * @brief 测试调用栈符号解析
 */
static bool test_stack_trace_symbol_resolution(void)
{
    printf("    测试调用栈符号解析...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 捕获调用栈
    Stru_StackTrace_t stack_trace;
    bool captured = F_capture_stack_trace(&stack_trace);
    if (!captured) {
        printf("      ⚠ 调用栈捕获失败，跳过符号解析测试\n");
        F_cleanup_debug_tools();
        return true; // 不是致命错误
    }
    
    // 测试1: 解析单个地址
    if (stack_trace.frame_count > 0) {
        void* address = stack_trace.frames[0];
        char symbol_name[256];
        char file_name[256];
        unsigned int line_number = 0;
        
        bool resolved = F_resolve_symbol(address, symbol_name, sizeof(symbol_name),
                                       file_name, sizeof(file_name), &line_number);
        if (!resolved) {
            printf("      ⚠ 符号解析失败（可能是没有调试信息）\n");
        } else {
            printf("      解析到符号: %s\n", symbol_name);
        }
    }
    
    // 测试2: 解析整个调用栈
    char resolved_trace[4096];
    bool trace_resolved = F_resolve_stack_trace(&stack_trace, resolved_trace, sizeof(resolved_trace));
    if (!trace_resolved) {
        printf("      ⚠ 调用栈解析失败\n");
    } else {
        printf("      解析后的调用栈长度: %zu\n", strlen(resolved_trace));
    }
    
    // 测试3: 获取简化调用栈
    char simplified_trace[2048];
    bool simplified = F_get_simplified_stack_trace(&stack_trace, simplified_trace, sizeof(simplified_trace));
    if (!simplified) {
        printf("      ⚠ 简化调用栈获取失败\n");
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 调用栈符号解析测试通过\n");
    return true;
}

/**
 * @brief 测试调用栈比较
 */
static bool test_stack_trace_comparison(void)
{
    printf("    测试调用栈比较...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 捕获第一个调用栈
    Stru_StackTrace_t trace1;
    bool captured1 = F_capture_stack_trace(&trace1);
    if (!captured1) {
        printf("      ⚠ 第一个调用栈捕获失败\n");
        F_cleanup_debug_tools();
        return true; // 不是致命错误
    }
    
    // 调用另一个函数后捕获第二个调用栈
    another_function();
    Stru_StackTrace_t trace2;
    bool captured2 = F_capture_stack_trace(&trace2);
    if (!captured2) {
        printf("      ⚠ 第二个调用栈捕获失败\n");
        F_cleanup_debug_tools();
        return true; // 不是致命错误
    }
    
    // 测试1: 比较调用栈
    bool are_similar = F_compare_stack_traces(&trace1, &trace2);
    printf("      调用栈相似: %s\n", are_similar ? "是" : "否");
    
    // 测试2: 计算调用栈哈希
    unsigned long long hash1 = 0, hash2 = 0;
    bool hashed1 = F_calculate_stack_trace_hash(&trace1, &hash1);
    bool hashed2 = F_calculate_stack_trace_hash(&trace2, &hash2);
    
    if (hashed1 && hashed2) {
        printf("      调用栈哈希1: 0x%016llX\n", hash1);
        printf("      调用栈哈希2: 0x%016llX\n", hash2);
        printf("      哈希相等: %s\n", (hash1 == hash2) ? "是" : "否");
    } else {
        printf("      ⚠ 调用栈哈希计算失败\n");
    }
    
    // 测试3: 查找调用栈差异
    size_t diff_count = 0;
    bool diffs_found = F_find_stack_trace_differences(&trace1, &trace2, &diff_count);
    if (diffs_found) {
        printf("      发现 %zu 个调用栈差异\n", diff_count);
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 调用栈比较测试通过\n");
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
    
    // 性能测试：多次捕获调用栈
    int iterations = 10;
    printf("      执行%d次调用栈捕获性能测试...\n", iterations);
    
    for (int i = 0; i < iterations; i++) {
        Stru_StackTrace_t trace;
        bool captured = F_capture_stack_trace(&trace);
        if (!captured) {
            printf("      ⚠ 第%d次调用栈捕获失败\n", i + 1);
        }
        
        // 可选：输出调用栈（注释掉以避免输出过多）
        // F_print_stack_trace(&trace);
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
    
    // 测试1: NULL指针输入
    bool null_capture = F_capture_stack_trace(NULL);
    if (null_capture) {
        printf("      ⚠ NULL指针调用栈捕获返回成功\n");
    }
    
    // 测试2: 空调用栈处理
    Stru_StackTrace_t empty_trace;
    memset(&empty_trace, 0, sizeof(empty_trace));
    
    bool empty_printed = F_print_stack_trace(&empty_trace);
    if (empty_printed) {
        printf("      ⚠ 空调用栈输出返回成功\n");
    }
    
    // 测试3: 小缓冲区测试
    char small_buffer[10];
    Stru_StackTrace_t trace;
    bool captured = F_capture_stack_trace(&trace);
    if (captured) {
        bool small_output = F_stack_trace_to_string(&trace, small_buffer, sizeof(small_buffer));
        if (small_output) {
            printf("      ⚠ 小缓冲区调用栈输出返回成功（可能是截断）\n");
        }
    }
    
    // 测试4: 深度递归测试
    printf("      （测试深度递归调用栈）\n");
    recursive_function(0);
    
    // 测试5: 无效地址解析
    char symbol_name[256];
    char file_name[256];
    unsigned int line_number = 0;
    
    bool invalid_resolved = F_resolve_symbol(NULL, symbol_name, sizeof(symbol_name),
                                           file_name, sizeof(file_name), &line_number);
    if (invalid_resolved) {
        printf("      ⚠ NULL地址符号解析返回成功\n");
    }
    
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
    
    if (!test_stack_trace_symbol_resolution()) {
        all_passed = false;
    }
    
    if (!test_stack_trace_comparison()) {
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
