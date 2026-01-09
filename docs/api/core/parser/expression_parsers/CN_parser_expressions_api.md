# CN_Language 表达式解析模块 API 文档

## 概述

表达式解析模块是CN_Language语法分析器的核心组件之一，专门负责解析各种类型的表达式。本模块采用递归下降解析算法，实现高效、准确的表达式解析，支持CN_Language的所有表达式类型。

## 模块信息

- **模块名称**: 表达式解析模块 (Expression Parsers)
- **所属层级**: 核心层 (Core Layer)
- **模块版本**: 1.1.0
- **最后更新**: 2026-01-09
- **维护者**: CN_Language架构委员会
- **许可证**: MIT

## 文件结构

### 头文件
- `src/core/parser/expression_parsers/CN_parser_expressions.h` - 主头文件，包含所有表达式解析函数的声明

### 实现文件
- `src/core/parser/expression_parsers/CN_parser_expressions_main.c` - 表达式解析主模块
- `src/core/parser/expression_parsers/CN_parser_unary_expressions.c` - 一元表达式解析模块
- `src/core/parser/expression_parsers/CN_parser_primary_expressions.c` - 基本表达式解析模块
- `src/core/parser/expression_parsers/CN_parser_function_calls.c` - 函数调用解析模块
- `src/core/parser/expression_parsers/CN_parser_postfix_expressions.c` - 后缀表达式解析模块

## API 函数参考

### 主要表达式解析函数

#### `F_parse_expression`
```c
Stru_AstNode_t* F_parse_expression(Stru_ParserInterface_t* interface);
```

**描述**: 解析单个表达式，根据运算符优先级调用相应的子解析函数。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 表达式AST节点指针
- 失败: NULL

**示例**:
```c
Stru_AstNode_t* expr = F_parse_expression(parser);
if (expr != NULL) {
    // 处理表达式
}
```

#### `F_parse_assignment_expression`
```c
Stru_AstNode_t* F_parse_assignment_expression(Stru_ParserInterface_t* interface);
```

**描述**: 解析赋值表达式，如：`变量 = 值`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 赋值表达式AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: x = 42
Stru_AstNode_t* assignment = F_parse_assignment_expression(parser);
```

#### `F_parse_logical_or_expression`
```c
Stru_AstNode_t* F_parse_logical_or_expression(Stru_ParserInterface_t* interface);
```

**描述**: 解析逻辑或表达式，如：`条件1 或 条件2`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 逻辑或表达式AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: a > 0 或 b < 10
Stru_AstNode_t* logical_or = F_parse_logical_or_expression(parser);
```

#### `F_parse_logical_and_expression`
```c
Stru_AstNode_t* F_parse_logical_and_expression(Stru_ParserInterface_t* interface);
```

**描述**: 解析逻辑与表达式，如：`条件1 且 条件2`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 逻辑与表达式AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: x > 0 且 x < 100
Stru_AstNode_t* logical_and = F_parse_logical_and_expression(parser);
```

#### `F_parse_equality_expression`
```c
Stru_AstNode_t* F_parse_equality_expression(Stru_ParserInterface_t* interface);
```

**描述**: 解析相等性表达式，如：`a == b`, `a != b`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 相等性表达式AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: x == y
Stru_AstNode_t* equality = F_parse_equality_expression(parser);
```

#### `F_parse_relational_expression`
```c
Stru_AstNode_t* F_parse_relational_expression(Stru_ParserInterface_t* interface);
```

**描述**: 解析关系表达式，如：`a < b`, `a >= b`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 关系表达式AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: x < 100
Stru_AstNode_t* relational = F_parse_relational_expression(parser);
```

#### `F_parse_additive_expression`
```c
Stru_AstNode_t* F_parse_additive_expression(Stru_ParserInterface_t* interface);
```

**描述**: 解析加法表达式，如：`a + b`, `a - b`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 加法表达式AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: a + b - c
Stru_AstNode_t* additive = F_parse_additive_expression(parser);
```

#### `F_parse_multiplicative_expression`
```c
Stru_AstNode_t* F_parse_multiplicative_expression(Stru_ParserInterface_t* interface);
```

**描述**: 解析乘法表达式，如：`a * b`, `a / b`, `a % b`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 乘法表达式AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: a * b / c
Stru_AstNode_t* multiplicative = F_parse_multiplicative_expression(parser);
```

### 一元和基本表达式解析函数

#### `F_parse_unary_expression`
```c
Stru_AstNode_t* F_parse_unary_expression(Stru_ParserInterface_t* interface);
```

**描述**: 解析一元表达式，如：`-a`, `非 a`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 一元表达式AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: -x
Stru_AstNode_t* unary = F_parse_unary_expression(parser);
```

