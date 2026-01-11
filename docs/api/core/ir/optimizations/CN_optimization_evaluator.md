# CN_optimization_evaluator - 优化效果评估器API文档

## 概述

`CN_optimization_evaluator` 模块提供了优化效果评估功能，用于评估优化前后的效果，计算改进比例，生成优化报告。该模块是CN_Language编译器优化系统的重要组成部分。

## 头文件

```c
#include "CN_optimization_evaluator.h"
```

## 数据结构

### Stru_OptimizationMetrics_t - 优化指标结构体

```c
typedef struct {
    double instruction_reduction_ratio;      ///< 指令减少比例
    double code_size_reduction_ratio;        ///< 代码大小减少比例
    double estimated_performance_improvement; ///< 估计性能改进
    double memory_usage_improvement;         ///< 内存使用改进
    double compilation_time_overhead;        ///< 编译时间开销
    double optimization_quality_score;       ///< 优化质量评分（0-100）
} Stru_OptimizationMetrics_t;
```

**字段说明：**
- `instruction_reduction_ratio`: 指令减少比例，范围0.0-1.0
- `code_size_reduction_ratio`: 代码大小减少比例，范围0.0-1.0
- `estimated_performance_improvement`: 估计性能改进，范围0.0-1.0
- `memory_usage_improvement`: 内存使用改进，范围0.0-1.0
- `compilation_time_overhead`: 编译时间开销，范围0.0-1.0
- `optimization_quality_score`: 优化质量评分，范围0-100

### Stru_OptimizationEvaluationResult_t - 优化评估结果结构体

```c
typedef struct {
    bool evaluation_successful;              ///< 评估是否成功
    Stru_OptimizationMetrics_t metrics;      ///< 优化指标
    char* evaluation_report;                 ///< 评估报告
    char* recommendations;                   ///< 优化建议
    double evaluation_time;                  ///< 评估时间（秒）
} Stru_OptimizationEvaluationResult_t;
```

**字段说明：**
- `evaluation_successful`: 评估是否成功
- `metrics`: 优化指标
- `evaluation_report`: 评估报告字符串，需要调用者释放
- `recommendations`: 优化建议字符串，需要调用者释放
- `evaluation_time`: 评估时间（秒）

### Stru_OptimizationBenchmark_t - 优化基准测试结构体

```c
typedef struct {
    const char* benchmark_name;              ///< 基准测试名称
    void* original_ir;                       ///< 原始IR
    void* optimized_ir;                      ///< 优化后IR
    size_t iteration_count;                  ///< 迭代次数
    double* execution_times;                 ///< 执行时间数组
    size_t time_count;                       ///< 时间数量
} Stru_OptimizationBenchmark_t;
```

**字段说明：**
- `benchmark_name`: 基准测试名称
- `original_ir`: 原始IR指针
- `optimized_ir`: 优化后IR指针
- `iteration_count`: 迭代次数
- `execution_times`: 执行时间数组，需要调用者释放
- `time_count`: 时间数量

### Stru_OptimizationEvaluatorInterface_t - 优化效果评估器接口结构体

```c
typedef struct Stru_OptimizationEvaluatorInterface_t {
    // 基本信息
    const char* (*get_name)(void);
    const char* (*get_version)(void);
    
    // 效果评估
    Stru_OptimizationEvaluationResult_t* (*evaluate_optimization)(
        void* original_ir,
        void* optimized_ir,
        Eum_IrOptimizationType opt_type);
    
    Stru_OptimizationEvaluationResult_t* (*evaluate_optimization_level)(
        void* original_ir,
        void* optimized_ir,
        Eum_IrOptimizationLevel level);
    
    // 基准测试
    bool (*run_benchmark)(Stru_OptimizationBenchmark_t* benchmark);
    Stru_OptimizationBenchmark_t* (*create_benchmark)(const char* name,
                                                     void* original_ir,
                                                     void* optimized_ir);
    void (*destroy_benchmark)(Stru_OptimizationBenchmark_t* benchmark);
    
    // 报告生成
    char* (*generate_evaluation_report)(const Stru_OptimizationEvaluationResult_t* result);
    bool (*save_report_to_file)(const Stru_OptimizationEvaluationResult_t* result,
                               const char* filename);
    
    // 统计分析
    bool (*analyze_optimization_trends)(Stru_OptimizationEvaluationResult_t** results,
                                       size_t count,
                                       Stru_OptimizationMetrics_t* trends);
    
    // 资源管理
    void (*destroy_evaluation_result)(Stru_OptimizationEvaluationResult_t* result);
    void (*destroy)(void);
    
    // 内部状态
    void* internal_state;
} Stru_OptimizationEvaluatorInterface_t;
```

