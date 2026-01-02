# CN_Language Token模块 API 文档

## 概述

本文档详细描述了CN_Language Token模块的所有API接口。Token模块提供了完整的Token类型定义、创建、销毁、查询和操作功能。

## 头文件

```c
#include "CN_token.h"
```

## 数据类型

### 枚举类型

#### Eum_TokenType

Token类型枚举，定义了CN_Language中所有可能的Token类型。

```c
enum Eum_TokenType
{
    // 关键字类型（70+种）
    Eum_TOKEN_KEYWORD_VARIABLE,      // 变量
    Eum_TOKEN_KEYWORD_INTEGER,       // 整数
    Eum_TOKEN_KEYWORD_FLOAT,         // 小数
    Eum_TOKEN_KEYWORD_STRING,        // 字符串
    Eum_TOKEN_KEYWORD_BOOLEAN,       // 布尔
    Eum_TOKEN_KEYWORD_ARRAY,         // 数组
    Eum_TOKEN_KEYWORD_STRUCT,        // 结构体
    Eum_TOKEN_KEYWORD_ENUM,          // 枚举
    Eum_TOKEN_KEYWORD_POINTER,       // 指针
    Eum_TOKEN_KEYWORD_REFERENCE,     // 引用
    // ... 更多关键字类型
    
    // 标识符和字面量
    Eum_TOKEN_IDENTIFIER,            // 标识符
    Eum_TOKEN_INTEGER_LITERAL,       // 整数字面量
    Eum_TOKEN_FLOAT_LITERAL,         // 小数字面量
    Eum_TOKEN_STRING_LITERAL,        // 字符串字面量
    Eum_TOKEN_BOOLEAN_LITERAL,       // 布尔字面量
    
    // 运算符
    Eum_TOKEN_OPERATOR_PLUS,         // +
    Eum_TOKEN_OPERATOR_MINUS,        // -
    Eum_TOKEN_OPERATOR_MULTIPLY,     // *
    Eum_TOKEN_OPERATOR_DIVIDE,       // /
    Eum_TOKEN_OPERATOR_MODULO,       // %
    Eum_TOKEN_OPERATOR_ASSIGN,       // =
    Eum_TOKEN_OPERATOR_EQUAL,        // ==
    Eum_TOKEN_OPERATOR_NOT_EQUAL,    // !=
    Eum_TOKEN_OPERATOR_LESS,         // <
    Eum_TOKEN_OPERATOR_GREATER,      // >
    Eum_TOKEN_OPERATOR_LESS_EQUAL,   // <=
    Eum_TOKEN_OPERATOR_GREATER_EQUAL,// >=
    
    // 分隔符
    Eum_TOKEN_SEMICOLON,             // ;
    Eum_TOKEN_COMMA,                 // ,
    Eum_TOKEN_DOT,                   // .
    Eum_TOKEN_COLON,                 // :
    Eum_TOKEN_LEFT_PAREN,            // (
    Eum_TOKEN_RIGHT_PAREN,           // )
    Eum_TOKEN_LEFT_BRACE,            // {
    Eum_TOKEN_RIGHT_BRACE,           // }
    Eum_TOKEN_LEFT_BRACKET,          // [
    Eum_TOKEN_RIGHT_BRACKET,         // ]
    
    // 特殊Token
    Eum_TOKEN_EOF,                   // 文件结束
    Eum_TOKEN_ERROR,                 // 错误Token
    Eum_TOKEN_COMMENT,               // 注释
    Eum_TOKEN_WHITESPACE,            // 空白字符
    Eum_TOKEN_NEWLINE                // 换行符
};
```

### 结构体类型

#### Stru_Token_t

Token结构体，表示一个词法单元。

