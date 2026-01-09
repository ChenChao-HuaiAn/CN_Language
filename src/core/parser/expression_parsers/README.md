# CN_Language 表达式解析模块

## 概述

表达式解析模块是CN_Language语法分析器的核心组件之一，专门负责解析各种类型的表达式。表达式是CN_Language中最基本的计算单元，包括字面量、标识符、运算符组合等。本模块遵循项目架构规范，采用递归下降解析算法，实现高效、准确的表达式解析。

## 模块功能

### 核心功能
1. **表达式解析** - 解析各种类型的表达式，构建表达式AST节点
2. **运算符优先级处理** - 正确处理运算符优先级和结合性
3. **递归下降解析** - 使用递归下降算法解析复杂表达式
4. **错误检测** - 检测表达式语法错误

### 支持的表达式类型
- 字面量表达式：数字、字符串、布尔值
- 标识符表达式：变量名、函数名
- 一元表达式：负号、逻辑非
- 二元表达式：算术、关系、逻辑运算
- 赋值表达式：变量赋值
- 函数调用表达式：函数调用
- 括号表达式：改变运算优先级

## 文件结构

### 主要文件
1. **CN_parser_expressions.h** - 表达式解析头文件
   - 声明所有表达式解析函数
   - 定义表达式解析接口

### 模块化实现文件
2. **CN_parser_expressions_main.c** - 表达式解析主模块
   - 实现主要表达式解析函数
   - 处理运算符优先级链（赋值、逻辑、关系、算术等）

3. **CN_parser_unary_expressions.c** - 一元表达式解析模块
   - 解析一元表达式（负号、逻辑非等）
   - 处理一元运算符优先级

4. **CN_parser_primary_expressions.c** - 基本表达式解析模块
   - 解析基本表达式（字面量、标识符、括号表达式）
   - 实现字面量、标识符、括号表达式解析

5. **CN_parser_function_calls.c** - 函数调用解析模块
   - 解析函数调用表达式
   - 解析参数列表

### 依赖关系
- 语法分析器接口 (`../CN_parser_interface.h`)
- 语法错误处理 (`../error_handling/CN_parser_errors.h`)
- 语法分析器工具 (`../utils/CN_parser_utils.h`)
- 令牌模块 (`../../token/CN_token_types.h`)
- AST模块 (`../../ast/CN_ast.h`)
- 基础设施层容器 (`../../../infrastructure/containers/array/CN_dynamic_array.h`)

## 表达式解析函数

### 主要解析函数

#### `F_parse_expression()`
解析单个表达式，根据运算符优先级调用相应的子解析函数。

#### `F_parse_assignment_expression()`
解析赋值表达式，如：`变量 = 值`。

#### `F_parse_logical_or_expression()`
解析逻辑或表达式，如：`条件1 或 条件2`。

#### `F_parse_logical_and_expression()`
解析逻辑与表达式，如：`条件1 且 条件2`。

#### `F_parse_equality_expression()`
解析相等性表达式，如：`a == b`, `a != b`。

#### `F_parse_relational_expression()`
解析关系表达式，如：`a < b`, `a >= b`。

#### `F_parse_additive_expression()`
解析加法表达式，如：`a + b`, `a - b`。

#### `F_parse_multiplicative_expression()`
解析乘法表达式，如：`a * b`, `a / b`, `a % b`。

#### `F_parse_unary_expression()`
解析一元表达式，如：`-a`, `非 a`。

#### `F_parse_primary_expression()`
解析基本表达式，包括字面量、标识符、括号表达式等。

### 辅助解析函数

#### `F_parse_literal_expression()`
解析字面量表达式，如：`42`, `3.14`, `"字符串"`, `真`, `假`。

#### `F_parse_identifier_expression()`
解析标识符表达式，如：`变量名`, `函数名`。

#### `F_parse_parenthesized_expression()`
解析括号表达式，如：`(表达式)`。

#### `F_parse_function_call_expression()`
解析函数调用表达式，如：`函数名(参数1, 参数2)`。