## 工厂函数

### F_create_optimization_evaluator_interface

```c
Stru_OptimizationEvaluatorInterface_t* F_create_optimization_evaluator_interface(void);
```

**功能：** 创建优化效果评估器接口实例

**参数：** 无

**返回值：**
- 成功：返回评估器接口实例指针
- 失败：返回NULL

**示例：**
```c
Stru_OptimizationEvaluatorInterface_t* evaluator = 
    F_create_optimization_evaluator_interface();
if (!evaluator) {
    // 错误处理
}
```

### F_destroy_optimization_evaluator_interface

```c
void F_destroy_optimization_evaluator_interface(Stru_OptimizationEvaluatorInterface_t* evaluator);
```

**功能：** 销毁优化效果评估器接口实例及其所有资源

**参数：**
- `evaluator`: 评估器接口实例

**返回值：** 无

**示例：**
```c
F_destroy_optimization_evaluator_interface(evaluator);
```

## 工具函数

### F_create_optimization_metrics

```c
Stru_OptimizationMetrics_t* F_create_optimization_metrics(void);
```

**功能：** 创建新的优化指标结构体

**参数：** 无

**返回值：**
- 成功：返回新创建的优化指标指针
- 失败：返回NULL

**示例：**
```c
Stru_OptimizationMetrics_t* metrics = F_create_optimization_metrics();
if (!metrics) {
    // 错误处理
}
```

### F_destroy_optimization_metrics

```c
void F_destroy_optimization_metrics(Stru_OptimizationMetrics_t* metrics);
```

**功能：** 销毁优化指标结构体

**参数：**
- `metrics`: 优化指标

**返回值：** 无

**示例：**
```c
F_destroy_optimization_metrics(metrics);
```

### F_create_optimization_evaluation_result

```c
Stru_OptimizationEvaluationResult_t* F_create_optimization_evaluation_result(void);
```

**功能：** 创建新的优化评估结果

**参数：** 无

**返回值：**
- 成功：返回新创建的评估结果指针
- 失败：返回NULL

**示例：**
```c
Stru_OptimizationEvaluationResult_t* result = F_create_optimization_evaluation_result();
if (!result) {
    // 错误处理
}
```

### F_set_optimization_evaluation_result_error

```c
void F_set_optimization_evaluation_result_error(Stru_OptimizationEvaluationResult_t* result,
                                               const char* error_message);
```

**功能：** 设置评估结果的错误信息

**参数：**
- `result`: 评估结果
- `error_message`: 错误信息

**返回值：** 无

**示例：**
```c
F_set_optimization_evaluation_result_error(result, "IR参数无效");
```

### F_calculate_instruction_reduction_ratio

```c
double F_calculate_instruction_reduction_ratio(size_t original_count,
                                              size_t optimized_count);
```

**功能：** 计算优化后指令减少的比例

**参数：**
- `original_count`: 原始指令数量
- `optimized_count`: 优化后指令数量

**返回值：** 减少比例（0.0-1.0）

**示例：**
```c
double ratio = F_calculate_instruction_reduction_ratio(100, 80);
// ratio = 0.2 (减少了20%)
```

### F_estimate_performance_improvement

