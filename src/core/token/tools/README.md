# 令牌工具函数模块

## 概述

令牌工具函数模块提供了一系列实用工具函数，用于令牌的格式化输出、类型转换、调试和序列化等操作。本模块是令牌系统的辅助模块，提供高级功能而不涉及核心业务逻辑。

## 模块职责

- 令牌格式化输出和字符串表示
- 令牌类型名称转换
- 调试和日志输出支持
- 序列化和反序列化辅助函数
- 通用工具函数

## 文件结构

```
src/core/token/tools/
├── CN_token_tools.h    # 头文件 - 声明工具函数
├── CN_token_tools.c    # 源文件 - 实现工具函数
└── README.md           # 本文件 - 模块文档
```

## API 参考

### 令牌字符串表示

#### F_token_to_string

将令牌转换为可读的字符串表示。

```c
char* F_token_to_string(const Stru_Token_t* token);
```

**参数：**
- `token`：令牌指针

**返回值：**
- `char*`：字符串表示（调用者负责释放）
- `NULL`：转换失败（内存不足或参数无效）

**输出格式：**
```
类型("词素") at 行:列 = 值
```

**示例输出：**
```
KEYWORD_VAR("变量") at 1:1
LITERAL_INTEGER("42") at 1:8 = 42
OPERATOR_PLUS("+") at 1:11
```

#### F_token_to_string_simple

将令牌转换为简化的字符串表示。

```c
char* F_token_to_string_simple(const Stru_Token_t* token);
```

**参数：**
- `token`：令牌指针

**返回值：**
- `char*`：简化字符串表示（调用者负责释放）

**输出格式：**
```
类型(词素)
```

**示例输出：**
```
KEYWORD_VAR(变量)
LITERAL_INTEGER(42)
```

#### F_token_to_json

将令牌转换为JSON格式字符串。

```c
char* F_token_to_json(const Stru_Token_t* token);
```

**参数：**
- `token`：令牌指针

**返回值：**
- `char*`：JSON字符串（调用者负责释放）

**输出格式：**
```json
{
  "type": "KEYWORD_VAR",
  "lexeme": "变量",
  "line": 1,
  "column": 1,
  "value": null,
  "category": "KEYWORD"
}
```

### 类型名称转换

#### F_get_token_type_name

获取令牌类型的名称字符串。

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

#### F_get_token_type_short_name

获取令牌类型的简短名称。

