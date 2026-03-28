#ifndef CN_SUPPORT_MEMORY_PROFILER_H
#define CN_SUPPORT_MEMORY_PROFILER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/*
 * 内存占用分析模块
 * 用于统计和记录编译器内部关键数据结构的内存占用情况
 */

#ifdef __cplusplus
extern "C" {
#endif

/* 内存统计类别 */
typedef enum {
    CN_MEM_CATEGORY_AST,         /* AST 节点 */
    CN_MEM_CATEGORY_SYMBOL,      /* 符号表 */
    CN_MEM_CATEGORY_IR,          /* IR 中间表示 */
    CN_MEM_CATEGORY_DIAGNOSTICS, /* 诊断信息 */
    CN_MEM_CATEGORY_OTHER,       /* 其他 */
    CN_MEM_CATEGORY_COUNT        /* 类别总数 */
} CnMemCategory;

/* 单个类别的内存统计信息 */
typedef struct {
    size_t allocation_count;     /* 分配次数 */
    size_t total_bytes;          /* 总分配字节数 */
    size_t peak_bytes;           /* 峰值字节数 */
    size_t current_bytes;        /* 当前占用字节数 */
} CnMemCategoryStats;

/* 全局内存统计信息 */
typedef struct {
    CnMemCategoryStats categories[CN_MEM_CATEGORY_COUNT];
    size_t total_allocation_count;  /* 总分配次数 */
    size_t total_bytes_allocated;   /* 总分配字节数 */
    size_t peak_memory_usage;       /* 峰值内存占用 */
    size_t current_memory_usage;    /* 当前内存占用 */
    bool enabled;                   /* 是否启用内存分析 */
} CnMemStats;

/* 初始化内存统计 */
void cn_mem_stats_init(CnMemStats *stats);

/* 启用/禁用内存统计 */
void cn_mem_stats_set_enabled(CnMemStats *stats, bool enabled);

/* 记录一次内存分配 */
void cn_mem_stats_record_alloc(CnMemStats *stats, CnMemCategory category, size_t bytes);

/* 记录一次内存释放 */
void cn_mem_stats_record_free(CnMemStats *stats, CnMemCategory category, size_t bytes);

/* 获取类别名称 */
const char* cn_mem_category_name(CnMemCategory category);

/* 打印内存统计报告到文件 */
void cn_mem_stats_print(const CnMemStats *stats, FILE *out);

/* 导出内存统计到 JSON 格式文件 */
bool cn_mem_stats_export_json(const CnMemStats *stats, const char *filename);

/* 导出内存统计到 CSV 格式文件 */
bool cn_mem_stats_export_csv(const CnMemStats *stats, const char *filename);

/* 重置内存统计 */
void cn_mem_stats_reset(CnMemStats *stats);

/* 获取指定类别的统计信息 */
const CnMemCategoryStats* cn_mem_stats_get_category(const CnMemStats *stats, CnMemCategory category);

#ifdef __cplusplus
}
#endif

#endif /* CN_SUPPORT_MEMORY_PROFILER_H */