```c
double F_estimate_performance_improvement(const Stru_OptimizationMetrics_t* metrics,
                                         Eum_IrOptimizationType opt_type);
```

**功能：** 根据优化类型和指标估算性能改进

**参数：**
- `metrics`: 优化指标
- `opt_type`: 优化类型

**返回值：** 估计性能改进比例（0.0-1.0）

**示例：**
```c
double improvement = F_estimate_performance_improvement(metrics, Eum_IR_OPT_CONSTANT_FOLDING);
```

### F_calculate_optimization_quality_score

```c
double F_calculate_optimization_quality_score(const Stru_OptimizationMetrics_t* metrics);
```

**功能：** 计算优化的质量评分

**参数：**
- `metrics`: 优化指标

**返回值：** 质量评分（0-100）

**示例：**
```c
double score = F_calculate_optimization_quality_score(metrics);
// score = 85.5 (质量评分85.5)
```

### F_generate_optimization_recommendations

```c
char* F_generate_optimization_recommendations(const Stru_OptimizationEvaluationResult_t* result);
```

**功能：** 根据评估结果生成优化建议

**参数：**
- `result`: 评估结果

**返回值：**
- 成功：返回优化建议字符串，需要调用者释放
- 失败：返回NULL

**示例：**
```c
char* recommendations = F_generate_optimization_recommendations(result);
if (recommendations) {
    printf("优化建议: %s\n", recommendations);
    cn_free(recommendations);
}
```

## 版本信息函数

### F_get_optimization_evaluator_version

```c
void F_get_optimization_evaluator_version(int* major, int* minor, int* patch);
```

**功能：** 获取优化评估器的版本信息

**参数：**
- `major`: 输出参数，主版本号
- `minor`: 输出参数，次版本号
- `patch`: 输出参数，修订号

**返回值：** 无

**示例：**
```c
int major, minor, patch;
F_get_optimization_evaluator_version(&major, &minor, &patch);
printf("版本: %d.%d.%d\n", major, minor, patch);
```

### F_get_optimization_evaluator_version_string

```c
const char* F_get_optimization_evaluator_version_string(void);
```

**功能：** 获取优化评估器的版本字符串

**参数：** 无

**返回值：** 版本字符串

**示例：**
```c
const char* version = F_get_optimization_evaluator_version_string();
printf("版本: %s\n", version);
```

## 接口函数

### get_name

```c
const char* get_name(void);
```

**功能：** 获取评估器名称

**参数：** 无

**返回值：** 评估器名称

**示例：**
```c
const char* name = evaluator->get_name();
printf("评估器名称: %s\n", name);
```

### get_version

```c
const char* get_version(void);
```

**功能：** 获取评估器版本

**参数：** 无

**返回值：** 评估器版本

**示例：**
```c
const char* version = evaluator->get_version();
printf("评估器版本: %s\n", version);
```

### evaluate_optimization

```c
Stru_OptimizationEvaluationResult_t* evaluate_optimization(
    void* original_ir,
    void* optimized_ir,
    Eum_IrOptimizationType opt_type);
```

**功能：** 评估优化前后的效果，计算改进比例

**参数：**
- `original_ir`: 原始IR
- `optimized_ir`: 优化后IR
- `opt_type`: 优化类型

**返回值：**
- 成功：返回评估结果指针
- 失败：返回NULL

**示例：**
```c
Stru_OptimizationEvaluationResult_t* result = 
    evaluator->evaluate_optimization(original_ir, optimized_ir, 
                                     Eum_IR_OPT_CONSTANT_FOLDING);
```

### evaluate_optimization_level

```c
Stru_OptimizationEvaluationResult_t* evaluate_optimization_level(
    void* original_ir,
    void* optimized_ir,
    Eum_IrOptimizationLevel level);
```

**功能：** 评估优化级别前后的效果

**参数：**
- `original_ir`: 原始IR
- `optimized_ir`: 优化后IR
- `level`: 优化级别

