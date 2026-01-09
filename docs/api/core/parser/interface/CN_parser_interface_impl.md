# CN_Language 语法分析器接口实现模块 API 文档

## 概述

接口实现模块是CN_Language语法分析器的核心组件，负责实现语法分析器抽象接口的所有功能。本模块遵循项目架构规范，采用接口模式设计，实现模块解耦和可扩展性。

### 主要功能

1. **接口函数实现** - 实现所有语法分析器接口函数
2. **状态管理** - 管理语法分析器的内部状态
3. **委托分发** - 将解析任务委托给相应的子模块
4. **资源管理** - 管理语法分析器的生命周期和资源

### 架构位置

- **所属模块**：语法分析器模块 (`src/core/parser/`)
- **子模块类型**：接口实现子模块
- **依赖模块**：所有其他parser子模块、词法分析器接口、AST模块
- **被依赖模块**：语法分析器主接口

## 核心函数

### 初始化函数

#### `F_parser_initialize()`

**功能**：初始化语法分析器，设置词法分析器接口和内部状态。

**函数原型**：
```c
bool F_parser_initialize(Stru_ParserInterface_t* interface, Stru_LexerInterface_t* lexer_interface);
```

**参数**：
- `interface`：语法分析器接口指针
- `lexer_interface`：词法分析器接口指针

**返回值**：
- `true`：初始化成功
- `false`：初始化失败

**前置条件**：
- 词法分析器接口已正确初始化
- 语法分析器接口指针有效

**后置条件**：
- 语法分析器内部状态初始化完成
- 准备好进行语法分析

**实现细节**：
1. 分配内部状态结构体
2. 设置词法分析器接口
3. 初始化错误处理接口
4. 设置默认配置选项
5. 初始化令牌缓冲区

### 程序解析函数

#### `F_parser_parse_program()`

**功能**：解析整个程序，构建完整的AST。

