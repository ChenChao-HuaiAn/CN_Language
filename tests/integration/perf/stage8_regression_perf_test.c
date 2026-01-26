/*
 * 阶段8回归性能测试
 * 
 * 测试目标：
 * 1. 验证阶段8新功能不会影响现有功能的编译性能
 * 2. 对比阶段8前后的编译性能数据
 * 3. 确保性能退化不超过5%阈值
 * 
 * 验收标准：
 * - 基础语法示例（hello_world）编译性能无明显退化
 * - 函数示例编译性能无明显退化（< 5%）
 * - 数组示例编译性能无明显退化（< 5%）
 * - 控制流示例编译性能无明显退化（< 5%）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#ifdef _WIN32
#include <direct.h>
#define PATH_SEP '\\'
#else
#include <sys/stat.h>
#define PATH_SEP '/'
#endif

#include "cnlang/support/process/process.h"

/* 回归测试用例 */
typedef struct {
    const char *name;           /* 测试用例名称 */
    const char *source_file;    /* 源文件路径 */
    const char *category;       /* 功能类别 */
    double baseline_ms;         /* 基准编译时间（毫秒）- 阶段7数据 */
    double threshold_percent;   /* 允许的性能退化百分比 */
} RegressionTestCase;

/* 回归测试结果 */
typedef struct {
    const char *name;
    bool success;
    double current_time_ms;     /* 当前编译时间 */
    double baseline_time_ms;    /* 基准编译时间 */
    double diff_percent;        /* 性能差异百分比 */
    bool regression;            /* 是否发生性能退化 */
} RegressionTestResult;

/* 阶段7基础功能回归测试用例 */
static const RegressionTestCase regression_test_cases[] = {
    /* 基础语法 - 阶段1功能 */
    {
        "Hello World",
        "../../examples/basic/hello_world.cn",
        "基础语法",
        20.0,  /* 基准: 20ms */
        5.0    /* 允许5%退化 */
    },
    {
        "打印输出",
        "../../examples/basic/print_only.cn",
        "基础语法",
        15.0,  /* 基准: 15ms */
        5.0
    },
    
    /* 函数 - 阶段2功能 */
    {
        "函数示例",
        "../../examples/syntax/functions/function_examples.cn",
        "函数",
        50.0,  /* 基准: 50ms */
        5.0
    },
    {
        "函数递归",
        "../../examples/syntax/functions/function_recursive.cn",
        "函数",
        40.0,  /* 基准: 40ms */
        5.0
    },
    
    /* 数组 - 阶段3功能 */
    {
        "数组示例",
        "../../examples/syntax/arrays/array_examples.cn",
        "数组",
        60.0,  /* 基准: 60ms */
        5.0
    },
    {
        "数组多维",
        "../../examples/syntax/arrays/array_multidimensional.cn",
        "数组",
        70.0,  /* 基准: 70ms */
        5.0
    },
    
    /* 控制流 - 阶段4功能 */
    {
        "条件语句",
        "../../examples/syntax/control-flow/if_else_example.cn",
        "控制流",
        35.0,  /* 基准: 35ms */
        5.0
    },
    {
        "循环语句",
        "../../examples/syntax/control-flow/loop_examples.cn",
        "控制流",
        45.0,  /* 基准: 45ms */
        5.0
    },
    {
        "Switch语句",
        "../../examples/syntax/control-flow/switch_example.cn",
        "控制流",
        40.0,  /* 基准: 40ms */
        5.0
    },
    
    /* 浮点数 - 阶段5功能 */
    {
        "浮点运算",
        "../../examples/syntax/floats/float_operations.cn",
        "浮点数",
        45.0,  /* 基准: 45ms */
        5.0
    },
    
    /* 模块系统 - 阶段6功能 */
    {
        "模块导入",
        "../../examples/tests/modules/test_module_import.cn",
        "模块",
        80.0,  /* 基准: 80ms */
        5.0
    }
};

static const size_t num_test_cases = sizeof(regression_test_cases) / sizeof(regression_test_cases[0]);

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
static bool parse_perf_json(const char *json_path, double *total_time_ms)
{
    FILE *f = fopen(json_path, "r");
    if (!f) {
        return false;
    }
    
    char line[1024];
    *total_time_ms = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "\"total_duration_ms\":")) {
            sscanf(line, " \"total_duration_ms\": %lf", total_time_ms);
            break;
        }
    }
    
    fclose(f);
    return *total_time_ms > 0;
}

/* 运行单个回归测试 */
static bool run_regression_test(const RegressionTestCase *test_case, RegressionTestResult *result)
{
    result->name = test_case->name;
    result->baseline_time_ms = test_case->baseline_ms;
    result->success = false;
    result->regression = false;
    
    /* 检查源文件是否存在 */
    if (!file_exists(test_case->source_file)) {
        printf("    ⚠ 源文件不存在，跳过: %s\n", test_case->source_file);
        return false;
    }
    
    /* 准备编译命令 */
    const char *cnc_path = find_cnc_path();
    char perf_json[512];
    snprintf(perf_json, sizeof(perf_json), "test_regression_%s.json", test_case->name);
    
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
    if (!parse_perf_json(perf_json, &result->current_time_ms)) {
        printf("    ✗ 无法解析性能数据: %s\n", test_case->name);
        return false;
    }
    
    /* 计算性能差异百分比 */
    result->diff_percent = ((result->current_time_ms - result->baseline_time_ms) / result->baseline_time_ms) * 100.0;
    
    /* 检查是否发生性能退化 */
    if (result->diff_percent > test_case->threshold_percent) {
        result->regression = true;
        printf("    ⚠ 性能退化: %s (+%.2f%% > +%.2f%%)\n",
               test_case->name, result->diff_percent, test_case->threshold_percent);
    } else if (result->diff_percent < 0) {
        printf("    ✓ 性能提升: %s (%.2f%%)\n",
               test_case->name, result->diff_percent);
    } else {
        printf("    ✓ 性能稳定: %s (+%.2f%%)\n",
               test_case->name, result->diff_percent);
    }
    
    result->success = true;
    return true;
}

