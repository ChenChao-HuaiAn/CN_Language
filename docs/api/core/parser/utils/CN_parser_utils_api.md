# CN_Language 语法分析器工具模块 API 文档

## 概述

语法分析器工具模块是CN_Language语法分析器的辅助组件，提供各种工具函数支持语法分析器的其他模块。本模块包含令牌处理、AST操作、类型检查等通用功能，遵循单一职责原则，确保代码复用和模块解耦。

## 模块信息

- **模块名称**: 语法分析器工具模块 (Parser Utilities)
- **所属层级**: 核心层 (Core Layer)
- **模块版本**: 1.0.0
- **最后更新**: 2026-01-08
- **维护者**: CN_Language架构委员会
- **许可证**: MIT

## 文件结构

### 头文件
- `src/core/parser/utils/CN_parser_utils.h` - 主头文件，包含所有工具函数的声明

### 实现文件
- `src/core/parser/utils/CN_parser_utils.c` - 工具模块源文件，实现所有工具函数
- `src/core/parser/utils/CN_parser_token_utils.c` - 令牌工具函数实现
- `src/core/parser/utils/CN_parser_ast_utils.c` - AST工具函数实现

## API 函数参考

### 令牌工具函数

#### `F_is_keyword_token`
```c
bool F_is_keyword_token(Eum_TokenType token_type);
```

**描述**: 检查令牌是否为关键字。

**参数**:
- `token_type`: 令牌类型枚举值

**返回值**:
- `true`: 令牌是关键字
- `false`: 令牌不是关键字

**示例**:
```c
if (F_is_keyword_token(Eum_TOKEN_KEYWORD_VAR)) {
    printf("这是变量关键字\n");
}
```

#### `F_is_operator_token`
```c
bool F_is_operator_token(Eum_TokenType token_type);
```

**描述**: 检查令牌是否为运算符。

**参数**:
- `token_type`: 令牌类型枚举值

**返回值**:
- `true`: 令牌是运算符
- `false`: 令牌不是运算符

**示例**:
```c
if (F_is_operator_token(Eum_TOKEN_PLUS)) {
    printf("这是加法运算符\n");
}
```

#### `F_is_delimiter_token`
```c
bool F_is_delimiter_token(Eum_TokenType token_type);
```

**描述**: 检查令牌是否为分隔符。

**参数**:
- `token_type`: 令牌类型枚举值

**返回值**:
- `true`: 令牌是分隔符
- `false`: 令牌不是分隔符

**示例**:
```c
if (F_is_delimiter_token(Eum_TOKEN_SEMICOLON)) {
    printf("这是分号分隔符\n");
}
```

#### `F_is_literal_token`
```c
bool F_is_literal_token(Eum_TokenType token_type);
```

**描述**: 检查令牌是否为字面量。

**参数**:
- `token_type`: 令牌类型枚举值

**返回值**:
- `true`: 令牌是字面量
- `false`: 令牌不是字面量

**示例**:
```c
if (F_is_literal_token(Eum_TOKEN_INTEGER_LITERAL)) {
    printf("这是整数字面量\n");
}
```

#### `F_is_type_keyword`
```c
bool F_is_type_keyword(Eum_TokenType token_type);
```

**描述**: 检查令牌是否为类型关键字。

**参数**:
- `token_type`: 令牌类型枚举值

**返回值**:
- `true`: 令牌是类型关键字
- `false`: 令牌不是类型关键字

**示例**:
```c
if (F_is_type_keyword(Eum_TOKEN_KEYWORD_INT)) {
    printf("这是整数类型关键字\n");
}
```

#### `F_get_operator_precedence`
```c
int F_get_operator_precedence(Eum_TokenType token_type);
```

**描述**: 获取运算符的优先级。

**参数**:
- `token_type`: 运算符令牌类型

**返回值**:
- 运算符优先级，值越大优先级越高
- 如果不是运算符，返回0

**示例**:
```c
int precedence = F_get_operator_precedence(Eum_TOKEN_PLUS);
printf("加法运算符优先级: %d\n", precedence);
```

#### `F_is_right_associative`
```c
bool F_is_right_associative(Eum_TokenType token_type);
```

**描述**: 检查运算符是否为右结合。

**参数**:
- `token_type`: 运算符令牌类型

**返回值**:
- `true`: 运算符是右结合
- `false`: 运算符是左结合或不是运算符