**函数原型**：
```c
Stru_AstNode_t* F_parser_parse_program(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 程序AST根节点
- `NULL`：解析失败或遇到致命错误

**支持的语法结构**：
- 程序声明
- 模块声明
- 导入声明
- 全局声明序列

**实现细节**：
1. 创建程序AST节点
2. 解析程序头部（程序名、版本等）
3. 循环解析全局声明
4. 检查程序结束标记
5. 返回程序AST根节点

### 语句解析委托函数

#### `F_parser_parse_statement()`

**功能**：解析单个语句，委托给语句解析模块。

**函数原型**：
```c
Stru_AstNode_t* F_parser_parse_statement(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 语句AST节点
- `NULL`：解析失败或语句结束

**实现细节**：
1. 获取当前令牌类型
2. 根据令牌类型调用相应的语句解析函数
3. 处理语句结束标记（分号）
4. 返回语句AST节点

### 表达式解析委托函数

#### `F_parser_parse_expression()`

**功能**：解析单个表达式，委托给表达式解析模块。

**函数原型**：
```c
Stru_AstNode_t* F_parser_parse_expression(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 表达式AST节点
- `NULL`：解析失败

**实现细节**：
1. 调用表达式解析模块的入口函数
2. 处理表达式优先级和结合性
3. 返回表达式AST节点

### 声明解析委托函数

#### `F_parser_parse_declaration()`

**功能**：解析声明，委托给声明解析模块。

**函数原型**：
```c
Stru_AstNode_t* F_parser_parse_declaration(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 声明AST节点
- `NULL`：解析失败

**实现细节**：
1. 获取当前令牌类型
2. 根据令牌类型调用相应的声明解析函数
3. 处理声明结束标记
4. 返回声明AST节点

## 内部辅助函数

### 状态管理函数

#### `F_get_parser_state()`

**功能**：获取语法分析器的内部状态。

**函数原型**：
```c
Stru_ParserState_t* F_get_parser_state(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- 语法分析器内部状态指针
- `NULL`：接口无效

#### `F_advance_token()`

**功能**：前进到下一个令牌。

**函数原型**：
```c
bool F_advance_token(Stru_ParserInterface_t* interface);
```

**参数**：
- `interface`：语法分析器接口指针

**返回值**：
- `true`：成功前进
- `false`：到达文件结尾或错误

#### `F_check_token_type()`

**功能**：检查当前令牌类型。

**函数原型**：
```c
bool F_check_token_type(Stru_ParserInterface_t* interface, Eum_TokenType expected_type);
```

**参数**：
- `interface`：语法分析器接口指针
- `expected_type`：期望的令牌类型

**返回值**：
- `true`：当前令牌类型匹配期望类型
- `false`：类型不匹配

#### `F_match_token_type()`

**功能**：匹配令牌类型，如果匹配则前进。

**函数原型**：
```c
bool F_match_token_type(Stru_ParserInterface_t* interface, Eum_TokenType expected_type);
```

**参数**：
- `interface`：语法分析器接口指针
- `expected_type`：期望的令牌类型

**返回值**：
- `true`：匹配成功并前进
- `false`：类型不匹配

#### `F_consume_token()`

**功能**：消耗当前令牌，报告缺失令牌错误。

**函数原型**：
```c
bool F_consume_token(Stru_ParserInterface_t* interface, Eum_TokenType expected_type, const char* error_message);
```

**参数**：
- `interface`：语法分析器接口指针
- `expected_type`：期望的令牌类型
- `error_message`：错误信息（如果令牌缺失）

**返回值**：
- `true`：成功消耗令牌
- `false`：令牌不匹配

## 内部状态结构

### Stru_ParserState_t

语法分析器内部状态结构体：

```c
typedef struct Stru_ParserState_t {
    // 词法分析器接口
    Stru_LexerInterface_t* lexer_interface;
    
    // 错误处理接口
    Stru_SyntaxErrorHandler_t* error_handler;
    
    // 当前令牌信息
    Stru_Token_t* current_token;
    Stru_Token_t* previous_token;
    Stru_Token_t* next_token;
    
    // 解析状态
    size_t current_line;
    size_t current_column;
    bool is_at_end;
    
    // 配置选项
    size_t max_errors;
    Eum_ParserRecoveryMode recovery_mode;
    
    // 令牌缓冲区
    Stru_DynamicArray_t* token_buffer;
    size_t buffer_position;
    
    // AST节点栈（用于嵌套结构）
    Stru_Stack_t* ast_node_stack;
    
    // 作用域栈
    Stru_Stack_t* scope_stack;
    
    // 资源引用（用于内存管理）
    void** allocated_resources;
    size_t resource_count;
} Stru_ParserState_t;
```

## 使用示例

### 示例1：使用接口实现函数

```c
#include "src/core/parser/interface/CN_parser_interface_impl.h"

// 创建语法分析器接口
Stru_ParserInterface_t* parser = F_create_parser_interface();

// 直接使用接口实现函数（不推荐，应通过接口调用）
Stru_ParserState_t* state = F_get_parser_state(parser);
if (state != NULL) {
    // 访问内部状态
    printf("当前行号: %zu\n", state->current_line);
    printf("当前列号: %zu\n", state->current_column);
}

// 清理资源
parser->destroy(parser);
```

### 示例2：自定义解析逻辑

```c
// 自定义语句解析逻辑
Stru_AstNode_t* custom_parse_statement(Stru_ParserInterface_t* interface) {
    // 获取内部状态
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL) {
        return NULL;
    }
    
    // 检查当前令牌
    if (F_check_token_type(interface, Eum_TOKEN_KEYWORD_IF)) {
        // 调用内置的条件语句解析
        return F_parser_parse_statement(interface);
    } else {
        // 自定义解析逻辑
        // ...
    }
    
    return NULL;
}
```

## 配置选项

### 错误恢复模式

接口实现模块支持以下错误恢复模式：

1. **RECOVERY_NONE**：不进行错误恢复，遇到错误立即停止
2. **RECOVERY_MINIMAL**：最小错误恢复，跳过当前语句
3. **RECOVERY_AGGRESSIVE**：积极错误恢复，尝试继续解析

### 最大错误数量

默认最大错误数量为100，超过此数量后停止解析。

## 性能考虑

### 令牌缓冲区

- 预读取令牌到缓冲区，减少词法分析器调用
- 缓冲区大小可配置，默认大小为10个令牌
- 支持令牌回退（peek）操作

### 状态缓存

- 缓存当前行号和列号，避免重复计算
- 维护令牌历史，支持错误上下文生成

## 扩展性

### 自定义状态管理

可以通过以下方式扩展状态管理：

1. **添加自定义状态字段**：在`Stru_ParserState_t`中添加新字段
2. **自定义初始化逻辑**：重写初始化函数
3. **自定义清理逻辑**：重写销毁函数

### 插件支持

支持通过插件接口添加自定义解析逻辑：

1. **注册自定义解析函数**
2. **替换默认解析实现**
3. **添加新的语法结构支持**

## 测试指南

### 单元测试

接口实现模块包含以下单元测试：

1. **初始化测试**：测试语法分析器初始化
2. **状态管理测试**：测试内部状态操作
3. **委托函数测试**：测试解析委托逻辑
4. **错误处理测试**：测试错误恢复机制

### 集成测试

1. **与词法分析器集成**：测试令牌流处理
2. **与子模块集成**：测试委托分发逻辑
3. **端到端测试**：测试完整解析流程

## 相关文档

1. [语法分析器主API文档](../CN_parser_api.md)
2. [表达式解析模块API文档](../expression_parsers/CN_parser_expressions.md)
3. [语句解析模块API文档](../statement_parsers/CN_parser_statements.md)
4. [声明解析模块API文档](../declaration_parsers/CN_parser_declarations.md)
5. [错误处理模块API文档](../error_handling/CN_parser_errors.md)
6. [工具函数模块API文档](../utils/CN_parser_utils.md)

---
*文档最后更新：2026-01-08*
*版本：1.0.0*
