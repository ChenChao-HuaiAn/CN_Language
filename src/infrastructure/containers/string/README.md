# 字符串模块 (String Module)

## 概述

字符串模块提供了动态字符串数据结构的实现，支持UTF-8编码。字符串模块提供了丰富的字符串操作功能，包括创建、销毁、追加、查找、替换等。

## 特性

- **UTF-8支持**：完整支持UTF-8编码的字符串
- **动态扩容**：自动调整字符串容量以适应内容
- **丰富操作**：提供查找、替换、子字符串、大小写转换等操作
- **内存管理**：自动管理内存分配和释放
- **C字符串兼容**：与C风格字符串无缝互操作

## 数据结构

### Stru_String_t

字符串的主要结构体：

```c
typedef struct Stru_String_t
{
    char* data;                 // 字符串数据（UTF-8编码）
    size_t length;              // 字符串长度（字符数）
    size_t capacity;            // 当前分配的容量（字节数）
    size_t byte_length;         // 字符串字节长度
} Stru_String_t;
```

## API 接口

### 创建和销毁

- `F_create_string(size_t initial_capacity)` - 创建字符串
- `F_create_string_from_cstr(const char* cstr)` - 从C字符串创建字符串
- `F_destroy_string(Stru_String_t* str)` - 销毁字符串

### 基本信息

- `F_string_length(Stru_String_t* str)` - 获取字符串长度（字符数）
- `F_string_byte_length(Stru_String_t* str)` - 获取字符串字节长度
- `F_string_cstr(Stru_String_t* str)` - 获取C风格字符串

### 字符串操作

- `F_string_append(Stru_String_t* dest, Stru_String_t* src)` - 追加字符串
- `F_string_append_cstr(Stru_String_t* dest, const char* cstr)` - 追加C字符串
- `F_string_clear(Stru_String_t* str)` - 清空字符串
- `F_string_compare(Stru_String_t* str1, Stru_String_t* str2)` - 比较字符串
- `F_string_copy(Stru_String_t* dest, Stru_String_t* src)` - 复制字符串

### 高级操作

- `F_string_substring(Stru_String_t* str, size_t start, size_t length)` - 获取子字符串
- `F_string_find(Stru_String_t* str, Stru_String_t* substr, size_t start_pos)` - 查找子字符串
- `F_string_replace(Stru_String_t* str, Stru_String_t* old_substr, Stru_String_t* new_substr)` - 替换子字符串
- `F_string_to_upper(Stru_String_t* str)` - 转换为大写
- `F_string_to_lower(Stru_String_t* str)` - 转换为小写
- `F_string_trim(Stru_String_t* str)` - 去除空白字符

## 使用示例

```c
#include "CN_string.h"
#include <stdio.h>

int main() {
    // 从C字符串创建字符串
    Stru_String_t* str1 = F_create_string_from_cstr("你好，");
    Stru_String_t* str2 = F_create_string_from_cstr("世界！");
    
    if (str1 == NULL || str2 == NULL) {
        printf("创建字符串失败\n");
        return 1;
    }
    
    // 追加字符串
    F_string_append(str1, str2);
    printf("合并后的字符串: %s\n", F_string_cstr(str1));
    
    // 获取字符串信息
    printf("字符串长度: %zu 字符\n", F_string_length(str1));
    printf("字符串字节长度: %zu 字节\n", F_string_byte_length(str1));
    
    // 获取子字符串
    Stru_String_t* substr = F_string_substring(str1, 0, 2);
    if (substr != NULL) {
        printf("子字符串(0-2): %s\n", F_string_cstr(substr));
        F_destroy_string(substr);
    }
    
    // 查找子字符串
    Stru_String_t* search = F_create_string_from_cstr("世界");
    int64_t pos = F_string_find(str1, search, 0);
    if (pos >= 0) {
        printf("'世界'在位置: %lld\n", pos);
    }
    F_destroy_string(search);
    
    // 替换子字符串
    Stru_String_t* old_str = F_create_string_from_cstr("世界");
    Stru_String_t* new_str = F_create_string_from_cstr("宇宙");
    F_string_replace(str1, old_str, new_str);
    printf("替换后的字符串: %s\n", F_string_cstr(str1));
    
    // 大小写转换（示例）
    Stru_String_t* english = F_create_string_from_cstr("Hello World");
    F_string_to_upper(english);
    printf("大写: %s\n", F_string_cstr(english));
    F_string_to_lower(english);
    printf("小写: %s\n", F_string_cstr(english));
    
    // 清理
    F_destroy_string(str1);
    F_destroy_string(str2);
    F_destroy_string(old_str);
    F_destroy_string(new_str);
    F_destroy_string(english);
    
    return 0;
}
```

## 性能特征

- **时间复杂度**：
  - 创建：O(n)
  - 追加：平均O(1)，最坏O(n)（需要扩容）
  - 查找：O(n*m)，其中n是主字符串长度，m是子字符串长度
  - 替换：O(n+m)，其中n是原字符串长度，m是新字符串长度
  - 子字符串：O(k)，其中k是子字符串长度

- **空间复杂度**：O(n)，其中n是字符串容量

## 内存管理

字符串模块负责管理以下内存：
1. 字符串结构体本身
2. 字符串数据缓冲区

用户只需要：
1. 调用创建函数创建字符串
2. 使用字符串操作函数
3. 调用销毁函数释放字符串

## 错误处理

所有函数都包含错误检查：
- 无效参数返回适当错误值（NULL、false、-1等）
- 内存分配失败返回错误
- 位置越界返回错误

## UTF-8支持

字符串模块完全支持UTF-8编码：
- `length`字段存储字符数（不是字节数）
- `byte_length`字段存储字节长度
- 所有位置参数都是字符索引（不是字节索引）
- 支持多字节字符的正确处理

## 线程安全

当前实现不是线程安全的。如果需要在多线程环境中使用，需要外部同步机制。

## 依赖关系

- C标准库：`stdlib.h`, `string.h`, `stddef.h`, `stdbool.h`, `stdint.h`
- 无其他项目模块依赖

## 测试

模块包含完整的单元测试，覆盖所有API接口和边界条件。测试覆盖率目标：
- 语句覆盖率：≥90%
- 分支覆盖率：≥80%
- 函数覆盖率：100%

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本字符串功能 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
