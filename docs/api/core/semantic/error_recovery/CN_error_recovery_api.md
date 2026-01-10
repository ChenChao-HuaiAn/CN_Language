# CN_error_recovery API 文档

## 概述

`CN_error_recovery` 模块提供详细的错误报告和恢复机制，包括错误信息收集、上下文跟踪、恢复策略执行和错误统计分析功能。本模块采用抽象接口模式，定义了4个核心接口，支持多种错误输出格式和恢复策略。

## 文件结构

```
src/core/semantic/error_recovery/
├── CN_error_recovery.h          # 头文件：接口定义和数据结构
├── CN_error_recovery.c          # 实现文件：接口实现
└── README.md                    # 模块文档
```

## 头文件包含

```c
#include "CN_error_recovery.h"
```

## 数据类型

### 枚举类型

#### Eum_ErrorSeverityLevel
错误严重性级别枚举。

```c
typedef enum Eum_ErrorSeverityLevel
{
    Eum_ERROR_SEVERITY_INFO = 0,        ///< 信息级别（非错误）
    Eum_ERROR_SEVERITY_WARNING = 1,     ///< 警告级别
    Eum_ERROR_SEVERITY_ERROR = 2,       ///< 错误级别
    Eum_ERROR_SEVERITY_CRITICAL = 3,    ///< 严重错误级别
    Eum_ERROR_SEVERITY_FATAL = 4        ///< 致命错误级别
} Eum_ErrorSeverityLevel;
```

#### Eum_ErrorRecoveryStrategy
错误恢复策略枚举。

```c
typedef enum Eum_ErrorRecoveryStrategy
{
    Eum_RECOVERY_STRATEGY_SKIP = 0,     ///< 跳过错误继续处理
    Eum_RECOVERY_STRATEGY_REPAIR = 1,   ///< 尝试修复错误
    Eum_RECOVERY_STRATEGY_ROLLBACK = 2, ///< 回滚到安全状态
    Eum_RECOVERY_STRATEGY_ABORT = 3,    ///< 中止处理
    Eum_RECOVERY_STRATEGY_RETRY = 4     ///< 重试操作
} Eum_ErrorRecoveryStrategy;
```

#### Eum_ErrorOutputFormat
错误输出格式枚举。

```c
typedef enum Eum_ErrorOutputFormat
{
    Eum_ERROR_FORMAT_TEXT = 0,          ///< 纯文本格式
    Eum_ERROR_FORMAT_JSON = 1,          ///< JSON格式
    Eum_ERROR_FORMAT_XML = 2,           ///< XML格式
    Eum_ERROR_FORMAT_HTML = 3,          ///< HTML格式
    Eum_ERROR_FORMAT_MARKDOWN = 4       ///< Markdown格式
} Eum_ErrorOutputFormat;
```

### 结构体类型

#### Stru_DetailedError_t
详细的错误信息结构体。

```c
struct Stru_DetailedError_t
{
    // 基本错误信息
    int error_code;                     ///< 错误代码
    const char* error_type;             ///< 错误类型名称
    const char* error_message;          ///< 错误消息
    Eum_ErrorSeverityLevel severity;    ///< 错误严重性级别
    
    // 位置信息
    size_t line;                        ///< 错误行号
    size_t column;                      ///< 错误列号
    const char* file_name;              ///< 文件名
    const char* function_name;          ///< 函数名
    const char* module_name;            ///< 模块名
    
    // 上下文信息
    const char* source_snippet;         ///< 源代码片段
    const char* surrounding_context;    ///< 周围上下文
    void* ast_node;                     ///< 相关AST节点
    void* symbol_info;                  ///< 相关符号信息
    
    // 时间信息
    time_t timestamp;                   ///< 错误发生时间戳
    uint64_t process_id;                ///< 进程ID
    uint64_t thread_id;                 ///< 线程ID
    
    // 修复建议
    const char* suggested_fix;          ///< 建议的修复方法
    const char* documentation_link;     ///< 相关文档链接
    
    // 调用栈信息
    void* call_stack;                   ///< 调用栈信息（实现特定）
    size_t call_stack_depth;            ///< 调用栈深度
    
    // 额外数据
    void* extra_data;                   ///< 额外数据（实现特定）
    size_t extra_data_size;             ///< 额外数据大小
};
```

