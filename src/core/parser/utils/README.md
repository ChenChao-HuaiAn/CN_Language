# CN_Language 语法分析器工具模块

## 概述

工具模块是CN_Language语法分析器的辅助组件，提供各种工具函数支持语法分析器的其他模块。本模块包含令牌处理、AST操作、类型检查等通用功能，遵循单一职责原则，确保代码复用和模块解耦。

## 模块功能

### 核心功能
1. **令牌工具** - 提供令牌类型检查和分类功能
2. **AST工具** - 提供AST节点创建和操作功能
3. **类型工具** - 提供类型检查和兼容性判断功能
4. **通用工具** - 提供字符串处理、格式化等通用功能

### 工具类别
- 令牌分类工具：关键字、运算符、分隔符、字面量识别
- 运算符工具：优先级、结合性判断
- AST构建工具：各种AST节点创建函数
- 类型系统工具：类型兼容性检查、类型特征判断
- 字符串工具：字符串复制、格式化

## 文件结构

### 主要文件
1. **CN_parser_utils.h** - 工具模块头文件
   - 声明所有工具函数
   - 定义工具函数接口

2. **CN_parser_utils.c** - 工具模块源文件
   - 实现所有工具函数
   - 提供工具函数的具体实现

### 依赖关系
- 语法分析器接口 (`../CN_parser_interface.h`)
- 令牌模块 (`../../token/CN_token_types.h`)
- AST模块 (`../../ast/CN_ast.h`)
- 基础设施层容器 (`../../../infrastructure/containers/array/CN_dynamic_array.h`)

## 工具函数

### 令牌工具函数

#### `F_is_keyword_token()`
检查令牌是否为关键字。

#### `F_is_operator_token()`
检查令牌是否为运算符。

#### `F_is_delimiter_token()`
检查令牌是否为分隔符。

#### `F_is_literal_token()`
检查令牌是否为字面量。

#### `F_is_type_keyword()`
检查令牌是否为类型关键字。

#### `F_get_operator_precedence()`
获取运算符的优先级。

#### `F_is_right_associative()`
检查运算符是否为右结合。

### AST工具函数

#### `F_create_binary_expression_node()`
创建二元表达式AST节点。

#### `F_create_unary_expression_node()`
创建一元表达式AST节点。

#### `F_create_literal_node()`
创建字面量AST节点。

#### `F_create_identifier_node()`
创建标识符AST节点。

#### `F_create_function_call_node()`
创建函数调用AST节点。

#### `F_create_variable_declaration_node()`
创建变量声明AST节点。

#### `F_create_function_declaration_node()`
创建函数声明AST节点。

### 类型工具函数

#### `F_check_type_compatibility()`
检查两种类型是否兼容。

#### `F_get_type_name()`
获取类型节点的类型名称。

#### `F_is_numeric_type()`
检查类型是否为数值类型（整数或浮点数）。

#### `F_is_boolean_type()`
检查类型是否为布尔类型。

#### `F_is_string_type()`
检查类型是否为字符串类型。

#### `F_is_array_type()`
检查类型是否为数组类型。

#### `F_is_pointer_type()`
检查类型是否为指针类型。

### 其他工具函数

#### `F_copy_string()`
复制字符串，返回新分配的字符串。

#### `F_format_error_message()`
格式化错误信息字符串。

#### `F_get_current_position()`
获取语法分析器当前位置信息。

#### `F_is_at_end_of_file()`
检查是否到达文件结尾。

## 运算符优先级

工具模块定义了CN_Language的运算符优先级：

| 优先级 | 运算符 | 结合性 | 描述 |
|--------|--------|--------|------|
| 10 | `()` | 左到右 | 括号 |
| 9 | `函数()` | 左到右 | 函数调用 |
| 8 | `-`, `非` | 右到左 | 一元运算符 |
| 7 | `*`, `/`, `%` | 左到右 | 乘法运算符 |
| 6 | `+`, `-` | 左到右 | 加法运算符 |
| 5 | `<`, `<=`, `>`, `>=` | 左到右 | 关系运算符 |
| 4 | `==`, `!=` | 左到右 | 相等性运算符 |
| 3 | `且` | 左到右 | 逻辑与 |
| 2 | `或` | 左到右 | 逻辑或 |
| 1 | `=` | 右到左 | 赋值运算符 |

