# CN_Language 语法分析器模块 API 文档

## 概述

语法分析器模块是CN_Language编译器的核心组件之一，负责将词法分析器生成的令牌序列转换为抽象语法树（AST）。本模块采用接口模式设计，实现了模块间的解耦，支持灵活的语法分析策略和错误处理机制。

### 主要功能

1. **语法分析**：解析CN语言程序，构建完整的抽象语法树
2. **AST构建**：创建和管理AST节点，支持复杂的程序结构表示
3. **错误处理**：检测和报告语法错误，支持错误恢复机制
4. **状态管理**：维护语法分析器的内部状态，支持增量分析

### 架构位置

- **所属层级**：核心层（Core Layer）
- **依赖模块**：基础设施层（内存管理、容器）、词法分析器、AST模块
- **被依赖模块**：语义分析器、代码生成器、应用层工具

## 核心接口

### Stru_ParserInterface_t

语法分析器抽象接口，定义了语法分析器的完整功能集。

#### 结构定义

```c
typedef struct Stru_ParserInterface_t {
    // 初始化函数
    bool (*initialize)(struct Stru_ParserInterface_t* interface, Stru_LexerInterface_t* lexer_interface);
    
    // 核心功能 - 语法分析
    Stru_AstNode_t* (*parse_program)(struct Stru_ParserInterface_t* interface);
    Stru_AstNode_t* (*parse_statement)(struct Stru_ParserInterface_t* interface);
    Stru_AstNode_t* (*parse_expression)(struct Stru_ParserInterface_t* interface);
    Stru_AstNode_t* (*parse_declaration)(struct Stru_ParserInterface_t* interface);
    
    // 核心功能 - AST构建
    Stru_AstNode_t* (*create_ast_node)(struct Stru_ParserInterface_t* interface, Eum_AstNodeType type, Stru_Token_t* token);
    bool (*add_child_node)(struct Stru_ParserInterface_t* interface, Stru_AstNode_t* parent, Stru_AstNode_t* child);
    bool (*set_node_attribute)(struct Stru_ParserInterface_t* interface, Stru_AstNode_t* node, const char* key, void* value);
    void* (*get_node_attribute)(struct Stru_ParserInterface_t* interface, Stru_AstNode_t* node, const char* key);
    
    // 错误处理
    void (*report_error)(struct Stru_ParserInterface_t* interface, size_t line, size_t column, const char* message, Stru_Token_t* token);
    bool (*has_errors)(struct Stru_ParserInterface_t* interface);
    Stru_DynamicArray_t* (*get_errors)(struct Stru_ParserInterface_t* interface);
    void (*clear_errors)(struct Stru_ParserInterface_t* interface);
    
    // 状态查询
    Stru_Token_t* (*get_current_token)(struct Stru_ParserInterface_t* interface);
    Stru_Token_t* (*get_previous_token)(struct Stru_ParserInterface_t* interface);
    Stru_Token_t* (*get_next_token)(struct Stru_ParserInterface_t* interface);
    
    // 资源管理
    void (*reset)(struct Stru_ParserInterface_t* interface);
    void (*destroy)(struct Stru_ParserInterface_t* interface);
    
    // 内部状态（不直接暴露）
    void* internal_state;
} Stru_ParserInterface_t;
```

#### 函数说明

##### 初始化函数

**`initialize`**
- **功能**：初始化语法分析器，设置词法分析器接口
- **参数**：
  - `interface`：语法分析器接口指针
  - `lexer_interface`：词法分析器接口指针
- **返回值**：`true`表示初始化成功，`false`表示失败
- **前置条件**：词法分析器接口已正确初始化
- **后置条件**：语法分析器准备好进行语法分析

##### 语法分析函数

**`parse_program`**
- **功能**：解析整个程序，构建完整的AST
- **参数**：`interface`：语法分析器接口指针
- **返回值**：程序AST根节点，`NULL`表示错误
- **支持的语法结构**：程序、模块、导入声明、全局声明等

