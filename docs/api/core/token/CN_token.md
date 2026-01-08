# CN_Language 令牌模块 API 文档

## 概述

令牌（Token）模块是CN_Language编译器的核心数据模型，定义了源代码中所有基本语法单元的结构和类型。本模块实现了对70个中文关键字的完整类型定义，为词法分析器、语法分析器等后续阶段提供统一的数据接口。

## 数据结构

### Eum_TokenType 枚举

定义了所有可能的令牌类型，包括70个中文关键字。

```c
typedef enum {
    // ============================================
    // 基本令牌类型
    // ============================================
    
    // 文件结束标记
    Eum_TOKEN_EOF = 0,
    
    // 错误标记
    Eum_TOKEN_ERROR,
    
    // 标识符
    Eum_TOKEN_IDENTIFIER,
    
    // ============================================
    // 字面量类型
    // ============================================
    
    Eum_TOKEN_LITERAL_INTEGER,      // 整数字面量
    Eum_TOKEN_LITERAL_FLOAT,        // 浮点数字面量
    Eum_TOKEN_LITERAL_STRING,       // 字符串字面量
    Eum_TOKEN_LITERAL_CHAR,         // 字符字面量
    Eum_TOKEN_LITERAL_BOOLEAN,      // 布尔字面量
    
    // ============================================
    // 运算符类型
    // ============================================
    
    Eum_TOKEN_OPERATOR_PLUS,        // + 加法运算符
    Eum_TOKEN_OPERATOR_MINUS,       // - 减法运算符
    Eum_TOKEN_OPERATOR_MULTIPLY,    // * 乘法运算符
    Eum_TOKEN_OPERATOR_DIVIDE,      // / 除法运算符
    Eum_TOKEN_OPERATOR_MODULO,      // % 取模运算符
    Eum_TOKEN_OPERATOR_ASSIGN,      // = 赋值运算符
    Eum_TOKEN_OPERATOR_EQUAL,       // == 相等运算符
    Eum_TOKEN_OPERATOR_NOT_EQUAL,   // != 不等运算符
    Eum_TOKEN_OPERATOR_LESS,        // < 小于运算符
    Eum_TOKEN_OPERATOR_GREATER,     // > 大于运算符
    Eum_TOKEN_OPERATOR_LESS_EQUAL,  // <= 小于等于运算符
    Eum_TOKEN_OPERATOR_GREATER_EQUAL, // >= 大于等于运算符
    
    // ============================================
    // 分隔符类型
    // ============================================
    
    Eum_TOKEN_DELIMITER_COMMA,      // , 逗号分隔符
    Eum_TOKEN_DELIMITER_SEMICOLON,  // ; 分号分隔符
    Eum_TOKEN_DELIMITER_LPAREN,     // ( 左圆括号
    Eum_TOKEN_DELIMITER_RPAREN,     // ) 右圆括号
    Eum_TOKEN_DELIMITER_LBRACE,     // { 左花括号
    Eum_TOKEN_DELIMITER_RBRACE,     // } 右花括号
    Eum_TOKEN_DELIMITER_LBRACKET,   // [ 左方括号
    Eum_TOKEN_DELIMITER_RBRACKET,   // ] 右方括号
    
    // ============================================
    // 中文关键字（70个）
    // ============================================
    
    // 变量和类型声明关键字
    Eum_TOKEN_KEYWORD_VAR,          // 变量
    Eum_TOKEN_KEYWORD_CONST,        // 常量
    Eum_TOKEN_KEYWORD_TYPE,         // 类型
    Eum_TOKEN_KEYWORD_CLASS,        // 类
    Eum_TOKEN_KEYWORD_OBJECT,       // 对象
    
    // 控制结构关键字
    Eum_TOKEN_KEYWORD_IF,           // 如果
    Eum_TOKEN_KEYWORD_ELSE,         // 否则
    Eum_TOKEN_KEYWORD_WHILE,        // 当
    Eum_TOKEN_KEYWORD_FOR,          // 循环
    Eum_TOKEN_KEYWORD_BREAK,        // 中断
    Eum_TOKEN_KEYWORD_CONTINUE,     // 继续
    Eum_TOKEN_KEYWORD_SWITCH,       // 选择
    Eum_TOKEN_KEYWORD_CASE,         // 情况
    Eum_TOKEN_KEYWORD_DEFAULT,      // 默认
    
    // 函数相关关键字
    Eum_TOKEN_KEYWORD_FUNCTION,     // 函数
    Eum_TOKEN_KEYWORD_RETURN,       // 返回
    Eum_TOKEN_KEYWORD_MAIN,         // 主程序
    Eum_TOKEN_KEYWORD_VOID,         // 无
    
    // 逻辑运算符关键字
    Eum_TOKEN_KEYWORD_AND,          // 与
    Eum_TOKEN_KEYWORD_OR,           // 或
    Eum_TOKEN_KEYWORD_NOT,          // 非
    
    // 字面量关键字
    Eum_TOKEN_KEYWORD_TRUE,         // 真
    Eum_TOKEN_KEYWORD_FALSE,        // 假
    Eum_TOKEN_KEYWORD_NULL,         // 空
    
    // 模块系统关键字
    Eum_TOKEN_KEYWORD_MODULE,       // 模块
    Eum_TOKEN_KEYWORD_IMPORT,       // 导入
    Eum_TOKEN_KEYWORD_EXPORT,       // 导出
    
    // 运算符关键字
    Eum_TOKEN_KEYWORD_ADD,          // 加
    Eum_TOKEN_KEYWORD_SUBTRACT,     // 减
    Eum_TOKEN_KEYWORD_MULTIPLY,     // 乘
    Eum_TOKEN_KEYWORD_DIVIDE,       // 除
    Eum_TOKEN_KEYWORD_MODULO,       // 模
    
    // 类型关键字
    Eum_TOKEN_KEYWORD_INTEGER,      // 整数
    Eum_TOKEN_KEYWORD_FLOAT,        // 浮点数
    Eum_TOKEN_KEYWORD_STRING,       // 字符串
    Eum_TOKEN_KEYWORD_BOOLEAN,      // 布尔
    Eum_TOKEN_KEYWORD_CHAR,         // 字符
    
    // 其他关键字
    Eum_TOKEN_KEYWORD_TRY,          // 尝试
    Eum_TOKEN_KEYWORD_CATCH,        // 捕获
    Eum_TOKEN_KEYWORD_THROW,        // 抛出
    Eum_TOKEN_KEYWORD_FINALLY,      // 最终
    
    // ... 其他45个关键字
} Eum_TokenType;
```

