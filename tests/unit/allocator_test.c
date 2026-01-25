#include "cnlang/runtime/allocator.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/*
 * 内存分配器功能与性能测试
 * 测试各种分配策略、内存池管理和泄漏检测功能
 */

// =============================================================================
// 基础功能测试
// =============================================================================

// 测试1: 分配器创建和销毁
static void test_allocator_create_destroy(void)
{
    printf("测试1: 分配器创建和销毁\n");
    
    CnAllocator *allocator = cn_allocator_create(1024 * 1024, CN_ALLOC_STRATEGY_FIRST_FIT);
    assert(allocator != NULL);
    assert(allocator->default_pool_size == 1024 * 1024);
    assert(allocator->default_strategy == CN_ALLOC_STRATEGY_FIRST_FIT);
    
    cn_allocator_destroy(allocator);
    
    printf("[PASS] 分配器创建和销毁测试\n\n");
}

// 测试2: 内存池创建和管理
static void test_pool_creation(void)
{
    printf("测试2: 内存池创建和管理\n");
    
    CnAllocator *allocator = cn_allocator_create(0, CN_ALLOC_STRATEGY_FIRST_FIT);
    assert(allocator != NULL);
    
    // 创建多个内存池
    CnMemPool *pool1 = cn_pool_create(allocator, "pool1", 64 * 1024, CN_ALLOC_STRATEGY_FIRST_FIT);
    assert(pool1 != NULL);
    assert(pool1->total_size == 64 * 1024);
    assert(pool1->free_size == 64 * 1024);
    assert(pool1->used_size == 0);
    
    CnMemPool *pool2 = cn_pool_create(allocator, "pool2", 128 * 1024, CN_ALLOC_STRATEGY_BEST_FIT);
    assert(pool2 != NULL);
    
    CnMemPool *pool3 = cn_pool_create(allocator, "pool3", 256 * 1024, CN_ALLOC_STRATEGY_WORST_FIT);
    assert(pool3 != NULL);
    
    assert(allocator->pool_count == 3);
    
    // 查找内存池
    CnMemPool *found = cn_pool_find(allocator, "pool2");
    assert(found == pool2);
    
    found = cn_pool_find(allocator, "nonexistent");
    assert(found == NULL);
    
    cn_allocator_destroy(allocator);
    
    printf("[PASS] 内存池创建和管理测试\n\n");
}

// =============================================================================
// 分配算法测试
// =============================================================================

// 测试3: 首次适应算法
static void test_first_fit_allocation(void)
{
    printf("测试3: 首次适应算法\n");
    
    CnAllocator *allocator = cn_allocator_create(0, CN_ALLOC_STRATEGY_FIRST_FIT);
    CnMemPool *pool = cn_pool_create(allocator, "test", 1024, CN_ALLOC_STRATEGY_FIRST_FIT);
    
    // 分配多个块
    void *ptr1 = cn_pool_alloc(pool, 100);
    assert(ptr1 != NULL);
    
    void *ptr2 = cn_pool_alloc(pool, 200);
    assert(ptr2 != NULL);
    
    void *ptr3 = cn_pool_alloc(pool, 150);
    assert(ptr3 != NULL);
    
    assert(pool->alloc_count == 3);
    assert(pool->used_size > 0);
    
    // 释放中间的块
    cn_pool_free(pool, ptr2);
    assert(pool->free_count == 1);
    
    // 再次分配，应该使用首次适应找到空闲块
    void *ptr4 = cn_pool_alloc(pool, 50);
    assert(ptr4 != NULL);
    
    cn_allocator_destroy(allocator);
    
    printf("[PASS] 首次适应算法测试\n\n");
}

// 测试4: 最佳适应算法
static void test_best_fit_allocation(void)
{
    printf("测试4: 最佳适应算法\n");
    
    CnAllocator *allocator = cn_allocator_create(0, CN_ALLOC_STRATEGY_BEST_FIT);
    CnMemPool *pool = cn_pool_create(allocator, "test", 2048, CN_ALLOC_STRATEGY_BEST_FIT);
    
    // 分配和释放，创建不同大小的空闲块
    void *ptr1 = cn_pool_alloc(pool, 100);
    void *ptr2 = cn_pool_alloc(pool, 200);
    void *ptr3 = cn_pool_alloc(pool, 300);
    void *ptr4 = cn_pool_alloc(pool, 150);
    
    // 释放部分块，创建碎片
    cn_pool_free(pool, ptr2); // 释放200字节
    cn_pool_free(pool, ptr3); // 释放300字节
    
    // 分配180字节，最佳适应应该选择200字节的块
    void *ptr5 = cn_pool_alloc(pool, 180);
    assert(ptr5 != NULL);
    
    cn_allocator_destroy(allocator);
    
    printf("[PASS] 最佳适应算法测试\n\n");
}

