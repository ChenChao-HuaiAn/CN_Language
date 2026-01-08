# CN_Language 词法分析器令牌扫描器模块 API 文档

## 概述

令牌扫描器模块是词法分析器的核心组件，负责识别和提取各种类型的令牌，包括标识符、数字字面量、字符串字面量等。它将字符流转换为有意义的令牌序列，为语法分析提供输入。

## 核心功能

### 支持的令牌类型
- **标识符**：变量名、函数名等
- **关键字**：70个中文关键字
- **数字字面量**：整数和浮点数
- **字符串字面量**：带引号的字符串
- **运算符**：算术、比较、赋值等运算符
- **分隔符**：括号、逗号、分号等

## API 参考

### 主扫描函数

#### F_scan_next_token

扫描源代码中的下一个令牌。

**函数签名：**
```c
Stru_Token_t* F_scan_next_token(Stru_LexerScannerState_t* state);
```

**参数：**
- `state`：扫描器状态指针

**返回值：**
- `Stru_Token_t*`：新创建的令牌
- `NULL`：错误或已到达文件末尾

**扫描流程：**
1. 跳过空白字符和注释
2. 检查文件结束
3. 根据下一个字符选择适当的扫描器
4. 调用相应的扫描函数
5. 处理错误情况

**示例：**
```c
Stru_LexerScannerState_t* state = ...;
Stru_Token_t* token = F_scan_next_token(state);
if (token != NULL) {
    printf("令牌类型: %d, 词素: %s\n", token->type, token->lexeme);
    F_destroy_token(token);
}
```

### 特定令牌扫描函数

#### F_scan_identifier

扫描标识符或关键字。

**函数签名：**
```c
Stru_Token_t* F_scan_identifier(Stru_LexerScannerState_t* state);
```

**参数：**
- `state`：扫描器状态指针

**返回值：**
- `Stru_Token_t*`：标识符或关键字令牌
- `NULL`：扫描失败

**识别规则：**
- 以字母或下划线开头
- 后续字符可以是字母、数字或下划线
- 支持中文字符
- 如果是关键字，返回相应的关键字令牌类型

**示例：**
```c
// 源代码: "变量名"
Stru_Token_t* token = F_scan_identifier(state);
// token->type = Eum_TOKEN_IDENTIFIER
// token->lexeme = "变量名"
```

#### F_scan_number

扫描数字字面量（整数或浮点数）。

**函数签名：**
```c
Stru_Token_t* F_scan_number(Stru_LexerScannerState_t* state);
```

**参数：**
- `state`：扫描器状态指针

**返回值：**
- `Stru_Token_t*`：数字字面量令牌
- `NULL`：扫描失败

**支持格式：**
- 整数：`123`, `-456`, `+789`
- 浮点数：`3.14`, `-2.5`, `+0.001`
- 科学计数法：`1.23e4`, `-5.6e-7`

**示例：**
```c
// 源代码: "3.14"
Stru_Token_t* token = F_scan_number(state);
// token->type = Eum_TOKEN_FLOAT_LITERAL
// token->lexeme = "3.14"
// token->float_value = 3.14
```

#### F_scan_string

扫描字符串字面量。

**函数签名：**
```c
Stru_Token_t* F_scan_string(Stru_LexerScannerState_t* state);
```

**参数：**
- `state`：扫描器状态指针

**返回值：**
- `Stru_Token_t*`：字符串字面量令牌
- `NULL`：扫描失败

**字符串格式：**
- 以双引号（`"`）包围
- 支持转义字符：`\n`, `\t`, `\"`, `\\`
- 支持中文字符
- 跨行字符串（支持续行符）

**示例：**
```c
// 源代码: "\"Hello, 世界!\""
Stru_Token_t* token = F_scan_string(state);
// token->type = Eum_TOKEN_STRING_LITERAL
// token->lexeme = "Hello, 世界!"
```

### 字符分类函数

#### F_is_alpha

检查字符是否为字母（包括中文字符）。

**函数签名：**
```c
bool F_is_alpha(char c);
```

**参数：**
- `c`：要检查的字符

**返回值：**
- `true`：字符是字母
- `false`：字符不是字母

**支持范围：**
- ASCII字母：`a-z`, `A-Z`
- 下划线：`_`
- 中文字符：UTF-8编码的中文字符

#### F_is_digit

检查字符是否为数字。

**函数签名：**
```c
bool F_is_digit(char c);
```

**参数：**
- `c`：要检查的字符

**返回值：**
- `true`：字符是数字
- `false`：字符不是数字

**支持范围：**
- ASCII数字：`0-9`

#### F_is_alpha_numeric

检查字符是否为字母或数字。

**函数签名：**
```c
bool F_is_alpha_numeric(char c);
```

**参数：**
- `c`：要检查的字符

**返回值：**
- `true`：字符是字母或数字
- `false`：字符不是字母或数字

## 使用示例

### 基本令牌扫描

```c
#include "src/core/lexer/token_scanners/CN_lexer_token_scanners.h"
#include "src/core/lexer/scanner/CN_lexer_scanner.h"

int main(void) {
    // 创建和初始化扫描器状态
    Stru_LexerScannerState_t* state = F_create_scanner_state();
    const char* source = "变量 x = 42 + 3.14";
    F_initialize_scanner_state(state, source, strlen(source), "example.cn");
    
    // 扫描所有令牌
    while (state->current_pos < state->source_length) {
        Stru_Token_t* token = F_scan_next_token(state);
        
        if (token == NULL) {
            if (F_scanner_has_errors(state)) {
                fprintf(stderr, "扫描错误: %s\n", F_get_scanner_last_error(state));
            }
            break;
        }
        
        // 处理令牌
        printf("令牌: 类型=%d, 词素='%s', 位置=%zu:%zu\n",
               token->type, token->lexeme,
               token->line, token->column);
        
        F_destroy_token(token);
    }
    
    F_destroy_scanner_state(state);
    return 0;
}
```