**返回值：**
- 成功：返回评估结果指针
- 失败：返回NULL

**示例：**
```c
Stru_OptimizationEvaluationResult_t* result = 
    evaluator->evaluate_optimization_level(original_ir, optimized_ir, 
                                           Eum_IR_OPT_LEVEL_O2);
```

### run_benchmark

```c
bool run_benchmark(Stru_OptimizationBenchmark_t* benchmark);
```

**功能：** 运行优化基准测试

**参数：**
- `benchmark`: 基准测试

**返回值：**
- 成功：返回true
- 失败：返回false

**示例：**
```c
bool success = evaluator->run_benchmark(benchmark);
```

### create_benchmark

```c
Stru_OptimizationBenchmark_t* create_benchmark(const char* name,
                                              void* original_ir,
                                              void* optimized_ir);
```

**功能：** 创建新的优化基准测试

**参数：**
- `name`: 基准测试名称
- `original_ir`: 原始IR
- `optimized_ir`: 优化后IR

**返回值：**
- 成功：返回基准测试指针
- 失败：返回NULL

**示例：**
```c
Stru_OptimizationBenchmark_t* benchmark = 
    evaluator->create_benchmark("test_benchmark", original_ir, optimized_ir);
```

### destroy_benchmark

```c
void destroy_benchmark(Stru_OptimizationBenchmark_t* benchmark);
```

**功能：** 销毁基准测试及其所有资源

**参数：**
- `benchmark`: 基准测试

**返回值：** 无

**示例：**
```c
evaluator->destroy_benchmark(benchmark);
```

### generate_evaluation_report

```c
char* generate_evaluation_report(const Stru_OptimizationEvaluationResult_t* result);
```

**功能：** 生成优化评估的详细报告

**参数：**
- `result`: 评估结果

**返回值：**
- 成功：返回评估报告字符串，需要调用者释放
- 失败：返回NULL

**示例：**
```c
char* report = evaluator->generate_evaluation_report(result);
if (report) {
    printf("%s\n", report);
    cn_free(report);
}
```

### save_report_to_file

```c
bool save_report_to_file(const Stru_OptimizationEvaluationResult_t* result,
                        const char* filename);
```

**功能：** 将评估报告保存到文件

**参数：**
- `result`: 评估结果
- `filename`: 文件名

**返回值：**
- 成功：返回true
- 失败：返回false

**示例：**
```c
bool saved = evaluator->save_report_to_file(result, "optimization_report.txt");
```

### analyze_optimization_trends

```c
bool analyze_optimization_trends(Stru_OptimizationEvaluationResult_t** results,
                                size_t count,
                                Stru_OptimizationMetrics_t* trends);
```

**功能：** 分析多个优化评估结果的趋势

**参数：**
- `results`: 评估结果数组
- `count`: 结果数量
- `trends`: 输出参数，趋势指标

**返回值：**
- 成功：返回true
- 失败：返回false

**示例：**
```c
Stru_OptimizationMetrics_t trends;
bool success = evaluator->analyze_optimization_trends(results, count, &trends);
```

### destroy_evaluation_result

```c
void destroy_evaluation_result(Stru_OptimizationEvaluationResult_t* result);
```

**功能：** 销毁评估结果及其所有资源

**参数：**
- `result`: 评估结果

**返回值：** 无

**示例：**
```c
evaluator->destroy_evaluation_result(result);
```

### destroy

```c
void destroy(void);
```

**功能：** 销毁优化效果评估器接口及其所有资源

**参数：** 无

**返回值：** 无

**示例：**
```c
evaluator->destroy();
```

## 使用示例

### 完整示例：评估优化效果

