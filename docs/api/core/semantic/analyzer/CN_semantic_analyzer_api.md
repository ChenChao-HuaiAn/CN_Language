# CN_Semantic_Analyzer API 文档

## 概述

主语义分析器模块是CN_Language语义分析系统的核心组件，负责整合所有语义分析功能，提供完整的语义分析服务。该模块协调符号表管理、作用域分析、类型检查、错误报告等子模块，实现统一的语义分析接口。

## 模块职责

### 1. 协调功能
- 整合所有语义分析子模块
- 管理语义分析流程
- 协调模块间通信和数据传递

### 2. 分析功能
- AST遍历和分析
- 符号解析和绑定
- 类型检查和推断
- 语义规则验证

### 3. 错误处理
- 收集和报告语义错误
- 错误恢复和继续分析
- 错误分类和过滤

### 4. 状态管理
- 管理语义分析状态
- 支持分析过程重置
- 维护分析上下文

## 模块结构

```
src/core/semantic/analyzer/
├── CN_semantic_analyzer.h      # 主语义分析器接口定义
├── CN_semantic_analyzer.c      # 主语义分析器实现
├── CN_ast_analyzer.h           # AST分析器接口定义
├── CN_ast_analyzer.c           # AST分析器实现
└── README.md                   # 模块文档
```

## 接口定义

### 主语义分析器接口结构体

```c
typedef struct Stru_SemanticAnalyzerInterface_t
{
    // 初始化语义分析器
    bool (*initialize)(Stru_SemanticAnalyzerInterface_t* analyzer,
                      Stru_SymbolTableInterface_t* symbol_table,
                      Stru_ScopeManagerInterface_t* scope_manager,
                      Stru_TypeCheckerInterface_t* type_checker,
                      Stru_SemanticErrorReporterInterface_t* error_reporter);
    
    // 分析AST节点
    bool (*analyze_ast)(Stru_SemanticAnalyzerInterface_t* analyzer,
                       void* ast_node);
    
    // 分析单个声明
    bool (*analyze_declaration)(Stru_SemanticAnalyzerInterface_t* analyzer,
                               void* declaration_node);
    
    // 分析单个语句
    bool (*analyze_statement)(Stru_SemanticAnalyzerInterface_t* analyzer,
                             void* statement_node);
    
    // 分析单个表达式
    void* (*analyze_expression)(Stru_SemanticAnalyzerInterface_t* analyzer,
                               void* expression_node);
    
    // 获取符号表接口
    Stru_SymbolTableInterface_t* (*get_symbol_table)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    // 获取作用域管理器接口
    Stru_ScopeManagerInterface_t* (*get_scope_manager)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    // 获取类型检查器接口
    Stru_TypeCheckerInterface_t* (*get_type_checker)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    // 获取错误报告器接口
    Stru_SemanticErrorReporterInterface_t* (*get_error_reporter)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    // 重置语义分析器状态
    void (*reset)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    // 销毁语义分析器
    void (*destroy)(Stru_SemanticAnalyzerInterface_t* analyzer);
    
    // 私有数据指针
    void* private_data;
} Stru_SemanticAnalyzerInterface_t;
```

## API 函数

### 工厂函数

#### `F_create_semantic_analyzer_interface`

```c
Stru_SemanticAnalyzerInterface_t* F_create_semantic_analyzer_interface(void);
```

**功能**：创建主语义分析器接口实例。

**参数**：无

**返回值**：
- 成功：指向新创建的主语义分析器接口实例的指针
- 失败：NULL

**注意**：
- 创建的实例需要调用`initialize`函数进行初始化才能使用
- 实例包含所有必要的子模块接口指针

**示例**：
```c
Stru_SemanticAnalyzerInterface_t* analyzer = F_create_semantic_analyzer_interface();
if (analyzer == NULL) {
    // 处理错误
}
```

#### `F_destroy_semantic_analyzer_interface`

```c
void F_destroy_semantic_analyzer_interface(Stru_SemanticAnalyzerInterface_t* interface);
```

**功能**：销毁主语义分析器接口实例。

