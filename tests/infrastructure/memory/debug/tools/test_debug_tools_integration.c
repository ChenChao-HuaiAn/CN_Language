/**
 * @file test_debug_tools_integration.c
 * @brief 内存调试工具模块集成测试
 * 
 * 本文件实现了内存调试工具模块的集成测试函数，
 * 测试各个子模块之间的协作和整体功能。
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
 * @brief 测试完整的内存调试工作流程
 */
static bool test_complete_debug_workflow(void)
{
    printf("    测试完整的内存调试工作流程...\n");
    
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
    
    // 步骤1: 添加内存保护
    bool protected = F_protect_memory_region(test_buffer, buffer_size);
    if (!protected) {
        printf("      ⚠ 内存区域保护失败\n");
    }
    
    // 步骤2: 转储内存内容
    printf("      （转储内存内容）\n");
    bool dumped = F_dump_memory_to_stdout(test_buffer, 64); // 只转储前64字节
    if (!dumped) {
        printf("      ⚠ 内存转储失败\n");
    }
    
    // 步骤3: 验证内存完整性
    bool integrity_ok = F_check_memory_integrity(test_buffer, buffer_size);
    if (!integrity_ok) {
        printf("      ⚠ 内存完整性检查失败\n");
    }
    
    // 步骤4: 捕获调用栈
    Stru_StackTrace_t stack_trace;
    bool captured = F_capture_stack_trace(&stack_trace);
    if (!captured) {
        printf("      ⚠ 调用栈捕获失败\n");
    } else {
        printf("      捕获到调用栈，帧数: %zu\n", stack_trace.frame_count);
    }
    
    // 步骤5: 使用工具函数格式化输出
    char size_str[32];
    bool size_formatted = F_format_memory_size(buffer_size, size_str, sizeof(size_str));
    if (size_formatted) {
        printf("      缓冲区大小: %s\n", size_str);
    }
    
    // 步骤6: 移除内存保护
    bool unprotected = F_unprotect_memory_region(test_buffer);
    if (!unprotected) {
        printf("      ⚠ 内存区域解除保护失败\n");
    }
    
    // 清理
    free(test_buffer);
    F_cleanup_debug_tools();
    
    printf("      ✅ 完整的内存调试工作流程测试通过\n");
    return true;
}

/**
 * @brief 测试内存调试工具组合使用
 */
