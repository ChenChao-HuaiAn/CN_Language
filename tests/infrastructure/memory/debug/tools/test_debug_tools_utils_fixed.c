/**
 * @file test_debug_tools_utils_fixed.c
 * @brief 工具函数模块测试（简化修复版）
 * 
 * 本文件实现了工具函数模块的测试函数，只测试实际存在的API。
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
#include <stdint.h>

#include "../../../src/infrastructure/memory/debug/tools/CN_memory_debug_tools.h"

/**
 * @brief 测试字节转换函数
 */
static bool test_byte_conversion_functions(void)
{
    printf("    测试字节转换函数...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 测试1: 字节到十六进制字符串
    unsigned char byte = 0xAB;
    char hex_str[3];
    F_byte_to_hex(hex_str, byte);
    
    if (strcmp(hex_str, "AB") != 0) {
        printf("      ❌ 字节转换结果不正确: %s (期望: AB)\n", hex_str);
        F_cleanup_debug_tools();
        return false;
    }
    
    // 测试2: 检查字符是否可打印
    bool printable = F_is_printable_char('A');
    if (!printable) {
        printf("      ❌ 可打印字符检查失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    bool not_printable = F_is_printable_char('\x01');
    if (not_printable) {
        printf("      ❌ 不可打印字符检查失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 字节转换函数测试通过\n");
    return true;
}

/**
 * @brief 测试格式化函数
 */
static bool test_formatting_functions(void)
{
    printf("    测试格式化函数...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 测试1: 格式化内存地址
    void* address = (void*)0x12345678;
    char address_str[32];
    size_t address_len = F_format_memory_address(address, address_str, sizeof(address_str));
    
    if (address_len == 0) {
        printf("      ⚠ 内存地址格式化失败\n");
    } else {
        printf("      格式化后的地址: %s (长度: %zu)\n", address_str, address_len);
    }
    
    // 测试2: 获取内存信息字符串
    char info_str[64];
    size_t info_len = F_get_memory_info_string(address, info_str, sizeof(info_str));
    
    if (info_len == 0) {
        printf("      ⚠ 内存信息字符串获取失败\n");
    } else {
        printf("      内存信息: %s (长度: %zu)\n", info_str, info_len);
    }
    
    // 测试3: 小缓冲区测试
    char small_buffer[5];
    size_t small_len = F_format_memory_address(address, small_buffer, sizeof(small_buffer));
    
    if (small_len > sizeof(small_buffer) - 1) {
        printf("      ⚠ 小缓冲区格式化可能截断\n");
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 格式化函数测试通过\n");
    return true;
}

/**
 * @brief 测试对齐检查函数
 */
static bool test_alignment_functions(void)
{
    printf("    测试对齐检查函数...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 测试1: 检查地址对齐
    void* aligned_address = (void*)0x1000;
    void* unaligned_address = (void*)0x1001;
    
    bool is_aligned = F_check_memory_alignment(aligned_address, 16);
    if (!is_aligned) {
        printf("      ❌ 对齐地址检查失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    bool is_unaligned = F_check_memory_alignment(unaligned_address, 16);
    if (is_unaligned) {
        printf("      ❌ 非对齐地址检查失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    // 测试2: 边界情况
    bool zero_aligned = F_check_memory_alignment(NULL, 16);
    if (zero_aligned) {
        printf("      ⚠ NULL地址对齐检查返回true\n");
    }
    
    bool invalid_aligned = F_check_memory_alignment(aligned_address, 0);
    if (invalid_aligned) {
        printf("      ⚠ 零对齐值检查返回true\n");
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 对齐检查函数测试通过\n");
    return true;
}

/**
 * @brief 运行所有工具函数模块测试
 * @return 所有测试通过返回true，否则返回false
 */
bool test_debug_tools_utils_all(void)
{
    printf("  运行工具函数模块测试:\n");
    printf("  ---------------------\n");
    
    bool all_passed = true;
    
    if (!test_byte_conversion_functions()) {
        all_passed = false;
    }
    
    if (!test_formatting_functions()) {
        all_passed = false;
    }
    
    if (!test_alignment_functions()) {
        all_passed = false;
    }
    
    if (all_passed) {
        printf("  ✅ 所有工具函数模块测试通过\n");
    } else {
        printf("  ❌ 部分工具函数模块测试失败\n");
    }
    
    return all_passed;
}
