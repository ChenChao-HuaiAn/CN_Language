# CN_string API 文档

## 概述

`CN_string` 模块提供了动态字符串数据结构的实现，支持UTF-8编码。字符串模块采用模块化设计，分为以下子模块：

1. **核心模块** (`string_core/`) - 字符串创建、销毁、基本属性
2. **操作模块** (`string_operations/`) - 字符串追加、比较、复制、截取
3. **搜索模块** (`string_search/`) - 字符串查找、替换、分割、连接
4. **转换模块** (`string_transform/`) - 大小写转换、空白处理、反转、填充
5. **工具模块** (`string_utils/`) - UTF-8处理、字符分类、格式化、哈希计算

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
    uint32_t version;           /**< 字符串版本号 */
    uint32_t flags;             /**< 字符串标志位 */
} Stru_String_t;
```

**字段说明：**

- `data`: 指向字符串数据的指针（UTF-8编码，以null结尾）
- `length`: 字符串中的字符数（不是字节数）
- `capacity`: 当前分配的容量（字节数）
- `byte_length`: 字符串的实际字节长度
- `version`: 字符串版本号，用于版本管理
- `flags`: 字符串标志位，用于存储状态信息

### Stru_StringArray_t

字符串数组结构体，用于分割和连接操作。

```c
typedef struct Stru_StringArray_t
{
    Stru_String_t** items;      /**< 字符串指针数组 */
    size_t count;               /**< 字符串数量 */
    size_t capacity;            /**< 数组容量 */
} Stru_StringArray_t;
```

### Stru_UTF8Iterator_t

UTF-8字符迭代器结构体。

```c
typedef struct Stru_UTF8Iterator_t
{
    const char* data;           /**< 字符串数据 */
    size_t length;              /**< 字符串长度（字节） */
    size_t position;            /**< 当前位置（字节） */
    size_t char_index;          /**< 字符索引 */
} Stru_UTF8Iterator_t;
```

## 核心模块函数

### 字符串创建和销毁

```c
Stru_String_t* F_string_create_empty(void);
Stru_String_t* F_string_create_from_cstr(const char* cstr);
Stru_String_t* F_string_create_copy(const Stru_String_t* src);
Stru_String_t* F_string_create_with_capacity(size_t capacity);
void F_string_destroy(Stru_String_t* str);
```

### 字符串基本属性

```c
size_t F_string_length(const Stru_String_t* str);
size_t F_string_capacity(const Stru_String_t* str);
size_t F_string_byte_length(const Stru_String_t* str);
const char* F_string_cstr(const Stru_String_t* str);
bool F_string_is_empty(const Stru_String_t* str);
```

### 字符串容量管理

```c
bool F_string_reserve(Stru_String_t* str, size_t new_capacity);
bool F_string_shrink_to_fit(Stru_String_t* str);
void F_string_clear(Stru_String_t* str);
```

## 操作模块函数

### 字符串追加

```c
bool F_string_append_cstr(Stru_String_t* str, const char* cstr);
bool F_string_append_char(Stru_String_t* str, char ch);
bool F_string_append_format(Stru_String_t* str, const char* format, ...);
bool F_string_append_string(Stru_String_t* dest, const Stru_String_t* src);
```

### 字符串比较

```c
bool F_string_equals(const Stru_String_t* str1, const Stru_String_t* str2);
bool F_string_equals_cstr(const Stru_String_t* str, const char* cstr);
int F_string_compare(const Stru_String_t* str1, const Stru_String_t* str2);
bool F_string_starts_with(const Stru_String_t* str, const char* prefix);
bool F_string_ends_with(const Stru_String_t* str, const char* suffix);
```

### 字符串复制和截取

```c
bool F_string_assign_cstr(Stru_String_t* str, const char* cstr);
bool F_string_assign_string(Stru_String_t* dest, const Stru_String_t* src);
Stru_String_t* F_string_substring(const Stru_String_t* str, size_t start, size_t length);
bool F_string_truncate(Stru_String_t* str, size_t new_length);
```

### 字符串插入和删除

```c
bool F_string_insert_cstr(Stru_String_t* str, size_t pos, const char* cstr);
bool F_string_insert_char(Stru_String_t* str, size_t pos, char ch);
bool F_string_insert_string(Stru_String_t* str, size_t pos, const Stru_String_t* insert_str);
bool F_string_erase(Stru_String_t* str, size_t pos, size_t count);
```

## 搜索模块函数

### 字符串查找

```c
size_t F_string_find(const Stru_String_t* str, const char* substr, size_t start_pos);
size_t F_string_find_char(const Stru_String_t* str, char ch, size_t start_pos);
size_t F_string_rfind(const Stru_String_t* str, const char* substr, size_t start_pos);
size_t F_string_rfind_char(const Stru_String_t* str, char ch, size_t start_pos);
```

### 字符串包含和计数

```c
bool F_string_contains(const Stru_String_t* str, const char* substr);
bool F_string_contains_cstr(const Stru_String_t* str, const char* cstr);
size_t F_string_count(const Stru_String_t* str, const char* substr);
size_t F_string_count_char(const Stru_String_t* str, char ch);
```

### 字符串替换

```c
bool F_string_replace(Stru_String_t* str, size_t pos, size_t count, const char* replacement);
size_t F_string_replace_all(Stru_String_t* str, const char* old_substr, const char* new_substr);
size_t F_string_replace_all_cstr(Stru_String_t* str, const char* old_cstr, const char* new_cstr);
```

### 字符串分割和连接

```c
Stru_StringArray_t* F_string_split(const Stru_String_t* str, const char* delimiter);
Stru_StringArray_t* F_string_split_cstr(const char* cstr, const char* delimiter);
Stru_String_t* F_string_join(const Stru_StringArray_t* array, const char* separator);
```

## 转换模块函数

### 大小写转换

```c
bool F_string_to_lower(Stru_String_t* str);
bool F_string_to_upper(Stru_String_t* str);
bool F_string_capitalize(Stru_String_t* str);
bool F_string_title(Stru_String_t* str);
bool F_string_swap_case(Stru_String_t* str);
```

### 空白处理

```c
bool F_string_trim_left(Stru_String_t* str);
bool F_string_trim_right(Stru_String_t* str);
bool F_string_trim(Stru_String_t* str);
bool F_string_compress_whitespace(Stru_String_t* str);
bool F_string_is_whitespace(const Stru_String_t* str);
```

### 字符串反转和填充

```c
bool F_string_reverse(Stru_String_t* str);
bool F_string_pad_left(Stru_String_t* str, size_t total_length, char pad_char);
bool F_string_pad_right(Stru_String_t* str, size_t total_length, char pad_char);
bool F_string_pad_center(Stru_String_t* str, size_t total_length, char pad_char);
```

### 编码转换

```c
Stru_String_t* F_string_escape(const Stru_String_t* str);
Stru_String_t* F_string_unescape(const Stru_String_t* str);
Stru_String_t* F_string_url_encode(const Stru_String_t* str);
Stru_String_t* F_string_url_decode(const Stru_String_t* str);
Stru_String_t* F_string_html_encode(const Stru_String_t* str);
Stru_String_t* F_string_html_decode(const Stru_String_t* str);
```

## 工具模块函数

### UTF-8处理

```c
bool F_string_is_valid_utf8(const Stru_String_t* str);
Stru_String_t* F_string_fix_utf8(const Stru_String_t* str);
size_t F_string_count_utf8_chars(const Stru_String_t* str);
Stru_String_t* F_string_get_utf8_char(const Stru_String_t* str, size_t char_index);
uint32_t F_string_get_utf8_codepoint(const Stru_String_t* str, size_t char_index);
Stru_String_t* F_string_create_from_utf8_codepoint(uint32_t codepoint);
```

### 字符分类

```c
bool F_string_is_alpha(const Stru_String_t* str);
bool F_string_is_digit(const Stru_String_t* str);
bool F_string_is_alnum(const Stru_String_t* str);
bool F_string_is_ascii(const Stru_String_t* str);
bool F_string_is_lower(const Stru_String_t* str);
bool F_string_is_upper(const Stru_String_t* str);
bool F_string_is_xdigit(const Stru_String_t* str);
bool F_string_is_printable(const Stru_String_t* str);
```

### 格式化功能

```c
Stru_String_t* F_string_format(const char* format, ...);
bool F_string_parse_int(const Stru_String_t* str, int* value);
bool F_string_parse_float(const Stru_String_t* str, double* value);
bool F_string_parse_bool(const Stru_String_t* str, bool* value);
int F_string_to_int(const Stru_String_t* str);
double F_string_to_float(const Stru_String_t* str);
Stru_String_t* F_string_from_int(int value);
Stru_String_t* F_string_from_float(double value, int precision);
```

### 校验和哈希

```c
uint32_t F_string_hash(const Stru_String_t* str);
uint32_t F_string_crc32(const Stru_String_t* str);
bool F_string_md5(const Stru_String_t* str, char* output, size_t output_size);
bool F_string_sha1(const Stru_String_t* str, char* output, size_t output_size);
uint16_t F_string_checksum(const Stru_String_t* str);
```

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

### STRING_NPOS
```c
#define STRING_NPOS ((size_t)-1)
```
表示未找到的特殊值。

## 使用示例

```c
#include "infrastructure/containers/string/CN_string.h"
#include <stdio.h>