#### `F_parse_primary_expression`
```c
Stru_AstNode_t* F_parse_primary_expression(Stru_ParserInterface_t* interface);
```

**描述**: 解析基本表达式，包括字面量、标识符、括号表达式等。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 基本表达式AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: 42, "hello", (x + y)
Stru_AstNode_t* primary = F_parse_primary_expression(parser);
```

#### `F_parse_literal_expression`
```c
Stru_AstNode_t* F_parse_literal_expression(Stru_ParserInterface_t* interface);
```

**描述**: 解析字面量表达式，如：`42`, `3.14`, `"字符串"`, `真`, `假`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 字面量表达式AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: 3.14
Stru_AstNode_t* literal = F_parse_literal_expression(parser);
```

#### `F_parse_identifier_expression`
```c
Stru_AstNode_t* F_parse_identifier_expression(Stru_ParserInterface_t* interface);
```

**描述**: 解析标识符表达式，如：`变量名`, `函数名`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 标识符表达式AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: myVariable
Stru_AstNode_t* identifier = F_parse_identifier_expression(parser);
```

#### `F_parse_parenthesized_expression`
```c
Stru_AstNode_t* F_parse_parenthesized_expression(Stru_ParserInterface_t* interface);
```

**描述**: 解析括号表达式，如：`(表达式)`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 括号表达式AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: (x + y)
Stru_AstNode_t* parenthesized = F_parse_parenthesized_expression(parser);
```

### 函数调用和参数解析函数

#### `F_parse_function_call_expression`
```c
Stru_AstNode_t* F_parse_function_call_expression(Stru_ParserInterface_t* interface);
```

**描述**: 解析函数调用表达式，如：`函数名(参数1, 参数2)`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 函数调用表达式AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: calculate(10, 20)
Stru_AstNode_t* func_call = F_parse_function_call_expression(parser);
```

#### `F_parse_argument_list`
```c
Stru_DynamicArray_t* F_parse_argument_list(Stru_ParserInterface_t* interface);
```

**描述**: 解析函数调用的参数列表。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 参数表达式数组指针
- 失败: NULL

**示例**:
```c
// 解析: (10, 20, 30)
Stru_DynamicArray_t* args = F_parse_argument_list(parser);
```

#### `F_parse_postfix_expression`
```c
Stru_AstNode_t* F_parse_postfix_expression(Stru_ParserInterface_t* interface,
                                          Stru_AstNode_t* base);
```

**描述**: 解析后缀表达式，包括成员访问和数组索引。

**参数**:
- `interface`: 语法分析器接口指针
- `base`: 基础表达式节点

**返回值**:
- 成功: 后缀表达式AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: array[0]
Stru_AstNode_t* base = F_parse_identifier_expression(parser);
Stru_AstNode_t* postfix = F_parse_postfix_expression(parser, base);
```

#### `F_parse_conditional_expression`
```c
Stru_AstNode_t* F_parse_conditional_expression(Stru_ParserInterface_t* interface);
```

**描述**: 解析条件表达式（三元运算符），如：`条件 ? 真值 : 假值`。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- 成功: 条件表达式AST节点指针
- 失败: NULL

**示例**:
```c
// 解析: x > 0 ? 1 : -1
Stru_AstNode_t* conditional = F_parse_conditional_expression(parser);
```

## 运算符优先级

表达式解析模块遵循CN_Language的运算符优先级规则：

| 优先级 | 运算符 | 结合性 | 描述 | 对应解析函数 |
|--------|--------|--------|------|-------------|
| 1 | `()` | 左到右 | 括号 | `F_parse_parenthesized_expression` |
| 2 | `函数()` | 左到右 | 函数调用 | `F_parse_function_call_expression` |
| 3 | `-`, `非` | 右到左 | 一元运算符 | `F_parse_unary_expression` |
| 4 | `*`, `/`, `%` | 左到右 | 乘法运算符 | `F_parse_multiplicative_expression` |
| 5 | `+`, `-` | 左到右 | 加法运算符 | `F_parse_additive_expression` |
| 6 | `<`, `<=`, `>`, `>=` | 左到右 | 关系运算符 | `F_parse_relational_expression` |
| 7 | `==`, `!=` | 左到右 | 相等性运算符 | `F_parse_equality_expression` |
| 8 | `且` | 左到右 | 逻辑与 | `F_parse_logical_and_expression` |
| 9 | `或` | 左到右 | 逻辑或 | `F_parse_logical_or_expression` |
| 10 | `=` | 右到左 | 赋值运算符 | `F_parse_assignment_expression` |

