# 令牌字面量值操作模块

## 概述

令牌字面量值操作模块负责处理令牌中的各种字面量值，包括整数、浮点数、字符串、字符和布尔值。本模块提供了统一的接口来设置、获取和操作令牌的字面量值。

## 模块职责

- 设置令牌的字面量值（整数、浮点数、字符串、字符、布尔值）
- 获取令牌的字面量值
- 验证字面量值的类型兼容性
- 提供安全的字面量值访问接口

## 文件结构

```
src/core/token/values/
├── CN_token_values.h    # 头文件 - 声明字面量值操作函数
├── CN_token_values.c    # 源文件 - 实现字面量值操作函数
└── README.md            # 本文件 - 模块文档
```

## API 参考

### 整数值操作

#### F_token_set_int_value

设置令牌的整数值。

```c
void F_token_set_int_value(Stru_Token_t* token, long value);
```

**参数：**
- `token`：令牌指针
- `value`：整数值

**适用类型：**
- `Eum_TOKEN_LITERAL_INTEGER`

**注意事项：**
- 如果令牌类型不匹配，函数不执行任何操作
- 值存储在令牌的`literal.int_value`字段中

#### F_token_get_int_value

获取令牌的整数值。

```c
long F_token_get_int_value(const Stru_Token_t* token);
```

**参数：**
- `token`：令牌指针

**返回值：**
- 整数值
- 如果令牌类型不匹配或令牌为NULL，返回0

**安全特性：**
- 类型安全检查
- NULL指针检查

### 浮点数值操作

#### F_token_set_float_value

设置令牌的浮点数值。

```c
void F_token_set_float_value(Stru_Token_t* token, double value);
```

**参数：**
- `token`：令牌指针
- `value`：浮点数值

**适用类型：**
- `Eum_TOKEN_LITERAL_FLOAT`

#### F_token_get_float_value

获取令牌的浮点数值。

```c
double F_token_get_float_value(const Stru_Token_t* token);
```

**参数：**
- `token`：令牌指针

**返回值：**
- 浮点数值
- 如果令牌类型不匹配或令牌为NULL，返回0.0

### 字符串值操作

#### F_token_set_string_value

设置令牌的字符串值。

```c
void F_token_set_string_value(Stru_Token_t* token, const char* value);
```

**参数：**
- `token`：令牌指针
- `value`：字符串值（会被复制）

**适用类型：**
- `Eum_TOKEN_LITERAL_STRING`

**内存管理：**
- 复制字符串值
- 释放旧的字符串值（如果存在）
- 处理NULL值（设置为空字符串）

#### F_token_get_string_value

获取令牌的字符串值。

```c
const char* F_token_get_string_value(const Stru_Token_t* token);
```

**参数：**
- `token`：令牌指针

**返回值：**
- 字符串值指针（只读）
- 如果令牌类型不匹配或令牌为NULL，返回空字符串""

### 字符值操作

#### F_token_set_char_value

设置令牌的字符值。

```c
void F_token_set_char_value(Stru_Token_t* token, char value);
```

**参数：**
- `token`：令牌指针
- `value`：字符值

**适用类型：**
- `Eum_TOKEN_LITERAL_CHAR`

#### F_token_get_char_value

获取令牌的字符值。

```c
char F_token_get_char_value(const Stru_Token_t* token);
```

**参数：**
- `token`：令牌指针

**返回值：**
- 字符值
- 如果令牌类型不匹配或令牌为NULL，返回'\0'

### 布尔值操作

#### F_token_set_bool_value

设置令牌的布尔值。

```c
void F_token_set_bool_value(Stru_Token_t* token, bool value);
```

**参数：**
- `token`：令牌指针
- `value`：布尔值

**适用类型：**
- `Eum_TOKEN_LITERAL_BOOLEAN`

#### F_token_get_bool_value

获取令牌的布尔值。

```c
bool F_token_get_bool_value(const Stru_Token_t* token);
```

**参数：**
- `token`：令牌指针

**返回值：**
- 布尔值
- 如果令牌类型不匹配或令牌为NULL，返回false