**示例**:
```c
if (F_is_right_associative(Eum_TOKEN_ASSIGN)) {
    printf("赋值运算符是右结合的\n");
}
```

### AST工具函数

#### `F_create_binary_expression_node`
```c
Stru_AstNode_t* F_create_binary_expression_node(Stru_ParserInterface_t* interface,
                                               Stru_Token_t* operator_token,
                                               Stru_AstNode_t* left,
                                               Stru_AstNode_t* right);
```

**描述**: 创建二元表达式AST节点。

**参数**:
- `interface`: 语法分析器接口指针
- `operator_token`: 运算符令牌
- `left`: 左表达式节点
- `right`: 右表达式节点

**返回值**:
- 成功: 二元表达式AST节点指针
- 失败: NULL

**示例**:
```c
Stru_AstNode_t* binary_expr = F_create_binary_expression_node(
    parser, plus_token, left_expr, right_expr);
```

#### `F_create_unary_expression_node`
```c
Stru_AstNode_t* F_create_unary_expression_node(Stru_ParserInterface_t* interface,
                                              Stru_Token_t* operator_token,
                                              Stru_AstNode_t* operand);
```

**描述**: 创建一元表达式AST节点。

**参数**:
- `interface`: 语法分析器接口指针
- `operator_token`: 运算符令牌
- `operand`: 操作数表达式节点

**返回值**:
- 成功: 一元表达式AST节点指针
- 失败: NULL

**示例**:
```c
Stru_AstNode_t* unary_expr = F_create_unary_expression_node(
    parser, minus_token, operand_expr);
```

#### `F_create_compound_assignment_node`
```c
Stru_AstNode_t* F_create_compound_assignment_node(Stru_ParserInterface_t* interface,
                                                 Stru_Token_t* operator_token,
                                                 Stru_AstNode_t* left,
                                                 Stru_AstNode_t* right);
```

**描述**: 创建复合赋值表达式AST节点。

**参数**:
- `interface`: 语法分析器接口指针
- `operator_token`: 复合赋值运算符令牌
- `left`: 左表达式节点
- `right`: 右表达式节点

**返回值**:
- 成功: 复合赋值表达式AST节点指针
- 失败: NULL

**示例**:
```c
Stru_AstNode_t* compound_assign = F_create_compound_assignment_node(
    parser, plus_assign_token, left_expr, right_expr);
```

#### `F_create_literal_node`
```c
Stru_AstNode_t* F_create_literal_node(Stru_ParserInterface_t* interface,
                                     Stru_Token_t* token);
```

**描述**: 创建字面量AST节点。

**参数**:
- `interface`: 语法分析器接口指针
- `token`: 字面量令牌

**返回值**:
- 成功: 字面量AST节点指针
- 失败: NULL

**示例**:
```c
Stru_AstNode_t* literal_node = F_create_literal_node(parser, integer_token);
```

#### `F_create_identifier_node`
```c
Stru_AstNode_t* F_create_identifier_node(Stru_ParserInterface_t* interface,
                                        Stru_Token_t* token);
```

**描述**: 创建标识符AST节点。

**参数**:
- `interface`: 语法分析器接口指针
- `token`: 标识符令牌

**返回值**:
- 成功: 标识符AST节点指针
- 失败: NULL

**示例**:
```c
Stru_AstNode_t* identifier_node = F_create_identifier_node(parser, id_token);
```

#### `F_create_function_call_node`
```c
Stru_AstNode_t* F_create_function_call_node(Stru_ParserInterface_t* interface,
                                           Stru_Token_t* function_name,
                                           Stru_DynamicArray_t* arguments);
```

**描述**: 创建函数调用AST节点。

**参数**:
- `interface`: 语法分析器接口指针
- `function_name`: 函数名令牌
- `arguments`: 参数表达式数组

**返回值**:
- 成功: 函数调用AST节点指针
- 失败: NULL

**示例**:
```c
Stru_AstNode_t* func_call = F_create_function_call_node(
    parser, func_name_token, args_array);
```

#### `F_create_variable_declaration_node`
```c
Stru_AstNode_t* F_create_variable_declaration_node(Stru_ParserInterface_t* interface,
                                                  Stru_Token_t* keyword_token,
                                                  Stru_Token_t* identifier_token,
                                                  Stru_Token_t* type_token,
                                                  Stru_AstNode_t* initializer);
```

**描述**: 创建变量声明AST节点。