```c
#include "CN_optimization_evaluator.h"
#include <stdio.h>

void evaluate_optimization_example(void* original_ir, void* optimized_ir) {
    // 创建评估器
    Stru_OptimizationEvaluatorInterface_t* evaluator = 
        F_create_optimization_evaluator_interface();
    if (!evaluator) {
        printf("无法创建优化评估器\n");
        return;
    }
    
    // 评估常量折叠优化
    Stru_OptimizationEvaluationResult_t* result = 
        evaluator->evaluate_optimization(original_ir, optimized_ir, 
                                         Eum_IR_OPT_CONSTANT_FOLDING);
    
    if (result) {
        if (result->evaluation_successful) {
            printf("优化评估成功\n");
            printf("优化质量评分: %.1f/100\n", result->metrics.optimization_quality_score);
            printf("指令减少比例: %.1f%%\n", result->metrics.instruction_reduction_ratio * 100);
            printf("代码大小减少比例: %.1f%%\n", result->metrics.code_size_reduction_ratio * 100);
            printf("估计性能改进: %.1f%%\n", result->metrics.estimated_performance_improvement * 100);
            
            // 生成并显示评估报告
            char* report = evaluator->generate_evaluation_report(result);
            if (report) {
                printf("\n评估报告:\n%s\n", report);
                cn_free(report);
            }
            
            // 生成优化建议
            char* recommendations = F_generate_optimization_recommendations(result);
            if (recommendations) {
                printf("\n优化建议:\n%s\n", recommendations);
                cn_free(recommendations);
            }
            
            // 保存报告到文件
            bool saved = evaluator->save_report_to_file(result, "optimization_report.txt");
            if (saved) {
                printf("评估报告已保存到 optimization_report.txt\n");
            }
        } else {
            printf("优化评估失败\n");
            if (result->evaluation_report) {
                printf("错误信息: %s\n", result->evaluation_report);
            }
        }
        
        // 销毁评估结果
        evaluator->destroy_evaluation_result(result);
    } else {
        printf("无法评估优化效果\n");
    }
    
    // 销毁评估器
    evaluator->destroy();
}
```

### 示例：运行基准测试

```c
#include "CN_optimization_evaluator.h"
#include <stdio.h>

void benchmark_example(void* original_ir, void* optimized_ir) {
    // 创建评估器
    Stru_OptimizationEvaluatorInterface_t* evaluator = 
        F_create_optimization_evaluator_interface();
    if (!evaluator) {
        printf("无法创建优化评估器\n");
        return;
    }
    
    // 创建基准测试
    Stru_OptimizationBenchmark_t* benchmark = 
        evaluator->create_benchmark("constant_folding_benchmark", 
                                   original_ir, optimized_ir);
    if (!benchmark) {
        printf("无法创建基准测试\n");
        evaluator->destroy();
        return;
    }
    
    // 设置迭代次数
    benchmark->iteration_count = 1000;
    
    // 运行基准测试
    bool success = evaluator->run_benchmark(benchmark);
    if (success) {
        printf("基准测试运行成功\n");
        
        // 计算平均执行时间
        double total_time = 0.0;
        for (size_t i = 0; i < benchmark->time_count; i++) {
            total_time += benchmark->execution_times[i];
        }
        double avg_time = total_time / benchmark->time_count;
        
        printf("平均执行时间: %.6f 秒\n", avg_time);
        printf("迭代次数: %zu\n", benchmark->iteration_count);
    } else {
        printf("基准测试运行失败\n");
    }
    
    // 销毁基准测试
    evaluator->destroy_benchmark(benchmark);
    
    // 销毁评估器
    evaluator->destroy();
}
```

### 示例：分析优化趋势

