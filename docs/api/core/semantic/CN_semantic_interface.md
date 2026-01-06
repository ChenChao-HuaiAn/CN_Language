# CN_Language 语义分析接口 API 文档

## 概述

语义分析接口是CN_Language项目的核心组件之一，负责对抽象语法树（AST）进行语义验证和分析。本接口采用面向接口的编程模式，提供完整的抽象接口定义，支持多种语义分析策略（类型检查、符号表管理、作用域分析等）。

## 接口架构

语义分析采用分层接口设计：

```
┌─────────────────────────────────────────┐
│   Stru_SemanticAnalyzerInterface_t      │ (主语义分析器接口)
├─────────────────────────────────────────┤
│  Stru_SymbolTableInterface_t            │ (符号表接口)
│  Stru_ScopeManagerInterface_t           │ (作用域管理器接口)
│  Stru_TypeCheckerInterface_t            │ (类型检查器接口)
│  Stru_SemanticErrorReporterInterface_t  │ (语义错误报告接口)
└─────────────────────────────────────────┘
```

## 接口定义

### Stru_SemanticAnalyzerInterface_t

语义分析器的主接口，提供统一的语义分析访问。

```c
typedef struct Stru_SemanticAnalyzerInterface_t {
    // 初始化函数
    bool (*initialize)(Stru_SemanticAnalyzerInterface_t* analyzer, const char* config);
    
    // 组件访问
    Stru_SymbolTableInterface_t* (*get_symbol_table)(Stru_SemanticAnalyzerInterface_t* analyzer);
    Stru_ScopeManagerInterface_t* (*get_scope_manager)(Stru_SemanticAnalyzerInterface_t* analyzer);
    Stru_TypeCheckerInterface_t* (*get_type_checker)(Stru_SemanticAnalyzerInterface_t* analyzer);
    Stru_SemanticErrorReporterInterface_t* (*get_error_reporter)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    // AST分析
    bool (*analyze_ast)(Stru_SemanticAnalyzerInterface_t* analyzer, Stru_ASTNode_t* ast_root);
    bool (*analyze_module)(Stru_SemanticAnalyzerInterface_t* analyzer, const char* module_name, Stru_ASTNode_t* module_ast);
    
    // 符号查询
    bool (*lookup_symbol)(Stru_SemanticAnalyzerInterface_t* analyzer, const char* symbol_name, Stru_SymbolInfo_t* result);
    bool (*resolve_type)(Stru_SemanticAnalyzerInterface_t* analyzer, Stru_ASTNode_t* type_node, Stru_TypeDescriptor_t* result);
    
    // 资源管理
    void (*destroy)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    // 私有数据
    void* private_data;
} Stru_SemanticAnalyzerInterface_t;
```

### Stru_SymbolTableInterface_t

符号表管理接口，负责符号的存储和查找。

```c
typedef struct Stru_SymbolTableInterface_t {
    // 初始化函数
    bool (*initialize)(Stru_SymbolTableInterface_t* symbol_table, size_t initial_capacity);
    
    // 符号管理
    bool (*add_symbol)(Stru_SymbolTableInterface_t* symbol_table, const Stru_SymbolInfo_t* symbol);
    bool (*remove_symbol)(Stru_SymbolTableInterface_t* symbol_table, const char* symbol_name);
    bool (*update_symbol)(Stru_SymbolTableInterface_t* symbol_table, const char* symbol_name, const Stru_SymbolInfo_t* new_info);
    
    // 符号查找
    bool (*lookup_symbol)(Stru_SymbolTableInterface_t* symbol_table, const char* symbol_name, Stru_SymbolInfo_t* result);
    bool (*lookup_symbol_in_scope)(Stru_SymbolTableInterface_t* symbol_table, const char* symbol_name, uint32_t scope_level, Stru_SymbolInfo_t* result);
    bool (*find_symbols_by_type)(Stru_SymbolTableInterface_t* symbol_table, const char* type_name, Stru_SymbolInfo_t* results, size_t max_results, size_t* found_count);
    
    // 作用域支持
    bool (*set_parent_scope)(Stru_SymbolTableInterface_t* symbol_table, Stru_SymbolTableInterface_t* parent_scope);
    uint32_t (*get_current_scope_level)(Stru_SymbolTableInterface_t* symbol_table);
    
    // 统计信息
    size_t (*get_symbol_count)(Stru_SymbolTableInterface_t* symbol_table);
    size_t (*get_memory_usage)(Stru_SymbolTableInterface_t* symbol_table);
    
    // 遍历功能
    bool (*iterate_symbols)(Stru_SymbolTableInterface_t* symbol_table, 
                           bool (*callback)(const Stru_SymbolInfo_t* symbol, void* user_data), 
                           void* user_data);
    
    // 资源管理
    void (*destroy)(Stru_SymbolTableInterface_t* symbol_table);
    
    // 私有数据
    void* private_data;
} Stru_SymbolTableInterface_t;
```