## 使用示例

### 基本用法
```c
#include "core/parser/expression_parsers/CN_parser_expressions.h"

// 获取语法分析器接口
Stru_ParserInterface_t* parser = F_create_parser_interface();
parser->initialize(parser, lexer);

// 解析表达式
Stru_AstNode_t* expr = F_parse_expression(parser);

// 检查表达式类型
if (expr != NULL) {
    switch (expr->type) {
        case Eum_AST_NODE_LITERAL:
            printf("字面量表达式\n");
            break;
        case Eum_AST_NODE_BINARY_EXPR:
            printf("二元表达式\n");
            break;
        case Eum_AST_NODE_UNARY_EXPR:
            printf("一元表达式\n");
            break;
        case Eum_AST_NODE_FUNCTION_CALL:
            printf("函数调用表达式\n");
            break;
        default:
            printf("其他表达式类型\n");
    }
}

// 清理资源
parser->destroy(parser);
```

### 解析复杂表达式
```c
// 解析简单表达式: 42
Stru_AstNode_t* literal = F_parse_expression(parser);

// 解析算术表达式: a + b * c
Stru_AstNode_t* arithmetic = F_parse_expression(parser);

// 解析带括号的表达式: (a + b) * c
Stru_AstNode_t* with_parentheses = F_parse_expression(parser);

// 解析函数调用: 最大值(a, b, c)
Stru_AstNode_t* function_call = F_parse_expression(parser);

// 解析条件表达式: x > 0 ? x : -x
Stru_AstNode_t* conditional = F_parse_expression(parser);
```

### 错误处理示例
```c
Stru_AstNode_t* expr = F_parse_expression(parser);
if (expr == NULL) {
    // 获取错误信息
    Stru_SyntaxError_t* error = parser->get_last_error(parser);
    if (error != NULL) {
        printf("解析错误: %s\n", error->message);
        printf("位置: 行 %d, 列 %d\n", error->line, error->column);
    }
    
    // 尝试错误恢复
    parser->recover_from_error(parser);
    
    // 继续解析后续表达式
    expr = F_parse_expression(parser);
}
```

## 设计原理

### 递归下降算法
表达式解析模块使用递归下降算法，每个优先级级别对应一个解析函数：
1. `F_parse_expression()` 调用 `F_parse_assignment_expression()`
2. `F_parse_assignment_expression()` 调用 `F_parse_logical_or_expression()`
3. `F_parse_logical_or_expression()` 调用 `F_parse_logical_and_expression()`
4. 依此类推，直到 `F_parse_primary_expression()`

### 运算符优先级处理
通过函数调用链实现运算符优先级：
- 低优先级运算符在高层函数中处理
- 高优先级运算符在低层函数中处理
- 通过递归调用实现正确的优先级顺序

### 错误恢复机制
- 在解析失败时返回NULL
- 报告详细的错误信息
- 支持继续解析后续表达式
- 与错误处理模块集成

## 性能考虑

### 递归深度
- 表达式解析使用递归算法
- 最大递归深度受限于栈大小
- 复杂表达式可能需要优化

### 内存使用
- 每个表达式节点独立分配内存
- 支持表达式节点重用
- 避免内存泄漏

## 测试

### 单元测试
表达式解析模块的单元测试位于：
- `tests/core/parser/test_parser_expressions.c`

### 测试用例类型
1. **字面量表达式测试**: 测试各种字面量的解析
2. **运算符优先级测试**: 测试运算符优先级是否正确
3. **复杂表达式测试**: 测试嵌套表达式的解析
4. **错误处理测试**: 测试语法错误的检测和恢复

### 运行测试
```bash
cd tests
make test_parser_expressions
./test_parser_expressions
```

## 扩展性

### 添加新的运算符
1. 在相应的优先级级别添加解析函数
2. 更新运算符优先级表
3. 添加对应的AST节点类型
4. 更新测试用例

### 自定义表达式类型
1. 添加新的表达式解析函数
2. 更新表达式类型枚举
3. 实现对应的AST构建逻辑
4. 添加相应的测试用例

## 依赖关系

### 内部依赖
- 语法分析器接口 (`../CN_parser_interface.h`)
- 语法错误处理 (`../error_handling/CN_parser_errors.h`)
-
