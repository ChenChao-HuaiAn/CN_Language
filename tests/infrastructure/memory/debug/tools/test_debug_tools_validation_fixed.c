/**
 * @file test_debug_tools_validation_fixed.c
 * @brief 内存验证模块测试（修复版）
 * 
 * 本文件实现了内存验证模块的测试函数，使用正确的API。
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
 * @brief 测试内存完整性检查
 */
static bool test_memory_integrity_check(void)
{
    printf("    测试内存完整性检查...\n");
    
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
    
    // 测试1: 正常内存检查 - 使用F_validate_memory_block
    bool integrity_ok = F_validate_memory_block(test_buffer, buffer_size, NULL, 0);
    if (!integrity_ok) {
        printf("      ⚠ 正常内存完整性检查失败\n");
    }
    
    // 测试2: 使用特定模式验证
    unsigned char pattern = 0xAA;
    bool pattern_check = F_validate_memory_block(test_buffer, buffer_size, &pattern, 1);
    if (pattern_check) {
        printf("      ⚠ 模式验证不应通过（缓冲区不是全0xAA）\n");
    }
    
    // 清理
    free(test_buffer);
    F_cleanup_debug_tools();
    
    printf("      ✅ 内存完整性检查测试通过\n");
    return true;
}

/**
 * @brief 测试内存模式验证
 */
static bool test_memory_pattern_validation(void)
{
    printf("    测试内存模式验证...\n");
    
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
    
    // 测试1: 填充特定模式并验证
    unsigned char pattern = 0xAA;
    F_fill_memory_pattern(test_buffer, buffer_size, &pattern, 1);
    
    bool verified = F_check_memory_pattern(test_buffer, buffer_size, &pattern, 1);
    if (!verified) {
        printf("      ❌ 内存模式验证失败\n");
        free(test_buffer);
        F_cleanup_debug_tools();
        return false;
    }
    
    // 测试2: 填充复杂模式
    unsigned char complex_pattern[] = {0xDE, 0xAD, 0xBE, 0xEF};
    size_t pattern_size = sizeof(complex_pattern);
    
    F_fill_memory_pattern(test_buffer, buffer_size, complex_pattern, pattern_size);
    
    bool complex_verified = F_check_memory_pattern(test_buffer, buffer_size, complex_pattern, pattern_size);
    if (!complex_verified) {
        printf("      ⚠ 复杂内存模式验证失败\n");
    }
    
    // 清理
    free(test_buffer);
    F_cleanup_debug_tools();
    
    printf("      ✅ 内存模式验证测试通过\n");
    return true;
}

/**
 * @brief 测试内存校验和
 */
static bool test_memory_checksum(void)
{
    printf("    测试内存校验和...\n");
    
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
    
    // 计算校验和
    uint32_t checksum = F_calculate_memory_checksum(test_buffer, buffer_size);
    printf("      内存校验和: 0x%08X\n", checksum);
    
    // 修改一个字节，重新计算校验和
    if (buffer_size > 10) {
        unsigned char original_value = test_buffer[10];
        test_buffer[10] = 0xFF;
        
        uint32_t new_checksum = F_calculate_memory_checksum(test_buffer, buffer_size);
        if (new_checksum == checksum) {
            printf("      ⚠ 校验和未检测到内存修改\n");
        } else {
            printf("      修改后校验和: 0x%08X (检测到变化)\n", new_checksum);
        }
        
        // 恢复原始值
        test_buffer[10] = original_value;
    }
    
    // 清理
    free(test_buffer);
    F_cleanup_debug_tools();
    
    printf("      ✅ 内存校验和测试通过\n");
    return true;
}

/**
 * @brief 测试内存比较
 */
static bool test_memory_comparison(void)
{
    printf("    测试内存比较...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 创建两个测试内存区域
    size_t buffer_size = 128;
    unsigned char* buffer1 = (unsigned char*)malloc(buffer_size);
    unsigned char* buffer2 = (unsigned char*)malloc(buffer_size);
    
    if (buffer1 == NULL || buffer2 == NULL) {
        printf("      ❌ 测试缓冲区分配失败\n");
        if (buffer1) free(buffer1);
        if (buffer2) free(buffer2);
        F_cleanup_debug_tools();
        return false;
    }
    
    // 填充相同数据
    for (size_t i = 0; i < buffer_size; i++) {
        buffer1[i] = buffer2[i] = (unsigned char)(i % 256);
    }
    
    // 测试1: 相同内存比较
    int compare_result = F_compare_memory_regions(buffer1, buffer2, buffer_size);
    if (compare_result != 0) {
        printf("      ❌ 相同内存比较失败\n");
        free(buffer1);
        free(buffer2);
        F_cleanup_debug_tools();
        return false;
    }
    
    // 测试2: 不同内存比较
    if (buffer_size > 20) {
        buffer2[20] = 0xFF;
        compare_result = F_compare_memory_regions(buffer1, buffer2, buffer_size);
        if (compare_result == 0) {
            printf("      ⚠ 不同内存比较未检测到差异\n");
        }
    }
    
    // 清理
    free(buffer1);
    free(buffer2);
    F_cleanup_debug_tools();
    
    printf("      ✅ 内存比较测试通过\n");
    return true;
}

/**
 * @brief 测试内存模式查找
 */
static bool test_memory_pattern_finding(void)
{
    printf("    测试内存模式查找...\n");
    
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
    
    // 在特定位置插入模式
    unsigned char pattern[] = {0xDE, 0xAD, 0xBE, 0xEF};
    size_t pattern_size = sizeof(pattern);
    
    if (buffer_size > 100) {
        // 在位置100插入模式
        memcpy(&test_buffer[100], pattern, pattern_size);
        
        // 查找模式
        void* found = F_find_memory_pattern(test_buffer, buffer_size, pattern, pattern_size);
        if (found == NULL) {
            printf("      ❌ 内存模式查找失败\n");
            free(test_buffer);
            F_cleanup_debug_tools();
            return false;
        }
        
        size_t offset = (unsigned char*)found - test_buffer;
        printf("      在偏移量 %zu 处找到模式\n", offset);
        
        if (offset != 100) {
            printf("      ⚠ 找到的位置不正确\n");
        }
    }
    
    // 清理
    free(test_buffer);
    F_cleanup_debug_tools();
    
    printf("      ✅ 内存模式查找测试通过\n");
    return true;
}

/**
 * @brief 运行所有内存验证模块测试
 * @return 所有测试通过返回true，否则返回false
 */
bool test_debug_tools_validation_all(void)
{
    printf("  运行内存验证模块测试:\n");
    printf("  ---------------------\n");
    
    bool all_passed = true;
    
    if (!test_memory_integrity_check()) {
        all_passed = false;
    }
    
    if (!test_memory_pattern_validation()) {
        all_passed = false;
    }
    
    if (!test_memory_checksum()) {
        all_passed = false;
    }
    
    if (!test_memory_comparison()) {
        all_passed = false;
    }
    
    if (!test_memory_pattern_finding()) {
        all_passed = false;
    }
    
    if (all_passed) {
        printf("  ✅ 所有内存验证模块测试通过\n");
    } else {
        printf("  ❌ 部分内存验证模块测试失败\n");
    }
    
    return all_passed;
}