#### Stru_ErrorContext_t
错误上下文结构体。

```c
struct Stru_ErrorContext_t
{
    // 编译状态
    const char* compilation_phase;      ///< 编译阶段（词法分析、语法分析、语义分析等）
    const char* current_pass;           ///< 当前处理过程
    
    // 符号表状态
    void* symbol_table_state;           ///< 符号表状态快照
    void* scope_stack_state;            ///< 作用域栈状态快照
    
    // 类型系统状态
    void* type_system_state;            ///< 类型系统状态快照
    
    // 内存状态
    size_t memory_usage;                ///< 内存使用量
    size_t heap_allocations;            ///< 堆分配数量
    
    // 性能指标
    uint64_t processing_time_ms;        ///< 处理时间（毫秒）
    size_t processed_nodes;             ///< 已处理的AST节点数量
    
    // 环境信息
    const char* platform;               ///< 平台信息
    const char* compiler_version;       ///< 编译器版本
    const char* language_version;       ///< 语言版本
};
```

#### Stru_ErrorRecoveryStrategy_t
错误恢复策略结构体。

```c
struct Stru_ErrorRecoveryStrategy_t
{
    Eum_ErrorRecoveryStrategy strategy; ///< 恢复策略类型
    const char* strategy_name;          ///< 策略名称
    const char* description;            ///< 策略描述
    
    // 策略参数
    int max_retries;                    ///< 最大重试次数（仅对重试策略有效）
    int retry_delay_ms;                 ///< 重试延迟（毫秒）
    bool allow_partial_recovery;        ///< 是否允许部分恢复
    
    // 恢复动作
    void (*recovery_action)(void* context, const Stru_DetailedError_t* error); ///< 恢复动作函数
    void* recovery_context;             ///< 恢复动作上下文
};
```

### 接口类型

#### Stru_DetailedErrorReporterInterface_t
详细的错误报告器接口。

```c
struct Stru_DetailedErrorReporterInterface_t
{
    bool (*initialize)(Stru_DetailedErrorReporterInterface_t* reporter,
                      size_t max_errors,
                      bool enable_call_stack);
    
    bool (*report_detailed_error)(Stru_DetailedErrorReporterInterface_t* reporter,
                                 const Stru_DetailedError_t* error,
                                 const Stru_ErrorContext_t* context);
    
    bool (*get_error_details)(Stru_DetailedErrorReporterInterface_t* reporter,
                             size_t error_index,
                             Stru_DetailedError_t* error,
                             Stru_ErrorContext_t* context);
    
    size_t (*format_error_report)(Stru_DetailedErrorReporterInterface_t* reporter,
                                 size_t error_index,
                                 Eum_ErrorOutputFormat format,
                                 char* buffer,
                                 size_t buffer_size);
    
    bool (*export_all_errors)(Stru_DetailedErrorReporterInterface_t* reporter,
                             Eum_ErrorOutputFormat format,
                             const char* file_path);
    
    void (*destroy)(Stru_DetailedErrorReporterInterface_t* reporter);
    
    void* private_data;
};
```

#### Stru_ErrorRecoveryInterface_t
错误恢复接口。

```c
struct Stru_ErrorRecoveryInterface_t
{
    bool (*initialize)(Stru_ErrorRecoveryInterface_t* recovery,
                      Eum_ErrorRecoveryStrategy default_strategy);
    
    bool (*register_recovery_strategy)(Stru_ErrorRecoveryInterface_t* recovery,
                                      const char* error_type,
                                      const Stru_ErrorRecoveryStrategy_t* strategy);
    
    bool (*perform_recovery)(Stru_ErrorRecoveryInterface_t* recovery,
                            const Stru_DetailedError_t* error,
                            const Stru_ErrorContext_t* context,
                            void** recovery_result);
    
    void (*destroy)(Stru_ErrorRecoveryInterface_t* recovery);
    
    void* private_data;
};
```

#### Stru_ErrorContextInterface_t
错误上下文接口。

