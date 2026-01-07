# 字符串模块 (String Module) - 版本 2.0.0

## 概述

字符串模块提供了完整、模块化的字符串处理功能，支持UTF-8编码和多字节字符处理。字符串是CN_Language项目中的基础数据结构，用于源代码解析、错误消息、标识符处理等场景。

本模块采用分层架构设计，分为五个子模块：
1. **核心模块** (`string_core/`) - 字符串创建、销毁和基本属性
2. **操作模块** (`string_operations/`) - 字符串追加、比较、复制等基本操作
3. **搜索模块** (`string_search/`) - 子字符串查找、替换、模式匹配
4. **转换模块** (`string_transform/`) - 大小写转换、空白处理、编码转换
5. **工具模块** (`string_utils/`) - UTF-8处理、字符分类、格式化等高级功能

## 特性

- **UTF-8支持**：完整支持UTF-8编码，正确处理多字节字符
- **模块化设计**：遵循单一职责原则，功能清晰分离
- **内存安全**：自动内存管理，防止缓冲区溢出
- **高效操作**：优化常用操作性能
- **类型安全**：通过接口封装确保类型安全
- **完整迭代器**：支持字符级和字节级遍历
- **批量操作**：支持批量字符串处理
- **工具函数**：提供格式化、解析、验证等高级功能

## 模块结构

```
string/
├── CN_string.h              # 主头文件（包含所有子模块）
├── README.md               # 模块文档（本文件）
├── string_core/            # 核心模块
│   ├── CN_string_core.h    # 核心头文件
│   ├── CN_string_core.c    # 核心实现
│   └── README.md          # 核心模块文档
├── string_operations/      # 操作模块
│   ├── CN_string_operations.h # 操作头文件
│   ├── CN_string_operations.c # 操作实现
│   └── README.md          # 操作模块文档
├── string_search/          # 搜索模块
│   ├── CN_string_search.h  # 搜索头文件
│   ├── CN_string_search.c  # 搜索实现
│   └── README.md          # 搜索模块文档
├── string_transform/       # 转换模块
│   ├── CN_string_transform.h # 转换头文件
│   ├── CN_string_transform.c # 转换实现
│   └── README.md          # 转换模块文档
└── string_utils/           # 工具模块
    ├── CN_string_utils.h   # 工具头文件
    ├── CN_string_utils.c   # 工具实现
    └── README.md          # 工具模块文档
```

## 数据结构

### Stru_String_t (核心模块)

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

### Stru_StringIterator_t (工具模块)

字符串迭代器结构体：

```c
typedef struct Stru_StringIterator_t
{
    Stru_String_t* string;          /**< 要遍历的字符串 */
    size_t current_byte_pos;        /**< 当前字节位置 */
    size_t current_char_pos;        /**< 当前字符位置 */
    Eum_StringIteratorMode mode;    /**< 迭代模式（字节/字符） */
} Stru_StringIterator_t;
```

## API 接口

### 核心模块 API

#### 创建和销毁
- `F_create_string(const char* initial_data)` - 从C字符串创建字符串
- `F_create_string_with_capacity(size_t initial_capacity)` - 创建指定容量的空字符串
- `F_destroy_string(Stru_String_t* string)` - 销毁字符串

#### 字符串属性
- `F_string_length(Stru_String_t* string)` - 获取字符串长度（字符数）
- `F_string_byte_length(Stru_String_t* string)` - 获取字符串字节长度
- `F_string_capacity(Stru_String_t* string)` - 获取字符串容量
- `F_string_is_empty(Stru_String_t* string)` - 检查字符串是否为空
- `F_string_c_str(Stru_String_t* string)` - 获取C风格字符串指针

#### 内存管理
- `F_string_reserve(Stru_String_t* string, size_t new_capacity)` - 调整字符串容量
- `F_string_shrink_to_fit(Stru_String_t* string)` - 收缩字符串以适合当前内容
- `F_string_clear(Stru_String_t* string)` - 清空字符串内容

### 操作模块 API

#### 字符串操作
- `F_string_append(Stru_String_t* dest, Stru_String_t* src)` - 追加字符串
- `F_string_append_cstr(Stru_String_t* dest, const char* src)` - 追加C字符串
- `F_string_append_char(Stru_String_t* dest, uint32_t codepoint)` - 追加Unicode字符
- `F_string_copy(Stru_String_t* src)` - 复制字符串
- `F_string_assign(Stru_String_t* dest, Stru_String_t* src)` - 赋值字符串
- `F_string_assign_cstr(Stru_String_t* dest, const char* src)` - 赋值C字符串

#### 字符串比较
- `F_string_equals(Stru_String_t* str1, Stru_String_t* str2)` - 比较字符串是否相等
- `F_string_equals_cstr(Stru_String_t* str, const char* cstr)` - 与C字符串比较
- `F_string_compare(Stru_String_t* str1, Stru_String_t* str2)` - 比较字符串（类似strcmp）
- `F_string_compare_cstr(Stru_String_t* str, const char* cstr)` - 与C字符串比较

