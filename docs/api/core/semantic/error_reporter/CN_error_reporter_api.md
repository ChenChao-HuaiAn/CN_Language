# CN_Language 错误报告器模块 API 文档

## 概述

错误报告器模块是CN_Language语义分析系统的核心组件之一，负责收集、管理和报告语义分析过程中发现的错误和警告。本模块提供完整的错误处理功能，支持错误分类、错误过滤、错误格式化和错误回调，为语义分析提供强大的错误处理能力。

## 接口定义

### Stru_SemanticErrorReporterInterface_t

语义错误报告接口，负责收集和报告语义错误。

```c
typedef struct Stru_SemanticErrorReporterInterface_t {
    // 初始化函数
    bool (*initialize)(Stru_SemanticErrorReporterInterface_t* error_reporter, size_t max_errors);
    
    // 错误报告
    bool (*report_error)(Stru_SemanticErrorReporterInterface_t* error_reporter,
                        Eum_SemanticErrorType error_type,
                        const char* message,
                        uint32_t line,
                        uint32_t column,
                        const char* module_name);
    bool (*report_warning)(Stru_SemanticErrorReporterInterface_t* error_reporter,
                          Eum_SemanticErrorType warning_type,
                          const char* message,
                          uint32_t line,
                          uint32_t column,
                          const char* module_name);
    
    // 错误查询
    uint32_t (*get_error_count)(Stru_SemanticErrorReporterInterface_t* error_reporter);
    uint32_t (*get_warning_count)(Stru_SemanticErrorReporterInterface_t* error_reporter);
    bool (*get_error)(Stru_SemanticErrorReporterInterface_t* error_reporter,
                     uint32_t index,
                     Stru_SemanticError_t* result);
    bool (*get_warning)(Stru_SemanticErrorReporterInterface_t* error_reporter,
                       uint32_t index,
                       Stru_SemanticError_t* result);
    
    // 错误过滤
    bool (*clear_errors)(Stru_SemanticErrorReporterInterface_t* error_reporter);
    bool (*clear_warnings)(Stru_SemanticErrorReporterInterface_t* error_reporter);
    bool (*filter_errors_by_type)(Stru_SemanticErrorReporterInterface_t* error_reporter,
                                 Eum_SemanticErrorType error_type);
    
    // 回调设置
    bool (*set_error_callback)(Stru_SemanticErrorReporterInterface_t* error_reporter,
                              void (*callback)(const Stru_SemanticError_t* error, void* user_data),
                              void* user_data);
    bool (*set_warning_callback)(Stru_SemanticErrorReporterInterface_t* error_reporter,
                                void (*callback)(const Stru_SemanticError_t* warning, void* user_data),
                                void* user_data);
    
    // 错误格式
    bool (*format_error)(Stru_SemanticErrorReporterInterface_t* error_reporter,
                        const Stru_SemanticError_t* error,
                        char* buffer,
                        size_t buffer_size);
    
    // 资源管理
    void (*destroy)(Stru_SemanticErrorReporterInterface_t* error_reporter);
    
    // 私有数据
    void* private_data;
} Stru_SemanticErrorReporterInterface_t;
```

## 辅助数据结构

### Stru_SemanticError_t

语义错误结构体，存储错误的详细信息。

```c
typedef struct Stru_SemanticError_t {
    Eum_SemanticErrorType error_type;      // 错误类型
    char message[CN_MAX_ERROR_MESSAGE_LENGTH]; // 错误消息
    uint32_t line;                         // 行号
    uint32_t column;                       // 列号
    char module_name[CN_MAX_MODULE_NAME_LENGTH]; // 模块名称
    char symbol_name[CN_MAX_SYMBOL_NAME_LENGTH]; // 相关符号名称
    char type_name[CN_MAX_TYPE_NAME_LENGTH]; // 相关类型名称
    uint64_t timestamp;                    // 时间戳
    void* extra_data;                      // 额外数据（可选）
} Stru_SemanticError_t;
```

### Eum_SemanticErrorType

语义错误类型枚举。

