# CN_Language 语义分析模块 API 文档

## 概述

语义分析模块是CN_Language项目的核心组件之一，负责对抽象语法树（AST）进行语义验证和分析。本模块采用分层架构和SOLID设计原则，提供可扩展、可替换的语义分析实现。

## 模块架构

语义分析模块采用模块化设计，分为以下子模块：

1. **符号表模块** (`symbol_table/`) - 管理程序中的符号信息
2. **作用域管理器模块** (`scope_manager/`) - 管理作用域层次结构
3. **类型检查器模块** (`type_checker/`) - 检查类型兼容性和推断类型
4. **语义错误报告器模块** (`error_reporter/`) - 收集和报告语义错误
5. **主语义分析器模块** (`analyzer/`) - 整合所有组件进行完整语义分析
6. **工厂模块** (`factory/`) - 提供统一的创建接口

## 设计原则

### 1. 接口设计
- 所有模块通过抽象接口暴露功能
- 接口使用前向声明，减少编译依赖
- 接口定义明确的输入输出契约

### 2. 依赖管理
- 遵循单向依赖：应用层 → 核心层 → 基础设施层
- 模块间通过接口指针通信
- 使用工厂模式创建实例

### 3. 错误处理
- 统一的错误报告机制
- 错误信息包含位置和类型
- 支持错误回调函数

### 4. 内存管理
- 谁创建谁销毁原则
- 通过接口的destroy函数释放资源
- 支持自定义内存分配器

## 快速开始

### 包含头文件

```c
// 包含主接口文件（包含所有子模块）
#include "src/core/semantic/CN_semantic_interface.h"

// 或者单独包含需要的模块
#include "src/core/semantic/symbol_table/CN_symbol_table.h"
#include "src/core/semantic/scope_manager/CN_scope_manager.h"
#include "src/core/semantic/type_checker/CN_type_checker.h"
#include "src/core/semantic/error_reporter/CN_error_reporter.h"
#include "src/core/semantic/analyzer/CN_semantic_analyzer.h"
#include "src/core/semantic/factory/CN_semantic_factory.h"
```

### 创建语义分析器

```c
// 方法1：使用工厂函数创建完整系统
Stru_SemanticAnalyzerInterface_t* analyzer = 
    F_create_complete_semantic_analyzer_system(100); // 最大100个错误

// 方法2：手动创建和组装组件
Stru_SymbolTableInterface_t* symbol_table = F_create_symbol_table_interface();
Stru_ScopeManagerInterface_t* scope_manager = F_create_scope_manager_interface();
Stru_TypeCheckerInterface_t* type_checker = F_create_type_checker_interface();
Stru_SemanticErrorReporterInterface_t* error_reporter = 
    F_create_semantic_error_reporter_interface();
Stru_SemanticAnalyzerInterface_t* analyzer = F_create_semantic_analyzer_interface();

// 初始化所有组件
symbol_table->initialize(symbol_table, NULL);
scope_manager->initialize(scope_manager, symbol_table);
type_checker->initialize(type_checker, scope_manager);
error_reporter->initialize(error_reporter, 100);
analyzer->initialize(analyzer, symbol_table, scope_manager, type_checker, error_reporter);
```

### 进行语义分析

```c
// 分析AST
void* ast_root = get_ast_root(); // 获取AST根节点
bool success = analyzer->analyze_ast(analyzer, ast_root);

// 检查错误
if (!success) {
    Stru_SemanticErrorReporterInterface_t* error_reporter = 
        analyzer->get_error_reporter(analyzer);
    
    size_t error_count = error_reporter->get_error_count(error_reporter);
    printf("发现 %zu 个语义错误\n", error_count);
}
```

### 清理资源

```c
// 销毁语义分析器（会自动销毁所有组件）
F_destroy_semantic_analyzer_interface(analyzer);

// 或者手动销毁各个组件
F_destroy_symbol_table_interface(symbol_table);
F_destroy_scope_manager_interface(scope_manager);
F_destroy_type_checker_interface(type_checker);
F_destroy_semantic_error_reporter_interface(error_reporter);
F_destroy_semantic_analyzer_interface(analyzer);
```

## 各模块详细文档

- [符号表模块](symbol_table/README.md) - 符号管理相关API
- [作用域管理器模块](scope_manager/README.md) - 作用域管理相关API
- [类型检查器模块](type_checker/README.md) - 类型检查相关API
- [语义错误报告器模块](error_reporter/README.md) - 错误报告相关API
- [主语义分析器模块](analyzer/README.md) - 主分析器相关API
- [工厂模块](factory/README.md) - 工厂函数相关API

## 数据类型

### 枚举类型