```c
struct Stru_ErrorContextInterface_t
{
    bool (*initialize)(Stru_ErrorContextInterface_t* context,
                      bool enable_snapshots);
    
    bool (*capture_context)(Stru_ErrorContextInterface_t* context,
                           const char* phase,
                           const char* pass,
                           Stru_ErrorContext_t* captured_context);
    
    void (*destroy)(Stru_ErrorContextInterface_t* context);
    
    void* private_data;
};
```

#### Stru_ErrorStatisticsInterface_t
错误统计接口。

```c
struct Stru_ErrorStatisticsInterface_t
{
    bool (*initialize)(Stru_ErrorStatisticsInterface_t* statistics);
    
    bool (*add_error)(Stru_ErrorStatisticsInterface_t* statistics,
                     const Stru_DetailedError_t* error,
                     const Stru_ErrorContext_t* context);
    
    size_t (*get_error_frequency)(Stru_ErrorStatisticsInterface_t* statistics,
                                 void** error_type_counts,
                                 size_t max_types);
    
    bool (*get_error_distribution)(Stru_ErrorStatisticsInterface_t* statistics,
                                  void** file_distribution,
                                  void** module_distribution,
                                  void** severity_distribution);
    
    size_t (*get_error_trend)(Stru_ErrorStatisticsInterface_t* statistics,
                             time_t time_period,
                             void** trend_data,
                             size_t max_data_points);
    
    void (*destroy)(Stru_ErrorStatisticsInterface_t* statistics);
    
    void* private_data;
};
```

## 工厂函数

### F_create_detailed_error_reporter_interface
创建详细的错误报告器接口实例。

```c
Stru_DetailedErrorReporterInterface_t* F_create_detailed_error_reporter_interface(void);
```

**返回值**：
- 成功：返回错误报告器接口实例指针
- 失败：返回NULL

**说明**：
- 创建的实例需要通过 `F_destroy_detailed_error_reporter_interface` 释放
- 实例创建后需要调用 `initialize` 函数进行初始化

### F_create_error_recovery_interface
创建错误恢复接口实例。

```c
Stru_ErrorRecoveryInterface_t* F_create_error_recovery_interface(void);
```

**返回值**：
- 成功：返回错误恢复接口实例指针
- 失败：返回NULL

**说明**：
- 创建的实例需要通过 `F_destroy_error_recovery_interface` 释放
- 实例创建后需要调用 `initialize` 函数进行初始化

### F_create_error_context_interface
创建错误上下文接口实例。

```c
Stru_ErrorContextInterface_t* F_create_error_context_interface(void);
```

**返回值**：
- 成功：返回错误上下文接口实例指针
- 失败：返回NULL

**说明**：
- 创建的实例需要通过 `F_destroy_error_context_interface` 释放
- 实例创建后需要调用 `initialize` 函数进行初始化

### F_create_error_statistics_interface
创建错误统计接口实例。

```c
Stru_ErrorStatisticsInterface_t* F_create_error_statistics_interface(void);
```

**返回值**：
- 成功：返回错误统计接口实例指针
- 失败：返回NULL

**说明**：
- 创建的实例需要通过 `F_destroy_error_statistics_interface` 释放
- 实例创建后需要调用 `initialize` 函数进行初始化

### F_destroy_detailed_error_reporter_interface
销毁详细的错误报告器接口实例。

```c
void F_destroy_detailed_error_reporter_interface(Stru_DetailedErrorReporterInterface_t* interface);
```

**参数**：
- `interface`：要销毁的错误报告器接口实例

**说明**：
- 如果接口实例为NULL，函数不执行任何操作
- 函数会调用接口的 `destroy` 方法释放资源

### F_destroy_error_recovery_interface
销毁错误恢复接口实例。

```c
void F_destroy_error_recovery_interface(Stru_ErrorRecoveryInterface_t* interface);
```

**参数**：
- `interface`：要销毁的错误恢复接口实例

**说明**：
- 如果接口实例为NULL，函数不执行任何操作
- 函数会调用接口的 `destroy` 方法释放资源

### F_destroy_error_context_interface
销毁错误上下文接口实例。

```c
void F_destroy_error_context_interface(Stru_ErrorContextInterface_t* interface);
```