**参数**:
- `interface`: 语法分析器接口指针
- `keyword_token`: 关键字令牌（如"变量"）
- `identifier_token`: 标识符令牌
- `type_token`: 类型令牌（可选，可为NULL）
- `initializer`: 初始化表达式节点（可选，可为NULL）

**返回值**:
- 成功: 变量声明AST节点指针
- 失败: NULL

**示例**:
```c
Stru_AstNode_t* var_decl = F_create_variable_declaration_node(
    parser, var_keyword_token, id_token, type_token, init_expr);
```

#### `F_create_function_declaration_node`
```c
Stru_AstNode_t* F_create_function_declaration_node(Stru_ParserInterface_t* interface,
                                                  Stru_Token_t* keyword_token,
                                                  Stru_Token_t* identifier_token,
                                                  Stru_DynamicArray_t* parameters,
                                                  Stru_AstNode_t* return_type,
                                                  Stru_AstNode_t* body);
```

**描述**: 创建函数声明AST节点。

**参数**:
- `interface`: 语法分析器接口指针
- `keyword_token`: 关键字令牌（如"函数"）
- `identifier_token`: 函数名令牌
- `parameters`: 参数声明数组
- `return_type`: 返回类型节点（可选，可为NULL）
- `body`: 函数体语句节点

**返回值**:
- 成功: 函数声明AST节点指针
- 失败: NULL

**示例**:
```c
Stru_AstNode_t* func_decl = F_create_function_declaration_node(
    parser, func_keyword_token, func_name_token, params_array, return_type_node, body_stmt);
```

#### `F_create_cast_expression_node`
```c
Stru_AstNode_t* F_create_cast_expression_node(Stru_ParserInterface_t* interface,
                                             Stru_Token_t* cast_token,
                                             Stru_AstNode_t* type_node,
                                             Stru_AstNode_t* operand);
```

**描述**: 创建类型转换表达式AST节点。

**参数**:
- `interface`: 语法分析器接口指针
- `cast_token`: 类型转换令牌（通常是左括号）
- `type_node`: 目标类型节点
- `operand`: 要转换的表达式节点

**返回值**:
- 成功: 类型转换表达式AST节点指针
- 失败: NULL

**示例**:
```c
Stru_AstNode_t* cast_expr = F_create_cast_expression_node(
    parser, lparen_token, target_type_node, operand_expr);
```

#### `F_create_member_access_node`
```c
Stru_AstNode_t* F_create_member_access_node(Stru_ParserInterface_t* interface,
                                           Stru_Token_t* dot_token,
                                           Stru_AstNode_t* object,
                                           Stru_Token_t* member_name);
```

**描述**: 创建成员访问表达式AST节点。

**参数**:
- `interface`: 语法分析器接口指针
- `dot_token`: 点运算符令牌
- `object`: 对象表达式节点
- `member_name`: 成员名称令牌

**返回值**:
- 成功: 成员访问表达式AST节点指针
- 失败: NULL

**示例**:
```c
Stru_AstNode_t* member_access = F_create_member_access_node(
    parser, dot_token, object_expr, member_token);
```

#### `F_create_array_index_node`
```c
Stru_AstNode_t* F_create_array_index_node(Stru_ParserInterface_t* interface,
                                         Stru_Token_t* lbracket_token,
                                         Stru_AstNode_t* array,
                                         Stru_AstNode_t* index,
                                         Stru_Token_t* rbracket_token);
```

**描述**: 创建数组索引表达式AST节点。

**参数**:
- `interface`: 语法分析器接口指针
- `lbracket_token`: 左方括号令牌
- `array`: 数组表达式节点
- `index`: 索引表达式节点
- `rbracket_token`: 右方括号令牌（可选，可为NULL）

**返回值**:
- 成功: 数组索引表达式AST节点指针
- 失败: NULL

**示例**:
```c
Stru_AstNode_t* array_index = F_create_array_index_node(
    parser, lbracket_token, array_expr, index_expr, rbracket_token);
```

#### `F_create_conditional_expression_node`
```c
Stru_AstNode_t* F_create_conditional_expression_node(Stru_ParserInterface_t* interface,
                                                    Stru_Token_t* question_token,
                                                    Stru_AstNode_t* condition,
                                                    Stru_AstNode_t* true_expr,
                                                    Stru_AstNode_t* false_expr,
                                                    Stru_Token_t* colon_token);
```

**描述**: 创建条件表达式AST节点（三元运算符）。

