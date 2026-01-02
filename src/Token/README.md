# Token 模块

## 概述

Token模块是CN_Language词法分析器的核心组件，负责定义和操作词法单元（Token）。Token是源代码的最小有意义的单元，如关键字、标识符、字面量、运算符和分隔符。

## 功能特性

- **完整的Token类型定义**：支持CN_Language的所有关键字、运算符、字面量和分隔符
- **内存管理**：提供Token的创建、复制和销毁功能
- **信息查询**：支持Token类型名称、类别、优先级等信息的查询
- **调试支持**：提供Token的打印和字符串表示功能
- **类型检查**：支持Token类型的分类检查（关键字、运算符、字面量、分隔符）

## 文件结构

```
src/Token/
├── CN_token.h      # Token类型定义和接口声明
├── CN_token.c      # Token类型实现
└── README.md       # 模块文档（本文件）
```

## API 文档

### 数据结构

#### `enum Eum_TokenType`
Token类型枚举，定义了CN_Language中所有可能的Token类型。

主要分类：
- **关键字**：`Eum_TOKEN_KEYWORD_VARIABLE` 到 `Eum_TOKEN_KEYWORD_FINALLY`
- **标识符和字面量**：`Eum_TOKEN_IDENTIFIER`、`Eum_TOKEN_INTEGER_LITERAL`等
- **运算符**：`Eum_TOKEN_OPERATOR_PLUS` 到 `Eum_TOKEN_OPERATOR_GREATER_EQUAL`
- **分隔符**：`Eum_TOKEN_SEMICOLON` 到 `Eum_TOKEN_RIGHT_BRACKET`
- **特殊Token**：`Eum_TOKEN_EOF`、`Eum_TOKEN_ERROR`等

#### `struct Stru_Token_t`
Token结构体，包含以下字段：
- `type`：Token类型（`enum Eum_TokenType`）
- `lexeme`：Token的原始字符串表示
- `lexeme_length`：字符串长度
- `line_number`：所在行号（从1开始）
- `column_number`：所在列号（从1开始）
- `literal_value`：字面量值（仅对字面量Token有效）

### 核心函数

#### `F_token_create`
```c
Stru_Token_t* F_token_create(enum Eum_TokenType type, const char* lexeme, 
                             size_t lexeme_length, int line_number, int column_number);
```
创建新的Token。

**参数**：
- `type`：Token类型
- `lexeme`：Token的原始字符串（会被复制）
- `lexeme_length`：字符串长度
- `line_number`：行号
- `column_number`：列号

**返回值**：新创建的Token指针，失败返回NULL

#### `F_token_destroy`
```c
void F_token_destroy(Stru_Token_t* token);
```
销毁Token，释放所有分配的内存。

#### `F_token_copy`
```c
Stru_Token_t* F_token_copy(const Stru_Token_t* src);
```
复制Token，创建新的Token副本。

#### `F_token_type_to_string`
```c
const char* F_token_type_to_string(enum Eum_TokenType type);
```
获取Token类型的可读字符串表示。

#### `F_token_get_category`
```c
const char* F_token_get_category(enum Eum_TokenType type);
```
获取Token类别（"关键字"、"运算符"、"字面量"、"分隔符"等）。

#### `F_token_print`
```c
void F_token_print(const Stru_Token_t* token, bool show_position, bool show_literal_value);
```
打印Token信息，支持显示位置信息和字面量值。

### 类型检查函数

#### `F_token_is_keyword`
```c
bool F_token_is_keyword(enum Eum_TokenType type);
```
检查Token是否为关键字。

#### `F_token_is_operator`
```c
bool F_token_is_operator(enum Eum_TokenType type);
```
检查Token是否为运算符。

#### `F_token_is_literal`
```c
bool F_token_is_literal(enum Eum_TokenType type);
```
检查Token是否为字面量。

#### `F_token_is_separator`
```c
bool F_token_is_separator(enum Eum_TokenType type);
```
检查Token是否为分隔符。

### 运算符相关函数

#### `F_token_get_precedence`
```c
int F_token_get_precedence(enum Eum_TokenType type);
```
获取运算符的优先级（值越小优先级越高）。

#### `F_token_get_associativity`
```c
int F_token_get_associativity(enum Eum_TokenType type);
```
获取运算符的结合性（0表示左结合，1表示右结合）。

## 使用示例