### Stru_ScopeManagerInterface_t

作用域管理器接口，负责作用域的创建和管理。

```c
typedef struct Stru_ScopeManagerInterface_t {
    // 初始化函数
    bool (*initialize)(Stru_ScopeManagerInterface_t* scope_manager);
    
    // 作用域管理
    bool (*enter_scope)(Stru_ScopeManagerInterface_t* scope_manager, const char* scope_name);
    bool (*leave_scope)(Stru_ScopeManagerInterface_t* scope_manager);
    bool (*create_named_scope)(Stru_ScopeManagerInterface_t* scope_manager, const char* scope_name, uint32_t* scope_id);
    
    // 作用域查询
    uint32_t (*get_current_scope_level)(Stru_ScopeManagerInterface_t* scope_manager);
    const char* (*get_current_scope_name)(Stru_ScopeManagerInterface_t* scope_manager);
    bool (*get_scope_info)(Stru_ScopeManagerInterface_t* scope_manager, uint32_t scope_level, Stru_ScopeInfo_t* result);
    
    // 作用域层次
    bool (*is_in_scope)(Stru_ScopeManagerInterface_t* scope_manager, uint32_t scope_level);
    bool (*is_parent_scope)(Stru_ScopeManagerInterface_t* scope_manager, uint32_t parent_level, uint32_t child_level);
    uint32_t (*find_common_ancestor)(Stru_ScopeManagerInterface_t* scope_manager, uint32_t scope1, uint32_t scope2);
    
    // 作用域遍历
    bool (*iterate_scopes)(Stru_ScopeManagerInterface_t* scope_manager,
                          bool (*callback)(const Stru_ScopeInfo_t* scope, void* user_data),
                          void* user_data);
    
    // 资源管理
    void (*destroy)(Stru_ScopeManagerInterface_t* scope_manager);
    
    // 私有数据
    void* private_data;
} Stru_ScopeManagerInterface_t;
```

### Stru_TypeCheckerInterface_t

类型检查器接口，负责类型验证和推断。

