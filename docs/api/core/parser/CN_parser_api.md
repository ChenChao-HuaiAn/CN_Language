# CN_Language 语法分析器模块 API 文档

## 概述

语法分析器模块是CN_Language编译器的核心组件之一，负责将词法分析器生成的令牌序列转换为抽象语法树（AST）。本模块采用模块化架构设计，遵循单一职责原则，实现了高度解耦的组件结构。每个子模块专注于特定的语法分析功能，通过清晰的接口进行通信。

### 主要功能

1. **语法分析**：解析CN语言程序，构建完整的抽象语法树
2. **AST构建**：创建和管理AST节点，支持复杂的程序结构表示
3. **错误处理**：检测和报告语法错误，支持错误恢复机制
4. **模块化设计**：支持可扩展的子模块架构

### 架构位置

- **所属层级**：核心层（Core Layer）
- **依赖模块**：基础设施层（内存管理、容器）、词法分析器、AST模块
- **被依赖模块**：语义分析器、代码生成器、应用层工具

### 模块化结构

语法分析器模块采用分层子模块设计：

```
src/core/parser/
├── CN_parser_interface.h          # 主接口定义
├── CN_parser_interface.c          # 工厂函数
├── CN_syntax_error.h              # 语法错误接口
├── README.md                      # 模块概述
├── interface/                     # 接口实现子模块
│   ├── CN_parser_interface_impl.h
│   ├── CN_parser_interface_impl.c
│   └── README.md
├── expression_parsers/            # 表达式解析子模块
│   ├── CN_parser_expressions.h
│   ├── CN_parser_expressions_main.c
│   ├── CN_parser_unary_expressions.c
│   ├── CN_parser_primary_expressions.c
│   ├── CN_parser_function_calls.c
│   └── README.md
├── statement_parsers/             # 语句解析子模块
│   ├── CN_parser_statements.h
│   ├── CN_parser_statements_main.c
│   ├── CN_parser_control_statements.c
│   ├── CN_parser_loop_statements.c
│   ├── CN_parser_conditional_statements.c
│   ├── CN_parser_block_statements.c
│   └── README.md
├── declaration_parsers/           # 声明解析子模块
│   ├── CN_parser_declarations.h
│   ├── CN_parser_declarations_main.c
│   ├── CN_parser_variable_declarations.c
│   ├── CN_parser_function_declarations.c
│   ├── CN_parser_struct_declarations.c
│   ├── CN_parser_enum_declarations.c
│   ├── CN_parser_module_declarations.c
│   ├── CN_parser_type_declarations.c
│   ├── CN_parser_parameter_lists.c
│   ├── CN_parser_type_expressions.c
│   ├── CN_parser_member_lists.c
│   └── README.md
├── error_handling/                # 错误处理子模块
│   ├── CN_parser_errors.h
│   ├── CN_parser_errors.c
│   └── README.md
└── utils/                         # 工具函数子模块
    ├── CN_parser_utils.h
    ├── CN_parser_utils.c
    └── README.md
```

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
- **实现**：委托给`interface`子模块的`F_parser_parse_program()`函数

**`parse_statement`**
- **功能**：解析单个语句
- **参数**：`interface`：语法分析器接口指针
- **返回值**：语句AST节点，`NULL`表示错误或结束
- **实现**：委托给`statement_parsers`子模块的`F_parse_statement()`函数

**`parse_expression`**
- **功能**：解析单个表达式
- **参数**：`interface`：语法分析器接口指针
- **返回值**：表达式AST节点，`NULL`表示错误
- **实现**：委托给`expression_parsers`子模块的`F_parse_expression()`函数

**`parse_declaration`**
- **功能**：解析声明
- **参数**：`interface`：语法分析器接口指针
- **返回值**：声明AST节点，`NULL`表示错误
- **实现**：委托给`declaration_parsers`子模块的`F_parse_declaration()`函数

##### AST构建函数

**`create_ast_node`**
- **功能**：创建指定类型的AST节点
- **参数**：
  - `interface`：语法分析器接口指针
  - `type`：AST节点类型
  - `token`：关联的令牌（可选）
- **返回值**：新创建的AST节点，`NULL`表示错误
- **实现**：使用`utils`子模块的AST工具函数

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
- **实现**：委托给`error_handling`子模块的`F_report_parser_error()`函数

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
- **实现位置**：`src/core/parser/CN_parser_interface.c`