**参数**：
- `interface`：要销毁的错误上下文接口实例

**说明**：
- 如果接口实例为NULL，函数不执行任何操作
- 函数会调用接口的 `destroy` 方法释放资源

### F_destroy_error_statistics_interface
销毁错误统计接口实例。

```c
void F_destroy_error_statistics_interface(Stru_ErrorStatisticsInterface_t* interface);
```

**参数**：
- `interface`：要销毁的错误统计接口实例

**说明**：
- 如果接口实例为NULL，函数不执行任何操作
- 函数会调用接口的 `destroy` 方法释放资源

## 接口方法详解

### 详细的错误报告器接口方法

#### initialize
初始化详细的错误报告器。

```c
bool initialize(Stru_DetailedErrorReporterInterface_t* reporter,
                size_t max_errors,
                bool enable_call_stack);
```

**参数**：
- `reporter`：错误报告器实例
- `max_errors`：最大错误数量限制（0表示无限制）
- `enable_call_stack`：是否启用调用栈跟踪

**返回值**：
- 成功：返回true
- 失败：返回false

#### report_detailed_error
报告详细的错误。

```c
bool report_detailed_error(Stru_DetailedErrorReporterInterface_t* reporter,
                          const Stru_DetailedError_t* error,
                          const Stru_ErrorContext_t* context);
```

**参数**：
- `reporter`：错误报告器实例
- `error`：详细的错误信息
- `context`：错误上下文（可选，可为NULL）

**返回值**：
- 成功：返回true
- 失败：返回false

#### get_error_details
获取错误详细信息。

```c
bool get_error_details(Stru_DetailedErrorReporterInterface_t* reporter,
                      size_t error_index,
                      Stru_DetailedError_t* error,
                      Stru_ErrorContext_t* context);
```

**参数**：
- `reporter`：错误报告器实例
- `error_index`：错误索引（从0开始）
- `error`：输出参数，错误详细信息
- `context`：输出参数，错误上下文（可选，可为NULL）

**返回值**：
- 成功：返回true
- 失败：返回false

#### format_error_report
格式化错误报告。

```c
size_t format_error_report(Stru_DetailedErrorReporterInterface_t* reporter,
                          size_t error_index,
                          Eum_ErrorOutputFormat format,
                          char* buffer,
                          size_t buffer_size);
```

**参数**：
- `reporter`：错误报告器实例
- `error_index`：错误索引（从0开始）
- `format`：输出格式
- `buffer`：输出缓冲区
- `buffer_size`：缓冲区大小

**返回值**：
- 成功：返回格式化后的字符串长度（不包括终止符）
- 失败：返回0

#### export_all_errors
导出所有错误报告。

```c
bool export_all_errors(Stru_DetailedErrorReporterInterface_t* reporter,
                      Eum_ErrorOutputFormat format,
                      const char* file_path);
```

**参数**：
- `reporter`：错误报告器实例
- `format`：输出格式
- `file_path`：输出文件路径

**返回值**：
- 成功：返回true
- 失败：返回false

#### destroy
销毁详细的错误报告器。

```c
void destroy(Stru_DetailedErrorReporterInterface_t* reporter);
```

**参数**：
- `reporter`：错误报告器实例

### 错误恢复接口方法

#### initialize
初始化错误恢复器。

```c
bool initialize(Stru_ErrorRecoveryInterface_t* recovery,
                Eum_ErrorRecoveryStrategy default_strategy);
```

**参数**：
- `recovery`：错误恢复器实例
- `default_strategy`：默认恢复策略

**返回值**：
- 成功：返回true
- 失败：返回false

#### register_recovery_strategy
注册恢复策略。

```c
bool register_recovery_strategy(Stru_ErrorRecoveryInterface_t* recovery,
                               const char* error_type,
                               const Stru_ErrorRecoveryStrategy_t* strategy);
```

**参数**：
- `recovery`：错误恢复器实例
- `error_type`：错误类型
- `strategy`：恢复策略

**返回值**：
- 成功：返回true
- 失败：返回false

#### perform_recovery
执行错误恢复。

