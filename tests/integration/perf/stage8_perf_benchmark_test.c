/*
 * 阶段8性能基准测试
 * 
 * 测试目标：
 * 1. 测试新增语法特性（指针、结构体、枚举、函数指针等）的编译性能
 * 2. 生成性能基准数据，用于后续性能回归检测
 * 3. 验证性能分析工具的准确性
 * 
 * 验收标准：
 * - 所有阶段8特性的编译时间应在可接受范围内（< 100ms for small files）
 * - 各编译阶段的耗时分布合理（词法<20%，语法<30%，语义<30%，代码生成<20%）
 * - 性能数据可导出为JSON/CSV格式
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#ifdef _WIN32
#include <direct.h>
#define PATH_SEP '\\'
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/stat.h>
#define PATH_SEP '/'
#endif

#include "cnlang/support/process/process.h"

/* 测试用例结构 */
typedef struct {
    const char *name;           /* 测试用例名称 */
    const char *source_file;    /* 源文件路径 */
    const char *feature;        /* 测试的语法特性 */
    double max_time_ms;         /* 最大允许编译时间（毫秒） */
} PerfTestCase;

/* 性能测试结果 */
typedef struct {
    const char *name;
    bool success;
    double total_time_ms;
    double lexer_time_ms;
    double parser_time_ms;
    double semantic_time_ms;
    double codegen_time_ms;
} PerfTestResult;

/* 阶段8性能测试用例 */
static const PerfTestCase stage8_test_cases[] = {
    /* 指针类型测试 */
    {
        "指针基础操作",
        "../../examples/syntax/pointers/pointer_example.cn",
        "指针",
        100.0
    },
    {
        "指针运算",
        "../../examples/syntax/pointers/pointer_arithmetic.cn",
        "指针",
        100.0
    },
    
    /* 结构体测试 */
    {
        "结构体完整示例",
        "../../examples/syntax/structs/struct_complete_example.cn",
        "结构体",
        150.0
    },
    {
        "结构体内存布局",
        "../../examples/syntax/structs/struct_memory_layout.cn",
        "结构体",
        100.0
    },
    
    /* 枚举测试 */
    {
        "枚举基础操作",
        "../../examples/syntax/enums/enum_basic.cn",
        "枚举",
        80.0
    },
    
    /* 函数指针测试 */
    {
        "函数指针示例",
        "../../examples/syntax/functions/function_pointer_example.cn",
        "函数指针",
        100.0
    },
    
    /* 预处理器测试 */
    {
        "预处理器宏定义",
        "../../examples/syntax/preprocessor/macro_basic.cn",
        "预处理器",
        80.0
    },
    {
        "预处理器条件编译",
        "../../examples/syntax/preprocessor/conditional_compilation.cn",
        "预处理器",
        90.0
    },
    
    /* 系统编程特性测试 */
    {
        "位操作",
        "../../examples/system/bitwise/bitwise_operations.cn",
        "位操作",
        80.0
    },
    {
        "内联汇编",
        "../../examples/system/inline-asm/inline_asm_basic.cn",
        "内联汇编",
        90.0
    },
    {
        "原子操作",
        "../../examples/system/concurrency/atomic_operations.cn",
        "原子操作",
        100.0
    },
    
    /* 综合测试 */
    {
        "内存管理器演示",
        "../../examples/system/memory/memory_manager_demo.cn",
        "综合",
        200.0
    },
    {
        "任务调度器演示",
        "../../examples/system/concurrency/task_scheduler_demo.cn",
        "综合",
        200.0
    },
    {
        "语法验收测试",
        "../../examples/tests/features/syntax_verification_test.cn",
        "综合",
        250.0
    }
};

static const size_t num_test_cases = sizeof(stage8_test_cases) / sizeof(stage8_test_cases[0]);

/* 查找cnc编译器路径 */
static const char* find_cnc_path(void)
{
    static char cnc_path[512];
    const char *candidates[] = {
        "cnc.exe",
        "cnc",
        "../cnc.exe",
        "../cnc",
        "../../cnc.exe",
        "../../cnc",
        "./build/cnc.exe",
        "./build/cnc",
        "./build/Debug/cnc.exe",
        "./build/Release/cnc.exe"
    };
    
    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        FILE *f = fopen(candidates[i], "r");
        if (f) {
            fclose(f);
            strncpy(cnc_path, candidates[i], sizeof(cnc_path) - 1);
            cnc_path[sizeof(cnc_path) - 1] = '\0';
            return cnc_path;
        }
    }
    
    return "cnc";
}

/* 检查文件是否存在 */
static bool file_exists(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

/* 解析性能JSON输出 */
static bool parse_perf_json(const char *json_path, PerfTestResult *result)
{
    FILE *f = fopen(json_path, "r");
    if (!f) {
        return false;
    }
    
    char line[1024];
    result->total_time_ms = 0;
    result->lexer_time_ms = 0;
    result->parser_time_ms = 0;
    result->semantic_time_ms = 0;
    result->codegen_time_ms = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "\"total_duration_ms\":")) {
            sscanf(line, " \"total_duration_ms\": %lf", &result->total_time_ms);
        } else if (strstr(line, "\"lexer_duration_ms\":")) {
            sscanf(line, " \"lexer_duration_ms\": %lf", &result->lexer_time_ms);
        } else if (strstr(line, "\"parser_duration_ms\":")) {
            sscanf(line, " \"parser_duration_ms\": %lf", &result->parser_time_ms);
        } else if (strstr(line, "\"semantic_duration_ms\":")) {
            sscanf(line, " \"semantic_duration_ms\": %lf", &result->semantic_time_ms);
        } else if (strstr(line, "\"codegen_duration_ms\":")) {
            sscanf(line, " \"codegen_duration_ms\": %lf", &result->codegen_time_ms);
        }
    }
    
    fclose(f);
    return result->total_time_ms > 0;
}