#### `F_parse_argument_list()`
解析函数调用的参数列表。

## 运算符优先级

表达式解析模块遵循CN_Language的运算符优先级规则：

| 优先级 | 运算符 | 结合性 | 描述 |
|--------|--------|--------|------|
| 1 | `()` | 左到右 | 括号 |
| 2 | `函数()` | 左到右 | 函数调用 |
| 3 | `-`, `非` | 右到左 | 一元运算符 |
| 4 | `*`, `/`, `%` | 左到右 | 乘法运算符 |
| 5 | `+`, `-` | 左到右 | 加法运算符 |
| 6 | `<`, `<=`, `>`, `>=` | 左到右 | 关系运算符 |
| 7 | `==`, `!=` | 左到右 | 相等性运算符 |
| 8 | `且` | 左到右 | 逻辑与 |
| 9 | `或` | 左到右 | 逻辑或 |
| 10 | `=` | 右到左 | 赋值运算符 |

## 使用示例

### 基本用法
```c
#include "expression_parsers/CN_parser_expressions.h"

// 获取语法分析器接口
Stru_ParserInterface_t* parser = F_create_parser_interface();
parser->initialize(parser, lexer);

// 解析表达式
Stru_AstNode_t* expr = F_parse_expression(parser);

// 检查表达式类型
if (expr != NULL) {
    switch (expr->type) {
        case Eum_AST_NODE_LITERAL:
            // 处理字面量表达式
            break;
        case Eum_AST_NODE_BINARY_EXPR:
            // 处理二元表达式
            break;
        // ... 其他表达式类型
    }
}

// 清理资源
parser->destroy(parser);
```

### 解析复杂表达式
```c
// 解析表达式: a + b * c
Stru_AstNode_t* expr = F_parse_expression(parser);

// 解析表达式: (a + b) * c
Stru_AstNode_t* expr2 = F_parse_expression(parser);

// 解析表达式: 函数名(参数1, 参数2)
Stru_AstNode_t* expr3 = F_parse_expression(parser);
```

## 设计原理

### 递归下降算法
表达式解析模块使用递归下降算法，每个优先级级别对应一个解析函数：
1. `F_parse_expression()` 调用 `F_parse_assignment_expression()`
2. `F_parse_assignment_expression()` 调用 `F_parse_logical_or_expression()`
3. 依此类推，直到 `F_parse_primary_expression()`

### 运算符优先级处理
通过函数调用链实现运算符优先级：
- 低优先级运算符在高层函数中处理
- 高优先级运算符在低层函数中处理
- 通过递归调用实现正确的优先级顺序

### 错误恢复
- 在解析失败时返回NULL
- 报告详细的错误信息
- 支持继续解析后续表达式

## 测试

### 单元测试
- 测试各种表达式类型的解析
- 测试运算符优先级
- 测试错误处理

### 测试用例示例
```c
// 测试字面量表达式
TEST_ASSERT_NOT_NULL(F_parse_literal_expression(parser));

// 测试二元表达式
TEST_ASSERT_NOT_NULL(F_parse_binary_expression(parser));

// 测试运算符优先级
TEST_ASSERT_EQUAL_INT(expected_precedence, actual_precedence);
```

## 性能考虑

### 递归深度
- 表达式解析使用递归算法
- 最大递归深度受限于栈大小
- 复杂表达式可能需要优化

### 内存使用
- 每个表达式节点独立分配内存
- 支持表达式节点重用
- 避免内存泄漏

## 扩展性

### 添加新的运算符
1. 在相应的优先级级别添加解析函数
2. 更新运算符优先级表
3. 添加对应的AST节点类型

### 自定义表达式类型
1. 添加新的表达式解析函数
2. 更新表达式类型枚举
3. 实现对应的AST构建逻辑

## 维护信息

### 版本历史
- **1.1.0** (2026-01-09): 模块化重构，拆分为多个专门的文件
- **1.0.0** (2026-01-08): 初始版本，基于模块化架构设计

### 维护者
CN_Language架构委员会

### 许可证
MIT许可证
