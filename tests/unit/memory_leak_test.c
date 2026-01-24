#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cnlang/support/memory_profiler.h"

/*
 * 内存泄漏检测与边界场景测试
 * 针对阶段7内存优化的回归测试
 */

/* 测试1: 重复分配释放无泄漏 */
static void test_no_memory_leak_repeated_alloc(void)
{
    CnMemStats stats;
    cn_mem_stats_init(&stats);
    cn_mem_stats_set_enabled(&stats, true);
    
    #define ROUNDS 100
    
    for (int i = 0; i < ROUNDS; i++) {
        // 模拟分配
        size_t alloc_size = 1024;
        cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_AST, alloc_size);
        
        // 模拟释放
        cn_mem_stats_record_free(&stats, CN_MEM_CATEGORY_AST, alloc_size);
    }
    
    // 验证: 多次分配释放后，当前内存使用应该为0
    assert(stats.current_memory_usage == 0);
    assert(stats.peak_memory_usage > 0);
    assert(stats.total_allocation_count == ROUNDS);
    
    printf("重复分配释放测试:\n");
    printf("  轮次: %d\n", ROUNDS);
    printf("  当前内存: %zu 字节 (应为0)\n", stats.current_memory_usage);
    printf("  峰值内存: %zu 字节\n", stats.peak_memory_usage);
    
    printf("[PASS] test_no_memory_leak_repeated_alloc\n\n");
    
    #undef ROUNDS
}

/* 测试2: 不同分类内存泄漏检测 */
static void test_memory_leak_detection_by_category(void)
{
    CnMemStats stats;
    cn_mem_stats_init(&stats);
    cn_mem_stats_set_enabled(&stats, true);
    
    // AST 正常释放
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_AST, 1000);
    cn_mem_stats_record_free(&stats, CN_MEM_CATEGORY_AST, 1000);
    
    // Symbol 泄漏 500 字节
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_SYMBOL, 1500);
    cn_mem_stats_record_free(&stats, CN_MEM_CATEGORY_SYMBOL, 1000);
    
    // IR 泄漏 200 字节
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_IR, 200);
    
    // 验证泄漏检测
    size_t leaked = stats.current_memory_usage;
    assert(leaked == 700);  // 500 + 200
    
    const CnMemCategoryStats *ast_stats = cn_mem_stats_get_category(&stats, CN_MEM_CATEGORY_AST);
    const CnMemCategoryStats *sym_stats = cn_mem_stats_get_category(&stats, CN_MEM_CATEGORY_SYMBOL);
    const CnMemCategoryStats *ir_stats = cn_mem_stats_get_category(&stats, CN_MEM_CATEGORY_IR);
    
    printf("分类内存泄漏检测:\n");
    printf("  AST 当前内存: %zu 字节\n", ast_stats->current_bytes);
    printf("  Symbol 当前内存: %zu 字节\n", sym_stats->current_bytes);
    printf("  IR 当前内存: %zu 字节\n", ir_stats->current_bytes);
    printf("  总泄漏: %zu 字节\n", leaked);
    
    assert(ast_stats->current_bytes == 0);
    assert(sym_stats->current_bytes == 500);
    assert(ir_stats->current_bytes == 200);
    
    printf("[PASS] test_memory_leak_detection_by_category\n\n");
}

/* 测试3: 零大小分配边界测试 */
static void test_zero_size_allocation(void)
{
    CnMemStats stats;
    cn_mem_stats_init(&stats);
    cn_mem_stats_set_enabled(&stats, true);
    
    // 记录零大小分配
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_AST, 0);
    
    // 验证: 零大小分配不应增加内存使用
    assert(stats.current_memory_usage == 0);
    assert(stats.total_allocation_count == 1);
    
    printf("零大小分配测试:\n");
    printf("  分配次数: %zu\n", stats.total_allocation_count);
    printf("  当前内存: %zu 字节\n", stats.current_memory_usage);
    
    printf("[PASS] test_zero_size_allocation\n\n");
}

/* 测试4: 超大分配测试 */
static void test_huge_allocation(void)
{
    CnMemStats stats;
    cn_mem_stats_init(&stats);
    cn_mem_stats_set_enabled(&stats, true);
    
    // 分配 100MB（避免太大导致测试缓慢）
    size_t huge_size = 100ULL * 1024 * 1024;
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_IR, huge_size);
    
    assert(stats.current_memory_usage == huge_size);
    assert(stats.peak_memory_usage == huge_size);
    
    printf("超大分配测试:\n");
    printf("  分配大小: %.2f MB\n", huge_size / (1024.0 * 1024.0));
    printf("  当前内存: %zu 字节\n", stats.current_memory_usage);
    
    // 释放
    cn_mem_stats_record_free(&stats, CN_MEM_CATEGORY_IR, huge_size);
    assert(stats.current_memory_usage == 0);
    
    printf("[PASS] test_huge_allocation\n\n");
}

/* 测试5: 峰值内存追踪 */
static void test_peak_memory_tracking(void)
{
    CnMemStats stats;
    cn_mem_stats_init(&stats);
    cn_mem_stats_set_enabled(&stats, true);
    
    // 第一次分配
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_AST, 1000);
    assert(stats.peak_memory_usage == 1000);
    
    // 第二次分配，达到更高峰值
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_SYMBOL, 2000);
    assert(stats.peak_memory_usage == 3000);
    
    // 释放第一次分配，峰值保持
    cn_mem_stats_record_free(&stats, CN_MEM_CATEGORY_AST, 1000);
    assert(stats.peak_memory_usage == 3000);
    assert(stats.current_memory_usage == 2000);
    
    // 再次分配，超过之前峰值
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_IR, 5000);
    assert(stats.peak_memory_usage == 7000);
    
    printf("峰值内存追踪测试:\n");
    printf("  当前内存: %zu 字节\n", stats.current_memory_usage);
    printf("  峰值内存: %zu 字节\n", stats.peak_memory_usage);
    
    printf("[PASS] test_peak_memory_tracking\n\n");
}

int main(void)
{
    printf("========== 内存泄漏检测与边界测试 ==========\n\n");
    
    test_no_memory_leak_repeated_alloc();
    test_memory_leak_detection_by_category();
    test_zero_size_allocation();
    test_huge_allocation();
    test_peak_memory_tracking();
    
    printf("============================================\n");
    printf("所有内存测试通过！✓\n");
    printf("============================================\n");
    
    return 0;
}
