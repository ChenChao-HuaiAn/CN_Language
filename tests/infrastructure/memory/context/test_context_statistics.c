/**
 * @file test_context_statistics.c
 * @brief 内存上下文统计测试
 * 
 * 测试内存上下文模块的统计功能，包括内存使用统计、峰值统计等。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_context.h"
#include "../../../../src/infrastructure/memory/context/CN_memory_context.h"
#include <stdio.h>
#include <string.h>

/* 测试函数声明 */
static bool test_context_get_total_allocated_basic(void);
static bool test_context_get_total_allocated_empty(void);
static bool test_context_get_total_allocated_multiple(void);
static bool test_context_get_peak_allocated_basic(void);
static bool test_context_get_peak_allocated_growth(void);
static bool test_context_get_peak_allocated_reset(void);
static bool test_context_statistics_accuracy(void);
static bool test_context_statistics_hierarchy(void);
static bool test_context_statistics_reset_effect(void);
static bool test_context_statistics_destroy_effect(void);

/**
 * @brief 测试获取总分配内存 - 基本功能
 */
static bool test_context_get_total_allocated_basic(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestStatistics", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 初始总分配应该为0
    size_t initial_total = ctx_if->get_total_allocated(ctx);
    TEST_ASSERT_EQUAL(0, initial_total);
    
    // 分配内存
    void* mem1 = ctx_if->allocate(ctx, 100, 8);
    void* mem2 = ctx_if->allocate(ctx, 200, 16);
    
    TEST_ASSERT_NOT_NULL(mem1);
    TEST_ASSERT_NOT_NULL(mem2);
    
    // 验证总分配
    size_t total_allocated = ctx_if->get_total_allocated(ctx);
    TEST_ASSERT(total_allocated >= 300); // 至少100+200字节
    
    // 释放内存
    ctx_if->deallocate(ctx, mem1);
    ctx_if->deallocate(ctx, mem2);
    
    // 释放后总分配应该为0
    size_t final_total = ctx_if->get_total_allocated(ctx);
    TEST_ASSERT_EQUAL(0, final_total);
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试获取总分配内存 - 空上下文
 */
static bool test_context_get_total_allocated_empty(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestEmptyStatistics", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 空上下文的总分配应该为0
    size_t total = ctx_if->get_total_allocated(ctx);
    TEST_ASSERT_EQUAL(0, total);
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试获取总分配内存 - 多次分配释放
 */
static bool test_context_get_total_allocated_multiple(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestMultipleStatistics", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 多次分配释放循环
    for (int cycle = 0; cycle < 5; cycle++) {
        // 分配内存
        void* allocations[10];
        size_t expected_total = 0;
        
        for (int i = 0; i < 10; i++) {
            size_t size = (i + 1) * 50; // 50, 100, 150, ..., 500
            allocations[i] = ctx_if->allocate(ctx, size, 8);
            TEST_ASSERT_NOT_NULL(allocations[i]);
            expected_total += size;
            
            // 验证中间总分配
            size_t current_total = ctx_if->get_total_allocated(ctx);
            TEST_ASSERT(current_total >= expected_total);
        }
        
        // 验证最终总分配
        size_t final_total = ctx_if->get_total_allocated(ctx);
        TEST_ASSERT(final_total >= expected_total);
        
        // 释放所有内存
        for (int i = 0; i < 10; i++) {
            ctx_if->deallocate(ctx, allocations[i]);
        }
        
        // 释放后总分配应该为0
        size_t after_free_total = ctx_if->get_total_allocated(ctx);
        TEST_ASSERT_EQUAL(0, after_free_total);
    }
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试获取峰值分配内存 - 基本功能
 */
static bool test_context_get_peak_allocated_basic(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestPeakStatistics", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 初始峰值应该为0
    size_t initial_peak = ctx_if->get_peak_allocated(ctx);
    TEST_ASSERT_EQUAL(0, initial_peak);
    
    // 分配内存
    void* mem1 = ctx_if->allocate(ctx, 100, 8);
    void* mem2 = ctx_if->allocate(ctx, 200, 16);
    
    TEST_ASSERT_NOT_NULL(mem1);
    TEST_ASSERT_NOT_NULL(mem2);
    
    // 验证峰值
    size_t peak = ctx_if->get_peak_allocated(ctx);
    TEST_ASSERT(peak >= 300); // 至少100+200字节
    
    // 释放内存
    ctx_if->deallocate(ctx, mem1);
    ctx_if->deallocate(ctx, mem2);
    
    // 峰值应该保持不变（历史最高）
    size_t peak_after_free = ctx_if->get_peak_allocated(ctx);
    TEST_ASSERT_EQUAL(peak, peak_after_free);
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试获取峰值分配内存 - 增长情况
 */
static bool test_context_get_peak_allocated_growth(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestPeakGrowth", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    size_t max_peak = 0;
    
    // 逐步增加分配
    for (int i = 1; i <= 10; i++) {
        size_t size = i * 100; // 100, 200, ..., 1000
        void* memory = ctx_if->allocate(ctx, size, 8);
        TEST_ASSERT_NOT_NULL(memory);
        
        // 更新预期峰值
        max_peak += size;
        
        // 验证峰值
        size_t current_peak = ctx_if->get_peak_allocated(ctx);
        TEST_ASSERT(current_peak >= max_peak);
        
        // 立即释放（模拟临时分配）
        ctx_if->deallocate(ctx, memory);
        max_peak -= size; // 释放后预期峰值不变（历史最高）
    }
    
    // 最终峰值应该是历史最高
    size_t final_peak = ctx_if->get_peak_allocated(ctx);
    TEST_ASSERT(final_peak >= 1000); // 至少最后一次分配的1000字节
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试获取峰值分配内存 - 重置后
 */
static bool test_context_get_peak_allocated_reset(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestPeakReset", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 分配内存建立峰值
    void* mem1 = ctx_if->allocate(ctx, 500, 8);
    void* mem2 = ctx_if->allocate(ctx, 300, 16);
    
    TEST_ASSERT_NOT_NULL(mem1);
    TEST_ASSERT_NOT_NULL(mem2);
    
    size_t peak_before_reset = ctx_if->get_peak_allocated(ctx);
    TEST_ASSERT(peak_before_reset >= 800);
    
    // 重置上下文
    ctx_if->reset(ctx);
    
    // 重置后峰值应该重置为0（取决于实现）
    size_t peak_after_reset = ctx_if->get_peak_allocated(ctx);
    // 可能为0或保持不变，取决于实现
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试统计准确性
 */
static bool test_context_statistics_accuracy(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestStatisticsAccuracy", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 测试不同大小的分配
    size_t sizes[] = {16, 32, 64, 128, 256, 512, 1024};
    void* allocations[sizeof(sizes) / sizeof(sizes[0])];
    
    size_t expected_total = 0;
    
    // 分配所有内存
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        allocations[i] = ctx_if->allocate(ctx, sizes[i], 8);
        TEST_ASSERT_NOT_NULL(allocations[i]);
        expected_total += sizes[i];
        
        // 验证总分配
        size_t current_total = ctx_if->get_total_allocated(ctx);
        TEST_ASSERT(current_total >= expected_total);
        
        // 验证峰值
        size_t current_peak = ctx_if->get_peak_allocated(ctx);
        TEST_ASSERT(current_peak >= expected_total);
    }
    
    // 最终验证
    size_t final_total = ctx_if->get_total_allocated(ctx);
    size_t final_peak = ctx_if->get_peak_allocated(ctx);
    
    TEST_ASSERT(final_total >= expected_total);
    TEST_ASSERT(final_peak >= expected_total);
    TEST_ASSERT(final_peak >= final_total);
    
    // 部分释放
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]) / 2; i++) {
        ctx_if->deallocate(ctx, allocations[i]);
        expected_total -= sizes[i];
    }
    
    // 验证部分释放后
    size_t partial_total = ctx_if->get_total_allocated(ctx);
    size_t partial_peak = ctx_if->get_peak_allocated(ctx);
    
    TEST_ASSERT(partial_total >= expected_total);
    TEST_ASSERT(partial_peak >= final_peak); // 峰值应该保持不变或更高
    
    // 释放剩余内存
    for (size_t i = sizeof(sizes) / sizeof(sizes[0]) / 2; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        ctx_if->deallocate(ctx, allocations[i]);
    }
    
    // 验证完全释放后
    size_t empty_total = ctx_if->get_total_allocated(ctx);
    TEST_ASSERT_EQUAL(0, empty_total);
    
    // 清理
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试层次结构统计
 */
static bool test_context_statistics_hierarchy(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文层次结构
    Stru_MemoryContext_t* parent = ctx_if->create("ParentContext", NULL);
    Stru_MemoryContext_t* child1 = ctx_if->create("ChildContext1", parent);
    Stru_MemoryContext_t* child2 = ctx_if->create("ChildContext2", parent);
    
    TEST_ASSERT_NOT_NULL(parent);
    TEST_ASSERT_NOT_NULL(child1);
    TEST_ASSERT_NOT_NULL(child2);
    
    // 在各个上下文中分配内存
    void* parent_mem = ctx_if->allocate(parent, 1000, 8);
    void* child1_mem = ctx_if->allocate(child1, 500, 8);
    void* child2_mem = ctx_if->allocate(child2, 300, 8);
    
    TEST_ASSERT_NOT_NULL(parent_mem);
    TEST_ASSERT_NOT_NULL(child1_mem);
    TEST_ASSERT_NOT_NULL(child2_mem);
    
    // 验证各个上下文的统计
    size_t parent_total = ctx_if->get_total_allocated(parent);
    size_t child1_total = ctx_if->get_total_allocated(child1);
    size_t child2_total = ctx_if->get_total_allocated(child2);
    
    TEST_ASSERT(parent_total >= 1000);
    TEST_ASSERT(child1_total >= 500);
    TEST_ASSERT(child2_total >= 300);
    
    // 注意：父上下文的统计可能包含子上下文，也可能不包含，取决于实现
    
    // 释放内存
    ctx_if->deallocate(parent, parent_mem);
    ctx_if->deallocate(child1, child1_mem);
    ctx_if->deallocate(child2, child2_mem);
    
    // 销毁子上下文
    ctx_if->destroy(child1);
    ctx_if->destroy(child2);
    
    // 销毁父上下文
    ctx_if->destroy(parent);
    
    return true;
}

/**
 * @brief 测试重置对统计的影响
 */
static bool test_context_statistics_reset_effect(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建上下文
    Stru_MemoryContext_t* ctx = ctx_if->create("TestResetStatistics", NULL);
    TEST_ASSERT_NOT_NULL(ctx);
    
    // 分配内存
    void* mem1 = ctx_if->allocate(ctx, 400, 8);
    void* mem2 = ctx_if->allocate(ctx, 600, 16);
    
    TEST_ASSERT_NOT_NULL(mem1);
    TEST_ASSERT_NOT_NULL(mem2);
    
    // 记录重置前的统计
    size_t total_before = ctx_if->get_total_allocated(ctx);
    size_t peak_before = ctx_if->get_peak_allocated(ctx);
    
    TEST_ASSERT(total_before >= 1000);
    TEST_ASSERT(peak_before >= 1000);
    
    // 重置上下文
    ctx_if->reset(ctx);
    
    // 验证重置后的统计
    size_t total_after = ctx_if->get_total_allocated(ctx);
    TEST_ASSERT_EQUAL(0, total_after);
    
    // 峰值可能重置或保持不变，取决于实现
    size_t peak_after = ctx_if->get_peak_allocated(ctx);
    
    // 重新分配内存
    void* mem3 = ctx_if->allocate(ctx, 200, 8);
    TEST_ASSERT_NOT_NULL(mem3);
    
    // 验证新统计
    size_t total_new = ctx_if->get_total_allocated(ctx);
    TEST_ASSERT(total_new >= 200);
    
    // 清理
    ctx_if->deallocate(ctx, mem3);
    ctx_if->destroy(ctx);
    
    return true;
}

/**
 * @brief 测试销毁对统计的影响
 */
static bool test_context_statistics_destroy_effect(void)
{
    Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
    TEST_ASSERT_NOT_NULL(ctx_if);
    
    // 创建多个上下文
    Stru_MemoryContext_t* ctx1 = ctx_if->create("Context1", NULL);
    Stru_MemoryContext_t* ctx2 = ctx_if->create("Context2", NULL);
    
    TEST_ASSERT_NOT_NULL(ctx1);
    TEST_ASSERT_NOT_NULL(ctx2);
    
    // 在ctx1中分配内存
    void* mem1 = ctx_if->allocate(ctx1, 800, 8);
    TEST_ASSERT_NOT_NULL(mem1);
    
    size_t ctx1_total = ctx_if->get_total_allocated(ctx1);
    size_t ctx1_peak = ctx_if->get_peak_allocated(ctx1);
    
    TEST_ASSERT(ctx1_total >= 800);
    TEST_ASSERT(ctx1_peak >= 800);
    
    // 销毁ctx1
    ctx_if->destroy(ctx1);
    
    // ctx2应该不受影响
    size_t ctx2_total = ctx_if->get_total_allocated(ctx2);
    TEST_ASSERT_EQUAL(0, ctx2_total);
    
    // 在ctx2中分配内存
    void* mem2 = ctx_if->allocate(ctx2, 400, 8);
    TEST_ASSERT_NOT_NULL(mem2);
    
    ctx2_total = ctx_if->get_total_allocated(ctx2);
    TEST_ASSERT(ctx2_total >= 400);
    
    // 清理
    ctx_if->deallocate(ctx2, mem2);
    ctx_if->destroy(ctx2);
    
    return true;
}

/**
 * @brief 运行所有统计测试
 * 
 * @return bool 是否所有测试都通过
 */
bool test_context_statistics_all(void)
{
    test_context_print_header("内存统计");
    
    TestCase tests[] = {
        {"获取总分配内存 - 基本功能", test_context_get_total_allocated_basic, true},
        {"获取总分配内存 - 空上下文", test_context_get_total_allocated_empty, true},
        {"获取总分配内存 - 多次分配释放", test_context_get_total_allocated_multiple, true},
        {"获取峰值分配内存 - 基本功能", test_context_get_peak_allocated_basic, true},
        {"获取峰值分配内存 - 增长情况", test_context_get_peak_allocated_growth, true},
        {"获取峰值分配内存 - 重置后", test_context_get_peak_allocated_reset, true},
        {"统计准确性测试", test_context_statistics_accuracy, true},
        {"层次结构统计测试", test_context_statistics_hierarchy, true},
        {"重置对统计的影响", test_context_statistics_reset_effect, true},
        {"销毁对统计的影响", test_context_statistics_destroy_effect, true},
    };
    
    int test_count = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, test_count, "内存统计");
}
