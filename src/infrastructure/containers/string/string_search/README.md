# 字符串搜索模块 (String Search Module)

## 概述

字符串搜索模块提供了字符串的搜索、查找和替换功能。这些功能对于文本处理、模式匹配和字符串操作至关重要。

## 特性

- **高效搜索**：使用优化的搜索算法
- **UTF-8支持**：正确处理多字节字符的搜索
- **双向搜索**：支持正向和反向搜索
- **模式匹配**：支持前缀、后缀和包含检查
- **替换功能**：支持单次和全局替换
- **内存安全**：自动处理内存分配和边界检查

## API 接口

### 查找操作

#### `F_string_find`
```c
size_t F_string_find(Stru_String_t* str, Stru_String_t* substr);
```
查找子字符串。在字符串中查找子字符串的首次出现位置。

**参数**：
- `str`：源字符串指针
- `substr`：要查找的子字符串指针

**返回值**：
- 找到：子字符串的起始位置（字符索引，从0开始）
- 未找到：SIZE_MAX
- 错误：SIZE_MAX（如果str或substr为NULL，或substr为空）

#### `F_string_find_cstr`
```c
size_t F_string_find_cstr(Stru_String_t* str, const char* substr);
```
查找C子字符串。在字符串中查找C子字符串的首次出现位置。

**参数**：
- `str`：源字符串指针
- `substr`：要查找的C子字符串指针（可为NULL）

**返回值**：
- 找到：子字符串的起始位置（字符索引，从0开始）
- 未找到：SIZE_MAX
- 错误：SIZE_MAX（如果str为NULL，或substr为NULL/空）

#### `F_string_find_char`
```c
size_t F_string_find_char(Stru_String_t* str, uint32_t codepoint);
```
查找字符。在字符串中查找Unicode字符的首次出现位置。

**参数**：
- `str`：源字符串指针
- `codepoint`：要查找的Unicode码点

**返回值**：
- 找到：字符的位置（字符索引，从0开始）
- 未找到：SIZE_MAX
- 错误：SIZE_MAX（如果str为NULL）

#### `F_string_rfind`
```c
size_t F_string_rfind(Stru_String_t* str, Stru_String_t* substr);
```
从后向前查找子字符串。在字符串中从后向前查找子字符串的首次出现位置。

**参数**：
- `str`：源字符串指针
- `substr`：要查找的子字符串指针

**返回值**：
- 找到：子字符串的起始位置（字符索引，从0开始）
- 未找到：SIZE_MAX
- 错误：SIZE_MAX（如果str或substr为NULL，或substr为空）

#### `F_string_rfind_cstr`
```c
size_t F_string_rfind_cstr(Stru_String_t* str, const char* substr);
```
从后向前查找C子字符串。在字符串中从后向前查找C子字符串的首次出现位置。

**参数**：
- `str`：源字符串指针
- `substr`：要查找的C子字符串指针（可为NULL）

**返回值**：
- 找到：子字符串的起始位置（字符索引，从0开始）
- 未找到：SIZE_MAX
- 错误：SIZE_MAX（如果str为NULL，或substr为NULL/空）

### 替换操作

#### `F_string_replace`
```c
bool F_string_replace(Stru_String_t* str, Stru_String_t* old_substr, Stru_String_t* new_substr);
```
替换子字符串。替换字符串中首次出现的子字符串。

**参数**：
- `str`：要修改的字符串指针
- `old_substr`：要替换的旧子字符串指针
- `new_substr`：替换后的新子字符串指针

**返回值**：
- 成功：true（即使未找到old_substr也返回true）
- 失败：false（如果str、old_substr或new_substr为NULL，或内存分配失败）

#### `F_string_replace_cstr`
```c
bool F_string_replace_cstr(Stru_String_t* str, const char* old_substr, const char* new_substr);
```
替换C子字符串。替换字符串中首次出现的C子字符串。

**参数**：
- `str`：要修改的字符串指针
- `old_substr`：要替换的旧C子字符串指针（可为NULL）
- `new_substr`：替换后的新C子字符串指针（可为NULL）

**返回值**：
- 成功：true（即使未找到old_substr也返回true）
- 失败：false（如果str为NULL，或内存分配失败）

**注意**：如果old_substr为NULL或空，不执行替换；如果new_substr为NULL，使用空字符串替换

#### `F_string_replace_all`
```c
bool F_string_replace_all(Stru_String_t* str, Stru_String_t* old_substr, Stru_String_t* new_substr);
```
替换所有匹配的子字符串。替换字符串中所有出现的子字符串。