### 特定令牌类型扫描

```c
#include "src/core/lexer/token_scanners/CN_lexer_token_scanners.h"

void scan_specific_tokens(Stru_LexerScannerState_t* state) {
    // 保存当前位置
    size_t saved_pos = state->current_pos;
    
    // 查看下一个字符
    char next_char = F_peek_char(state);
    
    if (F_is_alpha(next_char)) {
        // 扫描标识符或关键字
        Stru_Token_t* token = F_scan_identifier(state);
        if (token != NULL) {
            printf("标识符/关键字: %s\n", token->lexeme);
            F_destroy_token(token);
        }
    } else if (F_is_digit(next_char) || next_char == '-' || next_char == '+') {
        // 扫描数字
        Stru_Token_t* token = F_scan_number(state);
        if (token != NULL) {
            printf("数字: %s\n", token->lexeme);
            F_destroy_token(token);
        }
    } else if (next_char == '"') {
        // 扫描字符串
        Stru_Token_t* token = F_scan_string(state);
        if (token != NULL) {
            printf("字符串: %s\n", token->lexeme);
            F_destroy_token(token);
        }
    } else {
        // 恢复位置，让主扫描函数处理
        state->current_pos = saved_pos;
    }
}
```

### 错误处理示例

```c
#include "src/core/lexer/token_scanners/CN_lexer_token_scanners.h"

Stru_Token_t* safe_scan_next_token(Stru_LexerScannerState_t* state) {
    // 保存错误状态
    bool had_error = F_scanner_has_errors(state);
    char saved_error[256] = {0};
    if (had_error) {
        strncpy(saved_error, F_get_scanner_last_error(state), sizeof(saved_error) - 1);
    }
    
    // 尝试扫描令牌
    Stru_Token_t* token = F_scan_next_token(state);
    
    if (token == NULL) {
        // 检查是否有新错误
        if (F_scanner_has_errors(state) && !had_error) {
            // 创建错误令牌
            token = F_create_token(Eum_TOKEN_ERROR, 
                                  F_get_scanner_last_error(state),
                                  state->current_line, 
                                  state->current_column);
        }
    }
    
    return token;
}
```

## 性能考虑

### 内存使用
- 每个令牌：约64-128字节
- 临时缓冲区：按需分配
- 字符串处理：复制字符串内容

### 时间复杂性
- 标识符扫描：O(n)，其中n为标识符长度
- 数字扫描：O(n)，其中n为数字长度
- 字符串扫描：O(n)，其中n为字符串长度
- 字符分类：O(1)

### 优化建议
1. **缓冲区重用**：重用令牌对象减少分配开销
2. **提前退出**：在确定令牌类型后尽早退出扫描
3. **批量处理**：对于连续的同类型令牌，考虑批量处理

## 错误处理

### 常见错误类型
1. **无效标识符**：以数字开头的标识符
2. **无效数字格式**：多个小数点，无效的科学计数法
3. **未终止的字符串**：缺少结束引号
4. **无效转义序列**：未知的转义字符

### 错误恢复策略
- 创建错误令牌包含错误信息
- 跳过无效字符继续扫描
- 提供详细的位置信息

### 错误令牌示例
```c
// 源代码: "123abc" (无效标识符)
Stru_Token_t* token = F_scan_identifier(state);
// token->type = Eum_TOKEN_ERROR
// token->lexeme = "标识符不能以数字开头: 123abc"
```

## 扩展指南

### 添加新令牌类型
要添加新的令牌类型扫描器：

1. 在`CN_token_types.h`中添加枚举值
2. 添加新的扫描函数
3. 在主扫描函数中添加字符分类逻辑
4. 更新相关测试用例

### 自定义扫描算法
可以创建自定义的扫描函数：

```c
// 自定义标识符扫描器
Stru_Token_t* custom_scan_identifier(Stru_LexerScannerState_t* state) {
    // 保存起始位置
    size_t start_pos = state->current_pos;
    size_t start_line = state->current_line;
    size_t start_column = state->current_column;
    
    // 自定义扫描逻辑...
    
    // 提取词素
    size_t length = state->current_pos - start_pos;
    char* lexeme = malloc(length + 1);
    strncpy(lexeme, state->source + start_pos, length);
    lexeme[length] = '\0';
    
    // 创建令牌
    Stru_Token_t* token = F_create_token(Eum_TOKEN_IDENTIFIER,
                                        lexeme,
                                        start_line,
                                        start_column);
    free(lexeme);
    return token;
}
```

## 相关文档

- [扫描器模块 API 文档](../scanner/CN_lexer_scanner.md)
- [关键字模块 API 文档](../keywords/CN_lexer_keywords.md)
- [运算符模块 API 文档](../operators/CN_lexer_operators.md)
- [令牌模块 API 文档](../../token/CN_token.md)

## 修订历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-08 | 初始版本发布 |
| 1.0.1 | 2026-01-08 | 修复字符串扫描错误 |

## 版权声明

版权所有 © 2026 CN_Language项目团队。保留所有权利。

本文档是CN_Language项目的一部分，遵循项目许可证条款。