```c
typedef struct Stru_TypeCheckerInterface_t {
    // 初始化函数
    bool (*initialize)(Stru_TypeCheckerInterface_t* type_checker, const char* type_rules_config);
    
    // 类型检查
    bool (*check_type_compatibility)(Stru_TypeCheckerInterface_t* type_checker, 
                                    const Stru_TypeDescriptor_t* type1, 
                                    const Stru_TypeDescriptor_t* type2);
    bool (*check_assignment_compatibility)(Stru_TypeCheckerInterface_t* type_checker,
                                          const Stru_TypeDescriptor_t* target_type,
                                          const Stru_TypeDescriptor_t* source_type);
    bool (*check_function_call)(Stru_TypeCheckerInterface_t* type_checker,
                               const Stru_TypeDescriptor_t* function_type,
                               const Stru_TypeDescriptor_t* arg_types,
                               size_t arg_count);
    
    // 类型推断
    bool (*infer_expression_type)(Stru_TypeCheckerInterface_t* type_checker,
                                 Stru_ASTNode_t* expression_node,
                                 Stru_TypeDescriptor_t* result);
    bool (*infer_function_return_type)(Stru_TypeCheckerInterface_t* type_checker,
                                      Stru_ASTNode_t* function_node,
                                      Stru_TypeDescriptor_t* result);
    
    // 类型转换
    bool (*can_convert_type)(Stru_TypeCheckerInterface_t* type_checker,
                            const Stru_TypeDescriptor_t* from_type,
                            const Stru_TypeDescriptor_t* to_type);
    bool (*get_conversion_cost)(Stru_TypeCheckerInterface_t* type_checker,
                               const Stru_TypeDescriptor_t* from_type,
                               const Stru_TypeDescriptor_t* to_type,
                               int* cost);
    
    // 类型注册
    bool (*register_type)(Stru_TypeCheckerInterface_t* type_checker,
                         const Stru_TypeDescriptor_t* type_descriptor);
    bool (*unregister_type)(Stru_TypeCheckerInterface_t* type_checker,
                           const char* type_name);
    
    // 类型查询
    bool (*get_type_descriptor)(Stru_TypeCheckerInterface_t* type_checker,
                               const char* type_name,
                               Stru_TypeDescriptor_t* result);
    bool (*is_type_defined)(Stru_TypeCheckerInterface_t* type_checker,
                           const char* type_name);
    
    // 资源管理
    void (*destroy)(Stru_TypeCheckerInterface_t* type_checker);
    
    // 私有数据
    void* private_data;
} Stru_TypeCheckerInterface_t;
```

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

### Stru_SymbolInfo_t

符号信息结构体。

```c
typedef struct Stru_SymbolInfo_t {
    char name[CN_MAX_SYMBOL_NAME_LENGTH];  // 符号名称
    char type_name[CN_MAX_TYPE_NAME_LENGTH]; // 类型名称
    uint32_t scope_level;                   // 作用域级别
    bool is_constant;                       // 是否为常量
    bool is_initialized;                    // 是否已初始化
    void* value;                            // 符号值（可选）
    void* extra_data;                       // 额外数据（可选）
    uint64_t flags;                         // 标志位
} Stru_SymbolInfo_t;
```

### Stru_ScopeInfo_t

作用域信息结构体。

```c
typedef struct Stru_ScopeInfo_t {
    uint32_t scope_id;                      // 作用域ID
    char scope_name[CN_MAX_SCOPE_NAME_LENGTH]; // 作用域名称
    uint32_t parent_scope_id;               // 父作用域ID
    uint32_t depth;                         // 作用域深度
    size_t symbol_count;                    // 符号数量
    void* extra_data;                       // 额外数据（可选）
} Stru_ScopeInfo_t;
```

### Stru_TypeDescriptor_t

类型描述符结构体。

```c
typedef struct Stru_TypeDescriptor_t {
    char type_name[CN_MAX_TYPE_NAME_LENGTH]; // 类型名称
    Eum_TypeCategory type_category;         // 类型类别
    size_t size;                            // 类型大小（字节）
    bool is_signed;                         // 是否有符号
    bool is_const;                          // 是否为常量类型
    uint32_t alignment;                     // 对齐要求
    void* extra_data;                       // 额外数据（可选）
} Stru_TypeDescriptor_t;
```

### Eum_TypeCategory

类型类别枚举。

```c
typedef enum Eum_TypeCategory {
    Eum_TYPE_CATEGORY_PRIMITIVE,           // 基本类型
    Eum_TYPE_CATEGORY_COMPOSITE,           // 复合类型
    Eum_TYPE_CATEGORY_FUNCTION,            // 函数类型
    Eum_TYPE_CATEGORY_ARRAY,               // 数组类型
    Eum_TYPE_CATEGORY_POINTER,             // 指针类型
    Eum_TYPE_CATEGORY_USER_DEFINED,        // 用户定义类型
    Eum_TYPE_CATEGORY_GENERIC,             // 泛型类型
    Eum_TYPE_CATEGORY_UNKNOWN              // 未知类型
} Eum_TypeCategory;
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

### Stru_SemanticError_t

语义错误结构体。

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

## API 参考

### F_create_semantic_analyzer_interface

创建主语义分析器接口实例。

**函数签名：**
```c
Stru_SemanticAnalyzerInterface_t* F_create_semantic_analyzer_interface(void);
```

**返回值：**
- `Stru_SemanticAnalyzerInterface_t*`：新创建的语义分析器接口实例
- `NULL`：创建失败（内存不足）

**示例：**
```c
Stru_SemanticAnalyzerInterface_t* analyzer = F_create_semantic_analyzer_interface();
if (analyzer == NULL) {
    fprintf(stderr, "无法创建语义分析器\n");
    return 1;
}