```c
#include "CN_optimization_evaluator.h"
#include <stdio.h>

void analyze_trends_example(Stru_OptimizationEvaluationResult_t** results, size_t count) {
    // 创建评估器
    Stru_OptimizationEvaluatorInterface_t* evaluator = 
        F_create_optimization_evaluator_interface();
    if (!evaluator) {
        printf("无法创建优化评估器\n");
        return;
    }
    
    // 分析趋势
    Stru_OptimizationMetrics_t trends;
    bool success = evaluator->analyze_optimization_trends(results, count, &trends);
    
    if (success) {
        printf("优化趋势分析结果:\n");
        printf("平均指令减少比例: %.1f%%\n", trends.instruction_reduction_ratio * 100);
        printf("平均代码大小减少比例: %.1f%%\n", trends.code_size_reduction_ratio * 100);
        printf("平均性能改进: %.1f%%\n", trends.estimated_performance_improvement * 100);
        printf("平均优化质量评分: %.1f/100\n", trends.optimization_quality_score);
    } else {
        printf("无法分析优化趋势\n");
    }
    
    // 销毁评估器
    evaluator->destroy();
}
```

## 错误处理

### 常见错误

1. **内存分配失败**: 当系统内存不足时，内存分配函数可能返回NULL
2. **无效参数**: 传入NULL指针或无效的IR指针
3. **文件操作失败**: 保存报告到文件时可能失败
4. **评估失败**: 优化前后IR不兼容或无法计算指标

### 错误处理示例

```c
#include "CN_optimization_evaluator.h"
#include <stdio.h>

void error_handling_example(void* original_ir, void* optimized_ir) {
    Stru_OptimizationEvaluatorInterface_t* evaluator = 
        F_create_optimization_evaluator_interface();
    if (!evaluator) {
        printf("错误: 无法创建优化评估器（内存不足）\n");
        return;
    }
    
    Stru_OptimizationEvaluationResult_t* result = 
        evaluator->evaluate_optimization(original_ir, optimized_ir, 
                                         Eum_IR_OPT_CONSTANT_FOLDING);
    
    if (!result) {
        printf("错误: 无法创建评估结果\n");
        evaluator->destroy();
        return;
    }
    
    if (!result->evaluation_successful) {
        printf("优化评估失败\n");
        if (result->evaluation_report) {
            printf("错误信息: %s\n", result->evaluation_report);
        }
    }
    
    evaluator->destroy_evaluation_result(result);
    evaluator->destroy();
}
```

## 性能注意事项

1. **内存使用**: 评估器会分配内存存储评估结果和报告，使用后应及时释放
2. **执行时间**: 复杂的优化评估可能需要较长时间，建议在后台线程执行
3. **基准测试**: 运行基准测试时，迭代次数不宜过大，避免消耗过多系统资源
4. **并发访问**: 评估器不是线程安全的，多线程访问需要同步

## 兼容性

### 支持的优化类型

- `Eum_IR_OPT_CONSTANT_FOLDING`: 常量折叠
- `Eum_IR_OPT_DEAD_CODE_ELIMINATION`: 死代码消除
- `Eum_IR_OPT_COMMON_SUBEXPRESSION_ELIMINATION`: 公共子表达式消除
- `Eum_IR_OPT_LOOP_OPTIMIZATION`: 循环优化
- `Eum_IR_OPT_INLINING`: 函数内联
- `Eum_IR_OPT_PEEPHOLE`: 窥孔优化

### 支持的优化级别

- `Eum_IR_OPT_LEVEL_O0`: 无优化
- `Eum_IR_OPT_LEVEL_O1`: 基本优化
- `Eum_IR_OPT_LEVEL_O2`: 标准优化
- `Eum_IR_OPT_LEVEL_O3`: 激进优化
- `Eum_IR_OPT_LEVEL_Os`: 优化代码大小
- `Eum_IR_OPT_LEVEL_Oz`: 最大化代码大小优化

## 版本历史

### v1.0.0 (2026-01-11)
- 初始版本
- 实现优化效果评估核心功能
- 支持多种优化类型和级别的评估
- 提供基准测试和趋势分析功能
- 生成详细的评估报告和优化建议

## 相关文档

- [优化模块README](../src/core/ir/optimizations/README.md)
- [IR优化器接口](../src/core/ir/CN_ir_optimizer.h)
- [CN_Language架构文档](../../../architecture/中间代码表示设计文档.md)

## 许可证

MIT License

Copyright (c) 2026 CN_Language项目组

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
