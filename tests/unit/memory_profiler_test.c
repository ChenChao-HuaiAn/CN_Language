#include "cnlang/support/memory_profiler.h"
#include <stdio.h>
#include <assert.h>

/* 测试：初始化内存统计 */
static void test_mem_stats_init(void)
{
    CnMemStats stats;
    cn_mem_stats_init(&stats);
    
    assert(stats.enabled == false);
    assert(stats.total_allocation_count == 0);
    assert(stats.total_bytes_allocated == 0);
    assert(stats.peak_memory_usage == 0);
    assert(stats.current_memory_usage == 0);
    
    for (int i = 0; i < CN_MEM_CATEGORY_COUNT; i++) {
        assert(stats.categories[i].allocation_count == 0);
        assert(stats.categories[i].total_bytes == 0);
        assert(stats.categories[i].peak_bytes == 0);
        assert(stats.categories[i].current_bytes == 0);
    }
    
    printf("✓ test_mem_stats_init 通过\n");
}

/* 测试：启用/禁用内存统计 */
static void test_mem_stats_enable_disable(void)
{
    CnMemStats stats;
    cn_mem_stats_init(&stats);
    
    assert(stats.enabled == false);
    
    cn_mem_stats_set_enabled(&stats, true);
    assert(stats.enabled == true);
    
    cn_mem_stats_set_enabled(&stats, false);
    assert(stats.enabled == false);
    
    printf("✓ test_mem_stats_enable_disable 通过\n");
}

/* 测试：记录内存分配 */
static void test_mem_stats_record_alloc(void)
{
    CnMemStats stats;
    cn_mem_stats_init(&stats);
    cn_mem_stats_set_enabled(&stats, true);
    
    /* 记录第一次分配 */
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_AST, 1024);
    
    assert(stats.total_allocation_count == 1);
    assert(stats.total_bytes_allocated == 1024);
    assert(stats.current_memory_usage == 1024);
    assert(stats.peak_memory_usage == 1024);
    
    assert(stats.categories[CN_MEM_CATEGORY_AST].allocation_count == 1);
    assert(stats.categories[CN_MEM_CATEGORY_AST].total_bytes == 1024);
    assert(stats.categories[CN_MEM_CATEGORY_AST].current_bytes == 1024);
    assert(stats.categories[CN_MEM_CATEGORY_AST].peak_bytes == 1024);
    
    /* 记录第二次分配 */
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_AST, 512);
    
    assert(stats.total_allocation_count == 2);
    assert(stats.total_bytes_allocated == 1536);
    assert(stats.current_memory_usage == 1536);
    assert(stats.peak_memory_usage == 1536);
    
    assert(stats.categories[CN_MEM_CATEGORY_AST].allocation_count == 2);
    assert(stats.categories[CN_MEM_CATEGORY_AST].total_bytes == 1536);
    assert(stats.categories[CN_MEM_CATEGORY_AST].current_bytes == 1536);
    assert(stats.categories[CN_MEM_CATEGORY_AST].peak_bytes == 1536);
    
    printf("✓ test_mem_stats_record_alloc 通过\n");
}

/* 测试：记录内存释放 */
static void test_mem_stats_record_free(void)
{
    CnMemStats stats;
    cn_mem_stats_init(&stats);
    cn_mem_stats_set_enabled(&stats, true);
    
    /* 分配内存 */
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_IR, 2048);
    
    /* 释放部分内存 */
    cn_mem_stats_record_free(&stats, CN_MEM_CATEGORY_IR, 1024);
    
    assert(stats.current_memory_usage == 1024);
    assert(stats.peak_memory_usage == 2048);  /* 峰值不变 */
    
    assert(stats.categories[CN_MEM_CATEGORY_IR].current_bytes == 1024);
    assert(stats.categories[CN_MEM_CATEGORY_IR].peak_bytes == 2048);
    
    /* 释放剩余内存 */
    cn_mem_stats_record_free(&stats, CN_MEM_CATEGORY_IR, 1024);
    
    assert(stats.current_memory_usage == 0);
    assert(stats.categories[CN_MEM_CATEGORY_IR].current_bytes == 0);
    
    printf("✓ test_mem_stats_record_free 通过\n");
}

