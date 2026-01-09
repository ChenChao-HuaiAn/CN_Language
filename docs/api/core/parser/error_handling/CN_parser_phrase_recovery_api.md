# CN_Language 语法分析器短语级恢复API文档

## 概述

短语级恢复模块是CN_Language语法分析器错误处理系统的重要组成部分，专门负责在语法分析过程中进行短语级别的错误恢复。该模块提供智能的错误恢复策略，包括插入缺失令牌、删除多余令牌、替换错误令牌等，帮助语法分析器从错误中恢复并继续分析。

## 核心数据结构

### 短语级恢复策略枚举 (`Eum_PhraseRecoveryStrategy`)

```c
typedef enum Eum_PhraseRecoveryStrategy {
    Eum_RECOVERY_INSERT_TOKEN,      ///< 插入缺失的令牌
    Eum_RECOVERY_DELETE_TOKEN,      ///< 删除多余的令牌
    Eum_RECOVERY_REPLACE_TOKEN,     ///< 替换错误的令牌
    Eum_RECOVERY_SKIP_TOKEN,        ///< 跳过当前令牌
    Eum_RECOVERY_REORDER_TOKENS,    ///< 重新排序令牌
    Eum_RECOVERY_NONE               ///< 不进行短语级恢复
} Eum_PhraseRecoveryStrategy;
```

### 短语级恢复结果结构体 (`Stru_PhraseRecoveryResult_t`)

```c
typedef struct Stru_PhraseRecoveryResult_t {
    bool success;                           ///< 恢复是否成功
    Eum_PhraseRecoveryStrategy strategy;    ///< 使用的恢复策略
    char* suggestion;                       ///< 恢复建议
    size_t tokens_affected;                 ///< 受影响的令牌数量
    Stru_Token_t* inserted_token;           ///< 插入的令牌（如果有）
    Stru_Token_t* deleted_token;            ///< 删除的令牌（如果有）
    Stru_Token_t* replaced_token;           ///< 替换的令牌（如果有）
} Stru_PhraseRecoveryResult_t;
```

## API函数

### 恢复结果管理函数

#### `F_create_phrase_recovery_result()`
创建并初始化一个新的短语级恢复结果对象。

**函数原型：**
```c
Stru_PhraseRecoveryResult_t* F_create_phrase_recovery_result(void);
```

**参数：**
- 无

**返回值：**
- 成功：返回新创建的恢复结果对象指针
- 失败：返回NULL

**示例：**
```c
Stru_PhraseRecoveryResult_t* result = F_create_phrase_recovery_result();
if (result == NULL) {
    // 处理内存分配失败
}
```

#### `F_destroy_phrase_recovery_result()`
释放短语级恢复结果对象占用的所有内存。

**函数原型：**
```c
void F_destroy_phrase_recovery_result(Stru_PhraseRecoveryResult_t* result);
```

**参数：**
- `result`：要销毁的恢复结果对象

**返回值：**
- 无

**示例：**
```c
Stru_PhraseRecoveryResult_t* result = F_create_phrase_recovery_result();
// 使用恢复结果...
F_destroy_phrase_recovery_result(result);
```

### 短语级恢复函数

#### `F_try_phrase_recovery()`
尝试使用短语级恢复策略修复语法错误。

**函数原型：**
```c
Stru_PhraseRecoveryResult_t* F_try_phrase_recovery(Stru_ParserInterface_t* interface,
                                                  Stru_SyntaxError_t* error);
```

**参数：**
- `interface`：语法分析器接口指针
- `error`：发生的语法错误

**返回值：**
- 成功：返回恢复结果对象指针
- 失败：返回NULL

**示例：**
```c
Stru_SyntaxError_t* error = F_report_unexpected_token_error(parser, token, "期望分号");
Stru_PhraseRecoveryResult_t* recovery = F_try_phrase_recovery(parser, error);
if (recovery != NULL && recovery->success) {
    // 应用恢复策略
    F_apply_recovery_strategy(parser, recovery);
}
```

#### `F_try_insert_missing_token()`
尝试插入缺失的令牌来修复语法错误。

**函数原型：**
```c
Stru_PhraseRecoveryResult_t* F_try_insert_missing_token(Stru_ParserInterface_t* interface,
                                                       Stru_SyntaxError_t* error);
```

**参数：**
- `interface`：语法分析器接口指针
- `error`：发生的语法错误

**返回值：**
- 成功：返回恢复结果对象指针
- 失败：返回NULL

