# CN_Language 错误报告器模块

## 概述

错误报告器模块负责收集、管理和报告语义分析过程中发现的错误。本模块提供灵活的错误收集机制、错误分类、错误位置跟踪和错误信息格式化功能。

## 功能特性

- **错误收集**：收集语义分析过程中的所有错误
- **错误分类**：支持错误严重性分级（错误、警告、信息）
- **位置跟踪**：记录错误发生的行号和列号
- **错误格式化**：提供格式化的错误信息输出
- **错误过滤**：支持按严重性过滤错误
- **内存管理**：自动管理错误信息内存，防止内存泄漏

## 接口定义

### 错误信息结构

```c
typedef struct Stru_SemanticError_t
{
    Eum_SemanticErrorType type;          ///< 错误类型
    Eum_SemanticErrorSeverity severity;  ///< 错误严重性
    const char* message;                 ///< 错误信息
    size_t line;                         ///< 错误行号
    size_t column;                       ///< 错误列号
    const char* file_name;               ///< 文件名（可选）
    void* extra_data;                    ///< 额外数据指针
} Stru_SemanticError_t;
```

### 错误报告器接口

```c
typedef struct Stru_SemanticErrorReporterInterface_t
{
    bool (*initialize)(Stru_SemanticErrorReporterInterface_t* self, void* config);
    bool (*report_error)(Stru_SemanticErrorReporterInterface_t* self,
                        const Stru_SemanticError_t* error);
    bool (*report_error_details)(Stru_SemanticErrorReporterInterface_t* self,
                                Eum_SemanticErrorType type,
                                Eum_SemanticErrorSeverity severity,
                                const char* message,
                                size_t line,
                                size_t column,
                                const char* file_name);
    size_t (*get_error_count)(Stru_SemanticErrorReporterInterface_t* self);
    size_t (*get_error_count_by_severity)(Stru_SemanticErrorReporterInterface_t* self,
                                         Eum_SemanticErrorSeverity severity);
    size_t (*get_all_errors)(Stru_SemanticErrorReporterInterface_t* self,
                            Stru_SemanticError_t* errors,
                            size_t max_errors);
    size_t (*get_errors_by_severity)(Stru_SemanticErrorReporterInterface_t* self,
                                    Stru_SemanticError_t* errors,
                                    size_t max_errors,
                                    Eum_SemanticErrorSeverity severity);
    bool (*has_errors)(Stru_SemanticErrorReporterInterface_t* self);
    bool (*has_errors_of_severity)(Stru_SemanticErrorReporterInterface_t* self,
                                  Eum_SemanticErrorSeverity severity);
    void (*clear_errors)(Stru_SemanticErrorReporterInterface_t* self);
    void (*destroy)(Stru_SemanticErrorReporterInterface_t* self);
    void* private_data;
} Stru_SemanticErrorReporterInterface_t;
```

## 使用示例

### 基本使用

```c
#include "CN_error_reporter.h"

int main() {
    // 创建错误报告器实例
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        F_create_semantic_error_reporter_interface();
    
    if (!error_reporter) {
        return 1;
    }
    
    // 初始化错误报告器（最大100个错误）
    if (!error_reporter->initialize(error_reporter, (void*)100)) {
        F_destroy_semantic_error_reporter_interface(error_reporter);
        return 1;
    }
    
    // 报告类型错误
    Stru_SemanticError_t type_error = {
        .type = Eum_SEMANTIC_ERROR_TYPE_MISMATCH,
        .severity = Eum_SEMANTIC_ERROR_SEVERITY_ERROR,
        .message = "类型不匹配：不能将字符串赋值给整数变量",
        .line = 10,
        .column = 5,
        .file_name = "test.cn",
        .extra_data = NULL
    };
    
    if (error_reporter->report_error(error_reporter, &type_error)) {
        printf("错误报告成功\n");
    }
    
    // 使用便捷函数报告错误
    error_reporter->report_error_details(
        error_reporter,
        Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL,
        Eum_SEMANTIC_ERROR_SEVERITY_ERROR,
        "未定义的符号：unknown_variable",
        15,
        8,
        "test.cn"
    );
    
    // 报告警告
    error_reporter->report_error_details(
        error_reporter,
        Eum_SEMANTIC_ERROR_UNUSED_VARIABLE,
        Eum_SEMANTIC_ERROR_SEVERITY_WARNING,
        "未使用的变量：temp",
        20,
        3,
        "test.cn"
    );
    
    // 获取错误统计信息
    size_t total_errors = error_reporter->get_error_count(error_reporter);
    size_t error_count = error_reporter->get_error_count_by_severity(
        error_reporter, Eum_SEMANTIC_ERROR_SEVERITY_ERROR);
    size_t warning_count = error_reporter->get_error_count_by_severity(
        error_reporter, Eum_SEMANTIC_ERROR_SEVERITY_WARNING);
    
    printf("总问题数: %zu\n", total_errors);
    printf("错误数: %zu\n", error_count);
    printf("警告数: %zu\n", warning_count);
    
    // 检查是否有错误
    if (error_reporter->has_errors(error_reporter)) {
        printf("发现语义错误\n");
    }
    
    if (error_reporter->has_errors_of_severity(
        error_reporter, Eum_SEMANTIC_ERROR_SEVERITY_ERROR)) {
        printf("发现严重错误\n");
    }
    
    // 获取所有错误
    Stru_SemanticError_t errors[10];
    size_t actual_count = error_reporter->get_all_errors(error_reporter, errors, 10);
    
    for (size_t i = 0; i < actual_count; i++) {
        printf("错误 %zu: %s (行: %zu, 列: %zu, 文件: %s)\n",
               i, errors[i].message, errors[i].line, 
               errors[i].column, errors[i].file_name ? errors[i].file_name : "<未知>");
    }
    
    // 清除所有错误
    error_reporter->clear_errors(error_reporter);
    printf("错误已清除，剩余错误数: %zu\n", error_reporter->get_error_count(error_reporter));
    
    // 销毁错误报告器
    F_destroy_semantic_error_reporter_interface(error_reporter);
    
    return 0;
}
```

