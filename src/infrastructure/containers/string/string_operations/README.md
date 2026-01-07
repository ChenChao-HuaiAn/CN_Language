# 字符串操作模块 (String Operations Module)

## 概述

字符串操作模块提供了字符串的基本操作功能，包括追加、比较、复制、赋值和子字符串操作。这些操作是字符串处理中最常用的功能。

## 特性

- **高效操作**：优化字符串操作性能
- **UTF-8支持**：正确处理多字节字符的操作
- **内存安全**：自动处理内存分配和边界检查
- **类型安全**：通过接口封装确保类型安全
- **完整错误处理**：所有操作都包含错误检查

## API 接口

### 字符串操作

#### `F_string_append`
```c
bool F_string_append(Stru_String_t* dest, Stru_String_t* src);
```
追加字符串。将源字符串追加到目标字符串末尾。

**参数**：
- `dest`：目标字符串指针
- `src`：源字符串指针

**返回值**：
- 成功：true
- 失败：false（如果dest或src为NULL，或内存分配失败）

#### `F_string_append_cstr`
```c
bool F_string_append_cstr(Stru_String_t* dest, const char* src);
```
追加C字符串。将C字符串追加到目标字符串末尾。

**参数**：
- `dest`：目标字符串指针
- `src`：源C字符串指针（可为NULL）

**返回值**：
- 成功：true
- 失败：false（如果dest为NULL，或内存分配失败）

**注意**：如果src为NULL，不执行任何操作，返回true

#### `F_string_append_char`
```c
bool F_string_append_char(Stru_String_t* dest, uint32_t codepoint);
```
追加Unicode字符。将Unicode字符追加到目标字符串末尾。

**参数**：
- `dest`：目标字符串指针
- `codepoint`：要追加的Unicode码点

**返回值**：
- 成功：true
- 失败：false（如果dest为NULL，codepoint无效，或内存分配失败）

#### `F_string_copy`
```c
Stru_String_t* F_string_copy(Stru_String_t* src);
```
复制字符串。创建源字符串的完整副本。

**参数**：
- `src`：源字符串指针

**返回值**：
- 成功：指向新字符串的指针
- 失败：NULL（如果src为NULL，或内存分配失败）

#### `F_string_assign`
```c
bool F_string_assign(Stru_String_t* dest, Stru_String_t* src);
```
赋值字符串。将源字符串的内容赋值给目标字符串。

**参数**：
- `dest`：目标字符串指针
- `src`：源字符串指针

**返回值**：
- 成功：true
- 失败：false（如果dest或src为NULL，或内存分配失败）

**注意**：目标字符串的原有内容会被替换

#### `F_string_assign_cstr`
```c
bool F_string_assign_cstr(Stru_String_t* dest, const char* src);
```
赋值C字符串。将C字符串的内容赋值给目标字符串。

**参数**：
- `dest`：目标字符串指针
- `src`：源C字符串指针（可为NULL）

**返回值**：
- 成功：true
- 失败：false（如果dest为NULL，或内存分配失败）

**注意**：如果src为NULL，目标字符串被清空

### 字符串比较

#### `F_string_equals`
```c
bool F_string_equals(Stru_String_t* str1, Stru_String_t* str2);
```
比较字符串是否相等。比较两个字符串的内容是否完全相同。

**参数**：
- `str1`：第一个字符串指针
- `str2`：第二个字符串指针

**返回值**：
- 相等：true
- 不相等：false（如果任一字符串为NULL，返回false）

#### `F_string_equals_cstr`
```c
bool F_string_equals_cstr(Stru_String_t* str, const char* cstr);
```
与C字符串比较。比较字符串与C字符串的内容是否完全相同。

**参数**：
- `str`：字符串指针
- `cstr`：C字符串指针（可为NULL）

**返回值**：
- 相等：true
- 不相等：false（如果str为NULL，返回false）