**错误类型支持：**
- `Eum_ERROR_MISSING_TOKEN`
- `Eum_ERROR_UNCLOSED_PAREN`
- `Eum_ERROR_UNCLOSED_BRACKET`
- `Eum_ERROR_UNCLOSED_BRACE`

#### `F_try_delete_extra_token()`
尝试删除多余的令牌来修复语法错误。

**函数原型：**
```c
Stru_PhraseRecoveryResult_t* F_try_delete_extra_token(Stru_ParserInterface_t* interface,
                                                     Stru_SyntaxError_t* error);
```

**参数：**
- `interface`：语法分析器接口指针
- `error`：发生的语法错误

**返回值：**
- 成功：返回恢复结果对象指针
- 失败：返回NULL

**错误类型支持：**
- `Eum_ERROR_UNEXPECTED_TOKEN`

#### `F_try_replace_wrong_token()`
尝试替换错误的令牌来修复语法错误。

**函数原型：**
```c
Stru_PhraseRecoveryResult_t* F_try_replace_wrong_token(Stru_ParserInterface_t* interface,
                                                      Stru_SyntaxError_t* error);
```

**参数：**
- `interface`：语法分析器接口指针
- `error`：发生的语法错误

**返回值：**
- 成功：返回恢复结果对象指针
- 失败：返回NULL

**错误类型支持：**
- `Eum_ERROR_UNEXPECTED_TOKEN`
- `Eum_ERROR_MISMATCHED_TOKEN`

#### `F_try_skip_current_token()`
尝试跳过当前令牌来修复语法错误。

**函数原型：**
```c
Stru_PhraseRecoveryResult_t* F_try_skip_current_token(Stru_ParserInterface_t* interface,
                                                     Stru_SyntaxError_t* error);
```

**参数：**
- `interface`：语法分析器接口指针
- `error`：发生的语法错误

**返回值：**
- 成功：返回恢复结果对象指针
- 失败：返回NULL

**错误类型支持：**
- 所有其他错误类型

### 错误分析和建议函数

#### `F_analyze_error_context()`
分析错误发生的上下文，确定可能的恢复策略。

**函数原型：**
```c
int F_analyze_error_context(Stru_ParserInterface_t* interface,
                           Stru_SyntaxError_t* error,
                           char* context_buffer,
                           size_t buffer_size);
```

**参数：**
- `interface`：语法分析器接口指针
- `error`：发生的语法错误
- `context_buffer`：上下文缓冲区
- `buffer_size`：缓冲区大小

**返回值：**
- 成功：返回写入缓冲区的字符数
- 失败：返回0

**示例：**
```c
char context[256];
int len = F_analyze_error_context(parser, error, context, sizeof(context));
if (len > 0) {
    printf("错误上下文: %s\n", context);
}
```

#### `F_generate_fix_suggestion()`
根据错误类型和上下文生成修复建议。

**函数原型：**
```c
int F_generate_fix_suggestion(Stru_ParserInterface_t* interface,
                             Stru_SyntaxError_t* error,
                             char* suggestion_buffer,
                             size_t buffer_size);
```

**参数：**
- `interface`：语法分析器接口指针
- `error`：发生的语法错误
- `suggestion_buffer`：建议缓冲区
- `buffer_size`：缓冲区大小

**返回值：**
- 成功：返回写入缓冲区的字符数
- 失败：返回0

**示例：**
```c
char suggestion[256];
int len = F_generate_fix_suggestion(parser, error, suggestion, sizeof(suggestion));
if (len > 0) {
    printf("修复建议: %s\n", suggestion);
}
```

#### `F_check_common_error_pattern()`
检查错误是否匹配常见的错误模式。

**函数原型：**
```c
bool F_check_common_error_pattern(Stru_ParserInterface_t* interface,
                                 Stru_SyntaxError_t* error,
                                 char* pattern_buffer,
                                 size_t buffer_size);
```

**参数：**
- `interface`：语法分析器接口指针
- `error`：发生的语法错误
- `pattern_buffer`：模式缓冲区
- `buffer_size`：缓冲区大小

**返回值：**
- 匹配常见模式：返回true
- 不匹配：返回false

**支持的常见错误模式：**
1. 缺失分号错误模式
2. 括号不匹配错误模式
3. 意外的逗号错误模式
4. 意外的右括号错误模式
5. 意外的运算符错误模式

### 恢复策略应用函数

#### `F_apply_recovery_strategy()`
应用短语级恢复策略到语法分析器。

**函数原型：**
```c
bool F_apply_recovery_strategy(Stru_ParserInterface_t* interface,
                              Stru_PhraseRecoveryResult_t* result);
```