#### 子字符串操作
- `F_string_substring(Stru_String_t* str, size_t start, size_t length)` - 获取子字符串
- `F_string_slice(Stru_String_t* str, size_t start, size_t end)` - 获取字符串切片

### 搜索模块 API

#### 查找操作
- `F_string_find(Stru_String_t* str, Stru_String_t* substr)` - 查找子字符串
- `F_string_find_cstr(Stru_String_t* str, const char* substr)` - 查找C子字符串
- `F_string_find_char(Stru_String_t* str, uint32_t codepoint)` - 查找字符
- `F_string_rfind(Stru_String_t* str, Stru_String_t* substr)` - 从后向前查找子字符串
- `F_string_rfind_cstr(Stru_String_t* str, const char* substr)` - 从后向前查找C子字符串

#### 替换操作
- `F_string_replace(Stru_String_t* str, Stru_String_t* old_substr, Stru_String_t* new_substr)` - 替换子字符串
- `F_string_replace_cstr(Stru_String_t* str, const char* old_substr, const char* new_substr)` - 替换C子字符串
- `F_string_replace_all(Stru_String_t* str, Stru_String_t* old_substr, Stru_String_t* new_substr)` - 替换所有匹配的子字符串

#### 模式匹配
- `F_string_starts_with(Stru_String_t* str, Stru_String_t* prefix)` - 检查是否以指定前缀开头
- `F_string_starts_with_cstr(Stru_String_t* str, const char* prefix)` - 检查是否以C前缀开头
- `F_string_ends_with(Stru_String_t* str, Stru_String_t* suffix)` - 检查是否以指定后缀结尾
- `F_string_ends_with_cstr(Stru_String_t* str, const char* suffix)` - 检查是否以C后缀结尾
- `F_string_contains(Stru_String_t* str, Stru_String_t* substr)` - 检查是否包含子字符串

### 转换模块 API

#### 大小写转换
- `F_string_to_lower(Stru_String_t* str)` - 转换为小写
- `F_string_to_upper(Stru_String_t* str)` - 转换为大写
- `F_string_to_title(Stru_String_t* str)` - 转换为标题格式

#### 空白处理
- `F_string_trim(Stru_String_t* str)` - 去除两端空白
- `F_string_trim_left(Stru_String_t* str)` - 去除左侧空白
- `F_string_trim_right(Stru_String_t* str)` - 去除右侧空白
- `F_string_normalize_whitespace(Stru_String_t* str)` - 规范化空白字符

#### 编码转换
- `F_string_to_utf8(Stru_String_t* str)` - 确保UTF-8编码
- `F_string_from_utf16(const uint16_t* utf16_data, size_t length)` - 从UTF-16创建字符串
- `F_string_to_utf16(Stru_String_t* str, uint16_t** out_buffer, size_t* out_length)` - 转换为UTF-16

### 工具模块 API

#### UTF-8处理
- `F_string_utf8_char_at(Stru_String_t* str, size_t char_index)` - 获取指定位置的UTF-8字符
- `F_string_utf8_next_char(const char* str, size_t* pos)` - 获取下一个UTF-8字符
- `F_string_utf8_prev_char(const char* str, size_t* pos)` - 获取上一个UTF-8字符
- `F_string_utf8_strlen(const char* str)` - 计算UTF-8字符串长度

#### 字符分类
- `F_string_is_alpha(uint32_t codepoint)` - 检查是否为字母字符
- `F_string_is_digit(uint32_t codepoint)` - 检查是否为数字字符
- `F_string_is_alnum(uint32_t codepoint)` - 检查是否为字母数字字符
- `F_string_is_space(uint32_t codepoint)` - 检查是否为空白字符
- `F_string_is_punctuation(uint32_t codepoint)` - 检查是否为标点符号

#### 格式化
- `F_string_format(Stru_String_t* str, const char* format, ...)` - 格式化字符串
- `F_string_append_format(Stru_String_t* str, const char* format, ...)` - 追加格式化字符串
- `F_string_create_format(const char* format, ...)` - 创建格式化字符串

#### 解析
- `F_string_to_int(Stru_String_t* str, int* out_value)` - 解析为整数
- `F_string_to_double(Stru_String_t* str, double* out_value)` - 解析为浮点数
- `F_string_split(Stru_String_t* str, Stru_String_t* delimiter, Stru_String_t*** out_tokens, size_t* out_count)` - 分割字符串