int main() {
    // 创建字符串
    Stru_String_t* str = F_string_create_from_cstr("Hello, 世界! 🌍");
    
    // 基本属性
    printf("长度: %zu 字符, %zu 字节\n", 
           F_string_length(str), 
           F_string_byte_length(str));
    
    // 追加操作
    F_string_append_cstr(str, " Welcome!");
    
    // 查找操作
    size_t pos = F_string_find(str, "世界", 0);
    if (pos != STRING_NPOS) {
        printf("找到'世界'在位置: %zu\n", pos);
    }
    
    // 替换操作
    F_string_replace_all(str, "Hello", "Hi");
    
    // 转换操作
    Stru_String_t* upper = F_string_create_copy(str);
    F_string_to_upper(upper);
    printf("大写: %s\n", F_string_cstr(upper));
    
    // 分割操作
    Stru_StringArray_t* parts = F_string_split(str, " ");
    printf("分割为 %zu 部分\n", parts->count);
    
    // 清理
    F_string_destroy(str);
    F_string_destroy(upper);
    // 注意：需要实现F_string_array_destroy来清理parts
    
    return 0;
}
```

## UTF-8支持

字符串模块完全支持UTF-8编码：

1. **长度计算**：正确计算Unicode字符数
2. **子字符串**：按字符位置操作
3. **大小写转换**：支持Unicode字符（部分语言）
4. **查找操作**：正确处理多字节字符
5. **字符分类**：支持Unicode字符分类

## 性能特征

### 时间复杂度
- 创建：O(1)
- 销毁：O(1)
- 追加：平均O(1)，最坏O(n)（需要扩容）
- 比较：O(n)
- 复制：O(n)
- 子字符串：O(n)
- 查找：O(n*m)，其中n是字符串长度，m是子字符串长度
- UTF-8操作：O(n)

### 空间复杂度
- O(n)，其中n是字符串容量

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本 |
| 2.0.0 | 2026-01-07 | 模块化重构，新增功能 |

## 相关文档

- [字符串模块README](../../../../src/infrastructure/containers/string/README.md)
- [核心模块文档](../../../../src/infrastructure/containers/string/string_core/README.md)
- [操作模块文档](../../../../src/infrastructure/containers/string/string_operations/README.md)
- [搜索模块文档](../../../../src/infrastructure/containers/string/string_search/README.md)
- [转换模块文档](../../../../src/infrastructure/containers/string/string_transform/README.md)
- [工具模块文档](../../../../src/infrastructure/containers/string/string_utils/README.md)
- [CN_Language项目架构文档](../../../../architecture/001-中文编程语言CN_Language开发规划.md)
