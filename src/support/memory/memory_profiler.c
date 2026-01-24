#include "cnlang/support/memory_profiler.h"
#include <string.h>

/* 初始化内存统计 */
void cn_mem_stats_init(CnMemStats *stats)
{
    if (!stats) {
        return;
    }
    
    memset(stats, 0, sizeof(CnMemStats));
    stats->enabled = false;
}

/* 启用/禁用内存统计 */
void cn_mem_stats_set_enabled(CnMemStats *stats, bool enabled)
{
    if (stats) {
        stats->enabled = enabled;
    }
}

/* 记录一次内存分配 */
void cn_mem_stats_record_alloc(CnMemStats *stats, CnMemCategory category, size_t bytes)
{
    if (!stats || !stats->enabled || category >= CN_MEM_CATEGORY_COUNT) {
        return;
    }
    
    CnMemCategoryStats *cat_stats = &stats->categories[category];
    
    /* 更新类别统计 */
    cat_stats->allocation_count++;
    cat_stats->total_bytes += bytes;
    cat_stats->current_bytes += bytes;
    
    if (cat_stats->current_bytes > cat_stats->peak_bytes) {
        cat_stats->peak_bytes = cat_stats->current_bytes;
    }
    
    /* 更新全局统计 */
    stats->total_allocation_count++;
    stats->total_bytes_allocated += bytes;
    stats->current_memory_usage += bytes;
    
    if (stats->current_memory_usage > stats->peak_memory_usage) {
        stats->peak_memory_usage = stats->current_memory_usage;
    }
}

/* 记录一次内存释放 */
void cn_mem_stats_record_free(CnMemStats *stats, CnMemCategory category, size_t bytes)
{
    if (!stats || !stats->enabled || category >= CN_MEM_CATEGORY_COUNT) {
        return;
    }
    
    CnMemCategoryStats *cat_stats = &stats->categories[category];
    
    /* 更新类别统计 */
    if (cat_stats->current_bytes >= bytes) {
        cat_stats->current_bytes -= bytes;
    } else {
        cat_stats->current_bytes = 0;
    }
    
    /* 更新全局统计 */
    if (stats->current_memory_usage >= bytes) {
        stats->current_memory_usage -= bytes;
    } else {
        stats->current_memory_usage = 0;
    }
}

/* 获取类别名称 */
const char* cn_mem_category_name(CnMemCategory category)
{
    switch (category) {
        case CN_MEM_CATEGORY_AST:
            return "AST 节点";
        case CN_MEM_CATEGORY_SYMBOL:
            return "符号表";
        case CN_MEM_CATEGORY_IR:
            return "IR 中间表示";
        case CN_MEM_CATEGORY_DIAGNOSTICS:
            return "诊断信息";
        case CN_MEM_CATEGORY_OTHER:
            return "其他";
        default:
            return "未知";
    }
}

