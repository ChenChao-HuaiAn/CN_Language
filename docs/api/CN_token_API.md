# Token 模块 API 文档

## 概述

本文档详细描述了Token模块的所有公共接口。Token模块提供了词法单元（Token）的定义、创建、操作和查询功能。

## 数据结构

### 枚举类型

#### `enum Eum_TokenType`
Token类型枚举，定义了CN_Language中所有可能的Token类型。

```c
enum Eum_TokenType
{
    // 关键字类型（70个预定义中文关键字）
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
    Eum_TOKEN_IDENTIFIER,            // 标识符（变量名、函数名等）
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
    
    // 分隔符和标点符号
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

#### `struct Stru_Token_t`
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

#### `struct Stru_TokenPosition_t`
Token位置信息结构体，用于记录Token在源代码中的位置。

```c
typedef struct Stru_TokenPosition_t
{
    int line_number;                 ///< 行号（从1开始）
    int column_number;               ///< 列号（从1开始）
    size_t offset;                   ///< 在源代码中的字节偏移量
    const char* filename;            ///< 源文件名
} Stru_TokenPosition_t;
```

## 函数接口

### Token生命周期管理

#### `F_token_create`
创建新的Token。

```c
Stru_Token_t* F_token_create(enum Eum_TokenType type, const char* lexeme, 
                             size_t lexeme_length, int line_number, int column_number);
```

**参数**：
- `type`: Token类型
- `lexeme`: Token的原始字符串（会被复制）
- `lexeme_length`: 字符串长度
- `line_number`: 行号（从1开始）
- `column_number`: 列号（从1开始）

**返回值**：新创建的Token指针，失败返回NULL

**注意**：调用者负责使用`F_token_destroy`释放返回的Token

#### `F_token_destroy`
销毁Token，释放所有分配的内存。

```c
void F_token_destroy(Stru_Token_t* token);
```

**参数**：
- `token`: 要销毁的Token指针

**注意**：此函数会释放Token内部分配的所有内存，并将指针设为NULL

#### `F_token_copy`
复制Token，创建新的Token副本。

```c
Stru_Token_t* F_token_copy(const Stru_Token_t* src);
```

**参数**：
- `src`: 源Token

**返回值**：新创建的Token副本，失败返回NULL

### Token信息查询

#### `F_token_type_to_string`
获取Token类型的可读字符串表示。

```c
const char* F_token_type_to_string(enum Eum_TokenType type);
```

**参数**：
- `type`: Token类型枚举值

**返回值**：类型名称字符串（静态常量，不需要释放）

#### `F_token_get_category`
获取Token类别。

```c
const char* F_token_get_category(enum Eum_TokenType type);
```

**参数**：
- `type`: Token类型

**返回值**：类别字符串（"关键字"、"标识符"、"字面量"、"运算符"、"分隔符"等）

#### `F_token_print`
打印Token信息。

```c
void F_token_print(const Stru_Token_t* token, bool show_position, bool show_literal_value);
```

**参数**：
- `token`: 要打印的Token
- `show_position`: 是否显示位置信息
- `show_literal_value`: 是否显示字面量值

#### `F_token_to_string`
获取Token的字符串表示。

```c
size_t F_token_to_string(const Stru_Token_t* token, char* buffer, size_t buffer_size);
```

**参数**：
- `token`: Token
- `buffer`: 输出缓冲区
- `buffer_size`: 缓冲区大小

**返回值**：实际写入的字符数（不包括结尾的null字符）

### Token类型检查

#### `F_token_is_keyword`
检查Token是否为关键字。

```c
bool F_token_is_keyword(enum Eum_TokenType type);
```

**参数**：
- `type`: Token类型

**返回值**：如果是关键字返回true，否则返回false

#### `F_token_is_operator`
检查Token是否为运算符。

```c
bool F_token_is_operator(enum Eum_TokenType type);
```

**参数**：
- `type`: Token类型

**返回值**：如果是运算符返回true，否则返回false

#### `F_token_is_literal`
检查Token是否为字面量。

```c
bool F_token_is_literal(enum Eum_TokenType type);
```

**参数**：
- `type`: Token类型

**返回值**：如果是字面量返回true，否则返回false

#### `F_token_is_separator`
检查Token是否为分隔符。

```c
bool F_token_is_separator(enum Eum_TokenType type);
```

**参数**：
- `type`: Token类型

**返回值**：如果是分隔符返回true，否则返回false

### Token比较

#### `F_token_equals`
比较两个Token是否相等。

```c
bool F_token_equals(const Stru_Token_t* token1, const Stru_Token_t* token2);
```

**参数**：
- `token1`: 第一个Token
- `token2`: 第二个Token

**返回值**：如果两个Token类型和值都相等返回true，否则返回false

### 运算符相关函数

#### `F_token_get_precedence`
获取Token的优先级（仅对运算符有效）。

```c
int F_token_get_precedence(enum Eum_TokenType type);
```

**参数**：
- `type`: Token类型（必须是运算符类型）

**返回值**：优先级值（值越小优先级越高），如果不是运算符返回-1

#### `F_token_get_associativity`
获取Token的结合性。

```c
int F_token_get_associativity(enum Eum_TokenType type);
```

**参数**：
- `type`: Token类型（必须是运算符类型）

**返回值**：结合性：0表示左结合，1表示右结合，-1表示不是运算符

## 错误处理

所有函数都遵循以下错误处理原则：

1. **内存分配失败**：返回NULL或适当的值
2. **无效参数**：返回默认值或错误指示
3. **边界检查**：进行必要的参数验证

## 线程安全性

Token模块的函数不是线程安全的。如果需要在多线程环境中使用，调用者需要提供适当的同步机制。

## 内存管理

1. `F_token_create`返回的Token需要调用`F_token_destroy`释放
2. `F_token_copy`返回的Token副本也需要调用`F_token_destroy`释放
3. 所有查询函数返回的字符串都是静态常量，不需要释放

## 版本历史

### v1.0.0 (2026-01-02)
- 初始版本
- 定义完整的Token类型枚举
- 实现Token的创建、销毁、复制功能
- 实现Token信息查询和打印功能
- 实现运算符优先级和结合性查询

## 相关文档

- [README.md](./README.md) - 模块概述和使用说明
- [EXAMPLES.md](./EXAMPLES.md) - 使用示例
- [DESIGN.md](./DESIGN.md) - 设计原理