## 使用示例

### 令牌工具使用
```c
#include "utils/CN_parser_utils.h"

// 检查令牌类型
Eum_TokenType token_type = Eum_TOKEN_PLUS;
if (F_is_operator_token(token_type)) {
    int precedence = F_get_operator_precedence(token_type);
    bool is_right_assoc = F_is_right_associative(token_type);
    printf("运算符优先级: %d, 右结合: %s\n", precedence, is_right_assoc ? "是" : "否");
}

// 检查关键字
if (F_is_keyword_token(Eum_TOKEN_KEYWORD_VAR)) {
    printf("这是变量关键字\n");
}
```

### AST工具使用
```c
// 创建二元表达式节点
Stru_Token_t* plus_token = parser->get_current_token(parser);
Stru_AstNode_t* left_expr = F_parse_primary_expression(parser);
Stru_AstNode_t* right_expr = F_parse_primary_expression(parser);
Stru_AstNode_t* binary_expr = F_create_binary_expression_node(parser, plus_token, left_expr, right_expr);

// 创建字面量节点
Stru_Token_t* literal_token = parser->get_current_token(parser);
Stru_AstNode_t* literal_node = F_create_literal_node(parser, literal_token);
```

### 类型工具使用
```c
// 检查类型兼容性
Stru_AstNode_t* type1 = F_parse_type_expression(parser);
Stru_AstNode_t* type2 = F_parse_type_expression(parser);
if (F_check_type_compatibility(type1, type2)) {
    printf("类型兼容\n");
}

// 检查类型特征
if (F_is_numeric_type(type_node)) {
    printf("这是数值类型\n");
}
if (F_is_array_type(type_node)) {
    printf("这是数组类型\n");
}
```

## 设计原理

### 单一职责
每个工具函数只负责一个特定的功能，确保代码的可维护性和可测试性。

### 无状态设计
工具函数不维护内部状态，所有函数都是纯函数或仅依赖于输入参数。

### 错误安全
工具函数进行参数验证，确保输入的有效性，避免崩溃。

### 内存管理
创建对象的函数负责内存分配，调用者负责释放。

## 测试

### 单元测试
- 测试令牌分类功能
- 测试运算符优先级和结合性
- 测试AST节点创建
- 测试类型兼容性检查

### 测试用例示例
```c
// 测试运算符优先级
TEST_ASSERT_EQUAL_INT(7, F_get_operator_precedence(Eum_TOKEN_STAR));
TEST_ASSERT_EQUAL_INT(6, F_get_operator_precedence(Eum_TOKEN_PLUS));

// 测试类型检查
TEST_ASSERT_TRUE(F_is_numeric_type(numeric_type_node));
TEST_ASSERT_FALSE(F_is_string_type(numeric_type_node));

// 测试AST创建
TEST_ASSERT_NOT_NULL(F_create_literal_node(parser, literal_token));
TEST_ASSERT_NOT_NULL(F_create_binary_expression_node(parser, op_token, left, right));
```

## 性能考虑

### 函数调用开销
工具函数设计为轻量级，避免不必要的计算和内存分配。

### 缓存优化
频繁使用的计算结果可以缓存，如运算符优先级表。

### 内存管理
避免频繁的内存分配和释放，使用对象池或重用机制。

## 扩展性

### 添加新的工具函数
1. 在头文件中声明新函数
2. 在源文件中实现函数
3. 添加相应的单元测试

### 自定义运算符优先级
1. 修改运算符优先级表
2. 更新相关文档
3. 测试优先级变化的影响

## 维护信息

### 版本历史
- **1.0.0** (2026-01-08): 初始版本，基于模块化架构设计

### 维护者
CN_Language架构委员会

### 许可证
MIT许可证
