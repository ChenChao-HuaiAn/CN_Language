# CN_Language 令牌模块 API 参考

## 概述

本文档提供CN_Language令牌模块的完整API参考，包括传统API和模块化接口API。

## 传统API（向后兼容）

### 令牌生命周期管理

#### F_create_token
创建新令牌。

```c
Stru_Token_t* F_create_token(Eum_TokenType type, const char* lexeme, size_t line, size_t column);
```

**参数：**
- `type`: 令牌类型
- `lexeme`: 词素字符串（会被复制）
- `line`: 行号（从1开始）
- `column`: 列号（从1开始）

**返回值：**
- 成功：新创建的令牌指针
- 失败：NULL（内存不足）

**示例：**
```c
Stru_Token_t* token = F_create_token(Eum_TOKEN_KEYWORD_VAR, "变量", 1, 1);
```

#### F_destroy_token
销毁令牌，释放所有相关资源。

```c
void F_destroy_token(Stru_Token_t* token);
```

**参数：**
- `token`: 要销毁的令牌指针

**注意事项：**
- 如果令牌为NULL，函数不执行任何操作
- 会释放令牌的所有内存，包括词素字符串

#### F_token_copy
复制令牌，创建深拷贝。

```c
Stru_Token_t* F_token_copy(const Stru_Token_t* token);
```

**参数：**
- `token`: 要复制的源令牌指针

**返回值：**
- 成功：新复制的令牌指针
- 失败：NULL

### 字面量值操作

#### F_token_set_int_value
设置整数字面量值。

```c
void F_token_set_int_value(Stru_Token_t* token, long value);
```

**适用类型：** `Eum_TOKEN_LITERAL_INTEGER`

#### F_token_set_float_value
设置浮点数字面量值。

```c
void F_token_set_float_value(Stru_Token_t* token, double value);
```

**适用类型：** `Eum_TOKEN_LITERAL_FLOAT`

#### F_token_set_bool_value
设置布尔字面量值。

```c
void F_token_set_bool_value(Stru_Token_t* token, bool value);
```

**适用类型：** `Eum_TOKEN_LITERAL_BOOLEAN`

#### F_token_get_int_value
获取整数字面量值。

```c
long F_token_get_int_value(const Stru_Token_t* token);
```

#### F_token_get_float_value
获取浮点数字面量值。

```c
double F_token_get_float_value(const Stru_Token_t* token);
```

#### F_token_get_bool_value
获取布尔字面量值。

```c
bool F_token_get_bool_value(const Stru_Token_t* token);
```

### 类型查询和分类

#### F_token_is_keyword
判断是否为关键字令牌。

```c
bool F_token_is_keyword(Eum_TokenType type);
```

**返回值：**
- `true`: 是关键字
- `false`: 不是关键字

#### F_token_is_operator
判断是否为运算符令牌。

```c
bool F_token_is_operator(Eum_TokenType type);
```

#### F_token_is_literal
判断是否为字面量令牌。

```c
bool F_token_is_literal(Eum_TokenType type);
```

#### F_token_is_delimiter
判断是否为分隔符令牌。

```c
bool F_token_is_delimiter(Eum_TokenType type);
```

#### F_token_get_precedence
获取关键字优先级。

```c
int F_token_get_precedence(Eum_TokenType type);
```

**返回值：**
- 1-10: 运算符优先级（值越小优先级越高）
- 0: 非运算符关键字

#### F_token_get_chinese_keyword
获取关键字的中文表示。

```c
const char* F_token_get_chinese_keyword(Eum_TokenType type);
```

#### F_token_get_english_keyword
获取关键字的英文表示。

```c
const char* F_token_get_english_keyword(Eum_TokenType type);
```

#### F_token_get_keyword_category
获取关键字的分类。

```c
int F_token_get_keyword_category(Eum_TokenType type);
```

**返回值：**
- 1-9: 关键字分类编号
- 0: 非关键字

### 工具函数

#### F_token_type_to_string
令牌类型转字符串。

```c
const char* F_token_type_to_string(Eum_TokenType type);
```

#### F_token_equals
比较两个令牌。

```c
bool F_token_equals(const Stru_Token_t* token1, const Stru_Token_t* token2);
```

