# 字符串工具模块 (String Utilities Module)

## 概述

字符串工具模块提供了字符串的高级工具功能，包括UTF-8处理、字符分类、格式化、解析和迭代器。这些功能为字符串处理提供了强大的工具集。

## 特性

- **完整UTF-8支持**：正确处理所有UTF-8编码的字符
- **Unicode字符分类**：支持完整的Unicode字符分类
- **灵活格式化**：支持可变参数格式化
- **强大解析**：支持数字、字符串等解析
- **完整迭代器**：支持字符级和字节级遍历
- **内存安全**：自动处理内存分配和边界检查

## API 接口

### UTF-8处理

#### `F_string_utf8_char_at`
```c
uint32_t F_string_utf8_char_at(Stru_String_t* str, size_t char_index);
```
获取指定位置的UTF-8字符。获取字符串中指定字符位置的Unicode码点。

**参数**：
- `str`：字符串指针
- `char_index`：字符位置（从0开始）

**返回值**：
- 成功：Unicode码点
- 失败：0（如果str为NULL，或char_index超出范围）

#### `F_string_utf8_next_char`
```c
uint32_t F_string_utf8_next_char(const char* str, size_t* pos);
```
获取下一个UTF-8字符。从指定字节位置获取下一个UTF-8字符。

**参数**：
- `str`：C字符串指针
- `pos`：输入/输出参数，字节位置指针

**返回值**：
- 成功：Unicode码点，同时更新pos到下一个字符开始位置
- 失败：0（如果str为NULL，pos为NULL，或到达字符串末尾）

#### `F_string_utf8_prev_char`
```c
uint32_t F_string_utf8_prev_char(const char* str, size_t* pos);
```
获取上一个UTF-8字符。从指定字节位置获取上一个UTF-8字符。

**参数**：
- `str`：C字符串指针
- `pos`：输入/输出参数，字节位置指针

**返回值**：
- 成功：Unicode码点，同时更新pos到上一个字符开始位置
- 失败：0（如果str为NULL，pos为NULL，或到达字符串开头）

#### `F_string_utf8_strlen`
```c
size_t F_string_utf8_strlen(const char* str);
```
计算UTF-8字符串长度。计算UTF-8字符串中的字符数（不是字节数）。

**参数**：
- `str`：C字符串指针

**返回值**：
- 成功：字符数
- 失败：0（如果str为NULL）

### 字符分类

#### `F_string_is_alpha`
```c
bool F_string_is_alpha(uint32_t codepoint);
```
检查是否为字母字符。检查Unicode码点是否为字母字符。

**参数**：
- `codepoint`：Unicode码点

**返回值**：
- 是字母字符：true
- 不是字母字符：false

#### `F_string_is_digit`
```c
bool F_string_is_digit(uint32_t codepoint);
```
检查是否为数字字符。检查Unicode码点是否为数字字符。

**参数**：
- `codepoint`：Unicode码点

**返回值**：
- 是数字字符：true
- 不是数字字符：false

#### `F_string_is_alnum`
```c
bool F_string_is_alnum(uint32_t codepoint);
```
检查是否为字母数字字符。检查Unicode码点是否为字母或数字字符。

**参数**：
- `codepoint`：Unicode码点

**返回值**：
- 是字母数字字符：true
- 不是字母数字字符：false

#### `F_string_is_space`
```c
bool F_string_is_space(uint32_t codepoint);
```
检查是否为空白字符。检查Unicode码点是否为空白字符。

**参数**：
- `codepoint`：Unicode码点

**返回值**：
- 是空白字符：true
- 不是空白字符：false

#### `F_string_is_punctuation`
```c
bool F_string_is_punctuation(uint32_t codepoint);
```
检查是否为标点符号。检查Unicode码点是否为标点符号。

**参数**：
- `codepoint`：Unicode码点

**返回值**：
- 是标点符号：true
- 不是标点符号：false

### 格式化

#### `F_string_format`
```c
bool F_string_format(Stru_String_t* str, const char* format, ...);
```
格式化字符串。使用格式化字符串和参数格式化字符串。

**参数**：
- `str`：目标字符串指针
- `format`：格式化字符串
- `...`：可变参数列表

**返回值**：
- 成功：true
- 失败：false（如果str或format为NULL，或内存分配失败）

**支持格式**：
- `%d`, `%i`：有符号整数
- `%u`：无符号整数
- `%f`, `%g`, `%e`：浮点数
- `%c`：字符
- `%s`：字符串
- `%p`：指针
- `%x`, `%X`：十六进制整数

