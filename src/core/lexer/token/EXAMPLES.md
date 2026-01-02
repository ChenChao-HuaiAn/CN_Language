# CN_Language Token模块 使用示例

## 概述

本文档提供CN_Language Token模块的完整使用示例，涵盖从基本操作到高级用法的各种场景。

## 目录

1. [基本示例](#基本示例)
2. [Token创建示例](#token创建示例)
3. [Token查询示例](#token查询示例)
4. [Token操作示例](#token操作示例)
5. [运算符属性示例](#运算符属性示例)
6. [错误处理示例](#错误处理示例)
7. [内存管理示例](#内存管理示例)
8. [综合示例](#综合示例)

## 基本示例

### 示例1：包含头文件和基本结构

```c
#include <stdio.h>
#include <stdlib.h>
#include "CN_token.h"

int main()
{
    printf("CN_Language Token模块示例程序\n");
    printf("==============================\n\n");
    
    return 0;
}
```

### 示例2：简单的Token创建和销毁

```c
#include <stdio.h>
#include "CN_token.h"

void basic_token_example()
{
    printf("=== 基本Token示例 ===\n");
    
    // 创建标识符Token
    Stru_Token_t* token = F_token_create(
        Eum_TOKEN_IDENTIFIER,    // Token类型
        "myVariable",            // 原始字符串
        0,                       // 自动计算长度
        1,                       // 行号
        1                        // 列号
    );
    
    if (token == NULL) {
        printf("创建Token失败\n");
        return;
    }
    
    // 打印Token信息
    printf("创建的Token信息:\n");
    F_token_print(token, true, false);
    
    // 销毁Token
    F_token_destroy(token);
    printf("Token已销毁\n\n");
}
```

## Token创建示例

### 示例3：创建各种类型的Token

```c
#include <stdio.h>
#include "CN_token.h"

void create_various_tokens()
{
    printf("=== 创建各种类型的Token ===\n");
    
    // 1. 创建关键字Token
    Stru_Token_t* keyword_token = F_token_create(
        Eum_TOKEN_KEYWORD_IF,    // 如果关键字
        "如果",                  // 中文关键字
        0,                       // 自动计算长度
        2,                       // 行号
        5                        // 列号
    );
    
    if (keyword_token != NULL) {
        printf("关键字Token: ");
        F_token_print(keyword_token, true, false);
        F_token_destroy(keyword_token);
    }
    
    // 2. 创建运算符Token
    Stru_Token_t* operator_token = F_token_create(
        Eum_TOKEN_OPERATOR_PLUS, // + 运算符
        "+",                     // 运算符符号
        0,                       // 自动计算长度
        3,                       // 行号
        10                       // 列号
    );
    
    if (operator_token != NULL) {
        printf("运算符Token: ");
        F_token_print(operator_token, true, false);
        F_token_destroy(operator_token);
    }
    
    // 3. 创建分隔符Token
    Stru_Token_t* separator_token = F_token_create(
        Eum_TOKEN_SEMICOLON,     // 分号
        ";",                     // 分隔符符号
        0,                       // 自动计算长度
        4,                       // 行号
        15                       // 列号
    );
    
    if (separator_token != NULL) {
        printf("分隔符Token: ");
        F_token_print(separator_token, true, false);
        F_token_destroy(separator_token);
    }
    
    printf("\n");
}
```

### 示例4：创建字面量Token

```c
#include <stdio.h>
#include "CN_token.h"

void create_literal_tokens()
{
    printf("=== 创建字面量Token ===\n");
    
    // 注意：当前实现中F_token_create_literal函数可能未实现
    // 这里展示如何使用F_token_create创建字面量Token
    
    // 1. 整数字面量
    Stru_Token_t* int_token = F_token_create(
        Eum_TOKEN_INTEGER_LITERAL,
        "12345",
        0,
        5,
        1
    );
    
    if (int_token != NULL) {
        // 设置字面量值
        int_token->literal_value.integer_value = 12345;
        printf("整数字面量Token: ");
        F_token_print(int_token, true, true);
        F_token_destroy(int_token);
    }
    
    // 2. 浮点数字面量
    Stru_Token_t* float_token = F_token_create(
        Eum_TOKEN_FLOAT_LITERAL,
        "3.14159",
        0,
        6,
        1
    );
    
    if (float_token != NULL) {
        float_token->literal_value.float_value = 3.14159;
        printf("浮点数字面量Token: ");
        F_token_print(float_token, true, true);
        F_token_destroy(float_token);
    }
    
    // 3. 布尔字面量
    Stru_Token_t* bool_token = F_token_create(
        Eum_TOKEN_BOOLEAN_LITERAL,
        "真",
        0,
        7,
        1
    );
    
    if (bool_token != NULL) {
        bool_token->literal_value.boolean_value = true;
        printf("布尔字面量Token: ");
        F_token_print(bool_token, true, true);
        F_token_destroy(bool_token);
    }
    
    printf("\n");
}
```

## Token查询示例

### 示例5：查询Token类型信息

```c
#include <stdio.h>
#include "CN_token.h"

void query_token_info()
{
    printf("=== 查询Token类型信息 ===\n");
    
    // 测试各种Token类型
    enum Eum_TokenType test_types[] = {
        Eum_TOKEN_KEYWORD_VARIABLE,
        Eum_TOKEN_IDENTIFIER,
        Eum_TOKEN_INTEGER_LITERAL,
        Eum_TOKEN_OPERATOR_PLUS,
        Eum_TOKEN_SEMICOLON,
        Eum_TOKEN_EOF
    };
    
    const char* type_names[] = {
        "Eum_TOKEN_KEYWORD_VARIABLE",
        "Eum_TOKEN_IDENTIFIER", 
        "Eum_TOKEN_INTEGER_LITERAL",
        "Eum_TOKEN_OPERATOR_PLUS",
        "Eum_TOKEN_SEMICOLON",
        "Eum_TOKEN_EOF"
    };
    
    int count = sizeof(test_types) / sizeof(test_types[0]);
    
    for (int i = 0; i < count; i++) {
        enum Eum_TokenType type = test_types[i];
        
        printf("类型: %s\n", type_names[i]);
        printf("  类型名称: %s\n", F_token_type_to_string(type));
        printf("  类别: %s\n", F_token_get_category(type));
        printf("  是关键字? %s\n", F_token_is_keyword(type) ? "是" : "否");
        printf("  是运算符? %s\n", F_token_is_operator(type) ? "是" : "否");
        printf("  是字面量? %s\n", F_token_is_literal(type) ? "是" : "否");
        printf("  是分隔符? %s\n", F_token_is_separator(type) ? "是" : "否");
        printf("\n");
    }
}
```

### 示例6：检查Token属性

```c
#include <stdio.h>
#include "CN_token.h"

void check_token_properties()
{
    printf("=== 检查Token属性 ===\n");
    
    // 创建测试Token
    Stru_Token_t* tokens[5];
    
    tokens[0] = F_token_create(Eum_TOKEN_KEYWORD_IF, "如果", 0, 1, 1);
    tokens[1] = F_token_create(Eum_TOKEN_IDENTIFIER, "变量名", 0, 1, 5);
    tokens[2] = F_token_create(Eum_TOKEN_OPERATOR_PLUS, "+", 0, 1, 10);
    tokens[3] = F_token_create(Eum_TOKEN_INTEGER_LITERAL, "123", 0, 1, 12);
    tokens[4] = F_token_create(Eum_TOKEN_SEMICOLON, ";", 0, 1, 15);
    
    const char* token_descriptions[] = {
        "关键字Token (如果)",
        "标识符Token (变量名)",
        "运算符Token (+)",
        "字面量Token (123)",
        "分隔符Token (;)"
    };
    
    for (int i = 0; i < 5; i++) {
        if (tokens[i] != NULL) {
            printf("%s:\n", token_descriptions[i]);
            F_token_print(tokens[i], false, false);
            
            // 检查属性
            enum Eum_TokenType type = tokens[i]->type;
            printf("  关键字: %s\n", F_token_is_keyword(type) ? "是" : "否");
            printf("  运算符: %s\n", F_token_is_operator(type) ? "是" : "否");
            printf("  字面量: %s\n", F_token_is_literal(type) ? "是" : "否");
            printf("  分隔符: %s\n", F_token_is_separator(type) ? "是" : "否");
            printf("\n");
            
            F_token_destroy(tokens[i]);
        }
    }
}
```

## Token操作示例

### 示例7：Token复制和比较

```c
#include <stdio.h>
#include "CN_token.h"

void token_copy_and_compare()
{
    printf("=== Token复制和比较 ===\n");
    
    // 创建原始Token
    Stru_Token_t* original = F_token_create(
        Eum_TOKEN_KEYWORD_WHILE,
        "当",
        0,
        10,
        5
    );
    
    if (original == NULL) {
        printf("创建原始Token失败\n");
        return;
    }
    
    printf("原始Token: ");
    F_token_print(original, true, false);
    
    // 复制Token
    Stru_Token_t* copy = F_token_copy(original);
    
    if (copy == NULL) {
        printf("复制Token失败\n");
        F_token_destroy(original);
        return;
    }
    
    printf("复制的Token: ");
    F_token_print(copy, true, false);
    
    // 比较Token
    printf("比较结果:\n");
    printf("  原始 == 复制: %s\n", F_token_equals(original, copy) ? "相等" : "不相等");
    printf("  原始 == 原始: %s\n", F_token_equals(original, original) ? "相等" : "不相等");
    printf("  复制 == 复制: %s\n", F_token_equals(copy, copy) ? "相等" : "不相等");
    
    // 创建不同的Token进行比较
    Stru_Token_t* different = F_token_create(
        Eum_TOKEN_KEYWORD_FOR,
        "循环",
        0,
        10,
        5
    );
    
    if (different != NULL) {
        printf("不同的Token: ");
        F_token_print(different, true, false);
        printf("  原始 == 不同: %s\n", F_token_equals(original, different) ? "相等" : "不相等");
        F_token_destroy(different);
    }
    
    // 清理
    F_token_destroy(original);
    F_token_destroy(copy);
    
    printf("\n");
}
```

### 示例8：Token字符串表示

```c
#include <stdio.h>
#include <string.h>
#include "CN_token.h"

void token_string_representation()
{
    printf("=== Token字符串表示 ===\n");
    
    // 创建Token
    Stru_Token_t* token = F_token_create(
        Eum_TOKEN_KEYWORD_FUNCTION,
        "函数",
        0,
        20,
        8
    );
    
    if (token == NULL) {
        printf("创建Token失败\n");
        return;
    }
    
    // 方法1：使用F_token_print
    printf("方法1 - 使用F_token_print:\n");
    printf("  ");
    F_token_print(token, true, false);
    
    // 方法2：使用F_token_to_string（如果实现）
    // 注意：当前实现中F_token_to_string函数可能未实现
    // 这里展示如何使用缓冲区
    
    printf("方法2 - 手动构建字符串:\n");
    const char* type_name = F_token_type_to_string(token->type);
    const char* category = F_token_get_category(token->type);
    
    char buffer[256];
    if (token->lexeme != NULL) {
        snprintf(buffer, sizeof(buffer), 
                 "Token[%s]: '%s' (%s) 位置: %d:%d",
                 category, token->lexeme, type_name,
                 token->line_number, token->column_number);
    } else {
        snprintf(buffer, sizeof(buffer),
                 "Token[%s]: (%s) 位置: %d:%d",
                 category, type_name,
                 token->line_number, token->column_number);
    }
    
    printf("  %s\n", buffer);
    
    // 清理
    F_token_destroy(token);
    
    printf("\n");
}
```

## 运算符属性示例

### 示例9：运算符优先级和结合性

```c
#include <stdio.h>
#include "CN_token.h"

void operator_properties()
{
    printf("=== 运算符优先级和结合性 ===\n");
    
    // 测试各种运算符
    struct {
        enum Eum_TokenType type;
        const char* name;
    } operators[] = {
        {Eum_TOKEN_OPERATOR_MULTIPLY, "*"},
        {Eum_TOKEN_OPERATOR_DIVIDE, "/"},
        {Eum_TOKEN_OPERATOR_PLUS, "+"},
        {Eum_TOKEN_OPERATOR_MINUS, "-"},
        {Eum_TOKEN_OPERATOR_LESS, "<"},
        {Eum_TOKEN_OPERATOR_GREATER, ">"},
        {Eum_TOKEN_OPERATOR_EQUAL, "=="},
        {Eum_TOKEN_OPERATOR_NOT_EQUAL, "!="},
        {Eum_TOKEN_OPERATOR_ASSIGN, "="},
        {Eum_TOKEN_DOT, "."},
        {Eum_TOKEN_LEFT_PAREN, "("}
    };
    
    int count = sizeof(operators) / sizeof(operators[0]);
    
    printf("运算符优先级表（值越小优先级越高）:\n");
    printf("====================================\n");
    
    for (int i = 0; i < count; i++) {
        enum Eum_TokenType type = operators[i].type;
        int precedence = F_token_get_precedence(type);
        int associativity = F_token_get_associativity(type);
        
        const char* assoc_str = "未知";
        if (associativity == 0) assoc_str = "左结合";
        else if (associativity == 1) assoc_str = "右结合";
        
        printf("%-4s : 优先级=%2d, 结合性=%s\n", 
               operators[i].name, precedence, assoc_str);
    }
    
    printf("\n");
    
    // 演示优先级计算
    printf("优先级计算示例:\n");
    printf("表达式: a + b * c\n");
    printf("  '+' 优先级: %d\n", F_token_get_precedence(Eum_TOKEN_OPERATOR_PLUS));
    printf("  '*' 优先级: %d\n", F_token_get_precedence(Eum_TOKEN_OPERATOR_MULTIPLY));
    printf("  结果: 先计算 b * c，然后计算 a + (b * c)\n");
    
    printf("\n");
}
```

### 示例10：使用优先级进行表达式解析

```c
#include <stdio.h>
#include "CN_token.h"

void expression_parsing_example()
{
    printf("=== 使用优先级进行表达式解析 ===\n");
    
    // 模拟表达式 Token 序列
    enum Eum_TokenType expression[] = {
        Eum_TOKEN_IDENTIFIER,        // a
        Eum_TOKEN_OPERATOR_PLUS,     // +
        Eum_TOKEN_IDENTIFIER,        // b
        Eum_TOKEN_OPERATOR_MULTIPLY, // *
        Eum_TOKEN_IDENTIFIER,        // c
        Eum_TOKEN_OPERATOR_MINUS,    // -
        Eum_TOKEN_IDENTIFIER,        // d
        Eum_TOKEN_OPERATOR_DIVIDE,   // /
        Eum_TOKEN_IDENTIFIER         // e
    };
    
    const char* token_names[] = {"a", "+", "b", "*", "c", "-", "d", "/", "e"};
    int count = sizeof(expression) / sizeof(expression[0]);
    
    printf("表达式: ");
    for (int i = 0; i < count; i++) {
        printf("%s ", token_names[i]);
    }
    printf("\n\n");
    
    printf("解析步骤:\n");
    
    // 简单的手动解析演示
    for (int i = 1; i < count; i += 2) { // 只处理运算符位置
        if (F_token_is_operator(expression[i])) {
            int precedence = F_token_get_precedence(expression[i]);
            printf("  运算符 '%s': 优先级=%d\n", token_names[i], precedence);
        }
    }
    
    printf("\n根据优先级构建语法树:\n");
    printf("  1. b * c (优先级: 3)\n");
    printf("  2. d / e (优先级: 3)\n"); 
    printf("  3. a + (b * c) (优先级: 4)\n");
    printf("  4. (a + (b * c)) - (d / e) (优先级: 4)\n");
    
    printf("\n");
}
```
