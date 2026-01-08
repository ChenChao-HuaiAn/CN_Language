# CN_Language 词法分析器扫描器模块 API 文档

## 概述

扫描器模块是词法分析器的基础组件，负责源代码的字符级操作，包括字符读取、位置跟踪、空白字符跳过和错误状态管理。它提供低级的字符操作功能，为令牌扫描提供底层支持。

## 核心数据结构

### Stru_LexerScannerState_t

扫描器状态结构体，包含所有扫描相关的状态信息。

```c
typedef struct Stru_LexerScannerState_t {
    const char* source;          ///< 源代码字符串
    size_t source_length;        ///< 源代码长度（字节数）
    const char* source_name;     ///< 源文件名（可选）
    size_t current_pos;          ///< 当前位置（字节偏移）
    size_t current_line;         ///< 当前行号（从1开始）
    size_t current_column;       ///< 当前列号（从1开始）
    bool has_error;              ///< 是否有错误
    char error_message[256];     ///< 错误信息缓冲区
} Stru_LexerScannerState_t;
```

## API 参考

### 状态管理函数

#### F_create_scanner_state

创建新的扫描器状态实例。

**函数签名：**
```c
Stru_LexerScannerState_t* F_create_scanner_state(void);
```

**返回值：**
- `Stru_LexerScannerState_t*`：新创建的扫描器状态实例
- `NULL`：创建失败（内存不足）

**示例：**
```c
Stru_LexerScannerState_t* state = F_create_scanner_state();
if (state == NULL) {
    fprintf(stderr, "无法创建扫描器状态\n");
    return 1;
}
```

#### F_destroy_scanner_state

销毁扫描器状态实例，释放所有资源。

**函数签名：**
```c
void F_destroy_scanner_state(Stru_LexerScannerState_t* state);
```

**参数：**
- `state`：要销毁的扫描器状态指针

**注意事项：**
- 调用此函数后，状态指针不再有效
- 此函数会释放所有内部资源

#### F_initialize_scanner_state

初始化扫描器状态，设置源代码和源文件名。

**函数签名：**
```c
void F_initialize_scanner_state(Stru_LexerScannerState_t* state,
                               const char* source,
                               size_t source_length,
                               const char* source_name);
```

**参数：**
- `state`：扫描器状态指针
- `source`：源代码字符串（UTF-8编码）
- `source_length`：源代码长度（字节数）
- `source_name`：源文件名（可选，可为NULL）

**示例：**
```c
const char* source = "变量 x = 42";
F_initialize_scanner_state(state, source, strlen(source), "test.cn");
```

#### F_reset_scanner_state

重置扫描器状态到初始状态，可以重新扫描相同的源代码。

**函数签名：**
```c
void F_reset_scanner_state(Stru_LexerScannerState_t* state);
```

**参数：**
- `state`：扫描器状态指针

**效果：**
- 将当前位置重置为源代码开头
- 清除所有错误状态
- 行号和列号重置为1:1

### 字符操作函数

#### F_peek_char

查看下一个字符而不移动位置。

**函数签名：**
```c
char F_peek_char(const Stru_LexerScannerState_t* state);
```

**参数：**
- `state`：扫描器状态指针

**返回值：**
- `char`：下一个字符
- `'\0'`：已到达文件末尾

**示例：**
```c
char next_char = F_peek_char(state);
if (next_char != '\0') {
    printf("下一个字符是: %c\n", next_char);
}
```

#### F_next_char

获取下一个字符并更新位置。

**函数签名：**
```c
char F_next_char(Stru_LexerScannerState_t* state);
```

**参数：**
- `state`：扫描器状态指针

**返回值：**
- `char`：下一个字符
- `'\0'`：已到达文件末尾

**位置更新规则：**
- 普通字符：列号+1
- 换行符（`\n`）：行号+1，列号重置为1
- Windows换行符（`\r\n`）：正确处理为单个换行

**示例：**
```c
while (state->current_pos < state->source_length) {
    char c = F_next_char(state);
    printf("字符: %c (位置: %zu:%zu)\n", 
           c, state->current_line, state->current_column);
}
```

#### F_skip_whitespace

跳过空白字符（空格、制表符、换行符）。

**函数签名：**
```c
void F_skip_whitespace(Stru_LexerScannerState_t* state);
```