**`parse_statement`**
- **功能**：解析单个语句
- **参数**：`interface`：语法分析器接口指针
- **返回值**：语句AST节点，`NULL`表示错误或结束
- **支持的语句类型**：赋值语句、控制流语句、表达式语句等

**`parse_expression`**
- **功能**：解析单个表达式
- **参数**：`interface`：语法分析器接口指针
- **返回值**：表达式AST节点，`NULL`表示错误
- **支持的表达式类型**：算术表达式、逻辑表达式、函数调用等

**`parse_declaration`**
- **功能**：解析声明
- **参数**：`interface`：语法分析器接口指针
- **返回值**：声明AST节点，`NULL`表示错误
- **支持的声明类型**：变量声明、函数声明、结构体声明、模块声明

##### AST构建函数

**`create_ast_node`**
- **功能**：创建指定类型的AST节点
- **参数**：
  - `interface`：语法分析器接口指针
  - `type`：AST节点类型
  - `token`：关联的令牌（可选）
- **返回值**：新创建的AST节点，`NULL`表示错误

**`add_child_node`**
- **功能**：向父AST节点添加子节点
- **参数**：
  - `interface`：语法分析器接口指针
  - `parent`：父节点
  - `child`：子节点
- **返回值**：`true`表示添加成功，`false`表示失败

**`set_node_attribute`**
- **功能**：设置AST节点的属性值
- **参数**：
  - `interface`：语法分析器接口指针
  - `node`：AST节点
  - `key`：属性键
  - `value`：属性值
- **返回值**：`true`表示设置成功，`false`表示失败

**`get_node_attribute`**
- **功能**：获取AST节点的属性值
- **参数**：
  - `interface`：语法分析器接口指针
  - `node`：AST节点
  - `key`：属性键
- **返回值**：属性值，`NULL`表示不存在

##### 错误处理函数

**`report_error`**
- **功能**：报告语法分析过程中发现的错误
- **参数**：
  - `interface`：语法分析器接口指针
  - `line`：错误行号
  - `column`：错误列号
  - `message`：错误信息
  - `token`：出错的令牌（可选）
- **返回值**：无

**`has_errors`**
- **功能**：检查语法分析过程中是否发生了错误
- **参数**：`interface`：语法分析器接口指针
- **返回值**：`true`表示有错误，`false`表示没有错误

**`get_errors`**
- **功能**：获取所有语法错误的列表
- **参数**：`interface`：语法分析器接口指针
- **返回值**：错误信息数组，`NULL`表示无错误

**`clear_errors`**
- **功能**：清除所有已记录的语法错误
- **参数**：`interface`：语法分析器接口指针
- **返回值**：无

##### 状态查询函数

**`get_current_token`**
- **功能**：获取语法分析器当前正在处理的令牌
- **参数**：`interface`：语法分析器接口指针
- **返回值**：当前令牌，`NULL`表示结束

**`get_previous_token`**
- **功能**：获取语法分析器前一个处理的令牌
- **参数**：`interface`：语法分析器接口指针
- **返回值**：前一个令牌，`NULL`表示不存在

**`get_next_token`**
- **功能**：获取语法分析器下一个将要处理的令牌
- **参数**：`interface`：语法分析器接口指针
- **返回值**：下一个令牌，`NULL`表示结束

##### 资源管理函数

**`reset`**
- **功能**：重置语法分析器到初始状态
- **参数**：`interface`：语法分析器接口指针
- **返回值**：无

**`destroy`**
- **功能**：释放语法分析器占用的所有资源
- **参数**：`interface`：语法分析器接口指针
- **返回值**：无

### 工厂函数

**`F_create_parser_interface`**
- **功能**：创建语法分析器接口实例
- **参数**：无
- **返回值**：新创建的语法分析器接口实例
- **内存管理**：调用者负责在不再使用时调用`destroy`函数

