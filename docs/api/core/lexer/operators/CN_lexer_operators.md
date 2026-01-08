# CN_Language 词法分析器运算符模块 API 文档

## 概述

运算符模块负责识别CN_Language的各种运算符，包括算术运算符、比较运算符、赋值运算符等。它提供高效的运算符查找和识别功能，支持将运算符词素映射到相应的令牌类型。

## 支持的运算符

### 运算符类别
1. **算术运算符**：`+`, `-`, `*`, `/`, `%`
2. **比较运算符**：`==`, `!=`, `<`, `>`, `<=`, `>=`
3. **赋值运算符**：`=`, `+=`, `-=`, `*=`, `/=`, `%=`
4. **逻辑运算符**：`&&`, `||`, `!`
5. **位运算符**：`&`, `|`, `^`, `~`, `<<`, `>>`
6. **其他运算符**：`++`, `--`, `?:`, `->`, `.`

## API 参考

### 运算符识别函数

#### F_identify_operator

识别运算符类型。

**函数签名：**
```c
Eum_TokenType F_identify_operator(const char* lexeme);
```

**参数：**
- `lexeme`：运算符词素字符串

**返回值：**
- `Eum_TokenType`：运算符对应的令牌类型
- `Eum_TOKEN_ERROR`：如果不是有效运算符

**示例：**
```c
const char* lexeme = "+";
Eum_TokenType type = F_identify_operator(lexeme);
if (type != Eum_TOKEN_ERROR) {
    printf("运算符: %s -> %d\n", lexeme, type);
}
```

#### F_is_operator_char

检查字符是否为运算符字符。

**函数签名：**
```c
bool F_is_operator_char(char c);
```

**参数：**
- `c`：要检查的字符

**返回值：**
- `true`：字符是运算符字符
- `false`：字符不是运算符字符

**支持的运算符字符：**
```c
'+', '-', '*', '/', '%', '=', '!', '<', '>', '&', '|', '^', '~'
```

#### F_is_operator

检查词素是否为有效运算符。

**函数签名：**
```c
bool F_is_operator(const char* lexeme);
```

**参数：**
- `lexeme`：运算符词素字符串

**返回值：**
- `true`：是有效运算符
- `false`：不是有效运算符

### 初始化函数

#### F_initialize_operator_table

初始化运算符查找表。

**函数签名：**
```c
void F_initialize_operator_table(void);
```

**说明：**
- 内部使用，通常不需要手动调用
- 在第一次运算符识别时自动初始化
- 线程安全的延迟初始化

#### F_destroy_operator_table

清理运算符查找表。

**函数签名：**
```c
void F_destroy_operator_table(void);
```

**说明：**
- 在程序结束时清理资源
- 通常不需要手动调用

## 使用示例

### 基本运算符识别

```c
#include "src/core/lexer/operators/CN_lexer_operators.h"

void identify_operators(void) {
    const char* test_cases[] = {
        "+",        // 算术运算符
        "==",       // 比较运算符
        "=",        // 赋值运算符
        "&&",       // 逻辑运算符
        "++",       // 其他运算符
        "@"         // 无效运算符
    };
    
    for (int i = 0; i < 6; i++) {
        const char* lexeme = test_cases[i];
        Eum_TokenType type = F_identify_operator(lexeme);
        
        if (type == Eum_TOKEN_ERROR) {
            printf("%s: 无效运算符\n", lexeme);
        } else {
            printf("%s: 运算符 (类型: %d)\n", lexeme, type);
        }
    }
}
```

### 运算符字符检查

```c
#include "src/core/lexer/operators/CN_lexer_operators.h"

void check_operator_chars(const char* str) {
    printf("检查字符串: %s\n", str);
    
    for (int i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        if (F_is_operator_char(c)) {
            printf("  位置 %d: '%c' 是运算符字符\n", i, c);
        }
    }
}
```

### 运算符分类

```c
#include "src/core/lexer/operators/CN_lexer_operators.h"

void categorize_operator(const char* lexeme) {
    Eum_TokenType type = F_identify_operator(lexeme);
    
    switch (type) {
        case Eum_TOKEN_PLUS:
        case Eum_TOKEN_MINUS:
        case Eum_TOKEN_MULTIPLY:
        case Eum_TOKEN_DIVIDE:
            printf("%s: 算术运算符\n", lexeme);
            break;
            
        case Eum_TOKEN_EQUAL:
        case Eum_TOKEN_NOT_EQUAL:
        case Eum_TOKEN_LESS:
        case Eum_TOKEN_GREATER:
            printf("%s: 比较运算符\n", lexeme);
            break;
            
        case Eum_TOKEN_ASSIGN:
        case Eum_TOKEN_PLUS_ASSIGN:
        case Eum_TOKEN_MINUS_ASSIGN:
            printf("%s: 赋值运算符\n", lexeme);
            break;
            
        case Eum_TOKEN_LOGICAL_AND:
        case Eum_TOKEN_LOGICAL_OR:
        case Eum_TOKEN_LOGICAL_NOT:
            printf("%s: 逻辑运算符\n", lexeme);
            break;
            
        default:
            if (type != Eum_TOKEN_ERROR) {
                printf("%s: 其他运算符\n", lexeme);
            }
            break;
    }
}
```