#### `F_string_append_format`
```c
bool F_string_append_format(Stru_String_t* str, const char* format, ...);
```
追加格式化字符串。将格式化结果追加到字符串末尾。

**参数**：
- `str`：目标字符串指针
- `format`：格式化字符串
- `...`：可变参数列表

**返回值**：
- 成功：true
- 失败：false（如果str或format为NULL，或内存分配失败）

#### `F_string_create_format`
```c
Stru_String_t* F_string_create_format(const char* format, ...);
```
创建格式化字符串。创建新的格式化字符串。

**参数**：
- `format`：格式化字符串
- `...`：可变参数列表

**返回值**：
- 成功：指向新创建的字符串的指针
- 失败：NULL（如果format为NULL，或内存分配失败）

### 解析

#### `F_string_to_int`
```c
bool F_string_to_int(Stru_String_t* str, int* out_value);
```
解析为整数。将字符串解析为整数。

**参数**：
- `str`：源字符串指针
- `out_value`：输出参数，解析的整数值

**返回值**：
- 成功：true
- 失败：false（如果str或out_value为NULL，或解析失败）

**注意**：支持十进制、十六进制（0x前缀）、八进制（0前缀）

#### `F_string_to_double`
```c
bool F_string_to_double(Stru_String_t* str, double* out_value);
```
解析为浮点数。将字符串解析为双精度浮点数。

**参数**：
- `str`：源字符串指针
- `out_value`：输出参数，解析的浮点数值

**返回值**：
- 成功：true
- 失败：false（如果str或out_value为NULL，或解析失败）

#### `F_string_split`
```c
bool F_string_split(Stru_String_t* str, Stru_String_t* delimiter, 
                    Stru_String_t*** out_tokens, size_t* out_count);
```
分割字符串。使用分隔符分割字符串。

**参数**：
- `str`：源字符串指针
- `delimiter`：分隔符字符串指针
- `out_tokens`：输出参数，指向令牌数组的指针
- `out_count`：输出参数，令牌数量

**返回值**：
- 成功：true
- 失败：false（如果str、delimiter、out_tokens或out_count为NULL，或内存分配失败）

**注意**：调用者负责释放out_tokens指向的数组和每个令牌字符串

### 迭代器

#### `F_create_string_iterator`
```c
Stru_StringIterator_t* F_create_string_iterator(Stru_String_t* string);
```
创建字符串迭代器。创建字符串的迭代器。

**参数**：
- `string`：要遍历的字符串指针

**返回值**：
- 成功：指向新创建的迭代器的指针
- 失败：NULL（如果string为NULL，或内存分配失败）

#### `F_create_string_iterator_with_mode`
```c
Stru_StringIterator_t* F_create_string_iterator_with_mode(Stru_String_t* string, 
                                                          Eum_StringIteratorMode mode);
```
创建带模式的字符串迭代器。创建指定模式的字符串迭代器。

**参数**：
- `string`：要遍历的字符串指针
- `mode`：迭代模式（字节模式或字符模式）

**返回值**：
- 成功：指向新创建的迭代器的指针
- 失败：NULL（如果string为NULL，或内存分配失败）

#### `F_destroy_string_iterator`
```c
void F_destroy_string_iterator(Stru_StringIterator_t* iterator);
```
销毁迭代器。销毁字符串迭代器。

**参数**：
- `iterator`：要销毁的迭代器指针

**注意**：如果iterator为NULL，函数不执行任何操作

#### `F_string_iterator_has_next`
```c
bool F_string_iterator_has_next(Stru_StringIterator_t* iterator);
```
检查是否有下一个元素。检查迭代器是否还有下一个元素。

**参数**：
- `iterator`：迭代器指针

**返回值**：
- 有下一个元素：true
- 没有下一个元素：false（如果iterator为NULL）

#### `F_string_iterator_next`
```c
bool F_string_iterator_next(Stru_StringIterator_t* iterator, uint32_t* out_codepoint);
```
获取下一个字符。获取迭代器的下一个字符。

**参数**：
- `iterator`：迭代器指针
- `out_codepoint`：输出参数，Unicode码点

**返回值**：
- 成功：true
- 失败：false（如果iterator或out_codepoint为NULL，或没有下一个元素）

#### `F_string_iterator_current_byte_pos`
```c
size_t F_string_iterator_current_byte_pos(Stru_StringIterator_t* iterator);
```
获取当前字节位置。获取迭代器的当前字节位置。

