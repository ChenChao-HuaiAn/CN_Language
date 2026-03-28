#ifndef CN_SUPPORT_PERF_H
#define CN_SUPPORT_PERF_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/*
 * 性能分析模块
 * 用于测量和记录编译器各阶段的性能数据
 */

#ifdef __cplusplus
extern "C" {
#endif

/* 编译阶段类型 */
typedef enum {
    CN_PERF_PHASE_TOTAL,           /* 总耗时 */
    CN_PERF_PHASE_LEXER,           /* 词法分析 */
    CN_PERF_PHASE_PARSER,          /* 语法分析 */
    CN_PERF_PHASE_SEMANTIC,        /* 语义分析（包含作用域构建、名称解析、类型检查） */
    CN_PERF_PHASE_SEMANTIC_SCOPE,  /* 语义分析 - 作用域构建 */
    CN_PERF_PHASE_SEMANTIC_RESOLVE,/* 语义分析 - 名称解析 */
    CN_PERF_PHASE_SEMANTIC_TYPECHECK, /* 语义分析 - 类型检查 */
    CN_PERF_PHASE_IR_GEN,          /* IR 生成 */
    CN_PERF_PHASE_IR_OPT,          /* IR 优化 */
    CN_PERF_PHASE_CODEGEN,         /* 代码生成 */
    CN_PERF_PHASE_BACKEND_COMPILE, /* 后端编译（调用外部编译器） */
    CN_PERF_PHASE_COUNT            /* 阶段数量 */
} CnPerfPhase;

/* 性能测量点 */
typedef struct {
    CnPerfPhase phase;
    uint64_t start_time_us;  /* 开始时间（微秒） */
    uint64_t end_time_us;    /* 结束时间（微秒） */
    uint64_t duration_us;    /* 持续时间（微秒） */
    bool is_active;          /* 是否正在测量 */
} CnPerfMeasurement;

/* 性能统计数据 */
typedef struct {
    CnPerfMeasurement measurements[CN_PERF_PHASE_COUNT];
    bool enabled;            /* 是否启用性能测量 */
    const char *source_file; /* 源文件名 */
    size_t source_size;      /* 源文件大小（字节） */
} CnPerfStats;

/* 获取当前时间戳（微秒） */
uint64_t cn_perf_get_timestamp_us(void);

/* 初始化性能统计 */
void cn_perf_stats_init(CnPerfStats *stats, const char *source_file, size_t source_size);

/* 启用/禁用性能测量 */
void cn_perf_stats_set_enabled(CnPerfStats *stats, bool enabled);

/* 开始测量某个阶段 */
void cn_perf_start(CnPerfStats *stats, CnPerfPhase phase);

/* 结束测量某个阶段 */
void cn_perf_end(CnPerfStats *stats, CnPerfPhase phase);

/* 获取某个阶段的耗时（微秒） */
uint64_t cn_perf_get_duration(const CnPerfStats *stats, CnPerfPhase phase);

/* 获取某个阶段的耗时（毫秒） */
double cn_perf_get_duration_ms(const CnPerfStats *stats, CnPerfPhase phase);

/* 获取某个阶段的耗时占比（百分比） */
double cn_perf_get_percentage(const CnPerfStats *stats, CnPerfPhase phase);

/* 获取阶段名称 */
const char* cn_perf_phase_name(CnPerfPhase phase);

/* 打印性能统计到文件 */
void cn_perf_print_stats(const CnPerfStats *stats, FILE *out);

/* 导出性能统计到 JSON 格式文件 */
bool cn_perf_export_json(const CnPerfStats *stats, const char *filename);

/* 导出性能统计到 CSV 格式文件 */
bool cn_perf_export_csv(const CnPerfStats *stats, const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* CN_SUPPORT_PERF_H */