// 使用语义分析器...

analyzer->destroy(analyzer);
```

### analyzer->initialize

初始化语义分析器。

**函数签名：**
```c
bool initialize(Stru_SemanticAnalyzerInterface_t* analyzer, const char* config);
```

**参数：**
- `analyzer`：语义分析器接口指针
- `config`：配置字符串（JSON格式），可为NULL使用默认配置

**返回值：**
- `true`：初始化成功
- `false`：初始化失败

**配置示例：**
```json
{
    "max_errors": 100,
    "strict_mode": true,
    "enable_warnings": true,
    "type_inference": true,
    "scope_tracking": true,
    "symbol_table_size": 1024,
    "error_callback": null
}
```

### analyzer->analyze_ast

分析抽象语法树。

**函数签名：**
```c
bool analyze_ast(Stru_SemanticAnalyzerInterface_t* analyzer, Stru_ASTNode_t* ast_root);
```

**参数：**
- `analyzer`：语义分析器接口指针
- `ast_root`：AST根节点

**返回值：**
- `true`：分析成功（没有错误或只有警告）
- `false`：分析失败（有错误）

### analyzer->lookup_symbol

查找符号。

**函数签名：**
```c
bool lookup_symbol(Stru_SemanticAnalyzerInterface_t* analyzer, const char* symbol_name, Stru_SymbolInfo_t* result);
```

**参数：**
- `analyzer`：语义分析器接口指针
- `symbol_name`：符号名称
- `result`：输出参数，存储找到的符号信息

**返回值：**
- `true`：找到符号
- `false`：未找到符号

### F_create_symbol_table_interface

创建符号表接口实例。

**函数签名：**
```c
Stru_SymbolTableInterface_t* F_create_symbol_table_interface(void);
```

### F_create_scope_manager_interface

创建作用域管理器接口实例。

**函数签名：**
```c
Stru_ScopeManagerInterface_t* F_create_scope_manager_interface(void);
```

### F_create_type_checker_interface

创建类型检查器接口实例。

**函数签名：**
```c
Stru_TypeCheckerInterface_t* F_create_type_checker_interface(void);
```

### F_create_semantic_error_reporter_interface

创建语义错误报告接口实例。

**函数签名：**
```c
Stru_SemanticErrorReporterInterface_t* F_create_semantic_error_reporter_interface(void);
```

## 使用模式

### 基本使用模式

```c
#include "CN_semantic_interface.h"