### Stru_Token_t 结构体

令牌的核心数据结构，包含所有必要的信息。

```c
typedef struct Stru_Token_t {
    // ============================================
    // 基本属性
    // ============================================
    
    Eum_TokenType type;             ///< 令牌类型
    char* lexeme;                   ///< 词素（源代码中的原始字符串）
    size_t line;                    ///< 行号（从1开始）
    size_t column;                  ///< 列号（从1开始）
    
    // ============================================
    // 字面量值（根据类型使用其中一个）
    // ============================================
    
    union {
        long int_value;             ///< 整数值（用于Eum_TOKEN_LITERAL_INTEGER）
        double float_value;         ///< 浮点数值（用于Eum_TOKEN_LITERAL_FLOAT）
        char* string_value;         ///< 字符串值（用于Eum_TOKEN_LITERAL_STRING）
        char char_value;            ///< 字符值（用于Eum_TOKEN_LITERAL_CHAR）
        bool bool_value;            ///< 布尔值（用于Eum_TOKEN_LITERAL_BOOLEAN）
    } literal;
    
    // ============================================
    // 扩展信息
    // ============================================
    
    void* extra_data;               ///< 扩展数据指针（用于存储额外信息）
    size_t extra_data_size;         ///< 扩展数据大小（字节数）
    
} Stru_Token_t;
```

### Stru_KeywordInfo_t 结构体

关键字信息结构体，用于查询关键字详细信息。