// 测试5: 最差适应算法
static void test_worst_fit_allocation(void)
{
    printf("测试5: 最差适应算法\n");
    
    CnAllocator *allocator = cn_allocator_create(0, CN_ALLOC_STRATEGY_WORST_FIT);
    CnMemPool *pool = cn_pool_create(allocator, "test", 2048, CN_ALLOC_STRATEGY_WORST_FIT);
    
    void *ptr1 = cn_pool_alloc(pool, 100);
    void *ptr2 = cn_pool_alloc(pool, 200);
    void *ptr3 = cn_pool_alloc(pool, 300);
    
    cn_pool_free(pool, ptr2);
    cn_pool_free(pool, ptr3);
    
    // 最差适应应该选择最大的空闲块
    void *ptr4 = cn_pool_alloc(pool, 50);
    assert(ptr4 != NULL);
    
    cn_allocator_destroy(allocator);
    
    printf("[PASS] 最差适应算法测试\n\n");
}

// 测试6: 下次适应算法
static void test_next_fit_allocation(void)
{
    printf("测试6: 下次适应算法\n");
    
    CnAllocator *allocator = cn_allocator_create(0, CN_ALLOC_STRATEGY_NEXT_FIT);
    CnMemPool *pool = cn_pool_create(allocator, "test", 2048, CN_ALLOC_STRATEGY_NEXT_FIT);
    
    void *ptr1 = cn_pool_alloc(pool, 100);
    void *ptr2 = cn_pool_alloc(pool, 100);
    void *ptr3 = cn_pool_alloc(pool, 100);
    
    // 下次适应应该从上次分配位置继续
    assert(pool->last_alloc != NULL);
    
    cn_allocator_destroy(allocator);
    
    printf("[PASS] 下次适应算法测试\n\n");
}

// =============================================================================
// 内存泄漏检测测试
// =============================================================================

// 测试7: 泄漏检测
static void test_leak_detection(void)
{
    printf("测试7: 内存泄漏检测\n");
    
    CnAllocator *allocator = cn_allocator_create(0, CN_ALLOC_STRATEGY_FIRST_FIT);
    CnMemPool *pool = cn_pool_create(allocator, "leak_test", 4096, CN_ALLOC_STRATEGY_FIRST_FIT);
    
    // 启用泄漏检测
    cn_allocator_set_leak_detection(allocator, true);
    
    // 分配一些内存但不释放
    void *ptr1 = cn_pool_alloc(pool, 100);
    void *ptr2 = cn_pool_alloc(pool, 200);
    void *ptr3 = cn_pool_alloc(pool, 300);
    
    // 只释放一部分
    cn_pool_free(pool, ptr2);
    
    // 检查泄漏
    size_t leaks = cn_allocator_check_leaks(allocator);
    assert(leaks == 2); // ptr1 和 ptr3 未释放
    
    printf("检测到 %zu 个内存泄漏\n", leaks);
    
    // 打印泄漏报告
    cn_allocator_print_leak_report(allocator);
    
    cn_allocator_destroy(allocator);
    
    printf("[PASS] 内存泄漏检测测试\n\n");
}

// 测试8: 无泄漏场景
static void test_no_leak(void)
{
    printf("测试8: 无泄漏场景\n");
    
    CnAllocator *allocator = cn_allocator_create(0, CN_ALLOC_STRATEGY_FIRST_FIT);
    CnMemPool *pool = cn_pool_create(allocator, "no_leak", 4096, CN_ALLOC_STRATEGY_FIRST_FIT);
    
    cn_allocator_set_leak_detection(allocator, true);
    
    // 分配并全部释放
    void *ptr1 = cn_pool_alloc(pool, 100);
    void *ptr2 = cn_pool_alloc(pool, 200);
    void *ptr3 = cn_pool_alloc(pool, 300);
    
    cn_pool_free(pool, ptr1);
    cn_pool_free(pool, ptr2);
    cn_pool_free(pool, ptr3);
    
    // 不应有泄漏
    size_t leaks = cn_allocator_check_leaks(allocator);
    assert(leaks == 0);
    
    printf("无内存泄漏，测试通过\n");
    
    cn_allocator_destroy(allocator);
    
    printf("[PASS] 无泄漏场景测试\n\n");
}