### 集成使用示例

```c
void semantic_analysis_with_error_reporting(void) {
    // 创建错误报告器
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        F_create_semantic_error_reporter_interface();
    error_reporter->initialize(error_reporter, (void*)50); // 最大50个错误
    
    // 创建语义分析器（传入错误报告器）
    Stru_SemanticAnalyzerInterface_t* analyzer = 
        F_create_semantic_analyzer_interface(error_reporter);
    
    if (!analyzer) {
        F_destroy_semantic_error_reporter_interface(error_reporter);
        return;
    }
    
    // 进行语义分析
    void* ast_root = get_ast_root(); // 假设有AST根节点
    bool success = analyzer->analyze_ast(analyzer, ast_root);
    
    if (!success) {
        // 获取错误信息
        size_t error_count = error_reporter->get_error_count(error_reporter);
        printf("语义分析失败，发现 %zu 个问题\n", error_count);
        
        // 只获取错误（不包括警告）
        Stru_SemanticError_t errors[20];
        size_t actual_errors = error_reporter->get_errors_by_severity(
            error_reporter, errors, 20, Eum_SEMANTIC_ERROR_SEVERITY_ERROR);
        
        for (size_t i = 0; i < actual_errors; i++) {
            printf("[错误] %s (行 %zu)\n", errors[i].message, errors[i].line);
        }
        
        // 获取警告
        size_t actual_warnings = error_reporter->get_errors_by_severity(
            error_reporter, errors, 20, Eum_SEMANTIC_ERROR_SEVERITY_WARNING);
        
        for (size_t i = 0; i < actual_warnings; i++) {
            printf("[警告] %s (行 %zu)\n", errors[i].message, errors[i].line);
        }
    } else {
        printf("语义分析成功\n");
    }
    
    // 销毁资源
    F_destroy_semantic_analyzer_interface(analyzer);
    F_destroy_semantic_error_reporter_interface(error_reporter);
}
```

## 实现细节

### 数据结构

错误报告器使用动态数组存储错误信息：

```c
typedef struct Stru_SemanticErrorReporterPrivateData_t
{
    Stru_SemanticError_t* errors;        ///< 错误数组
    size_t capacity;                     ///< 数组容量
    size_t size;                         ///< 当前错误数量
    size_t max_errors;                   ///< 最大错误数量（0表示无限制）
} Stru_SemanticErrorReporterPrivateData_t;
```

### 算法复杂度

- **报告错误**：O(1) 平均情况（动态数组追加）
- **获取错误数量**：O(1)（直接返回大小）
- **获取所有错误**：O(n)（需要复制错误数组）
- **按严重性过滤错误**：O(n)（需要遍历错误数组）
- **清除错误**：O(1)（重置大小）

### 内存管理

- 错误报告器自动管理错误信息内存
- 报告错误时复制错误信息
- 支持最大错误数量限制，防止内存耗尽
- 销毁时释放所有相关内存

## 配置选项

错误报告器支持以下配置选项：

```c
// 通过初始化参数配置
void* config = (void*)100; // 最大100个错误

// 或者使用配置结构
typedef struct Stru_ErrorReporterConfig_t
{
    size_t max_errors;                   ///< 最大错误数量（0表示无限制）
    bool store_call_stack;               ///< 是否存储调用栈
    void* (*malloc_func)(size_t);        ///< 内存分配函数
    void (*free_func)(void*);            ///< 内存释放函数
} Stru_ErrorReporterConfig_t;
```

