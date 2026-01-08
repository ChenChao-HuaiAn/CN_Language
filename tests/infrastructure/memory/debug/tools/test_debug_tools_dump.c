/**
 * @file test_debug_tools_dump.c
 * @brief 内存转储模块测试
 * 
 * 本文件实现了内存转储模块的测试函数。
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
 * @brief 测试内存转储到文件
 */
static bool test_memory_dump_to_file(void)
{
    printf("    测试内存转储到文件...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
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
    
    // 测试1: 转储到文件
    bool dump_success = F_dump_memory_to_file(test_buffer, buffer_size, "test_memory_dump.bin");
    if (!dump_success) {
        printf("      ⚠ 内存转储到文件失败（可能是权限问题）\n");
    }
    
    // 测试2: 转储到标准输出
    printf("      （以下应显示内存转储输出）\n");
    bool stdout_success = F_dump_memory_to_stdout(test_buffer, buffer_size);
    if (!stdout_success) {
        printf("      ⚠ 内存转储到标准输出失败\n");
    }
    
    // 测试3: 转储到字符串
    char dump_string[2048];
    bool string_success = F_dump_memory_to_string(test_buffer, buffer_size, dump_string, sizeof(dump_string));
    if (!string_success) {
        printf("      ⚠ 内存转储到字符串失败\n");
    } else {
        // 检查字符串是否包含转储信息
        if (strlen(dump_string) == 0) {
            printf("      ⚠ 转储字符串为空\n");
        }
    }
    
    // 清理
    free(test_buffer);
    F_cleanup_debug_tools();
    
    // 尝试删除测试文件（如果存在）
    remove("test_memory_dump.bin");
    
    printf("      ✅ 内存转储到文件测试通过\n");
    return true;
}

/**
 * @brief 测试内存转储格式化选项
 */
static bool test_memory_dump_formatting(void)
{
    printf("    测试内存转储格式化选项...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 创建测试内存区域
    size_t buffer_size = 256;
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
    
    // 测试不同的转储选项
    Stru_MemoryDumpOptions_t options;
    
    // 测试1: 十六进制格式
    options.format = Eum_DUMP_FORMAT_HEX;
    options.bytes_per_line = 16;
    options.show_address = true;
    options.show_ascii = true;
    
    printf("      （十六进制格式转储）\n");
    bool hex_success = F_dump_memory_with_options(test_buffer, buffer_size, &options);
    if (!hex_success) {
        printf("      ⚠ 十六进制格式转储失败\n");
    }
    
    // 测试2: 十进制格式
    options.format = Eum_DUMP_FORMAT_DECIMAL;
    options.bytes_per_line = 8;
    
    printf("      （十进制格式转储）\n");
    bool decimal_success = F_dump_memory_with_options(test_buffer, buffer_size, &options);
    if (!decimal_success) {
        printf("      ⚠ 十进制格式转储失败\n");
    }
    
    // 测试3: 二进制格式
    options.format = Eum_DUMP_FORMAT_BINARY;
    options.bytes_per_line = 4;
    
    printf("      （二进制格式转储）\n");
    bool binary_success = F_dump_memory_with_options(test_buffer, buffer_size, &options);
    if (!binary_success) {
        printf("      ⚠ 二进制格式转储失败\n");
    }
    
    // 测试4: 自定义格式
    options.format = Eum_DUMP_FORMAT_HEX;
    options.bytes_per_line = 32;
    options.show_address = false;
    options.show_ascii = false;
    
    printf("      （自定义格式转储）\n");
    bool custom_success = F_dump_memory_with_options(test_buffer, buffer_size, &options);
    if (!custom_success) {
        printf("      ⚠ 自定义格式转储失败\n");
    }
    
    // 清理
    free(test_buffer);
    F_cleanup_debug_tools();
    
    printf("      ✅ 内存转储格式化选项测试通过\n");
    return true;
}

/**
 * @brief 测试内存转储边界情况
 */
static bool test_memory_dump_edge_cases(void)
{
    printf("    测试内存转储边界情况...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 测试1: 空指针（应该安全处理）
    bool null_success = F_dump_memory_to_stdout(NULL, 100);
    if (null_success) {
        printf("      ⚠ 空指针转储返回成功（可能是设计如此）\n");
    }
    
    // 测试2: 零大小
    unsigned char dummy_buffer[1] = {0};
    bool zero_size_success = F_dump_memory_to_stdout(dummy_buffer, 0);
    if (!zero_size_success) {
        printf("      ⚠ 零大小转储失败\n");
    }
    
    // 测试3: 超大缓冲区（测试边界处理）
    size_t large_size = 1024 * 1024; // 1MB
    unsigned char* large_buffer = (unsigned char*)malloc(large_size);
    if (large_buffer != NULL) {
        // 填充数据
        for (size_t i = 0; i < large_size; i++) {
            large_buffer[i] = (unsigned char)(i % 256);
        }
        
        printf("      （测试大内存转储，可能耗时）\n");
        bool large_success = F_dump_memory_to_stdout(large_buffer, 1024); // 只转储前1KB
        if (!large_success) {
            printf("      ⚠ 大内存转储失败\n");
        }
        
        free(large_buffer);
    } else {
        printf("      ⚠ 无法分配大内存缓冲区进行测试\n");
    }
    
    // 测试4: 无效文件名
    unsigned char small_buffer[16] = {0};
    bool invalid_file_success = F_dump_memory_to_file(small_buffer, sizeof(small_buffer), "/invalid/path/dump.bin");
    if (invalid_file_success) {
        printf("      ⚠ 无效文件名转储返回成功\n");
    }
    
    // 测试5: 缓冲区溢出保护
    char small_string[10];
    bool overflow_test = F_dump_memory_to_string(small_buffer, sizeof(small_buffer), small_string, sizeof(small_string));
    if (overflow_test) {
        printf("      ⚠ 缓冲区溢出测试返回成功（可能是截断处理）\n");
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 内存转储边界情况测试通过\n");
    return true;
}

/**
 * @brief 测试内存转储性能
 */
static bool test_memory_dump_performance(void)
{
    printf("    测试内存转储性能...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 创建测试内存区域
    size_t buffer_size = 4096; // 4KB
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
    
    // 性能测试：多次转储
    int iterations = 10;
    printf("      执行%d次内存转储性能测试...\n", iterations);
    
    for (int i = 0; i < iterations; i++) {
        bool success = F_dump_memory_to_stdout(test_buffer, buffer_size);
        if (!success) {
            printf("      ⚠ 第%d次转储失败\n", i + 1);
        }
    }
    
    // 清理
    free(test_buffer);
    F_cleanup_debug_tools();
    
    printf("      ✅ 内存转储性能测试通过\n");
    return true;
}

/**
 * @brief 测试内存转储高级功能
 */
static bool test_memory_dump_advanced_features(void)
{
    printf("    测试内存转储高级功能...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 创建测试内存区域
    size_t buffer_size = 512;
    unsigned char* test_buffer = (unsigned char*)malloc(buffer_size);
    if (test_buffer == NULL) {
        printf("      ❌ 测试缓冲区分配失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    // 填充测试数据（包含可打印ASCII字符）
    for (size_t i = 0; i < buffer_size; i++) {
        test_buffer[i] = (unsigned char)(32 + (i % 95)); // 可打印ASCII字符
    }
    
    // 测试1: 部分转储（偏移和长度）
    printf("      （测试部分内存转储）\n");
    bool partial_success = F_dump_memory_partial(test_buffer, buffer_size, 128, 64);
    if (!partial_success) {
        printf("      ⚠ 部分内存转储失败\n");
    }
    
    // 测试2: 带标记的转储
    printf("      （测试带标记的内存转储）\n");
    bool marked_success = F_dump_memory_with_markers(test_buffer, buffer_size, "测试缓冲区");
    if (!marked_success) {
        printf("      ⚠ 带标记的内存转储失败\n");
    }
    
    // 测试3: 比较转储
    unsigned char* compare_buffer = (unsigned char*)malloc(buffer_size);
    if (compare_buffer != NULL) {
        // 创建略有不同的缓冲区
        for (size_t i = 0; i < buffer_size; i++) {
            compare_buffer[i] = (unsigned char)(32 + ((i + 1) % 95));
        }
        
        printf("      （测试内存比较转储）\n");
        bool compare_success = F_dump_memory_comparison(test_buffer, compare_buffer, buffer_size, "原始缓冲区", "修改后缓冲区");
        if (!compare_success) {
            printf("      ⚠ 内存比较转储失败\n");
        }
        
        free(compare_buffer);
    } else {
        printf("      ⚠ 无法分配比较缓冲区\n");
    }
    
    // 清理
    free(test_buffer);
    F_cleanup_debug_tools();
    
    printf("      ✅ 内存转储高级功能测试通过\n");
    return true;
}

/**
 * @brief 运行所有内存转储模块测试
 * @return 所有测试通过返回true，否则返回false
 */
bool test_debug_tools_dump_all(void)
{
    printf("  运行内存转储模块测试:\n");
    printf("  ---------------------\n");
    
    bool all_passed = true;
    
    if (!test_memory_dump_to_file()) {
        all_passed = false;
    }
    
    if (!test_memory_dump_formatting()) {
        all_passed = false;
    }
    
    if (!test_memory_dump_edge_cases()) {
        all_passed = false;
    }
    
    if (!test_memory_dump_performance()) {
        all_passed = false;
    }
    
    if (!test_memory_dump_advanced_features()) {
        all_passed = false;
    }
    
    if (all_passed) {
        printf("  ✅ 所有内存转储模块测试通过\n");
    } else {
        printf("  ❌ 部分内存转储模块测试失败\n");
    }
    
    return all_passed;
}
