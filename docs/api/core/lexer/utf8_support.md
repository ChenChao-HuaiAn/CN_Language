# UTF-8 支持模块 API 文档

## 概述

UTF-8 支持模块为 CN_Language 词法分析器提供完整的 UTF-8 编码支持，特别针对中文字符的处理进行了优化。该模块实现了 UTF-8 字符的解码、验证和分类功能。

## 数据结构

### Stru_UTF8CharInfo_t

UTF-8 字符信息结构体，包含字符的完整信息。

```c
typedef struct Stru_UTF8CharInfo_t {
    uint32_t code_point;      // Unicode 码点
    size_t byte_length;       // UTF-8 编码的字节长度
    bool is_valid;           // 是否为有效的 UTF-8 编码
    bool is_letter;          // 是否为字母字符
    bool is_chinese;         // 是否为中文字符
} Stru_UTF8CharInfo_t;
```

## API 函数

### 字符信息获取

#### `F_get_utf8_char_info`

获取 UTF-8 字符的完整信息。

```c
Stru_UTF8CharInfo_t F_get_utf8_char_info(const char* str, size_t max_len);
```

**参数：**
- `str`: UTF-8 字符串指针
- `max_len`: 最大可读取长度

**返回值：**
- `Stru_UTF8CharInfo_t`: UTF-8 字符信息结构体

**示例：**
```c
const char* chinese_char = "变";
Stru_UTF8CharInfo_t info = F_get_utf8_char_info(chinese_char, 3);
if (info.is_valid && info.is_chinese) {
    printf("中文字符: U+%04X, 字节长度: %zu\n", info.code_point, info.byte_length);
}
```

### 字符分类

#### `F_is_chinese_char`

检查 Unicode 码点是否为中文字符。

```c
bool F_is_chinese_char(uint32_t code_point);
```

**参数：**
- `code_point`: Unicode 码点

**返回值：**
- `true`: 是中文字符
- `false`: 不是中文字符

**支持的中文字符范围：**
1. 基本汉字：U+4E00 - U+9FFF
2. 扩展A区：U+3400 - U+4DBF
3. 兼容汉字：U+F900 - U+FAFF

#### `F_is_alpha_utf8_complete`

检查字符是否为字母（包括中文字符）。

```c
bool F_is_alpha_utf8_complete(const char* str, size_t max_len);
```

**参数：**
- `str`: UTF-8 字符串指针
- `max_len`: 最大可读取长度

**返回值：**
- `true`: 是字母字符
- `false`: 不是字母字符

#### `F_is_digit_utf8`

检查字符是否为数字。

```c
bool F_is_digit_utf8(const char* str, size_t max_len);
```

**参数：**
- `str`: UTF-8 字符串指针
- `max_len`: 最大可读取长度

**返回值：**
- `true`: 是数字字符
- `false`: 不是数字字符

**支持的数字类型：**
1. ASCII 数字：0-9
2. 全角数字：U+FF10 - U+FF19

#### `F_is_alnum_utf8`

检查字符是否为字母或数字。

```c
bool F_is_alnum_utf8(const char* str, size_t max_len);
```

**参数：**
- `str`: UTF-8 字符串指针
- `max_len`: 最大可读取长度

**返回值：**
- `true`: 是字母或数字字符
- `false`: 不是字母或数字字符

### 标识符字符检查

#### `F_is_identifier_start_utf8`

检查字符是否为标识符起始字符。

```c
bool F_is_identifier_start_utf8(const char* str, size_t max_len);
```

**参数：**
- `str`: UTF-8 字符串指针
- `max_len`: 最大可读取长度

**返回值：**
- `true`: 可以作为标识符起始字符
- `false`: 不能作为标识符起始字符

**有效的标识符起始字符：**
1. 下划线 (_)
2. 字母字符（包括中文字符）

#### `F_is_identifier_continue_utf8`

检查字符是否为标识符继续字符。

```c
bool F_is_identifier_continue_utf8(const char* str, size_t max_len);
```

**参数：**
- `str`: UTF-8 字符串指针
- `max_len`: 最大可读取长度

**返回值：**
- `true`: 可以作为标识符继续字符
- `false`: 不能作为标识符继续字符

**有效的标识符继续字符：**
1. 下划线 (_)
2. 字母字符（包括中文字符）
3. 数字字符

### UTF-8 编码处理

#### `F_get_utf8_char_length`

获取 UTF-8 字符的字节长度。

```c
size_t F_get_utf8_char_length(unsigned char first_byte);
```

**参数：**
- `first_byte`: UTF-8 字符的第一个字节

**返回值：**
- `size_t`: UTF-8 字符的字节长度（1-4），0 表示无效编码

**编码长度规则：**
- 0x00-0x7F: 1 字节（ASCII）
- 0xC0-0xDF: 2 字节
- 0xE0-0xEF: 3 字节
- 0xF0-0xF7: 4 字节

#### `F_decode_utf8_char`

解码 UTF-8 字符。

```c
size_t F_decode_utf8_char(const char* str, size_t length, uint32_t* code_point);
```

**参数：**
- `str`: UTF-8 字符串指针
- `length`: 字符串长度
- `code_point`: 输出参数，存储解码后的 Unicode 码点

**返回值：**
- `size_t`: 实际解码的字节数，0 表示解码失败