## 语法错误处理接口

### Stru_SyntaxErrorHandler_t

语法错误处理接口，定义了完整的错误报告、查询和管理功能。

#### 结构定义

```c
typedef struct Stru_SyntaxErrorHandler_t {
    // 错误报告
    Stru_SyntaxError_t* (*report_error)(struct Stru_SyntaxErrorHandler_t* handler,
                                       Eum_SyntaxErrorType type,
                                       Eum_SyntaxErrorSeverity severity,
                                       size_t line, size_t column,
                                       const char* message,
                                       Stru_Token_t* token);
    
    Stru_SyntaxError_t* (*report_unexpected_token)(struct Stru_SyntaxErrorHandler_t* handler,
                                                  Stru_Token_t* token,
                                                  const char* expected);
    
    Stru_SyntaxError_t* (*report_missing_token)(struct Stru_SyntaxErrorHandler_t* handler,
                                               size_t line, size_t column,
                                               const char* expected);
    
    // 错误查询
    bool (*has_errors)(struct Stru_SyntaxErrorHandler_t* handler);
    bool (*has_errors_of_severity)(struct Stru_SyntaxErrorHandler_t* handler,
                                  Eum_SyntaxErrorSeverity severity);
    size_t (*get_error_count)(struct Stru_SyntaxErrorHandler_t* handler);
    size_t (*get_error_count_by_severity)(struct Stru_SyntaxErrorHandler_t* handler,
                                         Eum_SyntaxErrorSeverity severity);
    Stru_DynamicArray_t* (*get_errors)(struct Stru_SyntaxErrorHandler_t* handler);
    Stru_DynamicArray_t* (*get_errors_by_severity)(struct Stru_SyntaxErrorHandler_t* handler,
                                                  Eum_SyntaxErrorSeverity severity);
    Stru_SyntaxError_t* (*get_last_error)(struct Stru_SyntaxErrorHandler_t* handler);
    
    // 错误管理
    void (*clear_errors)(struct Stru_SyntaxErrorHandler_t* handler);
    void (*clear_errors_by_severity)(struct Stru_SyntaxErrorHandler_t* handler,
                                    Eum_SyntaxErrorSeverity severity);
    void (*set_max_errors)(struct Stru_SyntaxErrorHandler_t* handler, size_t max_errors);
    size_t (*get_max_errors)(struct Stru_SyntaxErrorHandler_t* handler);
    
    // 错误格式化
    int (*format_error)(struct Stru_SyntaxErrorHandler_t* handler,
                       const Stru_SyntaxError_t* error,
                       char* buffer, size_t buffer_size);
    void (*print_all_errors)(struct Stru_SyntaxErrorHandler_t* handler);
    
    // 资源管理
    void (*destroy)(struct Stru_SyntaxErrorHandler_t* handler);
    
    // 内部状态
    void* internal_state;
} Stru_SyntaxErrorHandler_t;
```

### 错误类型枚举

#### Eum_SyntaxErrorSeverity

语法错误严重级别枚举：

```c
typedef enum Eum_SyntaxErrorSeverity {
    Eum_SEVERITY_INFO,        // 信息级别（非错误）
    Eum_SEVERITY_WARNING,     // 警告级别（可能有问题）
    Eum_SEVERITY_ERROR,       // 错误级别（语法错误）
    Eum_SEVERITY_FATAL        // 致命错误级别（无法恢复）
} Eum_SyntaxErrorSeverity;
```

#### Eum_SyntaxErrorType

语法错误类型枚举，包含以下类别：

1. **词法错误**：由词法分析器报告
   - `Eum_ERROR_UNKNOWN_TOKEN`：未知令牌
   - `Eum_ERROR_UNTERMINATED_STRING`：未终止的字符串
   - `Eum_ERROR_UNTERMINATED_COMMENT`：未终止的注释
   - `Eum_ERROR_INVALID_NUMBER`：无效的数字字面量
   - `Eum_ERROR_INVALID_ESCAPE`：无效的转义序列

