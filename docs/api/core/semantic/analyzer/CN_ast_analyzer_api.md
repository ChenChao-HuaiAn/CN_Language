# CN_Language AST分析器模块 API 文档

## 概述

AST分析器模块是CN_Language语义分析系统的核心组件，负责AST遍历、符号解析和绑定、语义错误检测等功能。本模块提供完整的AST节点分析能力，支持程序、模块、声明、语句和表达式的语义分析，为编译器提供强大的语义检查能力。

## 接口定义

### Stru_AstAnalyzerInterface_t

AST分析器接口，负责AST遍历和语义分析。

```c
typedef struct Stru_AstAnalyzerInterface_t {
    // 初始化函数
    bool (*initialize)(Stru_AstAnalyzerInterface_t* analyzer,
                      Stru_SemanticAnalyzerInterface_t* semantic_analyzer);
    
    // AST分析
    bool (*analyze_ast)(Stru_AstAnalyzerInterface_t* analyzer,
                       Stru_AstNodeInterface_t* ast_node);
    
    // 程序节点分析
    bool (*analyze_program)(Stru_AstAnalyzerInterface_t* analyzer,
                           Stru_AstNodeInterface_t* node,
                           Stru_AstAnalysisContext_t* context);
    
    // 模块节点分析
    bool (*analyze_module)(Stru_AstAnalyzerInterface_t* analyzer,
                          Stru_AstNodeInterface_t* node,
                          Stru_AstAnalysisContext_t* context);
    
    // 声明节点分析
    bool (*analyze_declaration)(Stru_AstAnalyzerInterface_t* analyzer,
                               Stru_AstNodeInterface_t* node,
                               Stru_AstAnalysisContext_t* context);
    
    // 变量声明分析
    bool (*analyze_variable_declaration)(Stru_AstAnalyzerInterface_t* analyzer,
                                        Stru_AstNodeInterface_t* node,
                                        Stru_AstAnalysisContext_t* context);
    
    // 函数声明分析
    bool (*analyze_function_declaration)(Stru_AstAnalyzerInterface_t* analyzer,
                                        Stru_AstNodeInterface_t* node,
                                        Stru_AstAnalysisContext_t* context);
    
    // 结构体声明分析
    bool (*analyze_struct_declaration)(Stru_AstAnalyzerInterface_t* analyzer,
                                      Stru_AstNodeInterface_t* node,
                                      Stru_AstAnalysisContext_t* context);
    
    // 语句节点分析
    bool (*analyze_statement)(Stru_AstAnalyzerInterface_t* analyzer,
                             Stru_AstNodeInterface_t* node,
                             Stru_AstAnalysisContext_t* context);
    
    // 表达式语句分析
    bool (*analyze_expression_statement)(Stru_AstAnalyzerInterface_t* analyzer,
                                        Stru_AstNodeInterface_t* node,
                                        Stru_AstAnalysisContext_t* context);
    
    // if语句分析
    bool (*analyze_if_statement)(Stru_AstAnalyzerInterface_t* analyzer,
                                Stru_AstNodeInterface_t* node,
                                Stru_AstAnalysisContext_t* context);
    
    // while语句分析
    bool (*analyze_while_statement)(Stru_AstAnalyzerInterface_t* analyzer,
                                   Stru_AstNodeInterface_t* node,
                                   Stru_AstAnalysisContext_t* context);
    
    // for语句分析
    bool (*analyze_for_statement)(Stru_AstAnalyzerInterface_t* analyzer,
                                 Stru_AstNodeInterface_t* node,
                                 Stru_AstAnalysisContext_t* context);
    
    // return语句分析
    bool (*analyze_return_statement)(Stru_AstAnalyzerInterface_t* analyzer,
                                    Stru_AstNodeInterface_t* node,
                                    Stru_AstAnalysisContext_t* context);
    
    // 表达式节点分析
    void* (*analyze_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                               Stru_AstNodeInterface_t* node,
                               Stru_AstAnalysisContext_t* context);
    
    // 二元表达式分析
    void* (*analyze_binary_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                                      Stru_AstNodeInterface_t* node,
                                      Stru_AstAnalysisContext_t* context);
    
    // 一元表达式分析
    void* (*analyze_unary_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                                     Stru_AstNodeInterface_t* node,
                                     Stru_AstAnalysisContext_t* context);
    
    // 字面量表达式分析
    void* (*analyze_literal_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                                       Stru_AstNodeInterface_t* node,
                                       Stru_AstAnalysisContext_t* context);
    
    // 标识符表达式分析（符号解析）
    void* (*analyze_identifier_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                                          Stru_AstNodeInterface_t* node,
                                          Stru_AstAnalysisContext_t* context);
    
    // 函数调用表达式分析
    void* (*analyze_call_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                                    Stru_AstNodeInterface_t* node,
                                    Stru_AstAnalysisContext_t* context);
    
    // 赋值表达式分析
    void* (*analyze_assignment_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                                          Stru_AstNodeInterface_t* node,
                                          Stru_AstAnalysisContext_t* context);
    
    // 类型转换表达式分析
    void* (*analyze_cast_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                                    Stru_AstNodeInterface_t* node,
                                    Stru_AstAnalysisContext_t* context);
    
    // 条件表达式分析
    void* (*analyze_conditional_expression)(Stru_AstAnalyzerInterface_t* analyzer,
                                           Stru_AstNodeInterface_t* node,
                                           Stru_AstAnalysisContext_t* context);
    
    // 用户数据管理
    void (*set_user_data)(Stru_AstAnalyzerInterface_t* analyzer,
                         void* user_data);
    void* (*get_user_data)(Stru_AstAnalyzerInterface_t* analyzer);
    
    // 状态管理
    void (*reset)(Stru_AstAnalyzerInterface_t* analyzer);
    
    // 资源管理
    void (*destroy)(Stru_AstAnalyzerInterface_t* analyzer);
    
    // 私有数据
    void* private_data;
} Stru_AstAnalyzerInterface_t;
```