```c
typedef struct Stru_Token_t
{
    enum Eum_TokenType type;         ///< Token类型
    char* lexeme;                    ///< Token的原始字符串表示
    size_t lexeme_length;            ///< 字符串长度
    int line_number;                 ///< 所在行号（从1开始）
    int column_number;               ///< 所在列号（从1开始）
    union
    {
        long integer_value;          ///< 整数值（如果Token是整数）
        double float_value;          ///< 浮点数值（如果Token是小数）
        bool boolean_value;          ///< 布尔值（如果Token是布尔）
    } literal_value;                 ///< 字面量值（仅对字面量Token有效）
} Stru_Token_t;
```

#### Stru_TokenPosition_t

Token位置信息结构体，用于错误报告和调试。

```c
typedef struct Stru_TokenPosition_t
{
    int line_number;                 ///< 行号（从1开始）
    int column_number;               ///< 列号（从1开始）
    size_t offset;                   ///< 在源代码中的字节偏移量
    const char* filename;            ///< 源文件名
} Stru_TokenPosition_t;
```

## 函数参考

### Token创建和销毁函数

#### F_token_create

创建新的Token。

```c
Stru_Token_t* F_token_create(enum Eum_TokenType type, const char* lexeme, 
                             size_t lexeme_length, int line_number, int column_number);
```

**参数**:
- `type`: Token类型
- `lexeme`: Token的原始字符串（会被复制），可以为NULL
- `lexeme_length`: 字符串长度（字节数），如果为0且lexeme不为NULL则自动计算
- `line_number`: 行号（从1开始）
- `column_number`: 列号（从1开始）

**返回值**:
- 成功: 新创建的Token指针
- 失败: NULL（参数错误或内存不足）

**说明**:
- 调用者负责使用`F_token_destroy()`释放返回的Token
- 对lexeme字符串进行深拷贝
- 如果lexeme为NULL，则lexeme_length必须为0

#### F_token_create_literal

创建字面量Token。

```c
Stru_Token_t* F_token_create_literal(enum Eum_TokenType type, const char* lexeme,
                                     size_t lexeme_length, int line_number, 
                                     int column_number, long value);
```

**参数**:
- `type`: 字面量类型（必须是`Eum_TOKEN_INTEGER_LITERAL`、`Eum_TOKEN_FLOAT_LITERAL`、
          `Eum_TOKEN_BOOLEAN_LITERAL`或`Eum_TOKEN_STRING_LITERAL`之一）
- `lexeme`: Token的原始字符串表示
- `lexeme_length`: 字符串长度（字节数）
- `line_number`: 行号（从1开始）
- `column_number`: 列号（从1开始）
- `value`: 字面量值（根据类型不同，使用不同的union成员）

**返回值**:
- 成功: 新创建的Token指针
- 失败: NULL（参数错误或内存不足）

**说明**:
- 专门用于创建字面量Token
- 设置literal_value字段为提供的值
- 调用者负责使用`F_token_destroy()`释放返回的Token

#### F_token_destroy

销毁Token。

```c
void F_token_destroy(Stru_Token_t* token);
```

**参数**:
- `token`: 要销毁的Token指针

**返回值**: 无

**说明**:
- 对NULL参数安全
- 释放Token内部分配的所有内存
- 调用后token指针变为无效，不应再使用

#### F_token_copy

复制Token。

```c
Stru_Token_t* F_token_copy(const Stru_Token_t* src);
```

**参数**:
- `src`: 源Token

**返回值**:
- 成功: 新创建的Token副本
- 失败: NULL（参数错误或内存不足）

**说明**:
- 创建深拷贝，包括lexeme字符串
- 调用者负责使用`F_token_destroy()`释放返回的Token
- 对NULL参数安全，返回NULL

### Token信息查询函数

#### F_token_type_to_string

获取Token类型名称。

```c
const char* F_token_type_to_string(enum Eum_TokenType type);
```

**参数**:
- `type`: Token类型枚举值

**返回值**:
- Token类型名称字符串（静态常量，不需要释放）
- 未知类型返回"未知Token类型"

**说明**:
- 返回中文类型名称
- 用于调试和显示