int main() {
    // 创建语义分析器
    Stru_SemanticAnalyzerInterface_t* analyzer = F_create_semantic_analyzer_interface();
    if (analyzer == NULL) {
        return 1;
    }
    
    // 初始化语义分析器
    const char* config = "{\"max_errors\": 100, \"strict_mode\": true}";
    if (!analyzer->initialize(analyzer, config)) {
        analyzer->destroy(analyzer);
        return 1;
    }
    
    // 获取AST根节点（假设有函数提供）
    Stru_ASTNode_t* ast_root = get_ast_from_source("let x = 10 + 20;");
    
    // 分析AST
    bool success = analyzer->analyze_ast(analyzer, ast_root);
    
    if (!success) {
        // 获取错误信息
        Stru_SemanticErrorReporterInterface_t* error_reporter = analyzer->get_error_reporter(analyzer);
        uint32_t error_count = error_reporter->get_error_count(error_reporter);
        
        for (uint32_t i = 0; i < error_count; i++) {
            Stru_SemanticError_t error;
            error_reporter->get_error(error_reporter, i, &error);
            printf("错误 %u: %s (行: %u, 列: %u)\n", 
                   i, error.message, error.line, error.column);
        }
    }
    
    // 查找符号
    Stru_SymbolInfo_t symbol;
    if (analyzer->lookup_symbol(analyzer, "x", &symbol)) {
        printf("找到符号: %s, 类型: %s\n", symbol.name, symbol.type_name);
    }
    
    // 清理
    analyzer->destroy(analyzer);
    
    return success ? 0 : 1;
}
```

### 符号表使用模式

```c
void symbol_table_example(Stru_SemanticAnalyzerInterface_t* analyzer) {
    // 获取符号表接口
    Stru_SymbolTableInterface_t* symbol_table = analyzer->get_symbol_table(analyzer);
    
    // 初始化符号表
    if (!symbol_table->initialize(symbol_table, 1024)) {
        return;
    }
    
    // 添加符号
    Stru_SymbolInfo_t symbol1 = {
        .name = "变量1",
        .type_name = "整数",
        .scope_level = 0,
        .is_constant = false,
        .is_initialized = true,
        .value = NULL,
        .extra_data = NULL,
        .flags = 0
    };
    
    Stru_SymbolInfo_t symbol2 = {
        .name = "常量1",
        .type_name = "浮点数",
        .scope_level = 0,
        .is_constant = true,
        .is_initialized = true,
        .value = NULL,
        .extra_data = NULL,
        .flags = 0
    };
    
    symbol_table->add_symbol(symbol_table, &symbol1);
    symbol_table->add_symbol(symbol_table, &symbol2);
    
    // 查找符号
    Stru_SymbolInfo_t found_symbol;
    if (symbol_table->lookup_symbol(symbol_table, "变量1", &found_symbol)) {
        printf("找到符号: %s, 类型: %s, 常量: %s\n", 
               found_symbol.name, found_symbol.type_name,
               found_symbol.is_constant ? "是" : "否");
    }
    
    // 遍历符号
    symbol_table->iterate_symbols(symbol_table, symbol_callback, NULL);
    
    // 获取统计信息
    size_t count = symbol_table->get_symbol_count(symbol_table);
    size_t memory = symbol_table->get_memory_usage(symbol_table);
    printf("符号数量: %zu, 内存使用: %zu 字节\n", count, memory);
}

