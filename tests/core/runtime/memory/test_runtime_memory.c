/**
 * @file test_runtime_memory.c
 * @brief 运行时内存管理模块单元测试
 
 * 本文件包含运行时内存管理模块的单元测试，测试内存管理的各项功能。
 * 遵循项目测试规范，每个测试功能在单独的文件中。
 * 
 * @author CN_Language测试团队
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../../../src/core/runtime/memory/CN_runtime_memory_interface.h"

// ============================================================================
// 测试辅助函数
// ============================================================================

/**
 * @brief 打印测试结果
 */
static void print_test_result(const char* test_name, bool passed)
{
    printf("  %-40s %s\n", test_name, passed ? "[PASS]" : "[FAIL]");
}

// ============================================================================
// 测试用例
// ============================================================================

/**
 * @brief 测试内存管理接口创建和销毁
 */
static bool test_memory_interface_create_destroy(void)
{
    printf("测试内存管理接口创建和销毁:\n");
    
    // 测试1: 创建内存管理接口
    Stru_RuntimeMemoryInterface_t* memory = F_create_runtime_memory_interface();
    bool test1_passed = (memory != NULL);
    print_test_result("创建内存管理接口", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 初始化内存管理接口
    bool init_result = memory->initialize(memory, 1024 * 1024, false); // 1MB堆，无GC
    bool test2_passed = init_result;
    print_test_result("初始化内存管理接口", test2_passed);
    
    // 测试3: 销毁内存管理接口
    memory->destroy(memory);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁内存管理接口", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试对象内存分配
 */
static bool test_memory_allocate_object(void)
{
    printf("\n测试对象内存分配:\n");
    
    // 创建内存管理接口
    Stru_RuntimeMemoryInterface_t* memory = F_create_runtime_memory_interface();
    if (memory == NULL) {
        return false;
    }
    
    memory->initialize(memory, 1024 * 1024, false);
    
    // 测试1: 分配小对象
    void* small_object = memory->allocate_object(memory, 64, 1);
    bool test1_passed = (small_object != NULL);
    print_test_result("分配小对象（64字节）", test1_passed);
    
    // 测试2: 分配大对象
    void* large_object = memory->allocate_object(memory, 1024 * 1024, 2);
    bool test2_passed = (large_object != NULL);
    print_test_result("分配大对象（1MB）", test2_passed);
    
    // 测试3: 分配零字节对象
    void* zero_object = memory->allocate_object(memory, 0, 3);
    bool test3_passed = (zero_object == NULL); // 应该失败
    print_test_result("分配零字节对象（应该失败）", test3_passed);
    
    // 测试4: 释放对象
    if (small_object != NULL) {
        memory->free_object(memory, small_object);
    }
    if (large_object != NULL) {
        memory->free_object(memory, large_object);
    }
    bool test4_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("释放对象", test4_passed);
    
    // 清理
    memory->destroy(memory);
    
    return test1_passed && test2_passed && test3_passed && test4_passed;
}

/**
 * @brief 测试数组内存分配
 */
static bool test_memory_allocate_array(void)
{
    printf("\n测试数组内存分配:\n");
    
    // 创建内存管理接口
    Stru_RuntimeMemoryInterface_t* memory = F_create_runtime_memory_interface();
    if (memory == NULL) {
        return false;
    }
    
    memory->initialize(memory, 1024 * 1024, false);
    
    // 测试1: 分配小数组
    void* small_array = memory->allocate_array(memory, sizeof(int), 10, 1);
    bool test1_passed = (small_array != NULL);
    print_test_result("分配小数组（10个int）", test1_passed);
    
    // 测试2: 分配大数组
    void* large_array = memory->allocate_array(memory, sizeof(double), 1000, 2);
    bool test2_passed = (large_array != NULL);
    print_test_result("分配大数组（1000个double）", test2_passed);
    
    // 测试3: 分配零元素数组
    void* zero_array = memory->allocate_array(memory, sizeof(int), 0, 3);
    bool test3_passed = (zero_array == NULL); // 应该失败
    print_test_result("分配零元素数组（应该失败）", test3_passed);
    
    // 测试4: 释放数组
    if (small_array != NULL) {
        memory->free_object(memory, small_array);
    }
    if (large_array != NULL) {
        memory->free_object(memory, large_array);
    }
    bool test4_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("释放数组", test4_passed);
    
    // 清理
    memory->destroy(memory);
    
    return test1_passed && test2_passed && test3_passed && test4_passed;
}

/**
 * @brief 测试内存统计
 */
static bool test_memory_statistics(void)
{
    printf("\n测试内存统计:\n");
    
    // 创建内存管理接口
    Stru_RuntimeMemoryInterface_t* memory = F_create_runtime_memory_interface();
    if (memory == NULL) {
        return false;
    }
    
    memory->initialize(memory, 1024 * 1024, false);
    
    // 获取初始统计
    size_t initial_total_heap = 0;
    size_t initial_used_heap = 0;
    size_t initial_object_count = 0;
    size_t initial_gc_count = 0;
    
    memory->get_memory_stats(memory, &initial_total_heap, &initial_used_heap,
                            &initial_object_count, &initial_gc_count);
    
    bool test1_passed = (initial_total_heap == 1024 * 1024) && 
                       (initial_used_heap == 0) && 
                       (initial_object_count == 0) && 
                       (initial_gc_count == 0);
    print_test_result("初始内存统计正确", test1_passed);
    
    // 分配一些对象
    void* object1 = memory->allocate_object(memory, 100, 1);
    void* object2 = memory->allocate_object(memory, 200, 2);
    void* object3 = memory->allocate_object(memory, 300, 3);
    
    // 获取分配后统计
    size_t after_alloc_total_heap = 0;
    size_t after_alloc_used_heap = 0;
    size_t after_alloc_object_count = 0;
    size_t after_alloc_gc_count = 0;
    
    memory->get_memory_stats(memory, &after_alloc_total_heap, &after_alloc_used_heap,
                            &after_alloc_object_count, &after_alloc_gc_count);
    
    bool test2_passed = (after_alloc_object_count == 3);
    print_test_result("分配后对象数量为3", test2_passed);
    
    // 释放一个对象
    if (object2 != NULL) {
        memory->free_object(memory, object2);
    }
    
    // 获取释放后统计
    size_t after_free_total_heap = 0;
    size_t after_free_used_heap = 0;
    size_t after_free_object_count = 0;
    size_t after_free_gc_count = 0;
    
    memory->get_memory_stats(memory, &after_free_total_heap, &after_free_used_heap,
                            &after_free_object_count, &after_free_gc_count);
    
    bool test3_passed = (after_free_object_count == 2);
    print_test_result("释放后对象数量为2", test3_passed);
    
    // 清理剩余对象
    if (object1 != NULL) {
        memory->free_object(memory, object1);
    }
    if (object3 != NULL) {
        memory->free_object(memory, object3);
    }
    
    // 清理
    memory->destroy(memory);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试内存重置
 */
static bool test_memory_reset(void)
{
    printf("\n测试内存重置:\n");
    
    // 创建内存管理接口
    Stru_RuntimeMemoryInterface_t* memory = F_create_runtime_memory_interface();
    if (memory == NULL) {
        return false;
    }
    
    memory->initialize(memory, 1024 * 1024, false);
    
    // 分配一些对象
    void* object1 = memory->allocate_object(memory, 100, 1);
    void* object2 = memory->allocate_object(memory, 200, 2);
    
    // 使用变量以避免未使用警告
    (void)object1;
    (void)object2;
    
    // 获取重置前统计
    size_t before_reset_object_count = 0;
    memory->get_memory_stats(memory, NULL, NULL, &before_reset_object_count, NULL);
    
    bool test1_passed = (before_reset_object_count == 2);
    print_test_result("重置前有2个对象", test1_passed);
    
    // 重置内存管理器
    memory->reset(memory);
    
    // 获取重置后统计
    size_t after_reset_object_count = 0;
    memory->get_memory_stats(memory, NULL, NULL, &after_reset_object_count, NULL);
    
    bool test2_passed = (after_reset_object_count == 0);
    print_test_result("重置后对象数量为0", test2_passed);
    
    // 清理
    memory->destroy(memory);
    
    return test1_passed && test2_passed;
}

// ============================================================================
// 主测试函数
// ============================================================================

/**
 * @brief 运行所有内存管理测试
 */
int main(void)
{
    printf("========================================\n");
    printf("运行时内存管理模块单元测试\n");
    printf("========================================\n\n");
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // 运行所有测试用例
    bool (*test_functions[])(void) = {
        test_memory_interface_create_destroy,
        test_memory_allocate_object,
        test_memory_allocate_array,
        test_memory_statistics,
        test_memory_reset
    };
    
    const char* test_names[] = {
        "内存管理接口创建和销毁",
        "对象内存分配",
        "数组内存分配",
        "内存统计",
        "内存重置"
    };
    
    size_t num_tests = sizeof(test_functions) / sizeof(test_functions[0]);
    
    for (size_t i = 0; i < num_tests; i++) {
        printf("\n测试用例: %s\n", test_names[i]);
        printf("----------------------------------------\n");
        
        bool result = test_functions[i]();
        total_tests++;
        if (result) {
            passed_tests++;
        }
        
        printf("结果: %s\n\n", result ? "通过" : "失败");
    }
    
    // 打印测试摘要
    printf("========================================\n");
    printf("测试摘要\n");
    printf("========================================\n");
    printf("总测试用例: %d\n", total_tests);
    printf("通过: %d\n", passed_tests);
    printf("失败: %d\n", total_tests - passed_tests);
    printf("通过率: %.1f%%\n", total_tests > 0 ? (passed_tests * 100.0 / total_tests) : 0.0);
    
    return (passed_tests == total_tests) ? 0 : 1;
}