#### 迭代器
- `F_create_string_iterator(Stru_String_t* string)` - 创建字符串迭代器
- `F_create_string_iterator_with_mode(Stru_String_t* string, Eum_StringIteratorMode mode)` - 创建带模式的字符串迭代器
- `F_destroy_string_iterator(Stru_StringIterator_t* iterator)` - 销毁迭代器
- `F_string_iterator_has_next(Stru_StringIterator_t* iterator)` - 检查是否有下一个元素
- `F_string_iterator_next(Stru_StringIterator_t* iterator, uint32_t* out_codepoint)` - 获取下一个字符
- `F_string_iterator_current_byte_pos(Stru_StringIterator_t* iterator)` - 获取当前字节位置
- `F_string_iterator_current_char_pos(Stru_StringIterator_t* iterator)` - 获取当前字符位置

### 模块管理 API
- `F_string_get_version(int* major, int* minor, int* patch)` - 获取模块版本
- `F_string_get_version_string(void)` - 获取版本字符串
- `F_string_module_init(void)` - 初始化模块
- `F_string_module_cleanup(void)` - 清理模块

## 使用示例

### 基本使用示例

```c
#include "CN_string.h"
#include <stdio.h>

int main() {
    // 初始化字符串模块
    F_string_module_init();
    
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
    
    // 追加字符串
    Stru_String_t* append_str = F_create_string(" 欢迎使用CN_Language！");
    F_string_append(str, append_str);
    printf("追加后: %s\n", F_string_c_str(str));
    
    // 比较字符串
    Stru_String_t* compare_str = F_create_string("你好，世界！ 欢迎使用CN_Language！");
    bool equal = F_string_equals(str, compare_str);
    printf("字符串相等: %s\n", equal ? "是" : "否");
    
    // 清理
    F_destroy_string(str);
    F_destroy_string(append_str);
    F_destroy_string(compare_str);
    F_string_module_cleanup();
    
    return 0;
}
```

### UTF-8处理示例

```c
#include "CN_string.h"
#include <stdio.h>

int main() {
    // UTF-8字符串示例（包含中英文混合）
    Stru_String_t* str = F_create_string("Hello 世界！🌍");
    
    printf("字符串: %s\n", F_string_c_str(str));
    printf("字符数: %zu\n", F_string_length(str));  // 注意：🌍是一个字符
    printf("字节数: %zu\n", F_string_byte_length(str));
    
    // 遍历UTF-8字符
    Stru_StringIterator_t* iter = F_create_string_iterator(str);
    printf("UTF-8字符遍历:\n");
    
    while (F_string_iterator_has_next(iter)) {
        uint32_t codepoint;
        F_string_iterator_next(iter, &codepoint);
        printf("U+%04X ", codepoint);
    }
    printf("\n");
    
    F_destroy_string_iterator(iter);
    F_destroy_string(str);
    
    return 0;
}
```

### 搜索和替换示例

```c
#include "CN_string.h"
#include <stdio.h>

int main() {
    Stru_String_t* str = F_create_string("这是一个测试字符串，测试字符串的功能。");
    
    // 查找子字符串
    size_t pos = F_string_find_cstr(str, "测试");
    printf("'测试'首次出现位置: %zu\n", pos);
    
    // 替换子字符串
    F_string_replace_cstr(str, "测试", "示例");
    printf("替换后: %s\n", F_string_c_str(str));
    
    // 检查前缀和后缀
    bool starts = F_string_starts_with_cstr(str, "这是一个");
    bool ends = F_string_ends_with_cstr(str, "功能。");
    printf("以'这是一个'开头: %s\n", starts ? "是" : "否");
    printf("以'功能。'结尾: %s\n", ends ? "是" : "否");
    
    F_destroy_string(str);
    
    return 0;
}
```

### 格式化示例

```c
#include "CN_string.h"
#include <stdio.h>

int main() {
    // 格式化字符串
    Stru_String_t* formatted = F_string_create_format("用户: %s, 年龄: %d, 分数: %.2f", 
                                                      "张三", 25, 95.5);
    if (formatted) {
        printf("格式化结果: %s\n", F_string_c_str(formatted));
        F_destroy_string(formatted);
    }
    
    // 解析字符串
    Stru_String_t* num_str = F_create_string("12345");
    int value;
    if (F_string_to_int(num_str, &value)) {
        printf("解析的整数: %d\n", value);
    }
    
    F_destroy_string(num_str);
    
    return 0;
}
```

## 典型应用场景

### 1. 源代码解析
```c
// 编译器词法分析器
// 处理UTF-8编码的源代码，提取标识符、关键字、字符串字面量
// 使用字符串模块进行高效的字符处理和模式匹配
```

### 2. 错误消息处理
```c
// 编译器错误报告
// 格式化错误消息，包含源代码位置和错误描述
// 使用字符串格式化功能生成用户友好的错误消息
```

### 3. 标识符处理
```c
// 变量名、函数名等标识符处理
// 验证标识符合法性（字母开头，包含字母数字下划线）
// 支持Unicode标识符（如中文变量名）
```

### 4. 字符串字面量处理
```c
// 处理源代码中的字符串字面量
//