**参数**：
- `iterator`：迭代器指针

**返回值**：
- 当前字节位置（如果iterator为NULL返回0）

#### `F_string_iterator_current_char_pos`
```c
size_t F_string_iterator_current_char_pos(Stru_StringIterator_t* iterator);
```
获取当前字符位置。获取迭代器的当前字符位置。

**参数**：
- `iterator`：迭代器指针

**返回值**：
- 当前字符位置（如果iterator为NULL返回0）

## 使用示例

```c
#include "CN_string_utils.h"
#include <stdio.h>

int main() {
    // UTF-8处理示例
    Stru_String_t* str = F_create_string("Hello 世界！🌍");
    
    // 获取指定位置的字符
    uint32_t char_at = F_string_utf8_char_at(str, 6); // '世'
    printf("位置6的字符: U+%04X\n", char_at);
    
    // 计算UTF-8字符串长度
    size_t char_len = F_string_utf8_strlen(F_string_c_str(str));
    printf("字符数: %zu (字节数: %zu)\n", char_len, F_string_byte_length(str));
    
    // 字符分类示例
    printf("'H'是字母: %s\n", F_string_is_alpha('H') ? "是" : "否");
    printf("'5'是数字: %s\n", F_string_is_digit('5') ? "是" : "否");
    printf("' '是空白: %s\n", F_string_is_space(' ') ? "是" : "否");
    printf("'!'是标点: %s\n", F_string_is_punctuation('!') ? "是" : "否");
    
    // 格式化示例
    Stru_String_t* formatted = F_string_create_format("用户: %s, 年龄: %d, 分数: %.2f", 
                                                      "张三", 25, 95.5);
    if (formatted) {
        printf("格式化结果: %s\n", F_string_c_str(formatted));
        F_destroy_string(formatted);
    }
    
    // 解析示例
    Stru_String_t* num_str = F_create_string("12345");
    int int_value;
    if (F_string_to_int(num_str, &int_value)) {
        printf("解析的整数: %d\n", int_value);
    }
    
    Stru_String_t* float_str = F_create_string("3.14159");
    double double_value;
    if (F_string_to_double(float_str, &double_value)) {
        printf("解析的浮点数: %f\n", double_value);
    }
    
    // 分割字符串示例
    Stru_String_t* csv_str = F_create_string("apple,banana,cherry,date");
    Stru_String_t** tokens = NULL;
    size_t token_count = 0;
    
    if (F_string_split(csv_str, F_create_string(","), &tokens, &token_count)) {
        printf("分割结果 (%zu个令牌):\n", token_count);
        for (size_t i = 0; i < token_count; i++) {
            printf("  [%zu] %s\n", i, F_string_c_str(tokens[i]));
            F_destroy_string(tokens[i]);
        }
        free(tokens);
    }
    
    // 迭代器示例
    Stru_StringIterator_t* iter = F_create_string_iterator(str);
    printf("UTF-8字符遍历:\n");
    
    while (F_string_iterator_has_next(iter)) {
        uint32_t codepoint;
        F_string_iterator_next(iter, &codepoint);
        printf("U+%04X ", codepoint);
    }
    printf("\n");
    
    printf("迭代器位置 - 字节: %zu, 字符: %zu\n",
           F_string_iterator_current_byte_pos(iter),
           F_string_iterator_current_char_pos(iter));
    
    F_destroy_string_iterator(iter);
    
    // 复杂示例：处理CSV数据
    Stru_String_t* csv_data = F_create_string("John Doe,25,85.5,New York");
    Stru_String_t** fields = NULL;
    size_t field_count = 0;
    
    if (F_string_split(csv_data, F_create_string(","), &fields, &field_count)) {
        printf("\nCSV解析结果:\n");
        printf("姓名: %s\n", F_string_c_str(fields[0]));
        
        int age;
        if (F_string_to_int(fields[1], &age)) {
            printf("年龄: %d\n", age);
        }
        
        double score;
        if (F_string_to_double(fields[2], &score)) {
            printf("分数: %.1f\n", score);
        }
        
        printf("城市: %s\n", F_string_c_str(fields[3]));
        
        // 清理字段
        for (size_t i = 0; i < field_count; i++) {
            F_destroy_string(fields[i]);
        }
        free(fields);
    }
    
    // 清理
    F_destroy_string(str);
    F_destroy_string(num_str);
    F_destroy_string(float_str);
    F_destroy_string(csv_str);
    F_destroy_string(csv_data);
    
    return 0;
}
```