**参数**:
- `interface`: 语法分析器接口指针
- `question_token`: 问号运算符令牌
- `condition`: 条件表达式节点
- `true_expr`: 真值表达式节点
- `false_expr`: 假值表达式节点
- `colon_token`: 冒号运算符令牌（可选，可为NULL）

**返回值**:
- 成功: 条件表达式AST节点指针
- 失败: NULL

**示例**:
```c
Stru_AstNode_t* conditional_expr = F_create_conditional_expression_node(
    parser, question_token, cond_expr, true_expr, false_expr, colon_token);
```

### 类型工具函数

#### `F_check_type_compatibility`
```c
bool F_check_type_compatibility(Stru_AstNode_t* type1, Stru_AstNode_t* type2);
```

**描述**: 检查两种类型是否兼容。

**参数**:
- `type1`: 类型1节点
- `type2`: 类型2节点

**返回值**:
- `true`: 类型兼容
- `false`: 类型不兼容

**示例**:
```c
if (F_check_type_compatibility(type1, type2)) {
    printf("类型兼容，可以进行赋值\n");
}
```

#### `F_get_type_name`
```c
const char* F_get_type_name(Stru_AstNode_t* type_node);
```

**描述**: 获取类型节点的类型名称。

**参数**:
- `type_node`: 类型节点

**返回值**:
- 类型名称字符串
- 如果类型节点无效，返回NULL

**示例**:
```c
const char* type_name = F_get_type_name(type_node);
printf("类型名称: %s\n", type_name);
```

#### `F_is_numeric_type`
```c
bool F_is_numeric_type(Stru_AstNode_t* type_node);
```

**描述**: 检查类型是否为数值类型（整数或浮点数）。

**参数**:
- `type_node`: 类型节点

**返回值**:
- `true`: 是数值类型
- `false`: 不是数值类型

**示例**:
```c
if (F_is_numeric_type(type_node)) {
    printf("可以进行算术运算\n");
}
```

#### `F_is_boolean_type`
```c
bool F_is_boolean_type(Stru_AstNode_t* type_node);
```

**描述**: 检查类型是否为布尔类型。

**参数**:
- `type_node`: 类型节点

**返回值**:
- `true`: 是布尔类型
- `false`: 不是布尔类型

**示例**:
```c
if (F_is_boolean_type(type_node)) {
    printf("可以进行逻辑运算\n");
}
```

#### `F_is_string_type`
```c
bool F_is_string_type(Stru_AstNode_t* type_node);
```

**描述**: 检查类型是否为字符串类型。

**参数**:
- `type_node`: 类型节点

**返回值**:
- `true`: 是字符串类型
- `false`: 不是字符串类型

**示例**:
```c
if (F_is_string_type(type_node)) {
    printf("可以进行字符串连接操作\n");
}
```

#### `F_is_array_type`
```c
bool F_is_array_type(Stru_AstNode_t* type_node);
```

**描述**: 检查类型是否为数组类型。

**参数**:
- `type_node`: 类型节点

**返回值**:
- `true`: 是数组类型
- `false`: 不是数组类型

**示例**:
```c
if (F_is_array_type(type_node)) {
    printf("可以进行数组索引操作\n");
}
```

#### `F_is_pointer_type`
```c
bool F_is_pointer_type(Stru_AstNode_t* type_node);
```

**描述**: 检查类型是否为指针类型。

**参数**:
- `type_node`: 类型节点

**返回值**:
- `true`: 是指针类型
- `false`: 不是指针类型

**示例**:
```c
if (F_is_pointer_type(type_node)) {
    printf("可以进行指针解引用操作\n");
}
```

### 其他工具函数

#### `F_copy_string`
```c
char* F_copy_string(const char* str);
```

**描述**: 复制字符串，返回新分配的字符串。

**参数**:
- `str`: 要复制的字符串

**返回值**:
- 成功: 新分配的字符串指针
- 失败: NULL（内存分配失败时）

**示例**:
```c
char* original = "Hello, World!";
char* copy = F_copy_string(original);
if (copy != NULL) {
    printf("复制的字符串: %s\n", copy);
    free(copy); // 使用后需要释放内存
}
```

#### `F_format_error_message`
```c
char* F_format_error_message(const char* format, ...);
```

**描述**: 格式化错误信息字符串。

**参数**:
- `format`: 格式字符串
- `...`: 可变参数

**返回值**:
- 成功: 格式化后的字符串指针
- 失败: NULL（内存分配失败时）

