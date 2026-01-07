# 字符串核心模块 (String Core Module)

## 概述

字符串核心模块提供了字符串数据结构的基本操作实现。这是字符串模块的基础，负责字符串的创建、销毁、内存管理和基本属性查询。

## 特性

- **UTF-8支持**：完整支持UTF-8编码，正确处理多字节字符
- **动态内存**：自动调整字符串容量以适应内容
- **内存安全**：防止缓冲区溢出，自动管理内存
- **高效操作**：优化内存分配和字符串操作
- **类型安全**：通过接口封装确保类型安全

## 数据结构

### Stru_String_t

字符串的主要结构体：

```c
typedef struct Stru_String_t
{
    char* data;             /**< 字符串数据（UTF-8编码） */
    size_t length;          /**< 字符串长度（字符数，UTF-8字符） */
    size_t capacity;        /**< 当前分配的容量（字节数） */
    size_t byte_length;     /**< 字节长度（UTF-8字节数） */
} Stru_String_t;
```

## API 接口

### 创建和销毁

#### `F_create_string`
```c
Stru_String_t* F_create_string(const char* initial_data);
```
从C字符串创建字符串。分配并初始化一个新的字符串。

**参数**：
- `initial_data`：初始C字符串内容（可为NULL）

**返回值**：
- 成功：指向新创建的字符串的指针
- 失败：NULL（如果内存分配失败）

**注意**：如果initial_data为NULL，创建空字符串

#### `F_create_string_with_capacity`
```c
Stru_String_t* F_create_string_with_capacity(size_t initial_capacity);
```
创建指定容量的空字符串。

**参数**：
- `initial_capacity`：初始容量（字节数）

**返回值**：
- 成功：指向新创建的字符串的指针
- 失败：NULL（如果内存分配失败）

#### `F_destroy_string`
```c
void F_destroy_string(Stru_String_t* string);
```
销毁字符串。释放字符串占用的所有内存。

**参数**：
- `string`：要销毁的字符串指针

**注意**：如果string为NULL，函数不执行任何操作

### 字符串属性

#### `F_string_length`
```c
size_t F_string_length(Stru_String_t* string);
```
获取字符串长度。返回字符串中的字符数（UTF-8字符）。

**参数**：
- `string`：字符串指针

**返回值**：
- 字符串长度（如果string为NULL返回0）

#### `F_string_byte_length`
```c
size_t F_string_byte_length(Stru_String_t* string);
```
获取字符串字节长度。返回字符串的字节数。

**参数**：
- `string`：字符串指针

**返回值**：
- 字符串字节长度（如果string为NULL返回0）

#### `F_string_capacity`
```c
size_t F_string_capacity(Stru_String_t* string);
```
获取字符串容量。返回字符串当前分配的容量（字节数）。

**参数**：
- `string`：字符串指针

**返回值**：
- 字符串容量（如果string为NULL返回0）

#### `F_string_is_empty`
```c
bool F_string_is_empty(Stru_String_t* string);
```
检查字符串是否为空。检查字符串是否不包含任何字符。

**参数**：
- `string`：字符串指针

**返回值**：
- 字符串为空：true
- 字符串不为空：false（如果string为NULL返回true）

#### `F_string_c_str`
```c
const char* F_string_c_str(Stru_String_t* string);
```
获取C风格字符串指针。返回指向字符串内部数据的指针。

**参数**：
- `string`：字符串指针

**返回值**：
- 指向字符串数据的指针（如果string为NULL返回NULL）

**注意**：返回的指针指向字符串内部数据，不应被修改或释放

### 内存管理

#### `F_string_reserve`
```c
bool F_string_reserve(Stru_String_t* string, size_t new_capacity);
```
调整字符串容量。调整字符串的容量。

**参数**：
- `string`：字符串指针
- `new_capacity`：新的容量（字节数）

**返回值**：
- 成功：true
- 失败：false（如果string为NULL，或内存分配失败）

**注意**：如果new_capacity小于当前字节长度，函数返回false

#### `F_string_shrink_to_fit`
```c
bool F_string_shrink_to_fit(Stru_String_t* string);
```
收缩字符串以适合当前内容。减少字符串容量以匹配当前内容。

**参数**：
- `string`：字符串指针

**返回值**：
- 成功：true
- 失败：false（如果string为NULL，或内存分配失败）