**注意**：如果cstr为NULL，比较字符串是否为空

#### `F_string_compare`
```c
int F_string_compare(Stru_String_t* str1, Stru_String_t* str2);
```
比较字符串。按字典顺序比较两个字符串。

**参数**：
- `str1`：第一个字符串指针
- `str2`：第二个字符串指针

**返回值**：
- str1 < str2：负值
- str1 == str2：0
- str1 > str2：正值
- 如果任一字符串为NULL，返回-1

#### `F_string_compare_cstr`
```c
int F_string_compare_cstr(Stru_String_t* str, const char* cstr);
```
与C字符串比较。按字典顺序比较字符串与C字符串。

**参数**：
- `str`：字符串指针
- `cstr`：C字符串指针（可为NULL）

**返回值**：
- str < cstr：负值
- str == cstr：0
- str > cstr：正值
- 如果str为NULL，返回-1

### 子字符串操作

#### `F_string_substring`
```c
Stru_String_t* F_string_substring(Stru_String_t* str, size_t start, size_t length);
```
获取子字符串。从指定位置开始提取指定长度的子字符串。

**参数**：
- `str`：源字符串指针
- `start`：起始位置（字符索引，从0开始）
- `length`：要提取的长度（字符数）

**返回值**：
- 成功：指向新子字符串的指针
- 失败：NULL（如果str为NULL，或参数无效）

**注意**：如果start超出范围，返回NULL；如果length超出范围，提取到字符串末尾

#### `F_string_slice`
```c
Stru_String_t* F_string_slice(Stru_String_t* str, size_t start, size_t end);
```
获取字符串切片。提取从start到end（不包括end）的字符串切片。

**参数**：
- `str`：源字符串指针
- `start`：起始位置（字符索引，从0开始）
- `end`：结束位置（字符索引，不包括此位置）

**返回值**：
- 成功：指向新字符串切片的指针
- 失败：NULL（如果str为NULL，或参数无效）

**注意**：如果start >= end，返回空字符串；如果end超出范围，提取到字符串末尾

## 使用示例

```c
#include "CN_string_operations.h"
#include <stdio.h>

int main() {
    // 创建字符串
    Stru_String_t* str1 = F_create_string("Hello");
    Stru_String_t* str2 = F_create_string(" World");
    
    // 追加字符串
    F_string_append(str1, str2);
    printf("追加后: %s\n", F_string_c_str(str1));
    
    // 追加C字符串
    F_string_append_cstr(str1, "!");
    printf("追加C字符串后: %s\n", F_string_c_str(str1));
    
    // 追加Unicode字符
    F_string_append_char(str1, 0x1F600); // 😀
    printf("追加表情符号后: %s\n", F_string_c_str(str1));
    
    // 复制字符串
    Stru_String_t* copy = F_string_copy(str1);
    if (copy) {
        printf("复制结果: %s\n", F_string_c_str(copy));
    }
    
    // 比较字符串
    Stru_String_t* compare_str = F_create_string("Hello World!😀");
    bool equal = F_string_equals(str1, compare_str);
    printf("字符串相等: %s\n", equal ? "是" : "否");
    
    // 与C字符串比较
    equal = F_string_equals_cstr(str1, "Hello World!😀");
    printf("与C字符串相等: %s\n", equal ? "是" : "否");
    
    // 字典顺序比较
    Stru_String_t* str_a = F_create_string("apple");
    Stru_String_t* str_b = F_create_string("banana");
    int cmp_result = F_string_compare(str_a, str_b);
    printf("'apple' vs 'banana': %d\n", cmp_result);
    
    // 获取子字符串
    Stru_String_t* substr = F_string_substring(str1, 6, 5); // "World"
    if (substr) {
        printf("子字符串: %s\n", F_string_c_str(substr));
        F_destroy_string(substr);
    }
    
    // 获取字符串切片
    Stru_String_t* slice = F_string_slice(str1, 0, 5); // "Hello"
    if (slice) {
        printf("字符串切片: %s\n", F_string_c_str(slice));
        F_destroy_string(slice);
    }
    
    // 赋值操作
    Stru_String_t* target = F_create_string("原始内容");
    F_string_assign(target, str1);
    printf("赋值后: %s\n", F_string_c_str(target));
    
    // 清理
    F_destroy_string(str1);
    F_destroy_string(str2);
    F_destroy_string(copy);
    F_destroy_string(compare_str);
    F_destroy_string(str_a);
    F_destroy_string(str_b);
    F_destroy_string(target);
    
    return 0;
}
```