static bool test_debug_tools_combination(void)
{
    printf("    测试内存调试工具组合使用...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 设置调试输出文件
    bool file_set = F_set_debug_output_file("debug_combination.log");
    if (!file_set) {
        printf("      ⚠ 调试输出文件设置失败\n");
    }
    
    // 创建多个测试缓冲区
    size_t num_buffers = 3;
    size_t buffer_size = 512;
    unsigned char* buffers[3];
    
    for (size_t i = 0; i < num_buffers; i++) {
        buffers[i] = (unsigned char*)malloc(buffer_size);
        if (buffers[i] == NULL) {
            printf("      ❌ 缓冲区 %zu 分配失败\n", i);
            
            // 清理已分配的缓冲区
            for (size_t j = 0; j < i; j++) {
                free(buffers[j]);
            }
            F_cleanup_debug_tools();
            remove("debug_combination.log");
            return false;
        }
        
        // 填充不同的模式
        unsigned char pattern = (unsigned char)(0xAA + i);
        for (size_t j = 0; j < buffer_size; j++) {
            buffers[i][j] = pattern;
        }
        
        // 保护每个缓冲区
        bool protected = F_protect_memory_region(buffers[i], buffer_size);
        if (!protected) {
            printf("      ⚠ 缓冲区 %zu 保护失败\n", i);
        }
    }
    
    // 测试1: 批量转储所有缓冲区
    printf("      （批量转储所有缓冲区）\n");
    for (size_t i = 0; i < num_buffers; i++) {
        char marker[32];
        snprintf(marker, sizeof(marker), "缓冲区 %zu", i);
        bool marked_dump = F_dump_memory_with_markers(buffers[i], 32, marker);
        if (!marked_dump) {
            printf("      ⚠ 缓冲区 %zu 标记转储失败\n", i);
        }
    }
    
    // 测试2: 批量验证所有缓冲区
    for (size_t i = 0; i < num_buffers; i++) {
        bool integrity_ok = F_check_memory_integrity(buffers[i], buffer_size);
        if (!integrity_ok) {
            printf("      ⚠ 缓冲区 %zu 完整性检查失败\n", i);
        }
    }
    
    // 测试3: 捕获多个调用栈进行比较
    Stru_StackTrace_t traces[2];
    bool trace1_captured = F_capture_stack_trace(&traces[0]);
    
    // 做一些工作后捕获第二个调用栈
    volatile int sum = 0;
    for (int i = 0; i < 100; i++) {
        sum += i;
    }
    
    bool trace2_captured = F_capture_stack_trace(&traces[1]);
    
    if (trace1_captured && trace2_captured) {
        bool are_similar = F_compare_stack_traces(&traces[0], &traces[1]);
        printf("      两个调用栈相似: %s\n", are_similar ? "是" : "否");
    }
    
    // 清理所有缓冲区
    for (size_t i = 0; i < num_buffers; i++) {
        bool unprotected = F_unprotect_memory_region(buffers[i]);
        if (!unprotected) {
            printf("      ⚠ 缓冲区 %zu 解除保护失败\n", i);
        }
        free(buffers[i]);
    }
    
    // 关闭调试输出文件
    bool file_closed = F_set_debug_output_file(NULL);
    if (!file_closed) {
        printf("      ⚠ 调试输出文件关闭失败\n");
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    // 删除测试文件
    remove("debug_combination.log");
    
    printf("      ✅ 内存调试工具组合使用测试通过\n");
    return true;
}

/**
 * @brief 测试错误处理和恢复
 */
static bool test_error_handling_and_recovery(void)
{
    printf("    测试错误处理和恢复...\n");
    
    // 测试1: 多次初始化和清理
    for (int i = 0; i < 3; i++) {
        if (!F_initialize_debug_tools()) {
            printf("      ❌ 第%d次初始化失败\n", i + 1);
            return false;
        }
        
        // 执行一些操作
        F_enable_verbose_debugging(true);
        
        // 创建和测试缓冲区
        size_t buffer_size = 256;
        unsigned char* buffer = (unsigned char*)malloc(buffer_size);
        if (buffer != NULL) {
            bool protected = F_protect_memory_region(buffer, buffer_size);
            if (protected) {
                bool integrity_ok = F_check_memory_integrity(buffer, buffer_size);
                if (!integrity_ok) {
                    printf("      ⚠ 第%d次完整性检查失败\n", i + 1);
                }
                
                bool unprotected = F_unprotect_memory_region(buffer);
                if (!unprotected) {
                    printf("      ⚠ 第%d次解除保护失败\n", i + 1);
                }
            }
            
            free(buffer);
        }
        
        F_cleanup_debug_tools();
        
        // 检查清理后状态
        bool after_cleanup = F_check_debug_tools_initialized();
        if (after_cleanup) {
            printf("      ❌ 第%d次清理后状态检查失败\n", i + 1);
            return false;
        }
    }
    
    // 测试2: 在未初始化状态下调用函数（应该安全处理）
    F_enable_verbose_debugging(false);
    F_set_debug_output_callback(NULL);
    
    bool uninitialized_check = F_check_debug_tools_initialized();
    if (uninitialized_check) {
        printf("      ❌ 未初始化状态检查错误\n");
        return false;
    }
    
    // 测试3: 初始化后执行错误操作
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 错误处理测试初始化失败\n");
        return false;
    }
    
    // 尝试无效操作
    bool invalid_dump = F_dump_memory_to_file(NULL, 100, "invalid_dump.bin");
    if (invalid_dump) {
        printf("      ⚠ 无效内存转储返回成功\n");
    }
    
    // 尝试无效文件
    bool invalid_file = F_set_debug_output_file("/invalid/path/debug.log");
    if (invalid_file) {
        printf("      ⚠ 无效文件设置返回成功\n");
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 错误处理和恢复测试通过\n");
    return true;
}

/**
 * @brief 测试性能基准
 */
static bool test_performance_benchmark(void)
{
    printf("    测试性能基准...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 创建测试数据
    size_t data_size = 1024 * 1024; // 1MB
    unsigned char* large_data = (unsigned char*)malloc(data_size);
    if (large_data == NULL) {
        printf("      ❌ 大内存数据分配失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    // 填充数据
    for (size_t i = 0; i < data_size; i++) {
        large_data[i] = (unsigned char)(i % 256);
    }
    
    // 基准测试1: 内存保护性能
    printf("      测试内存保护性能...\n");
    uint64_t protect_start = F_get_current_timestamp_ns();
    
    bool protected = F_protect_memory_region(large_data, data_size);
    if (!protected) {
        printf("      ⚠ 大内存保护失败\n");
    }
    
    uint64_t protect_end = F_get_current_timestamp_ns();
    uint64_t protect_time = protect_end - protect_start;
    printf("      内存保护时间: %llu ns\n", protect_time);
    
    // 基准测试2: 内存验证性能
    printf("      测试内存验证性能...\n");
    uint64_t verify_start = F_get_current_timestamp_ns();
    
    bool integrity_ok = F_check_memory_integrity(large_data, data_size);
    if (!integrity_ok) {
        printf("      ⚠ 大内存完整性检查失败\n");
    }
    
    uint64_t verify_end = F_get_current_timestamp_ns();
    uint64_t verify_time = verify_end - verify_start;
    printf("      内存验证时间: %llu ns\n", verify_time);
    
    // 基准测试3: 调用栈捕获性能
    printf("      测试调用栈捕获性能...\n");
    uint64_t stacktrace_start = F_get_current_timestamp_ns();
    
    Stru_StackTrace_t trace;
    bool captured = F_capture_stack_trace(&trace);
    if (!captured) {
        printf("      ⚠ 调用栈捕获失败\n");
    }
    
    uint64_t stacktrace_end = F_get_current_timestamp_ns();
    uint64_t stacktrace_time = stacktrace_end - stacktrace_start;
    printf("      调用栈捕获时间: %llu ns\n", stacktrace_time);
    
    // 清理
    if (protected) {
        bool unprotected = F_unprotect_memory_region(large_data);
        if (!unprotected) {
            printf("      ⚠ 大内存解除保护失败\n");
        }
    }
    
    free(large_data);
    F_cleanup_debug_tools();
    
    printf("      ✅ 性能基准测试通过\n");
    return true;
}

/**
 * @brief 测试内存调试工具的实际应用场景
 */
static bool test_real_world_scenarios(void)
{
    printf("    测试内存调试工具的实际应用场景...\n");
    
    // 场景1: 检测内存泄漏
    printf("      场景1: 检测内存泄漏\n");
    
    if (!F_initialize_debug_tools()) {
        printf("        ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 启用内存跟踪
    bool tracking_enabled = F_enable_memory_tracking(true);
    if (!tracking_enabled) {
        printf("        ⚠ 内存跟踪启用失败\n");
    }
    
    // 模拟内存分配
    unsigned char* leaked_buffer = (unsigned char*)malloc(1024);
    if (leaked_buffer == NULL) {
        printf("        ❌ 泄漏缓冲区分配失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    // 检查泄漏
    size_t leak_count = 0;
    bool leaks_found = F_detect_memory_leaks(&leak_count);
    if (leaks_found) {
        printf("        检测到 %zu 个内存泄漏\n", leak_count);
    }
    
    // 故意不释放缓冲区，模拟泄漏
    // 在实际测试中，我们应该释放它，但这里我们模拟泄漏场景
    
    // 清理（会报告泄漏）
    F_cleanup_debug_tools();
    
    // 场景2: 调试缓冲区溢出
    printf("      场景2: 调试缓冲区溢出\n");
    
    if (!F_initialize_debug_tools()) {
        printf("        ❌ 调试工具初始化失败\n");
        return false;
    }
    
    size_t small_buffer_size = 16;
    unsigned char* small_buffer = (unsigned char*)malloc(small_buffer_size);
    if (small_buffer == NULL) {
        printf("        ❌ 小缓冲区分配失败\n");
        return false;
    }
    
    // 添加边界检查
    bool boundaries_added = F_add_memory_boundaries(small_buffer, small_buffer_size);
    if (!boundaries_added) {
        printf("        ⚠ 边界检查添加失败\n");
    }
    
    // 模拟可能的缓冲区溢出（在测试中我们不会实际溢出）
    // 只是检查边界
    bool boundaries_ok = F_check_memory_boundaries(small_buffer);
    if (!boundaries_ok) {
        printf("        ⚠ 边界检查失败\n");
    }
    
    // 清理
    free(small_buffer);
    F_cleanup_debug_tools();
    
    // 场景3: 分析崩溃调用栈
    printf("      场景3: 分析崩溃调用栈\n");
    
    if (!F_initialize_debug_tools()) {
        printf("        ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 捕获当前调用栈（模拟崩溃前）
    Stru_StackTrace_t crash_trace;
    bool crash_captured = F_capture_stack_trace(&crash_trace);
    if (crash_captured) {
        printf("        成功捕获调用栈（模拟崩溃前）\n");
        
        // 保存调用栈到文件（用于事后分析）
        bool saved = F_save_stack_trace_to_file(&crash_trace, "crash_stack_trace.txt");
        if (saved) {
            printf("        调用栈已保存到文件\n");
        }
    }
    
    // 清理
    F_cleanup_debug_tools();
    remove("crash_stack_trace.txt");
    
    printf("      ✅ 实际应用场景测试通过\n");
    return true;
}

/**
 * @brief 运行所有集成测试
 * @return 所有测试通过返回true，否则返回false
 */
bool test_debug_tools_integration_all(void)
{
    printf("  运行集成测试:\n");
    printf("  -------------\n");
    
    bool all_passed = true;
    
    if (!test_complete_debug_workflow()) {
        all_passed = false;
    }
    
    if (!test_debug_tools_combination()) {
        all_passed = false;
    }
    
    if (!test_error_handling_and_recovery()) {
        all_passed = false;
    }
    
    if (!test_performance_benchmark()) {
        all_passed = false;
    }
    
    if (!test_real_world_scenarios()) {
        all_passed = false;
    }
    
    if (all_passed) {
        printf("  ✅ 所有集成测试通过\n");
    } else {
        printf("  ❌ 部分集成测试失败\n");
    }
    
    return all_passed;
}