```c
bool perform_recovery(Stru_ErrorRecoveryInterface_t* recovery,
                     const Stru_DetailedError_t* error,
                     const Stru_ErrorContext_t* context,
                     void** recovery_result);
```

**参数**：
- `recovery`：错误恢复器实例
- `error`：错误信息
- `context`：错误上下文（可选，可为NULL）
- `recovery_result`：输出参数，恢复结果（可选，可为NULL）

**返回值**：
- 成功：返回true
- 失败：返回false

#### destroy
销毁错误恢复器。

```c
void destroy(Stru_ErrorRecoveryInterface_t* recovery);
```

**参数**：
- `recovery`：错误恢复器实例

### 错误上下文接口方法

#### initialize
初始化错误上下文管理器。

```c
bool initialize(Stru_ErrorContextInterface_t* context,
                bool enable_snapshots);
```

**参数**：
- `context`：错误上下文管理器实例
- `enable_snapshots`：是否启用状态快照

**返回值**：
- 成功：返回true
- 失败：返回false

#### capture_context
捕获当前上下文。

```c
bool capture_context(Stru_ErrorContextInterface_t* context,
                    const char* phase,
                    const char* pass,
                    Stru_ErrorContext_t* captured_context);
```

**参数**：
- `context`：错误上下文管理器实例
- `phase`：当前编译阶段
- `pass`：当前处理过程
- `captured_context`：输出参数，捕获的上下文

**返回值**：
- 成功：返回true
- 失败：返回false

#### destroy
销毁错误上下文管理器。

```c
void destroy(Stru_ErrorContextInterface_t* context);
```

**参数**：
- `context`：错误上下文管理器实例

### 错误统计接口方法

#### initialize
初始化错误统计器。

```c
bool initialize(Stru_ErrorStatisticsInterface_t* statistics);
```

**参数**：
- `statistics`：错误统计器实例

**返回值**：
- 成功：返回true
- 失败：返回false

#### add_error
添加错误到统计。

```c
bool add_error(Stru_ErrorStatisticsInterface_t* statistics,
              const Stru_DetailedError_t* error,
              const Stru_ErrorContext_t* context);
```

**参数**：
- `statistics`：错误统计器实例
- `error`：错误信息
- `context`：错误上下文（可选，可为NULL）

**返回值**：
- 成功：返回true
- 失败：返回false

#### get_error_frequency
获取错误频率统计。

```c
size_t get_error_frequency(Stru_ErrorStatisticsInterface_t* statistics,
                          void** error_type_counts,
                          size_t max_types);
```

**参数**：
- `statistics`：错误统计器实例
- `error_type_counts`：输出参数，错误类型计数数组
- `max_types`：最大错误类型数量

**返回值**：
- 成功：返回实际错误类型数量
- 失败：返回0

#### get_error_distribution
获取错误分布统计。

```c
bool get_error_distribution(Stru_ErrorStatisticsInterface_t* statistics,
                           void** file_distribution,
                           void** module_distribution,
                           void** severity_distribution);
```

**参数**：
- `statistics`：错误统计器实例
- `file_distribution`：输出参数，文件分布（可选，可为NULL）
- `module_distribution`：输出参数，模块分布（可选，可为NULL）
- `severity_distribution`：输出参数，严重性分布（可选，可为NULL）

**返回值**：
- 成功：返回true
- 失败：返回false

#### get_error_trend
获取错误趋势分析。

```c
size_t get_error_trend(Stru_ErrorStatisticsInterface_t* statistics,
                      time_t time_period,
                      void** trend_data,
                      size_t max_data_points);
```

**参数**：
- `statistics`：错误统计器实例
- `time_period`：时间周期（秒）
- `trend_data`：输出参数，趋势数据数组
- `max_data_points`：最大数据点数量

**返回值**：
- 成功：返回实际数据点数量
- 失败：返回0

#### destroy
销毁错误统计器。

```c
void destroy(Stru_ErrorStatisticsInterface_t* statistics);
```

**参数**：
- `statistics`：错误统计器实例

## 使用示例

### 基本示例