// =============================================================================
// 内存整理测试
// =============================================================================

// 测试9: 块合并
static void test_block_merging(void)
{
    printf("测试9: 内存块合并\n");
    
    CnAllocator *allocator = cn_allocator_create(0, CN_ALLOC_STRATEGY_FIRST_FIT);
    CnMemPool *pool = cn_pool_create(allocator, "merge_test", 4096, CN_ALLOC_STRATEGY_FIRST_FIT);
    
    // 分配多个连续的块
    void *ptr1 = cn_pool_alloc(pool, 100);
    void *ptr2 = cn_pool_alloc(pool, 100);
    void *ptr3 = cn_pool_alloc(pool, 100);
    void *ptr4 = cn_pool_alloc(pool, 100);
    
    size_t blocks_before = pool->block_count;
    printf("合并前块数量: %zu\n", blocks_before);
    
    // 释放相邻的块
    cn_pool_free(pool, ptr2);
    cn_pool_free(pool, ptr3);
    
    // 合并空闲块
    size_t merged = cn_pool_merge_free_blocks(pool);
    printf("合并了 %zu 个块\n", merged);
    
    size_t blocks_after = pool->block_count;
    printf("合并后块数量: %zu\n", blocks_after);
    
    assert(blocks_after < blocks_before);
    
    cn_allocator_destroy(allocator);
    
    printf("[PASS] 内存块合并测试\n\n");
}

// =============================================================================
// 统计功能测试
// =============================================================================

// 测试10: 统计信息
static void test_statistics(void)
{
    printf("测试10: 统计信息\n");
    
    CnAllocator *allocator = cn_allocator_create(0, CN_ALLOC_STRATEGY_FIRST_FIT);
    CnMemPool *pool = cn_pool_create(allocator, "stats_test", 8192, CN_ALLOC_STRATEGY_FIRST_FIT);
    
    // 执行一些分配和释放操作
    void *ptr1 = cn_pool_alloc(pool, 500);
    void *ptr2 = cn_pool_alloc(pool, 1000);
    void *ptr3 = cn_pool_alloc(pool, 1500);
    
    cn_pool_free(pool, ptr2);
    
    // 获取池统计
    size_t total, used, free, blocks;
    cn_pool_get_stats(pool, &total, &used, &free, &blocks);
    
    printf("池统计:\n");
    printf("  总大小: %zu\n", total);
    printf("  已使用: %zu\n", used);
    printf("  空闲: %zu\n", free);
    printf("  块数量: %zu\n", blocks);
    
    assert(total == 8192);
    assert(used > 0);
    assert(free > 0);
    assert(used + free == total);
    
    // 打印详细统计
    cn_pool_print_stats(pool);
    
    // 打印分配器统计
    cn_allocator_print_stats(allocator);
    
    cn_allocator_destroy(allocator);
    
    printf("[PASS] 统计信息测试\n\n");
}

// =============================================================================
// 边界测试
// =============================================================================

// 测试11: 边界条件
static void test_edge_cases(void)
{
    printf("测试11: 边界条件测试\n");
    
    CnAllocator *allocator = cn_allocator_create(0, CN_ALLOC_STRATEGY_FIRST_FIT);
    CnMemPool *pool = cn_pool_create(allocator, "edge_test", 1024, CN_ALLOC_STRATEGY_FIRST_FIT);
    
    // 测试零大小分配
    void *ptr1 = cn_pool_alloc(pool, 0);
    assert(ptr1 == NULL);
    
    // 测试超大分配（超过池大小）
    void *ptr2 = cn_pool_alloc(pool, 2048);
    assert(ptr2 == NULL);
    
    // 测试空指针释放
    cn_pool_free(pool, NULL);
    
    // 测试重复释放
    void *ptr3 = cn_pool_alloc(pool, 100);
    cn_pool_free(pool, ptr3);
    cn_pool_free(pool, ptr3); // 应该有警告
    
    cn_allocator_destroy(allocator);
    
    printf("[PASS] 边界条件测试\n\n");
}

