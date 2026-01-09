# CN_Language 语句解析模块

## 概述

语句解析模块是CN_Language语法分析器的核心组件之一，专门负责解析各种类型的语句。语句是CN_Language程序的基本执行单元，包括表达式语句、控制流语句、声明语句等。本模块遵循项目架构规范，采用递归下降解析算法，实现高效、准确的语句解析。

## 模块功能

### 核心功能
1. **语句解析** - 解析各种类型的语句，构建语句AST节点
2. **控制流解析** - 解析条件语句、循环语句等控制流结构
3. **代码块解析** - 解析代码块和语句列表
4. **错误检测** - 检测语句语法错误

### 支持的语句类型
- 表达式语句：表达式后跟分号
- 代码块语句：大括号包围的语句序列
- 条件语句：如果-否则结构
- 循环语句：当循环、循环结构
- 控制语句：中断、继续、返回
- 声明语句：变量声明
- 空语句：单独的分号

## 文件结构

### 主要文件
1. **CN_parser_statements.h** - 语句解析头文件
   - 声明所有语句解析函数
   - 定义语句解析接口

### 模块化源文件
为了遵循单一职责原则和500行文件限制，语句解析模块被拆分为以下专门的文件：

2. **CN_parser_statements_main.c** - 语句解析主模块
   - 实现主语句解析函数 `F_parse_statement()`
   - 实现语句类型分发逻辑
   - 实现语句列表解析函数 `F_parse_statement_list()`
   - 实现变量声明语句解析函数 `F_parse_variable_declaration_statement()`

3. **CN_parser_control_statements.c** - 控制语句解析模块
   - 实现中断语句解析 `F_parse_break_statement()`
   - 实现继续语句解析 `F_parse_continue_statement()`
   - 实现返回语句解析 `F_parse_return_statement()`
   - 实现空语句解析 `F_parse_empty_statement()`

4. **CN_parser_loop_statements.c** - 循环语句解析模块
   - 实现while循环语句解析 `F_parse_while_statement()`
   - 实现for循环语句解析 `F_parse_for_statement()`

5. **CN_parser_conditional_statements.c** - 条件语句解析模块
   - 实现if条件语句解析 `F_parse_if_statement()`

6. **CN_parser_block_statements.c** - 代码块和表达式语句解析模块
   - 实现表达式语句解析 `F_parse_expression_statement()`
   - 实现代码块语句解析 `F_parse_block_statement()`

### 依赖关系
- 语法分析器接口 (`../CN_parser_interface.h`)
- 语法错误处理 (`../CN_syntax_error.h`)
- 表达式解析模块 (`../expression_parsers/CN_parser_expressions.h`)
- 声明解析模块 (`../declaration_parsers/CN_parser_declarations.h`)
- 令牌模块 (`../../token/CN_token_types.h`)
- AST模块 (`../../ast/CN_ast.h`)
- 基础设施层容器 (`../../../infrastructure/containers/array/CN_dynamic_array.h`)

## 语句解析函数

### 主要解析函数

#### `F_parse_statement()`
解析单个语句，根据当前令牌类型调用相应的子解析函数。

#### `F_parse_expression_statement()`
解析表达式语句，如：`表达式;`。

#### `F_parse_block_statement()`
解析代码块语句，如：`{ 语句1; 语句2; }`。

#### `F_parse_if_statement()`
解析条件语句，如：`如果 (条件) { ... }`。

#### `F_parse_while_statement()`
解析循环语句，如：`当 (条件) { ... }`。

#### `F_parse_for_statement()`
解析for循环语句，如：`循环 (初始化; 条件; 更新) { ... }`。

#### `F_parse_break_statement()`
解析中断语句，如：`中断;`。

#### `F_parse_continue_statement()`
解析继续语句，如：`继续;`。

#### `F_parse_return_statement()`
解析返回语句，如：`返回 值;`。

#### `F_parse_empty_statement()`
解析空语句，如：`;`。

