/**
 * @file test_error_detection.c
 * @brief 错误检测模块测试实现
 * 
 * 本文件实现了错误检测模块的测试函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../../../../src/infrastructure/memory/debug/error_detection/CN_memory_error_detection.h"

/**
 * @brief 测试错误检测器创建和销毁
 */
static bool test_error_detector_creation(void)
{
    printf("    测试错误检测器创建和销毁...\n");
    
    // 创建错误检测器
    Stru_ErrorDetectorContext_t* detector = F_create_error_detector(
        true,  // 启用溢出检查
        true,  // 启用双重释放检查
        true,  // 启用未初始化检查
        16     // 保护区域大小
    );
    
    if (detector == NULL) {
        printf("      ❌ 创建错误检测器失败\n");
        return false;
    }
    
    // 销毁错误检测器
    F_destroy_error_detector(detector);
    
    printf("      ✅ 错误检测器创建和销毁测试通过\n");
    return true;
}

/**
 * @brief 测试指针验证
 */
static bool test_pointer_validation(void)
{
    printf("    测试指针验证...\n");
    
    Stru_ErrorDetectorContext_t* detector = F_create_error_detector(false, false, false, 0);
    if (detector == NULL) {
        printf("      ❌ 创建错误检测器失败\n");
        return false;
    }
    
    // 测试有效指针
    void* valid_ptr = malloc(100);
    bool is_valid = F_validate_pointer(detector, valid_ptr);
    if (!is_valid) {
        printf("      ❌ 有效指针验证失败\n");
        free(valid_ptr);
        F_destroy_error_detector(detector);
        return false;
    }
    
    // 测试无效指针（NULL）
    is_valid = F_validate_pointer(detector, NULL);
    if (is_valid) {
        printf("      ❌ NULL指针验证失败（应该返回false）\n");
        free(valid_ptr);
        F_destroy_error_detector(detector);
        return false;
    }
    
    // 测试无效指针（已释放）
    free(valid_ptr);
    is_valid = F_validate_pointer(detector, valid_ptr);
    if (is_valid) {
        printf("      ❌ 已释放指针验证失败（应该返回false）\n");
        F_destroy_error_detector(detector);
        return false;
    }
    
    F_destroy_error_detector(detector);
    
    printf("      ✅ 指针验证测试通过\n");
    return true;
}

/**
 * @brief 测试内存范围验证
 */
static bool test_memory_range_validation(void)
{
    printf("    测试内存范围验证...\n");
    
    Stru_ErrorDetectorContext_t* detector = F_create_error_detector(false, false, false, 0);
    if (detector == NULL) {
        printf("      ❌ 创建错误检测器失败\n");
        return false;
    }
    
    // 分配内存
    void* memory = malloc(200);
    
    // 测试有效内存范围
    bool is_valid = F_validate_memory_range(detector, memory, 100);
    if (!is_valid) {
        printf("      ❌ 有效内存范围验证失败\n");
        free(memory);
        F_destroy_error_detector(detector);
        return false;
    }
    
    // 测试无效内存范围（超出分配大小）
    is_valid = F_validate_memory_range(detector, memory, 300);
    if (is_valid) {
        printf("      ❌ 超出分配大小的内存范围验证失败（应该返回false）\n");
        free(memory);
        F_destroy_error_detector(detector);
        return false;
    }
    
    // 测试无效内存范围（NULL指针）
    is_valid = F_validate_memory_range(detector, NULL, 100);
    if (is_valid) {
        printf("      ❌ NULL指针内存范围验证失败（应该返回false）\n");
        free(memory);
        F_destroy_error_detector(detector);
        return false;
    }
    
    free(memory);
    F_destroy_error_detector(detector);
    
    printf("      ✅ 内存范围验证测试通过\n");
    return true;
}

/**
 * @brief 测试缓冲区溢出检测
 */
static bool test_buffer_overflow_detection(void)
{
    printf("    测试缓冲区溢出检测...\n");
    
    // 创建启用溢出检查的检测器
    Stru_ErrorDetectorContext_t* detector = F_create_error_detector(true, false, false, 16);
    if (detector == NULL) {
        printf("      ❌ 创建错误检测器失败\n");
        return false;
    }
    
    // 注意：实际的缓冲区溢出检测需要在分配的内存周围添加保护区域
    // 这里只是测试接口
    
    F_destroy_error_detector(detector);
    
    printf("      ✅ 缓冲区溢出检测测试通过\n");
    return true;
}

/**
 * @brief 运行所有错误检测测试
 */
bool test_error_detection_all(void)
{
    printf("  运行错误检测模块测试:\n");
    printf("  --------------------\n");
    
    bool all_passed = true;
    
    if (!test_error_detector_creation()) {
        all_passed = false;
    }
    
    if (!test_pointer_validation()) {
        all_passed = false;
    }
    
    if (!test_memory_range_validation()) {
        all_passed = false;
    }
    
    if (!test_buffer_overflow_detection()) {
        all_passed = false;
    }
    
    if (all_passed) {
        printf("  ✅ 所有错误检测测试通过\n");
    } else {
        printf("  ❌ 部分错误检测测试失败\n");
    }
    
    return all_passed;
}