**参数**：
- `str`：要修改的字符串指针
- `old_substr`：要替换的旧子字符串指针
- `new_substr`：替换后的新子字符串指针

**返回值**：
- 成功：true（即使未找到old_substr也返回true）
- 失败：false（如果str、old_substr或new_substr为NULL，或内存分配失败）

### 模式匹配

#### `F_string_starts_with`
```c
bool F_string_starts_with(Stru_String_t* str, Stru_String_t* prefix);
```
检查是否以指定前缀开头。检查字符串是否以指定的前缀开头。

**参数**：
- `str`：字符串指针
- `prefix`：前缀字符串指针

**返回值**：
- 是：true
- 否：false（如果str或prefix为NULL，或prefix为空）

#### `F_string_starts_with_cstr`
```c
bool F_string_starts_with_cstr(Stru_String_t* str, const char* prefix);
```
检查是否以C前缀开头。检查字符串是否以指定的C前缀开头。

**参数**：
- `str`：字符串指针
- `prefix`：C前缀字符串指针（可为NULL）

**返回值**：
- 是：true
- 否：false（如果str为NULL，或prefix为NULL/空）

#### `F_string_ends_with`
```c
bool F_string_ends_with(Stru_String_t* str, Stru_String_t* suffix);
```
检查是否以指定后缀结尾。检查字符串是否以指定的后缀结尾。

**参数**：
- `str`：字符串指针
- `suffix`：后缀字符串指针

**返回值**：
- 是：true
- 否：false（如果str或suffix为NULL，或suffix为空）

#### `F_string_ends_with_cstr`
```c
bool F_string_ends_with_cstr(Stru_String_t* str, const char* suffix);
```
检查是否以C后缀结尾。检查字符串是否以指定的C后缀结尾。

**参数**：
- `str`：字符串指针
- `suffix`：C后缀字符串指针（可为NULL）

**返回值**：
- 是：true
- 否：false（如果str为NULL，或suffix为NULL/空）

#### `F_string_contains`
```c
bool F_string_contains(Stru_String_t* str, Stru_String_t* substr);
```
检查是否包含子字符串。检查字符串是否包含指定的子字符串。

**参数**：
- `str`：字符串指针
- `substr`：子字符串指针

**返回值**：
- 包含：true
- 不包含：false（如果str或substr为NULL，或substr为空）

## 使用示例

```c
#include "CN_string_search.h"
#include <stdio.h>

int main() {
    // 创建测试字符串
    Stru_String_t* str = F_create_string("这是一个测试字符串，测试字符串的功能。");
    
    // 查找子字符串
    size_t pos = F_string_find_cstr(str, "测试");
    printf("'测试'首次出现位置: %zu\n", pos);
    
    // 从后向前查找
    size_t rpos = F_string_rfind_cstr(str, "测试");
    printf("'测试'最后出现位置: %zu\n", rpos);
    
    // 查找字符
    size_t char_pos = F_string_find_char(str, '测');
    printf("'测'字符位置: %zu\n", char_pos);
    
    // 替换子字符串
    F_string_replace_cstr(str, "测试", "示例");
    printf("替换后: %s\n", F_string_c_str(str));
    
    // 替换所有匹配
    Stru_String_t* str2 = F_create_string("苹果苹果苹果");
    F_string_replace_all(str2, F_create_string("苹果"), F_create_string("香蕉"));
    printf("全局替换后: %s\n", F_string_c_str(str2));
    
    // 检查前缀
    bool starts = F_string_starts_with_cstr(str, "这是一个");
    printf("以'这是一个'开头: %s\n", starts ? "是" : "否");
    
    // 检查后缀
    bool ends = F_string_ends_with_cstr(str, "功能。");
    printf("以'功能。'结尾: %s\n", ends ? "是" : "否");
    
    // 检查包含
    bool contains = F_string_contains(str, F_create_string("示例"));
    printf("包含'示例': %s\n", contains ? "是" : "否");
    
    // 复杂替换示例
    Stru_String_t* text = F_create_string("Hello World! Hello Universe!");
    F_string_replace_all(text, F_create_string("Hello"), F_create_string("Hi"));
    printf("复杂替换: %s\n", F_string_c_str(text));
    
    // 清理
    F_destroy_string(str);
    F_destroy_string(str2);
    F_destroy_string(text);
    
    return 0;
}
```

## 内部实现

### 搜索算法

搜索模块使用以下算法：

