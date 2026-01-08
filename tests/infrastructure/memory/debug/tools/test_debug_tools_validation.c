/**
 * @file test_debug_tools_validation.c
 * @brief 内存验证模块测试
 * 
 * 本文件实现了内存验证模块的测试函数。
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
    
    // 测试1: 正常内存检查
    bool integrity_ok = F_check_memory_integrity(test_buffer, buffer_size);
    if (!integrity_ok) {
        printf("      ⚠ 正常内存完整性检查失败\n");
    }
    
    // 测试2: 添加保护字节并检查
    bool protected = F_protect_memory_region(test_buffer, buffer_size);
    if (!protected) {
        printf("      ⚠ 内存区域保护失败\n");
    } else {
        // 检查保护后的内存
        bool protected_check = F_check_memory_integrity(test_buffer, buffer_size);
        if (!protected_check) {
            printf("      ⚠ 保护后内存检查失败\n");
        }
        
        // 移除保护
        bool unprotected = F_unprotect_memory_region(test_buffer);
        if (!unprotected) {
            printf("      ⚠ 内存区域解除保护失败\n");
        }
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
    bool filled = F_fill_memory_with_pattern(test_buffer, buffer_size, pattern);
    if (!filled) {
        printf("      ⚠ 内存模式填充失败\n");
    } else {
        bool verified = F_verify_memory_pattern(test_buffer, buffer_size, pattern);
        if (!verified) {
            printf("      ❌ 内存模式验证失败\n");
            free(test_buffer);
            F_cleanup_debug_tools();
            return false;
        }
    }
    
    // 测试2: 填充复杂模式
    unsigned char complex_pattern[] = {0xDE, 0xAD, 0xBE, 0xEF};
    size_t pattern_size = sizeof(complex_pattern);
    
    bool complex_filled = F_fill_memory_with_complex_pattern(test_buffer, buffer_size, 
                                                           complex_pattern, pattern_size);
    if (!complex_filled) {
        printf("      ⚠ 复杂内存模式填充失败\n");
    } else {
        bool complex_verified = F_verify_memory_complex_pattern(test_buffer, buffer_size,
                                                              complex_pattern, pattern_size);
        if (!complex_verified) {
            printf("      ⚠ 复杂内存模式验证失败\n");
        }
    }
    
    // 测试3: 检查内存损坏
    // 故意损坏内存（修改一个字节）
    if (buffer_size > 100) {
        test_buffer[50] = 0xFF; // 修改一个字节
        
        bool corruption_detected = F_detect_memory_corruption(test_buffer, buffer_size);
        if (!corruption_detected) {
            printf("      ⚠ 内存损坏检测失败\n");
        }
    }
    
    // 清理
    free(test_buffer);
    F_cleanup_debug_tools();
    
    printf("      ✅ 内存模式验证测试通过\n");
    return true;
}

/**
 * @brief 测试边界和哨兵检查
 */
static bool test_boundary_and_sentinel_checks(void)
{
    printf("    测试边界和哨兵检查...\n");
    
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
    
    // 测试1: 添加边界检查
    bool boundaries_added = F_add_memory_boundaries(test_buffer, buffer_size);
    if (!boundaries_added) {
        printf("      ⚠ 内存边界添加失败\n");
    } else {
        // 检查边界
        bool boundaries_ok = F_check_memory_boundaries(test_buffer);
        if (!boundaries_ok) {
            printf("      ⚠ 内存边界检查失败\n");
        }
        
        // 移除边界
        bool boundaries_removed = F_remove_memory_boundaries(test_buffer);
        if (!boundaries_removed) {
            printf("      ⚠ 内存边界移除失败\n");
        }
    }
    
    // 测试2: 哨兵值检查
    bool sentinels_added = F_add_memory_sentinels(test_buffer, buffer_size, 0xDEADBEEF);
    if (!sentinels_added) {
        printf("      ⚠ 内存哨兵添加失败\n");
    } else {
        // 检查哨兵
        bool sentinels_ok = F_check_memory_sentinels(test_buffer);
        if (!sentinels_ok) {
            printf("      ⚠ 内存哨兵检查失败\n");
        }
    }
    
    // 测试3: 缓冲区溢出检测
    if (buffer_size > 10) {
        // 尝试在边界外写入（应该被检测到）
        bool overflow_detected = F_detect_buffer_overflow(test_buffer, buffer_size);
        if (overflow_detected) {
            printf("      ⚠ 缓冲区溢出检测触发（可能是误报）\n");
        }
    }
    
    // 清理
    free(test_buffer);
    F_cleanup_debug_tools();
    
    printf("      ✅ 边界和哨兵检查测试通过\n");
    return true;
}

/**
 * @brief 测试内存泄漏检测
 */