### 通用值操作

#### F_token_set_literal_value

根据令牌类型设置相应的字面量值。

```c
bool F_token_set_literal_value(Stru_Token_t* token, const void* value);
```

**参数：**
- `token`：令牌指针
- `value`：指向值的指针（类型根据令牌类型确定）

**返回值：**
- `true`：设置成功
- `false`：设置失败（类型不匹配或参数无效）

**支持的类型：**
- 整数：`long*`
- 浮点数：`double*`
- 字符串：`const char*`
- 字符：`char*`
- 布尔值：`bool*`

#### F_token_get_literal_value

根据令牌类型获取相应的字面量值。

```c
bool F_token_get_literal_value(const Stru_Token_t* token, void* out_value);
```

**参数：**
- `token`：令牌指针
- `out_value`：输出值的指针（类型根据令牌类型确定）

**返回值：**
- `true`：获取成功
- `false`：获取失败（类型不匹配或参数无效）

### 类型检查和验证

#### F_token_is_literal_type

检查令牌类型是否为字面量类型。

```c
bool F_token_is_literal_type(Eum_TokenType type);
```

**参数：**
- `type`：令牌类型

**返回值：**
- `true`：是字面量类型
- `false`：不是字面量类型

**支持的字面量类型：**
- `Eum_TOKEN_LITERAL_INTEGER`
- `Eum_TOKEN_LITERAL_FLOAT`
- `Eum_TOKEN_LITERAL_STRING`
- `Eum_TOKEN_LITERAL_CHAR`
- `Eum_TOKEN_LITERAL_BOOLEAN`

#### F_token_validate_literal_value

验证令牌的字面量值是否有效。

```c
bool F_token_validate_literal_value(const Stru_Token_t* token);
```

**验证规则：**
- 整数：始终有效
- 浮点数：检查是否为NaN或无穷大
- 字符串：检查指针有效性
- 字符：检查是否为有效ASCII字符
- 布尔值：始终有效

## 使用示例

### 基本使用

```c
#include "src/core/token/values/CN_token_values.h"
#include <stdio.h>

int main(void) {
    // 创建整数令牌
    Stru_Token_t* int_token = F_create_token(
        Eum_TOKEN_LITERAL_INTEGER,
        "42",
        1, 1
    );
    
    // 设置整数值
    F_token_set_int_value(int_token, 42);
    
    // 获取整数值
    long int_value = F_token_get_int_value(int_token);
    printf("整数值: %ld\n", int_value);
    
    // 创建字符串令牌
    Stru_Token_t* str_token = F_create_token(
        Eum_TOKEN_LITERAL_STRING,
        "\"hello\"",
        1, 8
    );
    
    // 设置字符串值
    F_token_set_string_value(str_token, "hello");
    
    // 获取字符串值
    const char* str_value = F_token_get_string_value(str_token);
    printf("字符串值: %s\n", str_value);
    
    // 类型检查
    if (F_token_is_literal_type(int_token->type)) {
        printf("令牌是字面量类型\n");
    }
    
    // 验证值
    if (F_token_validate_literal_value(int_token)) {
        printf("整数值有效\n");
    }
    
    // 清理
    F_destroy_token(int_token);
    F_destroy_token(str_token);
    
    return 0;
}
```

### 通用值操作

