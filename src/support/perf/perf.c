#include "cnlang/support/perf.h"
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

/* 获取当前时间戳（微秒） */
uint64_t cn_perf_get_timestamp_us(void)
{
#ifdef _WIN32
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    /* 转换为微秒 */
    return (uint64_t)((counter.QuadPart * 1000000ULL) / frequency.QuadPart);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
#endif
}

/* 初始化性能统计 */
void cn_perf_stats_init(CnPerfStats *stats, const char *source_file, size_t source_size)
{
    if (!stats) {
        return;
    }

    memset(stats, 0, sizeof(CnPerfStats));
    stats->enabled = false;
    stats->source_file = source_file;
    stats->source_size = source_size;

    /* 初始化每个测量点 */
    for (int i = 0; i < CN_PERF_PHASE_COUNT; i++) {
        stats->measurements[i].phase = (CnPerfPhase)i;
        stats->measurements[i].start_time_us = 0;
        stats->measurements[i].end_time_us = 0;
        stats->measurements[i].duration_us = 0;
        stats->measurements[i].is_active = false;
    }
}

/* 启用/禁用性能测量 */
void cn_perf_stats_set_enabled(CnPerfStats *stats, bool enabled)
{
    if (stats) {
        stats->enabled = enabled;
    }
}

/* 开始测量某个阶段 */
void cn_perf_start(CnPerfStats *stats, CnPerfPhase phase)
{
    if (!stats || !stats->enabled || phase >= CN_PERF_PHASE_COUNT) {
        return;
    }

    CnPerfMeasurement *m = &stats->measurements[phase];
    m->start_time_us = cn_perf_get_timestamp_us();
    m->is_active = true;
}

/* 结束测量某个阶段 */
void cn_perf_end(CnPerfStats *stats, CnPerfPhase phase)
{
    if (!stats || !stats->enabled || phase >= CN_PERF_PHASE_COUNT) {
        return;
    }

    CnPerfMeasurement *m = &stats->measurements[phase];
    if (!m->is_active) {
        return;
    }

    m->end_time_us = cn_perf_get_timestamp_us();
    m->duration_us = m->end_time_us - m->start_time_us;
    m->is_active = false;
}

/* 获取某个阶段的耗时（微秒） */
uint64_t cn_perf_get_duration(const CnPerfStats *stats, CnPerfPhase phase)
{
    if (!stats || phase >= CN_PERF_PHASE_COUNT) {
        return 0;
    }
    return stats->measurements[phase].duration_us;
}

/* 获取某个阶段的耗时（毫秒） */
double cn_perf_get_duration_ms(const CnPerfStats *stats, CnPerfPhase phase)
{
    uint64_t us = cn_perf_get_duration(stats, phase);
    return (double)us / 1000.0;
}

/* 获取某个阶段的耗时占比（百分比） */
double cn_perf_get_percentage(const CnPerfStats *stats, CnPerfPhase phase)
{
    if (!stats || phase >= CN_PERF_PHASE_COUNT) {
        return 0.0;
    }

    uint64_t total = stats->measurements[CN_PERF_PHASE_TOTAL].duration_us;
    if (total == 0) {
        return 0.0;
    }

    uint64_t duration = stats->measurements[phase].duration_us;
    return ((double)duration / (double)total) * 100.0;
}

/* 获取阶段名称 */
const char* cn_perf_phase_name(CnPerfPhase phase)
{
    switch (phase) {
        case CN_PERF_PHASE_TOTAL:
            return "总耗时";
        case CN_PERF_PHASE_LEXER:
            return "词法分析";
        case CN_PERF_PHASE_PARSER:
            return "语法分析";
        case CN_PERF_PHASE_SEMANTIC:
            return "语义分析";
        case CN_PERF_PHASE_SEMANTIC_SCOPE:
            return "  - 作用域构建";
        case CN_PERF_PHASE_SEMANTIC_RESOLVE:
            return "  - 名称解析";
        case CN_PERF_PHASE_SEMANTIC_TYPECHECK:
            return "  - 类型检查";
        case CN_PERF_PHASE_IR_GEN:
            return "IR 生成";
        case CN_PERF_PHASE_IR_OPT:
            return "IR 优化";
        case CN_PERF_PHASE_CODEGEN:
            return "代码生成";
        case CN_PERF_PHASE_BACKEND_COMPILE:
            return "后端编译";
        default:
            return "未知阶段";
    }
}

