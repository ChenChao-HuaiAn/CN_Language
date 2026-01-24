#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cnlang/support/perf.h"

/*
 * 性能分析模块单元测试
 */

/* 测试初始化 */
static void test_perf_init(void)
{
    CnPerfStats stats;
    cn_perf_stats_init(&stats, "test.cn", 1024);

    assert(stats.enabled == false);
    assert(stats.source_file != NULL);
    assert(strcmp(stats.source_file, "test.cn") == 0);
    assert(stats.source_size == 1024);

    for (int i = 0; i < CN_PERF_PHASE_COUNT; i++) {
        assert(stats.measurements[i].phase == (CnPerfPhase)i);
        assert(stats.measurements[i].start_time_us == 0);
        assert(stats.measurements[i].end_time_us == 0);
        assert(stats.measurements[i].duration_us == 0);
        assert(stats.measurements[i].is_active == false);
    }

    printf("✓ test_perf_init 通过\n");
}

/* 测试启用/禁用 */
static void test_perf_enable_disable(void)
{
    CnPerfStats stats;
    cn_perf_stats_init(&stats, "test.cn", 1024);

    cn_perf_stats_set_enabled(&stats, true);
    assert(stats.enabled == true);

    cn_perf_stats_set_enabled(&stats, false);
    assert(stats.enabled == false);

    printf("✓ test_perf_enable_disable 通过\n");
}

/* 测试时间戳获取 */
static void test_perf_timestamp(void)
{
    uint64_t t1 = cn_perf_get_timestamp_us();
    
    /* 简单延迟 */
    volatile int sum = 0;
    for (int i = 0; i < 100000; i++) {
        sum += i;
    }
    
    uint64_t t2 = cn_perf_get_timestamp_us();
    
    assert(t2 > t1);
    assert(t2 - t1 > 0);

    printf("✓ test_perf_timestamp 通过 (耗时: %llu 微秒)\n", (unsigned long long)(t2 - t1));
}

/* 测试测量功能 */
static void test_perf_measurement(void)
{
    CnPerfStats stats;
    cn_perf_stats_init(&stats, "test.cn", 1024);
    cn_perf_stats_set_enabled(&stats, true);

    /* 测量一个阶段 */
    cn_perf_start(&stats, CN_PERF_PHASE_LEXER);
    
    /* 模拟工作 */
    volatile int sum = 0;
    for (int i = 0; i < 100000; i++) {
        sum += i;
    }
    
    cn_perf_end(&stats, CN_PERF_PHASE_LEXER);

    /* 验证测量结果 */
    uint64_t duration = cn_perf_get_duration(&stats, CN_PERF_PHASE_LEXER);
    assert(duration > 0);
    assert(stats.measurements[CN_PERF_PHASE_LEXER].is_active == false);

    double duration_ms = cn_perf_get_duration_ms(&stats, CN_PERF_PHASE_LEXER);
    assert(duration_ms > 0);

    printf("✓ test_perf_measurement 通过 (词法分析耗时: %.3f ms)\n", duration_ms);
}

/* 测试禁用时的测量 */
static void test_perf_measurement_disabled(void)
{
    CnPerfStats stats;
    cn_perf_stats_init(&stats, "test.cn", 1024);
    cn_perf_stats_set_enabled(&stats, false);  /* 禁用 */

    cn_perf_start(&stats, CN_PERF_PHASE_PARSER);
    
    /* 模拟工作 */
    volatile int sum = 0;
    for (int i = 0; i < 100000; i++) {
        sum += i;
    }
    
    cn_perf_end(&stats, CN_PERF_PHASE_PARSER);

    /* 验证未测量 */
    uint64_t duration = cn_perf_get_duration(&stats, CN_PERF_PHASE_PARSER);
    assert(duration == 0);

    printf("✓ test_perf_measurement_disabled 通过\n");
}

/* 测试阶段名称 */
static void test_perf_phase_name(void)
{
    assert(strcmp(cn_perf_phase_name(CN_PERF_PHASE_TOTAL), "总耗时") == 0);
    assert(strcmp(cn_perf_phase_name(CN_PERF_PHASE_LEXER), "词法分析") == 0);
    assert(strcmp(cn_perf_phase_name(CN_PERF_PHASE_PARSER), "语法分析") == 0);
    assert(strcmp(cn_perf_phase_name(CN_PERF_PHASE_SEMANTIC), "语义分析") == 0);
    assert(strcmp(cn_perf_phase_name(CN_PERF_PHASE_IR_GEN), "IR 生成") == 0);
    assert(strcmp(cn_perf_phase_name(CN_PERF_PHASE_IR_OPT), "IR 优化") == 0);
    assert(strcmp(cn_perf_phase_name(CN_PERF_PHASE_CODEGEN), "代码生成") == 0);
    assert(strcmp(cn_perf_phase_name(CN_PERF_PHASE_BACKEND_COMPILE), "后端编译") == 0);

    printf("✓ test_perf_phase_name 通过\n");
}