```c
#include "CN_error_recovery.h"
#include <stdio.h>

int main(void)
{
    // 创建接口实例
    Stru_DetailedErrorReporterInterface_t* reporter = 
        F_create_detailed_error_reporter_interface();
    Stru_ErrorRecoveryInterface_t* recovery = 
        F_create_error_recovery_interface();
    Stru_ErrorContextInterface_t* context = 
        F_create_error_context_interface();
    Stru_ErrorStatisticsInterface_t* statistics = 
        F_create_error_statistics_interface();
    
    if (!reporter || !recovery || !context || !statistics)
    {
        printf("创建接口实例失败\n");
        return 1;
    }
    
    // 初始化接口
    if (!reporter->initialize(reporter, 100, true))
    {
        printf("初始化错误报告器失败\n");
        return 1;
    }
    
    if (!recovery->initialize(recovery, Eum_RECOVERY_STRATEGY_SKIP))
    {
        printf("初始化错误恢复器失败\n");
        return 1;
    }
    
    if (!context->initialize(context, true))
    {
        printf("初始化错误上下文管理器失败\n");
        return 1;
    }
    
    if (!statistics->initialize(statistics))
    {
        printf("初始化错误统计器失败\n");
        return 1;
    }
    
    // 使用示例...
    
    // 清理资源
    F_destroy_detailed_error_reporter_interface(reporter);
    F_destroy_error_recovery_interface(recovery);
    F_destroy_error_context_interface(context);
    F_destroy_error_statistics_interface(statistics);
    
    return 0;
}
```

### 错误报告示例

```c
void report_sample_error(Stru_DetailedErrorReporterInterface_t* reporter,
                        Stru_ErrorContextInterface_t* context)
{
    // 捕获当前上下文
    Stru_ErrorContext_t error_context;
    if (!context->capture_context(context, "语义分析", "类型检查", &error_context))
    {
        printf("捕获上下文失败\n");
        return;
    }
    
    // 创建错误信息
    Stru_DetailedError_t error = {
        .error_code = 1001,
        .error_type = "类型错误",
        .error_message = "变量类型不匹配：期望整数，实际为字符串",
        .severity = Eum_ERROR_SEVERITY_ERROR,
        .line = 42,
        .column = 15,
        .file_name = "example.cn",
        .function_name = "calculate_sum",
        .module_name = "数学模块",
        .suggested_fix = "检查变量类型声明或添加类型转换",
        .timestamp = time(NULL)
    };
    
    // 报告错误
    if (!reporter->report_detailed_error(reporter, &error, &error_context))
    {
        printf("报告错误失败\n");
        return;
    }
    
    printf("错误报告成功\n");
}
```

### 错误恢复示例

```c
void recovery_sample(Stru_ErrorRecoveryInterface_t* recovery,
                    Stru_DetailedErrorReporterInterface_t* reporter)
{
    // 注册自定义恢复策略
    Stru_ErrorRecoveryStrategy_t type_repair_strategy = {
        .strategy = Eum_RECOVERY_STRATEGY_REPAIR,
        .strategy_name = "类型修复策略",
        .description = "自动修复类型不匹配错误",
        .max_retries = 3,
        .retry_delay_ms = 100,
        .allow_partial_recovery = true,
        .recovery_action = type_repair_action,
        .recovery_context = NULL
    };
    
    if (!recovery->register_recovery_strategy(recovery, "类型错误", &type_repair_strategy))
    {
        printf("注册恢复策略失败\n");
        return;
    }
    
    // 获取错误信息（假设已报告错误）
    Stru_DetailedError_t error;
    Stru_ErrorContext_t context;
    if (!reporter->get_error_details(reporter, 0, &error, &context))
    {
        printf("获取错误详情失败\n");
        return;
    }
    
    // 执行错误恢复
    void* recovery_result = NULL;
    if (!recovery->perform_recovery(recovery, &error, &context, &recovery_result))
    {
        printf("执行错误恢复失败\n");
        return;
    }
    
    printf("错误恢复成功\n");
}
```

### 错误统计示例

