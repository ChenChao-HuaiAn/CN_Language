# 字符串转换模块 (String Transform Module)

## 概述

字符串转换模块提供了字符串的转换功能，包括大小写转换、空白处理、编码转换等。这些功能对于文本规范化、数据清洗和格式处理非常重要。

## 特性

- **Unicode支持**：完整支持Unicode字符的大小写转换
- **多语言支持**：正确处理中文、英文、表情符号等
- **内存安全**：自动处理内存分配和边界检查
- **原地操作**：大多数操作在原地进行，减少内存分配
- **编码转换**：支持UTF-8和UTF-16之间的转换

## API 接口

### 大小写转换

#### `F_string_to_lower`
```c
bool F_string_to_lower(Stru_String_t* str);
```
转换为小写。将字符串中的所有字符转换为小写。

**参数**：
- `str`：要转换的字符串指针

**返回值**：
- 成功：true
- 失败：false（如果str为NULL，或内存分配失败）

**注意**：只影响字母字符，其他字符保持不变

#### `F_string_to_upper`
```c
bool F_string_to_upper(Stru_String_t* str);
```
转换为大写。将字符串中的所有字符转换为大写。

**参数**：
- `str`：要转换的字符串指针

**返回值**：
- 成功：true
- 失败：false（如果str为NULL，或内存分配失败）

**注意**：只影响字母字符，其他字符保持不变

#### `F_string_to_title`
```c
bool F_string_to_title(Stru_String_t* str);
```
转换为标题格式。将字符串转换为标题格式（每个单词首字母大写）。

**参数**：
- `str`：要转换的字符串指针

**返回值**：
- 成功：true
- 失败：false（如果str为NULL，或内存分配失败）

**注意**：根据Unicode标准识别单词边界

### 空白处理

#### `F_string_trim`
```c
bool F_string_trim(Stru_String_t* str);
```
去除两端空白。去除字符串开头和结尾的所有空白字符。

**参数**：
- `str`：要处理的字符串指针

**返回值**：
- 成功：true
- 失败：false（如果str为NULL，或内存分配失败）

**注意**：空白字符包括空格、制表符、换行符等

#### `F_string_trim_left`
```c
bool F_string_trim_left(Stru_String_t* str);
```
去除左侧空白。去除字符串开头的所有空白字符。

**参数**：
- `str`：要处理的字符串指针

**返回值**：
- 成功：true
- 失败：false（如果str为NULL，或内存分配失败）

#### `F_string_trim_right`
```c
bool F_string_trim_right(Stru_String_t* str);
```
去除右侧空白。去除字符串结尾的所有空白字符。

**参数**：
- `str`：要处理的字符串指针

**返回值**：
- 成功：true
- 失败：false（如果str为NULL，或内存分配失败）

#### `F_string_normalize_whitespace`
```c
bool F_string_normalize_whitespace(Stru_String_t* str);
```
规范化空白字符。将连续的空白字符替换为单个空格。

**参数**：
- `str`：要处理的字符串指针

**返回值**：
- 成功：true
- 失败：false（如果str为NULL，或内存分配失败）

**注意**：同时去除开头和结尾的空白

### 编码转换

#### `F_string_to_utf8`
```c
bool F_string_to_utf8(Stru_String_t* str);
```
确保UTF-8编码。确保字符串使用有效的UTF-8编码。

**参数**：
- `str`：要处理的字符串指针

**返回值**：
- 成功：true
- 失败：false（如果str为NULL，或编码无效）

**注意**：如果字符串包含无效的UTF-8序列，会尝试修复或替换

#### `F_string_from_utf16`
```c
Stru_String_t* F_string_from_utf16(const uint16_t* utf16_data, size_t length);
```
从UTF-16创建字符串。从UTF-16编码的数据创建字符串。

**参数**：
- `utf16_data`：UTF-16编码的数据指针
- `length`：UTF-16代码单元的数量（不是字符数）

**返回值**：
- 成功：指向新创建的字符串的指针
- 失败：NULL（如果utf16_data为NULL，或内存分配失败）

#### `F_string_to_utf16`
```c
bool F_string_to_utf16(Stru_String_t* str, uint16_t** out_buffer, size_t* out_length);
```
转换为UTF-16。将字符串转换为UTF-16编码。

**参数**：
- `str`：源字符串指针
- `out_buffer`：输出参数，指向分配的UTF-16缓冲区
- `out_length`：输出参数，UTF-16代码单元的数量

**返回值**：
- 成功：true
- 失败：false（如果str、out_buffer或out_length为NULL，或内存分配失败）

**注意**：调用者负责释放out_buffer指向的内存

## 使用示例

