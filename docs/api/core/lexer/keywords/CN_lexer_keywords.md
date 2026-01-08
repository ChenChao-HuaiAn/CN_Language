# CN_Language 词法分析器关键字模块 API 文档

## 概述

关键字模块负责识别CN_Language的70个中文关键字。它提供高效的关键字查找和识别功能，支持将标识符词素映射到相应的令牌类型。

## 关键字类别

### 70个中文关键字分类
1. **变量和类型声明**：变量、常量、类型、类、对象等
2. **控制结构**：如果、否则、当、循环、中断、继续等
3. **函数相关**：函数、返回、主程序、无等
4. **逻辑运算符**：与、或、非等
5. **字面量**：真、假、空等
6. **模块系统**：模块、导入、导出等
7. **运算符关键字**：加、减、乘、除等
8. **类型关键字**：整数、浮点数、字符串、布尔等
9. **其他关键字**：尝试、捕获、抛出等

## API 参考

### 关键字识别函数

#### F_identify_keyword

识别关键字类型。

**函数签名：**
```c
Eum_TokenType F_identify_keyword(const char* lexeme);
```

**参数：**
- `lexeme`：标识符词素字符串

**返回值：**
- `Eum_TokenType`：关键字对应的令牌类型
- `Eum_TOKEN_IDENTIFIER`：如果不是关键字

**示例：**
```c
const char* lexeme = "变量";
Eum_TokenType type = F_identify_keyword(lexeme);
if (type != Eum_TOKEN_IDENTIFIER) {
    printf("关键字: %s -> %d\n", lexeme, type);
}
```

#### F_is_keyword

检查词素是否为关键字。

**函数签名：**
```c
bool F_is_keyword(const char* lexeme);
```

**参数：**
- `lexeme`：标识符词素字符串

**返回值：**
- `true`：是关键字
- `false`：不是关键字

**示例：**
```c
if (F_is_keyword("如果")) {
    printf("'如果' 是关键字\n");
}
```

### 初始化函数

#### F_initialize_keyword_table

初始化关键字查找表。

**函数签名：**
```c
void F_initialize_keyword_table(void);
```

**说明：**
- 内部使用，通常不需要手动调用
- 在第一次关键字识别时自动初始化
- 线程安全的延迟初始化

#### F_destroy_keyword_table

清理关键字查找表。

**函数签名：**
```c
void F_destroy_keyword_table(void);
```

**说明：**
- 在程序结束时清理资源
- 通常不需要手动调用

## 使用示例

### 基本关键字识别

```c
#include "src/core/lexer/keywords/CN_lexer_keywords.h"

void identify_keywords(void) {
    const char* test_cases[] = {
        "变量",     // 关键字
        "x",        // 标识符
        "如果",     // 关键字
        "函数",     // 关键字
        "未知词"    // 标识符
    };
    
    for (int i = 0; i < 5; i++) {
        const char* lexeme = test_cases[i];
        Eum_TokenType type = F_identify_keyword(lexeme);
        
        if (type == Eum_TOKEN_IDENTIFIER) {
            printf("%s: 标识符\n", lexeme);
        } else {
            printf("%s: 关键字 (类型: %d)\n", lexeme, type);
        }
    }
}
```

### 关键字过滤

```c
#include "src/core/lexer/keywords/CN_lexer_keywords.h"

bool is_valid_identifier(const char* name) {
    // 检查是否为空
    if (name == NULL || name[0] == '\0') {
        return false;
    }
    
    // 检查是否为关键字
    if (F_is_keyword(name)) {
        printf("错误: '%s' 是保留关键字\n", name);
        return false;
    }
    
    // 其他检查...
    return true;
}
```

### 关键字分类

