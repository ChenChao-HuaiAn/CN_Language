#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEP '\\'
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/stat.h>
#include <dirent.h>
#define PATH_SEP '/'
#endif

#include "cnlang/support/process/process.h"

/*
 * cnperf - CN Language 编译器性能分析工具
 * 用于批量测试多个 .cn 文件的编译性能
 */

typedef struct {
    char filename[512];
    double total_time_ms;
    size_t file_size;
} PerfResult;

typedef struct {
    PerfResult *results;
    size_t count;
    size_t capacity;
} PerfResultList;

/* 初始化结果列表 */
static void perf_result_list_init(PerfResultList *list)
{
    list->results = NULL;
    list->count = 0;
    list->capacity = 0;
}

/* 添加结果 */
static void perf_result_list_add(PerfResultList *list, const char *filename, 
                                  double total_time_ms, size_t file_size)
{
    if (list->count >= list->capacity) {
        size_t new_capacity = (list->capacity == 0) ? 16 : list->capacity * 2;
        PerfResult *new_results = (PerfResult *)realloc(list->results, 
                                                         new_capacity * sizeof(PerfResult));
        if (!new_results) {
            fprintf(stderr, "内存分配失败\n");
            return;
        }
        list->results = new_results;
        list->capacity = new_capacity;
    }

    PerfResult *r = &list->results[list->count++];
    strncpy(r->filename, filename, sizeof(r->filename) - 1);
    r->filename[sizeof(r->filename) - 1] = '\0';
    r->total_time_ms = total_time_ms;
    r->file_size = file_size;
}

/* 释放结果列表 */
static void perf_result_list_free(PerfResultList *list)
{
    if (list->results) {
        free(list->results);
        list->results = NULL;
    }
    list->count = 0;
    list->capacity = 0;
}

/* 获取文件大小 */
static size_t get_file_size(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f) {
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);

    return (size < 0) ? 0 : (size_t)size;
}

/* 运行编译器并获取性能数据 */
static bool run_compiler_with_perf(const char *cnc_path, const char *cn_file, 
                                    const char *perf_json_path, double *out_total_time_ms)
{
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "%s %s --perf --perf-output=%s", 
             cnc_path, cn_file, perf_json_path);

    int result;
    bool success = cn_support_run_command(cmd, &result);
    if (!success || result != 0) {
        return false;
    }

    /* 读取 JSON 文件并解析 total_duration_ms */
    FILE *f = fopen(perf_json_path, "r");
    if (!f) {
        return false;
    }

    char line[1024];
    bool found = false;
    while (fgets(line, sizeof(line), f)) {
        char *p = strstr(line, "\"total_duration_ms\":");
        if (p) {
            p += strlen("\"total_duration_ms\":");
            while (*p == ' ' || *p == '\t') p++;
            
            *out_total_time_ms = atof(p);
            found = true;
            break;
        }
    }

    fclose(f);
    return found;
}

/* 递归扫描目录中的 .cn 文件 */
static void scan_directory(const char *dir_path, PerfResultList *file_list)
{
#ifdef _WIN32
    WIN32_FIND_DATAA find_data;
    char search_path[512];
    snprintf(search_path, sizeof(search_path), "%s\\*", dir_path);

    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s\\%s", dir_path, find_data.cFileName);

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            scan_directory(full_path, file_list);
        } else {
            /* 检查是否是 .cn 文件 */
            size_t len = strlen(find_data.cFileName);
            if (len > 3 && strcmp(find_data.cFileName + len - 3, ".cn") == 0) {
                perf_result_list_add(file_list, full_path, 0, get_file_size(full_path));
            }
        }
    } while (FindNextFileA(hFind, &find_data));

    FindClose(hFind);
#else
    DIR *dir = opendir(dir_path);
    if (!dir) {
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            scan_directory(full_path, &st);
        } else {
            /* 检查是否是 .cn 文件 */
            size_t len = strlen(entry->d_name);
            if (len > 3 && strcmp(entry->d_name + len - 3, ".cn") == 0) {
                perf_result_list_add(file_list, full_path, 0, get_file_size(full_path));
            }
        }
    }

    closedir(dir);
#endif
}

/* 打印汇总报告 */
static void print_summary_report(const PerfResultList *results)
{
    if (results->count == 0) {
        printf("\n没有测试文件\n");
        return;
    }

    double total_time = 0;
    size_t total_size = 0;
    double min_time = results->results[0].total_time_ms;
    double max_time = results->results[0].total_time_ms;

    for (size_t i = 0; i < results->count; i++) {
        total_time += results->results[i].total_time_ms;
        total_size += results->results[i].file_size;
        
        if (results->results[i].total_time_ms < min_time) {
            min_time = results->results[i].total_time_ms;
        }
        if (results->results[i].total_time_ms > max_time) {
            max_time = results->results[i].total_time_ms;
        }
    }

    double avg_time = total_time / results->count;

    printf("\n========== 编译性能汇总报告 ==========\n");
    printf("测试文件数: %zu\n", results->count);
    printf("总源代码大小: %zu 字节 (%.2f KB)\n", total_size, total_size / 1024.0);
    printf("总编译耗时: %.3f ms (%.3f s)\n", total_time, total_time / 1000.0);
    printf("平均编译耗时: %.3f ms\n", avg_time);
    printf("最短编译耗时: %.3f ms\n", min_time);
    printf("最长编译耗时: %.3f ms\n", max_time);
    printf("======================================\n\n");

    /* 打印每个文件的耗时 */
    printf("%-50s %12s %12s\n", "文件名", "大小(字节)", "耗时(ms)");
    printf("--------------------------------------------------------------------------------\n");
    for (size_t i = 0; i < results->count; i++) {
        printf("%-50s %12zu %12.3f\n", 
               results->results[i].filename,
               results->results[i].file_size,
               results->results[i].total_time_ms);
    }
    printf("================================================================================\n");
}