#### `F_string_clear`
```c
void F_string_clear(Stru_String_t* string);
```
清空字符串。移除字符串中的所有字符，但不释放字符串本身。

**参数**：
- `string`：字符串指针

**注意**：清空后字符串长度变为0，但容量保持不变

## 使用示例

```c
#include "CN_string_core.h"
#include <stdio.h>

int main() {
    // 创建字符串
    Stru_String_t* str = F_create_string("你好，世界！");
    if (str == NULL) {
        printf("创建字符串失败\n");
        return 1;
    }
    
    printf("字符串内容: %s\n", F_string_c_str(str));
    printf("字符长度: %zu\n", F_string_length(str));
    printf("字节长度: %zu\n", F_string_byte_length(str));
    printf("容量: %zu\n", F_string_capacity(str));
    printf("是否为空: %s\n", F_string_is_empty(str) ? "是" : "否");
    
    // 调整容量
    if (F_string_reserve(str, 100)) {
        printf("容量调整成功，新容量: %zu\n", F_string_capacity(str));
    }
    
    // 收缩到合适大小
    if (F_string_shrink_to_fit(str)) {
        printf("收缩后容量: %zu\n", F_string_capacity(str));
    }
    
    // 清空字符串
    F_string_clear(str);
    printf("清空后字符长度: %zu\n", F_string_length(str));
    printf("清空后是否为空: %s\n", F_string_is_empty(str) ? "是" : "否");
    
    // 创建空字符串
    Stru_String_t* empty_str = F_create_string_with_capacity(50);
    if (empty_str) {
        printf("空字符串容量: %zu\n", F_string_capacity(empty_str));
        F_destroy_string(empty_str);
    }
    
    // 清理
    F_destroy_string(str);
    
    return 0;
}
```

## 内部实现

### UTF-8编码处理

字符串核心模块使用UTF-8编码，具有以下特点：
1. **多字节支持**：正确处理ASCII、中文、表情符号等多字节字符
2. **长度计算**：正确计算字符数（不是字节数）
3. **内存效率**：UTF-8是变长编码，对ASCII字符特别高效

### 动态内存管理

字符串使用动态内存分配，具有以下特点：
1. **自动扩容**：当字符串长度超过容量时自动扩容
2. **容量管理**：维护容量信息，减少频繁的内存分配
3. **内存安全**：所有内存操作都进行边界检查

### 内存布局

```
+-------------------+     +-------------------+
| Stru_String_t     |     | 字符串数据        |
| - data: pointer   |---->| "Hello 世界"      |
| - length: 8       |     | (UTF-8编码)       |
| - capacity: 32    |     +-------------------+
| - byte_length: 14 |
+-------------------+
```

### 错误处理

所有函数都包含完整的错误检查：
1. **参数验证**：检查NULL指针和无效参数
2. **内存分配检查**：检查malloc/realloc是否成功
3. **边界检查**：检查容量、长度等边界条件

## 性能特征

- **时间复杂度**：
  - 创建字符串：O(n)，其中n是初始字符串长度
  - 获取长度：O(1)
  - 调整容量：O(n)，其中n是字符串长度
  - 清空字符串：O(1)

- **空间复杂度**：O(n)，其中n是字符串容量

## 配置参数

可以通过修改以下常量来调整字符串的行为：

```c
// 默认初始容量（字节）
#define CN_STRING_INITIAL_CAPACITY 32

// 扩容因子
#define CN_STRING_GROWTH_FACTOR 2

// 最小容量（字节）
#define CN_STRING_MIN_CAPACITY 16
```

## 依赖关系

- C标准库：`stdlib.h`, `string.h`, `stddef.h`, `stdbool.h`, `stdint.h`
- 无其他项目模块依赖

## 测试

核心模块包含完整的单元测试，覆盖所有API接口和边界条件。测试文件位于`tests/infrastructure/containers/string/test_string_core.c`。

测试覆盖率目标：
- 语句覆盖率：≥95%
- 分支覆盖率：≥85%
- 函数覆盖率：100%

## 相关文档

- [字符串模块主文档](../README.md)
- [字符串操作模块文档](../string_operations/README.md)
- [字符串搜索模块文档](../string_search/README.md)
- [字符串转换模块文档](../string_transform/README.md)
- [字符串工具模块文档](../string_utils/README.md)

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本字符串功能 |
| 2.0.0 | 2026-01-07 | 模块化重构，作为核心模块 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