**参数**：
- `interface`：要销毁的主语义分析器接口实例

**返回值**：无

**注意**：
- 会释放所有相关资源
- 确保在销毁前不再使用该接口实例

**示例**：
```c
F_destroy_semantic_analyzer_interface(analyzer);
```

### 主语义分析器接口函数

#### `initialize`

```c
bool (*initialize)(Stru_SemanticAnalyzerInterface_t* analyzer,
                  Stru_SymbolTableInterface_t* symbol_table,
                  Stru_ScopeManagerInterface_t* scope_manager,
                  Stru_TypeCheckerInterface_t* type_checker,
                  Stru_SemanticErrorReporterInterface_t* error_reporter);
```

**功能**：初始化语义分析器。

**参数**：
- `analyzer`：语义分析器实例
- `symbol_table`：符号表接口实例
- `scope_manager`：作用域管理器接口实例
- `type_checker`：类型检查器接口实例
- `error_reporter`：错误报告器接口实例

**返回值**：
- `true`：初始化成功
- `false`：初始化失败

**注意**：
- 所有子模块接口实例必须已正确创建和初始化
- 初始化失败会保持分析器处于无效状态

**示例**：
```c
// 创建所有子模块
Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
Stru_TypeCheckerInterface_t* type_checker = F_create_type_checker_interface();
Stru_SemanticErrorReporterInterface_t* error_reporter = F_create_semantic_error_reporter_interface();

// 初始化子模块
symbol_table->initialize(symbol_table);
scope_manager->initialize(scope_manager);
type_checker->initialize(type_checker);
error_reporter->initialize(error_reporter);

// 初始化语义分析器
bool success = analyzer->initialize(analyzer, symbol_table, scope_manager, type_checker, error_reporter);
if (!success) {
    // 处理初始化失败
}
```

#### `analyze_ast`

```c
bool (*analyze_ast)(Stru_SemanticAnalyzerInterface_t* analyzer,
                   void* ast_node);
```

**功能**：分析AST节点。

**参数**：
- `analyzer`：语义分析器实例
- `ast_node`：AST根节点

**返回值**：
- `true`：分析成功（可能仍有警告）
- `false`：分析失败（有错误）

**注意**：
- 分析过程会收集所有语义错误和警告
- 即使有错误，分析也会尽可能继续
- 返回`false`表示有阻止编译继续的错误

**示例**：
```c
// 获取AST根节点
void* ast_root = get_ast_root();

// 分析AST
bool success = analyzer->analyze_ast(analyzer, ast_root);
if (!success) {
    // 获取错误信息
    Stru_SemanticErrorReporterInterface_t* error_reporter = analyzer->get_error_reporter(analyzer);
    size_t error_count = error_reporter->get_error_count(error_reporter);
    printf("语义分析失败，发现 %zu 个错误\n", error_count);
}
```

#### `analyze_declaration`

```c
bool (*analyze_declaration)(Stru_SemanticAnalyzerInterface_t* analyzer,
                           void* declaration_node);
```

**功能**：分析单个声明节点。

**参数**：
- `analyzer`：语义分析器实例
- `declaration_node`：声明AST节点

**返回值**：
- `true`：分析成功
- `false`：分析失败

**支持的声明类型**：
- 变量声明
- 函数声明
- 结构体声明
- 枚举声明
- 常量声明
- 参数声明

**示例**：
```c
// 分析变量声明
void* var_decl_node = get_variable_declaration_node();
bool success = analyzer->analyze_declaration(analyzer, var_decl_node);
if (!success) {
    // 处理声明分析失败
}
```

#### `analyze_statement`

```c
bool (*analyze_statement)(Stru_SemanticAnalyzerInterface_t* analyzer,
                         void* statement_node);
```

**功能**：分析单个语句节点。

**参数**：
- `analyzer`：语义分析器实例
- `statement_node`：语句AST节点

**返回值**：
- `true`：分析成功
- `false`：分析失败

