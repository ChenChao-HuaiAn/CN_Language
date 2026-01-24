#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "cnlang/support/memory/arena.h"

/*
 * Arena 分配器压力测试与性能测试
 * 针对阶段7性能优化的回归测试
 */

/* 测试1: 大量小对象分配性能 */
static void test_arena_many_small_allocs(void)
{
    CnArena *arena = cn_arena_new(65536);  // 64KB 块
    assert(arena != NULL);
    
    clock_t start = clock();
    
    // 分配 10000 个小对象
    void *ptrs[10000];
    for (int i = 0; i < 10000; i++) {
        ptrs[i] = cn_arena_alloc(arena, 32);  // 32 字节
        assert(ptrs[i] != NULL);
    }
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
    
    size_t total, block_count;
    cn_arena_get_stats(arena, &total, &block_count);
    
    printf("大量小对象分配:\n");
    printf("  分配数量: 10000\n");
    printf("  总内存: %zu 字节\n", total);
    printf("  块数量: %zu\n", block_count);
    printf("  耗时: %.3f ms\n", time_spent);
    printf("  平均每次分配: %.3f us\n", time_spent * 1000.0 / 10000);
    
    // 验证性能要求: 平均每次分配应该 < 1 us
    assert(time_spent * 1000.0 / 10000 < 1.0);
    
    cn_arena_free(arena);
    printf("[PASS] test_arena_many_small_allocs\n\n");
}

/* 测试2: 对比 malloc/free 性能 */
static void test_arena_vs_malloc_performance(void)
{
    #define ALLOC_COUNT 10000
    #define ALLOC_SIZE 64
    
    // Arena 分配性能
    clock_t arena_start = clock();
    CnArena *arena = cn_arena_new(0);
    void **arena_ptrs = (void **)malloc(ALLOC_COUNT * sizeof(void *));
    for (int i = 0; i < ALLOC_COUNT; i++) {
        arena_ptrs[i] = cn_arena_alloc(arena, ALLOC_SIZE);
    }
    cn_arena_free(arena);
    clock_t arena_end = clock();
    double arena_time = (double)(arena_end - arena_start) / CLOCKS_PER_SEC * 1000.0;
    free(arena_ptrs);
    
    // malloc/free 性能
    clock_t malloc_start = clock();
    void **malloc_ptrs = (void **)malloc(ALLOC_COUNT * sizeof(void *));
    for (int i = 0; i < ALLOC_COUNT; i++) {
        malloc_ptrs[i] = malloc(ALLOC_SIZE);
    }
    for (int i = 0; i < ALLOC_COUNT; i++) {
        free(malloc_ptrs[i]);
    }
    clock_t malloc_end = clock();
    double malloc_time = (double)(malloc_end - malloc_start) / CLOCKS_PER_SEC * 1000.0;
    free(malloc_ptrs);
    
    printf("Arena vs malloc/free 性能对比:\n");
    printf("  Arena 总耗时: %.3f ms\n", arena_time);
    printf("  malloc/free 总耗时: %.3f ms\n", malloc_time);
    printf("  性能提升: %.2fx\n", malloc_time / arena_time);
    
    // Arena 应该比 malloc/free 快至少 2 倍
    assert(malloc_time > arena_time * 2.0);
    
    printf("[PASS] test_arena_vs_malloc_performance\n\n");
    
    #undef ALLOC_COUNT
    #undef ALLOC_SIZE
}

/* 测试3: 对齐分配性能 */
static void test_arena_aligned_alloc_performance(void)
{
    CnArena *arena = cn_arena_new(0);
    assert(arena != NULL);
    
    clock_t start = clock();
    
    // 分配 1000 个不同对齐的对象
    for (int i = 0; i < 1000; i++) {
        void *p8 = cn_arena_alloc_aligned(arena, 64, 8);
        void *p16 = cn_arena_alloc_aligned(arena, 64, 16);
        
        assert(p8 != NULL);
        assert(p16 != NULL);
        
        // 验证对齐
        assert(((uintptr_t)p8 % 8) == 0);
        assert(((uintptr_t)p16 % 16) == 0);
    }
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
    
    printf("对齐分配性能:\n");
    printf("  分配数量: 2000 (各种对齐)\n");
    printf("  耗时: %.3f ms\n", time_spent);
    
    cn_arena_free(arena);
    printf("[PASS] test_arena_aligned_alloc_performance\n\n");
}

/* 测试4: 大对象分配性能 */
static void test_arena_large_alloc_performance(void)
{
    CnArena *arena = cn_arena_new(65536);  // 64KB 块
    assert(arena != NULL);
    
    clock_t start = clock();
    
    // 分配多个超大对象
    void *p1 = cn_arena_alloc(arena, 1024 * 1024);  // 1MB
    void *p2 = cn_arena_alloc(arena, 2 * 1024 * 1024);  // 2MB
    void *p3 = cn_arena_alloc(arena, 512 * 1024);  // 512KB
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
    
    assert(p1 != NULL);
    assert(p2 != NULL);
    assert(p3 != NULL);
    
    size_t total, block_count;
    cn_arena_get_stats(arena, &total, &block_count);
    
    printf("大对象分配性能:\n");
    printf("  总分配: %.2f MB\n", total / (1024.0 * 1024.0));
    printf("  块数量: %zu\n", block_count);
    printf("  耗时: %.3f ms\n", time_spent);
    
    cn_arena_free(arena);
    printf("[PASS] test_arena_large_alloc_performance\n\n");
}