## 内部实现

### UTF-8解码算法

UTF-8处理使用标准的UTF-8解码算法：

#### 1. UTF-8编码结构
```c
// UTF-8编码结构
typedef struct {
    uint8_t first_byte_mask;   // 首字节掩码
    uint8_t first_byte_value;  // 首字节值
    uint8_t follow_byte_mask;  // 后续字节掩码
    uint8_t follow_byte_value; // 后续字节值
    size_t total_bytes;        // 总字节数
} Utf8Encoding;

// UTF-8编码规则
static const Utf8Encoding utf8_encodings[] = {
    {0x80, 0x00, 0xC0, 0x80, 1}, // 1字节: 0xxxxxxx
    {0xE0, 0xC0, 0xC0, 0x80, 2}, // 2字节: 110xxxxx 10xxxxxx
    {0xF0, 0xE0, 0xC0, 0x80, 3}, // 3字节: 1110xxxx 10xxxxxx 10xxxxxx
    {0xF8, 0xF0, 0xC0, 0x80, 4}, // 4字节: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
};
```

#### 2. 解码实现
```c
// UTF-8解码核心函数
uint32_t decode_utf8(const char* str, size_t* pos) {
    uint8_t first_byte = (uint8_t)str[*pos];
    
    // 确定编码长度
    size_t bytes = utf8_bytes_from_first_byte(first_byte);
    if (bytes == 0 || bytes > 4) return 0; // 无效编码
    
    // 解码码点
    uint32_t codepoint = first_byte & utf8_first_byte_mask[bytes];
    
    for (size_t i = 1; i < bytes; i++) {
        uint8_t byte = (uint8_t)str[*pos + i];
        if ((byte & 0xC0) != 0x80) return 0; // 无效后续字节
        
        codepoint = (codepoint << 6) | (byte & 0x3F);
    }
    
    *pos += bytes;
    return codepoint;
}
```

### 字符分类实现

字符分类使用Unicode字符数据库：

#### 1. Unicode字符类别
```c
// Unicode通用类别
typedef enum {
    CATEGORY_LU, // 大写字母
    CATEGORY_LL, // 小写字母
    CATEGORY_LT, // 标题字母
    CATEGORY_LO, // 其他字母
    CATEGORY_LM, // 修饰字母
    CATEGORY_MN, // 非间距标记
    CATEGORY_MC, // 间距组合标记
    CATEGORY_ME, // 封闭标记
    CATEGORY_ND, // 十进制数字
    CATEGORY_NL, // 字母数字
    CATEGORY_NO, // 其他数字
    CATEGORY_PC, // 连接标点
    CATEGORY_PD, // 破折号标点
    CATEGORY_PS, // 开始标点
    CATEGORY_PE, // 结束标点
    CATEGORY_PI, // 初始标点
    CATEGORY_PF, // 最终标点
    CATEGORY_PO, // 其他标点
    CATEGORY_SM, // 数学符号
    CATEGORY_SC, // 货币符号
    CATEGORY_SK, // 修饰符号
    CATEGORY_SO, // 其他符号
    CATEGORY_ZS, // 空格分隔符
    CATEGORY_ZL, // 行分隔符
    CATEGORY_ZP, // 段分隔符
    CATEGORY_CC, // 控制字符
    CATEGORY_CF, // 格式字符
    CATEGORY_CS, // 代理字符
    CATEGORY_CO, // 私人使用字符
    CATEGORY_CN  // 未分配字符
} UnicodeCategory;

// 获取字符类别
UnicodeCategory get_unicode_category(uint32_t codepoint);
```

#### 2. 高效分类实现
使用多级查找表加速字符分类：
```c
// 第一级：块索引表（256个块，每块256个码点）
static const uint8_t block_index[65536];

// 第二级：类别表（每个块256个类别）
static const UnicodeCategory category_table[256][256];

// 快速分类函数
UnicodeCategory classify_codepoint_fast(uint32_t codepoint) {
    if (codepoint < 0x10000) {
        uint16_t block = block_index[codepoint >> 8];
        return category_table[block][codepoint & 0xFF];
    }
    return classify_codepoint_slow(codepoint); // 回退到慢速路径
}
```

### 格式化实现

格式化使用可变参数处理：

