#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "cnlang/support/process/process.h"

/* 前置声明 */
static bool integration_perf_file_exists(const char *filename);

/*
 * 编译器性能分析集成测试
 * 测试 cnc --perf 功能
 */

/* 获取可执行文件路径 */
static const char* get_cnc_path(void)
{
#ifdef _WIN32
    /* 在 Windows 上简单检查文件存在性 */
    if (integration_perf_file_exists("cnc.exe")) {
        return "cnc.exe";
    }
    if (integration_perf_file_exists("./cnc.exe")) {
        return "./cnc.exe";
    }
    if (integration_perf_file_exists("../cnc.exe")) {
        return "../cnc.exe";
    }
    if (integration_perf_file_exists("../../cnc.exe")) {
        return "../../cnc.exe";
    }
    return "cnc.exe";
#else
    if (access("cnc", X_OK) == 0) {
        return "cnc";
    }
    if (access("./cnc", X_OK) == 0) {
        return "./cnc";
    }
    if (access("../cnc", X_OK) == 0) {
        return "../cnc";
    }
    if (access("../../cnc", X_OK) == 0) {
        return "../../cnc";
    }
    return "cnc";
#endif
}

/* 检查文件是否存在 */
static bool integration_perf_file_exists(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

/* 测试基本性能分析功能 */
static void test_basic_perf(void)
{
    printf("测试 1: 基本性能分析功能\n");

    const char *cnc_path = get_cnc_path();
    const char *test_file = "../../examples/hello_world.cn";
    const char *perf_json = "test_perf_basic.json";

    if (!integration_perf_file_exists(test_file)) {
        printf("  跳过（测试文件不存在: %s）\n", test_file);
        return;
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s %s --perf --perf-output=%s", 
             cnc_path, test_file, perf_json);

    int result;
    bool success = cn_support_run_command(cmd, &result);
    
    assert(success);
    assert(result == 0);
    assert(integration_perf_file_exists(perf_json));

    /* 验证 JSON 内容 */
    FILE *f = fopen(perf_json, "r");
    assert(f != NULL);

    char line[1024];
    bool found_source_file = false;
    bool found_total_duration = false;
    bool found_phases = false;

    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "\"source_file\"")) {
            found_source_file = true;
        }
        if (strstr(line, "\"total_duration_ms\"")) {
            found_total_duration = true;
        }
        if (strstr(line, "\"phases\"")) {
            found_phases = true;
        }
    }

    fclose(f);

    assert(found_source_file);
    assert(found_total_duration);
    assert(found_phases);

    /* 清理 */
    remove(perf_json);

    printf("  ✓ 通过\n\n");
}

/* 测试 CSV 导出 */
static void test_csv_export(void)
{
    printf("测试 2: CSV 格式导出\n");

    const char *cnc_path = get_cnc_path();
    const char *test_file = "../../examples/hello_world.cn";
    const char *perf_csv = "test_perf_basic.csv";

    if (!integration_perf_file_exists(test_file)) {
        printf("  跳过（测试文件不存在: %s）\n", test_file);
        return;
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s %s --perf --perf-output=%s", 
             cnc_path, test_file, perf_csv);

    int result;
    bool success = cn_support_run_command(cmd, &result);
    
    assert(success);
    assert(result == 0);
    assert(integration_perf_file_exists(perf_csv));

    /* 验证 CSV 内容 */
    FILE *f = fopen(perf_csv, "r");
    assert(f != NULL);

    char line[1024];
    bool found_header = false;
    int line_count = 0;

    while (fgets(line, sizeof(line), f)) {
        line_count++;
        if (strstr(line, "Phase,Duration_ms,Percentage")) {
            found_header = true;
        }
    }

    fclose(f);

    assert(found_header);
    assert(line_count > 1);  /* 至少有表头和一条数据 */

    /* 清理 */
    remove(perf_csv);

    printf("  ✓ 通过\n\n");
}

/* 测试控制台输出 */
static void test_console_output(void)
{
    printf("测试 3: 控制台性能输出\n");

    const char *cnc_path = get_cnc_path();
    const char *test_file = "../../examples/hello_world.cn";

    if (!integration_perf_file_exists(test_file)) {
        printf("  跳过（测试文件不存在: %s）\n", test_file);
        return;
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s %s --perf", cnc_path, test_file);

    int result;
    bool success = cn_support_run_command(cmd, &result);
    
    assert(success);
    assert(result == 0);

    printf("  ✓ 通过\n\n");
}

/* 测试较大项目的性能分析 */
static void test_large_project_perf(void)
{
    printf("测试 4: 较大项目性能分析\n");

    const char *cnc_path = get_cnc_path();
    const char *test_file = "../../examples/function_examples.cn";
    const char *perf_json = "test_perf_large.json";

    if (!integration_perf_file_exists(test_file)) {
        printf("  跳过（测试文件不存在: %s）\n", test_file);
        return;
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s %s --perf --perf-output=%s", 
             cnc_path, test_file, perf_json);

    int result;
    bool success = cn_support_run_command(cmd, &result);
    
    assert(success);
    assert(result == 0);
    assert(integration_perf_file_exists(perf_json));

    /* 读取并验证性能数据 */
    FILE *f = fopen(perf_json, "r");
    assert(f != NULL);

    char line[1024];
    double total_duration = 0;

    while (fgets(line, sizeof(line), f)) {
        char *p = strstr(line, "\"total_duration_ms\":");
        if (p) {
            p += strlen("\"total_duration_ms\":");
            while (*p == ' ' || *p == '\t') p++;
            total_duration = atof(p);
            break;
        }
    }

    fclose(f);

    assert(total_duration > 0);
    printf("  总耗时: %.3f ms\n", total_duration);

    /* 清理 */
    remove(perf_json);

    printf("  ✓ 通过\n\n");
}

int main(void)
{
    printf("========== 编译器性能分析集成测试 ==========\n\n");

    test_basic_perf();
    test_csv_export();
    test_console_output();
    test_large_project_perf();

    printf("==========================================\n");
    printf("所有测试通过! ✓\n");
    printf("==========================================\n");

    return 0;
}