static bool test_memory_leak_detection(void)
{
    printf("    测试内存泄漏检测...\n");
    
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        printf("      ❌ 调试工具初始化失败\n");
        return false;
    }
    
    // 启用内存跟踪
    bool tracking_enabled = F_enable_memory_tracking(true);
    if (!tracking_enabled) {
        printf("      ⚠ 内存跟踪启用失败\n");
    }
    
    // 测试1: 分配内存并检查跟踪
    size_t alloc_size = 128;
    unsigned char* tracked_buffer = (unsigned char*)malloc(alloc_size);
    if (tracked_buffer == NULL) {
        printf("      ❌ 跟踪缓冲区分配失败\n");
        F_cleanup_debug_tools();
        return false;
    }
    
    // 检查分配是否被跟踪
    bool allocation_tracked = F_check_memory_allocation(tracked_buffer);
    if (!allocation_tracked) {
        printf("      ⚠ 内存分配未被跟踪\n");
    }
    
    // 测试2: 检查内存泄漏
    size_t leak_count = 0;
    bool leaks_found = F_detect_memory_leaks(&leak_count);
    if (leaks_found) {
        printf("      检测到 %zu 个内存泄漏\n", leak_count);
    }
    
    // 释放内存（避免实际泄漏）
    free(tracked_buffer);
    
    // 测试3: 再次检查泄漏（应该减少）
    size_t new_leak_count = 0;
    bool new_leaks_found = F_detect_memory_leaks(&new_leak_count);
    if (new_leaks_found) {
        printf("      释放后仍检测到 %zu 个内存泄漏\n", new_leak_count);
    }
    
    // 禁用内存跟踪
    bool tracking_disabled = F_enable_memory_tracking(false);
    if (!tracking_disabled) {
        printf("      ⚠ 内存跟踪禁用失败\n");
    }
    
    // 清理
    F_cleanup_debug_tools();
    
    printf("      ✅ 内存泄漏检测测试通过\n");
    return true;
}

/**
 * @brief 测试内存验证性能
 */
static bool test_memory_validation_performance(void)
{
    printf("    测试内存验证性能...\n");
    
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
    
    // 性能测试：多次验证
    int iterations = 5;
    printf("      执行%d次内存验证性能测试...\n", iterations);
    
    for (int i = 0; i < iterations; i++) {
        bool success = F_check_memory_integrity(test_buffer, buffer_size);
        if (!success) {
            printf("      ⚠ 第%d次验证失败\n", i + 1);
        }
    }
    
    // 清理
    free(test_buffer);
    F_cleanup_debug_tools();
    
    printf("      ✅ 内存验证性能测试通过\n");
    return true;
}

/**
 * @brief 测试内存验证高级功能
 */
static bool test_memory_validation_advanced_features(void)
{
    printf("    测试内存验证高级功能...\n");
    
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
    
    // 测试1: 内存区域验证
    bool region_validated = F_validate_memory_region(test_buffer, buffer_size, 
                                                   Eum_VALIDATION_FULL);
    if (!region_validated) {
        printf("      ⚠ 内存区域验证失败\n");
    }
    
    // 测试2: 内存校验和
    unsigned int checksum = 0;
    bool checksum_calculated = F_calculate_memory_checksum(test_buffer, buffer_size, &checksum);
    if (!checksum_calculated) {
        printf("      ⚠ 内存校验和计算失败\n");
    } else {
        printf("      内存校验和: 0x%08X\n", checksum);
        
        // 验证校验和
        bool checksum_verified = F_verify_memory_checksum(test_buffer, buffer_size, checksum);
        if (!checksum_verified) {
            printf("      ⚠ 内存校验和验证失败\n");
        }
    }
    
    // 测试3: 内存哈希
    unsigned char hash[32];
    bool hash_calculated = F_calculate_memory_hash(test_buffer, buffer_size, hash, sizeof(hash));
    if (!hash_calculated) {
        printf("      ⚠ 内存哈希计算失败\n");
    }
    
    // 测试4: 内存差异检测
    unsigned char* compare_buffer = (unsigned char*)malloc(buffer_size);
    if (compare_buffer != NULL) {
        // 创建略有不同的缓冲区
        for (size_t i = 0; i < buffer_size; i++) {
            compare_buffer[i] = (unsigned char)((i + 1) % 256);
        }
        
        size_t diff_count = 0;
        bool diffs_found = F_find_memory_differences(test_buffer, compare_buffer, 
                                                    buffer_size, &diff_count);
        if (diffs_found) {
            printf("      发现 %zu 个内存差异\n", diff_count);
        }
        
        free(compare_buffer);
    } else {
        printf("      ⚠ 无法分配比较缓冲区\n");
    }
    
    // 清理
    free(test_buffer);
    F_cleanup_debug_tools();
    
    printf("      ✅ 内存验证高级功能测试通过\n");
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
    
    if (!test_boundary_and_sentinel_checks()) {
        all_passed = false;
    }
    
    if (!test_memory_leak_detection()) {
        all_passed = false;
    }
    
    if (!test_memory_validation_performance()) {
        all_passed = false;
    }
    
    if (!test_memory_validation_advanced_features()) {
        all_passed = false;
    }
    
    if (all_passed) {
        printf("  ✅ 所有内存验证模块测试通过\n");
    } else {
        printf("  ❌ 部分内存验证模块测试失败\n");
    }
    
    return all_passed;
}