**参数：**
- `interface`：语法分析器接口指针
- `result`：恢复结果

**返回值：**
- 应用成功：返回true
- 应用失败：返回false

**支持的恢复策略：**
- `Eum_RECOVERY_INSERT_TOKEN`：插入令牌
- `Eum_RECOVERY_DELETE_TOKEN`：删除令牌
- `Eum_RECOVERY_REPLACE_TOKEN`：替换令牌
- `Eum_RECOVERY_SKIP_TOKEN`：跳过令牌
- `Eum_RECOVERY_REORDER_TOKENS`：重新排序令牌

## 使用示例

### 完整错误恢复流程

```c
#include "error_handling/CN_parser_phrase_recovery.h"

// 报告语法错误
Stru_SyntaxError_t* error = F_report_unexpected_token_error(parser, token, "期望分号");

// 尝试短语级恢复
Stru_PhraseRecoveryResult_t* recovery = F_try_phrase_recovery(parser, error);
if (recovery != NULL) {
    if (recovery->success) {
        // 分析错误上下文
        char context[256];
        F_analyze_error_context(parser, error, context, sizeof(context));
        
        // 生成修复建议
        char suggestion[256];
        F_generate_fix_suggestion(parser, error, suggestion, sizeof(suggestion));
        
        // 检查常见错误模式
        char pattern[256];
        if (F_check_common_error_pattern(parser, error, pattern, sizeof(pattern))) {
            printf("检测到常见错误模式: %s\n", pattern);
        }
        
        // 应用恢复策略
        if (F_apply_recovery_strategy(parser, recovery)) {
            printf("成功应用恢复策略: %s\n", recovery->suggestion);
        }
    }
    
    // 清理恢复结果
    F_destroy_phrase_recovery_result(recovery);
}

// 清理错误对象
if (error != NULL) {
    // 假设有错误销毁函数
    F_destroy_syntax_error(error);
}
```

### 特定错误类型的恢复

```c
// 处理缺失令牌错误
Stru_SyntaxError_t* missing_error = F_report_missing_token_error(parser, line, column, "分号");
Stru_PhraseRecoveryResult_t* insert_recovery = F_try_insert_missing_token(parser, missing_error);
if (insert_recovery != NULL && insert_recovery->success) {
    printf("建议插入令牌: %s\n", insert_recovery->suggestion);
    F_apply_recovery_strategy(parser, insert_recovery);
    F_destroy_phrase_recovery_result(insert_recovery);
}

// 处理意外令牌错误
Stru_SyntaxError_t* unexpected_error = F_report_unexpected_token_error(parser, token, "标识符");
Stru_PhraseRecoveryResult_t* delete_recovery = F_try_delete_extra_token(parser, unexpected_error);
if (delete_recovery != NULL && delete_recovery->success) {
    printf("建议删除令牌: %s\n", delete_recovery->suggestion);
    F_apply_recovery_strategy(parser, delete_recovery);
    F_destroy_phrase_recovery_result(delete_recovery);
}
```

## 设计原理

### 恢复策略选择

短语级恢复模块根据错误类型自动选择合适的恢复策略：

1. **缺失令牌错误** → 插入缺失令牌
2. **意外令牌错误** → 先尝试删除，再尝试替换
3. **令牌不匹配错误** → 替换错误令牌
4. **未关闭结构错误** → 插入缺失的关闭令牌
5. **其他错误类型** → 跳过当前令牌

### 智能建议生成

模块根据错误上下文生成智能修复建议：
- 基于错误位置和类型
- 考虑前后令牌的语法关系
- 识别常见错误模式
- 提供具体的修复示例

### 安全恢复

恢复策略设计确保不会引入新的错误：
- 验证恢复操作的合法性
- 检查语法约束
- 维护分析器状态一致性
- 提供回滚机制

## 性能考虑

### 内存管理
- 恢复结果对象动态分配
- 及时释放不再使用的对象
- 避免内存泄漏

### 性能优化
- 缓存常见错误模式
- 优化字符串操作
- 减少不必要的令牌复制

## 扩展性

### 添加新的恢复策略
1. 在恢复策略枚举中添加新策略
2. 实现对应的恢复函数
3. 更新策略选择逻辑
4. 测试新策略的有效性

### 自定义错误模式
1. 实现自定义错误模式检测函数
2. 注册到错误模式检测系统
3. 提供对应的修复建议

## 维护信息

### 版本历史
- **1.0.0** (2026-01-09): 初始版本，基于模块化架构设计

### 维护者
CN_Language架构委员会

### 许可证
MIT许可证