#### F_token_get_category

获取Token类别。

```c
const char* F_token_get_category(enum Eum_TokenType type);
```

**参数**:
- `type`: Token类型

**返回值**:
- 类别字符串（"关键字"、"标识符"、"字面量"、"运算符"、"分隔符"、"文件结束"、"错误"、"注释"、"空白字符"、"未知"）

**说明**:
- 将Token分类为更通用的类别
- 用于语法分析和错误报告

#### F_token_is_keyword

检查Token是否为关键字。

```c
bool F_token_is_keyword(enum Eum_TokenType type);
```

**参数**:
- `type`: Token类型

**返回值**:
- 如果是关键字: true
- 如果不是关键字: false

#### F_token_is_operator

检查Token是否为运算符。

```c
bool F_token_is_operator(enum Eum_TokenType type);
```

**参数**:
- `type`: Token类型

**返回值**:
- 如果是运算符: true
- 如果不是运算符: false

#### F_token_is_literal

检查Token是否为字面量。

```c
bool F_token_is_literal(enum Eum_TokenType type);
```

**参数**:
- `type`: Token类型

**返回值**:
- 如果是字面量: true
- 如果不是字面量: false

#### F_token_is_separator

检查Token是否为分隔符。

```c
bool F_token_is_separator(enum Eum_TokenType type);
```

**参数**:
- `type`: Token类型

**返回值**:
- 如果是分隔符: true
- 如果不是分隔符: false

### Token操作函数

#### F_token_print

打印Token信息。

```c
void F_token_print(const Stru_Token_t* token, bool show_position, bool show_literal_value);
```

**参数**:
- `token`: 要打印的Token
- `show_position`: 是否显示位置信息
- `show_literal_value`: 是否显示字面量值

**返回值**: 无

**说明**:
- 对NULL参数安全
- 输出到标准输出
- 格式化显示Token信息

#### F_token_to_string

获取Token的字符串表示。

```c
size_t F_token_to_string(const Stru_Token_t* token, char* buffer, size_t buffer_size);
```

**参数**:
- `token`: Token
- `buffer`: 输出缓冲区
- `buffer_size`: 缓冲区大小

**返回值**:
- 成功: 实际写入的字符数（不包括结尾的null字符）
- 失败: 0（参数错误或缓冲区不足）

**说明**:
- 将Token信息格式化为字符串
- 如果buffer为NULL或buffer_size为0，返回所需缓冲区大小
- 保证字符串以null结尾（如果缓冲区足够）

#### F_token_equals

比较两个Token是否相等。

```c
bool F_token_equals(const Stru_Token_t* token1, const Stru_Token_t* token2);
```

**参数**:
- `token1`: 第一个Token
- `token2`: 第二个Token

**返回值**:
- 相等: true
- 不相等: false
- 参数错误: false

**说明**:
- 比较Token类型、lexeme字符串、位置信息和字面量值
- 对NULL参数安全
- 如果两个指针相同，直接返回true

### 运算符属性函数

#### F_token_get_precedence

获取Token的优先级（仅对运算符有效）。

```c
int F_token_get_precedence(enum Eum_TokenType type);
```

**参数**:
- `type`: Token类型（必须是运算符类型）

**返回值**:
- 优先级值（值越小优先级越高）
- 如果不是运算符: -1

**说明**:
- 用于语法分析中的运算符优先级解析
- 支持常见的运算符优先级规则

#### F_token_get_associativity

获取Token的结合性。

```c
int F_token_get_associativity(enum Eum_TokenType type);
```

**参数**:
- `type`: Token类型（必须是运算符类型）

**返回值**:
- 结合性：0表示左结合，1表示右结合
- 如果不是运算符: -1

**说明**:
- 用于语法分析中的运算符结合性解析
- 大多数运算符是左结合，赋值运算符是右结合

## 使用示例

### 示例1：基本使用