```c
#include "src/core/token/values/CN_token_values.h"
#include <stdio.h>

void process_literal_token(Stru_Token_t* token) {
    // 使用通用接口处理字面量值
    if (F_token_is_literal_type(token->type)) {
        switch (token->type) {
            case Eum_TOKEN_LITERAL_INTEGER: {
                long value;
                if (F_token_get_literal_value(token, &value)) {
                    printf("整数值: %ld\n", value);
                }
                break;
            }
            case Eum_TOKEN_LITERAL_FLOAT: {
                double value;
                if (F_token_get_literal_value(token, &value)) {
                    printf("浮点数值: %f\n", value);
                }
                break;
            }
            case Eum_TOKEN_LITERAL_STRING: {
                char buffer[256];
                if (F_token_get_literal_value(token, buffer)) {
                    printf("字符串值: %s\n", buffer);
                }
                break;
            }
            case Eum_TOKEN_LITERAL_CHAR: {
                char value;
                if (F_token_get_literal_value(token, &value)) {
                    printf("字符值: %c\n", value);
                }
                break;
            }
            case Eum_TOKEN_LITERAL_BOOLEAN: {
                bool value;
                if (F_token_get_literal_value(token, &value)) {
                    printf("布尔值: %s\n", value ? "true" : "false");
                }
                break;
            }
            default:
                printf("未知的字面量类型\n");
        }
    }
}

int main(void) {
    // 创建不同类型的字面量令牌
    Stru_Token_t* tokens[5];
    
    // 整数令牌
    tokens[0] = F_create_token(Eum_TOKEN_LITERAL_INTEGER, "100", 1, 1);
    long int_val = 100;
    F_token_set_literal_value(tokens[0], &int_val);
    
    // 浮点数令牌
    tokens[1] = F_create_token(Eum_TOKEN_LITERAL_FLOAT, "3.14", 1, 5);
    double float_val = 3.14;
    F_token_set_literal_value(tokens[1], &float_val);
    
    // 字符串令牌
    tokens[2] = F_create_token(Eum_TOKEN_LITERAL_STRING, "\"test\"", 1, 10);
    const char* str_val = "test";
    F_token_set_literal_value(tokens[2], str_val);
    
    // 字符令牌
    tokens[3] = F_create_token(Eum_TOKEN_LITERAL_CHAR, "'a'", 1, 16);
    char char_val = 'a';
    F_token_set_literal_value(tokens[3], &char_val);
    
    // 布尔令牌
    tokens[4] = F_create_token(Eum_TOKEN_LITERAL_BOOLEAN, "true", 1, 20);
    bool bool_val = true;
    F_token_set_literal_value(tokens[4], &bool_val);
    
    // 处理所有令牌
    for (int i = 0; i < 5; i++) {
        process_literal_token(tokens[i]);
        F_destroy_token(tokens[i]);
    }
    
    return 0;
}
```

### 错误处理

```c
#include "src/core/token/values/CN_token_values.h"
#include <stdio.h>

bool safe_set_int_value(Stru_Token_t* token, long value) {
    if (token == NULL) {
        fprintf(stderr, "错误：令牌指针为NULL\n");
        return false;
    }
    
    if (token->type != Eum_TOKEN_LITERAL_INTEGER) {
        fprintf(stderr, "错误：令牌类型不匹配，期望整数类型\n");
        return false;
    }
    
    F_token_set_int_value(token, value);
    return true;
}

bool safe_get_string_value(const Stru_Token_t* token, char* buffer, size_t buffer_size) {
    if (token == NULL || buffer == NULL) {
        fprintf(stderr, "错误：参数为NULL\n");
        return false;
    }
    
    if (token->type != Eum_TOKEN_LITERAL_STRING) {
        fprintf(stderr, "错误：令牌类型不匹配，期望字符串类型\n");
        return false;
    }
    
    const char* str_value = F_token_get_string_value(token);
    if (str_value == NULL) {
        fprintf(stderr, "错误：字符串值为NULL\n");
        return false;
    }
    
    // 安全复制
    size_t len = strlen(str_value);
    if (len >= buffer_size) {
        fprintf(stderr, "错误：缓冲区太小\n");
        return false;
    }
    
    strncpy(buffer, str_value, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    
    return true;
}
```

## 内存管理

### 字符串值管理

字符串值的特殊处理：
1. **设置时**：复制字符串，分配新内存
2. **获取时**：返回只读指针，调用者不应修改
3. **销毁时**：自动释放字符串内存
4. **更新时**：释放旧字符串，复制新字符串

### 值复制策略

1. **基本类型**（整数、浮点数、字符、布尔值）：直接复制
2. **字符串类型**：深复制（分配新内存）
3. **扩展数据**：按需复制（如果存在复制回调）

## 性能考虑

