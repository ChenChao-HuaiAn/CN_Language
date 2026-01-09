# CN_Language 语法分析器错误处理模块

## 概述

错误处理模块是CN_Language语法分析器的核心组件之一，专门负责处理语法分析过程中的错误。本模块提供全面的错误检测、报告、恢复和格式化功能，确保语法分析器能够优雅地处理各种语法错误，并提供有用的错误信息帮助用户调试代码。

## 模块功能

### 核心功能
1. **错误检测** - 检测各种类型的语法错误
2. **错误报告** - 生成详细的错误信息
3. **错误恢复** - 在发生错误后尝试恢复分析
4. **错误格式化** - 格式化错误信息以便显示
5. **错误上下文** - 提供错误发生的上下文信息

### 支持的错误类型
- 意外令牌错误
- 缺失令牌错误
- 类型不匹配错误
- 重复声明错误
- 未声明标识符错误
- 参数数量错误
- 无效表达式错误
- 无效语句错误
- 无效声明错误
- 未关闭结构错误

## 文件结构

### 主要文件
1. **CN_parser_errors.h** - 错误处理头文件
   - 声明所有错误处理函数
   - 定义错误处理接口

2. **CN_parser_errors.c** - 错误处理源文件
   - 实现所有错误处理函数
   - 实现错误恢复逻辑

### 依赖关系
- 语法分析器接口 (`../CN_parser_interface.h`)
- 语法错误处理接口 (`../CN_syntax_error.h`)
- 令牌模块 (`../../token/CN_token_types.h`)
- 基础设施层容器 (`../../../infrastructure/containers/array/CN_dynamic_array.h`)

## 错误处理函数

### 错误报告函数

#### `F_report_parser_error()`
报告语法分析错误，支持自定义错误类型和严重级别。

#### `F_report_unexpected_token_error()`
报告意外令牌错误，如期望分号但找到其他令牌。

#### `F_report_missing_token_error()`
报告缺失令牌错误，如缺失右括号。

#### `F_report_type_mismatch_error()`
报告类型不匹配错误，如期望整数但找到字符串。

#### `F_report_duplicate_declaration_error()`
报告重复声明错误，如重复声明同一变量。

#### `F_report_undeclared_identifier_error()`
报告未声明标识符错误，如使用未声明的变量。

#### `F_report_argument_count_error()`
报告函数调用参数数量错误。

#### `F_report_invalid_expression_error()`
报告无效表达式错误。

#### `F_report_invalid_statement_error()`
报告无效语句错误。

#### `F_report_invalid_declaration_error()`
报告无效声明错误。

#### `F_report_unclosed_structure_error()`
报告未关闭结构错误，如未关闭的代码块。

### 错误恢复函数

#### `F_try_error_recovery()`
在发生错误后尝试恢复语法分析。

#### `F_synchronize_to_safe_point()`
在发生错误后同步到安全点继续分析。

#### `F_get_error_context()`
获取错误发生的上下文信息。

## 错误严重级别

错误处理模块支持多种错误严重级别：

| 级别 | 描述 | 处理方式 |
|------|------|----------|
| **ERROR** | 严重错误，无法继续分析 | 停止分析或尝试恢复 |
| **WARNING** | 警告，可以继续分析 | 继续分析，记录警告 |
| **INFO** | 信息性消息 | 记录信息，继续分析 |

## 错误恢复策略

### 同步恢复
在发生错误后，尝试同步到以下安全点：
1. 语句结束（分号）
2. 代码块结束（右大括号）
3. 函数声明结束
4. 模块声明结束

### 恐慌模式恢复
当无法正常恢复时，进入恐慌模式：
1. 跳过当前令牌
2. 寻找同步点
3. 重置分析状态
4. 继续分析

### 最大错误限制
默认情况下，语法分析器在报告100个错误后停止分析。可以通过配置修改此限制。

## 使用示例

### 基本用法
```c
#include "error_handling/CN_parser_errors.h"

// 获取语法分析器接口
Stru_ParserInterface_t* parser = F_create_parser_interface();
parser->initialize(parser, lexer);

// 报告意外令牌错误
Stru_Token_t* token = parser->get_current_token(parser);
Stru_SyntaxError_t* error = F_report_unexpected_token_error(parser, token, "期望分号");

// 尝试错误恢复
if (error != NULL && error->severity == Eum_SYNTAX_ERROR_SEVERITY_ERROR) {
    bool recovered = F_try_error_recovery(parser, error);
    if (!recovered) {
        // 无法恢复，停止分析
        return NULL;
    }
}

// 清理资源
parser->destroy(parser);
```

### 错误上下文获取
```c
// 获取错误上下文
char context[256];
int context_len = F_get_error_context(parser, error->line, error->column, context, sizeof(context));

if (context_len > 0) {
    printf("错误上下文: %s\n", context);
    printf("错误位置: 第%zu行, 第%zu列\n", error->line, error->column);
    printf("错误信息: %s\n", error->message);
}
```

## 设计原理

### 错误对象模型
- 每个错误都是独立的错误对象
- 错误对象包含完整的错误信息
- 支持错误链（一个错误导致另一个错误）

### 错误收集
- 错误收集在动态数组中
- 支持错误过滤和排序
- 支持错误统计

### 错误格式化
- 支持多种错误格式（文本、JSON、XML）
- 支持本地化错误消息
- 支持颜色高亮显示

## 测试

### 单元测试
- 测试各种错误类型的报告
- 测试错误恢复机制
- 测试错误上下文获取

### 测试用例示例
```c
// 测试意外令牌错误
TEST_ASSERT_NOT_NULL(F_report_unexpected_token_error(parser, token, "期望分号"));

// 测试错误恢复
TEST_ASSERT_TRUE(F_try_error_recovery(parser, error));

// 测试错误上下文
TEST_ASSERT_GREATER_THAN(0, F_get_error_context(parser, 10, 5, buffer, sizeof(buffer)));
```

## 性能考虑

### 错误收集
- 错误对象动态分配内存
- 支持错误对象池重用
- 避免内存泄漏

### 错误恢复
- 错误恢复可能消耗额外时间
- 同步操作可能跳过有效代码
- 需要平衡错误恢复和性能

## 扩展性

### 添加新的错误类型
1. 在错误类型枚举中添加新类型
2. 添加对应的错误报告函数
3. 实现错误消息格式化

### 自定义错误恢复策略
1. 实现自定义的错误恢复函数
2. 配置语法分析器使用自定义恢复策略
3. 测试恢复策略的有效性

## 维护信息

### 版本历史
- **1.0.0** (2026-01-08): 初始版本，基于模块化架构设计

### 维护者
CN_Language架构委员会

### 许可证
MIT许可证