## 辅助数据结构

### Stru_AstAnalysisContext_t

AST分析上下文结构体，存储分析过程中的上下文信息。

```c
typedef struct Stru_AstAnalysisContext_t {
    Stru_SemanticAnalyzerInterface_t* semantic_analyzer;  ///< 语义分析器接口
    Stru_AstNodeInterface_t* current_node;                ///< 当前分析的节点
    size_t depth;                                         ///< 当前分析深度
    void* user_data;                                      ///< 用户自定义数据
} Stru_AstAnalysisContext_t;
```

### Eum_AstNodeType

AST节点类型枚举。

```c
typedef enum Eum_AstNodeType {
    // 程序结构
    Eum_AST_PROGRAM,              // 程序节点
    Eum_AST_MODULE,               // 模块节点
    
    // 声明类型
    Eum_AST_VARIABLE_DECL,        // 变量声明
    Eum_AST_FUNCTION_DECL,        // 函数声明
    Eum_AST_STRUCT_DECL,          // 结构体声明
    Eum_AST_ENUM_DECL,            // 枚举声明
    Eum_AST_CONSTANT_DECL,        // 常量声明
    Eum_AST_PARAMETER_DECL,       // 参数声明
    
    // 语句类型
    Eum_AST_EXPRESSION_STMT,      // 表达式语句
    Eum_AST_IF_STMT,              // if语句
    Eum_AST_WHILE_STMT,           // while语句
    Eum_AST_FOR_STMT,             // for语句
    Eum_AST_RETURN_STMT,          // return语句
    Eum_AST_BREAK_STMT,           // break语句
    Eum_AST_CONTINUE_STMT,        // continue语句
    Eum_AST_BLOCK_STMT,           // 块语句
    
    // 表达式类型
    Eum_AST_BINARY_EXPR,          // 二元表达式
    Eum_AST_UNARY_EXPR,           // 一元表达式
    Eum_AST_LITERAL_EXPR,         // 字面量表达式
    Eum_AST_IDENTIFIER_EXPR,      // 标识符表达式
    Eum_AST_CALL_EXPR,            // 函数调用表达式
    Eum_AST_ASSIGN_EXPR,          // 赋值表达式
    Eum_AST_CAST_EXPR,            // 类型转换表达式
    Eum_AST_CONDITIONAL_EXPR,     // 条件表达式
    
    // 字面量类型
    Eum_AST_INT_LITERAL,          // 整数字面量
    Eum_AST_FLOAT_LITERAL,        // 浮点数字面量
    Eum_AST_STRING_LITERAL,       // 字符串字面量
    Eum_AST_BOOL_LITERAL,         // 布尔字面量
    Eum_AST_CHAR_LITERAL,         // 字符字面量
    Eum_AST_NULL_LITERAL,         // null字面量
    
    // 其他类型
    Eum_AST_TYPE_NODE,            // 类型节点
    Eum_AST_COMMENT_NODE,         // 注释节点
    Eum_AST_DIRECTIVE_NODE,       // 指令节点
    Eum_AST_ATTRIBUTE_NODE,       // 属性节点
    Eum_AST_ANNOTATION_NODE,      // 注解节点
    Eum_AST_DOCUMENTATION_NODE,   // 文档节点
    Eum_AST_UNKNOWN_NODE          // 未知节点
} Eum_AstNodeType;
```