### 时间复杂度

| 操作 | 平均时间复杂度 | 说明 |
|------|----------------|------|
| 设置整数值 | O(1) | 直接赋值 |
| 设置浮点数值 | O(1) | 直接赋值 |
| 设置字符值 | O(1) | 直接赋值 |
| 设置布尔值 | O(1) | 直接赋值 |
| 设置字符串值 | O(n) | 字符串复制 |
| 获取值 | O(1) | 直接访问 |
| 类型检查 | O(1) | 枚举比较 |

*注：n为字符串长度*

### 内存使用优化

1. **字符串池**：常用字符串使用字符串池减少重复分配
2. **小字符串优化**：短字符串使用栈存储
3. **延迟分配**：字符串值按需分配

## 线程安全性

### 安全级别

- **基本类型操作**：线程安全（原子操作）
- **字符串操作**：非线程安全（涉及内存分配）
- **通用值操作**：取决于具体类型

### 线程安全使用

```c
#include "src/core/token/values/CN_token_values.h"
#include <pthread.h>

// 线程安全的字符串值设置
void thread_safe_set_string_value(Stru_Token_t* token, const char* value) {
    // 复制字符串（线程安全）
    char* copied = cn_strdup(value);
    if (copied == NULL) {
        return;
    }
    
    // 需要外部同步
    pthread_mutex_lock(&token->mutex);
    
    // 释放旧字符串
    if (token->literal.string_value != NULL) {
        cn_free(token->literal.string_value);
    }
    
    // 设置新字符串
    token->literal.string_value = copied;
    
    pthread_mutex_unlock(&token->mutex);
}
```

## 测试覆盖

### 单元测试

测试文件：`tests/core/token/values/test_token_values.c`

**测试用例：**
1. 各种字面量类型的设置和获取
2. 类型兼容性验证
3. 边界值测试（最大/最小值）
4. 字符串操作测试（空字符串、长字符串）
5. 错误处理测试（NULL指针、类型不匹配）
6. 内存管理测试（泄漏检测）

### 集成测试

与以下模块集成测试：
1. 生命周期管理模块
2. 类型查询模块
3. 工具函数模块

## 依赖关系

### 内部依赖

- `CN_token_types.h`：令牌类型定义
- `CN_token_lifecycle.h`：令牌基本操作

### 外部依赖

- `CN_memory.h`：内存管理接口
- `CN_utils_string.h`：字符串工具函数

## 设计原则

### 类型安全

- 严格的类型检查
- 编译时类型验证（通过枚举）
- 运行时类型断言

### 资源安全

- 字符串值的正确内存管理
- 防止内存泄漏
- 防止悬空指针

### 接口一致性

- 统一的函数命名约定
- 一致的参数顺序
- 标准的错误处理模式

## 扩展指南

### 添加新字面量类型

1. 在`Eum_TokenType`枚举中添加新类型
2. 更新`F_token_is_literal_type()`函数
3. 添加相应的设置和获取函数
4. 更新通用值操作函数
5. 编写测试用例

### 自定义值验证

```c
// 自定义验证回调
typedef bool (*TokenValueValidator_t)(const void* value);

// 注册验证器
void F_token_register_value_validator(Eum_TokenType type, 
                                     TokenValueValidator_t validator);

// 使用示例
bool validate_positive_int(const void* value) {
    long int_val = *(const long*)value;
    return int_val > 0;
}

// 注册验证器
F_token_register_value_validator(Eum_TOKEN_LITERAL_INTEGER, 
                                validate_positive_int);
```

## 版本历史

| 版本 | 日期 | 变更描述 |
|------|------|----------|
| 1.0.0 | 2026-01-08 | 初始版本，支持5种字面量类型 |
| 1.0.1 | 2026-01-08 | 添加通用值操作接口 |
| 1.1.0 | 2026-01-08 | 添加值验证功能 |

## 相关文档

- [令牌模块主文档](../CN_token.md)
- [令牌生命周期管理模块](../lifecycle/README.md)
- [令牌模块化架构文档](../../../docs/api/core