bool symbol_callback(const Stru_SymbolInfo_t* symbol, void* user_data) {
    printf("符号: %s (类型: %s)\n", symbol->name, symbol->type_name);
    return true; // 继续遍历
}
```

### 类型检查使用模式

```c
void type_checking_example(Stru_SemanticAnalyzerInterface_t* analyzer) {
    // 获取类型检查器接口
    Stru_TypeCheckerInterface_t* type_checker = analyzer->get_type_checker(analyzer);
    
    // 初始化类型检查器
    if (!type_checker->initialize(type_checker, "{\"strict_mode\": true}")) {
        return;
    }
    
    // 注册类型
    Stru_TypeDescriptor_t int_type = {
        .type_name = "整数",
        .type_category = Eum_TYPE_CATEGORY_PRIMITIVE,
        .size = 4,
        .is_signed = true,
        .is_const = false,
        .alignment = 4,
        .extra_data = NULL
    };
    
    Stru_TypeDescriptor_t float_type = {
        .type_name = "浮点数",
        .type_category = Eum_TYPE_CATEGORY_PRIMITIVE,
        .size = 8,
        .is_signed = true,
        .is_const = false,
        .alignment = 8,
        .extra_data = NULL
    };
    
    type_checker->register_type(type_checker, &int_type);
    type_checker->register_type(type_checker, &float_type);
    
    // 检查类型兼容性
    bool compatible = type_checker->check_type_compatibility(type_checker, &int_type, &float_type);
    printf("整数和浮点数兼容: %s\n", compatible ? "是" : "否");
    
    // 检查赋值兼容性
    bool assignable = type_checker->check_assignment_compatibility(type_checker, &float_type, &int_type);
    printf("整数可赋值给浮点数: %s\n", assignable ? "是" : "否");
    
    // 推断表达式类型
    Stru_ASTNode_t* expression = create_binary_expression("+", 
        create_literal_node("10", &int_type),
        create_literal_node("20", &int_type));
    
    Stru_TypeDescriptor_t inferred_type;
    if (type_checker->infer_expression_type(type_checker, expression, &inferred_type)) {
        printf("表达式类型: %s\n", inferred_type.type_name);
    }
}
```

### 作用域管理使用模式

```c
void scope_management_example(Stru_SemanticAnalyzerInterface_t* analyzer) {
    // 获取作用域管理器接口
    Stru_ScopeManagerInterface_t* scope_manager = analyzer->get_scope_manager(analyzer);
    
    // 初始化作用域管理器
    if (!scope_manager->initialize(scope_manager)) {
        return;
    }
    
    // 进入全局作用域
    scope_manager->enter_scope(scope_manager, "全局作用域");
    
    // 进入函数作用域
    scope_manager->enter_scope(scope_manager, "函数作用域");
    
    // 获取当前作用域信息
    uint32_t current_level = scope_manager->get_current_scope_level(scope_manager);
    const char* current_name = scope_manager->get_current_scope_name(scope_manager);
    printf("当前作用域: %s (级别: %u)\n", current_name, current_level);
    
    // 检查作用域关系
    bool in_global = scope_manager->is_in_scope(scope_manager, 0);
    bool is_parent = scope_manager->is_parent_scope(scope_manager, 0, 1);
    printf("在全局作用域中: %s\n", in_global ? "是" : "否");
    printf("全局是函数的父作用域: %s\n", is_parent ? "是" : "否");
    
    // 离开函数作用域
    scope_manager->leave_scope(scope_manager);
    
    // 离开全局作用域
    scope_manager->leave_scope(scope_manager);
}
```

### 错误报告使用模式

```c
void error_reporting_example(Stru_SemanticAnalyzerInterface_t* analyzer) {
    // 获取错误报告接口
    Stru_SemanticErrorReporterInterface_t* error_reporter = analyzer->get_error_reporter(analyzer);
    
    // 初始化错误报告器
    if (!error_reporter->initialize(error_reporter, 100)) {
        return;
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
}

void error_callback(const Stru_SemanticError_t* error, void* user_data) {
    printf("[错误回调] %s: %s (行: %u, 列: %u)\n",
           error->module_name, error->message,
           error->line, error->column);
}
```

## 错误处理

### 错误类型

语义分析可能遇到以下类型的错误：

1. **符号错误**：未定义符号、重复符号、符号作用域违规
2. **类型错误**：类型不匹配、无效类型操作、类型转换错误
3. **作用域错误**：作用域嵌套错误、作用域访问违规
4. **语法错误**：虽然语法分析阶段已处理，但语义分析可能发现更深层次的语法问题
5. **逻辑错误**：语义上无效的操作，如除以零的常量表达式
6. **配置错误**：无效的配置参数
7. **资源错误**：内存不足、文件访问失败

### 错误恢复策略

语义分析器实现以下错误恢复策略：

1. **继续分析**：报告错误后继续分析其他部分
2. **错误抑制**：在达到最大错误数量后停止报告新错误
3. **部分结果**：即使有错误也提供部分分析结果
4. **错误上下文**：提供详细的错误上下文信息
5. **建议修复**：提供可能的修复建议

### 错误信息格式

错误信息包含以下内容：
- 错误类型和严重性
- 错误位置（文件、行、列）
- 错误描述和上下文
- 相关符号和类型信息
- 建议的修复方法

## 性能考虑

### 内存使用

1. **接口实例**：每个接口约80-160字节
2. **符号表**：每个符号约64-128字节，取决于额外数据
3. **类型系统**：类型描述符约48-96字节
4. **错误报告**：每个错误约128-256字节
5. **作用域管理**：每个作用域约32-64字节

### 时间复杂性

1. **符号查找**：平均O(1)，使用哈希表
2. **类型检查**：O(n)，取决于类型系统的复杂性
3. **作用域操作**：O(1) 栈操作
4. **错误报告**：O(1) 添加错误，O(n) 遍历错误
5. **AST遍历**：O(n)，其中n为AST节点数量

### 优化建议

1. **符号表优化**：使用合适的哈希函数和负载因子
2. **类型缓存**：缓存类型检查结果
3. **作用域池**：重用作用域对象
4. **延迟初始化**：按需初始化组件
5. **批量操作**：支持批量符号添加和类型注册

## 线程安全性

### 线程安全级别

语义分析接口是**非线程安全**的。多个线程不能同时访问同一个接口实例。

### 线程安全使用模式

```c
// 每个线程创建自己的分析器实例
void* analysis_thread(void* arg) {
    Stru_SemanticAnalyzerInterface_t* analyzer = F_create_semantic_analyzer_interface();
    analyzer->initialize(analyzer, NULL);
    
    // ... 进行语义分析 ...
    
    analyzer->destroy(analyzer);
    return NULL;
}

// 或者使用互斥锁保护共享实例
pthread_mutex_t analyzer_mutex = PTHREAD_MUTEX_INITIALIZER;
Stru_SemanticAnalyzerInterface_t* shared_analyzer = NULL;

void safe_analysis(Stru_ASTNode_t* ast) {
    pthread_mutex_lock(&analyzer_mutex);
    
    if (shared_analyzer == NULL) {
        shared_analyzer = F_create_semantic_analyzer_interface();
        shared_analyzer->initialize(shared_analyzer, NULL);
    }
    
    bool success = shared_analyzer->analyze_ast(shared_analyzer, ast);
    
    pthread_mutex_unlock(&analyzer_mutex);
}
```

## 版本兼容性

### API 稳定性

语义分析接口遵循以下版本兼容性规则：

1. **主版本号变更**：不兼容的API修改
2. **次版本号变更**：向下兼容的功能性新增
3. **修订号变更**：向下兼容的问题修正

### 向后兼容性保证

1. 现有函数签名不会改变
2. 现有枚举值不会删除
3. 结构体布局保持稳定
4. 错误代码含义不变

### 废弃策略

1. 废弃的API会标记为`DEPRECATED`
2. 废弃的API至少保留两个主版本
3. 提供迁移指南和替代方案

## 扩展指南

### 添加新的类型系统

1. 扩展类型描述符结构
2. 实现新的类型检查规则
3. 更新类型兼容性检查函数
4. 提供类型转换函数

### 添加新的符号表实现

1. 创建新的符号表接口实现
2. 实现符号的添加、查找和删除
3. 支持作用域链查找
4. 提供工厂函数

### 添加新的错误报告机制

1. 扩展错误报告接口
2. 实现新的错误收集和报告策略
3. 支持错误过滤和分类
4. 保持向后兼容性

## 测试策略

### 单元测试

测试每个接口函数的正确性：
- 正常情况测试
- 边界条件测试
- 错误处理测试
- 内存泄漏测试

### 集成测试

测试接口组合使用：
- 符号表 + 作用域管理器集成
- 类型检查器 + 错误报告器集成
- 完整语义分析流程测试

### 性能测试

建立性能基准：
- 符号查找性能
- 类型检查性能
- 错误报告性能
- 内存使用性能

### 兼容性测试

测试不同配置和环境的兼容性：
- 不同符号表大小配置
- 启用/禁用严格模式
- 启用/禁用类型推断
- 启用/禁用作用域跟踪

## 相关文档

- [语义分析模块 README](../../../../src/core/semantic/README.md)
- [架构设计文档](../../../architecture/001-中文编程语言CN_Language开发规划.md)
- [编码规范](../../../specifications/CN_Language项目 技术规范和编码标准.md)
- [AST模块文档](../ast/CN_ast_api.md)

## 修订历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本发布 |
| 1.0.1 | 2026-01-06 | 修复符号表查找错误 |
| 1.1.0 | 2026-01-06 | 添加作用域管理器接口 |
| 1.2.0 | 2026-01-06 | 添加类型检查器接口 |
| 1.3.0 | 2026-01-06 | 添加语义错误报告接口 |

## 版权声明

版权所有 © 2026 CN_Language项目团队。保留所有权利。

本文档是CN_Language项目的一部分，遵循项目许可证条款。