```c
#include <stdio.h>
#include "CN_token.h"

int main()
{
    // 创建标识符Token
    Stru_Token_t* token = F_token_create(Eum_TOKEN_IDENTIFIER, "变量名", 6, 1, 1);
    if (token == NULL) {
        printf("创建Token失败\n");
        return 1;
    }
    
    // 打印Token信息
    F_token_print(token, true, false);
    
    // 获取Token信息
    printf("类型: %s\n", F_token_type_to_string(token->type));
    printf("类别: %s\n", F_token_get_category(token->type));
    
    // 销毁Token
    F_token_destroy(token);
    
    return 0;
}
```

### 示例2：字面量Token

```c
#include <stdio.h>
#include "CN_token.h"

int main()
{
    // 创建整数字面量Token
    Stru_Token_t* int_token = F_token_create_literal(
        Eum_TOKEN_INTEGER_LITERAL, "123", 3, 1, 1, 123);
    
    if (int_token != NULL) {
        F_token_print(int_token, true, true);
        F_token_destroy(int_token);
    }
    
    return 0;
}
```

### 示例3：Token比较

```c
#include <stdio.h>
#include "CN_token.h"

int main()
{
    // 创建两个相同的Token
    Stru_Token_t* token1 = F_token_create(Eum_TOKEN_KEYWORD_IF, "如果", 6, 1, 1);
    Stru_Token_t* token2 = F_token_create(Eum_TOKEN_KEYWORD_IF, "如果", 6, 1, 1);
    
    if (token1 != NULL && token2 != NULL) {
        if (F_token_equals(token1, token2)) {
            printf("两个Token相等\n");
        } else {
            printf("两个Token不相等\n");
        }
    }
    
    if (token1 != NULL) F_token_destroy(token1);
    if (token2 != NULL) F_token_destroy(token2);
    
    return 0;
}
```

### 示例4：运算符优先级

```c
#include <stdio.h>
#include "CN_token.h"

int main()
{
    enum Eum_TokenType operators[] = {
        Eum_TOKEN_OPERATOR_PLUS,
        Eum_TOKEN_OPERATOR_MULTIPLY,
        Eum_TOKEN_OPERATOR_ASSIGN,
        Eum_TOKEN_OPERATOR_EQUAL
    };
    
    const char* operator_names[] = {"+", "*", "=", "=="};
    int count = sizeof(operators) / sizeof(operators[0]);
    
    for (int i = 0; i < count; i++) {
        int precedence = F_token_get_precedence(operators[i]);
        int associativity = F_token_get_associativity(operators[i]);
        
        printf("运算符 %s: 优先级=%d, 结合性=%s\n",
               operator_names[i],
               precedence,
               associativity == 0 ? "左结合" : 
               associativity == 1 ? "右结合" : "不是运算符");
    }
    
    return 0;
}
```

## 错误处理

所有函数都提供适当的错误处理：

1. **创建函数**：失败时返回NULL
2. **查询函数**：对无效参数返回安全值
3. **操作函数**：对NULL参数安全
4. **内存管理**：所有动态分配的内存都有对应的释放函数

## 内存管理

- 使用`F_token_create()`和`F_token_create_literal()`创建Token
- 使用`F_token_destroy()`释放Token
- 使用`F_token_copy()`创建深拷贝，需要单独销毁
- 不要手动释放Token结构体中的字符串指针

## 线程安全

当前版本不是线程安全的。如果需要在多线程环境中使用，需要外部同步。

## 版本历史

- v1.0.0 (2026-01-02): 初始版本
  - 完整的Token类型定义
  - Token创建、销毁和复制功能
  - Token信息查询和操作功能
  - 运算符优先级和结合性支持

## 相关文档

- [README.md](README.md) - 模块概述和使用指南
- [EXAMPLES.md](EXAMPLES.md) - 更多使用示例
- [DESIGN.md](DESIGN.md) - 设计原理和架构说明