```c
typedef enum Eum_SemanticErrorType {
    Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL,   // 未定义符号
    Eum_SEMANTIC_ERROR_TYPE_MISMATCH,      // 类型不匹配
    Eum_SEMANTIC_ERROR_DUPLICATE_SYMBOL,   // 重复符号
    Eum_SEMANTIC_ERROR_SCOPE_VIOLATION,    // 作用域违规
    Eum_SEMANTIC_ERROR_INVALID_OPERATION,  // 无效操作
    Eum_SEMANTIC_ERROR_INVALID_TYPE,       // 无效类型
    Eum_SEMANTIC_ERROR_INVALID_ARGUMENT,   // 无效参数
    Eum_SEMANTIC_ERROR_INVALID_RETURN,     // 无效返回
    Eum_SEMANTIC_ERROR_INVALID_ASSIGNMENT, // 无效赋值
    Eum_SEMANTIC_ERROR_INVALID_CONVERSION, // 无效转换
    Eum_SEMANTIC_ERROR_INVALID_ACCESS,     // 无效访问
    Eum_SEMANTIC_ERROR_INVALID_DECLARATION,// 无效声明
    Eum_SEMANTIC_ERROR_INVALID_EXPRESSION, // 无效表达式
    Eum_SEMANTIC_ERROR_INVALID_STATEMENT,  // 无效语句
    Eum_SEMANTIC_ERROR_INVALID_MODULE,     // 无效模块
    Eum_SEMANTIC_ERROR_INTERNAL            // 内部错误
} Eum_SemanticErrorType;
```

### Eum_ErrorSeverity

错误严重性枚举。

```c
typedef enum Eum_ErrorSeverity {
    Eum_ERROR_SEVERITY_INFO,               // 信息
    Eum_ERROR_SEVERITY_WARNING,            // 警告
    Eum_ERROR_SEVERITY_ERROR,              // 错误
    Eum_ERROR_SEVERITY_FATAL               // 致命错误
} Eum_ErrorSeverity;
```

## API 参考

### F_create_semantic_error_reporter_interface

创建语义错误报告接口实例。

**函数签名：**
```c
Stru_SemanticErrorReporterInterface_t* F_create_semantic_error_reporter_interface(void);
```

**返回值：**
- `Stru_SemanticErrorReporterInterface_t*`：新创建的语义错误报告接口实例
- `NULL`：创建失败（内存不足）

**示例：**
```c
Stru_SemanticErrorReporterInterface_t* error_reporter = F_create_semantic_error_reporter_interface();
if (error_reporter == NULL) {
    fprintf(stderr, "无法创建错误报告器\n");
    return 1;
}

// 使用错误报告器...

error_reporter->destroy(error_reporter);
```

### error_reporter->initialize

初始化错误报告器。

**函数签名：**
```c
bool initialize(Stru_SemanticErrorReporterInterface_t* error_reporter, size_t max_errors);
```

**参数：**
- `error_reporter`：错误报告器接口指针
- `max_errors`：最大错误数量限制（0表示无限制）

**返回值：**
- `true`：初始化成功
- `false`：初始化失败

### error_reporter->report_error

报告语义错误。

**函数签名：**
```c
bool report_error(Stru_SemanticErrorReporterInterface_t* error_reporter,
                 Eum_SemanticErrorType error_type,
                 const char* message,
                 uint32_t line,
                 uint32_t column,
                 const char* module_name);
```

**参数：**
- `error_reporter`：错误报告器接口指针
- `error_type`：错误类型
- `message`：错误消息
- `line`：行号（从1开始）
- `column`：列号（从1开始）
- `module_name`：模块名称（可为NULL）

**返回值：**
- `true`：报告成功
- `false`：报告失败（如达到最大错误数量）

### error_reporter->report_warning

报告语义警告。

**函数签名：**
```c
bool report_warning(Stru_SemanticErrorReporterInterface_t* error_reporter,
                   Eum_SemanticErrorType warning_type,
                   const char* message,
                   uint32_t line,
                   uint32_t column,
                   const char* module_name);
```

**参数：**
- `error_reporter`：错误报告器接口指针
- `warning_type`：警告类型
- `message`：警告消息
- `line`：行号（从1开始）
- `column`：列号（从1开始）
- `module_name`：模块名称（可为NULL）

**返回值：**
- `true`：报告成功
- `false`：报告失败