```c
typedef struct Stru_KeywordInfo_t {
    const char* chinese;            ///< 中文关键字
    const char* english;            ///< 英文对应词
    const char* description;        ///< 功能描述
    const char* category;           ///< 所属类别
    int precedence;                 ///< 优先级（0-10，越高优先级越高）
    Eum_TokenType token_type;       ///< 对应的令牌类型
} Stru_KeywordInfo_t;
```

## API 参考

### 令牌创建和销毁

#### F_create_token

创建新令牌。

**函数签名：**
```c
Stru_Token_t* F_create_token(Eum_TokenType type, 
                            const char* lexeme, 
                            size_t line, 
                            size_t column);
```

**参数：**
- `type`：令牌类型
- `lexeme`：词素字符串（会被复制）
- `line`：行号（从1开始）
- `column`：列号（从1开始）

**返回值：**
- `Stru_Token_t*`：新创建的令牌指针
- `NULL`：创建失败（内存不足）

**示例：**
```c
Stru_Token_t* token = F_create_token(
    Eum_TOKEN_KEYWORD_VAR,  // 类型：变量关键字
    "变量",                 // 词素
    1,                      // 行号
    1                       // 列号
);
```

#### F_destroy_token

销毁令牌，释放所有相关资源。

**函数签名：**
```c
void F_destroy_token(Stru_Token_t* token);
```

**参数：**
- `token`：要销毁的令牌指针

**注意事项：**
- 此函数会释放令牌的所有内存，包括词素字符串和扩展数据
- 调用此函数后，令牌指针不再有效
- 如果令牌为NULL，函数不执行任何操作

#### F_clone_token

克隆令牌，创建深拷贝。

**函数签名：**
```c
Stru_Token_t* F_clone_token(const Stru_Token_t* token);
```

**参数：**
- `token`：要克隆的源令牌指针

**返回值：**
- `Stru_Token_t*`：新克隆的令牌指针
- `NULL`：克隆失败

**特点：**
- 创建完全独立的副本
- 复制所有数据，包括扩展数据
- 调用者负责销毁返回的令牌

### 字面量值设置

#### F_token_set_int_value

设置令牌的整数值。

**函数签名：**
```c
void F_token_set_int_value(Stru_Token_t* token, long value);
```

**参数：**
- `token`：令牌指针
- `value`：整数值

**适用类型：**
- `Eum_TOKEN_LITERAL_INTEGER`

#### F_token_set_float_value

设置令牌的浮点数值。

**函数签名：**
```c
void F_token_set_float_value(Stru_Token_t* token, double value);
```

**参数：**
- `token`：令牌指针
- `value`：浮点数值

**适用类型：**
- `Eum_TOKEN_LITERAL_FLOAT`

#### F_token_set_string_value

设置令牌的字符串值。

**函数签名：**
```c
void F_token_set_string_value(Stru_Token_t* token, const char* value);
```

**参数：**
- `token`：令牌指针
- `value`：字符串值（会被复制）

**适用类型：**
- `Eum_TOKEN_LITERAL_STRING`

#### F_token_set_char_value

设置令牌的字符值。

**函数签名：**
```c
void F_token_set_char_value(Stru_Token_t* token, char value);
```

**参数：**
- `token`：令牌指针
- `value`：字符值

**适用类型：**
- `Eum_TOKEN_LITERAL_CHAR`

#### F_token_set_bool_value

设置令牌的布尔值。

**函数签名：**
```c
void F_token_set_bool_value(Stru_Token_t* token, bool value);
```

**参数：**
- `token`：令牌指针
- `value`：布尔值

**适用类型：**
- `Eum_TOKEN_LITERAL_BOOLEAN`

### 类型查询和分类

#### F_get_token_type_name

获取令牌类型的名称字符串。