#### `F_parse_variable_declaration_statement()`
解析变量声明语句，如：`变量 名称 = 值;`。

### 辅助解析函数

#### `F_parse_statement_list()`
解析代码块中的语句列表。

## 语句语法

### 表达式语句
```
表达式;
```

### 代码块语句
```
{
    语句1;
    语句2;
    ...
}
```

### 条件语句
```
如果 (条件) {
    // 条件为真时执行
}
```

```
如果 (条件) {
    // 条件为真时执行
} 否则 {
    // 条件为假时执行
}
```

### 循环语句
```
当 (条件) {
    // 条件为真时重复执行
}
```

```
循环 (初始化; 条件; 更新) {
    // 条件为真时重复执行
}
```

### 控制语句
```
中断;      // 退出当前循环
继续;      // 跳过当前循环迭代
返回 值;   // 从函数返回
```

### 变量声明语句
```
变量 名称 = 值;
```

## 使用示例

### 基本用法
```c
#include "statement_parsers/CN_parser_statements.h"

// 获取语法分析器接口
Stru_ParserInterface_t* parser = F_create_parser_interface();
parser->initialize(parser, lexer);

// 解析语句
Stru_AstNode_t* stmt = F_parse_statement(parser);

// 检查语句类型
if (stmt != NULL) {
    switch (stmt->type) {
        case Eum_AST_NODE_EXPRESSION_STMT:
            // 处理表达式语句
            break;
        case Eum_AST_NODE_IF_STMT:
            // 处理条件语句
            break;
        case Eum_AST_NODE_WHILE_STMT:
            // 处理循环语句
            break;
        // ... 其他语句类型
    }
}

// 清理资源
parser->destroy(parser);
```

### 解析复杂语句
```c
// 解析条件语句
Stru_AstNode_t* if_stmt = F_parse_if_statement(parser);

// 解析循环语句
Stru_AstNode_t* while_stmt = F_parse_while_statement(parser);

// 解析代码块
Stru_AstNode_t* block = F_parse_block_statement(parser);
```

## 设计原理

### 递归下降算法
语句解析模块使用递归下降算法：
1. `F_parse_statement()` 根据当前令牌类型分发到相应的解析函数
2. 每个语句类型有专门的解析函数
3. 支持嵌套语句结构

### 错误恢复
- 在解析失败时返回NULL
- 报告详细的错误信息
- 支持跳过错误语句继续解析

### 控制流处理
- 正确处理条件语句的嵌套
- 支持循环语句的嵌套
- 处理控制语句的作用域

## 测试

### 单元测试
- 测试各种语句类型的解析
- 测试控制流嵌套
- 测试错误处理

### 测试用例示例
```c
// 测试表达式语句
TEST_ASSERT_NOT_NULL(F_parse_expression_statement(parser));

// 测试条件语句
TEST_ASSERT_NOT_NULL(F_parse_if_statement(parser));

// 测试循环语句
TEST_ASSERT_NOT_NULL(F_parse_while_statement(parser));

// 测试代码块
TEST_ASSERT_NOT_NULL(F_parse_block_statement(parser));
```

## 性能考虑

### 递归深度
- 语句解析可能涉及多层嵌套
- 最大递归深度受限于栈大小
- 复杂嵌套结构可能需要优化

### 内存使用
- 每个语句节点独立分配内存
- 支持语句节点重用
- 避免内存泄漏

## 扩展性

### 添加新的语句类型
1. 添加新的语句解析函数
2. 更新语句类型枚举
3. 在`F_parse_statement()`中添加分发逻辑

### 自定义控制流结构
1. 添加新的控制流解析函数
2. 实现对应的AST构建逻辑
3. 更新语法规范

## 维护信息

### 版本历史
- **1.0.0** (2026-01-08): 初始版本，基于模块化架构设计
- **1.1.0** (2026-01-09): 模块化重构，拆分为多个专门文件以遵循单一职责原则

### 维护者
CN_Language架构委员会

### 许可证
MIT许可证