**比较规则：**
1. 类型必须相同
2. 词素必须相同（字符串比较）
3. 字面量值必须相同（根据类型）
4. 位置信息不参与比较

#### F_token_to_string
将令牌转换为可读的字符串表示。

```c
int F_token_to_string(const Stru_Token_t* token, char* buffer, size_t buffer_size);
```

**参数：**
- `token`: 令牌指针
- `buffer`: 输出缓冲区
- `buffer_size`: 缓冲区大小

**返回值：**
- 成功：写入的字符数（不包括终止空字符）
- 失败：-1

## 模块化接口API

### 获取接口实例

#### F_get_token_interface
获取默认令牌接口实例。

```c
const Stru_TokenInterface_t* F_get_token_interface(void);
```

**返回值：** 令牌接口实例指针

### 接口函数

#### create_token
创建令牌。

```c
Stru_Token_t* (*create_token)(Eum_TokenType type, const char* lexeme, size_t line, size_t column);
```

#### destroy_token
销毁令牌。

```c
void (*destroy_token)(Stru_Token_t* token);
```

#### copy_token
复制令牌。

```c
Stru_Token_t* (*copy_token)(const Stru_Token_t* token);
```

#### get_token_type
获取令牌类型。

```c
Eum_TokenType (*get_token_type)(const Stru_Token_t* token);
```

#### get_token_lexeme
获取令牌词素。

```c
const char* (*get_token_lexeme)(const Stru_Token_t* token);
```

#### get_token_position
获取令牌位置。

```c
void (*get_token_position)(const Stru_Token_t* token, size_t* line, size_t* column);
```

#### set_int_value
设置整数字面量值。

```c
void (*set_int_value)(Stru_Token_t* token, long value);
```

#### set_float_value
设置浮点数字面量值。

```c
void (*set_float_value)(Stru_Token_t* token, double value);
```

#### set_bool_value
设置布尔字面量值。

```c
void (*set_bool_value)(Stru_Token_t* token, bool value);
```

#### get_int_value
获取整数字面量值。

```c
long (*get_int_value)(const Stru_Token_t* token);
```

#### get_float_value
获取浮点数字面量值。

```c
double (*get_float_value)(const Stru_Token_t* token);
```

#### get_bool_value
获取布尔字面量值。

```c
bool (*get_bool_value)(const Stru_Token_t* token);
```

#### is_keyword
判断是否为关键字令牌。

```c
bool (*is_keyword)(Eum_TokenType type);
```

#### is_operator
判断是否为运算符令牌。

```c
bool (*is_operator)(Eum_TokenType type);
```

#### is_literal
判断是否为字面量令牌。

```c
bool (*is_literal)(Eum_TokenType type);
```

#### is_delimiter
判断是否为分隔符令牌。

```c
bool (*is_delimiter)(Eum_TokenType type);
```

#### get_precedence
获取关键字优先级。

```c
int (*get_precedence)(Eum_TokenType type);
```

#### get_chinese_keyword
获取关键字的中文表示。

```c
const char* (*get_chinese_keyword)(Eum_TokenType type);
```

#### get_english_keyword
获取关键字的英文表示。

```c
const char* (*get_english_keyword)(Eum_TokenType type);
```

#### get_keyword_category
获取关键字的分类。

```c
int (*get_keyword_category)(Eum_TokenType type);
```

#### type_to_string
令牌类型转字符串。

```c
const char* (*type_to_string)(Eum_TokenType type);
```

#### equals
比较两个令牌。

```c
bool (*equals)(const Stru_Token_t* token1, const Stru_Token_t* token2);
```

#### to_string
打印令牌信息。

```c
int (*to_string)(const Stru_Token_t* token, char* buffer, size_t buffer_size);
```

#### initialize
初始化令牌模块。

```c
bool (*initialize)(void);
```

#### cleanup
清理令牌模块。

```c
void (*cleanup)(void);
```

## 子模块API

### 生命周期管理模块 (lifecycle)

#### F_token_lifecycle_create
创建新令牌。

```c
Stru_Token_t* F_token_lifecycle_create(Eum_TokenType type, const char* lexeme, size_t line, size_t column);
```