## 子模块API

### 接口实现子模块 (`interface/`)

负责实现语法分析器抽象接口的所有功能。

#### 主要函数

- `F_parser_initialize()` - 初始化语法分析器
- `F_parser_parse_program()` - 解析整个程序
- `F_parser_parse_statement()` - 解析语句（委托给语句解析模块）
- `F_parser_parse_expression()` - 解析表达式（委托给表达式解析模块）
- `F_parser_parse_declaration()` - 解析声明（委托给声明解析模块）

### 表达式解析子模块 (`expression_parsers/`)

负责解析各种类型的表达式。

#### 主要函数

- `F_parse_expression()` - 解析表达式（入口函数）
- `F_parse_assignment_expression()` - 解析赋值表达式
- `F_parse_logical_or_expression()` - 解析逻辑或表达式
- `F_parse_logical_and_expression()` - 解析逻辑与表达式
- `F_parse_equality_expression()` - 解析相等性表达式
- `F_parse_relational_expression()` - 解析关系表达式
- `F_parse_additive_expression()` - 解析加法表达式
- `F_parse_multiplicative_expression()` - 解析乘法表达式
- `F_parse_unary_expression()` - 解析一元表达式
- `F_parse_primary_expression()` - 解析基本表达式

### 语句解析子模块 (`statement_parsers/`)

负责解析各种类型的语句。

#### 主要函数

- `F_parse_statement()` - 解析语句（入口函数）
- `F_parse_expression_statement()` - 解析表达式语句
- `F_parse_block_statement()` - 解析代码块语句
- `F_parse_if_statement()` - 解析条件语句
- `F_parse_while_statement()` - 解析循环语句
- `F_parse_for_statement()` - 解析for循环语句
- `F_parse_break_statement()` - 解析中断语句
- `F_parse_continue_statement()` - 解析继续语句
- `F_parse_return_statement()` - 解析返回语句

### 声明解析子模块 (`declaration_parsers/`)

负责解析各种类型的声明。

#### 主要函数

- `F_parse_declaration()` - 解析声明（入口函数）
- `F_parse_variable_declaration()` - 解析变量声明
- `F_parse_function_declaration()` - 解析函数声明
- `F_parse_struct_declaration()` - 解析结构体声明
- `F_parse_enum_declaration()` - 解析枚举声明
- `F_parse_module_declaration()` - 解析模块声明
- `F_parse_type_declaration()` - 解析类型声明

### 错误处理子模块 (`error_handling/`)

负责处理语法分析过程中的错误。

#### 主要函数

- `F_report_parser_error()` - 报告语法分析错误
- `F_report_unexpected_token_error()` - 报告意外令牌错误
- `F_report_missing_token_error()` - 报告缺失令牌错误
- `F_report_type_mismatch_error()` - 报告类型不匹配错误
- `F_report_duplicate_declaration_error()` - 报告重复声明错误
- `F_try_error_recovery()` - 尝试错误恢复
- `F_synchronize_to_safe_point()` - 同步到安全点

### 工具函数子模块 (`utils/`)

提供各种辅助工具函数。

#### 主要函数

- `F_is_keyword_token()` - 检查令牌是否为关键字
- `F_is_operator_token()` - 检查令牌是否为运算符
- `F_get_operator_precedence()` - 获取运算符优先级
- `F_create_binary_expression_node()` - 创建二元表达式节点
- `F_create_unary_expression_node()` - 创建一元表达式节点
- `F_create_literal_node()` - 创建字面量节点
- `F_check_type_compatibility()` - 检查类型兼容性
- `F_is_numeric_type()` - 检查是否为数值类型

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

### 示例2：使用子模块函数

```c
#include "src/core/parser/expression_parsers/CN_parser_expressions.h"
#include "src/core/parser/statement_parsers/CN_parser_statements.h"

// 直接使用表达式解析函数
Stru_ParserInterface_t* parser = F_create_parser_interface();
parser->initialize(parser, lexer);

// 解析表达式
Stru_AstNode_t* expr = F_parse_expression(parser);
if (expr != NULL) {
    printf("表达式解析成功\n");
}

// 解析语句
Stru_AstNode_t* stmt = F_parse_statement(parser);
if (stmt != NULL) {
    printf("语句解析成功\n");
}

// 清理资源
parser->destroy(parser);
```

