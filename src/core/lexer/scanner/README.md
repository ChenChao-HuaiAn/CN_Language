# 字符扫描器模块

## 概述

字符扫描器模块负责源代码的字符级操作，包括字符读取、位置跟踪、空白字符跳过和错误状态管理。它是词法分析器的基础组件，为令牌扫描提供底层支持。

## 文件说明

### CN_lexer_scanner.h
- 扫描器状态结构体定义 (`Stru_LexerScannerState_t`)
- 公共函数声明
- 错误状态定义

### CN_lexer_scanner.c
- 扫描器状态管理函数
- 字符操作函数
- 位置跟踪函数
- 错误处理函数

## 核心数据结构

### 扫描器状态结构体

```c
typedef struct Stru_LexerScannerState_t {
    const char* source;          ///< 源代码字符串
    size_t source_length;        ///< 源代码长度
    const char* source_name;     ///< 源文件名
    size_t current_pos;          ///< 当前位置（字节偏移）
    size_t current_line;         ///< 当前行号（从1开始）
    size_t current_column;       ///< 当前列号（从1开始）
    bool has_error;              ///< 是否有错误
    char error_message[256];     ///< 错误信息缓冲区
} Stru_LexerScannerState_t;
```

## 功能特性

### 1. 字符操作
- **查看字符** (`F_peek_char`) - 查看下一个字符而不移动位置
- **获取字符** (`F_next_char`) - 获取下一个字符并更新位置
- **字符分类** - 判断字符类型（空白、数字、字母等）

### 2. 位置跟踪
- **行号跟踪** - 自动跟踪换行符更新行号
- **列号跟踪** - 跟踪当前列位置
- **位置保存/恢复** - 支持临时保存和恢复扫描位置

### 3. 空白字符处理
- **空白字符跳过** (`F_skip_whitespace`) - 跳过空格、制表符、换行符
- **注释处理** (`F_skip_comment`) - 跳过单行注释（以#开头）
- **Windows换行符支持** - 正确处理\r\n换行符

### 4. 错误处理
- **错误设置** (`F_set_scanner_error`) - 设置错误信息和状态
- **错误清除** (`F_clear_scanner_error`) - 清除错误状态
- **错误检查** (`F_scanner_has_errors`) - 检查是否有错误
- **错误信息获取** (`F_get_scanner_last_error`) - 获取最后一个错误信息

## 主要函数

### 状态管理函数
- `F_create_scanner_state()` - 创建扫描器状态
- `F_destroy_scanner_state()` - 销毁扫描器状态
- `F_initialize_scanner_state()` - 初始化扫描器状态
- `F_reset_scanner_state()` - 重置扫描器状态

### 字符操作函数
- `F_peek_char()` - 查看下一个字符
- `F_next_char()` - 获取下一个字符
- `F_skip_whitespace()` - 跳过空白字符
- `F_skip_comment()` - 跳过注释

### 位置管理函数
- `F_get_scanner_position()` - 获取当前位置
- `F_get_scanner_source_name()` - 获取源文件名

### 错误处理函数
- `F_set_scanner_error()` - 设置错误
- `F_clear_scanner_error()` - 清除错误
- `F_scanner_has_errors()` - 检查是否有错误
- `F_get_scanner_last_error()` - 获取错误信息

## 依赖关系

### 内部依赖
- 无（基础模块）

### 外部依赖
- `stdlib.h` - 标准库函数
- `string.h` - 字符串处理函数
- `stdbool.h` - 布尔类型支持

## 架构合规性

### 单一职责原则
- 只负责字符级操作和位置跟踪
- 不包含令牌识别逻辑
- 每个函数不超过50行

### 接口最小化
- 只暴露必要的操作函数
- 隐藏内部状态管理细节
- 提供清晰的错误处理接口

### 数据封装
- 扫描器状态结构体对外部透明
- 通过函数操作状态，避免直接访问
- 确保状态一致性

## 使用示例

```c
#include "CN_lexer_scanner.h"

// 创建扫描器状态
Stru_LexerScannerState_t* state = F_create_scanner_state();

// 初始化状态
F_initialize_scanner_state(state, source, strlen(source), "test.cn");

// 跳过空白字符
F_skip_whitespace(state);

// 获取字符
while (state->current_pos < state->source_length) {
    char c = F_next_char(state);
    // 处理字符...
}

// 检查错误
if (F_scanner_has_errors(state)) {
    printf("错误: %s\n", F_get_scanner_last_error(state));
}

// 清理资源
F_destroy_scanner_state(state);
```

## 性能优化

### 1. 高效的位置跟踪
- 增量更新行号和列号
- 避免每次计算位置
- 支持快速位置保存/恢复

### 2. 最小化的内存分配
- 状态结构体一次分配
- 错误信息使用固定大小缓冲区
- 避免频繁的内存分配

### 3. 快速的字符操作
- 直接指针访问源代码
- 简单的字符分类逻辑
- 优化的空白字符跳过

## 错误处理策略

### 1. 错误类型
- **输入错误** - 无效的源代码
- **状态错误** - 扫描器状态不一致
- **资源错误** - 内存分配失败

### 2. 错误恢复
- 设置错误标志
- 保存错误信息
- 允许继续扫描（跳过错误字符）

### 3. 错误报告
- 详细的错误信息
- 错误位置（行号、列号）
- 可读的错误描述

## 维护指南

### 添加新功能
1. 在头文件中声明新函数
2. 在源文件中实现功能
3. 确保保持接口兼容性
4. 添加相应的测试用例

### 修改现有功能
1. 更新函数实现
2. 确保不影响现有调用者
3. 更新相关文档
4. 运行测试验证

## 版本历史

### v1.0.0 (2026-01-08)
- 初始版本发布
- 完整的字符扫描功能
- 位置跟踪和错误处理
- 通过单元测试验证