2. **语法错误**：语法分析过程中发现
   - `Eum_ERROR_UNEXPECTED_TOKEN`：意外的令牌
   - `Eum_ERROR_MISSING_TOKEN`：缺失的令牌
   - `Eum_ERROR_MISMATCHED_TOKEN`：不匹配的令牌
   - `Eum_ERROR_INVALID_EXPRESSION`：无效的表达式
   - `Eum_ERROR_INVALID_STATEMENT`：无效的语句
   - `Eum_ERROR_INVALID_DECLARATION`：无效的声明

3. **结构错误**：程序结构错误
   - `Eum_ERROR_UNEXPECTED_EOF`：意外的文件结束
   - `Eum_ERROR_UNCLOSED_BLOCK`：未关闭的代码块
   - `Eum_ERROR_UNCLOSED_PAREN`：未关闭的括号
   - `Eum_ERROR_UNCLOSED_BRACKET`：未关闭的方括号
   - `Eum_ERROR_UNCLOSED_BRACE`：未关闭的大括号

4. **语义错误**：语法分析阶段可检测的语义错误
   - `Eum_ERROR_DUPLICATE_DECLARATION`：重复声明
   - `Eum_ERROR_UNDECLARED_IDENTIFIER`：未声明的标识符
   - `Eum_ERROR_TYPE_MISMATCH`：类型不匹配
   - `Eum_ERROR_ARGUMENT_COUNT`：参数数量错误

5. **其他错误**：系统级错误
   - `Eum_ERROR_INTERNAL`：内部错误
   - `Eum_ERROR_MEMORY`：内存错误
   - `Eum_ERROR_IO`：输入输出错误

### 错误对象操作函数

**`F_create_syntax_error`**
- **功能**：创建语法错误对象
- **参数**：错误类型、严重级别、位置、信息等
- **返回值**：新创建的语法错误对象

**`F_destroy_syntax_error`**
- **功能**：销毁语法错误对象
- **参数**：要销毁的语法错误对象
- **返回值**：无

**`F_copy_syntax_error`**
- **功能**：复制语法错误对象
- **参数**：要复制的语法错误对象
- **返回值**：新复制的语法错误对象

**`F_syntax_error_type_to_string`**
- **功能**：错误类型转字符串
- **参数**：错误类型
- **返回值**：类型字符串表示

**`F_syntax_error_severity_to_string`**
- **功能**：错误严重级别转字符串
- **参数**：错误严重级别
- **返回值**：严重级别字符串表示

### 工厂函数

**`F_create_syntax_error_handler`**
- **功能**：创建错误处理接口实例
- **参数**：无
- **返回值**：新创建的错误处理接口实例

## 使用示例

### 示例1：基本语法分析

```c
#include "src/core/parser/CN_parser_interface.h"
#include "src/core/lexer/CN_lexer_interface.h"

// 创建词法分析器和语法分析器
Stru_LexerInterface_t* lexer = F_create_lexer_interface();
Stru_ParserInterface_t* parser = F_create_parser_interface();

// 初始化词法分析器
lexer->initialize(lexer, "程序 主函数() { 输出(\"你好，世界！\"); }");

// 初始化语法分析器
if (parser->initialize(parser, lexer)) {
    // 解析程序
    Stru_AstNode_t* program_ast = parser->parse_program(parser);
    
    if (program_ast != NULL) {
        // 成功解析，处理AST
        printf("程序解析成功！\n");
        
        // 检查是否有错误
        if (parser->has_errors(parser)) {
            printf("解析过程中发现警告或错误\n");
        }
    } else {
        printf("程序解析失败\n");
    }
}

// 清理资源
parser->destroy(parser);
lexer->destroy(lexer);
```