## 错误类型和严重性

### 错误类型

```c
typedef enum Eum_SemanticErrorType
{
    Eum_SEMANTIC_ERROR_TYPE_MISMATCH,        ///< 类型不匹配
    Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL,     ///< 未定义符号
    Eum_SEMANTIC_ERROR_REDEFINED_SYMBOL,     ///< 重复定义符号
    Eum_SEMANTIC_ERROR_UNUSED_VARIABLE,      ///< 未使用变量
    Eum_SEMANTIC_ERROR_UNREACHABLE_CODE,     ///< 不可达代码
    Eum_SEMANTIC_ERROR_INVALID_OPERATION,    ///< 无效操作
    Eum_SEMANTIC_ERROR_INVALID_ARGUMENTS,    ///< 无效参数
    Eum_SEMANTIC_ERROR_MISSING_RETURN,       ///< 缺少返回语句
    Eum_SEMANTIC_ERROR_INVALID_RETURN_TYPE,  ///< 无效返回类型
    Eum_SEMANTIC_ERROR_SCOPE_ERROR,          ///< 作用域错误
    Eum_SEMANTIC_ERROR_GENERIC               ///< 通用语义错误
} Eum_SemanticErrorType;
```

### 错误严重性

```c
typedef enum Eum_SemanticErrorSeverity
{
    Eum_SEMANTIC_ERROR_SEVERITY_INFO,        ///< 信息（不影响编译）
    Eum_SEMANTIC_ERROR_SEVERITY_WARNING,     ///< 警告（可能有问题）
    Eum_SEMANTIC_ERROR_SEVERITY_ERROR        ///< 错误（编译失败）
} Eum_SemanticErrorSeverity;
```

## 错误处理

错误报告器模块提供以下错误处理机制：

1. **内存分配失败**：返回false，不报告错误
2. **达到最大错误数量**：停止收集新错误，返回false
3. **无效参数**：返回false或0
4. **配置错误**：初始化失败

## 测试策略

### 单元测试

```c
// 测试错误报告和获取
void test_error_reporting_and_retrieval(void) {
    Stru_SemanticErrorReporterInterface_t* reporter = 
        F_create_semantic_error_reporter_interface();
    TEST_ASSERT_NOT_NULL(reporter);
    TEST_ASSERT_TRUE(reporter->initialize(reporter, (void*)10));
    
    // 报告错误
    Stru_SemanticError_t error = {
        .type = Eum_SEMANTIC_ERROR_TYPE_MISMATCH,
        .severity = Eum_SEMANTIC_ERROR_SEVERITY_ERROR,
        .message = "测试错误",
        .line = 1,
        .column = 1
    };
    
    TEST_ASSERT_TRUE(reporter->report_error(reporter, &error));
    TEST_ASSERT_EQUAL(1, reporter->get_error_count(reporter));
    TEST_ASSERT_TRUE(reporter->has_errors(reporter));
    
    // 获取错误
    Stru_SemanticError_t errors[5];
    size_t count = reporter->get_all_errors(reporter, errors, 5);
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL_STRING("测试错误", errors[0].message);
    
    F_destroy_semantic_error_reporter_interface(reporter);
}
```

### 集成测试

- 测试错误数量限制
- 测试错误过滤功能
- 测试内存管理
- 测试错误恢复机制

## 性能优化建议

1. **错误池**：预分配错误对象，减少内存分配
2. **批量操作**：支持批量错误报告和获取
3. **错误缓存**：缓存常用错误信息
4. **延迟格式化**：延迟错误信息格式化，提高性能

## 扩展指南

### 添加新的错误类型

1. 在`Eum_SemanticErrorType`枚举中添加新类型
2. 更新错误信息结构（如果需要）
3. 添加相应的错误处理逻辑
4. 添加测试用例

### 自定义错误报告器实现

1. 实现`Stru_SemanticErrorReporterInterface_t`接口
2. 提供工厂函数
3. 实现所有接口方法
4. 确保内存管理正确

## 相关模块

- **语义分析器**：使用错误报告器报告语义错误
- **类型检查器**：报告类型相关错误
- **符号表模块**：报告符号相关错误
- **作用域管理器**：报告作用域相关错误

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本 |
| 2.0.0 | 2026-01-09 | 模块化重构 |
| 2.0.1 | 2026-01-10 | 完善文档和测试 |

## 维护信息

- **最后更新**：2026年1月10日
- **维护者**：CN_Language语义分析团队
- **架构版本**：2.0.0
- **兼容性**：向后兼容所有版本

## 许可证

本模块遵循MIT许可证。详细信息请参阅项目根目录下的LICENSE文件。
