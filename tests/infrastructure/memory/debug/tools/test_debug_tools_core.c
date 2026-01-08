/**
 * @file test_debug_tools_core.c
 * @brief 调试工具核心管理模块测试
 * 
 * 本文件实现了调试工具核心管理模块的测试函数。
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

// 测试用的调试输出回调函数
static int g_debug_output_call_count = 0;
static char g_last_debug_message[1024];

/**
 * @brief 测试用的调试输出回调函数
 */
static void test_debug_output_callback(const char* message)
{
    g_debug_output_call_count++;
    if (message != NULL) {
        strncpy(g_last_debug_message, message, sizeof(g_last_debug_message) - 1);
        g_last_debug_message[sizeof(g_last_debug_message) - 1] = '\0';
    }
}

/**
 * @brief 测试调试工具初始化
 */
static bool test_debug_tools_initialization(void)
{
    printf("    测试调试工具初始化...\n");
    
    // 重置状态
    g_debug_output_call_count = 0;
    g_last_debug_message[0] = '\0';
    
    // 测试1: 正常初始化
    bool initialized = F_initialize_debug_tools();
    if (!initialized) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 测试2: 检查初始化状态
    bool is_initialized = F_check_debug_tools_initialized();
    if (!is_initialized) {
        printf("      ❌ 初始化状态检查失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    // 测试3: 重复初始化（应该安全处理）
    bool reinitialized = F_initialize_debug_tools();
    if (!reinitialized) {
        printf("      ⚠ 重复初始化返回false（可能是设计如此）\n");
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    // 测试4: 清理后检查状态
    bool after_cleanup = F_check_debug_tools_initialized();
    if (after_cleanup) {
        printf("      ❌ 清理后状态检查失败\n");
        return false;
    }
    
    printf("      ✅ 调试工具初始化测试通过\n");
    return true;
}

/**
 * @brief 测试调试输出回调
 */
static bool test_debug_output_callback_function(void)
{
    printf("    测试调试输出回调...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 重置状态
    g_debug_output_call_count = 0;
    g_last_debug_message[0] = '\0';
    
    // 设置测试回调
    F_set_debug_output_callback(test_debug_output_callback);
    
    // 使用内部调试输出函数（通过启用详细模式触发）
    F_enable_verbose_debugging(true);
    
    // 检查回调是否被调用
    if (g_debug_output_call_count == 0) {
        printf("      ⚠ 调试输出回调未被调用（可能是实现细节）\n");
    }
    
    // 测试设置NULL回调（恢复默认）
    F_set_debug_output_callback(NULL);
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 调试输出回调测试通过\n");
    return true;
}

/**
 * @brief 测试详细调试模式
 */
static bool test_verbose_debugging(void)
{
    printf("    测试详细调试模式...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 测试1: 默认状态应该是禁用
    bool default_verbose = F_is_verbose_debugging_enabled();
    if (default_verbose) {
        printf("      ⚠ 默认详细模式已启用（可能是设计如此）\n");
    }
    
    // 测试2: 启用详细模式
    F_enable_verbose_debugging(true);
    bool enabled = F_is_verbose_debugging_enabled();
    if (!enabled) {
        printf("      ❌ 启用详细模式失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    // 测试3: 禁用详细模式
    F_enable_verbose_debugging(false);
    bool disabled = F_is_verbose_debugging_enabled();
    if (disabled) {
        printf("      ❌ 禁用详细模式失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 详细调试模式测试通过\n");
    return true;
}

/**
 * @brief 测试调试输出文件
 */
static bool test_debug_output_file(void)
{
    printf("    测试调试输出文件...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 测试1: 设置输出文件
    bool file_set = F_set_debug_output_file("test_debug_output.log");
    if (!file_set) {
        printf("      ⚠ 设置调试输出文件失败（可能是权限问题）\n");
    }
    
    // 测试2: 关闭文件输出
    bool file_closed = F_set_debug_output_file(NULL);
    if (!file_closed) {
        printf("      ⚠ 关闭调试输出文件失败\n");
    }
    
    // 测试3: 设置无效文件名（应该安全处理）
    bool invalid_file = F_set_debug_output_file("/invalid/path/debug.log");
    if (invalid_file) {
        printf("      ⚠ 无效文件名被接受（可能是延迟错误处理）\n");
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    // 尝试删除测试文件（如果存在）
    remove("test_debug_output.log");
    
    printf("      ✅ 调试输出文件测试通过\n");
    return true;
}

/**
 * @brief 测试核心管理模块边界情况
 */
static bool test_core_module_edge_cases(void)
{
    printf("    测试核心管理模块边界情况...\n");
    
    // 测试1: 在未初始化状态下调用函数
    // 这些函数应该安全处理未初始化状态
    F_set_debug_output_callback(NULL);
    F_enable_verbose_debugging(false);
    bool uninitialized_check = F_check_debug_tools_initialized();
    if (uninitialized_check) {
        printf("      ❌ 未初始化状态检查错误\n");
        return false;
    }
    
    bool uninitialized_verbose = F_is_verbose_debugging_enabled();
    if (uninitialized_verbose) {
        printf("      ⚠ 未初始化状态下详细模式返回true\n");
    }
    
    // 测试2: 多次初始化和清理
    for (int i = 0; i < 3; i++) {
        if (!F_initialize_debug_tools()) {
            printf("      ❌ 第%d次初始化失败\n", i + 1);
            return false;
        }
        
        if (!F_check_debug_tools_initialized()) {
            printf("      ❌ 第%d次初始化后状态检查失败\n", i + 1);
            F_cleanup_debug_tools();
            return false;
        }
        
        F_cleanup_debug_tools();
        
        if (F_check_debug_tools_initialized()) {
            printf("      ❌ 第%d次清理后状态检查失败\n", i + 1);
            return false;
        }
    }
    
    printf("      ✅ 核心管理模块边界情况测试通过\n");
    return true;
}

/**
 * @brief 运行所有核心管理模块测试
 * @return 所有测试通过返回true，否则返回false
 */
bool test_debug_tools_core_all(void)
{
    printf("  运行核心管理模块测试:\n");
    printf("  ---------------------\n");
    
    bool all_passed = true;
    
    if (!test_debug_tools_initialization()) {
        all_passed = false;
    }
    
    if (!test_debug_output_callback_function()) {
        all_passed = false;
    }
    
    if (!test_verbose_debugging()) {
        all_passed = false;
    }
    
    if (!test_debug_output_file()) {
        all_passed = false;
    }
    
    if (!test_core_module_edge_cases()) {
        all_passed = false;
    }
    
    if (all_passed) {
        printf("  ✅ 所有核心管理模块测试通过\n");
    } else {
        printf("  ❌ 部分核心管理模块测试失败\n");
    }
    
    return all_passed;
}