/* 辅助函数：将字节数转换为易读的字符串 */
static void format_bytes(size_t bytes, char *buffer, size_t buffer_size)
{
    if (bytes < 1024) {
        snprintf(buffer, buffer_size, "%zu B", bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(buffer, buffer_size, "%.2f KB", bytes / 1024.0);
    } else {
        snprintf(buffer, buffer_size, "%.2f MB", bytes / (1024.0 * 1024.0));
    }
}

/* 打印内存统计报告到文件 */
void cn_mem_stats_print(const CnMemStats *stats, FILE *out)
{
    char buffer[64];
    
    if (!stats || !out) {
        return;
    }
    
    fprintf(out, "\n========== 编译器内存占用分析报告 ==========\n");
    
    /* 打印总体统计 */
    format_bytes(stats->peak_memory_usage, buffer, sizeof(buffer));
    fprintf(out, "峰值内存占用: %s\n", buffer);
    
    format_bytes(stats->current_memory_usage, buffer, sizeof(buffer));
    fprintf(out, "当前内存占用: %s\n", buffer);
    
    format_bytes(stats->total_bytes_allocated, buffer, sizeof(buffer));
    fprintf(out, "累计分配总量: %s\n", buffer);
    
    fprintf(out, "总分配次数: %zu\n", stats->total_allocation_count);
    fprintf(out, "==========================================\n\n");
    
    /* 打印各类别详细统计 */
    fprintf(out, "%-20s %12s %12s %12s %12s\n",
            "数据结构", "分配次数", "累计分配", "峰值占用", "当前占用");
    fprintf(out, "--------------------------------------------------------------------------------\n");
    
    for (int i = 0; i < CN_MEM_CATEGORY_COUNT; i++) {
        CnMemCategory category = (CnMemCategory)i;
        const CnMemCategoryStats *cat_stats = &stats->categories[category];
        
        /* 跳过没有分配的类别 */
        if (cat_stats->allocation_count == 0) {
            continue;
        }
        
        char total_buf[64], peak_buf[64], current_buf[64];
        format_bytes(cat_stats->total_bytes, total_buf, sizeof(total_buf));
        format_bytes(cat_stats->peak_bytes, peak_buf, sizeof(peak_buf));
        format_bytes(cat_stats->current_bytes, current_buf, sizeof(current_buf));
        
        fprintf(out, "%-20s %12zu %12s %12s %12s\n",
                cn_mem_category_name(category),
                cat_stats->allocation_count,
                total_buf,
                peak_buf,
                current_buf);
    }
    
    fprintf(out, "================================================================================\n");
}

/* 导出内存统计到 JSON 格式文件 */
bool cn_mem_stats_export_json(const CnMemStats *stats, const char *filename)
{
    if (!stats || !filename) {
        return false;
    }
    
    FILE *f = fopen(filename, "w");
    if (!f) {
        return false;
    }
    
    fprintf(f, "{\n");
    fprintf(f, "  \"peak_memory_bytes\": %zu,\n", stats->peak_memory_usage);
    fprintf(f, "  \"current_memory_bytes\": %zu,\n", stats->current_memory_usage);
    fprintf(f, "  \"total_allocated_bytes\": %zu,\n", stats->total_bytes_allocated);
    fprintf(f, "  \"total_allocation_count\": %zu,\n", stats->total_allocation_count);
    fprintf(f, "  \"categories\": [\n");
    
    bool first = true;
    for (int i = 0; i < CN_MEM_CATEGORY_COUNT; i++) {
        CnMemCategory category = (CnMemCategory)i;
        const CnMemCategoryStats *cat_stats = &stats->categories[category];
        
        if (cat_stats->allocation_count == 0) {
            continue;
        }
        
        if (!first) {
            fprintf(f, ",\n");
        }
        first = false;
        
        fprintf(f, "    {\n");
        fprintf(f, "      \"name\": \"%s\",\n", cn_mem_category_name(category));
        fprintf(f, "      \"allocation_count\": %zu,\n", cat_stats->allocation_count);
        fprintf(f, "      \"total_bytes\": %zu,\n", cat_stats->total_bytes);
        fprintf(f, "      \"peak_bytes\": %zu,\n", cat_stats->peak_bytes);
        fprintf(f, "      \"current_bytes\": %zu\n", cat_stats->current_bytes);
        fprintf(f, "    }");
    }
    
    fprintf(f, "\n  ]\n");
    fprintf(f, "}\n");
    
    fclose(f);
    return true;
}

/* 导出内存统计到 CSV 格式文件 */
bool cn_mem_stats_export_csv(const CnMemStats *stats, const char *filename)
{
    if (!stats || !filename) {
        return false;
    }
    
    FILE *f = fopen(filename, "w");
    if (!f) {
        return false;
    }
    
    /* CSV 表头 */
    fprintf(f, "Category,AllocationCount,TotalBytes,PeakBytes,CurrentBytes\n");
    
    /* 各类别数据 */
    for (int i = 0; i < CN_MEM_CATEGORY_COUNT; i++) {
        CnMemCategory category = (CnMemCategory)i;
        const CnMemCategoryStats *cat_stats = &stats->categories[category];
        
        if (cat_stats->allocation_count == 0) {
            continue;
        }
        
        fprintf(f, "%s,%zu,%zu,%zu,%zu\n",
                cn_mem_category_name(category),
                cat_stats->allocation_count,
                cat_stats->total_bytes,
                cat_stats->peak_bytes,
                cat_stats->current_bytes);
    }
    
    fclose(f);
    return true;
}

/* 重置内存统计 */
void cn_mem_stats_reset(CnMemStats *stats)
{
    if (!stats) {
        return;
    }
    
    bool was_enabled = stats->enabled;
    cn_mem_stats_init(stats);
    stats->enabled = was_enabled;
}

/* 获取指定类别的统计信息 */
const CnMemCategoryStats* cn_mem_stats_get_category(const CnMemStats *stats, CnMemCategory category)
{
    if (!stats || category >= CN_MEM_CATEGORY_COUNT) {
        return NULL;
    }
    
    return &stats->categories[category];
}