**示例**:
```c
char* error_msg = F_format_error_message("语法错误: 第%d行, 第%d列", line, column);
if (error_msg != NULL) {
    printf("%s\n", error_msg);
    free(error_msg); // 使用后需要释放内存
}
```

#### `F_get_current_position`
```c
void F_get_current_position(Stru_ParserInterface_t* interface, size_t* line, size_t* column);
```

**描述**: 获取语法分析器当前位置信息。

**参数**:
- `interface`: 语法分析器接口指针
- `line`: 输出行号（通过指针返回）
- `column`: 输出列号（通过指针返回）

**示例**:
```c
size_t line, column;
F_get_current_position(parser, &line, &column);
printf("当前位置: 行 %zu, 列 %zu\n", line, column);
```

#### `F_is_at_end_of_file`
```c
bool F_is_at_end_of_file(Stru_ParserInterface_t* interface);
```

**描述**: 检查是否到达文件结尾。

**参数**:
- `interface`: 语法分析器接口指针

**返回值**:
- `true`: 到达文件结尾
- `false`: 未到达文件结尾

**示例**:
```c
while (!F_is_at_end_of_file(parser)) {
    Stru_AstNode_t* stmt = F_parse_statement(parser);
    // 处理语句
}
```

## 运算符优先级表

工具模块定义了CN_Language的运算符优先级：

| 优先级 | 运算符 | 结合性 | 描述 | 对应令牌类型 |
|--------|--------|--------|------|-------------|
| 10 | `()` | 左到右 | 括号 | `Eum_TOKEN_LPAREN`, `Eum_TOKEN_RPAREN` |
| 9 | `函数()` | 左到右 | 函数调用 | `Eum_TOKEN_IDENTIFIER` + `Eum_TOKEN_LPAREN` |
| 8 | `-`, `非` | 右到左 | 一元运算符 | `Eum_TOKEN_MINUS`, `Eum_TOKEN_KEYWORD_NOT` |
| 7 | `*`, `/`, `%` | 左到右 | 乘法运算符 | `Eum_TOKEN_STAR`, `Eum_TOKEN_SLASH`, `Eum_TOKEN_PERCENT` |
| 6 | `+`, `-` | 左到右 | 加法运算符 | `Eum_TOKEN_PLUS`, `Eum_TOKEN_MINUS` |
| 5 | `<`, `<=`, `>`, `>=` | 左到右 | 关系运算符 | `Eum_TOKEN_LESS`, `Eum_TOKEN_LESS_EQUAL`, `Eum_TOKEN_GREATER`, `Eum_TOKEN_GREATER_EQUAL` |
| 4 | `==`, `!=` | 左到右 | 相等性运算符 | `Eum_TOKEN_EQUAL_EQUAL`, `Eum_TOKEN_BANG_EQUAL` |
| 3 | `且` | 左到右 | 逻辑与 | `Eum_TOKEN_KEYWORD_AND` |
| 2 | `或` | 左到右 | 逻辑或 | `Eum_TOKEN_KEYWORD_OR` |
| 1 | `=` | 右到左 | 赋值运算符 | `Eum_TOKEN_ASSIGN` |

## 使用示例

### 综合使用示例
```c
#include "core/parser/utils/CN_parser_utils.h"

// 初始化语法分析器
Stru_ParserInterface_t* parser = F_create_parser_interface();
parser->initialize(parser, lexer);

// 检查当前位置
size_t line, column;
F_get_current_position(parser, &line, &column);
printf("开始解析，当前位置: 行 %zu, 列 %zu\n", line, column);

// 解析表达式
while (!F_is_at_end_of_file(parser)) {
    // 检查当前令牌
    Stru_Token_t* token = parser->get_current_token(parser);
    
    if (F_is_keyword_token(token->type)) {
        printf("遇到关键字: %s\n", token->lexeme);
    } else if (F_is_operator_token(token->type)) {
        int precedence = F_get_operator_precedence(token->type);
        bool is_right_assoc = F_is_right_associative(token->type);
        printf("遇到运算符: %s, 优先级: %d, 右结合: %s\n", 
               token->lexeme, precedence, is_right_assoc ? "是" : "否");
    }
    
    // 解析语句
    Stru_AstNode_t* stmt = F_parse_statement(parser);
    if (stmt == NULL) {
        // 处理错误
        char* error_msg = F_format_error_message("解析错误，位置: 行 %zu, 列 %zu", line, column);
        if (error_msg != NULL) {
            printf("%s\n", error_msg);
            free(error_msg);
        }
        break;
    }
    
    // 处理AST节点
    // ...
}

// 清理资源
parser->destroy(parser);
```