### 示例2：错误处理

```c
#include "src/core/parser/CN_parser_interface.h"
#include "src/core/parser/CN_syntax_error.h"

// 创建语法分析器
Stru_ParserInterface_t* parser = F_create_parser_interface();

// 假设解析过程中发生错误
parser->report_error(parser, 10, 5, "缺少分号", NULL);

// 获取错误信息
if (parser->has_errors(parser)) {
    Stru_DynamicArray_t* errors = parser->get_errors(parser);
    
    if (errors != NULL) {
        size_t error_count = errors->size;
        printf("发现 %zu 个错误：\n", error_count);
        
        for (size_t i = 0; i < error_count; i++) {
            Stru_SyntaxError_t* error = (Stru_SyntaxError_t*)errors->get(errors, i);
            if (error != NULL) {
                printf("  第%zu行第%zu列：%s\n", 
                       error->line, error->column, error->message);
            }
        }
    }
}

// 清除错误并继续
parser->clear_errors(parser);

// 清理资源
parser->destroy(parser);
```

### 示例3：AST构建

```c
#include "src/core/parser/CN_parser_interface.h"
#include "src/core/ast/CN_ast.h"

// 创建语法分析器
Stru_ParserInterface_t* parser = F_create_parser_interface();

// 初始化语法分析器（需要先初始化词法分析器）
// 假设lexer已经初始化

// 创建AST节点
Stru_AstNode_t* variable_node = parser->create_ast_node(
    parser, 
    Eum_AST_VARIABLE_DECL, 
    NULL  // 没有关联的令牌
);

if (variable_node != NULL) {
    // 设置节点属性
    parser->set_node_attribute(parser, variable_node, "name", "计数器");
    parser->set_node_attribute(parser, variable_node, "type", "整数");
    
    // 创建表达式子节点
    Stru_AstNode_t* expr_node = parser->create_ast_node(
        parser, 
        Eum_AST_INTEGER_LITERAL, 
        NULL
    );
    
    if (expr_node != NULL) {
        parser->set_node_attribute(parser, expr_node, "value", "0");
        
        // 添加子节点
        if (parser->add_child_node(parser, variable_node, expr_node)) {
            printf("AST节点构建成功\n");
        }
    }
}

// 清理资源
parser->destroy(parser);
```

### 示例4：使用错误处理接口

```c
#include "src/core/parser/CN_syntax_error.h"

// 创建错误处理接口
Stru_SyntaxErrorHandler_t* error_handler = F_create_syntax_error_handler();

// 设置最大错误数量
error_handler->set_max_errors(error_handler, 100);

// 报告错误
Stru_SyntaxError_t* error = error_handler->report_error(
    error_handler,
    Eum_ERROR_UNEXPECTED_TOKEN,
    Eum_SEVERITY_ERROR,
    15, 8,
    "意外的令牌：期望分号，找到逗号",
    NULL  // 没有关联的令牌
);

// 检查错误
if (error_handler->has_errors(error_handler)) {
    size_t error_count = error_handler->get_error_count(error_handler);
    printf("发现 %zu 个错误\n", error_count);
    
    // 格式化并打印错误
    char error_buffer[256];
    error_handler->format_error(error_handler, error, error_buffer, sizeof(error_buffer));
    printf("错误信息：%s\n", error_buffer);
    
    // 打印所有错误
    error_handler->print_all_errors(error_handler);
}

// 清理资源
error_handler->destroy(error_handler);
```

## 配置选项

### 语法分析器配置

语法分析器支持以下配置选项：

1. **错误恢复模式**：
   - `RECOVERY_NONE`：不进行错误恢复，遇到错误立即停止
   - `RECOVERY_MINIMAL`：最小错误恢复，跳过当前语句
   - `RECOVERY_AGGRESSIVE`：积极错误恢复，尝试继续解析

2. **AST构建选项**：
   - 是否保留注释节点
   - 是否生成位置信息
   - 是否进行语法糖展开

