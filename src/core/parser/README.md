# CN_Language 语法分析器模块

## 概述

语法分析器模块是CN_Language编译器的核心组件之一，负责将词法分析器生成的令牌序列转换为抽象语法树（AST）。本模块遵循项目架构规范，采用接口模式设计，实现模块解耦和可扩展性。

## 模块结构

### 主要文件

1. **CN_parser_interface.h** - 语法分析器抽象接口定义
2. **CN_parser_interface.c** - 语法分析器接口实现
3. **CN_syntax_error.h** - 语法错误处理接口定义
4. **CN_ast.h** - 抽象语法树定义（位于`../ast/`目录）

### 依赖关系

- 词法分析器模块 (`../lexer/`)
- 令牌模块 (`../token/`)
- 抽象语法树模块 (`../ast/`)
- 基础设施层容器 (`../../infrastructure/containers/`)

## 接口设计

### Stru_ParserInterface_t

语法分析器的主要接口，包含以下功能：

#### 初始化函数
- `initialize()` - 初始化语法分析器，设置词法分析器接口

#### 核心功能
- `parse_program()` - 解析整个程序，构建完整的AST
- `parse_statement()` - 解析单个语句
- `parse_expression()` - 解析单个表达式
- `parse_declaration()` - 解析声明（变量、函数、结构体等）

#### AST构建功能
- `create_ast_node()` - 创建AST节点
- `add_child_node()` - 向父节点添加子节点
- `set_node_attribute()` - 设置节点属性
- `get_node_attribute()` - 获取节点属性

#### 错误处理
- `report_error()` - 报告语法错误
- `has_errors()` - 检查是否有错误
- `get_errors()` - 获取错误列表
- `clear_errors()` - 清除错误

#### 状态查询
- `get_current_token()` - 获取当前令牌
- `get_previous_token()` - 获取前一个令牌
- `get_next_token()` - 获取下一个令牌

#### 资源管理
- `reset()` - 重置语法分析器
- `destroy()` - 销毁语法分析器

### Stru_SyntaxErrorHandler_t

语法错误处理接口，专门处理语法分析过程中的错误：

#### 错误报告
- `report_error()` - 报告语法错误
- `report_unexpected_token()` - 报告意外令牌错误
- `report_missing_token()` - 报告缺失令牌错误

#### 错误查询
- `has_errors()` - 检查是否有错误
- `get_error_count()` - 获取错误数量
- `get_errors()` - 获取错误列表
- `get_last_error()` - 获取最后一个错误

#### 错误管理
- `clear_errors()` - 清除所有错误
- `set_max_errors()` - 设置最大错误数量

## 使用示例

### 基本用法

```c
#include "CN_parser_interface.h"
#include "CN_lexer_interface.h"

// 创建词法分析器
Stru_LexerInterface_t* lexer = F_create_lexer_interface();
lexer->initialize(lexer, source_code, strlen(source_code), "example.cn");

// 创建语法分析器
Stru_ParserInterface_t* parser = F_create_parser_interface();
parser->initialize(parser, lexer);

// 解析程序
Stru_AstNode_t* ast = parser->parse_program(parser);

// 检查错误
if (parser->has_errors(parser)) {
    Stru_DynamicArray_t* errors = parser->get_errors(parser);
    // 处理错误...
}

// 清理资源
parser->destroy(parser);
lexer->destroy(lexer);
```

### 错误处理

```c
// 创建错误处理接口
Stru_SyntaxErrorHandler_t* error_handler = F_create_syntax_error_handler();

// 报告错误
error_handler->report_unexpected_token(error_handler, token, "期望分号");

// 获取错误信息
if (error_handler->has_errors(error_handler)) {
    error_handler->print_all_errors(error_handler);
}

// 清理资源
error_handler->destroy(error_handler);
```

## 支持的语法结构

语法分析器支持CN_Language的所有语法结构，包括：

### 声明
- 变量声明：`变量 名称 = 值;`
- 函数声明：`函数 名称(参数) { ... }`
- 结构体声明：`结构体 名称 { ... }`
- 枚举声明：`枚举 名称 { ... }`
- 模块声明：`模块 名称 { ... }`

### 语句
- 表达式语句：`表达式;`
- 条件语句：`如果 (条件) { ... }`
- 循环语句：`当 (条件) { ... }`, `循环 (初始化; 条件; 更新) { ... }`
- 控制语句：`中断;`, `继续;`, `返回 值;`
- 代码块：`{ ... }`

### 表达式
- 字面量表达式：`42`, `3.14`, `"字符串"`, `真`, `假`
- 标识符表达式：`变量名`
- 二元表达式：`a + b`, `a * b`, `a == b`
- 一元表达式：`-a`, `非 a`
- 函数调用表达式：`函数名(参数)`
- 赋值表达式：`a = 值`

## 错误类型

语法分析器可以检测和报告多种错误类型：

### 词法错误（由词法分析器报告）
- 未知令牌
- 未终止的字符串
- 未终止的注释
- 无效的数字字面量

### 语法错误
- 意外的令牌
- 缺失的令牌
- 不匹配的令牌
- 无效的表达式
- 无效的语句
- 无效的声明

### 结构错误
- 意外的文件结束
- 未关闭的代码块
- 未关闭的括号
- 未关闭的方括号
- 未关闭的大括号

## 配置选项

### 最大错误数量
默认情况下，语法分析器在报告100个错误后停止解析。可以通过以下方式修改：

```c
// 通过错误处理接口设置
error_handler->set_max_errors(error_handler, 50);

// 或者在解析器初始化后设置
Stru_ParserState_t* state = (Stru_ParserState_t*)parser->internal_state;
state->max_errors = 50;
```

## 性能考虑

### 内存管理
- 语法分析器使用动态内存分配创建AST节点
- 调用者负责销毁AST树和语法分析器接口
- 错误对象由错误处理接口管理

### 错误恢复
- 语法分析器实现基本的错误恢复机制
- 在遇到错误时尝试跳过当前结构继续解析
- 支持设置最大错误数量限制

## 测试

语法分析器模块包含完整的单元测试和集成测试：

### 单元测试
- 测试单个语法结构的解析
- 测试错误检测和报告
- 测试AST构建功能

### 集成测试
- 测试与词法分析器的集成
- 测试完整程序的解析
- 测试错误恢复机制

## 扩展性

语法分析器设计为可扩展的：

### 添加新的语法结构
1. 在`CN_ast.h`中添加新的AST节点类型
2. 在`CN_parser_interface.c`中添加对应的解析函数
3. 更新接口函数实现

### 自定义错误处理
1. 实现自定义的错误处理接口
2. 在语法分析器初始化时设置自定义错误处理器
3. 实现特定的错误报告逻辑

## 版本历史

### 1.0.0 (2026-01-06)
- 初始版本
- 实现基本语法分析功能
- 支持变量声明、表达式语句等基本结构
- 实现语法错误处理接口

## 维护者

CN_Language架构委员会

## 许可证

MIT许可证
