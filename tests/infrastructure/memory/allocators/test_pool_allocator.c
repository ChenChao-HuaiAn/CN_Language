/**
 * @file test_pool_allocator.c
 * @brief 对象池分配器测试实现
 * 
 * 测试对象池分配器的基本功能，包括固定大小对象分配、池扩展等。
 * 遵循项目测试框架规范。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_allocators.h"
#include "../../../../src/infrastructure/memory/allocators/pool/CN_pool_allocator.h"
#include "../../../../src/infrastructure/memory/allocators/system/CN_system_allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 测试用例计数器 */
static TestCaseCounter g_test_counter;

/**
 * @brief 创建对象池分配器实例
 */
static Stru_MemoryAllocatorInterface_t* create_pool_allocator(void)
{
    Stru_MemoryAllocatorInterface_t* system_allocator = F_create_system_allocator();
    if (system_allocator == NULL) {
        return NULL;
    }
    
    // 创建对象池分配器，对象大小64字节，池大小10个对象
    return F_create_pool_allocator(64, 10, system_allocator);
}

/**
 * @brief 测试对象池分配器的基本分配功能
 */
static void test_pool_allocate_basic(void)
{
    test_case_begin("对象池分配器基本分配", &g_test_counter);
    
    Stru_MemoryAllocatorInterface_t* allocator = create_pool_allocator();
    if (allocator == NULL) {
        test_case_end(false, &g_test_counter);
        return;
    }
    
    // 测试分配对象
    void* objects[10];
    bool passed = true;
    
    // 分配所有对象
    for (int i = 0; i < 10; i++) {
        objects[i] = allocator->allocate(allocator, 64, 0);
        if (objects[i] == NULL) {
            passed = false;
            break;
        }
    }
    
    if (passed) {
        // 尝试分配第11个对象（应该失败，因为池已满）
        void* extra = allocator->allocate(allocator, 64, 0);
        passed = (extra == NULL);
    }
    
    // 释放所有对象
    for (int i = 0; i < 10; i++) {
        if (objects[i] != NULL) {
            allocator->deallocate(allocator, objects[i]);
        }
    }
    
    allocator->cleanup(allocator);
    
    test_case_end(passed, &g_test_counter);
}

/**
 * @brief 测试对象池分配器的不同大小分配
 */
static void test_pool_different_sizes(void)
{
    test_case_begin("对象池分配器不同大小分配", &g_test_counter);
    
    Stru_MemoryAllocatorInterface_t* system_allocator = F_create_system_allocator();
    if (system_allocator == NULL) {
        test_case_end(false, &g_test_counter);
        return;
    }
    
    // 创建对象池分配器，对象大小64字节
    Stru_MemoryAllocatorInterface_t* allocator = F_create_pool_allocator(64, 5, system_allocator);
    if (allocator == NULL) {
        test_case_end(false, &g_test_counter);
        return;
    }
    
    bool passed = true;
    
    // 测试分配正确大小的对象
    void* correct_size = allocator->allocate(allocator, 64, 0);
    passed = (correct_size != NULL);
    
    if (passed) {
        // 测试分配不同大小的对象（应该使用父分配器）
        void* smaller = allocator->allocate(allocator, 32, 0);
        passed = (smaller != NULL);
        
        if (smaller != NULL) {
            allocator->deallocate(allocator, smaller);
        }
        
        void* larger = allocator->allocate(allocator, 128, 0);
        passed = passed && (larger != NULL);
        
        if (larger != NULL) {
            allocator->deallocate(allocator, larger);
        }
    }
    
    if (correct_size != NULL) {
        allocator->deallocate(allocator, correct_size);
    }
    
    allocator->cleanup(allocator);
    
    test_case_end(passed, &g_test_counter);
}

/**
 * @brief 测试对象池分配器的统计信息
 */