3. **性能优化**：
   - 预读取令牌数量
   - 缓存大小
   - 并行解析选项

### 错误处理配置

1. **错误报告级别**：
   - 可以设置只报告特定严重级别的错误
   - 支持错误过滤规则

2. **错误格式化**：
   - 支持自定义错误信息格式
   - 支持多语言错误信息

3. **错误限制**：
   - 最大错误数量限制
   - 错误去重选项

## 性能考虑

### 内存使用

1. **AST内存管理**：
   - AST节点使用对象池进行分配
   - 支持增量垃圾回收
   - 内存使用统计和监控

2. **令牌缓存**：
   - 预读取令牌缓存优化
   - 令牌重用机制

### 解析性能

1. **解析算法**：
   - 使用递归下降解析算法
   - 支持LL(k)预测解析
   - 错误恢复性能优化

2. **并发支持**：
   - 支持多文件并行解析
   - 线程安全的接口设计

## 错误处理策略

### 错误恢复机制

语法分析器实现了多种错误恢复策略：

1. **恐慌模式恢复**：
   - 跳过错误令牌直到同步点
   - 同步点：分号、右大括号、行结束等

2. **短语级恢复**：
   - 局部错误修正
   - 令牌插入/删除/替换

3. **错误产生式**：
   - 为常见错误添加特殊产生式
   - 生成有意义的错误信息

### 错误报告格式

错误报告支持多种格式：

1. **控制台格式**：
   ```
   错误：第15行第8列
   意外的令牌：期望分号，找到逗号
   源代码：变量 计数器 = 10, 输出(计数器)
                              ^
   ```

2. **IDE集成格式**：
   - 支持LSP协议错误报告
   - 支持快速修复建议

3. **日志格式**：
   - 结构化错误日志
   - 支持错误统计和分析

## 扩展性

### 自定义语法扩展

语法分析器支持通过以下方式扩展：

1. **插件接口**：
   - 自定义语法规则插件
   - 第三方语言扩展支持

2. **配置驱动**：
   - 通过配置文件定义语法规则
   - 运行时语法加载

### 接口实现替换

支持替换以下组件：

1. **解析算法实现**：
   - 可以替换为不同的解析算法
   - 支持实验性解析策略

2. **错误处理实现**：
   - 自定义错误处理逻辑
   - 特定领域的错误恢复策略

## 测试指南

### 单元测试

语法分析器模块包含完整的单元测试：

1. **接口测试**：
   - 测试所有接口函数
   - 边界条件测试

2. **功能测试**：
   - 语法分析正确性测试
   - 错误处理测试

3. **性能测试**：
   - 解析性能基准测试
   - 内存使用测试

### 集成测试

1. **与词法分析器集成**：
   - 端到端解析测试
   - 错误传播测试

2. **与AST模块集成**：
   - AST构建正确性测试
   - 节点操作测试

## 版本历史

### 版本 1.0.0 (2026-01-06)
- 初始版本发布
- 完整的语法分析器接口定义
- 基本的AST构建功能
- 语法错误处理接口
- 支持CN语言基本语法结构

### 版本 1.1.0 (计划中)
- 性能优化
- 扩展错误恢复机制
- 支持更多语法结构
- 改进的API文档

## 相关文档

1. [语法规范](../specifications/CN_Language%20语法规范.md)
2. [AST模块API文档](../ast/CN_ast_api.md)
3. [词法分析器API文档](../lexer/CN_lexer_api.md)
4. [项目架构文档](../../architecture/001-中文编程语言CN_Language开发规划.md)

## 技术支持

如有问题或需要技术支持，请联系：
- 项目维护者：CN_Language架构委员会
- 问题跟踪：项目GitHub仓库
- 文档更新：docs/api/core/parser/目录

---
*文档最后更新：2026-01-06*
*版本：1.0.0*