#### F_token_lifecycle_destroy
销毁令牌。

```c
void F_token_lifecycle_destroy(Stru_Token_t* token);
```

#### F_token_lifecycle_copy
复制令牌。

```c
Stru_Token_t* F_token_lifecycle_copy(const Stru_Token_t* token);
```

#### F_token_lifecycle_equals
比较两个令牌。

```c
bool F_token_lifecycle_equals(const Stru_Token_t* token1, const Stru_Token_t* token2);
```

### 字面量值操作模块 (values)

#### F_token_values_set_int
设置整数字面量值。

```c
void F_token_values_set_int(Stru_Token_t* token, long value);
```

#### F_token_values_set_float
设置浮点数字面量值。

```c
void F_token_values_set_float(Stru_Token_t* token, double value);
```

#### F_token_values_set_bool
设置布尔字面量值。

```c
void F_token_values_set_bool(Stru_Token_t* token, bool value);
```

#### F_token_values_get_int
获取整数字面量值。

```c
long F_token_values_get_int(const Stru_Token_t* token);
```

#### F_token_values_get_float
获取浮点数字面量值。

```c
double F_token_values_get_float(const Stru_Token_t* token);
```

#### F_token_values_get_bool
获取布尔字面量值。

```c
bool F_token_values_get_bool(const Stru_Token_t* token);
```

### 类型查询模块 (query)

#### F_token_query_is_keyword
判断是否为关键字令牌。

```c
bool F_token_query_is_keyword(Eum_TokenType type);
```

#### F_token_query_is_operator
判断是否为运算符令牌。

```c
bool F_token_query_is_operator(Eum_TokenType type);
```

#### F_token_query_is_literal
判断是否为字面量令牌。

```c
bool F_token_query_is_literal(Eum_TokenType type);
```

#### F_token_query_is_delimiter
判断是否为分隔符令牌。

```c
bool F_token_query_is_delimiter(Eum_TokenType type);
```

#### F_token_query_get_precedence
获取运算符优先级。

```c
int F_token_query_get_precedence(Eum_TokenType type);
```

### 关键字管理模块 (keywords)

#### F_token_keywords_find_info
查找关键字信息。

```c
const Stru_KeywordInfo_t* F_token_keywords_find_info(Eum_TokenType type);
```

#### F_token_keywords_get_chinese
获取关键字的中文表示。

```c
const char* F_token_keywords_get_chinese(Eum_TokenType type);
```

#### F_token_keywords_get_english
获取关键字的英文表示。

```c
const char* F_token_keywords_get_english(Eum_TokenType type);
```

#### F_token_keywords_get_category
获取关键字的分类。

```c
int F_token_keywords_get_category(Eum_TokenType type);
```

#### F_token_keywords_get_count
获取关键字表大小。

```c
size_t F_token_keywords_get_count(void);
```

#### F_token_keywords_get_all
获取所有关键字信息。

```c
const Stru_KeywordInfo_t* F_token_keywords_get_all(void);
```

### 工具函数模块 (tools)

#### F_token_tools_type_to_string
令牌类型转字符串。

```c
const char* F_token_tools_type_to_string(Eum_TokenType type);
```

#### F_token_tools_to_string
打印令牌信息。

```c
int F_token_tools_to_string(const Stru_Token_t* token, char* buffer, size_t buffer_size);
```

#### F_token_tools_print
打印令牌信息到文件。

```c
int F_token_tools_print(const Stru_Token_t* token, FILE* stream);
```

#### F_token_tools_format_position
格式化令牌位置信息。

```c
int F_token_tools_format_position(size_t line, size_t column, char* buffer, size_t buffer_size);
```

## 错误处理

### 错误代码

| 错误代码 | 描述 | 可能原因 |
|---------|------|---------|
| TOKEN_ERROR_NONE | 无错误 | 操作成功 |
| TOKEN_ERROR_MEMORY | 内存错误 | 内存分配失败 |
| TOKEN_ERROR_INVALID_PARAM | 参数错误 | 传递了NULL指针或无效参数 |
| TOKEN_ERROR_TYPE_MISMATCH | 类型不匹配 | 为错误类型的令牌设置值 |
| TOKEN_ERROR_NOT_FOUND | 未找到 | 关键字未找到 |
| TOKEN_ERROR_BUFFER_TOO_SMALL | 缓冲区太小 | 输出缓冲区不足 |