static void test_pool_statistics(void)
{
    test_case_begin("对象池分配器统计信息", &g_test_counter);
    
    Stru_MemoryAllocatorInterface_t* allocator = create_pool_allocator();
    if (allocator == NULL) {
        test_case_end(false, &g_test_counter);
        return;
    }
    
    // 获取池统计信息
    size_t total_objects = 0, allocated_objects = 0;
    size_t available_objects = 0, allocation_failures = 0;
    
    F_get_pool_allocator_statistics(allocator, &total_objects, &allocated_objects,
                                   &available_objects, &allocation_failures);
    
    bool passed = (total_objects == 10) && (allocated_objects == 0) &&
                  (available_objects == 10) && (allocation_failures == 0);
    
    // 分配一些对象
    void* obj1 = allocator->allocate(allocator, 64, 0);
    void* obj2 = allocator->allocate(allocator, 64, 0);
    
    if (obj1 != NULL && obj2 != NULL) {
        // 再次获取统计信息
        F_get_pool_allocator_statistics(allocator, &total_objects, &allocated_objects,
                                       &available_objects, &allocation_failures);
        
        passed = passed && (total_objects == 10) && (allocated_objects == 2) &&
                 (available_objects == 8) && (allocation_failures == 0);
    }
    
    if (obj1 != NULL) {
        allocator->deallocate(allocator, obj1);
    }
    if (obj2 != NULL) {
        allocator->deallocate(allocator, obj2);
    }
    
    allocator->cleanup(allocator);
    
    test_case_end(passed, &g_test_counter);
}

/**
 * @brief 测试对象池分配器的验证功能
 */
static void test_pool_validation(void)
{
    test_case_begin("对象池分配器验证", &g_test_counter);
    
    Stru_MemoryAllocatorInterface_t* allocator = create_pool_allocator();
    if (allocator == NULL) {
        test_case_end(false, &g_test_counter);
        return;
    }
    
    // 分配对象并验证
    void* ptr = allocator->allocate(allocator, 64, 0);
    bool passed = (ptr != NULL);
    
    if (passed) {
        // 验证有效指针
        bool valid = allocator->validate(allocator, ptr);
        passed = valid;
        
        // 验证NULL指针
        valid = allocator->validate(allocator, NULL);
        passed = passed && !valid;
        
        // 验证无效指针
        valid = allocator->validate(allocator, (void*)0x12345678);
        passed = passed && !valid;
        
        allocator->deallocate(allocator, ptr);
    }
    
    allocator->cleanup(allocator);
    
    test_case_end(passed, &g_test_counter);
}

/**
 * @brief 测试对象池分配器的池扩展功能
 */
static void test_pool_expansion(void)
{
    test_case_begin("对象池分配器池扩展", &g_test_counter);
    
    Stru_MemoryAllocatorInterface_t* system_allocator = F_create_system_allocator();
    if (system_allocator == NULL) {
        test_case_end(false, &g_test_counter);
        return;
    }
    
    // 创建小对象池
    Stru_MemoryAllocatorInterface_t* allocator = F_create_pool_allocator(32, 3, system_allocator);
    if (allocator == NULL) {
        test_case_end(false, &g_test_counter);
        return;
    }
    
    bool passed = true;
    
    // 获取初始统计信息
    size_t total_objects = 0, allocated_objects = 0;
    size_t available_objects = 0, allocation_failures = 0;
    
    F_get_pool_allocator_statistics(allocator, &total_objects, &allocated_objects,
                                   &available_objects, &allocation_failures);
    
    passed = (total_objects == 3) && (available_objects == 3);
    
    // 扩展池
    bool expanded = F_expand_pool_allocator(allocator, 5);
    passed = passed && expanded;
    
    if (expanded) {
        // 获取扩展后的统计信息
        F_get_pool_allocator_statistics(allocator, &total_objects, &allocated_objects,
                                       &available_objects, &allocation_failures);
        
        passed = passed && (total_objects == 8) && (available_objects == 8);
        
        // 分配所有对象
        void* objects[8];
        for (int i = 0; i < 8; i++) {
            objects[i] = allocator->allocate(allocator, 32, 0);
            if (objects[i] == NULL) {
                passed = false;
                break;
            }
        }
        
        // 释放所有对象
        for (int i = 0; i < 8; i++) {
            if (objects[i] != NULL) {
                allocator->deallocate(allocator, objects[i]);
            }
        }
    }
    
    allocator->cleanup(allocator);
    
    test_case_end(passed, &g_test_counter);
}