### Stru_AstAnalysisResult_t

AST分析结果结构体。

```c
typedef struct Stru_AstAnalysisResult_t {
    bool success;                                          // 分析是否成功
    size_t error_count;                                    // 错误数量
    size_t warning_count;                                  // 警告数量
    Stru_SemanticError_t* errors;                          // 错误数组
    Stru_SemanticError_t* warnings;                        // 警告数组
    void* analysis_data;                                   // 分析数据
    size_t node_count;                                     // 分析的节点数量
    size_t symbol_count;                                   // 解析的符号数量
    double analysis_time;                                  // 分析时间（秒）
} Stru_AstAnalysisResult_t;
```

## API 参考

### F_create_ast_analyzer_interface

创建AST分析器接口实例。

**函数签名：**
```c
Stru_AstAnalyzerInterface_t* F_create_ast_analyzer_interface(void);
```

**返回值：**
- `Stru_AstAnalyzerInterface_t*`：新创建的AST分析器接口实例
- `NULL`：创建失败（内存不足）

**示例：**
```c
Stru_AstAnalyzerInterface_t* ast_analyzer = F_create_ast_analyzer_interface();
if (ast_analyzer == NULL) {
    fprintf(stderr, "无法创建AST分析器\n");
    return 1;
}

// 使用AST分析器...

ast_analyzer->destroy(ast_analyzer);
```

### ast_analyzer->initialize

初始化AST分析器。

**函数签名：**
```c
bool initialize(Stru_AstAnalyzerInterface_t* analyzer,
               Stru_SemanticAnalyzerInterface_t* semantic_analyzer);
```

**参数：**
- `analyzer`：AST分析器接口指针
- `semantic_analyzer`：语义分析器接口指针

**返回值：**
- `true`：初始化成功
- `false`：初始化失败

### ast_analyzer->analyze_ast

分析AST节点。

**函数签名：**
```c
bool analyze_ast(Stru_AstAnalyzerInterface_t* analyzer,
                Stru_AstNodeInterface_t* ast_node);
```

**参数：**
- `analyzer`：AST分析器接口指针
- `ast_node`：AST根节点

**返回值：**
- `true`：分析成功
- `false`：分析失败

### ast_analyzer->analyze_variable_declaration

分析变量声明节点。

**函数签名：**
```c
bool analyze_variable_declaration(Stru_AstAnalyzerInterface_t* analyzer,
                                 Stru_AstNodeInterface_t* node,
                                 Stru_AstAnalysisContext_t* context);
```

**参数：**
- `analyzer`：AST分析器接口指针
- `node`：变量声明节点
- `context`：分析上下文

**返回值：**
- `true`：分析成功
- `false`：分析失败

**功能说明：**
1. 检查变量名称是否合法
2. 检查同一作用域内是否已存在同名符号
3. 分析变量类型和初始化表达式
4. 将变量信息插入符号表
5. 设置符号的作用域和生命周期信息

### ast_analyzer->analyze_function_declaration

分析函数声明节点。

**函数签名：**
```c
bool analyze_function_declaration(Stru_AstAnalyzerInterface_t* analyzer,
                                 Stru_AstNodeInterface_t* node,
                                 Stru_AstAnalysisContext_t* context);
```

**参数：**
- `analyzer`：AST分析器接口指针
- `node`：函数声明节点
- `context`：分析上下文

**返回值：**
- `true`：分析成功
- `false`：分析失败