```c
// 符号类型枚举
typedef enum Eum_SymbolType {
    Eum_SYMBOL_VARIABLE,        // 变量符号
    Eum_SYMBOL_FUNCTION,        // 函数符号
    Eum_SYMBOL_STRUCT,          // 结构体符号
    Eum_SYMBOL_ENUM,            // 枚举符号
    Eum_SYMBOL_CONSTANT,        // 常量符号
    Eum_SYMBOL_PARAMETER,       // 参数符号
    Eum_SYMBOL_TYPE_ALIAS,      // 类型别名符号
    Eum_SYMBOL_MODULE,          // 模块符号
    Eum_SYMBOL_IMPORT,          // 导入符号
    Eum_SYMBOL_TEMPLATE         // 模板符号
} Eum_SymbolType;

// 作用域类型枚举
typedef enum Eum_ScopeType {
    Eum_SCOPE_GLOBAL,           // 全局作用域
    Eum_SCOPE_FUNCTION,         // 函数作用域
    Eum_SCOPE_BLOCK,            // 块作用域
    Eum_SCOPE_STRUCT,           // 结构体作用域
    Eum_SCOPE_LOOP,             // 循环作用域
    Eum_SCOPE_CONDITIONAL,      // 条件作用域
    Eum_SCOPE_TRY_CATCH         // try-catch作用域
} Eum_ScopeType;

// 类型兼容性枚举
typedef enum Eum_TypeCompatibility {
    Eum_TYPE_COMPATIBLE,        // 类型兼容
    Eum_TYPE_INCOMPATIBLE,      // 类型不兼容
    Eum_TYPE_CONVERTIBLE,       // 类型可转换
    Eum_TYPE_PROMOTABLE         // 类型可提升
} Eum_TypeCompatibility;

// 语义错误类型枚举
typedef enum Eum_SemanticErrorType {
    Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL,      // 未定义符号
    Eum_SEMANTIC_ERROR_REDEFINED_SYMBOL,      // 重复定义符号
    Eum_SEMANTIC_ERROR_TYPE_MISMATCH,         // 类型不匹配
    Eum_SEMANTIC_ERROR_INVALID_OPERATION,     // 无效操作
    Eum_SEMANTIC_ERROR_INVALID_ARGUMENTS,     // 无效参数
    Eum_SEMANTIC_ERROR_UNREACHABLE_CODE,      // 不可达代码
    Eum_SEMANTIC_ERROR_MISSING_RETURN,        // 缺少返回语句
    Eum_SEMANTIC_ERROR_INVALID_RETURN_TYPE,   // 无效返回类型
    Eum_SEMANTIC_ERROR_UNINITIALIZED_VARIABLE,// 未初始化变量
    Eum_SEMANTIC_ERROR_CONST_ASSIGNMENT,      // 常量赋值
    Eum_SEMANTIC_ERROR_SCOPE_VIOLATION,       // 作用域违规
    Eum_SEMANTIC_ERROR_CIRCULAR_DEPENDENCY,   // 循环依赖
    Eum_SEMANTIC_ERROR_INVALID_IMPORT,        // 无效导入
    Eum_SEMANTIC_ERROR_INVALID_EXPORT         // 无效导出
} Eum_SemanticErrorType;
```

### 结构体类型

```c
// 符号信息结构体
typedef struct Stru_SymbolInfo_t {
    const char* name;           // 符号名称
    Eum_SymbolType type;        // 符号类型
    void* type_info;            // 类型信息
    size_t line;                // 定义行号
    size_t column;              // 定义列号
    bool is_exported;           // 是否导出
    bool is_initialized;        // 是否已初始化
    bool is_constant;           // 是否为常量
    void* extra_data;           // 额外数据
} Stru_SymbolInfo_t;

// 作用域信息结构体
typedef struct Stru_ScopeInfo_t {
    Eum_ScopeType type;         // 作用域类型
    size_t depth;               // 作用域深度
    const char* name;           // 作用域名称
    Stru_SymbolTableInterface_t* symbol_table;  // 关联的符号表
    void* extra_data;           // 额外数据
} Stru_ScopeInfo_t;

// 类型检查结果结构体
typedef struct Stru_TypeCheckResult_t {
    Eum_TypeCompatibility compatibility;  // 类型兼容性
    const char* error_message;            // 错误消息
    void* converted_type;                 // 转换后的类型
    bool requires_explicit_cast;          // 是否需要显式转换
} Stru_TypeCheckResult_t;

// 语义错误信息结构体
typedef struct Stru_SemanticError_t {
    Eum_SemanticErrorType type;      // 错误类型
    const char* message;             // 错误消息
    size_t line;                     // 错误行号
    size_t column;                   // 错误列号
    const char* file_name;           // 文件名
    void* related_node;              // 相关AST节点
    void* extra_data;                // 额外数据
} Stru_SemanticError_t;
```

## 接口定义

所有接口都遵循相同的模式：

```c
struct Stru_InterfaceName_t {
    // 函数指针
    return_type (*function_name)(parameters...);
    
    // 私有数据指针
    void* private_data;
};
```

## 错误处理

语义分析模块使用统一的错误处理机制：

1. **错误收集**：所有语义错误通过错误报告器收集
2. **错误分类**：错误按类型分类，便于过滤和处理
3. **错误位置**：每个错误都包含行号、列号和文件名
4. **错误回调**：支持错误回调函数，实时处理错误

## 线程安全

- 接口实例不是线程安全的
- 多线程使用需要外部同步
- 建议每个线程创建独立的分析器实例

## 性能考虑

### 内存使用
- 接口实例内存开销小
- 符号表使用动态数组，支持自动扩容
- 作用域管理器使用栈结构，内存效率高

### 时间复杂度
- 符号查找：O(n) 线性查找
- 类型检查：O(1) 常量时间
- 作用域管理：O(1) 栈操作

## 扩展性

语义分析模块设计为高度可扩展：

1. **新类型系统**：通过扩展类型描述符和类型检查规则
2. **新符号表实现**：通过实现符号表接口
3. **新错误报告机制**：通过实现错误报告器接口
4. **新分析策略**：通过实现语义分析器接口

## 版本兼容性

- **版本 2.0.0**：模块化重构，向后兼容1.0.0接口
- **版本 1.0.0**：初始版本，统一接口设计

## 相关文档

- [架构设计文档](../../../architecture/001-中文编程语言CN_Language开发规划.md)
- [编码规范](../../../specifications/CN_Language项目 技术规范和编码标准.md)
- [测试文档](../../../../tests/core/semantic/README.md)

## 许可证

本模块遵循MIT许可证。详细信息请参阅项目根目录下的LICENSE文件。

---

*最后更新：2026年1月9日*
*文档版本：2.0.0*
*维护者：CN_Language语义分析团队*