/* 导出汇总报告到 CSV */
static bool export_summary_csv(const PerfResultList *results, const char *output_file)
{
    FILE *f = fopen(output_file, "w");
    if (!f) {
        return false;
    }

    fprintf(f, "Filename,FileSize,TotalTime_ms\n");
    for (size_t i = 0; i < results->count; i++) {
        fprintf(f, "%s,%zu,%.3f\n",
                results->results[i].filename,
                results->results[i].file_size,
                results->results[i].total_time_ms);
    }

    fclose(f);
    return true;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "用法: %s <目录或文件> [选项]\n", argv[0]);
        fprintf(stderr, "选项:\n");
        fprintf(stderr, "  --cnc <路径>        指定 cnc 编译器路径（默认: cnc 或 cnc.exe）\n");
        fprintf(stderr, "  --output <文件>     导出汇总报告到 CSV 文件\n");
        fprintf(stderr, "  --help             显示此帮助信息\n");
        return 1;
    }

    const char *target_path = argv[1];
    const char *cnc_path = NULL;
    const char *output_file = NULL;

    /* 解析命令行参数 */
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--cnc") == 0 && i + 1 < argc) {
            cnc_path = argv[++i];
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0) {
            fprintf(stderr, "CN Performance Analyzer (cnperf) - 编译器性能分析工具\n\n");
            fprintf(stderr, "用法: %s <目录或文件> [选项]\n\n", argv[0]);
            fprintf(stderr, "选项:\n");
            fprintf(stderr, "  --cnc <路径>        指定 cnc 编译器路径（默认: cnc 或 cnc.exe）\n");
            fprintf(stderr, "  --output <文件>     导出汇总报告到 CSV 文件\n");
            fprintf(stderr, "  --help             显示此帮助信息\n\n");
            fprintf(stderr, "示例:\n");
            fprintf(stderr, "  %s examples                    # 分析 examples 目录下所有 .cn 文件\n", argv[0]);
            fprintf(stderr, "  %s test.cn --output=perf.csv   # 分析单个文件并导出到 CSV\n", argv[0]);
            return 0;
        }
    }

    /* 默认编译器路径 */
    if (!cnc_path) {
#ifdef _WIN32
        cnc_path = "cnc.exe";
#else
        cnc_path = "cnc";
#endif
    }

    /* 创建临时目录存储性能数据 */
    const char *temp_dir = "perf_temp";
    mkdir(temp_dir, 0755);

    PerfResultList file_list;
    perf_result_list_init(&file_list);

    /* 扫描文件 */
    printf("正在扫描文件...\n");
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(target_path);
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        scan_directory(target_path, &file_list);
    } else {
        /* 单个文件 */
        perf_result_list_add(&file_list, target_path, 0, get_file_size(target_path));
    }
#else
    struct stat st;
    if (stat(target_path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            scan_directory(target_path, &file_list);
        } else {
            /* 单个文件 */
            perf_result_list_add(&file_list, target_path, 0, get_file_size(target_path));
        }
    }
#endif

    if (file_list.count == 0) {
        fprintf(stderr, "未找到 .cn 文件\n");
        perf_result_list_free(&file_list);
        return 1;
    }

    printf("找到 %zu 个 .cn 文件\n\n", file_list.count);

    /* 逐个编译并收集性能数据 */
    for (size_t i = 0; i < file_list.count; i++) {
        printf("[%zu/%zu] 正在测试: %s ... ", i + 1, file_list.count, file_list.results[i].filename);
        fflush(stdout);

        char perf_json[1024];
        snprintf(perf_json, sizeof(perf_json), "%s%cperf_%zu.json", temp_dir, PATH_SEP, i);

        double total_time_ms = 0;
        if (run_compiler_with_perf(cnc_path, file_list.results[i].filename, perf_json, &total_time_ms)) {
            file_list.results[i].total_time_ms = total_time_ms;
            printf("完成 (%.3f ms)\n", total_time_ms);
        } else {
            printf("失败\n");
            file_list.results[i].total_time_ms = 0;
        }
    }

    /* 打印汇总报告 */
    print_summary_report(&file_list);

    /* 导出 CSV */
    if (output_file) {
        if (export_summary_csv(&file_list, output_file)) {
            printf("汇总报告已导出到: %s\n", output_file);
        } else {
            fprintf(stderr, "无法导出汇总报告到: %s\n", output_file);
        }
    }

    /* 清理 */
    perf_result_list_free(&file_list);

    return 0;
}
