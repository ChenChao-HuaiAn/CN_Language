/**
 * @file test_leak_detection.c
 * @brief 泄漏检测模块测试实现
 * 
 * 本文件实现了泄漏检测模块的测试函数。
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

#include "../../../../../src/infrastructure/memory/debug/leak_detection/CN_memory_leak_detection.h"

/**
 * @brief 测试泄漏检测器创建和销毁
 */
static bool test_leak_detector_creation(void)
{
    printf("    测试泄漏检测器创建和销毁...\n");
    
    // 创建泄漏检测器
    Stru_LeakDetectorContext_t* detector = F_create_leak_detector(true, 10);
    if (detector == NULL) {
        printf("      ❌ 创建泄漏检测器失败\n");
        return false;
    }
    
    // 销毁泄漏检测器
    F_destroy_leak_detector(detector);
    
    printf("      ✅ 泄漏检测器创建和销毁测试通过\n");
    return true;
}

/**
 * @brief 测试内存分配记录
 */
static bool test_allocation_recording(void)
{
    printf("    测试内存分配记录...\n");
    
    Stru_LeakDetectorContext_t* detector = F_create_leak_detector(false, 0);
    if (detector == NULL) {
        printf("      ❌ 创建泄漏检测器失败\n");
        return false;
    }
    
    // 记录内存分配
    void* ptr1 = malloc(100);
    F_record_allocation(detector, ptr1, 100, "test.c", 10, "test_function");
    
    void* ptr2 = malloc(200);
    F_record_allocation(detector, ptr2, 200, "test.c", 20, "test_function");
    
    // 记录内存释放
    F_record_deallocation(detector, ptr1);
    free(ptr1);
    
    // 检查泄漏（应该有一个泄漏）
    size_t leaks = F_check_leaks(detector);
    if (leaks != 1) {
        printf("      ❌ 泄漏检查失败，期望1个泄漏，实际%zu个\n", leaks);
        F_destroy_leak_detector(detector);
        free(ptr2);
        return false;
    }
    
    // 释放剩余内存
    F_record_deallocation(detector, ptr2);
    free(ptr2);
    
    // 再次检查泄漏（应该没有泄漏）
    leaks = F_check_leaks(detector);
    if (leaks != 0) {
        printf("      ❌ 泄漏检查失败，期望0个泄漏，实际%zu个\n", leaks);
        F_destroy_leak_detector(detector);
        return false;
    }
    
    F_destroy_leak_detector(detector);
    
    printf("      ✅ 内存分配记录测试通过\n");
    return true;
}

/**
 * @brief 测试调用栈跟踪
 */
static bool test_stack_trace(void)
{
    printf("    测试调用栈跟踪...\n");
    
    // 创建启用调用栈跟踪的检测器
    Stru_LeakDetectorContext_t* detector = F_create_leak_detector(true, 5);
    if (detector == NULL) {
        printf("      ❌ 创建泄漏检测器失败\n");
        return false;
    }
    
    // 记录带调用栈信息的分配
    void* ptr = malloc(50);
    F_record_allocation(detector, ptr, 50, "stack_test.c", 30, "stack_test_function");
    
    // 检查泄漏
    size_t leaks = F_check_leaks(detector);
    if (leaks != 1) {
        printf("      ❌ 调用栈跟踪泄漏检查失败\n");
        F_destroy_leak_detector(detector);
        free(ptr);
        return false;
    }
    
    // 清理
    F_record_deallocation(detector, ptr);
    free(ptr);
    F_destroy_leak_detector(detector);
    
    printf("      ✅ 调用栈跟踪测试通过\n");
    return true;
}

/**
 * @brief 运行所有泄漏检测测试
 */
bool test_leak_detection_all(void)
{
    printf("  运行泄漏检测模块测试:\n");
    printf("  --------------------\n");
    
    bool all_passed = true;
    
    if (!test_leak_detector_creation()) {
        all_passed = false;
    }
    
    if (!test_allocation_recording()) {
        all_passed = false;
    }
    
    if (!test_stack_trace()) {
        all_passed = false;
    }
    
    if (all_passed) {
        printf("  ✅ 所有泄漏检测测试通过\n");
    } else {
        printf("  ❌ 部分泄漏检测测试失败\n");
    }
    
    return all_passed;
}