**支持的语句类型**：
- 表达式语句
- 条件语句（if/else）
- 循环语句（while/for）
- 返回语句
- 跳转语句（break/continue）
- 块语句
- 开关语句（switch/case）
- 异常处理语句（try/catch/finally）

**示例**：
```c
// 分析if语句
void* if_stmt_node = get_if_statement_node();
bool success = analyzer->analyze_statement(analyzer, if_stmt_node);
if (!success) {
    // 处理语句分析失败
}
```

#### `analyze_expression`

```c
void* (*analyze_expression)(Stru_SemanticAnalyzerInterface_t* analyzer,
                           void* expression_node);
```

**功能**：分析单个表达式节点。

**参数**：
- `analyzer`：语义分析器实例
- `expression_node`：表达式AST节点

**返回值**：
- 成功：表达式的类型信息指针
- 失败：NULL

**支持的表达式类型**：
- 字面量表达式（整数、浮点数、字符串、布尔值）
- 标识符表达式
- 二元表达式（算术、关系、逻辑运算）
- 一元表达式（正负、逻辑非、位非）
- 函数调用表达式
- 赋值表达式
- 类型转换表达式
- 条件表达式（三元运算符）
- 成员访问表达式（点运算符）
- 下标表达式（数组访问）
- 新建表达式（new）
- 删除表达式（delete）

**示例**：
```c
// 分析二元表达式
void* binary_expr_node = get_binary_expression_node();
void* type_info = analyzer->analyze_expression(analyzer, binary_expr_node);
if (type_info == NULL) {
    // 处理表达式分析失败
} else {
    // 使用类型信息
    printf("表达式类型: %p\n", type_info);
}
```

#### `get_symbol_table`

```c
Stru_SymbolTableInterface_t* (*get_symbol_table)(Stru_SemanticAnalyzerInterface_t* analyzer);
```

**功能**：获取符号表接口。

**参数**：
- `analyzer`：语义分析器实例

**返回值**：符号表接口指针

**示例**：
```c
Stru_SymbolTableInterface_t* symbol_table = analyzer->get_symbol_table(analyzer);
if (symbol_table != NULL) {
    // 使用符号表
    size_t symbol_count = symbol_table->get_symbol_count(symbol_table);
    printf("符号表中有 %zu 个符号\n", symbol_count);
}
```

#### `get_scope_manager`

```c
Stru_ScopeManagerInterface_t* (*get_scope_manager)(Stru_SemanticAnalyzerInterface_t* analyzer);
```

**功能**：获取作用域管理器接口。

**参数**：
- `analyzer`：语义分析器实例

**返回值**：作用域管理器接口指针

**示例**：
```c
Stru_ScopeManagerInterface_t* scope_manager = analyzer->get_scope_manager(analyzer);
if (scope_manager != NULL) {
    // 使用作用域管理器
    size_t scope_depth = scope_manager->get_scope_depth(scope_manager);
    printf("当前作用域深度: %zu\n", scope_depth);
}
```

#### `get_type_checker`

```c
Stru_TypeCheckerInterface_t* (*get_type_checker)(Stru_SemanticAnalyzerInterface_t* analyzer);
```

**功能**：获取类型检查器接口。

**参数**：
- `analyzer`：语义分析器实例

**返回值**：类型检查器接口指针

**示例**：
```c
Stru_TypeCheckerInterface_t* type_checker = analyzer->get_type_checker(analyzer);
if (type_checker != NULL) {
    // 使用类型检查器
    // ...
}
```

#### `get_error_reporter`

```c
Stru_SemanticErrorReporterInterface_t* (*get_error_reporter)(Stru_SemanticAnalyzerInterface_t* analyzer);
```

**功能**：获取错误报告器接口。

**参数**：
- `analyzer`：语义分析器实例

**返回值**：错误报告器接口指针

**示例**：
```c
Stru_SemanticErrorReporterInterface_t* error_reporter = analyzer->get_error_reporter(analyzer);
if (error_reporter != NULL) {
    // 获取错误信息
    size_t error_count = error_reporter->get_error_count(error_reporter);
    size_t warning_count = error_reporter->get_warning_count(error_reporter);
    printf("发现 %zu 个错误，%zu 个警告\n", error_count, warning_count);
}
```