```c
#include "CN_string_transform.h"
#include <stdio.h>

int main() {
    // 大小写转换示例
    Stru_String_t* str1 = F_create_string("Hello World! 你好，世界！");
    F_string_to_upper(str1);
    printf("转换为大写: %s\n", F_string_c_str(str1));
    
    F_string_to_lower(str1);
    printf("转换为小写: %s\n", F_string_c_str(str1));
    
    Stru_String_t* title_str = F_create_string("hello world example");
    F_string_to_title(title_str);
    printf("转换为标题: %s\n", F_string_c_str(title_str));
    
    // 空白处理示例
    Stru_String_t* whitespace_str = F_create_string("   太多空白   字符  在这里   ");
    printf("原始字符串: '%s'\n", F_string_c_str(whitespace_str));
    
    F_string_trim(whitespace_str);
    printf("去除两端空白后: '%s'\n", F_string_c_str(whitespace_str));
    
    Stru_String_t* messy_str = F_create_string("  这  是  一个  测试  ");
    F_string_normalize_whitespace(messy_str);
    printf("规范化空白后: '%s'\n", F_string_c_str(messy_str));
    
    // 编码转换示例
    Stru_String_t* chinese_str = F_create_string("中文测试");
    
    // 转换为UTF-16
    uint16_t* utf16_buffer = NULL;
    size_t utf16_length = 0;
    if (F_string_to_utf16(chinese_str, &utf16_buffer, &utf16_length)) {
        printf("UTF-16转换成功，长度: %zu\n", utf16_length);
        
        // 从UTF-16转换回来
        Stru_String_t* roundtrip_str = F_string_from_utf16(utf16_buffer, utf16_length);
        if (roundtrip_str) {
            printf("往返转换结果: %s\n", F_string_c_str(roundtrip_str));
            F_destroy_string(roundtrip_str);
        }
        
        // 释放UTF-16缓冲区
        free(utf16_buffer);
    }
    
    // 复杂示例：处理用户输入
    Stru_String_t* user_input = F_create_string("  user@EXAMPLE.COM  ");
    printf("原始输入: '%s'\n", F_string_c_str(user_input));
    
    // 去除空白并转换为小写
    F_string_trim(user_input);
    F_string_to_lower(user_input);
    printf("处理后: '%s'\n", F_string_c_str(user_input));
    
    // 清理
    F_destroy_string(str1);
    F_destroy_string(title_str);
    F_destroy_string(whitespace_str);
    F_destroy_string(messy_str);
    F_destroy_string(chinese_str);
    F_destroy_string(user_input);
    
    return 0;
}
```

## 内部实现

### 大小写转换算法

大小写转换使用Unicode标准，具有以下特点：

#### 1. Unicode大小写映射
使用Unicode标准的大小写映射表：
```c
// 简化的映射表结构
typedef struct {
    uint32_t code_point;
    uint32_t* mapping;  // 大小写映射
    size_t mapping_len;
} CaseMapping;

// 查找大小写映射
const CaseMapping* find_case_mapping(uint32_t code_point);
```

#### 2. 特殊字符处理
处理特殊的大小写转换规则：
- **土耳其语点i**：İ → i, ı → I
- **希腊语sigma**：Σ → σ（词尾ς）
- **德语sharp s**：ß → SS（大写）

#### 3. 上下文感知转换
某些字符的大小写转换依赖于上下文：
```c
// 希腊语sigma的上下文感知转换
if (code_point == 0x03A3) { // Σ
    if (is_word_final(context)) {
        return 0x03C2; // ς
    } else {
        return 0x03C3; // σ
    }
}
```

### 空白处理算法

空白处理使用Unicode标准，识别各种空白字符：

#### 1. Unicode空白字符分类
```c
// 检查是否为空白字符
bool is_whitespace(uint32_t code_point) {
    switch (code_point) {
        case 0x0020: // 空格
        case 0x0009: // 制表符
        case 0x000A: // 换行符
        case 0x000D: // 回车符
        case 0x00A0: // 不换行空格
        case 0x1680: // 欧甘文空格
        case 0x2000: // 半身空距
        case 0x2001: // 全身空距
        case 0x2002: // 半身空距
        case 0x2003: // 全身空距
        case 0x2004: // 三分之一空距
        case 0x2005: // 四分之一空距
        case 0x2006: // 六分之一空距
        case 0x2007: // 数字空格
        case 0x2008: // 标点空格
        case 0x2009: // 细空格
        case 0x200A: // 毛发空格
        case 0x2028: // 行分隔符
        case 0x2029: // 段分隔符
        case 0x202F: // 窄不换行空格
        case 0x205F: // 中等数学空格
        case 0x3000: // 表意文字空格
            return true;
        default:
            return false;
    }
}
```