### 示例1：创建和打印Token
```c
#include "CN_token.h"

int main()
{
    // 创建关键字Token
    Stru_Token_t* token = F_token_create(Eum_TOKEN_KEYWORD_VARIABLE, "变量", 6, 1, 1);
    
    if (token != NULL)
    {
        // 打印Token信息
        F_token_print(token, true, false);
        
        // 销毁Token
        F_token_destroy(token);
    }
    
    return 0;
}
```

### 示例2：检查Token类型
```c
#include "CN_token.h"
#include <stdio.h>

void check_token_type(enum Eum_TokenType type)
{
    if (F_token_is_keyword(type))
    {
        printf("这是关键字: %s\n", F_token_type_to_string(type));
    }
    else if (F_token_is_operator(type))
    {
        printf("这是运算符: %s, 优先级: %d\n", 
               F_token_type_to_string(type), 
               F_token_get_precedence(type));
    }
    else if (F_token_is_literal(type))
    {
        printf("这是字面量: %s\n", F_token_type_to_string(type));
    }
}

int main()
{
    check_token_type(Eum_TOKEN_KEYWORD_IF);      // 输出: 这是关键字: 如果
    check_token_type(Eum_TOKEN_OPERATOR_PLUS);   // 输出: 这是运算符: +, 优先级: 4
    check_token_type(Eum_TOKEN_INTEGER_LITERAL); // 输出: 这是字面量: 整数字面量
    
    return 0;
}
```

### 示例3：比较Token
```c
#include "CN_token.h"
#include <stdio.h>

int main()
{
    // 创建两个相同的Token
    Stru_Token_t* token1 = F_token_create(Eum_TOKEN_IDENTIFIER, "变量名", 9, 1, 1);
    Stru_Token_t* token2 = F_token_create(Eum_TOKEN_IDENTIFIER, "变量名", 9, 1, 1);
    
    if (token1 != NULL && token2 != NULL)
    {
        if (F_token_equals(token1, token2))
        {
            printf("两个Token相等\n");
        }
        else
        {
            printf("两个Token不相等\n");
        }
    }
    
    F_token_destroy(token1);
    F_token_destroy(token2);
    
    return 0;
}
```

## 设计原则

### 单一职责原则
Token模块专注于Token的定义和基本操作，不涉及词法分析的具体逻辑。

### 内存安全
所有创建的函数都返回动态分配的内存，调用者负责使用`F_token_destroy`释放内存。

### 错误处理
函数在失败时返回NULL或适当的值，调用者应检查返回值。

### 可扩展性
Token类型枚举设计为可扩展，可以方便地添加新的Token类型。

## 集成指南

### 包含头文件
```c
#include "CN_token.h"
```

### 编译选项
Token模块需要C标准库的支持，编译时应包含以下库：
- `stdlib.h`：内存分配
- `string.h`：字符串操作
- `stdio.h`：调试输出

### 依赖关系
Token模块是独立的，不依赖项目中的其他模块。

## 测试建议

### 单元测试
建议为以下功能编写单元测试：
1. Token的创建和销毁
2. Token的复制和比较
3. Token类型检查和分类
4. 运算符优先级和结合性

### 集成测试
在词法分析器中集成Token模块，测试：
1. 各种Token类型的正确识别
2. 位置信息的正确记录
3. 内存管理的正确性

## 性能考虑

### 内存使用
- Token结构体包含动态分配的字符串，使用后应及时释放
- 频繁创建和销毁Token时，考虑使用对象池技术

### 字符串处理
- `lexeme`字符串在创建时被复制，避免原始字符串被修改
- 对于频繁使用的Token（如关键字），考虑使用字符串池

## 维护说明

### 添加新的Token类型
1. 在`CN_token.h`的`enum Eum_TokenType`中添加新的枚举值
2. 在`CN_token.c`的`F_token_type_to_string`函数中添加对应的字符串表示
3. 根据需要更新类型检查函数

### 修改现有功能
1. 确保向后兼容性
2. 更新相关文档
3. 运行现有测试确保功能正常

## 版本历史

### v1.0.0 (2026-01-02)
- 初始版本
- 定义完整的Token类型枚举
- 实现Token的创建、销毁、复制功能
- 实现Token信息查询和打印功能
- 实现运算符优先级和结合性查询

## 许可证

本模块遵循MIT许可证，详见项目根目录的LICENSE文件。

## 贡献指南

欢迎提交Issue和Pull Request来改进Token模块。在提交更改前，请确保：
1. 代码符合项目编码规范
2. 添加或更新相应的测试
3. 更新相关文档