### 示例3：错误处理

```c
#include "src/core/parser/error_handling/CN_parser_errors.h"

// 获取语法分析器接口
Stru_ParserInterface_t* parser = F_create_parser_interface();
parser->initialize(parser, lexer);

// 报告意外令牌错误
Stru_Token_t* token = parser->get_current_token(parser);
Stru_SyntaxError_t* error = F_report_unexpected_token_error(parser, token, "期望分号");

// 尝试错误恢复
if (error != NULL && error->severity == Eum_SYNTAX_ERROR_SEVERITY_ERROR) {
    bool recovered = F_try_error_recovery(parser, error);
    if (!recovered) {
        // 无法恢复，停止分析
        return NULL;
    }
}

// 清理资源
parser->destroy(parser);
```

### 示例4：使用工具函数

```c
#include "src/core/parser/utils/CN_parser_utils.h"

// 检查运算符优先级
Eum_TokenType token_type = Eum_TOKEN_PLUS;
int precedence = F_get_operator_precedence(token_type);
printf("运算符'+'的优先级: %d\n", precedence);

// 检查令牌类型
if (F_is_keyword_token(Eum_TOKEN_KEYWORD_VAR)) {
    printf("这是变量关键字\n");
}

// 创建AST节点
Stru_Token_t* literal_token = parser->get_current_token(parser);
Stru_AstNode_t* literal_node = F_create_literal_node(parser, literal_token);
if (literal_node != NULL) {
    printf("字面量节点创建成功\n");
}
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

### 版本 2.2.0 (2026-01-09)
- **语句解析模块化**：将语句解析拆分为多个专门的文件
- **文件结构优化**：
  - `CN_parser_statements_main.c`：主语句解析函数和语句类型分发
  - `CN_parser_control_statements.c`：控制语句解析（中断、继续、返回、空语句）
  - `CN_parser_loop_statements.c`：循环语句解析（while、for）
  - `CN_parser_conditional_statements.c`：条件语句解析（if）
  - `CN_parser_block_statements.c`：代码块和表达式语句解析
- **单一职责原则**：每个文件专注于特定语句类型
- **文件大小合规**：所有文件不超过500行限制

### 版本 2.1.0 (2026-01-09)
- **表达式解析模块化**：将表达式解析拆分为多个专门的文件
- **文件结构优化**：
  - `CN_parser_expressions_main.c`：主解析函数和运算符优先级链
  - `CN_parser_unary_expressions.c`：一元表达式解析
  - `CN_parser_primary_expressions.c`：基本表达式解析
  - `CN_parser_function_calls.c`：函数调用解析
- **单一职责原则**：每个文件专注于特定表达式类型
- **文件大小合规**：所有文件不超过500行限制

### 版本 2.0.0 (2026-01-08)
- **模块化重构**：将语法分析器拆分为多个子模块
- **单一职责原则**：每个子模块专注于特定功能
- **文件大小限制**：每个.c文件不超过500行
- **函数大小限制**：每个函数不超过50行
- **完整文档**：为每个子模块创建README文档
- **测试框架**：模块化测试结构

### 版本 1.1.0 (计划中)
- 性能优化
- 扩展错误恢复机制
- 支持更多语法结构
- 改进的API文档

## 相关文档

1. [语法规范](../../specifications/CN_Language%20语法规范.md)
2. [AST模块API文档](../ast/CN_ast_api.md)
3. [词法分析器API文档](../lexer/CN_lexer_interface.md)
4. [项目架构文档](../../architecture/001-中文编程语言CN_Language开发规划.md)
5. [子模块文档](../parser/)：
   - [接口实现子模块](../parser/interface/README.md)
   - [表达式解析子模块](../parser/expression_parsers/README.md)
   - [语句解析子模块](../parser/statement_parsers/README.md)
   - [声明解析子模块](../parser/declaration_parsers/README.md)
   - [错误处理子模块](../parser/error_handling/README.md)
   - [工具函数子模块](../parser/utils/README.md)

## 技术支持

如有问题或需要技术支持，请联系：
- 项目维护者：CN_Language架构委员会
- 问题跟踪：项目GitHub仓库
- 文档更新：docs/api/core/parser/目录

---
*文档最后更新：2026-01-09*
*版本：2.2.0*