**参数：**
- `state`：扫描器状态指针

**跳过的字符：**
- 空格（`' '`）
- 制表符（`'\t'`）
- 换行符（`'\n'`）
- 回车符（`'\r'`）

**示例：**
```c
// 跳过所有空白字符
F_skip_whitespace(state);
```

#### F_skip_comment

跳过单行注释（以#开头）。

**函数签名：**
```c
void F_skip_comment(Stru_LexerScannerState_t* state);
```

**参数：**
- `state`：扫描器状态指针

**注释格式：**
- 以`#`字符开头
- 持续到行尾（`\n`或`\r\n`）
- 支持中文字符

**示例：**
```c
// 如果下一个字符是#，跳过注释
if (F_peek_char(state) == '#') {
    F_skip_comment(state);
}
```

### 位置管理函数

#### F_get_scanner_position

获取当前扫描位置（行号和列号）。

**函数签名：**
```c
void F_get_scanner_position(const Stru_LexerScannerState_t* state,
                           size_t* line,
                           size_t* column);
```

**参数：**
- `state`：扫描器状态指针
- `line`：输出参数，行号（从1开始）
- `column`：输出参数，列号（从1开始）

**示例：**
```c
size_t line, column;
F_get_scanner_position(state, &line, &column);
printf("当前位置：第%zu行，第%zu列\n", line, column);
```

#### F_get_scanner_source_name

获取当前正在扫描的源文件名。

**函数签名：**
```c
const char* F_get_scanner_source_name(const Stru_LexerScannerState_t* state);
```

**参数：**
- `state`：扫描器状态指针

**返回值：**
- `const char*`：源文件名
- 如果未设置源文件名，返回"unknown.cn"

### 错误处理函数

#### F_set_scanner_error

设置扫描器错误信息和状态。

**函数签名：**
```c
void F_set_scanner_error(Stru_LexerScannerState_t* state,
                        const char* format, ...);
```

**参数：**
- `state`：扫描器状态指针
- `format`：错误信息格式字符串（类似printf）
- `...`：格式参数

**示例：**
```c
F_set_scanner_error(state, "无效字符: %c (0x%02x)", c, c);
```

#### F_clear_scanner_error

清除扫描器错误状态。

**函数签名：**
```c
void F_clear_scanner_error(Stru_LexerScannerState_t* state);
```

**参数：**
- `state`：扫描器状态指针

**效果：**
- 清除错误标志
- 清空错误信息缓冲区

#### F_scanner_has_errors

检查扫描过程中是否发生了错误。

**函数签名：**
```c
bool F_scanner_has_errors(const Stru_LexerScannerState_t* state);
```

**参数：**
- `state`：扫描器状态指针

**返回值：**
- `true`：有错误
- `false`：没有错误

#### F_get_scanner_last_error

获取最后一个错误的详细描述。

**函数签名：**
```c
const char* F_get_scanner_last_error(const Stru_LexerScannerState_t* state);
```

**参数：**
- `state`：扫描器状态指针

**返回值：**
- `const char*`：错误信息字符串
- 如果没有错误，返回空字符串""

## 使用示例

### 基本使用

```c
#include "src/core/lexer/scanner/CN_lexer_scanner.h"

int main(void) {
    // 创建扫描器状态
    Stru_LexerScannerState_t* state = F_create_scanner_state();
    
    // 初始化状态
    const char* source = "变量 x = 42 + 3.14";
    F_initialize_scanner_state(state, source, strlen(source), "example.cn");
    
    // 跳过空白字符
    F_skip_whitespace(state);
    
    // 逐个读取字符
    while (state->current_pos < state->source_length) {
        char c = F_next_char(state);
        
        // 处理字符...
        if (c == '#') {
            F_skip_comment(state);
        } else if (c == ' ' || c == '\t' || c == '\n') {
            F_skip_whitespace(state);
        } else {
            printf("字符: %c (位置: %zu:%zu)\n", 
                   c, state->current_line, state->current_column);
        }
    }
    
    // 检查错误
    if (F_scanner_has_errors(state)) {
        fprintf(stderr, "扫描错误: %s\n", F_get_scanner_last_error(state));
    }
    
    // 清理资源
    F_destroy_scanner_state(state);
    return 0;
}
```