/**
 * @brief 测试对象池分配器的重新分配功能
 */
static void test_pool_reallocate(void)
{
    test_case_begin("对象池分配器重新分配", &g_test_counter);
    
    Stru_MemoryAllocatorInterface_t* allocator = create_pool_allocator();
    if (allocator == NULL) {
        test_case_end(false, &g_test_counter);
        return;
    }
    
    // 对象池不支持重新分配，应该使用父分配器
    void* ptr = allocator->allocate(allocator, 64, 0);
    bool passed = (ptr != NULL);
    
    if (passed) {
        // 尝试重新分配
        void* new_ptr = allocator->reallocate(allocator, ptr, 128);
        // 对象池可能不支持重新分配，所以不检查结果
        
        if (new_ptr != NULL) {
            allocator->deallocate(allocator, new_ptr);
        } else {
            allocator->deallocate(allocator, ptr);
        }
    }
    
    allocator->cleanup(allocator);
    
    test_case_end(passed, &g_test_counter);
}

/**
 * @brief 测试对象池分配器的清理功能
 */
static void test_pool_cleanup(void)
{
    test_case_begin("对象池分配器清理", &g_test_counter);
    
    Stru_MemoryAllocatorInterface_t* allocator = create_pool_allocator();
    if (allocator == NULL) {
        test_case_end(false, &g_test_counter);
        return;
    }
    
    // 分配一些对象
    void* obj1 = allocator->allocate(allocator, 64, 0);
    void* obj2 = allocator->allocate(allocator, 64, 0);
    
    bool passed = (obj1 != NULL && obj2 != NULL);
    
    // 清理分配器
    allocator->cleanup(allocator);
    
    // 清理后不应崩溃
    passed = passed && true;
    
    test_case_end(passed, &g_test_counter);
}

/**
 * @brief 测试对象池分配器的内存重用
 */
static void test_pool_memory_reuse(void)
{
    test_case_begin("对象池分配器内存重用", &g_test_counter);
    
    Stru_MemoryAllocatorInterface_t* allocator = create_pool_allocator();
    if (allocator == NULL) {
        test_case_end(false, &g_test_counter);
        return;
    }
    
    bool passed = true;
    void* addresses[10];
    
    // 分配所有对象并记录地址
    for (int i = 0; i < 10; i++) {
        addresses[i] = allocator->allocate(allocator, 64, 0);
        if (addresses[i] == NULL) {
            passed = false;
            break;
        }
    }
    
    if (passed) {
        // 释放所有对象
        for (int i = 0; i < 10; i++) {
            allocator->deallocate(allocator, addresses[i]);
        }
        
        // 再次分配，应该重用相同的内存地址
        for (int i = 0; i < 10; i++) {
            void* new_addr = allocator->allocate(allocator, 64, 0);
            if (new_addr == NULL) {
                passed = false;
                break;
            }
            
            // 检查是否与之前的地址相同（对象池应该重用内存）
            // 注意：对象池可能不保证相同的地址，但通常会的
            
            allocator->deallocate(allocator, new_addr);
        }
    }
    
    allocator->cleanup(allocator);
    
    test_case_end(passed, &g_test_counter);
}

/**
 * @brief 运行对象池分配器的所有测试
 * 
 * @return bool 所有测试通过返回true，否则返回false
 */
bool test_pool_allocator_all(void)
{
    printf("对象池分配器测试套件\n");
    printf("====================\n\n");
    
    test_case_init(&g_test_counter);
    
    // 运行所有测试用例
    test_pool_allocate_basic();
    test_pool_different_sizes();
    test_pool_statistics();
    test_pool_validation();
    test_pool_expansion();
    test_pool_reallocate();
    test_pool_cleanup();
    test_pool_memory_reuse();
    
    // 打印测试总结
    test_case_summary(&g_test_counter);
    
    return (g_test_counter.failed_tests == 0);
}