/* 测试：多类别分配 */
static void test_mem_stats_multiple_categories(void)
{
    CnMemStats stats;
    cn_mem_stats_init(&stats);
    cn_mem_stats_set_enabled(&stats, true);
    
    /* 不同类别分配 */
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_AST, 1024);
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_SYMBOL, 512);
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_IR, 2048);
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_DIAGNOSTICS, 256);
    
    /* 验证全局统计 */
    assert(stats.total_allocation_count == 4);
    assert(stats.total_bytes_allocated == 3840);
    assert(stats.current_memory_usage == 3840);
    assert(stats.peak_memory_usage == 3840);
    
    /* 验证各类别 */
    assert(stats.categories[CN_MEM_CATEGORY_AST].total_bytes == 1024);
    assert(stats.categories[CN_MEM_CATEGORY_SYMBOL].total_bytes == 512);
    assert(stats.categories[CN_MEM_CATEGORY_IR].total_bytes == 2048);
    assert(stats.categories[CN_MEM_CATEGORY_DIAGNOSTICS].total_bytes == 256);
    
    printf("✓ test_mem_stats_multiple_categories 通过\n");
}

/* 测试：峰值内存追踪 */
static void test_mem_stats_peak_tracking(void)
{
    CnMemStats stats;
    cn_mem_stats_init(&stats);
    cn_mem_stats_set_enabled(&stats, true);
    
    /* 分配 1KB */
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_AST, 1024);
    assert(stats.peak_memory_usage == 1024);
    
    /* 分配 2KB */
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_AST, 2048);
    assert(stats.peak_memory_usage == 3072);
    
    /* 释放 1KB，峰值不变 */
    cn_mem_stats_record_free(&stats, CN_MEM_CATEGORY_AST, 1024);
    assert(stats.peak_memory_usage == 3072);
    assert(stats.current_memory_usage == 2048);
    
    /* 再分配 512B，不超过峰值 */
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_AST, 512);
    assert(stats.peak_memory_usage == 3072);
    assert(stats.current_memory_usage == 2560);
    
    /* 分配大块内存，超过峰值 */
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_AST, 4096);
    assert(stats.peak_memory_usage == 6656);
    assert(stats.current_memory_usage == 6656);
    
    printf("✓ test_mem_stats_peak_tracking 通过\n");
}

/* 测试：禁用时不记录 */
static void test_mem_stats_disabled_no_record(void)
{
    CnMemStats stats;
    cn_mem_stats_init(&stats);
    cn_mem_stats_set_enabled(&stats, false);
    
    /* 尝试记录分配，应该不起作用 */
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_AST, 1024);
    
    assert(stats.total_allocation_count == 0);
    assert(stats.total_bytes_allocated == 0);
    assert(stats.current_memory_usage == 0);
    
    printf("✓ test_mem_stats_disabled_no_record 通过\n");
}

/* 测试：类别名称 */
static void test_mem_category_name(void)
{
    assert(cn_mem_category_name(CN_MEM_CATEGORY_AST) != NULL);
    assert(cn_mem_category_name(CN_MEM_CATEGORY_SYMBOL) != NULL);
    assert(cn_mem_category_name(CN_MEM_CATEGORY_IR) != NULL);
    assert(cn_mem_category_name(CN_MEM_CATEGORY_DIAGNOSTICS) != NULL);
    assert(cn_mem_category_name(CN_MEM_CATEGORY_OTHER) != NULL);
    
    printf("✓ test_mem_category_name 通过\n");
}

/* 测试：打印内存统计 */
static void test_mem_stats_print(void)
{
    CnMemStats stats;
    cn_mem_stats_init(&stats);
    cn_mem_stats_set_enabled(&stats, true);
    
    /* 模拟一些分配 */
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_AST, 2048);
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_SYMBOL, 1024);
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_IR, 4096);
    
    printf("\n--- 测试打印内存统计 ---\n");
    cn_mem_stats_print(&stats, stdout);
    printf("✓ test_mem_stats_print 通过\n");
}

/* 测试：重置内存统计 */
static void test_mem_stats_reset(void)
{
    CnMemStats stats;
    cn_mem_stats_init(&stats);
    cn_mem_stats_set_enabled(&stats, true);
    
    /* 记录一些分配 */
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_AST, 1024);
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_IR, 2048);
    
    assert(stats.total_allocation_count > 0);
    
    /* 重置 */
    cn_mem_stats_reset(&stats);
    
    /* 验证已重置但保持启用状态 */
    assert(stats.enabled == true);
    assert(stats.total_allocation_count == 0);
    assert(stats.total_bytes_allocated == 0);
    assert(stats.current_memory_usage == 0);
    
    printf("✓ test_mem_stats_reset 通过\n");
}

int main(void)
{
    printf("========== 内存分析模块单元测试 ==========\n\n");
    
    test_mem_stats_init();
    test_mem_stats_enable_disable();
    test_mem_stats_record_alloc();
    test_mem_stats_record_free();
    test_mem_stats_multiple_categories();
    test_mem_stats_peak_tracking();
    test_mem_stats_disabled_no_record();
    test_mem_category_name();
    test_mem_stats_print();
    test_mem_stats_reset();
    
    printf("\n========================================\n");
    printf("所有测试通过! ✓\n");
    printf("========================================\n");
    
    return 0;
}