/* 测试百分比计算 */
static void test_perf_percentage(void)
{
    CnPerfStats stats;
    cn_perf_stats_init(&stats, "test.cn", 1024);
    cn_perf_stats_set_enabled(&stats, true);

    /* 手动设置测量结果 */
    stats.measurements[CN_PERF_PHASE_TOTAL].duration_us = 1000000;  /* 1秒 */
    stats.measurements[CN_PERF_PHASE_LEXER].duration_us = 100000;   /* 0.1秒 */
    stats.measurements[CN_PERF_PHASE_PARSER].duration_us = 200000;  /* 0.2秒 */

    double lexer_pct = cn_perf_get_percentage(&stats, CN_PERF_PHASE_LEXER);
    double parser_pct = cn_perf_get_percentage(&stats, CN_PERF_PHASE_PARSER);

    assert(lexer_pct >= 9.9 && lexer_pct <= 10.1);   /* 约10% */
    assert(parser_pct >= 19.9 && parser_pct <= 20.1); /* 约20% */

    printf("✓ test_perf_percentage 通过 (词法: %.2f%%, 语法: %.2f%%)\n", lexer_pct, parser_pct);
}

/* 测试打印统计 */
static void test_perf_print_stats(void)
{
    CnPerfStats stats;
    cn_perf_stats_init(&stats, "test.cn", 1024);
    cn_perf_stats_set_enabled(&stats, true);

    /* 手动设置测量结果 */
    stats.measurements[CN_PERF_PHASE_TOTAL].duration_us = 1000000;
    stats.measurements[CN_PERF_PHASE_LEXER].duration_us = 100000;
    stats.measurements[CN_PERF_PHASE_PARSER].duration_us = 200000;
    stats.measurements[CN_PERF_PHASE_SEMANTIC].duration_us = 300000;
    stats.measurements[CN_PERF_PHASE_IR_GEN].duration_us = 150000;
    stats.measurements[CN_PERF_PHASE_CODEGEN].duration_us = 250000;

    printf("\n--- 测试打印统计 ---\n");
    cn_perf_print_stats(&stats, stdout);
    printf("✓ test_perf_print_stats 通过\n");
}

/* 测试导出 JSON */
static void test_perf_export_json(void)
{
    CnPerfStats stats;
    cn_perf_stats_init(&stats, "test.cn", 1024);
    cn_perf_stats_set_enabled(&stats, true);

    /* 手动设置测量结果 */
    stats.measurements[CN_PERF_PHASE_TOTAL].duration_us = 1000000;
    stats.measurements[CN_PERF_PHASE_LEXER].duration_us = 100000;
    stats.measurements[CN_PERF_PHASE_PARSER].duration_us = 200000;

    const char *json_file = "test_perf.json";
    bool success = cn_perf_export_json(&stats, json_file);
    assert(success);

    /* 验证文件存在 */
    FILE *f = fopen(json_file, "r");
    assert(f != NULL);
    fclose(f);

    /* 清理 */
    remove(json_file);

    printf("✓ test_perf_export_json 通过\n");
}

/* 测试导出 CSV */
static void test_perf_export_csv(void)
{
    CnPerfStats stats;
    cn_perf_stats_init(&stats, "test.cn", 1024);
    cn_perf_stats_set_enabled(&stats, true);

    /* 手动设置测量结果 */
    stats.measurements[CN_PERF_PHASE_TOTAL].duration_us = 1000000;
    stats.measurements[CN_PERF_PHASE_LEXER].duration_us = 100000;
    stats.measurements[CN_PERF_PHASE_PARSER].duration_us = 200000;

    const char *csv_file = "test_perf.csv";
    bool success = cn_perf_export_csv(&stats, csv_file);
    assert(success);

    /* 验证文件存在 */
    FILE *f = fopen(csv_file, "r");
    assert(f != NULL);
    fclose(f);

    /* 清理 */
    remove(csv_file);

    printf("✓ test_perf_export_csv 通过\n");
}

int main(void)
{
    printf("========== 性能分析模块单元测试 ==========\n\n");

    test_perf_init();
    test_perf_enable_disable();
    test_perf_timestamp();
    test_perf_measurement();
    test_perf_measurement_disabled();
    test_perf_phase_name();
    test_perf_percentage();
    test_perf_print_stats();
    test_perf_export_json();
    test_perf_export_csv();

    printf("\n========================================\n");
    printf("所有测试通过! ✓\n");
    printf("========================================\n");

    return 0;
}