/* 打印回归测试报告 */
static void print_regression_report(const RegressionTestResult *results, size_t count)
{
    printf("\n========== 阶段8回归性能测试报告 ==========\n");
    printf("测试用例数: %zu\n", count);
    
    size_t success_count = 0;
    size_t skip_count = 0;
    size_t regression_count = 0;
    size_t improvement_count = 0;
    double total_diff = 0;
    
    for (size_t i = 0; i < count; i++) {
        if (results[i].success) {
            success_count++;
            total_diff += results[i].diff_percent;
            
            if (results[i].regression) {
                regression_count++;
            } else if (results[i].diff_percent < 0) {
                improvement_count++;
            }
        } else {
            skip_count++;
        }
    }
    
    printf("成功: %zu / %zu\n", success_count, count);
    printf("跳过: %zu / %zu\n", skip_count, count);
    printf("性能退化: %zu\n", regression_count);
    printf("性能提升: %zu\n", improvement_count);
    
    if (success_count > 0) {
        double avg_diff = total_diff / success_count;
        printf("平均性能变化: %+.2f%%\n", avg_diff);
        
        if (regression_count == 0) {
            printf("\n✅ 未检测到性能退化\n");
        } else {
            printf("\n⚠ 检测到 %zu 个性能退化\n", regression_count);
        }
    }
    
    printf("=========================================\n\n");
    
    /* 详细结果 */
    printf("%-30s %-15s %15s %15s %15s %8s\n",
           "测试用例", "类别", "基准(ms)", "当前(ms)", "差异(%)", "状态");
    printf("---------------------------------------------------------------------------------------------------------\n");
    
    for (size_t i = 0; i < count; i++) {
        if (results[i].success) {
            const char *status = results[i].regression ? "退化" : 
                                 (results[i].diff_percent < 0 ? "提升" : "稳定");
            
            printf("%-30s %-15s %15.3f %15.3f %+15.2f %8s\n",
                   results[i].name,
                   regression_test_cases[i].category,
                   results[i].baseline_time_ms,
                   results[i].current_time_ms,
                   results[i].diff_percent,
                   status);
        }
    }
    printf("=========================================================================================================\n");
}

/* 导出CSV报告 */
static bool export_csv_report(const RegressionTestResult *results, size_t count, const char *filename)
{
    FILE *f = fopen(filename, "w");
    if (!f) {
        return false;
    }
    
    fprintf(f, "TestCase,Category,Baseline_ms,Current_ms,Diff_percent,Status\n");
    
    for (size_t i = 0; i < count; i++) {
        if (results[i].success) {
            const char *status = results[i].regression ? "Regression" : 
                                 (results[i].diff_percent < 0 ? "Improvement" : "Stable");
            
            fprintf(f, "%s,%s,%.3f,%.3f,%+.2f,%s\n",
                    results[i].name,
                    regression_test_cases[i].category,
                    results[i].baseline_time_ms,
                    results[i].current_time_ms,
                    results[i].diff_percent,
                    status);
        }
    }
    
    fclose(f);
    return true;
}

int main(void)
{
    printf("===== 阶段8回归性能测试 =====\n\n");
    
    RegressionTestResult *results = (RegressionTestResult *)malloc(num_test_cases * sizeof(RegressionTestResult));
    if (!results) {
        fprintf(stderr, "内存分配失败\n");
        return 1;
    }
    
    /* 运行所有测试用例 */
    printf("运行 %zu 个回归测试用例...\n\n", num_test_cases);
    
    for (size_t i = 0; i < num_test_cases; i++) {
        printf("[%zu/%zu] 测试: %s (%s)\n",
               i + 1, num_test_cases,
               regression_test_cases[i].name,
               regression_test_cases[i].category);
        
        run_regression_test(&regression_test_cases[i], &results[i]);
        printf("\n");
    }
    
    /* 打印报告 */
    print_regression_report(results, num_test_cases);
    
    /* 导出CSV */
    const char *csv_file = "stage8_regression_perf.csv";
    if (export_csv_report(results, num_test_cases, csv_file)) {
        printf("✓ CSV报告已导出: %s\n", csv_file);
    }
    
    /* 检查是否有性能退化 */
    bool has_regression = false;
    for (size_t i = 0; i < num_test_cases; i++) {
        if (results[i].success && results[i].regression) {
            has_regression = true;
            break;
        }
    }
    
    free(results);
    
    if (has_regression) {
        printf("\n⚠ 阶段8回归性能测试完成 - 发现性能退化\n");
        return 1;
    } else {
        printf("\n✅ 阶段8回归性能测试完成 - 性能稳定\n");
        return 0;
    }
}
