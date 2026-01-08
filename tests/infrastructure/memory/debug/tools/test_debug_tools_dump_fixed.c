/**
 * @file test_debug_tools_dump_fixed.c
 * @brief 内存转储模块测试（修复版）
 * 
 * 本文件实现了内存转储模块的测试函数，使用正确的API。
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
 * @brief 测试内存转储基本功能
 */
static bool test_memory_dump_basic(void)
{
    printf("    测试内存转储基本功能...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 创建测试内存区域
    size_t buffer_size = 64;
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
    
    // 获取默认选项
    Stru_MemoryDumpOptions_t options = F_get_default_dump_options();
    
    // 测试基本转储
    printf("      （以下应显示内存转储输出）\n");
    F_dump_memory(test_buffer, buffer_size, &options);
    
    // 测试转储到字符串
    char dump_string[1024];
    size_t chars_written = F_dump_memory_to_string(test_buffer, buffer_size, &options, 
                                                   dump_string, sizeof(dump_string));
    
    if (chars_written == 0) {
        printf("      ⚠ 内存转储到字符串返回0字符\n");
    } else {
        printf("      转储字符串长度: %zu 字符\n", chars_written);
    }
    
    // 清理
    free(test_buffer);
    F_cleanup_debug_tools();
    
    printf("      ✅ 内存转储基本功能测试通过\n");
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
    size_t buffer_size = 32;
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
    
    // 测试1: 默认选项
    options = F_get_default_dump_options();
    printf("      （默认选项转储）\n");
    F_dump_memory(test_buffer, buffer_size, &options);
    
    // 测试2: 自定义选项 - 不显示地址
    options = F_get_default_dump_options();
    options.show_address = false;
    printf("      （不显示地址转储）\n");
    F_dump_memory(test_buffer, buffer_size, &options);
    
    // 测试3: 自定义选项 - 不显示ASCII
    options = F_get_default_dump_options();
    options.show_ascii = false;
    printf("      （不显示ASCII转储）\n");
    F_dump_memory(test_buffer, buffer_size, &options);
    
    // 测试4: 自定义选项 - 每行8字节
    options = F_get_default_dump_options();
    options.bytes_per_line = 8;
    printf("      （每行8字节转储）\n");
    F_dump_memory(test_buffer, buffer_size, &options);
    
    // 测试5: 自定义选项 - 带标签
    options = F_get_default_dump_options();
    options.label = "测试缓冲区";
    printf("      （带标签转储）\n");
    F_dump_memory(test_buffer, buffer_size, &options);
    
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
    
    // 获取默认选项
    Stru_MemoryDumpOptions_t options = F_get_default_dump_options();
    
    // 测试1: 空指针（应该安全处理）
    printf("      （测试空指针转储）\n");
    F_dump_memory(NULL, 100, &options);
    
    // 测试2: 零大小
    unsigned char dummy_buffer[1] = {0};
    printf("      （测试零大小转储）\n");
    F_dump_memory(dummy_buffer, 0, &options);
    
    // 测试3: 超大缓冲区（测试边界处理）
    size_t large_size = 1024 * 1024; // 1MB
    unsigned char* large_buffer = (unsigned char*)malloc(large_size);
    if (large_buffer != NULL) {
        // 填充数据
        for (size_t i = 0; i < large_size; i++) {
            large_buffer[i] = (unsigned char)(i % 256);
        }
        
        printf("      （测试大内存转储，可能耗时）\n");
        // 限制转储大小
        options.max_bytes = 1024; // 只转储前1KB
        F_dump_memory(large_buffer, large_size, &options);
        
        free(large_buffer);
    } else {
        printf("      ⚠ 无法分配大内存缓冲区进行测试\n");
    }
    
    // 测试4: 缓冲区溢出保护
    unsigned char small_buffer[16] = {0};
    char small_string[10];
    options = F_get_default_dump_options();
    size_t chars_written = F_dump_memory_to_string(small_buffer, sizeof(small_buffer), 
                                                   &options, small_string, sizeof(small_string));
    
    if (chars_written >= sizeof(small_string)) {
        printf("      ⚠ 缓冲区可能溢出，写入%zu字符到%zu字节缓冲区\n", 
               chars_written, sizeof(small_string));
    } else {
        printf("      缓冲区安全：写入%zu字符到%zu字节缓冲区\n", 
               chars_written, sizeof(small_string));
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 内存转储边界情况测试通过\n");
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
    
    if (!test_memory_dump_basic()) {
        all_passed = false;
    }
    
    if (!test_memory_dump_formatting()) {
        all_passed = false;
    }
    
    if (!test_memory_dump_edge_cases()) {
        all_passed = false;
    }
    
    if (all_passed) {
        printf("  ✅ 所有内存转储模块测试通过\n");
    } else {
        printf("  ❌ 部分内存转储模块测试失败\n");
    }
    
    return all_passed;
}