**功能说明：**
1. 检查函数名称是否合法
2. 检查同一作用域内是否已存在同名符号
3. 进入函数作用域
4. 分析函数参数和返回类型
5. 分析函数体
6. 退出函数作用域
7. 将函数信息插入符号表

### ast_analyzer->analyze_identifier_expression

分析标识符表达式节点（符号解析）。

**函数签名：**
```c
void* analyze_identifier_expression(Stru_AstAnalyzerInterface_t* analyzer,
                                   Stru_AstNodeInterface_t* node,
                                   Stru_AstAnalysisContext_t* context);
```

**参数：**
- `analyzer`：AST分析器接口指针
- `node`：标识符表达式节点
- `context`：分析上下文

**返回值：**
- `void*`：符号的类型信息指针
- `NULL`：符号未定义或分析失败

**功能说明：**
1. 获取标识符名称
2. 在当前作用域中查找符号
3. 如果未找到，在父作用域中查找
4. 如果找到，返回符号的类型信息
5. 如果未找到，报告未定义符号错误

### ast_analyzer->destroy

销毁AST分析器，释放所有资源。

**函数签名：**
```c
void destroy(Stru_AstAnalyzerInterface_t* analyzer);
```

## 使用示例

### 基本使用模式

```c
#include "CN_ast_analyzer.h"
#include "CN_semantic_factory.h"

int main() {
    // 创建语义分析器系统
    Stru_SemanticAnalyzerInterface_t* semantic_analyzer = 
        F_create_complete_semantic_analyzer_system_optimized(100);
    if (semantic_analyzer == NULL) {
        return 1;
    }
    
    // 创建AST分析器
    Stru_AstAnalyzerInterface_t* ast_analyzer = F_create_ast_analyzer_interface();
    if (ast_analyzer == NULL) {
        F_destroy_semantic_analyzer_interface(semantic_analyzer);
        return 1;
    }
    
    // 初始化AST分析器
    if (!ast_analyzer->initialize(ast_analyzer, semantic_analyzer)) {
        ast_analyzer->destroy(ast_analyzer);
        F_destroy_semantic_analyzer_interface(semantic_analyzer);
        return 1;
    }
    
    // 假设有AST根节点
    Stru_AstNodeInterface_t* ast_root = create_ast_from_source("示例代码");
    if (ast_root == NULL) {
        ast_analyzer->destroy(ast_analyzer);
        F_destroy_semantic_analyzer_interface(semantic_analyzer);
        return 1;
    }
    
    // 分析AST
    bool analysis_success = ast_analyzer->analyze_ast(ast_analyzer, ast_root);
    if (!analysis_success) {
        // 获取错误信息
        Stru_SemanticErrorReporterInterface_t* error_reporter = 
            semantic_analyzer->get_error_reporter(semantic_analyzer);
        size_t error_count = error_reporter->get_error_count(error_reporter);
        
        printf("分析失败，发现 %zu 个错误：\n", error_count);
        for (size_t i = 0; i < error_count; i++) {
            Stru_SemanticError_t* error = error_reporter->get_error(error_reporter, i);
            printf("错误 %zu: %s (行: %zu, 列: %zu)\n", 
                   i + 1, error->message, error->line, error->column);
        }
    } else {
        printf("AST分析成功完成\n");
    }
    
    // 清理资源
    destroy_ast_node(ast_root);
    ast_analyzer->destroy(ast_analyzer);
    F_destroy_semantic_analyzer_interface(semantic_analyzer);
    
    return 0;
}
```

### 符号解析和绑定示例

