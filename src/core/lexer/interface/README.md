# 词法分析器接口层

## 概述

接口层提供词法分析器的抽象接口定义和实现。它遵循项目架构规范，使用接口模式实现模块解耦。

## 文件说明

### CN_lexer_interface_impl.h
- 接口实现头文件
- 声明工厂函数 `F_create_lexer_interface_impl()`
- 定义内部辅助函数声明

### CN_lexer_interface_impl.c
- 接口实现源文件
- 实现接口函数包装器
- 提供工厂函数的具体实现

## 接口设计

### 工厂函数

```c
/**
 * @brief 创建词法分析器接口实例（实现版本）
 * 
 * 创建并返回一个新的词法分析器接口实例。
 * 调用者负责在不再使用时调用destroy函数。
 * 
 * @return Stru_LexerInterface_t* 新创建的词法分析器接口实例
 */
Stru_LexerInterface_t* F_create_lexer_interface_impl(void);
```

### 接口函数包装器

接口层提供以下包装器函数，将抽象接口调用转发到具体的实现模块：

1. **初始化包装器** (`F_lexer_initialize_wrapper`)
   - 调用扫描器状态的初始化函数
   - 验证输入参数

2. **令牌获取包装器** (`F_lexer_next_token_wrapper`)
   - 调用令牌扫描器获取下一个令牌
   - 处理错误状态

3. **状态检查包装器** (`F_lexer_has_more_tokens_wrapper`)
   - 检查是否还有更多令牌
   - 保存和恢复扫描器状态

4. **批量令牌化包装器** (`F_lexer_tokenize_all_wrapper`)
   - 批量处理所有令牌
   - 使用动态数组存储结果

5. **位置获取包装器** (`F_lexer_get_position_wrapper`)
   - 获取当前扫描位置
   - 返回行号和列号

6. **错误处理包装器** (`F_lexer_has_errors_wrapper`, `F_lexer_get_last_error_wrapper`)
   - 检查错误状态
   - 获取错误信息

7. **资源管理包装器** (`F_lexer_reset_wrapper`, `F_lexer_destroy_wrapper`)
   - 重置扫描器状态
   - 释放所有资源

## 依赖关系

### 内部依赖
- `../scanner/CN_lexer_scanner.h` - 扫描器状态管理
- `../token_scanners/CN_lexer_token_scanners.h` - 令牌扫描功能
- `../../../infrastructure/containers/array/CN_dynamic_array.h` - 动态数组支持

### 外部依赖
- `stdlib.h` - 内存分配函数
- `CN_lexer_interface.h` - 抽象接口定义

## 架构合规性

### 单一职责原则
- 只负责接口实现和工厂函数
- 不包含具体的词法分析逻辑
- 每个函数不超过50行

### 接口隔离原则
- 提供完整的接口实现
- 隐藏内部实现细节
- 通过函数指针暴露功能

### 依赖倒置原则
- 依赖抽象接口而非具体实现
- 通过工厂函数创建实例
- 支持依赖注入

## 使用示例

```c
#include "CN_lexer_interface_impl.h"

// 创建词法分析器接口实例
Stru_LexerInterface_t* lexer = F_create_lexer_interface_impl();

// 使用接口...
lexer->initialize(lexer, source, strlen(source), "test.cn");

// 清理资源
lexer->destroy(lexer);
```

## 错误处理

接口层提供以下错误处理机制：

1. **参数验证** - 所有包装器函数都验证输入参数
2. **错误传播** - 将底层错误传播到接口层
3. **资源清理** - 确保在错误情况下正确释放资源

## 性能考虑

1. **最小化包装开销** - 包装器函数尽量简单，减少调用开销
2. **内存管理** - 合理管理内存分配和释放
3. **状态保存** - 在状态检查时保存和恢复扫描器状态

## 维护指南

### 添加新接口方法
1. 在 `CN_lexer_interface.h` 中添加方法声明
2. 在 `CN_lexer_interface_impl.h` 中添加包装器函数声明
3. 在 `CN_lexer_interface_impl.c` 中实现包装器函数
4. 在工厂函数中设置函数指针

### 修改现有实现
1. 更新包装器函数实现
2. 确保保持向后兼容性
3. 更新相关测试用例

## 版本历史

### v1.0.0 (2026-01-08)
- 初始版本发布
- 完整的接口实现
- 支持所有70个中文关键字
- 通过单元测试验证