#### 2. 高效空白去除算法
使用双指针技术进行原地操作：
```c
// 去除左侧空白
size_t left = 0;
while (left < len && is_whitespace(str[left])) {
    left++;
}
// 去除右侧空白
size_t right = len;
while (right > left && is_whitespace(str[right - 1])) {
    right--;
}
// 移动数据
memmove(str, str + left, right - left);
str[right - left] = '\0';
```

### 编码转换算法

编码转换处理UTF-8和UTF-16之间的转换：

#### 1. UTF-8到UTF-16转换
```c
// UTF-8到UTF-16转换的核心逻辑
while (utf8_pos < utf8_len) {
    uint32_t code_point = decode_utf8(utf8_data, &utf8_pos);
    encode_utf16(code_point, utf16_buffer, &utf16_pos);
}
```

#### 2. UTF-16到UTF-8转换
```c
// UTF-16到UTF-8转换的核心逻辑
while (utf16_pos < utf16_len) {
    uint32_t code_point = decode_utf16(utf16_data, &utf16_pos);
    encode_utf8(code_point, utf8_buffer, &utf8_pos);
}
```

#### 3. 代理对处理
正确处理UTF-16代理对：
```c
// 检查是否为高代理
bool is_high_surrogate(uint16_t code_unit) {
    return (code_unit >= 0xD800 && code_unit <= 0xDBFF);
}

// 检查是否为低代理
bool is_low_surrogate(uint16_t code_unit) {
    return (code_unit >= 0xDC00 && code_unit <= 0xDFFF);
}

// 从代理对解码码点
uint32_t decode_surrogate_pair(uint16_t high, uint16_t low) {
    return 0x10000 + ((high - 0xD800) << 10) + (low - 0xDC00);
}
```

### 错误处理

所有函数都包含完整的错误检查：
1. **参数验证**：检查NULL指针和无效参数
2. **编码验证**：检查UTF-8/UTF-16编码的有效性
3. **内存分配检查**：检查malloc/realloc是否成功
4. **边界检查**：检查索引范围、缓冲区大小等边界条件

## 性能特征

- **时间复杂度**：
  - 大小写转换：O(n)，其中n是字符串长度
  - 空白处理：O(n)，其中n是字符串长度
  - 编码转换：O(n)，其中n是字符串长度

- **空间复杂度**：
  - 大小写转换：O(1)额外空间（原地操作）
  - 空白处理：O(1)额外空间（原地操作）
  - 编码转换：O(n)额外空间（需要新缓冲区）

## 优化策略

### 1. 查表优化
使用查找表加速大小写转换：
```c
// 预计算的大小写映射表
static const uint32_t lower_to_upper[65536];
static const uint32_t upper_to_lower[65536];

// 快速查表转换
uint32_t to_upper_fast(uint32_t code_point) {
    if (code_point < 65536) {
        return lower_to_upper[code_point];
    }
    return slow_to_upper(code_point); // 回退到慢速路径
}
```

### 2. 批量操作
对于多个转换操作，使用批量处理：
```c
// 批量转换：先收集所有操作，然后一次性应用
typedef struct {
    enum { OP_LOWER, OP_UPPER, OP_TRIM } operation;
    size_t start;
    size_t end;
} StringOperation;

// 应用批量操作
bool apply_operations(Stru_String_t* str, StringOperation* ops, size_t count);
```

### 3. 内存重用
对于频繁的转换操作，重用内存缓冲区：
```c
// 线程局部的转换缓冲区
__thread char* conversion_buffer = NULL;
__thread size_t conversion_buffer_size = 0;

// 使用可重用的缓冲区
char* get_conversion_buffer(size_t required_size) {
    if (conversion_buffer_size < required_size) {
        conversion_buffer = realloc(conversion_buffer, required_size);
        conversion_buffer_size = required_size;
    }
    return conversion_buffer;
}
```

## 依赖关系

- C标准库：`stdlib.h`, `string.h`, `stddef.h`, `stdbool.h`, `stdint.h`, `wchar.h`
- 项目依赖：`CN_string_core.h`, `CN_string_operations.h`

## 测试

转换模块包含完整的单元测试，覆盖所有API接口和边界条件。测试文件位于`tests/infrastructure/containers/string/test_string_transform.c`。

测试覆盖率目标：
- 语句覆盖率：≥95%
- 分支覆盖率：≥85%
- 函数覆盖率：100%

## 相关文档

- [字符串模块主文档](../README.md)
- [字符串核心模块文档](../string_core/README.md)
- [字符串操作模块文档](../string_operations/README.md)
- [字符串搜索模块文档](../string_search/README.md)
- [字符串工具模块文档](../string_utils/README.md)

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本转换功能 |
| 2.0.0 | 2026-01-07 | 模块化重构，作为转换模块 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