```c
void symbol_resolution_example(Stru_AstAnalyzerInterface_t* ast_analyzer,
                              Stru_SemanticAnalyzerInterface_t* semantic_analyzer) {
    // 创建变量声明AST节点
    Stru_AstNodeInterface_t* var_decl = create_variable_declaration("计数器", "整数", "10");
    
    // 分析变量声明（符号绑定）
    Stru_AstAnalysisContext_t context;
    context.semantic_analyzer = semantic_analyzer;
    context.current_node = var_decl;
    context.depth = 0;
    context.user_data = NULL;
    
    bool bind_success = ast_analyzer->analyze_variable_declaration(
        ast_analyzer, var_decl, &context);
    
    if (bind_success) {
        printf("变量声明分析成功，符号已绑定\n");
        
        // 创建标识符表达式AST节点
        Stru_AstNodeInterface_t* identifier_expr = create_identifier_expression("计数器");
        
        // 分析标识符表达式（符号解析）
        void* symbol_type = ast_analyzer->analyze_identifier_expression(
            ast_analyzer, identifier_expr, &context);
        
        if (symbol_type != NULL) {
            printf("符号解析成功，符号类型: %p\n", symbol_type);
        } else {
            printf("符号解析失败\n");
        }
        
        // 清理AST节点
        destroy_ast_node(identifier_expr);
    } else {
        printf("变量声明分析失败\n");
    }
    
        // 清理AST节点
        destroy_ast_node(var_decl);
}

## 错误处理

### 错误类型

AST分析器操作可能遇到以下类型的错误：

1. **内存错误**：内存分配失败
2. **参数错误**：无效的参数（如NULL指针）
3. **语法错误**：AST节点结构不符合语法规则
4. **语义错误**：符号未定义、重复声明、类型不匹配等
5. **作用域错误**：违反作用域规则
6. **生命周期错误**：变量在生命周期外被使用

### 错误处理策略

1. **返回值检查**：所有函数都返回bool表示成功/失败，或返回NULL表示失败
2. **详细错误信息**：通过错误报告器提供详细的语义错误信息
3. **错误恢复**：部分失败不会影响其他操作，分析器会继续分析其他节点
4. **错误位置跟踪**：记录错误发生的具体位置（行号、列号、文件名）
5. **错误分类和过滤**：支持按类型、位置等条件过滤错误

## 性能

### 性能优化策略

1. **增量分析**：只分析变更的部分，避免重复分析
2. **结果缓存**：缓存分析结果，避免重复计算
3. **并行分析**：支持并行分析不同的AST子树
4. **延迟分析**：延迟非必要的分析操作
5. **符号表优化**：使用哈希表和缓存优化符号查找

### 性能指标

1. **分析时间**：AST分析的总时间
2. **内存使用**：分析过程中使用的内存量
3. **节点处理速度**：每秒处理的AST节点数量
4. **符号解析速度**：每秒解析的符号数量
5. **错误检测速度**：每秒检测的错误数量

## 设计原则

### 单一职责原则
- 每个函数不超过50行代码
- 每个文件不超过500行代码
- AST分析器只负责AST遍历和语义分析
- 符号解析和绑定功能分离到专门的函数

### 开闭原则
- 通过接口支持扩展
- 支持添加新的AST节点类型分析
- 支持不同的分析策略和算法

### 依赖倒置原则
- 高层模块定义接口
- 低层模块实现接口
- 通过依赖注入管理组件依赖

### 接口隔离原则
- 为不同客户端提供专用接口
- AST分析器接口与语义分析器接口分离
- 最小化接口依赖

### 里氏替换原则
- 接口实现完全遵守契约
- 子类可无缝替换父类
- 通过单元测试验证替换性

## 扩展性

### 新的AST节点类型
可以通过实现相应的分析函数来支持新的AST节点类型，无需修改现有代码。

### 自定义分析策略
支持通过配置选择不同的分析策略和优化级别。

### 插件架构
支持通过插件添加自定义的分析功能。

## 维护说明

### 代码规范
- 遵循项目编码标准
- 使用统一的命名约定（Stru_前缀表示结构体，F_前缀表示函数）
- 添加充分的注释和文档
- 每个函数前添加函数说明注释

### 版本管理
- 使用语义化版本控制
- 保持向后兼容性
- 明确记录变更历史

### 问题跟踪
- 记录已知问题和限制
- 跟踪性能瓶颈
- 收集用户反馈

## 参考资料

1. 《编译原理》（龙书）：语义分析的基本理论
2. 《现代编译原理》：语义分析的实际实现
3. CN_Language项目架构文档：项目整体架构设计
4. CN_Language语法规范：语言语法定义

## 版本历史

### v1.0.0 (2026-01-10)
- 初始版本
- 实现AST遍历和分析的基本框架
- 实现符号解析和绑定功能
- 提供完整的接口定义和基本实现
- 集成错误检测和报告功能

### 未来计划
- 实现更完整的类型分析
- 添加更多的语义检查
- 提高分析性能和准确性
- 支持更多的优化功能

---

*文档版本：1.0.0*
*创建日期：2026-01-10*
*维护者：CN_Language架构委员会*
