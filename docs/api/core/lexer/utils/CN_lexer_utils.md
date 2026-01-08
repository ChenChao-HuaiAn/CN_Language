# CN_Language 词法分析器工具函数模块 API 文档

## 概述

工具函数模块提供词法分析器所需的各种辅助函数，包括字符分类、字符串处理、内存管理等。它是词法分析器的基础工具库，为其他模块提供通用的功能支持。

## 功能分类

### 1. 字符分类函数
- 字母、数字、字母数字检查
- 空白字符、运算符字符、分隔符字符检查
- 中文字符支持

### 2. 字符串处理函数
- 安全字符串复制和比较
- 字符串长度和连接
- 内存安全的字符串操作

### 3. 内存管理函数
- 安全的内存分配和释放
- 边界检查的内存复制
- 内存错误检测

## API 参考

### 字符分类函数

#### F_is_alpha

检查字符是否为字母（包括中文字符）。

**函数签名：**
```c
bool F_is_alpha(char c);
```

**参数：**
- `c`：要检查的字符

**返回值：**
- `true`：字符是字母
- `false`：字符不是字母

**支持范围：**
- ASCII字母：`a-z`, `A-Z`
- 下划线：`_`
- 中文字符：UTF-8编码的中文字符

**示例：**
```c
if (F_is_alpha('A')) {
    printf("A 是字母\n");
}
if (F_is_alpha('测')) {  // 中文字符
    printf("'测' 是字母\n");
}
```

#### F_is_digit

检查字符是否为数字。

**函数签名：**
```c
bool F_is_digit(char c);
```

**参数：**
- `c`：要检查的字符

**返回值：**
- `true`：字符是数字
- `false`：字符不是数字

**支持范围：**
- ASCII数字：`0-9`

#### F_is_alpha_numeric

检查字符是否为字母或数字。

**函数签名：**
```c
bool F_is_alpha_numeric(char c);
```

**参数：**
- `c`：要检查的字符

**返回值：**
- `true`：字符是字母或数字
- `false`：字符不是字母或数字

#### F_is_whitespace

检查字符是否为空白字符。

**函数签名：**
```c
bool F_is_whitespace(char c);
```

**参数：**
- `c`：要检查的字符

**返回值：**
- `true`：字符是空白字符
- `false`：字符不是空白字符

**支持的空白字符：**
- 空格：`' '`
- 制表符：`'\t'`
- 换行符：`'\n'`
- 回车符：`'\r'`
- 垂直制表符：`'\v'`
- 换页符：`'\f'`

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

#### F_is_delimiter_char

检查字符是否为分隔符字符。

**函数签名：**
```c
bool F_is_delimiter_char(char c);
```

**参数：**
- `c`：要检查的字符

**返回值：**
- `true`：字符是分隔符字符
- `false`：字符不是分隔符字符

**支持的分隔符：**
- 括号：`(`, `)`, `[`, `]`, `{`, `}`
- 标点：`,`, `;`, `:`, `.`

### 字符串处理函数

#### F_copy_string

安全地复制字符串。

**函数签名：**
```c
char* F_copy_string(const char* src);
```

**参数：**
- `src`：源字符串

**返回值：**
- `char*`：新分配的字符串副本
- `NULL`：复制失败（内存不足或src为NULL）

**内存管理：**
- 调用者负责释放返回的字符串
- 使用`F_safe_free`释放内存

**示例：**
```c
const char* original = "Hello, 世界!";
char* copy = F_copy_string(original);
if (copy != NULL) {
    printf("复制的字符串: %s\n", copy);
    F_safe_free(copy);
}
```

#### F_compare_strings

比较两个字符串。

**函数签名：**
```c
int F_compare_strings(const char* str1, const char* str2);
```

**参数：**
- `str1`：第一个字符串
- `str2`：第二个字符串

**返回值：**
- `< 0`：str1 < str2
- `= 0`：str1 == str2
- `> 0`：str1 > str2

**安全特性：**
- 处理NULL指针（NULL < 非NULL）
- 空字符串有效比较

#### F_string_length

获取字符串长度。

**函数签名：**
```c
size_t F_string_length(const char* str);
```

**参数：**
- `str`：字符串

**返回值：**
- `size_t`：字符串长度（字节数）
- `0`：如果str为NULL

#### F_concat_strings

连接两个字符串。

**函数签名：**
```c
char* F_concat_strings(const char* str1, const char* str2);
```

**参数：**
- `str1`：第一个字符串
- `str2`：第二个字符串

**返回值：**
- `char*`：新分配的连接字符串
- `NULL`：连接失败

### 内存管理函数

#### F_safe_malloc

带错误检查的内存分配。

**函数签名：**
```c
void* F_safe_malloc(size_t size);
```

**参数：**
- `size`：要分配的字节数

**返回值：**
- `void*`：分配的内存指针
- `NULL`：分配失败

**安全特性：**
- 检查size为0的情况
- 分配失败时记录错误
- 内存初始化为0

#### F_safe_free

安全的内存释放。

**函数签名：**
```c
void F_safe_free(void* ptr);
```

**参数：**
- `ptr`：要释放的内存指针

**安全特性：**
- 检查NULL指针
- 释放后设置为NULL（如果传递指针的地址）
- 防止重复释放

#### F_safe_memcpy

带边界检查的内存复制。

**函数签名：**
```c
void* F_safe_memcpy(void* dest, const void* src, size_t n);
```

**参数：**
- `dest`：目标内存地址
- `src`：源内存地址
- `n`：要复制的字节数