```c
void statistics_sample(Stru_ErrorStatisticsInterface_t* statistics,
                      Stru_DetailedErrorReporterInterface_t* reporter)
{
    // 添加错误到统计（假设已报告多个错误）
    size_t error_count = 10; // 假设有10个错误
    for (size_t i = 0; i < error_count; i++)
    {
        Stru_DetailedError_t error;
        Stru_ErrorContext_t context;
        
        if (reporter->get_error_details(reporter, i, &error, &context))
        {
            statistics->add_error(statistics, &error, &context);
        }
    }
    
    // 获取错误分布
    void* severity_distribution = NULL;
    if (statistics->get_error_distribution(statistics, NULL, NULL, &severity_distribution))
    {
        printf("获取错误分布成功\n");
        // 处理分布数据...
    }
    
    // 获取错误趋势
    void* trend_data = NULL;
    size_t data_points = statistics->get_error_trend(statistics, 3600, &trend_data, 100);
    if (data_points > 0)
    {
        printf("获取错误趋势成功，数据点数量：%zu\n", data_points);
        // 处理趋势数据...
    }
}
```

## 错误处理

### 错误码定义

| 错误码 | 错误类型 | 描述 |
|--------|----------|------|
| 1000-1999 | 语法错误 | 源代码语法问题 |
| 2000-2999 | 语义错误 | 语义分析问题 |
| 3000-3999 | 类型错误 | 类型系统问题 |
| 4000-4999 | 优化错误 | 优化过程问题 |
| 5000-5999 | 代码生成错误 | 代码生成问题 |
| 6000-6999 | 运行时错误 | 运行时问题 |
| 7000-7999 | 系统错误 | 系统资源问题 |

### 错误处理建议

1. **检查返回值**：所有接口函数都返回布尔值表示成功/失败
2. **资源清理**：确保在错误情况下正确释放资源
3. **错误传播**：在适当的时候传播错误信息
4. **日志记录**：记录详细的错误信息以便调试

## 性能考虑

### 内存使用
- 错误信息结构体设计为紧凑格式
- 使用字符串池减少重复字符串的内存占用
- 实现内存使用限制和清理策略

### 处理速度
- 使用哈希表加速错误类型查找
- 实现批量处理和并行处理
- 优化格式化函数的性能

### 线程安全
- 所有接口都是线程安全的
- 使用读写锁保护共享数据
- 提供原子操作和无锁数据结构

## 兼容性

### 版本兼容性
- 保持接口的向后兼容性
- 使用版本号管理接口变更
- 提供迁移工具和文档

### 平台兼容性
- 支持Windows、Linux、macOS等主流平台
- 使用平台无关的API和数据类型
- 提供平台特定的优化

## 扩展指南

### 添加新的错误类型
1. 在错误枚举中添加新的错误代码
2. 实现相应的错误处理逻辑
3. 更新错误类型映射表

### 添加新的恢复策略
1. 定义新的恢复策略枚举值
2. 实现恢复策略函数
3. 注册策略到恢复器

### 添加新的输出格式
1. 定义新的输出格式枚举值
2. 实现格式化函数
3. 集成到错误报告器

## 常见问题

### Q: 如何限制错误报告的内存使用？
A: 通过 `initialize` 函数的 `max_errors` 参数限制最大错误数量。

### Q: 如何启用调用栈跟踪？
A: 通过 `initialize` 函数的 `enable_call_stack` 参数启用调用栈跟踪。

### Q: 如何导出错误报告？
A: 使用 `export_all_errors` 函数导出错误报告到文件。

### Q: 如何注册自定义恢复策略？
A: 使用 `register_recovery_strategy` 函数注册自定义恢复策略。

### Q: 如何获取错误统计信息？
A: 使用错误统计接口的相应方法获取频率、分布和趋势信息。

## 参考资源

- [CN_error_recovery模块文档](../../../../src/core/semantic/error_recovery/README.md)
- [CN_Language架构设计文档](../../../../docs/architecture/)
- [错误处理规范](../../../../docs/specifications/错误处理规范.md)

## 版本历史

### v1.0.0 (2026-01-10)
- 初始版本发布
- 实现基本的错误报告和恢复功能
- 提供4个核心接口和工厂函数
- 支持多种错误输出格式
- 实现错误统计和分析功能

## 许可证

本模块采用MIT许可证。详见项目根目录的LICENSE文件。
</content>