// =============================================================================
// 性能测试
// =============================================================================

// 测试12: 分配性能
static void test_allocation_performance(void)
{
    printf("测试12: 分配性能\n");
    
    const int iterations = 10000;
    void *ptrs[1000];
    
    // 测试各种策略的性能
    CnAllocStrategy strategies[] = {
        CN_ALLOC_STRATEGY_FIRST_FIT,
        CN_ALLOC_STRATEGY_BEST_FIT,
        CN_ALLOC_STRATEGY_WORST_FIT,
        CN_ALLOC_STRATEGY_NEXT_FIT
    };
    
    const char *strategy_names[] = {
        "首次适应", "最佳适应", "最差适应", "下次适应"
    };
    
    for (int s = 0; s < 4; s++) {
        CnAllocator *allocator = cn_allocator_create(0, strategies[s]);
        CnMemPool *pool = cn_pool_create(allocator, "perf_test", 
                                        10 * 1024 * 1024, strategies[s]);
        
        clock_t start = clock();
        
        // 执行大量分配和释放
        for (int i = 0; i < iterations; i++) {
            int idx = i % 1000;
            if (ptrs[idx]) {
                cn_pool_free(pool, ptrs[idx]);
            }
            ptrs[idx] = cn_pool_alloc(pool, 50 + (i % 500));
        }
        
        clock_t end = clock();
        double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
        
        printf("%s: %d 次操作耗时 %.3f 秒\n", 
               strategy_names[s], iterations, elapsed);
        printf("  平均每次: %.6f 秒\n", elapsed / iterations);
        
        cn_allocator_destroy(allocator);
    }
    
    printf("[PASS] 分配性能测试\n\n");
}

// =============================================================================
// 压力测试
// =============================================================================

// 测试13: 压力测试
static void test_stress(void)
{
    printf("测试13: 压力测试\n");
    
    CnAllocator *allocator = cn_allocator_create(0, CN_ALLOC_STRATEGY_FIRST_FIT);
    
    // 创建多个池
    CnMemPool *pools[10];
    for (int i = 0; i < 10; i++) {
        char name[32];
        snprintf(name, sizeof(name), "pool_%d", i);
        pools[i] = cn_pool_create(allocator, name, 1024 * 1024, 
                                 (CnAllocStrategy)(i % 4));
    }
    
    // 大量随机分配和释放
    const int operations = 1000;
    void *ptrs[100] = {0};
    
    for (int i = 0; i < operations; i++) {
        int pool_idx = i % 10;
        int ptr_idx = i % 100;
        
        if (ptrs[ptr_idx]) {
            cn_pool_free(pools[pool_idx], ptrs[ptr_idx]);
            ptrs[ptr_idx] = NULL;
        } else {
            ptrs[ptr_idx] = cn_pool_alloc(pools[pool_idx], 100 + (i % 900));
        }
    }
    
    // 打印最终统计
    printf("\n压力测试完成，最终统计:\n");
    cn_allocator_print_stats(allocator);
    
    cn_allocator_destroy(allocator);
    
    printf("[PASS] 压力测试\n\n");
}

// =============================================================================
// 主函数
// =============================================================================

int main(void)
{
    printf("========================================\n");
    printf("  CN Language 内存分配器测试套件\n");
    printf("========================================\n\n");
    
    // 基础功能测试
    test_allocator_create_destroy();
    test_pool_creation();
    
    // 分配算法测试
    test_first_fit_allocation();
    test_best_fit_allocation();
    test_worst_fit_allocation();
    test_next_fit_allocation();
    
    // 泄漏检测测试
    test_leak_detection();
    test_no_leak();
    
    // 内存整理测试
    test_block_merging();
    
    // 统计功能测试
    test_statistics();
    
    // 边界测试
    test_edge_cases();
    
    // 性能测试
    test_allocation_performance();
    
    // 压力测试
    test_stress();
    
    printf("========================================\n");
    printf("  所有测试通过！✓\n");
    printf("========================================\n");
    
    return 0;
}