#### 1. 可变参数处理
```c
// 格式化核心函数
bool format_string_impl(Stru_String_t* str, const char* format, va_list args) {
    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'd':
                case 'i': {
                    int value = va_arg(args, int);
                    append_integer(str, value, 10);
                    break;
                }
                case 'f':
                case 'g':
                case 'e': {
                    double value = va_arg(args, double);
                    append_double(str, value, *format);
                    break;
                }
                case 's': {
                    char* value = va_arg(args, char*);
                    F_string_append_cstr(str, value);
                    break;
                }
                // 其他格式处理...
            }
        } else {
            F_string_append_char(str, *format);
        }
        format++;
    }
    return true;
}
```

#### 2. 数字格式化
```c
// 整数格式化
void append_integer(Stru_String_t* str, int value, int base) {
    char buffer[32];
    int length = snprintf(buffer, sizeof(buffer), 
                         (base == 16) ? "%x" : "%d", value);
    F_string_append_cstr(str, buffer);
}

// 浮点数格式化
void append_double(Stru_String_t* str, double value, char format) {
    char buffer[64];
    int length = snprintf(buffer, sizeof(buffer), 
                         (format == 'f') ? "%.6f" : 
                         (format == 'e') ? "%.6e" : "%.6g", value);
    F_string_append_cstr(str, buffer);
}
```

### 解析实现

解析使用状态机处理：

#### 1. 整数解析状态机
```c
// 整数解析状态
typedef enum {
    STATE_START,
    STATE_SIGN,
    STATE_ZERO,
    STATE_DECIMAL,
    STATE_HEX,
    STATE_OCTAL,
    STATE_DONE,
    STATE_ERROR
} ParseState;

// 整数解析核心
bool parse_integer(const char* str, int* out_value) {
    ParseState state = STATE_START;
    int sign = 1;
    int value = 0;
    
    while (*str && state != STATE_DONE && state != STATE_ERROR) {
        char c = *str;
        
        switch (state) {
            case STATE_START:
                if (c == '+') state = STATE_SIGN;
                else if (c == '-') { sign = -1; state = STATE_SIGN; }
                else if (c == '0') state = STATE_ZERO;
                else if (c >= '1' && c <= '9') { value = c - '0'; state = STATE_DECIMAL; }
                else state = STATE_ERROR;
                break;
                
            case STATE_ZERO:
                if (c == 'x' || c == 'X') state = STATE_HEX;
                else if (c >= '0' && c <= '7') { value = c - '0'; state = STATE_OCTAL; }
                else if (c == '\0') state = STATE_DONE;
                else state = STATE_ERROR;
                break;
                
            // 其他状态处理...
        }
        
        str++;
    }
    
    if (state == STATE_DONE || state == STATE_DECIMAL || 
        state == STATE_HEX || state == STATE_OCTAL) {
        *out_value = value * sign;
        return true;
    }
    
    return false;
}
```

#### 2. 浮点数解析
使用标准库的strtod函数，但添加错误检查：
```c
bool parse_double(const char* str, double* out_value) {
    char* endptr;
    errno = 0;
    *out_value = strtod(str, &endptr);
    
    // 检查错误
    if (errno == ERANGE) {
        return false; // 溢出
    }
    if (endptr == str) {
        return false; // 没有数字
    }
    if (*endptr != '\0') {
        return false; // 额外字符
    }
    
    return true;
}
```

### 迭代器实现

迭代器提供灵活的遍历方式：

#### 1. 迭代器结构
```c
typedef struct Stru_StringIterator_t {
    Stru_String_t* string;          /**< 要遍历的字符串 */
    size_t current_byte_pos;        /**< 当前字节位置 */
    size_t current_char_pos;        /**< 当前字符位置 */
    Eum_StringIteratorMode mode;    /**< 迭代模式 */
} Stru_StringIterator_t;
```

#### 2. 迭代器操作
```c
// 获取下一个元素
bool iterator_next(Stru_StringIterator_t* iterator, uint32_t* out_codepoint) {
    if (iterator->mode == Eum_STRING_ITERATOR_BYTE) {
        // 字节模式
        if (iterator->current_byte_pos >= iterator->string->byte_length) {
            return false;
        }
        *out_codepoint = (uint8_t)iterator->string->data[iterator->current_byte_pos];
        iterator->current_byte_pos++;
        if (iterator->mode == Eum_STRING_ITERATOR_CHAR) {
            iterator->current_char_pos++;
        }
    } else {
        // 字符模式（UTF-8）
        if (iterator->current_byte_pos >= iterator->string->byte_length) {
            return false;
        }
        *out_codepoint = F_string_utf8_next_char(iterator->string->data, 
                                                &iterator->current_byte_pos);
        iterator->current_char_pos++;
    }
    return true;
}
```