/* 测试5: Reset 性能测试 */
static void test_arena_reset_performance(void)
{
    CnArena *arena = cn_arena_new(0);
    assert(arena != NULL);
    
    clock_t total_time = 0;
    
    // 多次分配和 reset
    for (int round = 0; round < 100; round++) {
        // 分配一些对象
        for (int i = 0; i < 100; i++) {
            cn_arena_alloc(arena, 64);
        }
        
        // 测量 reset 时间
        clock_t start = clock();
        cn_arena_reset(arena);
        clock_t end = clock();
        total_time += (end - start);
    }
    
    double avg_reset_time = (double)total_time / 100.0 / CLOCKS_PER_SEC * 1000000.0;  // 转换为微秒
    
    printf("Reset 性能:\n");
    printf("  执行次数: 100\n");
    printf("  平均耗时: %.3f us\n", avg_reset_time);
    
    // Reset 应该非常快 (< 10 us)
    assert(avg_reset_time < 10.0);
    
    cn_arena_free(arena);
    printf("[PASS] test_arena_reset_performance\n\n");
}

/* 测试6: 内存碎片测试 */
static void test_arena_fragmentation(void)
{
    CnArena *arena = cn_arena_new(4096);  // 4KB 块
    assert(arena != NULL);
    
    // 分配不同大小的对象，模拟真实场景
    size_t sizes[] = {16, 32, 64, 128, 256, 512, 24, 48, 96};
    int size_count = sizeof(sizes) / sizeof(sizes[0]);
    
    for (int round = 0; round < 100; round++) {
        for (int i = 0; i < size_count; i++) {
            void *p = cn_arena_alloc(arena, sizes[i]);
            assert(p != NULL);
        }
    }
    
    size_t total, block_count;
    cn_arena_get_stats(arena, &total, &block_count);
    
    // 计算理论最小内存和实际使用内存
    size_t theoretical_min = 0;
    for (int i = 0; i < size_count; i++) {
        theoretical_min += sizes[i];
    }
    theoretical_min *= 100;  // 100轮
    
    double fragmentation = (double)(total - theoretical_min) / theoretical_min * 100.0;
    
    printf("内存碎片测试:\n");
    printf("  理论最小: %zu 字节\n", theoretical_min);
    printf("  实际使用: %zu 字节\n", total);
    printf("  碎片率: %.2f%%\n", fragmentation);
    printf("  块数量: %zu\n", block_count);
    
    // 碎片率应该控制在合理范围内 (< 30%)
    assert(fragmentation < 30.0);
    
    cn_arena_free(arena);
    printf("[PASS] test_arena_fragmentation\n\n");
}

/* 测试7: 边界条件压力测试 */
static void test_arena_edge_cases_stress(void)
{
    CnArena *arena = cn_arena_new(1024);
    assert(arena != NULL);
    
    // 零大小分配
    void *p0 = cn_arena_alloc(arena, 0);
    assert(p0 == NULL);
    
    // 非常小的分配
    void *p1 = cn_arena_alloc(arena, 1);
    assert(p1 != NULL);
    
    // 恰好块大小
    void *p2 = cn_arena_alloc(arena, 1024);
    assert(p2 != NULL);
    
    // 略大于块大小
    void *p3 = cn_arena_alloc(arena, 1025);
    assert(p3 != NULL);
    
    // 非常大的分配
    void *p4 = cn_arena_alloc(arena, 10 * 1024 * 1024);  // 10MB
    assert(p4 != NULL);
    
    // 非对齐大小
    for (int i = 1; i <= 100; i++) {
        void *p = cn_arena_alloc(arena, i);
        assert(p != NULL);
    }
    
    size_t total, block_count;
    cn_arena_get_stats(arena, &total, &block_count);
    
    printf("边界条件压力测试:\n");
    printf("  总内存: %zu 字节\n", total);
    printf("  块数量: %zu\n", block_count);
    
    cn_arena_free(arena);
    printf("[PASS] test_arena_edge_cases_stress\n\n");
}

int main(void)
{
    printf("========== Arena 分配器压力与性能测试 ==========\n\n");
    
    test_arena_many_small_allocs();
    test_arena_vs_malloc_performance();
    test_arena_aligned_alloc_performance();
    test_arena_large_alloc_performance();
    test_arena_reset_performance();
    test_arena_fragmentation();
    test_arena_edge_cases_stress();
    
    printf("================================================\n");
    printf("所有压力测试通过！✓\n");
    printf("================================================\n");
    
    return 0;
}