### AST构建示例
```c
// 构建一个简单的表达式: a + b * c
Stru_Token_t* a_token = parser->get_current_token(parser); // 假设是标识符"a"
Stru_AstNode_t* a_node = F_create_identifier_node(parser, a_token);

parser->advance_token(parser); // 移动到+

Stru_Token_t* plus_token = parser->get_current_token(parser);

parser->advance_token(parser); // 移动到b

Stru_Token_t* b_token = parser->get_current_token(parser);
Stru_AstNode_t* b_node = F_create_identifier_node(parser, b_token);

parser->advance_token(parser); // 移动到*

Stru_Token_t* star_token = parser->get_current_token(parser);

parser->advance_token(parser); // 移动到c

Stru_Token_t* c_token = parser->get_current_token(parser);
Stru_AstNode_t* c_node = F_create_identifier_node(parser, c_token);

// 先构建乘法表达式: b * c
Stru_AstNode_t* multiply_expr = F_create_binary_expression_node(
    parser, star_token, b_node, c_node);

// 再构建加法表达式: a + (b * c)
Stru_AstNode_t* add_expr = F_create_binary_expression_node(
    parser, plus_token, a_node, multiply_expr);

printf("成功构建表达式AST节点\n");
```

## 设计原理

### 单一职责原则
每个工具函数只负责一个特定的功能：
- 令牌工具函数只处理令牌分类和检查
- AST工具函数只负责AST节点创建
- 类型工具函数只处理类型相关操作
- 其他工具函数提供通用功能

### 无状态设计
工具函数不维护内部状态，所有函数都是纯函数或仅依赖于输入参数：
- 避免副作用
- 提高可测试性
- 支持并发使用

### 错误安全
工具函数进行参数验证和错误处理：
- 检查NULL指针
- 验证输入有效性
- 提供有意义的错误返回值

### 内存管理
- 创建对象的函数负责内存分配
- 调用者负责释放分配的内存
- 使用统一的分配器接口

## 性能考虑

### 函数调用开销
- 工具函数设计为轻量级
- 避免不必要的计算
- 使用内联函数优化频繁调用的函数

### 缓存优化
- 运算符优先级表使用静态数组缓存
- 类型检查结果可以缓存
- 避免重复计算

### 内存管理优化
- 使用对象池重用频繁创建的对象
- 避免频繁的内存分配和释放
- 使用内存区域分配器

## 测试

### 单元测试
工具模块的单元测试位于：
- `tests/core/parser/test_parser_utils.c` - 工具函数测试

### 测试用例类型
1. **令牌工具测试**: 测试令牌分类和运算符优先级
2. **AST工具测试**: 测试AST节点创建函数
3. **类型工具测试**: 测试类型兼容性检查
4. **通用工具测试**: 测试字符串处理和格式化

### 运行测试
```bash
cd tests
make test_parser_utils
./test_parser_utils
```

## 扩展性

### 添加新的工具函数
1. 在`CN_parser_utils.h`头文件中声明新函数
2. 在相应的源文件中实现函数
3. 添加函数文档注释
4. 编写单元测试
5. 更新API文档

### 自定义运算符优先级
1. 修改`F_get_operator_precedence()`函数实现
2. 更新运算符优先级表
3. 测试优先级变化对表达式解析的影响
4. 更新相关文档

### 添加新的AST节点类型
1. 在AST模块中添加新的节点类型定义
2. 在工具模块中添加相应的创建函数
3. 更新类型检查函数
4. 添加测试用例

## 依赖关系

### 内部依赖
- 语法分析器接口 (`../CN_parser_interface.h`)
- 令牌模块 (`../../token/CN_token_types.h`)
- AST模块 (`../../ast/CN_ast.h`)
- 基础设施层容器 (`../../../infrastructure/containers/array/CN_dynamic_array.h`)

### 外部依赖
- C标准库 (`stdio.h`, `stdlib.h`, `string.h`, `stdarg.h`)
- 项目内存管理接口

## 维护信息

### 版本历史
- **1.0.0** (2026-01-08): 初始版本，基于模块化架构设计

### 维护者
CN_Language架构委员会

### 许可证
MIT许可证

### 兼容性
- 向后兼容所有公共API
- 遵循语义化版本控制
- 废弃的API会标记并保留至少两个主版本