#### `F_is_valid_utf8`

检查 UTF-8 编码是否有效。

```c
bool F_is_valid_utf8(const char* str, size_t length);
```

**参数：**
- `str`: UTF-8 字符串指针
- `length`: 字符串长度

**返回值：**
- `true`: 有效的 UTF-8 编码
- `false`: 无效的 UTF-8 编码

### 字符串操作

#### `F_skip_utf8_chars`

跳过指定数量的 UTF-8 字符。

```c
size_t F_skip_utf8_chars(const char* str, size_t length, size_t char_count);
```

**参数：**
- `str`: UTF-8 字符串指针
- `length`: 字符串长度
- `char_count`: 要跳过的字符数

**返回值：**
- `size_t`: 实际跳过的字节数

#### `F_count_utf8_chars`

计算 UTF-8 字符串的字符数。

```c
size_t F_count_utf8_chars(const char* str, size_t length);
```

**参数：**
- `str`: UTF-8 字符串指针
- `length`: 字符串长度

**返回值：**
- `size_t`: 字符串中的字符数

## 使用示例

### 示例 1：检查中文字符

```c
#include "src/core/lexer/utils/CN_lexer_utf8.h"

void check_chinese_char(const char* str) {
    Stru_UTF8CharInfo_t info = F_get_utf8_char_info(str, 4);
    if (info.is_valid) {
        if (info.is_chinese) {
            printf("中文字符: U+%04X\n", info.code_point);
        } else {
            printf("非中文字符: U+%04X\n", info.code_point);
        }
    } else {
        printf("无效的 UTF-8 编码\n");
    }
}
```

### 示例 2：验证标识符

```c
#include "src/core/lexer/utils/CN_lexer_utf8.h"

bool is_valid_identifier(const char* str, size_t length) {
    if (length == 0) return false;
    
    // 检查第一个字符
    if (!F_is_identifier_start_utf8(str, length)) {
        return false;
    }
    
    size_t pos = 0;
    while (pos < length) {
        // 获取当前字符信息
        Stru_UTF8CharInfo_t info = F_get_utf8_char_info(str + pos, length - pos);
        if (!info.is_valid) {
            return false;  // 无效的 UTF-8 编码
        }
        
        // 检查是否为标识符继续字符
        if (!F_is_identifier_continue_utf8(str + pos, length - pos)) {
            return false;
        }
        
        pos += info.byte_length;
    }
    
    return true;
}
```

### 示例 3：处理 UTF-8 字符串

```c
#include "src/core/lexer/utils/CN_lexer_utf8.h"

void process_utf8_string(const char* str, size_t length) {
    printf("字符串长度: %zu 字节\n", length);
    printf("字符数: %zu\n", F_count_utf8_chars(str, length));
    
    size_t pos = 0;
    size_t char_count = 0;
    
    while (pos < length) {
        Stru_UTF8CharInfo_t info = F_get_utf8_char_info(str + pos, length - pos);
        if (!info.is_valid) {
            printf("位置 %zu: 无效的 UTF-8 编码\n", pos);
            break;
        }
        
        char_count++;
        printf("字符 %zu: U+%04X, 字节长度: %zu", 
               char_count, info.code_point, info.byte_length);
        
        if (info.is_chinese) {
            printf(" (中文)");
        }
        printf("\n");
        
        pos += info.byte_length;
    }
}
```

## 错误处理

### 无效 UTF-8 编码处理

当遇到无效的 UTF-8 编码时，模块会：
1. 返回 `is_valid = false` 的字符信息
2. 在词法分析器中报告错误
3. 尝试跳过无效字节继续处理

### 边界情况处理

1. **长度不足**：当剩余长度不足以解码完整字符时，返回无效字符信息
2. **代理对**：拒绝 U+D800 - U+DFFF 范围内的码点（UTF-16 代理对）
3. **超出范围**：拒绝大于 0x10FFFF 的码点（超出 Unicode 范围）

## 性能考虑

### 内存使用
- 字符信息结构体：20 字节
- 无动态内存分配
- 栈上操作

### 时间复杂度
- 字符解码：O(1)
- 字符分类：O(1)
- 字符串遍历：O(n)，其中 n 是字符数

### 优化策略
1. **快速路径**：对 ASCII 字符使用快速检查
2. **缓存友好**：小结构体，适合 CPU 缓存
3. **分支预测**：优化条件判断

## 测试覆盖率

### 单元测试
- 有效 UTF-8 编码测试
- 无效 UTF-8 编码测试
- 中文字符识别测试
- 标识符字符检查测试
- 边界条件测试

### 集成测试
- 与词法分析器集成测试
- 大文件处理测试
- 性能基准测试

## 版本历史

### v1.0.0 (2026-01-09)
- 初始版本发布
- 完整的 UTF-8 解码支持
- 中文字符识别
- 标识符字符检查
- 通过所有单元测试

## 相关模块

- [词法分析器缓冲区模块](../scanner/buffer_management.md)
- [词法分析器扫描器模块](../scanner/character_scanner.md)
- [令牌扫描器模块](../token_scanners.md)

## 维护者

- CN_Language 架构委员会
- 联系方式：通过项目 Issue 跟踪系统

## 许可证

MIT License - 详见项目根目录 LICENSE 文件
