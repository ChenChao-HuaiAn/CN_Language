# CN_Language 语法分析器接口实现模块

## 概述

语法分析器接口实现模块是CN_Language语法分析器的核心组件，负责实现语法分析器抽象接口的所有功能。本模块遵循项目架构规范，采用接口模式设计，实现模块解耦和可扩展性。

## 模块功能

### 核心功能
1. **语法分析器初始化** - 初始化语法分析器，设置词法分析器接口
2. **程序解析** - 解析整个程序，构建完整的AST
3. **接口函数实现** - 实现所有语法分析器接口函数

### 内部状态管理
- 管理语法分析器的内部状态
- 维护当前令牌、前一个令牌和下一个令牌
- 管理错误处理接口
- 维护AST节点栈

### 辅助功能
- 令牌前进和匹配
- 令牌类型检查
- 错误报告和恢复

## 文件结构

### 主要文件
1. **CN_parser_interface_impl.h** - 接口实现头文件
   - 定义内部状态结构体
   - 声明接口实现函数
   - 声明内部辅助函数

2. **CN_parser_interface_impl.c** - 接口实现源文件
   - 实现所有接口函数
   - 实现内部辅助函数
   - 管理语法分析器状态

### 依赖关系
- 语法分析器接口 (`../CN_parser_interface.h`)
- 语法错误处理 (`../CN_syntax_error.h`)
- 词法分析器接口 (`../../lexer/CN_lexer_interface.h`)
- 令牌模块 (`../../token/CN_token_types.h`)
- AST模块 (`../../ast/CN_ast.h`)
- 基础设施层容器 (`../../../infrastructure/containers/array/CN_dynamic_array.h`)

## 接口函数实现

### 初始化函数
- `F_parser_initialize()` - 初始化语法分析器
- `F_parser_reset()` - 重置语法分析器
- `F_parser_destroy()` - 销毁语法分析器

### 核心解析函数
- `F_parser_parse_program()` - 解析整个程序
- `F_parser_parse_statement()` - 解析语句（委托给语句解析模块）
- `F_parser_parse_expression()` - 解析表达式（委托给表达式解析模块）
- `F_parser_parse_declaration()` - 解析声明（委托给声明解析模块）

### AST构建函数
- `F_parser_create_ast_node()` - 创建AST节点
- `F_parser_add_child_node()` - 添加子节点
- `F_parser_set_node_attribute()` - 设置节点属性
- `F_parser_get_node_attribute()` - 获取节点属性

### 错误处理函数
- `F_parser_report_error()` - 报告语法错误
- `F_parser_has_errors()` - 检查是否有错误
- `F_parser_get_errors()` - 获取错误列表
- `F_parser_clear_errors()` - 清除错误

### 状态查询函数
- `F_parser_get_current_token()` - 获取当前令牌
- `F_parser_get_previous_token()` - 获取前一个令牌
- `F_parser_get_next_token()` - 获取下一个令牌

## 内部辅助函数

### 令牌处理
- `F_get_parser_state()` - 获取解析器状态
- `F_advance_token()` - 前进到下一个令牌
- `F_check_token_type()` - 检查当前令牌类型
- `F_match_token_type()` - 匹配令牌类型
- `F_consume_token()` - 消耗令牌

### 状态管理
- 管理当前解析位置
- 维护令牌缓冲区
- 管理错误状态

## 使用示例

### 基本用法
```c
#include "CN_parser_interface.h"
#include "interface/CN_parser_interface_impl.h"

// 创建语法分析器
Stru_ParserInterface_t* parser = F_create_parser_interface();

// 初始化语法分析器
parser->initialize(parser, lexer);

// 解析程序
Stru_AstNode_t* ast = parser->parse_program(parser);

// 检查错误
if (parser->has_errors(parser)) {
    // 处理错误...
}

// 清理资源
parser->destroy(parser);
```

### 错误处理
```c
// 报告错误
parser->report_error(parser, line, column, "语法错误", token);

// 获取错误信息
if (parser->has_errors(parser)) {
    Stru_DynamicArray_t* errors = parser->get_errors(parser);
    // 处理错误...
}
```

## 设计原理

### 模块化设计
- 接口实现与具体解析逻辑分离
- 委托给专门的解析模块处理不同类型的语法结构
- 清晰的职责分离

### 错误恢复
- 支持基本的错误恢复机制
- 在遇到错误时尝试继续解析
- 可配置的最大错误数量

### 性能考虑
- 高效的令牌缓冲区管理
- 最小化的内存分配
- 避免不必要的复制

## 测试

### 单元测试
- 测试接口函数实现
- 测试错误处理机制
- 测试状态管理功能

### 集成测试
- 测试与词法分析器的集成
- 测试与AST模块的集成
- 测试错误恢复机制

## 维护信息

### 版本历史
- **1.0.0** (2026-01-08): 初始版本，基于模块化架构重构

### 维护者
CN_Language架构委员会

### 许可证
MIT许可证
