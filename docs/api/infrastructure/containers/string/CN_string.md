# CN_string API 文档

## 概述

`CN_string` 模块提供了动态字符串数据结构的实现，支持UTF-8编码。字符串模块支持常见的字符串操作，如创建、销毁、追加、比较、查找、替换等。

## 头文件

```c
#include "infrastructure/containers/string/CN_string.h"
```

## 数据结构

### Stru_String_t

字符串的主要结构体。

```c
typedef struct Stru_String_t
{
    char* data;                 /**< 字符串数据（UTF-8编码） */
    size_t length;              /**< 字符串长度（字符数） */
    size_t capacity;            /**< 当前分配的容量（字节数） */
    size_t byte_length;         /**< 字符串字节长度 */
} Stru_String_t;
```

**字段说明：**

- `data`: 指向字符串数据的指针（UTF-8编码，以null结尾）
- `length`: 字符串中的字符数（不是字节数）
- `capacity`: 当前分配的容量（字节数）
- `byte_length`: 字符串的实际字节长度

## 函数参考

### F_create_string

```c
Stru_String_t* F_create_string(size_t initial_capacity);
```

创建并初始化一个新的字符串。

**参数：**
- `initial_capacity`: 初始容量（字节数）

**返回值：**
- 成功：指向新创建的字符串的指针
- 失败：NULL（内存分配失败）

### F_create_string_from_cstr

```c
Stru_String_t* F_create_string_from_cstr(const char* cstr);
```

从C风格字符串创建新的字符串。

**参数：**
- `cstr`: C风格字符串（UTF-8编码）

**返回值：**
- 成功：指向新创建的字符串的指针
- 失败：NULL（参数无效或内存分配失败）

### F_destroy_string

```c
void F_destroy_string(Stru_String_t* str);
```

销毁字符串，释放所有相关内存。

**参数：**
- `str`: 要销毁的字符串指针

### F_string_length

```c
size_t F_string_length(Stru_String_t* str);
```

获取字符串中的字符数（不是字节数）。

**参数：**
- `str`: 字符串指针

**返回值：**
- 字符串长度，如果`str`为NULL返回0

### F_string_byte_length

```c
size_t F_string_byte_length(Stru_String_t* str);
```

获取字符串的字节长度。

**参数：**
- `str`: 字符串指针

**返回值：**
- 字符串字节长度，如果`str`为NULL返回0

### F_string_cstr

```c
const char* F_string_cstr(Stru_String_t* str);
```

返回字符串的C风格表示（以null结尾）。

**参数：**
- `str`: 字符串指针

**返回值：**
- C风格字符串，如果`str`为NULL返回NULL

### F_string_append

```c
bool F_string_append(Stru_String_t* dest, Stru_String_t* src);
```

在字符串末尾追加另一个字符串。

**参数：**
- `dest`: 目标字符串指针
- `src`: 源字符串指针

**返回值：**
- 成功：true
- 失败：false（参数无效或内存分配失败）

### F_string_append_cstr

```c
bool F_string_append_cstr(Stru_String_t* dest, const char* cstr);
```

在字符串末尾追加C风格字符串。

**参数：**
- `dest`: 目标字符串指针
- `cstr`: C风格字符串

**返回值：**
- 成功：true
- 失败：false（参数无效或内存分配失败）

### F_string_clear

```c
void F_string_clear(Stru_String_t* str);
```

清空字符串内容，但不释放字符串本身。

**参数：**
- `str`: 字符串指针

### F_string_compare

```c
int F_string_compare(Stru_String_t* str1, Stru_String_t* str2);
```

比较两个字符串是否相等。

**参数：**
- `str1`: 第一个字符串指针
- `str2`: 第二个字符串指针

**返回值：**
- 相等返回0，str1小于str2返回负数，否则返回正数

### F_string_copy

```c
bool F_string_copy(Stru_String_t* dest, Stru_String_t* src);
```

复制源字符串到目标字符串。

**参数：**
- `dest`: 目标字符串指针
- `src`: 源字符串指针

**返回值：**
- 成功：true
- 失败：false（参数无效或内存分配失败）

### F_string_substring

```c
Stru_String_t* F_string_substring(Stru_String_t* str, size_t start, size_t length);
```

获取字符串的子字符串。