### 错误处理示例

```c
#include "src/core/lexer/scanner/CN_lexer_scanner.h"

void process_source(const char* source) {
    Stru_LexerScannerState_t* state = F_create_scanner_state();
    F_initialize_scanner_state(state, source, strlen(source), "test.cn");
    
    // 尝试读取字符
    while (state->current_pos < state->source_length) {
        char c = F_next_char(state);
        
        // 检查无效字符
        if (c < 32 && c != '\n' && c != '\t' && c != '\r') {
            F_set_scanner_error(state, "控制字符不允许: 0x%02x", c);
            break;
        }
    }
    
    // 报告错误
    if (F_scanner_has_errors(state)) {
        size_t line, column;
        F_get_scanner_position(state, &line, &column);
        fprintf(stderr, "错误位置 %zu:%zu: %s\n", 
                line, column, F_get_scanner_last_error(state));
    }
    
    F_destroy_scanner_state(state);
}
```

## 性能考虑

### 内存使用
- 状态结构体：约300字节
- 错误信息缓冲区：256字节固定大小
- 无动态内存分配（除了状态创建）

### 时间复杂性
- 字符操作：O(1)
- 空白字符跳过：O(n)，其中n为空白字符数量
- 注释跳过：O(n)，其中n为注释长度

### 优化建议
1. **批量处理**：对于大量空白字符，使用`F_skip_whitespace`比多次调用`F_next_char`更高效
2. **错误检查**：在循环外检查错误，减少函数调用开销
3. **位置缓存**：如果需要频繁获取位置，考虑缓存位置信息

## 线程安全性

### 线程安全级别
扫描器模块是**非线程安全**的。多个线程不能同时访问同一个`Stru_LexerScannerState_t`实例。

### 线程安全使用模式
```c
// 每个线程创建自己的扫描器状态
void* thread_function(void* arg) {
    Stru_LexerScannerState_t* state = F_create_scanner_state();
    // ... 使用扫描器状态 ...
    F_destroy_scanner_state(state);
    return NULL;
}

// 或者使用互斥锁保护共享状态
pthread_mutex_t scanner_mutex = PTHREAD_MUTEX_INITIALIZER;
Stru_LexerScannerState_t* shared_state = NULL;

void safe_scanner_operation(const char* source) {
    pthread_mutex_lock(&scanner_mutex);
    
    if (shared_state == NULL) {
        shared_state = F_create_scanner_state();
    }
    
    F_initialize_scanner_state(shared_state, source, strlen(source), NULL);
    // ... 其他操作 ...
    
    pthread_mutex_unlock(&scanner_mutex);
}
```

## 错误处理策略

### 错误类型
1. **输入错误**：无效的源代码
2. **状态错误**：扫描器状态不一致
3. **编码错误**：无效的UTF-8序列
4. **内存错误**：内存分配失败

### 错误恢复
- 设置错误标志
- 保存错误信息
- 允许继续扫描（跳过错误字符）
- 提供错误位置信息

### 错误报告格式
```
位置 <行号>:<列号>: <错误描述>
示例: 位置 3:15: 无效字符: # (0x23)
```

## 版本兼容性

### API 稳定性
扫描器模块遵循以下版本兼容性规则：
- **主版本号变更**：不兼容的API修改
- **次版本号变更**：向下兼容的功能性新增
- **修订号变更**：向下兼容的问题修正

### 向后兼容性保证
1. 现有函数签名不会改变
2. 结构体布局保持稳定
3. 错误代码含义不变

## 相关文档

- [词法分析器接口 API 文档](../CN_lexer_interface.md)
- [令牌扫描器模块 API 文档](../token_scanners/CN_lexer_token_scanners.md)
- [关键字模块 API 文档](../keywords/CN_lexer_keywords.md)
- [运算符模块 API 文档](../operators/CN_lexer_operators.md)
- [工具函数模块 API 文档](../utils/CN_lexer_utils.md)

## 修订历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-08 | 初始版本发布 |
| 1.0.1 | 2026-01-08 | 修复错误处理函数 |

## 版权声明

版权所有 © 2026 CN_Language项目团队。保留所有权利。

本文档是CN_Language项目的一部分，遵循项目许可证条款。