### 错误处理示例

```c
#include "src/core/token/CN_token.h"
#include <stdio.h>

Stru_Token_t* safe_create_token(Eum_TokenType type, const char* lexeme, size_t line, size_t column) {
    if (lexeme == NULL) {
        fprintf(stderr, "错误：词素不能为NULL\n");
        return NULL;
    }
    
    Stru_Token_t* token = F_create_token(type, lexeme, line, column);
    if (token == NULL) {
        fprintf(stderr, "错误：无法创建令牌（内存不足）\n");
        return NULL;
    }
    
    return token;
}

void safe_set_int_value(Stru_Token_t* token, long value) {
    if (token == NULL) {
        fprintf(stderr, "错误：令牌指针为NULL\n");
        return;
    }
    
    if (token->type != Eum_TOKEN_LITERAL_INTEGER) {
        fprintf(stderr, "错误：令牌类型不匹配，期望整数类型\n");
        return;
    }
    
    F_token_set_int_value(token, value);
}
```

## 使用示例

### 基本使用

```c
#include "src/core/token/CN_token.h"
#include <stdio.h>

int main(void) {
    // 创建令牌
    Stru_Token_t* var_token = F_create_token(
        Eum_TOKEN_KEYWORD_VAR, "变量", 1, 1);
    
    Stru_Token_t* num_token = F_create_token(
        Eum_TOKEN_LITERAL_INTEGER, "42", 1, 8);
    
    // 设置字面量值
    F_token_set_int_value(num_token, 42);
    
    // 打印令牌信息
    printf("令牌1：");
    char buffer1[256];
    F_token_to_string(var_token, buffer1, sizeof(buffer1));
    printf("%s\n", buffer1);
    
    printf("令牌2：");
    char buffer2[256];
    F_token_to_string(num_token, buffer2, sizeof(buffer2));
    printf("%s\n", buffer2);
    
    // 查询关键字信息
    const char* chinese = F_token_get_chinese_keyword(Eum_TOKEN_KEYWORD_VAR);
    const char* english = F_token_get_english_keyword(Eum_TOKEN_KEYWORD_VAR);
    printf("关键字：%s (%s)\n", chinese, english);
    
    // 类型检查
    if (F_token_is_keyword(var_token->type)) {
        printf("令牌1是关键字\n");
    }
    
    if (F_token_is_literal(num_token->type)) {
        printf("令牌2是字面量\n");
    }
    
    // 清理
    F_destroy_token(var_token);
    F_destroy_token(num_token);
    
    return 0;
}
```

### 使用模块化接口

```c
#include "src/core/token/CN_token_interface.h"
#include <stdio.h>

int main(void) {
    // 获取令牌接口
    const Stru_TokenInterface_t* token_if = F_get_token_interface();
    
    // 初始化模块
    token_if->initialize();
    
    // 创建令牌
    Stru_Token_t* token = token_if->create_token(
        Eum_TOKEN_KEYWORD_VAR, "变量", 1, 1);
    
    // 使用令牌功能
    Eum_TokenType type = token_if->get_token_type(token);
    const char* lexeme = token_if->get_token_lexeme(token);
    
    printf("令牌类型：%d，词素：%s\n", type, lexeme);
    
    // 类型查询
    bool is_keyword = token_if->is_keyword(type);
    printf("是否为关键字：%s\n", is_keyword ? "是" : "否");
    
    // 打印令牌信息
    char buffer[256];
    token_if->to_string(token, buffer, sizeof(buffer));
    printf("令牌信息：%s\n", buffer);
    
    // 清理
    token_if->destroy_token(token);
    token_if->cleanup();
    
    return 0;
}
```

## 性能考虑

### 内存使用
- 每个令牌约64-128字节
- 词素字符串会被复制，增加内存使用
- 建议重用频繁创建的令牌类型

### 性能基准
| 操作 | 平均时间 | 内存分配 |
|------|----------|----------|
| 创建令牌 | 50-100 ns