#### 1. 简单字符串匹配（Naive String Matching）
用于短字符串搜索，实现简单高效：
```c
// 简化的搜索算法
for (i = 0; i <= n - m; i++) {
    for (j = 0; j < m; j++) {
        if (str[i + j] != substr[j]) break;
    }
    if (j == m) return i; // 找到匹配
}
```

#### 2. UTF-8感知搜索
由于UTF-8编码的特性，需要特殊处理：
1. **字符边界**：确保在字符边界处开始匹配
2. **多字节字符**：正确处理多字节字符的比较
3. **位置转换**：在字符位置和字节位置之间转换

### 替换算法

替换操作使用以下策略：
1. **查找位置**：使用搜索算法找到要替换的位置
2. **计算新长度**：计算替换后的新字符串长度
3. **分配内存**：分配足够的内存
4. **构建新字符串**：复制旧字符串的部分，插入新子字符串
5. **释放旧内存**：释放旧字符串的内存

### 性能优化

#### 1. 快速失败检查
```c
// 快速失败条件
if (str == NULL || substr == NULL) return SIZE_MAX;
if (F_string_is_empty(substr)) return SIZE_MAX;
if (F_string_length(substr) > F_string_length(str)) return SIZE_MAX;
```

#### 2. 内存预分配
对于替换操作，预先计算新字符串长度，一次性分配内存：
```c
// 计算新长度
size_t old_len = F_string_byte_length(old_substr);
size_t new_len = F_string_byte_length(new_substr);
size_t diff = new_len - old_len;
size_t new_total_len = str_byte_len + (diff * occurrence_count);

// 预分配内存
F_string_reserve(str, new_total_len);
```

#### 3. 缓存友好操作
使用连续内存操作，提高缓存效率：
```c
// 使用memmove进行内存移动
memmove(dest, src, count);
```

### 错误处理

所有函数都包含完整的错误检查：
1. **参数验证**：检查NULL指针和无效参数
2. **边界检查**：检查索引范围、字符串长度等边界条件
3. **内存分配检查**：检查malloc/realloc是否成功
4. **UTF-8验证**：验证字符串的UTF-8有效性

## 性能特征

- **时间复杂度**：
  - 搜索操作：平均O(n*m)，最坏O(n*m)，其中n是字符串长度，m是子字符串长度
  - 替换操作：平均O(n+m)，其中n是字符串长度，m是新子字符串长度
  - 模式匹配：平均O(k)，其中k是前缀/后缀长度

- **空间复杂度**：
  - 搜索操作：O(1)额外空间
  - 替换操作：O(n+m)额外空间（最坏情况）
  - 全局替换：O(n + k*m)额外空间，其中k是替换次数

## 优化策略

### 1. 使用更高效的搜索算法
对于长字符串搜索，可以考虑实现更高效的算法：
- **KMP算法**：O(n+m)时间复杂度，需要预处理
- **Boyer-Moore算法**：通常比KMP更快，适合长模式
- **Rabin-Karp算法**：使用哈希，适合多个模式搜索

### 2. 批量操作优化
对于多个搜索/替换操作，可以使用批量处理：
```c
// 批量搜索
size_t positions[10];
size_t count = F_string_find_all(str, substr, positions, 10);

// 批量替换
F_string_replace_multiple(str, old_substrs, new_substrs, count);
```

### 3. 缓存搜索结果
对于频繁搜索相同模式，可以缓存搜索结果：
```c
// 缓存搜索状态
typedef struct {
    Stru_String_t* pattern;
    size_t* next; // KMP的next数组
    size_t pattern_len;
} SearchCache;

// 使用缓存进行搜索
size_t F_string_find_with_cache(Stru_String_t* str, SearchCache* cache);
```

## 依赖关系

- C标准库：`stdlib.h`, `string.h`, `stddef.h`, `stdbool.h`, `stdint.h`
- 项目依赖：`CN_string_core.h`, `CN_string_operations.h`

## 测试

搜索模块包含完整的单元测试，覆盖所有API接口和边界条件。测试文件位于`tests/infrastructure/containers/string/test_string_search.c`。

测试覆盖率目标：
- 语句覆盖率：≥95%
- 分支覆盖率：≥85%
- 函数覆盖率：100%

## 相关文档

- [字符串模块主文档](../README.md)
- [字符串核心模块文档](../string_core/README.md)
- [字符串操作模块文档](../string_operations/README.md)
- [字符串转换模块文档](../string_transform/README.md)
- [字符串工具模块文档](../string_utils/README.md)

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本搜索功能 |
| 2.0.0 | 2026-01-07 | 模块化重构，作为搜索模块 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