**参数：**
- `str`: 源字符串指针
- `start`: 起始位置（字符索引）
- `length`: 子字符串长度

**返回值：**
- 新的子字符串指针，失败返回NULL

### F_string_find

```c
int64_t F_string_find(Stru_String_t* str, Stru_String_t* substr, size_t start_pos);
```

在字符串中查找子字符串。

**参数：**
- `str`: 源字符串指针
- `substr`: 要查找的子字符串
- `start_pos`: 起始查找位置

**返回值：**
- 子字符串位置（字符索引），未找到返回-1

### F_string_replace

```c
bool F_string_replace(Stru_String_t* str, Stru_String_t* old_substr, Stru_String_t* new_substr);
```

替换字符串中的子字符串。

**参数：**
- `str`: 字符串指针
- `old_substr`: 要替换的子字符串
- `new_substr`: 新的子字符串

**返回值：**
- 替换成功返回true，失败返回false

### F_string_to_upper

```c
bool F_string_to_upper(Stru_String_t* str);
```

将字符串转换为大写。

**参数：**
- `str`: 字符串指针

**返回值：**
- 转换成功返回true，失败返回false

**注意：** 当前实现仅处理ASCII字符

### F_string_to_lower

```c
bool F_string_to_lower(Stru_String_t* str);
```

将字符串转换为小写。

**参数：**
- `str`: 字符串指针

**返回值：**
- 转换成功返回true，失败返回false

**注意：** 当前实现仅处理ASCII字符

### F_string_trim

```c
bool F_string_trim(Stru_String_t* str);
```

去除字符串两端的空白字符。

**参数：**
- `str`: 字符串指针

**返回值：**
- 去除成功返回true，失败返回false

## 配置宏

### CN_STRING_INITIAL_CAPACITY
```c
#define CN_STRING_INITIAL_CAPACITY 32
```
字符串的初始容量。

### CN_STRING_GROWTH_FACTOR
```c
#define CN_STRING_GROWTH_FACTOR 2
```
字符串的扩容因子。

## 使用示例

```c
#include "infrastructure/containers/string/CN_string.h"
#include <stdio.h>

int main() {
    // 从C字符串创建字符串
    Stru_String_t* str1 = F_create_string_from_cstr("Hello, ");
    Stru_String_t* str2 = F_create_string_from_cstr("World!");
    
    // 追加字符串
    F_string_append(str1, str2);
    
    // 获取C风格字符串
    const char* cstr = F_string_cstr(str1);
    printf("拼接后的字符串: %s\n", cstr);
    
    // 获取字符串信息
    printf("字符数: %zu\n", F_string_length(str1));
    printf("字节数: %zu\n", F_string_byte_length(str1));
    
    // 子字符串操作
    Stru_String_t* substr = F_string_substring(str1, 7, 5);
    if (substr != NULL) {
        printf("子字符串: %s\n", F_string_cstr(substr));
        F_destroy_string(substr);
    }
    
    // 查找子字符串
    Stru_String_t* search = F_create_string_from_cstr("World");
    int64_t pos = F_string_find(str1, search, 0);
    if (pos >= 0) {
        printf("找到 'World' 在位置: %lld\n", pos);
    }
    
    // 清理
    F_destroy_string(search);
    F_destroy_string(str1);
    F_destroy_string(str2);
    
    return 0;
}
```

## UTF-8支持

字符串模块支持UTF-8编码，但有以下限制：

1. **长度计算**：`F_string_length`返回的是字符数（不是字节数）
2. **子字符串**：`F_string_substring`按字符位置操作（不是字节位置）
3. **大小写转换**：当前实现仅处理ASCII字符
4. **查找操作**：`F_string_find`使用字节级查找，但返回字符位置

## 性能特征

### 时间复杂度
- 创建：O(1)
- 销毁：O(1)
- 追加：平均O(1)，最坏O(n)（需要扩容）
- 比较：O(n)
- 复制：O(n)
- 子字符串：O(n)
- 查找：O(n*m)，其中n是字符串长度，m是子字符串长度

### 空间复杂度
- O(n)，其中n是字符串容量

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本 |

## 相关文档

- [字符串模块README](../src/infrastructure/containers/string/README.md)
- [CN_Language项目架构文档](../../../../architecture/001-中文编程语言CN_Language开发规划.md)