/* 运行单个性能测试 */
static bool run_perf_test(const PerfTestCase *test_case, PerfTestResult *result)
{
    result->name = test_case->name;
    result->success = false;
    
    /* 检查源文件是否存在 */
    if (!file_exists(test_case->source_file)) {
        printf("    ⚠ 源文件不存在，跳过: %s\n", test_case->source_file);
        return false;
    }
    
    /* 准备编译命令 */
    const char *cnc_path = find_cnc_path();
    char perf_json[512];
    snprintf(perf_json, sizeof(perf_json), "test_perf_%s.json", test_case->name);
    
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "%s %s --perf --perf-output=%s",
             cnc_path, test_case->source_file, perf_json);
    
    /* 执行编译 */
    int exit_code;
    bool success = cn_support_run_command(cmd, &exit_code);
    
    if (!success || exit_code != 0) {
        printf("    ✗ 编译失败: %s (exit code: %d)\n", test_case->name, exit_code);
        return false;
    }
    
    /* 解析性能数据 */
    if (!parse_perf_json(perf_json, result)) {
        printf("    ✗ 无法解析性能数据: %s\n", test_case->name);
        return false;
    }
    
    /* 验证性能是否在允许范围内 */
    if (result->total_time_ms > test_case->max_time_ms) {
        printf("    ⚠ 性能警告: %s (%.3f ms > %.3f ms)\n",
               test_case->name, result->total_time_ms, test_case->max_time_ms);
    }
    
    result->success = true;
    return true;
}

/* 打印性能测试报告 */
static void print_perf_report(const PerfTestResult *results, size_t count)
{
    printf("\n========== 阶段8性能基准测试报告 ==========\n");
    printf("测试用例数: %zu\n", count);
    
    size_t success_count = 0;
    size_t skip_count = 0;
    double total_time = 0;
    double max_time = 0;
    double min_time = -1;
    
    for (size_t i = 0; i < count; i++) {
        if (results[i].success) {
            success_count++;
            total_time += results[i].total_time_ms;
            
            if (results[i].total_time_ms > max_time) {
                max_time = results[i].total_time_ms;
            }
            if (min_time < 0 || results[i].total_time_ms < min_time) {
                min_time = results[i].total_time_ms;
            }
        } else {
            skip_count++;
        }
    }
    
    printf("成功: %zu / %zu\n", success_count, count);
    printf("跳过: %zu / %zu\n", skip_count, count);
    
    if (success_count > 0) {
        printf("总编译时间: %.3f ms\n", total_time);
        printf("平均编译时间: %.3f ms\n", total_time / success_count);
        printf("最快编译时间: %.3f ms\n", min_time);
        printf("最慢编译时间: %.3f ms\n", max_time);
    }
    
    printf("=========================================\n\n");
    
    /* 详细结果 */
    printf("%-40s %12s %12s %12s %12s %12s\n",
           "测试用例", "总时间(ms)", "词法(ms)", "语法(ms)", "语义(ms)", "代码生成(ms)");
    printf("-------------------------------------------------------------------------------------\n");
    
    for (size_t i = 0; i < count; i++) {
        if (results[i].success) {
            printf("%-40s %12.3f %12.3f %12.3f %12.3f %12.3f\n",
                   results[i].name,
                   results[i].total_time_ms,
                   results[i].lexer_time_ms,
                   results[i].parser_time_ms,
                   results[i].semantic_time_ms,
                   results[i].codegen_time_ms);
        }
    }
    printf("=====================================================================================\n");
}

/* 导出CSV报告 */
static bool export_csv_report(const PerfTestResult *results, size_t count, const char *filename)
{
    FILE *f = fopen(filename, "w");
    if (!f) {
        return false;
    }
    
    fprintf(f, "TestCase,Feature,TotalTime_ms,Lexer_ms,Parser_ms,Semantic_ms,Codegen_ms\n");
    
    for (size_t i = 0; i < count; i++) {
        if (results[i].success) {
            fprintf(f, "%s,%s,%.3f,%.3f,%.3f,%.3f,%.3f\n",
                    results[i].name,
                    stage8_test_cases[i].feature,
                    results[i].total_time_ms,
                    results[i].lexer_time_ms,
                    results[i].parser_time_ms,
                    results[i].semantic_time_ms,
                    results[i].codegen_time_ms);
        }
    }
    
    fclose(f);
    return true;
}

int main(void)
{
    printf("===== 阶段8性能基准测试 =====\n\n");
    
    PerfTestResult *results = (PerfTestResult *)malloc(num_test_cases * sizeof(PerfTestResult));
    if (!results) {
        fprintf(stderr, "内存分配失败\n");
        return 1;
    }
    
    /* 运行所有测试用例 */
    printf("运行 %zu 个性能测试用例...\n\n", num_test_cases);
    
    for (size_t i = 0; i < num_test_cases; i++) {
        printf("[%zu/%zu] 测试: %s (%s)\n",
               i + 1, num_test_cases,
               stage8_test_cases[i].name,
               stage8_test_cases[i].feature);
        
        run_perf_test(&stage8_test_cases[i], &results[i]);
        printf("\n");
    }
    
    /* 打印报告 */
    print_perf_report(results, num_test_cases);
    
    /* 导出CSV */
    const char *csv_file = "stage8_perf_benchmark.csv";
    if (export_csv_report(results, num_test_cases, csv_file)) {
        printf("✓ CSV报告已导出: %s\n", csv_file);
    }
    
    free(results);
    
    printf("\n✅ 阶段8性能基准测试完成\n");
    return 0;
}