```c
const char* F_get_token_type_short_name(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `const char*`：简短类型名称

**示例：**
```c
const char* short_name = F_get_token_type_short_name(Eum_TOKEN_KEYWORD_VAR);
printf("简短名称：%s\n", short_name);  // 输出：VAR
```

#### F_token_type_to_string

将令牌类型转换为完整描述字符串。

```c
char* F_token_type_to_string(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `char*`：描述字符串（调用者负责释放）

**输出格式：**
```
KEYWORD_VAR (变量) - 变量声明关键字
```

### 调试和输出

#### F_print_token

打印令牌信息到标准输出。

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

#### F_print_token_verbose

详细打印令牌信息。

```c
void F_print_token_verbose(const Stru_Token_t* token);
```

**参数：**
- `token`：令牌指针

**输出格式：**
```
令牌信息:
  类型: KEYWORD_VAR (变量声明关键字)
  词素: "变量"
  位置: 行 1, 列 1
  类别: 类型声明
  优先级: 10
  值: <none>
```

#### F_debug_token

调试输出令牌信息（仅在调试模式下有效）。

```c
void F_debug_token(const Stru_Token_t* token, const char* context);
```

**参数：**
- `token`：令牌指针
- `context`：上下文信息字符串

**特点：**
- 仅在定义了`CN_DEBUG`宏时有效
- 包含文件名和行号信息
- 可配置的输出级别

### 序列化辅助

#### F_serialize_token

将令牌序列化为二进制数据。

```c
size_t F_serialize_token(const Stru_Token_t* token, void* buffer, size_t buffer_size);
```

**参数：**
- `token`：令牌指针
- `buffer`：输出缓冲区
- `buffer_size`：缓冲区大小

**返回值：**
- 实际写入的字节数
- 0表示失败（缓冲区不足或参数无效）

**序列化格式：**
```
[类型:4字节][行号:4字节][列号:4字节][词素长度:4字节][词素数据...][值类型:1字节][值数据...]
```

#### F_deserialize_token

从二进制数据反序列化令牌。

```c
Stru_Token_t* F_deserialize_token(const void* data, size_t data_size);
```

**参数：**
- `data`：二进制数据指针
- `data_size`：数据大小

**返回值：**
- 反序列化的令牌指针（调用者负责销毁）
- NULL表示失败（数据无效或内存不足）

### 通用工具函数

#### F_token_clone_safe

安全克隆令牌（带错误处理）。

```c
Stru_Token_t* F_token_clone_safe(const Stru_Token_t* token, const char** error);
```

**参数：**
- `token`：源令牌指针
- `error`：错误信息输出指针（可选）

**返回值：**
- 克隆的令牌指针
- NULL表示失败，错误信息存储在error中

#### F_token_compare_safe

安全比较两个令牌。

```c
int F_token_compare_safe(const Stru_Token_t* token1, const Stru_Token_t* token2, const char** error);
```

**参数：**
- `token1`：第一个令牌指针
- `token2`：第二个令牌指针
- `error`：错误信息输出指针（可选）

**返回值：**
- `<0`：token1 < token2
- `=0`：token1 == token2
- `>0`：token1 > token2
- 比较失败时返回特殊错误码

#### F_token_hash_safe

安全计算令牌哈希值。

```c
size_t F_token_hash_safe(const Stru_Token_t* token, const char** error);
```

**参数：**
- `token`：令牌指针
- `error`：错误信息输出指针（可选）

**返回值：**
- 哈希值
- 0表示失败，错误信息存储在error中

### 格式化工具

#### F_format_token

格式化令牌为指定格式的字符串。

```c
char* F_format_token(const Stru_Token_t* token, const char* format);
```

**参数：**
- `token`：令牌指针
- `format`：格式字符串

**返回值：**
- 格式化后的字符串（调用者负责释放）

**格式说明符：**
- `%t`：类型名称
- `%T`：简短类型名称
- `%l`：词素
- `%L`：带引号的词素
- `%r`：行号
- `%c`：列号
- `%v`：值
- `%C`：类别
- `%p`：优先级

**示例：**
```c
char* str = F_format_token(token, "[%r:%c] %t(%l)");
// 输出：[1:1] KEYWORD_VAR(变量)
```

#### F_token_snprintf

安全格式化令牌到缓冲区。

```c
int F_token_snprintf(char* buffer, size_t size, const char* format, const Stru_Token_t* token);
```

**参数：**
- `buffer`：输出缓冲区
- `size`：缓冲区大小
- `format`：格式字符串
- `token`：令牌指针

**返回值：**
- 实际写入的字符数（不包括终止符）
- 负值表示错误

## 使用示例

### 基本字符串转换

```c
#include "src/core/token/tools/CN_token_tools.h"
#include <stdio.h>

int main(void) {
    // 创建测试令牌
    Stru_Token_t* var_token = F_create_token(
        Eum_TOKEN_KEYWORD_VAR,
        "变量",
        1, 1
    );
    
    Stru_Token_t* int_token = F_create_token(
        Eum_TOKEN_LITERAL_INTEGER,
        "42",
        1, 8
    );
    F_token_set_int_value(int_token, 42);
    
    // 转换为字符串
    char* str1 = F_token_to_string(var_token);
    char* str2 = F_token_to_string(int_token);
    
    printf("令牌1: %s\n", str1);
    printf("令牌2: %s\n", str2);
    
    // 类型名称
    const char* type_name = F_get_token_type_name(var_token->type);
    const char* short_name = F_get_token_type_short_name(var_token->type);
    
    printf("类型名称: %s\n", type_name);
    printf("简短名称: %s\n", short_name);
    
    // 清理
    free(str1);
    free(str2);
    F_destroy_token(var_token);
    F_destroy_token(int_token);
    
    return 0;
}
```

### 调试和输出

```c
#include "src/core/token/tools/CN_token_tools.h"
#include <stdio.h>

int main(void) {
    // 创建不同类型的令牌
    Stru_Token_t* tokens[3];
    
    tokens[0] = F_create_token(Eum_TOKEN_KEYWORD_VAR, "变量", 1, 1);
    tokens[1] = F_create_token(Eum_TOKEN_LITERAL_INTEGER, "100", 2, 5);
    F_token_set_int_value(tokens[1], 100);
    tokens[2] = F_create_token(Eum_TOKEN_OPERATOR_PLUS, "+", 3, 10);
    
    // 普通打印
    printf("普通打印:\n");
    for (int i = 0; i < 3; i++) {
        F_print_token(tokens[i]);
    }
    
    printf("\n详细打印:\n");
    for (int i = 0; i < 3; i++) {
        F_print_token_verbose(tokens[i]);
        printf("\n");
    }
    
    // 调试输出（仅在调试模式）
#ifdef CN_DEBUG
    printf("\n调试输出:\n");
    for (int i = 0; i < 3; i++) {
        F_debug_token(tokens[i], "测试上下文");
    }
#endif
    
    // 清理
    for (int i = 0; i < 3; i++) {
        F_destroy_token(tokens[i]);
    }
    
    return 0;
}
```

### 格式化和序列化

```c
#include "src/core/token/tools/CN_token_tools.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    // 创建测试令牌
    Stru_Token_t* token = F_create_token(
        Eum_TOKEN_LITERAL_STRING,
        "\"hello\"",
        1, 1
    );
    F_token_set_string_value(token, "hello");
    
    // 自定义格式化
    char* formatted = F_format_token(token, "Token: %t, Lexeme: %l, Value: %v");
    printf("格式化: %s\n", formatted);
    free(formatted);
    
    // 安全格式化到缓冲区
    char buffer[256];
    int len = F_token_snprintf(buffer, sizeof(buffer), 
                              "[%r:%c] %t(%L) = %v", token);
    printf("安全格式化: %s\n", buffer);
    
    // JSON格式
    char* json = F_token_to_json(token);
    printf("JSON格式:\n%s\n", json);
    free(json);
    
    // 序列化
    uint8_t serialized[512];
    size_t serialized_size = F_serialize_token(token, serialized, sizeof(serialized));
    
    if (serialized_size > 0) {
        printf("序列化大小: %zu 字节\n", serialized_size);
        
        // 反序列化
        Stru_Token_t* deserialized = F_deserialize_token(serialized, serialized_size);
        if (deserialized != NULL) {
            printf("反序列化成功\n");
            
            // 比较原始和反序列化的令牌
            if (F_compare_tokens(token, deserialized)) {
                printf("令牌相等\n");
            }
            
            F_destroy_token(deserialized);
        }
    }
    
    // 清理
    F_destroy_token(token);
    
    return 0;
}
```

### 安全操作

```c
#include "src/core/token/tools/CN_token_tools.h"
#include <stdio.h>

int main(void) {
    // 创建测试令牌
    Stru_Token_t* token = F_create_token(
        Eum_TOKEN_LITERAL_INTEGER,
        "123",
        1, 1
    );
    F_token_set_int_value(token, 123);
    
    // 安全克隆
    const char* error = NULL;
    Stru_Token_t* cloned = F_token_clone_safe(token, &error);
    
    if (cloned == NULL) {
        printf("克隆失败: %s\n", error);
    } else {
        printf("克隆成功\n");
        
        // 安全比较
        int comparison = F_token_compare_safe(token, cloned, &error);
        if (error != NULL) {
            printf("比较失败: %s\n", error);
        } else if (comparison == 0) {
            printf("令牌相等\n");
        }
        
        // 安全哈希
        size_t hash1 = F_token_hash_safe(token, &error);
        if (error != NULL) {
            printf("哈希失败: %s\n", error);
        } else {
            printf("哈希值: %zu\n", hash1);
        }
        
        F_destroy_token(cloned);
    }
    
    // 测试错误情况
    printf("\n测试错误处理:\n");
    
    // NULL令牌
    Stru_Token_t* null_clone = F_token_clone_safe(NULL, &error);
    if (null_clone == NULL && error != NULL) {
        printf("NULL克隆错误: %s\n", error);
    }
    
    // 无效比较
    int invalid_compare = F_token_compare_safe(token, NULL, &error);
    if (error != NULL) {
        printf("无效比较错误: %s\n", error);
    }
    
    // 清理
    F_destroy_token(token);
    
    return 0;
}
```

## 性能优化

### 字符串池优化

对常用字符串使用字符串池减少内存分配：

```c
// 字符串池
typedef struct {
    const char* key;
    char* value;
    size_t ref_count;
} StringPoolEntry_t;

// 从池中获取字符串
const char* get_string_from_pool(const char* str) {
    // 查找现有条目
    StringPoolEntry_t* entry = find_in_string_pool(str);
    if (entry != NULL) {
        entry->ref_count++;
        return entry->value;
    }
    
    // 创建新条目
    char* copied = cn_strdup(str);
    if (copied == NULL) {
        return NULL;
    }
    
    add_to_string_pool(str, copied);
    return copied;
}
```

### 格式化缓存

缓存格式化结果提高性能：

```c
// 格式化缓存条目
typedef struct {
    Eum_TokenType type;
    const char* lexeme;
    size_t line;
    size_t column;
    const char* format;
    char* result;
    uint32_t access_count;
} FormatCacheEntry_t;

// 带缓存的格式化
char* cached_format_token(const Stru_Token_t* token, const char* format) {
    // 生成缓存键
    uint64_t cache_key = generate_cache_key(token, format);
    
    // 检查缓存
    FormatCacheEntry_t* cached = find_in_format_cache(cache_key);
    if (cached != NULL) {
        cached->access_count++;
        return cn_strdup(cached->result); // 返回副本
    }
    
    // 缓存未命中，执行格式化
    char* result = F_format_token(token, format);
    if (result != NULL) {
        add_to_format_cache(cache_key, token, format, result);
    }
    
    return result;
}
```

## 内存管理

### 动态内存使用

| 操作 | 内存分配 | 释放责任 |
|------|----------|----------|
| `F_token_to_string` | 分配新字符串 | 调用者 |
| `F_token_to_json` | 分配新字符串 | 调用者 |
| `F_format_token` | 分配新字符串 | 调用者 |
| `F_deserialize_token` | 分配新令牌 | 调用者 |
| `F_token_clone_safe` | 分配新令牌 | 调用者 |

### 内存泄漏防护

```c
// 安全包装器
#define SAFE_TOKEN_STRING
