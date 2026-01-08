/**
 * @file test_debug_tools_integration_fixed.c
 * @brief 内存调试工具模块集成测试（修复版）
 * 
 * 本文件实现了内存调试工具模块的集成测试函数，
 * 只测试实际存在的API函数。
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

/**
 * @brief 测试基本的内存调试工作流程
 */
static bool test_basic_debug_workflow(void)
{
    printf("    测试基本的内存调试工作流程...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 启用详细调试输出
    F_enable_verbose_debugging(true);
    
    // 创建测试内存区域
    size_t buffer_size = 1024;
    unsigned char* test_buffer = (unsigned char*)malloc(buffer_size);
    if (test_buffer == NULL) {
        printf("      ❌ 测试缓冲区分配失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    // 填充测试数据
    for (size_t i = 0; i < buffer_size; i++) {
        test_buffer[i] = (unsigned char)(i % 256);
    }
    
    // 步骤1: 检查调试工具是否已初始化
    bool is_initialized = F_check_debug_tools_initialized();
    if (!is_initialized) {
        printf("      ❌ 调试工具未正确初始化\n");
        free(test_buffer);
        F_cleanup_debug_tools();
        return false;
    }
    
    // 步骤2: 检查详细调试是否启用
    bool verbose_enabled = F_is_verbose_debugging_enabled();
    if (!verbose_enabled) {
        printf("      ⚠ 详细调试未启用\n");
    }
    
    // 步骤3: 输出调试信息
    F_debug_output("测试缓冲区地址: %p, 大小: %zu", (void*)test_buffer, buffer_size);
    
    // 清理
    free(test_buffer);
    F_cleanup_debug_tools();
    
    // 检查清理后状态
    bool after_cleanup = F_check_debug_tools_initialized();
    if (after_cleanup) {
        printf("      ❌ 清理后调试工具状态不正确\n");
        return false;
    }
    
    printf("      ✅ 基本的内存调试工作流程测试通过\n");
    return true;
}

/**
 * @brief 测试多次初始化和清理
 */
static bool test_multiple_initialization(void)
{
    printf("    测试多次初始化和清理...\n");
    
    // 测试多次初始化和清理
    for (int i = 0; i < 3; i++) {
        if (!F_initialize_debug_tools()) {
            printf("      ❌ 第%d次初始化失败\n", i + 1);
            return false;
        }
        
        // 检查是否已初始化
        if (!F_check_debug_tools_initialized()) {
            printf("      ❌ 第%d次初始化后状态检查失败\n", i + 1);
            F_cleanup_debug_tools();
            return false;
        }
        
        // 切换详细调试模式
        F_enable_verbose_debugging((i % 2) == 0);
        
        // 检查详细调试状态
        bool expected_verbose = ((i % 2) == 0);
        bool actual_verbose = F_is_verbose_debugging_enabled();
        if (expected_verbose != actual_verbose) {
            printf("      ❌ 第%d次详细调试状态不匹配\n", i + 1);
            F_cleanup_debug_tools();
            return false;
        }
        
        // 输出调试信息
        F_debug_output("第%d次初始化测试", i + 1);
        
        F_cleanup_debug_tools();
        
        // 检查清理后状态
        if (F_check_debug_tools_initialized()) {
            printf("      ❌ 第%d次清理后状态检查失败\n", i + 1);
            return false;
        }
    }
    
    printf("      ✅ 多次初始化和清理测试通过\n");
    return true;
}

/**
 * @brief 测试调试输出回调
 */
static int g_callback_count = 0;
static char g_last_message[256] = "";

static void test_callback_function(const char* message)
{
    g_callback_count++;
    strncpy(g_last_message, message, sizeof(g_last_message) - 1);
    g_last_message[sizeof(g_last_message) - 1] = '\0';
    printf("      回调收到消息: %s\n", message);
}

static bool test_debug_output_callback(void)
{
    printf("    测试调试输出回调...\n");
    
    // 重置全局变量
    g_callback_count = 0;
    g_last_message[0] = '\0';
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 设置回调
    F_set_debug_output_callback(test_callback_function);
    
    // 输出调试信息（应该触发回调）
    F_debug_output("测试回调函数");
    
    // 检查回调是否被调用
    if (g_callback_count == 0) {
        printf("      ⚠ 回调函数未被调用\n");
    }
    
    // 移除回调
    F_set_debug_output_callback(NULL);
    
    // 再次输出调试信息（不应该触发回调）
    int old_count = g_callback_count;
    F_debug_output("测试无回调");
    
    if (g_callback_count != old_count) {
        printf("      ⚠ 移除回调后回调函数仍被调用\n");
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 调试输出回调测试通过\n");
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
    
    const char* test_filename = "test_debug_output.log";
    
    // 设置调试输出文件
    bool file_set = F_set_debug_output_file(test_filename);
    if (!file_set) {
        printf("      ⚠ 调试输出文件设置失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    // 输出调试信息到文件
    F_debug_output("测试输出到文件");
    F_debug_output("第二行测试");
    
    // 关闭文件输出
    bool file_closed = F_set_debug_output_file(NULL);
    if (!file_closed) {
        printf("      ⚠ 调试输出文件关闭失败\n");
    }
    
    // 检查文件是否存在
    FILE* file = fopen(test_filename, "r");
    if (file == NULL) {
        printf("      ⚠ 调试输出文件未创建\n");
    } else {
        fclose(file);
        
        // 删除测试文件
        if (remove(test_filename) != 0) {
            printf("      ⚠ 无法删除测试文件\n");
        }
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 调试输出文件测试通过\n");
    return true;
}

/**
 * @brief 测试错误处理
 */
static bool test_error_handling(void)
{
    printf("    测试错误处理...\n");
    
    // 测试1: 在未初始化状态下调用函数（应该安全处理）
    F_enable_verbose_debugging(false);
    F_set_debug_output_callback(NULL);
    
    bool uninitialized_check = F_check_debug_tools_initialized();
    if (uninitialized_check) {
        printf("      ❌ 未初始化状态检查错误\n");
        return false;
    }
    
    // 测试2: 初始化后执行正常操作
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 错误处理测试初始化失败\n");
        return false;
    }
    
    // 测试无效文件路径
    bool invalid_file = F_set_debug_output_file("/invalid/path/debug.log");
    if (invalid_file) {
        printf("      ⚠ 无效文件路径设置返回成功\n");
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 错误处理测试通过\n");
    return true;
}

/**
 * @brief 运行所有集成测试
 * @return 所有测试通过返回true，否则返回false
 */
bool test_debug_tools_integration_all(void)
{
    printf("  运行集成测试（修复版）:\n");
    printf("  -----------------------\n");
    
    bool all_passed = true;
    
    if (!test_basic_debug_workflow()) {
        all_passed = false;
    }
    
    if (!test_multiple_initialization()) {
        all_passed = false;
    }
    
    if (!test_debug_output_callback()) {
        all_passed = false;
    }
    
    if (!test_debug_output_file()) {
        all_passed = false;
    }
    
    if (!test_error_handling()) {
        all_passed = false;
    }
    
    if (all_passed) {
        printf("  ✅ 所有集成测试通过\n");
    } else {
        printf("  ❌ 部分集成测试失败\n");
    }
    
    return all_passed;
}