### 错误处理

所有函数都包含完整的错误检查：
1. **参数验证**：检查NULL指针和无效参数
2. **边界检查**：检查索引范围、缓冲区大小等边界条件
3. **内存分配检查**：检查malloc/realloc是否成功
4. **编码验证**：检查UTF-8编码的有效性
5. **格式验证**：检查格式化字符串的有效性

## 性能特征

- **时间复杂度**：
  - UTF-8处理：O(n)，其中n是字符串长度
  - 字符分类：O(1)（使用查找表）
  - 格式化：O(m + n)，其中m是格式化字符串长度，n是结果长度
  - 解析：O(n)，其中n是字符串长度
  - 迭代器遍历：O(n)，其中n是字符串长度

- **空间复杂度**：
  - UTF-8处理：O(1)额外空间
  - 字符分类：O(1)额外空间（使用预计算表）
  - 格式化：O(k)额外空间，其中k是临时缓冲区大小
  - 解析：O(1)额外空间
  - 迭代器：O(1)额外空间

## 优化策略

### 1. 查找表优化
使用多级查找表加速Unicode操作：
```c
// 三级查找表结构
static const uint8_t level1_table[256];        // 第一级：高字节
static const uint16_t level2_table[256][256];  // 第二级：中字节
static const uint8_t level3_table[65536][256]; // 第三级：低字节

// 快速查找
uint8_t fast_unicode_lookup(uint32_t codepoint) {
    uint8_t b1 = (codepoint >> 16) & 0xFF;
    uint8_t b2 = (codepoint >> 8) & 0xFF;
    uint8_t b3 = codepoint & 0xFF;
    
    uint16_t level2_index = level1_table[b1];
    uint16_t level3_index = level2_table[level2_index][b2];
    return level3_table[level3_index][b3];
}
```

### 2. 内存池
对于频繁的临时字符串操作，使用内存池：
```c
// 字符串内存池
typedef struct {
    char** buffers;
    size_t* sizes;
    size_t count;
    size_t capacity;
} StringPool;

// 从池中获取缓冲区
char* pool_get_buffer(StringPool* pool, size_t required_size) {
    for (size_t i = 0; i < pool->count; i++) {
        if (pool->sizes[i] >= required_size) {
            return pool->buffers[i];
        }
    }
    
    // 分配新缓冲区
    if (pool->count >= pool->capacity) {
        pool->capacity = pool->capacity ? pool->capacity * 2 : 16;
        pool->buffers = realloc(pool->buffers, pool->capacity * sizeof(char*));
        pool->sizes = realloc(pool->sizes, pool->capacity * sizeof(size_t));
    }
    
    pool->buffers[pool->count] = malloc(required_size);
    pool->sizes[pool->count] = required_size;
    return pool->buffers[pool->count++];
}
```

### 3. 缓存计算结果
对于昂贵的计算，缓存结果：
```c
// 字符串哈希缓存
typedef struct {
    Stru_String_t* string;
    uint32_t hash;
    size_t timestamp;
} HashCache;

// 带缓存的哈希计算
uint32_t cached_string_hash(Stru_String_t* str, HashCache* cache) {
    if (cache->string == str && cache->timestamp == str->timestamp) {
        return cache->hash; // 返回缓存值
    }
    
    // 计算新哈希
    uint32_t hash = compute_string_hash(str);
    
    // 更新缓存
    cache->string = str;
    cache->hash = hash;
    cache->timestamp = str->timestamp;
    
    return hash;
}
```

## 依赖关系

- C标准库：`stdlib.h`, `string.h`, `stddef.h`, `stdbool.h`, `stdint.h`, `stdarg.h`, `errno.h`, `ctype.h`
- 项目依赖：`CN_string_core.h`, `CN_string_operations.h`

## 测试

工具模块包含完整的单元测试，覆盖所有API接口和边界条件。测试文件位于`tests/infrastructure/containers/string/test_string_utils.c`。

测试覆盖率目标：
- 语句覆盖率：≥95%
- 分支覆盖率：≥85%
- 函数覆盖率：100%

## 相关文档

- [字符串模块主文档](../README.md)
- [字符串核心模块文档](../string_core/README.md)
- [字符串操作模块文档](../string_operations/README.md)
- [字符串搜索模块文档](../string_search/README.md)
- [字符串转换模块文档](../string_transform/README.md)

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现基本工具功能 |
| 2.0.0 | 2026-01-07 | 模块化重构，作为工具模块 |

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language架构委员会