#### `reset`

```c
void (*reset)(Stru_SemanticAnalyzerInterface_t* analyzer);
```

**功能**：重置语义分析器状态。

**参数**：
- `analyzer`：语义分析器实例

**返回值**：无

**注意**：
- 重置会清空所有分析状态
- 符号表、作用域管理器等子模块状态也会被重置
- 错误报告器中的错误和警告会被清空

**示例**：
```c
// 重置分析器状态，准备分析新文件
analyzer->reset(analyzer);
```

#### `destroy`

```c
void (*destroy)(Stru_SemanticAnalyzerInterface_t* analyzer);
```

**功能**：销毁语义分析器。

**参数**：
- `analyzer`：语义分析器实例

**返回值**：无

**注意**：
- 销毁后不能再使用分析器实例
- 不会销毁子模块接口实例，需要单独销毁

**示例**：
```c
analyzer->destroy(analyzer);
```

## 使用示例

### 完整语义分析流程示例

```c
#include "CN_semantic_interface.h"

// 完整的语义分析流程
bool perform_semantic_analysis(void* ast_root)
{
    // 1. 创建所有子模块
    Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
    Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
    Stru_TypeCheckerInterface_t* type_checker = F_create_type_checker_interface();
    Stru_SemanticErrorReporterInterface_t* error_reporter = F_create_semantic_error_reporter_interface();
    
    if (symbol_table == NULL || scope_manager == NULL || 
        type_checker == NULL || error_reporter == NULL) {
        // 清理已创建的资源
        if (symbol_table) F_destroy_symbol_table_interface(symbol_table);
        if (scope_manager) F_destroy_scope_manager_interface(scope_manager);
        if (type_checker) F_destroy_type_checker_interface(type_checker);
        if (error_reporter) F_destroy_semantic_error_reporter_interface(error_reporter);
        return false;
    }
    
    // 2. 初始化子模块
    symbol_table->initialize(symbol_table);
    scope_manager->initialize(scope_manager);
    type_checker->initialize(type_checker);
    error_reporter->initialize(error_reporter);
    
    // 3. 创建主语义分析器
    Stru_SemanticAnalyzerInterface_t* analyzer = F_create_semantic_analyzer_interface();
    if (analyzer == NULL) {
        // 清理资源
        F_destroy_symbol_table_interface(symbol_table);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_type_checker_interface(type_checker);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        return false;
    }
    
    // 4. 初始化主语义分析器
    bool init_success = analyzer->initialize(analyzer, symbol_table, scope_manager, 
                                            type_checker, error_reporter);
    if (!init_success) {
        F_destroy_semantic_analyzer_interface(analyzer);
        F_destroy_symbol_table_interface(symbol_table);
        F_destroy_scope_manager_interface(scope_manager);
        F_destroy_type_checker_interface(type_checker);
        F_destroy_semantic_error_reporter_interface(error_reporter);
        return false;
    }
    
    // 5. 执行语义分析
    bool analysis_success = analyzer->analyze_ast(analyzer, ast_root);
    
    // 6. 检查分析结果
    size_t error_count = error_reporter->get_error_count(error_reporter);
    size_t warning_count = error_reporter->get_warning_count(error_reporter);
    
    if (error_count > 0) {
        printf("语义分析完成，发现 %zu 个错误，%zu 个警告\n", error_count, warning_count);
        
        // 获取所有错误
        Stru_SemanticError_t** errors = error_reporter->get_all_errors(error_reporter, &error_count);
        for (size_t i = 0; i < error_count; i++) {
            printf("错误 %zu: %s (行 %zu, 列 %zu)\n", 
                   i + 1, errors[i]->message, errors[i]->line, errors[i]->column);
        }
        free(errors);
    } else {
        printf("语义分析成功，发现 %zu 个警告\n", warning_count);
    }
    
    // 7. 清理资源
    F_destroy_semantic_analyzer_interface(analyzer);
    F_destroy_symbol_table_interface