### error_reporter->get_error_count

获取错误数量。

**函数签名：**
```c
uint32_t get_error_count(Stru_SemanticErrorReporterInterface_t* error_reporter);
```

**返回值：**
- `uint32_t`：错误数量

### error_reporter->get_warning_count

获取警告数量。

**函数签名：**
```c
uint32_t get_warning_count(Stru_SemanticErrorReporterInterface_t* error_reporter);
```

**返回值：**
- `uint32_t`：警告数量

### error_reporter->get_error

获取指定索引的错误。

**函数签名：**
```c
bool get_error(Stru_SemanticErrorReporterInterface_t* error_reporter,
              uint32_t index,
              Stru_SemanticError_t* result);
```

**参数：**
- `error_reporter`：错误报告器接口指针
- `index`：错误索引（从0开始）
- `result`：输出参数，存储错误信息

**返回值：**
- `true`：获取成功
- `false`：获取失败（如索引无效）

### error_reporter->clear_errors

清空所有错误。

**函数签名：**
```c
bool clear_errors(Stru_SemanticErrorReporterInterface_t* error_reporter);
```

**返回值：**
- `true`：清空成功
- `false`：清空失败

### error_reporter->set_error_callback

设置错误回调函数。

**函数签名：**
```c
bool set_error_callback(Stru_SemanticErrorReporterInterface_t* error_reporter,
                       void (*callback)(const Stru_SemanticError_t* error, void* user_data),
                       void* user_data);
```

**参数：**
- `error_reporter`：错误报告器接口指针
- `callback`：回调函数，当报告错误时调用
- `user_data`：传递给回调函数的用户数据

**返回值：**
- `true`：设置成功
- `false`：设置失败

### error_reporter->format_error

格式化错误信息。

**函数签名：**
```c
bool format_error(Stru_SemanticErrorReporterInterface_t* error_reporter,
                 const Stru_SemanticError_t* error,
                 char* buffer,
                 size_t buffer_size);
```

**参数：**
- `error_reporter`：错误报告器接口指针
- `error`：错误信息
- `buffer`：输出缓冲区
- `buffer_size`：缓冲区大小

**返回值：**
- `true`：格式化成功
- `false`：格式化失败（如缓冲区不足）

### error_reporter->destroy

销毁错误报告器，释放所有资源。

**函数签名：**
```c
void destroy(Stru_SemanticErrorReporterInterface_t* error_reporter);
```

## 使用示例

### 基本使用模式

```c
#include "CN_error_reporter_interface.h"

int main() {
    // 创建错误报告器
    Stru_SemanticErrorReporterInterface_t* error_reporter = F_create_semantic_error_reporter_interface();
    if (error_reporter == NULL) {
        return 1;
    }
    
    // 初始化错误报告器
    if (!error_reporter->initialize(error_reporter, 100)) {
        error_reporter->destroy(error_reporter);
        return 1;
    }
    
    // 设置错误回调
    error_reporter->set_error_callback(error_reporter, error_callback, NULL);
    
    // 报告错误
    error_reporter->report_error(error_reporter,
                                Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL,
                                "未定义的变量 'x'",
                                10, 5,
                                "main.cn");
    
    error_reporter->report_warning(error_reporter,
                                  Eum_SEMANTIC_ERROR_TYPE_MISMATCH,
                                  "隐式类型转换可能丢失精度",
                                  15, 8,
                                  "main.cn");
    
    // 获取错误统计
    uint32_t error_count = error_reporter->get_error_count(error_reporter);
    uint32_t warning_count = error_reporter->get_warning_count(error_reporter);
    printf("错误数量: %u, 警告数量: %u\n", error_count, warning_count);
    
    // 格式化并显示错误
    for (uint32_t i = 0; i < error_count; i++) {
        Stru_SemanticError_t error;
        error_reporter->get_error(error_reporter, i, &error);
        
        char formatted[512];
        error_reporter->format_error(error_reporter, &error, formatted, sizeof(formatted));
        printf("%s\n", formatted);
    }
    
    // 清理
    error_reporter->destroy(error_reporter);
    
    return 0;
}

void error_callback(const Stru_SemanticError_t* error, void* user_data) {
    printf("[错误回调] %s: %s (行: %u, 列: %u)\n",
           error->module_name, error->message,
           error->line, error->column);
}
```