```c
#include "src/core/lexer/keywords/CN_lexer_keywords.h"

void categorize_keyword(const char* lexeme) {
    Eum_TokenType type = F_identify_keyword(lexeme);
    
    switch (type) {
        case Eum_TOKEN_KEYWORD_VAR:
        case Eum_TOKEN_KEYWORD_CONST:
        case Eum_TOKEN_KEYWORD_TYPE:
            printf("%s: 声明关键字\n", lexeme);
            break;
            
        case Eum_TOKEN_KEYWORD_IF:
        case Eum_TOKEN_KEYWORD_ELSE:
        case Eum_TOKEN_KEYWORD_WHILE:
            printf("%s: 控制结构关键字\n", lexeme);
            break;
            
        case Eum_TOKEN_KEYWORD_FUNCTION:
        case Eum_TOKEN_KEYWORD_RETURN:
            printf("%s: 函数关键字\n", lexeme);
            break;
            
        default:
            if (type != Eum_TOKEN_IDENTIFIER) {
                printf("%s: 其他关键字\n", lexeme);
            }
            break;
    }
}
```

## 性能考虑

### 查找算法
- **哈希查找**：O(1)平均查找时间
- **长度过滤**：首先检查词素长度
- **精确匹配**：确保准确的关键字识别

### 内存使用
- 关键字表：约2KB静态内存
- 查找表：约1KB动态内存
- 字符串常量：共享的只读内存

### 初始化开销
- 延迟初始化：第一次使用时初始化
- 一次性构建：构建后重复使用
- 线程安全：支持多线程环境

## 扩展指南

### 添加新关键字
要添加新的关键字：

1. 在`CN_token_types.h`中添加枚举值
2. 在关键字表中添加条目
3. 更新识别函数逻辑
4. 添加相应的测试用例

**示例：**
```c
// 在CN_token_types.h中添加
Eum_TOKEN_KEYWORD_NEW_FEATURE,

// 在关键字表中添加
{"新特性", Eum_TOKEN_KEYWORD_NEW_FEATURE},
```

### 自定义关键字表
可以创建自定义的关键字表：

```c
// 自定义关键字条目
typedef struct {
    const char* keyword;
    Eum_TokenType type;
} CustomKeywordEntry;

// 自定义关键字表
static const CustomKeywordEntry custom_keywords[] = {
    {"自定义1", Eum_TOKEN_KEYWORD_VAR},
    {"自定义2", Eum_TOKEN_KEYWORD_IF},
    {NULL, Eum_TOKEN_IDENTIFIER} // 结束标记
};

// 自定义识别函数
Eum_TokenType custom_identify_keyword(const char* lexeme) {
    for (int i = 0; custom_keywords[i].keyword != NULL; i++) {
        if (strcmp(lexeme, custom_keywords[i].keyword) == 0) {
            return custom_keywords[i].type;
        }
    }
    return Eum_TOKEN_IDENTIFIER;
}
```

## 错误处理

### 边界情况处理
1. **空指针**：返回`Eum_TOKEN_IDENTIFIER`
2. **空字符串**：返回`Eum_TOKEN_IDENTIFIER`
3. **超长词素**：快速返回`Eum_TOKEN_IDENTIFIER`

### 错误示例
```c
// 安全的关键字识别
Eum_TokenType safe_identify_keyword(const char* lexeme) {
    if (lexeme == NULL || lexeme[0] == '\0') {
        return Eum_TOKEN_IDENTIFIER;
    }
    
    // 长度过滤（关键字最长4个中文字符，12字节）
    size_t len = strlen(lexeme);
    if (len > 12) {
        return Eum_TOKEN_IDENTIFIER;
    }
    
    return F_identify_keyword(lexeme);
}
```

## 相关文档

- [令牌类型定义](../../token/CN_token_types.md)
- [扫描器模块 API 文档](../scanner/CN_lexer_scanner.md)
- [令牌扫描器模块 API 文档](../token_scanners/CN_lexer_token_scanners.md)

## 修订历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-08 | 初始版本发布，支持70个关键字 |
| 1.0.1 | 2026-01-08 | 修复关键字表初始化问题 |

## 版权声明

版权所有 © 2026 CN_Language项目团队。保留所有权利。

本文档是CN_Language项目的一部分，遵循项目许可证条款。