## 内部实现

### 追加操作实现

追加操作使用以下策略：
1. **容量检查**：检查目标字符串是否有足够容量
2. **自动扩容**：如果容量不足，自动扩容
3. **内存复制**：使用memcpy高效复制数据
4. **UTF-8处理**：正确处理多字节字符的边界

### 比较操作实现

比较操作使用以下策略：
1. **快速路径**：先比较字节长度，如果不同直接返回结果
2. **逐字节比较**：使用memcmp进行高效比较
3. **UTF-8感知**：虽然使用字节比较，但UTF-8编码保证比较正确性

### 子字符串操作实现

子字符串操作使用以下策略：
1. **边界检查**：验证起始位置和长度参数
2. **UTF-8定位**：将字符位置转换为字节位置
3. **内存分配**：分配适当大小的内存
4. **数据复制**：复制子字符串数据

### 错误处理

所有函数都包含完整的错误检查：
1. **参数验证**：检查NULL指针和无效参数
2. **边界检查**：检查索引范围、容量等边界条件
3. **内存分配检查**：检查malloc/realloc是否成功
4. **UTF-8验证**：验证Unicode码点的有效性

## 性能特征

- **时间复杂度**：
  - 追加操作：平均O(n)，其中n是源字符串长度
  - 比较操作：平均O(min(n,m))，其中n和m是字符串长度
  - 复制操作：O(n)，其中n是源字符串长度
  - 子字符串操作：O(k)，其中k是子字符串长度

- **空间复杂度**：
  - 追加操作：可能需要O(n)额外空间（如果需要扩容）
  - 复制操作：O(n)额外空间
  - 子字符串操作：O(k)额外空间

## 优化策略

### 内存预分配

对于频繁的追加操作，可以使用预分配策略：
```c
// 预分配足够容量
F_string_reserve(dest, F_string_byte_length(dest) + expected_additional_bytes);
// 然后进行多次追加操作
```

### 批量操作

对于多个字符串的追加，可以使用批量操作模式：
```c
// 先计算总长度
size_t total_len = 0;
for (int i = 0; i < count; i++) {
    total_len += F_string_byte_length(strings[i]);
}
// 预分配
F_string_reserve(dest, F_string_byte_length(dest) + total_len);
// 逐个追加
for (int i = 0; i < count; i++) {
    F_string_append(dest, strings[i]);
}
```

## 依赖关系

- C标准库：`stdlib.h`, `string.h`, `stddef.h`, `stdbool.h`, `stdint.h`
- 项目依赖：`CN_string_core.h`

## 测试

操作模块包含完整的单元测试，覆盖所有API接口和边界条件。测试文件位于`tests/infrastructure/containers/string/test_string_operations.c`。

测试覆盖率目标：
- 语句覆盖率：≥95%
- 分支覆盖率：≥85%
- 函数覆盖率：100%

## 相关文档

- [字符串模块主文档](../README.md)
- [字符串核心模块文档](../string_core/README.md)
- [字符串搜索模块文档](../string_search/README.md)
- [字符串转换模块文档](../string_transform/README.md)
- [字符串工具模块文档](../string_utils/README.md)

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本字符串操作 |
| 2.0.0 | 2026-01-07 | 模块化重构，作为操作模块 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