### 错误过滤和分类示例

```c
void error_filtering_example(Stru_SemanticErrorReporterInterface_t* error_reporter) {
    // 报告不同类型的错误
    error_reporter->report_error(error_reporter,
                                Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL,
                                "变量 'x' 未定义",
                                5, 10,
                                "test.cn");
    
    error_reporter->report_error(error_reporter,
                                Eum_SEMANTIC_ERROR_TYPE_MISMATCH,
                                "无法将整数赋值给字符串",
                                8, 15,
                                "test.cn");
    
    error_reporter->report_error(error_reporter,
                                Eum_SEMANTIC_ERROR_DUPLICATE_SYMBOL,
                                "函数 'foo' 重复定义",
                                12, 3,
                                "test.cn");
    
    error_reporter->report_error(error_reporter,
                                Eum_SEMANTIC_ERROR_SCOPE_VIOLATION,
                                "变量 'y' 超出作用域",
                                20, 7,
                                "test.cn");
    
    // 按类型过滤错误
    printf("所有错误:\n");
    print_all_errors(error_reporter);
    
    printf("\n仅类型不匹配错误:\n");
    error_reporter->filter_errors_by_type(error_reporter, Eum_SEMANTIC_ERROR_TYPE_MISMATCH);
    print_all_errors(error_reporter);
    
    // 重置过滤器
    error_reporter->filter_errors_by_type(error_reporter, Eum_SEMANTIC_ERROR_UNKNOWN);
}

void print_all_errors(Stru_SemanticErrorReporterInterface_t* error_reporter) {
    uint32_t error_count = error_reporter->get_error_count(error_reporter);
    
    for (uint32_t i = 0; i < error_count; i++) {
        Stru_SemanticError_t error;
        if (error_reporter->get_error(error_reporter, i, &error)) {
            printf("  [%s] %s (行: %u, 列: %u)\n",
                   get_error_type_name(error.error_type),
                   error.message, error.line, error.column);
        }
    }
}

const char* get_error_type_name(Eum_SemanticErrorType error_type) {
    switch (error_type) {
        case Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL: return "未定义符号";
        case Eum_SEMANTIC_ERROR_TYPE_MISMATCH: return "类型不匹配";
        case Eum_SEMANTIC_ERROR_DUPLICATE_SYMBOL: return "重复符号";
        case Eum_SEMANTIC_ERROR_SCOPE_VIOLATION: return "作用域违规";
        default: return "未知错误";
    }
}
```

### 错误格式化和输出示例

```c
void error_formatting_example(Stru_SemanticErrorReporterInterface_t* error_reporter) {
    // 报告详细错误
    Stru_SemanticError_t detailed_error = {
        .error_type = Eum_SEMANTIC_ERROR_TYPE_MISMATCH,
        .message = "无法将类型 '整数' 赋值给类型 '字符串'",
        .line = 25,
        .column = 12,
        .module_name = "example.cn",
        .symbol_name = "变量名",
        .type_name = "整数",
        .timestamp = get_current_timestamp()
    };
    
    // 直接使用错误结构体（假设有相关函数）
    report_detailed_error(error_reporter, &detailed_error);
    
    // 格式化错误为不同格式
    char simple_format[256];
    char detailed_format[512];
    char json_format[1024];
    
    // 简单格式
    error_reporter->format_error(error_reporter, &detailed_error, simple_format, sizeof(simple_format));
    printf("简单格式: %s\n", simple_format);
    
    // 详细格式（自定义格式化函数）
    format_error_detailed(&detailed_error, detailed_format, sizeof(detailed_format));
    printf("详细格式: %s\n", detailed_format);
    
    // JSON格式（自定义格式化函数）
    format_error_json(&detailed_error, json_format, sizeof(json_format));
    printf("JSON格式: %s\n", json_format);
}

void report_detailed_error(Stru_SemanticErrorReporterInterface_t* error_reporter,
                          const Stru_SemanticError_t* error) {
    // 使用现有接口报告错误
    error_reporter->report_error(error_reporter,
                                error->error_type,
                                error->