## 性能考虑

### 查找算法
- **长度优先**：首先尝试匹配多字符运算符
- **分组查找**：按长度分组存储运算符
- **精确匹配**：确保准确的运算符识别

### 内存使用
- 运算符表：约1KB静态内存
- 查找表：约0.5KB动态内存
- 字符串常量：共享的只读内存

### 初始化开销
- 延迟初始化：第一次使用时初始化
- 一次性构建：构建后重复使用
- 线程安全：支持多线程环境

## 扩展指南

### 添加新运算符
要添加新的运算符：

1. 在`CN_token_types.h`中添加枚举值
2. 在运算符表中添加条目
3. 更新识别函数逻辑
4. 添加相应的测试用例

**示例：**
```c
// 在CN_token_types.h中添加
Eum_TOKEN_NEW_OPERATOR,

// 在运算符表中添加
{"**", Eum_TOKEN_NEW_OPERATOR},  // 幂运算符
```

### 自定义运算符表
可以创建自定义的运算符表：

```c
// 自定义运算符条目
typedef struct {
    const char* operator;
    Eum_TokenType type;
} CustomOperatorEntry;

// 自定义运算符表
static const CustomOperatorEntry custom_operators[] = {
    {"**", Eum_TOKEN_MULTIPLY},  // 幂运算符
    {"//", Eum_TOKEN_DIVIDE},    // 整除运算符
    {NULL, Eum_TOKEN_ERROR}      // 结束标记
};

// 自定义识别函数
Eum_TokenType custom_identify_operator(const char* lexeme) {
    for (int i = 0; custom_operators[i].operator != NULL; i++) {
        if (strcmp(lexeme, custom_operators[i].operator) == 0) {
            return custom_operators[i].type;
        }
    }
    return F_identify_operator(lexeme); // 回退到标准识别
}
```

## 错误处理

### 边界情况处理
1. **空指针**：返回`Eum_TOKEN_ERROR`
2. **空字符串**：返回`Eum_TOKEN_ERROR`
3. **超长词素**：快速返回`Eum_TOKEN_ERROR`（运算符最长2字符）

### 错误示例
```c
// 安全的运算符识别
Eum_TokenType safe_identify_operator(const char* lexeme) {
    if (lexeme == NULL || lexeme[0] == '\0') {
        return Eum_TOKEN_ERROR;
    }
    
    // 长度过滤（运算符最长2字符）
    size_t len = strlen(lexeme);
    if (len > 2) {
        return Eum_TOKEN_ERROR;
    }
    
    return F_identify_operator(lexeme);
}
```

## 运算符优先级

### 优先级表
| 优先级 | 运算符 | 描述 |
|--------|--------|------|
| 1 | `++`, `--`, `!`, `~` | 一元运算符 |
| 2 | `*`, `/`, `%` | 乘性运算符 |
| 3 | `+`, `-` | 加性运算符 |
| 4 | `<<`, `>>` | 移位运算符 |
| 5 | `<`, `<=`, `>`, `>=` | 关系运算符 |
| 6 | `==`, `!=` | 相等运算符 |
| 7 | `&` | 按位与 |
| 8 | `^` | 按位异或 |
| 9 | `|` | 按位或 |
| 10 | `&&` | 逻辑与 |
| 11 | `||` | 逻辑或 |
| 12 | `?:` | 条件运算符 |
| 13 | `=`, `+=`, `-=`, 等 | 赋值运算符 |
| 14 | `,` | 逗号运算符 |

### 优先级查询函数
```c
// 获取运算符优先级
int get_operator_precedence(Eum_TokenType type) {
    switch (type) {
        case Eum_TOKEN_INCREMENT:
        case Eum_TOKEN_DECREMENT:
        case Eum_TOKEN_LOGICAL_NOT:
        case Eum_TOKEN_BITWISE_NOT:
            return 1;
        // ... 其他优先级
        default:
            return 0; // 非运算符或未知
    }
}
```

## 相关文档

- [令牌类型定义](../../token/CN_token_types.md)
- [扫描器模块 API 文档](../scanner/CN_lexer_scanner.md)
- [令牌扫描器模块 API 文档](../token_scanners/CN_lexer_token_scanners.md)

## 修订历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-08 | 初始版本发布，支持完整运算符集 |
| 1.0.1 | 2026-01-08 | 修复运算符表初始化问题 |

## 版权声明

版权所有 © 2026 CN_Language项目团队。保留所有权利。

本文档是CN_Language项目的一部分，遵循项目许可证条款。