**返回值：**
- `void*`：dest指针
- `NULL`：复制失败（参数无效）

**安全特性：**
- 检查NULL指针
- 检查重叠内存区域
- 边界检查

## 使用示例

### 字符分类示例

```c
#include "src/core/lexer/utils/CN_lexer_utils.h"

void classify_characters(const char* str) {
    printf("分类字符串: %s\n", str);
    
    for (int i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        
        if (F_is_alpha(c)) {
            printf("  '%c': 字母\n", c);
        } else if (F_is_digit(c)) {
            printf("  '%c': 数字\n", c);
        } else if (F_is_whitespace(c)) {
            printf("  '%c': 空白字符\n", c);
        } else if (F_is_operator_char(c)) {
            printf("  '%c': 运算符字符\n", c);
        } else if (F_is_delimiter_char(c)) {
            printf("  '%c': 分隔符字符\n", c);
        } else {
            printf("  '%c': 其他字符\n", c);
        }
    }
}
```

### 字符串处理示例

```c
#include "src/core/lexer/utils/CN_lexer_utils.h"

void string_operations(void) {
    // 字符串复制
    const char* original = "CN_Language";
    char* copy = F_copy_string(original);
    if (copy != NULL) {
        printf("原始: %s\n", original);
        printf("副本: %s\n", copy);
        
        // 字符串比较
        int result = F_compare_strings(original, copy);
        printf("比较结果: %d (0表示相等)\n", result);
        
        // 字符串连接
        char* concatenated = F_concat_strings(copy, " 编译器");
        if (concatenated != NULL) {
            printf("连接后: %s\n", concatenated);
            printf("长度: %zu\n", F_string_length(concatenated));
            F_safe_free(concatenated);
        }
        
        F_safe_free(copy);
    }
}
```

### 内存管理示例

```c
#include "src/core/lexer/utils/CN_lexer_utils.h"

void memory_operations(void) {
    // 安全内存分配
    int* array = (int*)F_safe_malloc(10 * sizeof(int));
    if (array == NULL) {
        fprintf(stderr, "内存分配失败\n");
        return;
    }
    
    // 初始化数组
    for (int i = 0; i < 10; i++) {
        array[i] = i * i;
    }
    
    // 安全内存复制
    int* array_copy = (int*)F_safe_malloc(10 * sizeof(int));
    if (array_copy != NULL) {
        F_safe_memcpy(array_copy, array, 10 * sizeof(int));
        
        // 验证复制
        bool match = true;
        for (int i = 0; i < 10; i++) {
            if (array[i] != array_copy[i]) {
                match = false;
                break;
            }
        }
        printf("内存复制验证: %s\n", match ? "成功" : "失败");
        
        F_safe_free(array_copy);
    }
    
    // 安全内存释放
    F_safe_free(array);
}
```

## 性能考虑

### 字符分类优化
- **查表法**：使用查找表快速分类字符
- **位运算**：使用位运算优化检查
- **内联函数**：关键函数使用内联优化

### 字符串处理优化
- **长度缓存**：缓存字符串长度减少重复计算
- **缓冲区重用**：重用缓冲区减少内存分配
- **批量操作**：支持批量字符串操作

### 内存管理优化
- **内存池**：使用内存池减少分配开销
- **对齐分配**：确保内存对齐提高访问速度
- **延迟释放**：延迟释放减少碎片

## 错误处理

### 字符分类错误
- 无效字符：返回`false`
- NULL指针：返回`false`
- 边界情况：正确处理

### 字符串处理错误
- 内存不足：返回`NULL`
- 无效参数：返回`NULL`或0
- 缓冲区溢出：检测并防止

### 内存管理错误
- 分配失败：返回`NULL`
- 释放错误：安全处理
- 内存泄漏：检测工具

## 扩展指南

### 添加新字符分类
要添加新的字符分类函数：

```c
// 添加新字符分类函数
bool F_is_hex_digit(char c) {
    return F_is_digit(c) || 
           (c >= 'a' && c <= 'f') || 
           (c >= 'A' && c <= 'F');
}

// 或者扩展现有函数
bool F_is_identifier_start(char c) {
    return F_is_alpha(c) || c == '_' || c == '$';
}
```

### 自定义字符串处理
可以创建自定义的字符串处理函数：

```c
// 自定义字符串修剪函数
char* F_trim_string(const char* str) {
    if (str == NULL) return NULL;
    
    // 跳过前导空白
    const char* start = str;
    while (*start && F_is_whitespace(*start)) {
        start++;
    }
    
    // 跳过尾部空白
    const char* end = str + strlen(str) - 1;
    while (end > start && F_is_whitespace(*end)) {
        end--;
    }
    
    // 复制修剪后的字符串
    size_t length = end - start + 1;
    char* result = F_safe_malloc(length + 1);
    if (result != NULL) {
        strncpy(result, start, length);
        result[length] = '\0';
    }
    
    return result;
}
```

## 相关文档

- [扫描器模块 API 文档](../scanner/CN_lexer_scanner.md)
- [令牌扫描器模块 API 文档](../token_scanners/CN_lexer_token_scanners.md)
- [关键字模块 API 文档](../keywords/CN_lexer_keywords.md)
- [运算符模块 API 文档](../operators/CN_lexer_operators.md)

## 修订历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-08 | 初始版本发布 |
| 1.0.1 | 2026-01-08 | 修复内存管理函数 |

## 版权声明

版权所有 © 2026 CN_Language项目团队。保留所有权利。

本文档是CN_Language项目的一部分，遵循项目许可证条款。