/* 打印性能统计到文件 */
void cn_perf_print_stats(const CnPerfStats *stats, FILE *out)
{
    if (!stats || !out) {
        return;
    }

    fprintf(out, "\n========== 编译性能分析报告 ==========\n");
    if (stats->source_file) {
        fprintf(out, "源文件: %s\n", stats->source_file);
    }
    fprintf(out, "源文件大小: %zu 字节\n", stats->source_size);
    fprintf(out, "======================================\n\n");

    /* 打印总耗时 */
    fprintf(out, "%-24s %12.3f ms %8s\n",
            cn_perf_phase_name(CN_PERF_PHASE_TOTAL),
            cn_perf_get_duration_ms(stats, CN_PERF_PHASE_TOTAL),
            "100.0%");
    fprintf(out, "--------------------------------------\n");

    /* 打印各阶段耗时 */
    for (int i = 1; i < CN_PERF_PHASE_COUNT; i++) {
        CnPerfPhase phase = (CnPerfPhase)i;
        uint64_t duration_us = cn_perf_get_duration(stats, phase);
        
        /* 跳过未测量或耗时为0的阶段 */
        if (duration_us == 0) {
            continue;
        }

        double duration_ms = cn_perf_get_duration_ms(stats, phase);
        double percentage = cn_perf_get_percentage(stats, phase);

        fprintf(out, "%-24s %12.3f ms %7.2f%%\n",
                cn_perf_phase_name(phase),
                duration_ms,
                percentage);
    }

    fprintf(out, "======================================\n");
}

/* 导出性能统计到 JSON 格式文件 */
bool cn_perf_export_json(const CnPerfStats *stats, const char *filename)
{
    if (!stats || !filename) {
        return false;
    }

    FILE *f = fopen(filename, "w");
    if (!f) {
        return false;
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"source_file\": \"%s\",\n", stats->source_file ? stats->source_file : "");
    fprintf(f, "  \"source_size\": %zu,\n", stats->source_size);
    fprintf(f, "  \"total_duration_ms\": %.3f,\n", cn_perf_get_duration_ms(stats, CN_PERF_PHASE_TOTAL));
    fprintf(f, "  \"phases\": [\n");

    bool first = true;
    for (int i = 1; i < CN_PERF_PHASE_COUNT; i++) {
        CnPerfPhase phase = (CnPerfPhase)i;
        uint64_t duration_us = cn_perf_get_duration(stats, phase);
        
        if (duration_us == 0) {
            continue;
        }

        if (!first) {
            fprintf(f, ",\n");
        }
        first = false;

        fprintf(f, "    {\n");
        fprintf(f, "      \"name\": \"%s\",\n", cn_perf_phase_name(phase));
        fprintf(f, "      \"duration_ms\": %.3f,\n", cn_perf_get_duration_ms(stats, phase));
        fprintf(f, "      \"percentage\": %.2f\n", cn_perf_get_percentage(stats, phase));
        fprintf(f, "    }");
    }

    fprintf(f, "\n  ]\n");
    fprintf(f, "}\n");

    fclose(f);
    return true;
}

/* 导出性能统计到 CSV 格式文件 */
bool cn_perf_export_csv(const CnPerfStats *stats, const char *filename)
{
    if (!stats || !filename) {
        return false;
    }

    FILE *f = fopen(filename, "w");
    if (!f) {
        return false;
    }

    /* CSV 表头 */
    fprintf(f, "Phase,Duration_ms,Percentage\n");

    /* 总耗时 */
    fprintf(f, "%s,%.3f,100.00\n",
            cn_perf_phase_name(CN_PERF_PHASE_TOTAL),
            cn_perf_get_duration_ms(stats, CN_PERF_PHASE_TOTAL));

    /* 各阶段耗时 */
    for (int i = 1; i < CN_PERF_PHASE_COUNT; i++) {
        CnPerfPhase phase = (CnPerfPhase)i;
        uint64_t duration_us = cn_perf_get_duration(stats, phase);
        
        if (duration_us == 0) {
            continue;
        }

        fprintf(f, "%s,%.3f,%.2f\n",
                cn_perf_phase_name(phase),
                cn_perf_get_duration_ms(stats, phase),
                cn_perf_get_percentage(stats, phase));
    }

    fclose(f);
    return true;
}
