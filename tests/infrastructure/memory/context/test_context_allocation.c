/**
 * @file test_context_allocation.c
 * @brief 内存上下文分配测试
 * 
 * 测试内存上下文模块的内存分配、重新分配、释放功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_context.h"
#include "../../../../src/infrastructure/memory/context/public/CN_memory_context.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* 测试函数声明 */
static bool test_context_allocate_basic(void);
static bool test_context_allocate_zero_size(void);
static bool test_context_allocate_large(void);
static bool test_context_allocate_alignment(void);
static bool test_context_allocate_multiple(void);
static bool test_context_deallocate_basic(void);
static bool test_context_deallocate_null(void);
static bool test_context_deallocate_multiple(void);
static bool test_context_reallocate_basic(void);
static bool test_context_reallocate_grow(void);
static bool test_context_reallocate_shrink(void);
static bool test_context_reallocate_null(void);
static bool test_context_get_allocated_size_basic(void);
static bool test_context_get_allocated_size_null(void);

/**
 * @brief 测试内存分配 - 基本功能
 */
static bool test_context_allocate_basic(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestAllocationContext", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 分配内存
    size_t size = 100;
    size_t alignment = 8;
    void* memory = ctx_if->allocate(ctx, size, alignment);
    
    TEST_ASSERT_NOT_NULL(memory);
    
    // 验证内存可写
    memset(memory, 0xAA, size);
    
    // 验证分配大小
    size_t allocated_size = ctx_if->get_allocated_size(ctx, memory);
    TEST_ASSERT(allocated_size >= size);
    
    // 清理
    ctx_if->deallocate(ctx, memory);
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试内存分配 - 零大小
 */
static bool test_context_allocate_zero_size(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestZeroAllocation", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 分配零大小内存
    void* memory = ctx_if->allocate(ctx, 0, 8);
    // 可能返回NULL或非NULL，取决于实现
    
    // 如果返回非NULL，应该可以安全释放
    if (memory != NULL) {
        ctx_if->deallocate(ctx, memory);
    }
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试内存分配 - 大内存
 */
static bool test_context_allocate_large(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestLargeAllocation", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 分配较大内存（1MB）
    size_t large_size = 1024 * 1024; // 1MB
    void* large_memory = ctx_if->allocate(ctx, large_size, 8);
    
    TEST_ASSERT_NOT_NULL(large_memory);
    
    // 验证内存可写
    memset(large_memory, 0x55, large_size);
    
    // 清理
    ctx_if->deallocate(ctx, large_memory);
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试内存分配 - 对齐要求
 */
static bool test_context_allocate_alignment(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestAlignment", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 测试不同对齐要求
    size_t sizes[] = {16, 32, 64, 128, 256};
    size_t alignments[] = {1, 2, 4, 8, 16, 32, 64, 128};
    
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        for (size_t j = 0; j < sizeof(alignments) / sizeof(alignments[0]); j++) {
            void* mem = ctx_if->allocate(ctx, sizes[i], alignments[j]);
            TEST_ASSERT_NOT_NULL(mem);
            
            // 验证对齐
            uintptr_t address = (uintptr_t)mem;
            TEST_ASSERT(address % alignments[j] == 0);
            
            // 使用内存
            memset(mem, i + j, sizes[i]);
            
            ctx_if->deallocate(ctx, mem);
        }
    }
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试内存分配 - 多次分配
 */
static bool test_context_allocate_multiple(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestMultipleAllocation", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 分配多个内存块
    const int num_allocations = 100;
    void** allocations = (void**)ctx_if->allocate(ctx, num_allocations * sizeof(void*), 8);
    TEST_ASSERT_NOT_NULL(allocations);
    
    for (int i = 0; i < num_allocations; i++) {
        size_t size = (i + 1) * 16; // 递增大小
        allocations[i] = ctx_if->allocate(ctx, size, 8);
        TEST_ASSERT_NOT_NULL(allocations[i]);
        
        // 使用内存
        memset(allocations[i], i, size);
    }
    
    // 验证统计信息
    size_t total_allocated = ctx_if->get_total_allocated(ctx);
    TEST_ASSERT(total_allocated > 0);
    
    // 释放所有内存
    for (int i = 0; i < num_allocations; i++) {
        ctx_if->deallocate(ctx, allocations[i]);
    }
    
    // 释放指针数组
    ctx_if->deallocate(ctx, allocations);
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试内存释放 - 基本功能
 */
static bool test_context_deallocate_basic(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestDeallocation", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 分配并释放内存
    void* memory = ctx_if->allocate(ctx, 100, 8);
    TEST_ASSERT_NOT_NULL(memory);
    
    // 释放内存
    ctx_if->deallocate(ctx, memory);
    
    // 再次分配（内存应该被重用）
    void* memory2 = ctx_if->allocate(ctx, 100, 8);
    TEST_ASSERT_NOT_NULL(memory2);
    
    // 清理
    ctx_if->deallocate(ctx, memory2);
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试内存释放 - NULL指针
 */
static bool test_context_deallocate_null(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestDeallocateNull", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 释放NULL指针应该安全
    ctx_if->deallocate(ctx, NULL);
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试内存释放 - 多次释放
 */
static bool test_context_deallocate_multiple(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestMultipleDeallocation", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 分配多个内存块
    void* mem1 = ctx_if->allocate(ctx, 50, 8);
    void* mem2 = ctx_if->allocate(ctx, 100, 16);
    void* mem3 = ctx_if->allocate(ctx, 150, 32);
    
    TEST_ASSERT_NOT_NULL(mem1);
    TEST_ASSERT_NOT_NULL(mem2);
    TEST_ASSERT_NOT_NULL(mem3);
    
    // 以不同顺序释放
    ctx_if->deallocate(ctx, mem2); // 先释放中间
    ctx_if->deallocate(ctx, mem3); // 再释放最后
    ctx_if->deallocate(ctx, mem1); // 最后释放第一个
    
    // 验证统计信息
    size_t total_allocated = ctx_if->get_total_allocated(ctx);
    TEST_ASSERT_EQUAL(0, total_allocated);
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试内存重新分配 - 基本功能
 */
static bool test_context_reallocate_basic(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestReallocation", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 初始分配
    size_t initial_size = 100;
    void* memory = ctx_if->allocate(ctx, initial_size, 8);
    TEST_ASSERT_NOT_NULL(memory);
    
    // 初始化数据
    memset(memory, 0xAA, initial_size);
    
    // 重新分配（相同大小）
    size_t same_size = 100;
    void* reallocated = ctx_if->reallocate(ctx, memory, same_size);
    TEST_ASSERT_NOT_NULL(reallocated);
    
    // 验证数据可能被保留（取决于实现）
    
    // 清理
    ctx_if->deallocate(ctx, reallocated);
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试内存重新分配 - 增长大小
 */
static bool test_context_reallocate_grow(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestReallocateGrow", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 初始分配
    size_t initial_size = 100;
    void* memory = ctx_if->allocate(ctx, initial_size, 8);
    TEST_ASSERT_NOT_NULL(memory);
    
    // 初始化数据
    const char* test_data = "Hello, World!";
    strcpy((char*)memory, test_data);
    
    // 增长大小
    size_t new_size = 200;
    void* grown_memory = ctx_if->reallocate(ctx, memory, new_size);
    TEST_ASSERT_NOT_NULL(grown_memory);
    
    // 验证旧数据被保留
    TEST_ASSERT_STRING_EQUAL(test_data, (char*)grown_memory);
    
    // 使用新空间
    strcat((char*)grown_memory, " Extended!");
    
    // 清理
    ctx_if->deallocate(ctx, grown_memory);
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试内存重新分配 - 缩小大小
 */
static bool test_context_reallocate_shrink(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestReallocateShrink", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 初始分配
    size_t initial_size = 200;
    void* memory = ctx_if->allocate(ctx, initial_size, 8);
    TEST_ASSERT_NOT_NULL(memory);
    
    // 初始化数据
    const char* test_data = "This is a test string that is longer than 50 characters.";
    strcpy((char*)memory, test_data);
    
    // 缩小大小
    size_t new_size = 50;
    void* shrunk_memory = ctx_if->reallocate(ctx, memory, new_size);
    TEST_ASSERT_NOT_NULL(shrunk_memory);
    
    // 验证数据被截断但前部分被保留
    // 注意：可能只保留前new_size字节
    
    // 清理
    ctx_if->deallocate(ctx, shrunk_memory);
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试内存重新分配 - NULL指针
 */
static bool test_context_reallocate_null(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestReallocateNull", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 重新分配NULL指针应该等同于分配
    size_t size = 100;
    void* memory = ctx_if->reallocate(ctx, NULL, size);
    TEST_ASSERT_NOT_NULL(memory);
    
    // 清理
    ctx_if->deallocate(ctx, memory);
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试获取分配大小 - 基本功能
 */
static bool test_context_get_allocated_size_basic(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestGetAllocatedSize", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 分配不同大小的内存
    size_t sizes[] = {16, 32, 64, 128, 256};
    
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        void* memory = ctx_if->allocate(ctx, sizes[i], 8);
        TEST_ASSERT_NOT_NULL(memory);
        
        // 获取分配大小
        size_t allocated_size = ctx_if->get_allocated_size(ctx, memory);
        TEST_ASSERT(allocated_size >= sizes[i]);
        
        ctx_if->deallocate(ctx, memory);
    }
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试获取分配大小 - NULL指针
 */
static bool test_context_get_allocated_size_null(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestGetAllocatedSizeNull", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 获取NULL指针的分配大小应该返回0
    size_t size = ctx_if->get_allocated_size(ctx, NULL);
    TEST_ASSERT_EQUAL(0, size);
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 运行所有内存分配测试
 * 
 * @return bool 是否所有测试都通过
 */
bool test_context_allocation_all(void)
{
    test_context_print_header("内存分配");
    
    TestCase tests[] = {
        {"内存分配 - 基本功能", test_context_allocate_basic, true},
        {"内存分配 - 零大小", test_context_allocate_zero_size, true},
        {"内存分配 - 大内存", test_context_allocate_large, true},
        {"内存分配 - 对齐要求", test_context_allocate_alignment, true},
        {"内存分配 - 多次分配", test_context_allocate_multiple, true},
        {"内存释放 - 基本功能", test_context_deallocate_basic, true},
        {"内存释放 - NULL指针", test_context_deallocate_null, true},
        {"内存释放 - 多次释放", test_context_deallocate_multiple, true},
        {"内存重新分配 - 基本功能", test_context_reallocate_basic, true},
        {"内存重新分配 - 增长大小", test_context_reallocate_grow, true},
        {"内存重新分配 - 缩小大小", test_context_reallocate_shrink, true},
        {"内存重新分配 - NULL指针", test_context_reallocate_null, true},
        {"获取分配大小 - 基本功能", test_context_get_allocated_size_basic, true},
        {"获取分配大小 - NULL指针", test_context_get_allocated_size_null, true},
    };
    
    int test_count = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, test_count, "内存分配");
}