**函数签名：**
```c
const char* F_get_token_type_name(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `const char*`：类型名称字符串
- 对于未知类型，返回"UNKNOWN"

**示例：**
```c
const char* name = F_get_token_type_name(Eum_TOKEN_KEYWORD_VAR);
printf("类型名称：%s\n", name);  // 输出：KEYWORD_VAR
```

#### F_is_keyword_token

检查令牌类型是否为关键字。

**函数签名：**
```c
bool F_is_keyword_token(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `true`：是关键字
- `false`：不是关键字

#### F_is_literal_token

检查令牌类型是否为字面量。

**函数签名：**
```c
bool F_is_literal_token(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `true`：是字面量
- `false`：不是字面量

#### F_is_operator_token

检查令牌类型是否为运算符。

**函数签名：**
```c
bool F_is_operator_token(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `true`：是运算符
- `false`：不是运算符

#### F_is_delimiter_token

检查令牌类型是否为分隔符。

**函数签名：**
```c
bool F_is_delimiter_token(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `true`：是分隔符
- `false`：不是分隔符

#### F_is_identifier_token

检查令牌类型是否为标识符。

**函数签名：**
```c
bool F_is_identifier_token(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `true`：是标识符
- `false`：不是标识符

### 关键字信息查询

#### F_get_keyword_info

通过中文关键字获取关键字信息。

**函数签名：**
```c
const Stru_KeywordInfo_t* F_get_keyword_info(const char* chinese_keyword);
```

**参数：**
- `chinese_keyword`：中文关键字字符串

**返回值：**
- `const Stru_KeywordInfo_t*`：关键字信息指针
- `NULL`：未找到对应的关键字

**示例：**
```c
const Stru_KeywordInfo_t* info = F_get_keyword_info("变量");
if (info != NULL) {
    printf("关键字：%s\n", info->chinese);
    printf("英文：%s\n", info->english);
    printf("描述：%s\n", info->description);
    printf("类别：%s\n", info->category);
    printf("优先级：%d\n", info->precedence);
}
```

#### F_get_keyword_by_english

通过英文名获取关键字信息。

**函数签名：**
```c
const Stru_KeywordInfo_t* F_get_keyword_by_english(const char* english_name);
```

**参数：**
- `english_name`：英文名称

**返回值：**
- `const Stru_KeywordInfo_t*`：关键字信息指针
- `NULL`：未找到对应的关键字

#### F_get_keyword_by_type

通过令牌类型获取关键字信息。

**函数签名：**
```c
const Stru_KeywordInfo_t* F_get_keyword_by_type(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型（必须是关键字类型）

**返回值：**
- `const Stru_KeywordInfo_t*`：关键字信息指针
- `NULL`：不是关键字类型或未找到

### 工具函数

#### F_token_to_string

将令牌转换为可读的字符串表示。

**函数签名：**
```c
char* F_token_to_string(const Stru_Token_t* token);
```

**参数：**
- `token`：令牌指针

**返回值：**
- `char*`：字符串表示（调用者负责释放）
- `NULL`：转换失败

**示例输出：**
```
KEYWORD_VAR("变量") at 1:1
LITERAL_INTEGER(42) at 1:8
OPERATOR_PLUS("+") at 1:11
```

#### F_print_token

打印令牌信息到标准输出。

**函数签名：**
```c
void F_print_token(const Stru_Token_t* token);
```

**参数：**
- `token`：令牌指针

**输出格式：**
```
[行:列] 类型(词素) = 值
```

**示例：**
```c
F_print_token(token);
// 输出：[1:1] KEYWORD_VAR("变量") = <none>
// 输出：[1:8] LITERAL_INTEGER("42") = 42
```

#### F_compare_tokens

比较两个令牌是否相等。

**函数签名：**
```c
bool F_compare_tokens(const Stru_Token_t* token1, const Stru_Token_t* token2);
```

**参数：**
- `token1`：第一个令牌指针
- `token2`：第二个令牌指针

**返回值：**
- `true`：令牌相等（类型、词素、值都相等）
- `false`：令牌不相等或任一令牌为NULL

**比较规则：**
1. 类型必须相同
2. 词素必须相同（字符串比较）
3. 字面量值必须相同（根据类型）
4. 位置信息不参与比较

#### F_token_hash

计算令牌的哈希值。

**函数签名：**
```c
size_t F_token_hash(const Stru_Token_t* token);
```

**参数：**
- `token`：令牌指针

**返回值：**
- `size_t`：哈希值
- `0`：令牌为NULL

**用途：**
- 用于哈希表存储
- 用于快速比较

## 使用示例

### 基本使用

```c
#include "src/core/token/CN_token.h"
#include <stdio.h>

int main(void) {
    // 创建令牌
    Stru_Token_t* var_token = F_create_token(
        Eum_TOKEN_KEYWORD_VAR,  // 类型：变量关键字
        "变量",                 // 词素
        1,                      // 行号
        1                       // 列号
    );
    
    Stru_Token_t* num_token = F_create_token(
        Eum_TOKEN_LITERAL_INTEGER,  // 类型：整数字面量
        "42",                      // 词素
        1,                         // 行号
        8                          // 列号
    );
    
    // 设置字面量值
    F_token_set_int_value(num_token, 42);
    
    // 打印令牌信息
    printf("令牌1：");
    F_print_token(var_token);
    
    printf("令牌2：");
    F_print_token(num_token);
    
    // 查询关键字信息
    const Stru_KeywordInfo_t* info = F_get_keyword_info("变量");
    if (info != NULL) {
        printf("\n关键字信息：\n");
        printf("  中文：%s\n", info->chinese);
        printf("  英文：%s\n", info->english);
        printf("  描述：%s\n", info->description);
        printf("  类别：%s\n", info->category);
        printf("  优先级：%d\n", info->precedence);
    }
    
    // 类型检查
    if (F_is_keyword_token(var_token->type)) {
        printf("\n令牌1是关键字\n");
    }
    
    if (F_is_literal_token(num_token->type)) {
        printf("令牌2是字面量\n");
    }
    
    // 转换为字符串
    char* str1 = F_token_to_string(var_token);
    char* str2 = F_token_to_string(num_token);
    printf("\n字符串表示：\n");
    printf("  令牌1：%s\n", str1);
    printf("  令牌2：%s\n", str2);
    
    // 清理
    free(str1);
    free(str2);
    F_destroy_token(var_token);
    F_destroy_token(num_token);
    
    return 0;
}
```

### 关键字处理

```c
#include "src/core/token/CN_token.h"
#include <stdio.h>

void process_keyword(const char* chinese_keyword) {
    const Stru_KeywordInfo_t* info = F_get_keyword_info(chinese_keyword);
    if (info == NULL) {
        printf("未找到关键字：%s\n", chinese_keyword);
        return;
    }
    
    printf("处理关键字：%s\n", chinese_keyword);
    printf("  英文对应：%s\n", info->english);
    printf("  功能描述：%s\n", info->description);
    printf("  所属类别：%s\n", info->category);
    
    // 根据类别进行不同处理
    if (strcmp(info->category, "控制结构") == 0) {
        printf("  处理：控制结构关键字\n");
    } else if (strcmp(info->category, "类型声明") == 0) {
        printf("  处理：类型声明关键字\n");
    } else if (strcmp(info->category, "运算符") == 0) {
        printf("  处理：运算符关键字\n");
    }
    
    // 创建对应的令牌
    Stru_Token_t* token = F_create_token(
        info->token_type,
        info->chinese,
        1, 1
    );
    
    // 使用令牌...
    
    F_destroy_token(token);
}

int main(void) {
    // 处理不同类型的关键字
    process_keyword("变量");
    process_keyword("如果");
    process_keyword("函数");
    process_keyword("真");
    
    return 0;
}
```

### 令牌比较和哈希

```c
#include "src/core/token/CN_token.h"
#include <stdio.h>

int main(void) {
    // 创建两个相同的令牌
    Stru_Token_t* token1 = F_create_token(
        Eum_TOKEN_LITERAL_INTEGER,
        "100",
        1, 1
    );
    F_token_set_int_value(token1, 100);
    
    Stru_Token_t* token2 = F_create_token(
        Eum_TOKEN_LITERAL_INTEGER,
        "100",
        2, 5  // 不同位置
    );
    F_token_set_int_value(token2, 100);
    
    // 比较令牌
    if (F_compare_tokens(token1, token2)) {
        printf("令牌相等（忽略位置）\n");
    } else {
        printf("令牌不相等\n");
    }
    
    // 计算哈希值
    size_t hash1 = F_token_hash(token1);
    size_t hash2 = F_token_hash(token2);
    
    printf("哈希值：\n");
    printf("  令牌1：%zu\n", hash1);
    printf("  令牌2：%zu\n", hash2);
    
    if (hash1 == hash2) {
        printf("哈希值相等\n");
    }
    
    // 克隆令牌
    Stru_Token_t* cloned = F_clone_token(token1);
    if (cloned != NULL) {
        printf("克隆成功\n");
        
        // 验证克隆
        if (F_compare_tokens(token1, cloned)) {
            printf("克隆令牌与原始令牌相等\n");
        }
        
        F_destroy_token(cloned);
    }
    
    // 清理
    F_destroy_token(token1);
    F_destroy_token(token2);
    
    return 0;
}
```

## 错误处理

### 常见错误

1. **内存分配失败**：`F_create_token()`返回NULL
2. **无效参数**：传递NULL指针或无效类型
3. **类型不匹配**：为错误类型的令牌设置值
4. **关键字未找到**：`F_get_keyword_info()`返回NULL

### 错误处理示例

```c
#include "src/core/token/CN_token.h"
#include <stdio.h>

Stru_Token_t* safe_create_token(Eum_TokenType type, 
                               const char* lexeme, 
                               size_t line, 
                               size_t column) {
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

int main(void) {
    // 安全创建令牌
    Stru_Token_t* token = safe_create_token(
        Eum_TOKEN_LITERAL_INTEGER,
        "123",
        1, 1
    );
    
    if (token == NULL) {
        return 1;
    }
    
    // 安全设置值
    safe_set_int_value(token, 123);
    
    // 安全查询关键字
    const char* keyword = "不存在的关键字";
    const Stru_KeywordInfo_t* info = F_get_keyword_info(keyword);
    if (info == NULL) {
        printf("警告：未找到关键字 '%s'\n", keyword);
    }
    
    F_destroy_token(token);
    return 0;
}
```

## 性能考虑

### 内存管理

1. **令牌大小**：每个令牌约64-128字节
2. **字符串复制**：词素字符串会被复制，增加内存使用
3. **扩展数据**：扩展数据会增加内存使用

### 优化建议

1. **重用令牌**：对于频繁创建的令牌类型，使用对象池
2. **字符串内联**：对于短词素，考虑使用内联存储
3. **延迟分配**：扩展数据按需分配

### 性能基准

| 操作 | 平均时间 | 内存分配 |
|------|----------|----------|
| 创建令牌 | 50-100 ns | 1次分配 |
| 销毁令牌 | 30-60 ns | 1次释放 |
| 设置值 | 10-20 ns | 可能分配 |
| 类型检查 | 5-10 ns | 无分配 |
| 关键字查询 | 20-40 ns | 无分配 |

## 线程安全性

### 线程安全级别

令牌模块是**部分线程安全**的：

1. **只读操作**：线程安全
   - `F_get_token_type_name()`
   - `F_is_keyword_token()` 等类型检查函数
   - `F_get_keyword_info()` 等查询函数
   - `F_token_hash()`

2. **写入操作**：非线程安全
   - `F_create_token()`
   - `F_destroy_token()`
   - `F_token_set_*_value()`
   - `F_clone_token()`

### 线程安全使用模式

```c
#include "src/core/token/CN_token.h"
#include <pthread.h>

// 线程局部存储
__thread Stru_Token_t* thread_local_token = NULL;

void* thread_function(void* arg) {
    // 每个线程有自己的令牌
    thread_local_token = F_create_token(
        Eum_TOKEN_IDENTIFIER,
        (char*)arg,
        1, 1
    );
    
    // 安全使用（只在本线程内）
    if (F_is_identifier_token(thread_local_token->type)) {
        // 处理标识符...
    }
    
    // 清理
    F_destroy_token(thread_local_token);
    thread_local_token = NULL;
    
    return NULL;
}

// 只读共享数据（线程安全）
void safe_print_token_info(const Stru_Token_t* token) {
    // 这些函数是线程安全的
    const char* type_name = F_get_token_type_name(token->type);
    size_t hash = F_token_hash(token);
    
    printf("类型：%s，哈希：%zu\n", type_name, hash);
}
```

## 扩展指南

### 添加新令牌类型

1. 在`Eum_TokenType`枚举中添加新类型
2. 更新类型分类函数（如果需要）
3. 添加类型名称到`F_get_token_type_name()`
4. 编写相应的测试

### 添加新关键字

1. 在`Eum_TokenType`枚举中添加关键字类型
2. 在关键字表中添加条目
3. 更新`F_get_keyword_info()`等函数
4. 确保向后兼容性

### 自定义扩展数据

```c
// 自定义扩展数据结构
typedef struct {
    int scope_level;
    char* namespace;
    void* symbol_table_entry;
} CustomExtraData_t;

// 设置扩展数据
void set_custom_extra_data(Stru_Token_t* token, CustomExtraData_t* data) {
    token->extra_data = data;
    token->extra_data_size = sizeof(CustomExtraData_t);
}

// 获取扩展数据
CustomExtraData_t* get_custom_extra_data(const Stru_Token_t* token) {
    if (token->extra_data_size == sizeof(CustomExtraData_t)) {
        return (CustomExtraData_t*)token->extra_data;
    }
    return NULL;
}
```

## 模块化架构

CN_Language 1.2.0版本引入了模块化架构，将令牌模块拆分为多个子模块，每个子模块专注于单一职责：

### 模块化结构

```
src/core/token/
├── CN_token.h                    # 主头文件（向后兼容）
├── CN_token.c                    # 适配器实现（向后兼容）
├── CN_token_interface.h          # 抽象接口定义
├── CN_token_types.h              # 类型定义（70个中文关键字）
├── lifecycle/                    # 令牌生命周期管理
│   ├── CN_token_lifecycle.h
│   └── CN_token_lifecycle.c
├── values/                       # 字面量值操作
│   ├── CN_token_values.h
│   └── CN_token_values.c
├── query/                        # 类型查询和分类
│   ├── CN_token_query.h
│   └── CN_token_query.c
├── keywords/                     # 关键字信息管理
│   ├── CN_token_keywords.h
│   └── CN_token_keywords.c
├── tools/                        # 工具函数
│   ├── CN_token_tools.h
│   └── CN_token_tools.c
└── interface/                    # 接口实现
    └── CN_token_interface_impl.c
```

### 新架构优势

1. **单一职责原则**：每个子模块不超过500行代码，每个函数不超过50行
2. **更好的可维护性**：模块间通过抽象接口通信，降低耦合度
3. **可测试性**：每个子模块有独立的测试文件
4. **可扩展性**：支持依赖注入，可替换模块实现
5. **向后兼容**：现有代码无需修改

### 使用新架构

```c
// 传统方式（向后兼容）
#include "src/core/token/CN_token.h"

// 新架构方式（模块化）
#include "src/core/token/CN_token_interface.h"
#include "src/core/token/lifecycle/CN_token_lifecycle.h"
#include "src/core/token/values/CN_token_values.h"
// ... 其他需要的子模块
```

### 详细文档

有关模块化架构的完整文档，请参阅：
- [令牌模块化架构文档](CN_token_module.md) - 详细的新架构设计、接口使用和测试框架

## 相关文档

- [词法分析器接口 API 文档](../lexer/CN_lexer_interface.md)
- [动态数组 API 文档](../../infrastructure/containers/array/CN_dynamic_array.md)
- [架构设计文档](../../../architecture/001-中文编程语言CN_Language开发规划.md)
- [令牌模块化架构文档](CN_token_module.md)

## 修订历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本发布，支持70个中文关键字 |
| 1.0.1 | 2026-01-06 | 修复内存泄漏问题 |
| 1.1.0 | 2026-01-06 | 添加关键字信息查询功能 |
| 1.2.0 | 2026-01-08 | 引入模块化架构，拆分为6个子模块 |

## 版权声明

版权所有 © 2026 CN_Language项目团队。保留所有权利。

本文档是CN_Language项目的一部分，遵循项目许可证条款。
