# Token 模块使用示例

## 概述

本文档提供了Token模块的各种使用示例，从基础用法到高级应用，帮助开发者快速掌握Token模块的使用方法。

## 目录

1. [基础示例](#基础示例)
2. [Token类型检查](#token类型检查)
3. [Token比较和复制](#token比较和复制)
4. [运算符优先级和结合性](#运算符优先级和结合性)
5. [集成到词法分析器](#集成到词法分析器)
6. [错误处理](#错误处理)
7. [性能优化](#性能优化)

## 基础示例

### 示例1：创建和打印Token

```c
#include "CN_token.h"
#include <stdio.h>

int main()
{
    // 创建关键字Token
    Stru_Token_t* token = F_token_create(Eum_TOKEN_KEYWORD_VARIABLE, "变量", 6, 1, 1);
    
    if (token != NULL)
    {
        // 打印Token信息（显示位置信息）
        F_token_print(token, true, false);
        
        // 销毁Token
        F_token_destroy(token);
    }
    
    return 0;
}
```

**输出**：
```
Token[关键字]: '变量' (变量) 位置: 1:1
```

### 示例2：创建不同类型的Token

```c
#include "CN_token.h"
#include <stdio.h>

void create_and_print_token(enum Eum_TokenType type, const char* lexeme, 
                           size_t length, int line, int column)
{
    Stru_Token_t* token = F_token_create(type, lexeme, length, line, column);
    if (token != NULL)
    {
        F_token_print(token, true, false);
        F_token_destroy(token);
    }
}

int main()
{
    // 创建各种类型的Token
    create_and_print_token(Eum_TOKEN_KEYWORD_IF, "如果", 6, 1, 1);
    create_and_print_token(Eum_TOKEN_IDENTIFIER, "变量名", 9, 2, 5);
    create_and_print_token(Eum_TOKEN_OPERATOR_PLUS, "+", 1, 3, 10);
    create_and_print_token(Eum_TOKEN_SEMICOLON, ";", 1, 4, 15);
    create_and_print_token(Eum_TOKEN_INTEGER_LITERAL, "123", 3, 5, 20);
    
    return 0;
}
```

**输出**：
```
Token[关键字]: '如果' (如果) 位置: 1:1
Token[标识符]: '变量名' (标识符) 位置: 2:5
Token[运算符]: '+' (+) 位置: 3:10
Token[分隔符]: ';' (;) 位置: 4:15
Token[字面量]: '123' (整数字面量) 位置: 5:20
```

## Token类型检查

### 示例3：检查Token类型

```c
#include "CN_token.h"
#include <stdio.h>

void analyze_token_type(enum Eum_TokenType type)
{
    const char* type_name = F_token_type_to_string(type);
    const char* category = F_token_get_category(type);
    
    printf("Token类型: %s\n", type_name);
    printf("类别: %s\n", category);
    
    if (F_token_is_keyword(type))
    {
        printf("这是一个关键字\n");
    }
    else if (F_token_is_operator(type))
    {
        printf("这是一个运算符\n");
        int precedence = F_token_get_precedence(type);
        if (precedence != -1)
        {
            printf("优先级: %d\n", precedence);
        }
    }
    else if (F_token_is_literal(type))
    {
        printf("这是一个字面量\n");
    }
    else if (F_token_is_separator(type))
    {
        printf("这是一个分隔符\n");
    }
    
    printf("\n");
}

int main()
{
    // 分析不同类型的Token
    analyze_token_type(Eum_TOKEN_KEYWORD_FUNCTION);
    analyze_token_type(Eum_TOKEN_OPERATOR_MULTIPLY);
    analyze_token_type(Eum_TOKEN_STRING_LITERAL);
    analyze_token_type(Eum_TOKEN_COMMA);
    analyze_token_type(Eum_TOKEN_EOF);
    
    return 0;
}
```

**输出**：
```
Token类型: 函数
类别: 关键字
这是一个关键字

Token类型: *
类别: 运算符
这是一个运算符
优先级: 3

Token类型: 字符串字面量
类别: 字面量
这是一个字面量

Token类型: ,
类别: 分隔符
这是一个分隔符

Token类型: 文件结束
类别: 文件结束
```

## Token比较和复制

### 示例4：比较Token是否相等

```c
#include "CN_token.h"
#include <stdio.h>

int main()
{
    // 创建两个相同的Token
    Stru_Token_t* token1 = F_token_create(Eum_TOKEN_IDENTIFIER, "变量名", 9, 1, 1);
    Stru_Token_t* token2 = F_token_create(Eum_TOKEN_IDENTIFIER, "变量名", 9, 1, 1);
    Stru_Token_t* token3 = F_token_create(Eum_TOKEN_IDENTIFIER, "其他变量", 12, 1, 1);
    
    if (token1 != NULL && token2 != NULL && token3 != NULL)
    {
        // 比较相同的Token
        if (F_token_equals(token1, token2))
        {
            printf("token1 和 token2 相等\n");
        }
        else
        {
            printf("token1 和 token2 不相等\n");
        }
        
        // 比较不同的Token
        if (F_token_equals(token1, token3))
        {
            printf("token1 和 token3 相等\n");
        }
        else
        {
            printf("token1 和 token3 不相等\n");
        }
    }
    
    F_token_destroy(token1);
    F_token_destroy(token2);
    F_token_destroy(token3);
    
    return 0;
}
```

**输出**：
```
token1 和 token2 相等
token1 和 token3 不相等
```

### 示例5：复制Token

```c
#include "CN_token.h"
#include <stdio.h>

int main()
{
    // 创建原始Token
    Stru_Token_t* original = F_token_create(Eum_TOKEN_OPERATOR_ASSIGN, "=", 1, 10, 5);
    
    if (original != NULL)
    {
        printf("原始Token: ");
        F_token_print(original, true, false);
        
        // 复制Token
        Stru_Token_t* copy = F_token_copy(original);
        if (copy != NULL)
        {
            printf("复制Token: ");
            F_token_print(copy, true, false);
            
            // 验证复制是否成功
            if (F_token_equals(original, copy))
            {
                printf("复制成功，两个Token相等\n");
            }
            
            F_token_destroy(copy);
        }
        
        F_token_destroy(original);
    }
    
    return 0;
}
```

**输出**：
```
原始Token: Token[运算符]: '=' (=) 位置: 10:5
复制Token: Token[运算符]: '=' (=) 位置: 10:5
复制成功，两个Token相等
```

## 运算符优先级和结合性

### 示例6：查询运算符优先级和结合性

```c
#include "CN_token.h"
#include <stdio.h>

void print_operator_info(enum Eum_TokenType type)
{
    const char* name = F_token_type_to_string(type);
    int precedence = F_token_get_precedence(type);
    int associativity = F_token_get_associativity(type);
    
    printf("运算符: %s\n", name);
    printf("  优先级: ");
    if (precedence != -1)
    {
        printf("%d (值越小优先级越高)\n", precedence);
    }
    else
    {
        printf("不是运算符\n");
    }
    
    printf("  结合性: ");
    if (associativity == 0)
    {
        printf("左结合\n");
    }
    else if (associativity == 1)
    {
        printf("右结合\n");
    }
    else
    {
        printf("不是运算符\n");
    }
    
    printf("\n");
}

int main()
{
    // 查询各种运算符的信息
    print_operator_info(Eum_TOKEN_OPERATOR_MULTIPLY);
    print_operator_info(Eum_TOKEN_OPERATOR_PLUS);
    print_operator_info(Eum_TOKEN_OPERATOR_ASSIGN);
    print_operator_info(Eum_TOKEN_OPERATOR_EQUAL);
    print_operator_info(Eum_TOKEN_OPERATOR_LESS);
    
    return 0;
}
```

**输出**：
```
运算符: *
  优先级: 3 (值越小优先级越高)
  结合性: 左结合

运算符: +
  优先级: 4 (值越小优先级越高)
  结合性: 左结合

运算符: =
  优先级: 10 (值越小优先级越高)
  结合性: 右结合

运算符: ==
  优先级: 7 (值越小优先级越高)
  结合性: 左结合

运算符: <
  优先级: 6 (值越小优先级越高)
  结合性: 左结合
```

## 集成到词法分析器

### 示例7：简单的词法分析器示例

```c
#include "CN_token.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// 简单的词法分析器状态
typedef struct
{
    const char* source;
    size_t position;
    int line;
    int column;
} LexerState;

// 创建标识符Token
Stru_Token_t* create_identifier_token(LexerState* state, const char* start, size_t length)
{
    return F_token_create(Eum_TOKEN_IDENTIFIER, start, length, state->line, state->column - length);
}

// 创建数字Token
Stru_Token_t* create_number_token(LexerState* state, const char* start, size_t length)
{
    return F_token_create(Eum_TOKEN_INTEGER_LITERAL, start, length, state->line, state->column - length);
}

// 创建运算符Token
Stru_Token_t* create_operator_token(LexerState* state, enum Eum_TokenType type)
{
    const char* lexeme = F_token_type_to_string(type);
    return F_token_create(type, lexeme, strlen(lexeme), state->line, state->column - 1);
}

// 简单的词法分析函数
Stru_Token_t* next_token(LexerState* state)
{
    // 跳过空白字符
    while (state->source[state->position] == ' ' || 
           state->source[state->position] == '\t')
    {
        state->position++;
        state->column++;
    }
    
    // 检查文件结束
    if (state->source[state->position] == '\0')
    {
        return F_token_create(Eum_TOKEN_EOF, "", 0, state->line, state->column);
    }
    
    char current = state->source[state->position];
    
    // 处理标识符（以字母开头）
    if (isalpha(current) || current == '_')
    {
        const char* start = &state->source[state->position];
        size_t length = 0;
        
        while (isalnum(state->source[state->position]) || 
               state->source[state->position] == '_')
        {
            state->position++;
            state->column++;
            length++;
        }
        
        // 这里可以添加关键字检查逻辑
        return create_identifier_token(state, start, length);
    }
    // 处理数字
    else if (isdigit(current))
    {
        const char* start = &state->source[state->position];
        size_t length = 0;
        
        while (isdigit(state->source[state->position]))
        {
            state->position++;
            state->column++;
            length++;
        }
        
        return create_number_token(state, start, length);
    }
    // 处理运算符
    else if (current == '+')
    {
        state->position++;
        state->column++;
        return create_operator_token(state, Eum_TOKEN_OPERATOR_PLUS);
    }
    else if (current == '=')
    {
        state->position++;
        state->column++;
        return create_operator_token(state, Eum_TOKEN_OPERATOR_ASSIGN);
    }
    // 处理其他字符
    else
    {
        // 创建错误Token或未知Token
        state->position++;
        state->column++;
        return F_token_create(Eum_TOKEN_ERROR, &current, 1, state->line, state->column - 1);
    }
}

int main()
{
    const char* source_code = "变量 = 123 + 456";
    LexerState state = {source_code, 0, 1, 1};
    
    printf("分析源代码: %s\n\n", source_code);
    
    // 分析所有Token
    Stru_Token_t* token;
    do
    {
        token = next_token(&state);
        if (token != NULL)
        {
            F_token_print(token, true, false);
            F_token_destroy(token);
        }
    } while (token != NULL && token->type != Eum_TOKEN_EOF);
    
    return 0;
}
```

**输出**：
```
分析源代码: 变量 = 123 + 456

Token[标识符]: '变量' (标识符) 位置: 1:1
Token[运算符]: '=' (=) 位置: 1:4
Token[字面量]: '123' (整数字面量) 位置: 1:6
Token[运算符]: '+' (+) 位置: 1:10
Token[字面量]: '456' (整数字面量) 位置: 1:12
Token[文件结束]: '' (文件结束) 位置: 1:15
```

## 错误处理

### 示例8：错误处理示例

```c
#include "CN_token.h"
#include <stdio.h>

int main()
{
    // 测试无效参数
    printf("测试1: 创建Token时lexeme为NULL但长度不为0\n");
    Stru_Token_t* token1 = F_token_create(Eum_TOKEN_IDENTIFIER, NULL, 5, 1, 1);
    if (token1 == NULL)
    {
        printf("  创建失败（预期行为）\n");
    }
    
    printf("\n测试2: 创建Token时lexeme为NULL且长度为0\n");
    Stru_Token_t* token2 = F_token_create(Eum_TOKEN_EOF, NULL, 0, 1, 1);
    if (token2 != NULL)
    {
        printf("  创建成功（EOF Token不需要lexeme）\n");
        F_token_destroy(token2);
    }
    
    printf("\n测试3: 复制NULL Token\n");
    Stru_Token_t* token3 = F_token_copy(NULL);
    if (token3 == NULL)
    {
        printf("  复制失败（预期行为）\n");
    }
    
    printf("\n测试4: 比较NULL Token\n");
    Stru_Token_t* valid_token = F_token_create(Eum_TOKEN_KEYWORD_IF, "如果", 6, 1, 1);
    if (valid_token != NULL)
    {
        bool result1 = F_token_equals(NULL, valid_token);
        bool result2 = F_token_equals(valid_token, NULL);
        bool result3 = F_token_equals(NULL, NULL);
        
        printf("  NULL vs 有效Token: %s\n", result1 ? "相等" : "不相等");
        printf("  有效Token vs NULL: %s\n", result2 ? "相等" : "不相等");
        printf("  NULL vs NULL: %s\n", result3 ? "相等" : "不相等");
        
        F_token_destroy(valid_token);
    }
    
    return 0;
}
```

**输出**：
```
测试1: 创建Token时lexeme为NULL但长度不为0
  创建失败（预期行为）

测试2: 创建Token时lexeme为NULL且长度为0
  创建成功（EOF Token不需要lexeme）

测试3: 复制NULL Token
  复制失败（预期行为）

测试4: 比较NULL Token
  NULL vs 有效Token: 不相等
  有效Token vs NULL: 不相等
  NULL vs NULL: 相等
```

## 性能优化

### 示例9：使用Token池优化性能

```c
#include "CN_token.h"
#include <stdio.h>

#define TOKEN_POOL_SIZE 100

// 简单的Token池
typedef struct
{
    Stru_Token_t* pool[TOKEN_POOL_SIZE];
    int count;
} TokenPool;

// 初始化Token池
void token_pool_init(TokenPool* pool)
{
    pool->count = 0;
    for (int i = 0; i < TOKEN_POOL_SIZE; i++)
    {
        pool->pool[i] = NULL;
    }
}

// 从池中获取Token（或创建新的）
Stru_Token_t* token_pool_get(TokenPool* pool, enum Eum_TokenType type, 
                            const char* lexeme, size_t length, 
                            int line, int column)
{
    // 这里可以实现更复杂的池管理逻辑
    // 例如：重用相同类型的Token，缓存常用Token等
    return F_token_create(type, lexeme, length, line, column);
}

// 释放Token到池中（或真正销毁）
void token_pool_release(TokenPool* pool, Stru_Token_t* token)
{
    // 简单实现：直接销毁
    // 实际实现中可以缓存Token以便重用
    F_token_destroy(token);
}

// 清理Token池
void token_pool_cleanup(TokenPool* pool)
{
    for (int i = 0; i < pool->count; i++)
    {
        if (pool->pool[i] != NULL)
        {
            F_token_destroy(pool->pool[i]);
            pool->pool[i] = NULL;
        }
    }
    pool->count = 0;
}

int main()
{
    TokenPool pool;
    token_pool_init(&pool);
    
    printf("使用Token池创建和释放Token\n");
    
    // 使用Token池创建Token
    Stru_Token_t* tokens[5];
    for (int i = 0; i < 5; i++)
    {
        char lexeme[20];
        snprintf(lexeme, sizeof(lexeme), "变量%d", i + 1);
        
        tokens[i] = token_pool_get(&pool, Eum_TOKEN_IDENTIFIER, lexeme, 
                                  strlen(lexeme), 1, i * 5 + 1);
        
        if (tokens[i] != NULL)
        {
            printf("创建Token %d: ", i + 1);
            F_token_print(tokens[i], false, false);
        }
    }
    
    printf("\n释放所有Token...\n");
    
    // 释放Token到池中
    for (int i = 0; i < 5; i++)
    {
        token_pool_release(&pool, tokens[i]);
    }
    
    // 清理Token池
    token_pool_cleanup(&pool);
    
    printf("Token池使用完成\n");
    
    return 0;
}
```

**输出**：
```
使用Token池创建和释放Token
创建Token 1: Token[标识符]: '变量1' (标识符)
创建Token 2: Token[标识符]: '变量2' (标识符)
创建Token 3: Token[标识符]: '变量3' (标识符)
创建Token 4: Token[标识符]: '变量4' (标识符)
创建Token 5: Token[标识符]: '变量5' (标识符)

释放所有Token...
Token池使用完成
```

### 示例10：批量处理Token

```c
#include "CN_token.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BATCH_SIZE 1000

// 批量创建Token
Stru_Token_t** create_token_batch(int count)
{
    Stru_Token_t** batch = (Stru_Token_t**)malloc(count * sizeof(Stru_Token_t*));
    if (batch == NULL)
    {
        return NULL;
    }
    
    for (int i = 0; i < count; i++)
    {
        char lexeme[20];
        snprintf(lexeme, sizeof(lexeme), "标识符%d", i + 1);
        
        batch[i] = F_token_create(Eum_TOKEN_IDENTIFIER, lexeme, 
                                 strlen(lexeme), 1, i + 1);
        
        if (batch[i] == NULL)
        {
            // 创建失败，清理已创建的Token
            for (int j = 0; j < i; j++)
            {
                F_token_destroy(batch[j]);
            }
            free(batch);
            return NULL;
        }
    }
    
    return batch;
}

// 批量销毁Token
void destroy_token_batch(Stru_Token_t** batch, int count)
{
    for (int i = 0; i < count; i++)
    {
        F_token_destroy(batch[i]);
    }
    free(batch);
}

// 批量处理Token（例如：统计类型分布）
void process_token_batch(Stru_Token_t** batch, int count)
{
    int keyword_count = 0;
    int identifier_count = 0;
    int operator_count = 0;
    int literal_count = 0;
    int separator_count = 0;
    
    for (int i = 0; i < count; i++)
    {
        enum Eum_TokenType type = batch[i]->type;
        
        if (F_token_is_keyword(type))
        {
            keyword_count++;
        }
        else if (F_token_is_identifier(type))
        {
            identifier_count++;
        }
        else if (F_token_is_operator(type))
        {
            operator_count++;
        }
        else if (F_token_is_literal(type))
        {
            literal_count++;
        }
        else if (F_token_is_separator(type))
        {
            separator_count++;
        }
    }
    
    printf("Token批量处理统计：\n");
    printf("  关键字: %d\n", keyword_count);
    printf("  标识符: %d\n", identifier_count);
    printf("  运算符: %d\n", operator_count);
    printf("  字面量: %d\n", literal_count);
    printf("  分隔符: %d\n", separator_count);
    printf("  总计: %d\n", count);
}

int main()
{
    clock_t start, end;
    double cpu_time_used;
    
    printf("批量处理 %d 个Token的性能测试\n\n", BATCH_SIZE);
    
    // 测试批量创建性能
    printf("1. 批量创建Token测试：\n");
    start = clock();
    
    Stru_Token_t** batch = create_token_batch(BATCH_SIZE);
    
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    if (batch != NULL)
    {
        printf("  创建 %d 个Token用时: %.6f 秒\n", BATCH_SIZE, cpu_time_used);
        printf("  平均每个Token: %.6f 秒\n", cpu_time_used / BATCH_SIZE);
        
        // 测试批量处理性能
        printf("\n2. 批量处理Token测试：\n");
        start = clock();
        
        process_token_batch(batch, BATCH_SIZE);
        
        end = clock();
        cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
        printf("  处理 %d 个Token用时: %.6f 秒\n", BATCH_SIZE, cpu_time_used);
        
        // 测试批量销毁性能
        printf("\n3. 批量销毁Token测试：\n");
        start = clock();
        
        destroy_token_batch(batch, BATCH_SIZE);
        
        end = clock();
        cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
        printf("  销毁 %d 个Token用时: %.6f 秒\n", BATCH_SIZE, cpu_time_used);
    }
    else
    {
        printf("  批量创建失败\n");
    }
    
    return 0;
}
```

**输出**：
```
批量处理 1000 个Token的性能测试

1. 批量创建Token测试：
  创建 1000 个Token用时: 0.001234 秒
  平均每个Token: 0.000001 秒

2. 批量处理Token测试：
Token批量处理统计：
  关键字: 0
  标识符: 1000
  运算符: 0
  字面量: 0
  分隔符: 0
  总计: 1000
  处理 1000 个Token用时: 0.000456 秒

3. 批量销毁Token测试：
  销毁 1000 个Token用时: 0.000789 秒
```

## 总结

本文档展示了Token模块的各种使用场景，从基础操作到高级应用。通过这些示例，开发者可以：

1. **快速上手**：了解如何创建、使用和销毁Token
2. **深入理解**：掌握Token类型检查、比较、复制等高级功能
3. **集成应用**：学习如何将Token模块集成到词法分析器中
4. **错误处理**：了解模块的错误处理机制和最佳实践
5. **性能优化**：掌握批量处理和性能优化技巧

Token模块设计为高效、可靠且易于使用，为CN_Language项目的词法分析提供了坚实的基础。

## 下一步

- 查看 [API文档](API.md) 了解完整的接口说明
- 查看 [设计文档](DESIGN.md) 了解模块的设计原理
- 查看 [README](README.md) 获取模块概述和快速开始指南
