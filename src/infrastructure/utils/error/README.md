# CN_Language 错误处理框架

## 概述

CN_Language错误处理框架是一个统一、模块化的错误处理系统，为CN_Language项目提供完整的错误管理功能。框架基于分层架构设计，遵循SOLID原则，支持错误码定义、错误上下文传递和错误链管理。

## 功能特性

### 1. 统一错误码系统
- **分层错误码**：按模块分类的错误码，便于识别和定位问题
- **错误码范围**：
  - `0x0000-0x0FFF`: 通用错误
  - `0x1000-0x1FFF`: 基础设施层错误
  - `0x2000-0x2FFF`: 核心层错误
  - `0x3000-0x3FFF`: 应用层错误
  - `0x4000-0x4FFF`: 用户自定义错误
- **错误描述**：每个错误码都有详细的中文描述

### 2. 错误上下文传递
- **完整上下文信息**：包含错误码、消息、文件名、行号、列号、函数名、模块名等
- **时间戳和线程ID**：支持多线程环境下的错误追踪
- **可恢复性标记**：标识错误是否可恢复
- **用户自定义数据**：支持附加用户数据

### 3. 错误链支持
- **错误链管理**：支持多个相关错误的链式管理
- **错误链操作**：添加、查找、遍历、合并、格式化等
- **错误严重性分析**：自动识别最严重的错误
- **错误摘要生成**：生成错误统计摘要

### 4. 全局错误处理
- **全局回调**：支持注册全局错误处理回调
- **自动报告**：可配置自动报告到标准错误输出
- **错误统计**：记录错误发生次数和类型统计
- **框架配置**：灵活的配置选项

## 模块结构

```
src/infrastructure/utils/error/
├── CN_error.h              # 主头文件，统一接口
├── CN_error_codes.h        # 错误码定义
├── CN_error_context.h      # 错误上下文定义
├── CN_error_chain.h        # 错误链定义
├── CN_error_codes.c        # 错误码实现
├── CN_error_context.c      # 错误上下文实现
├── CN_error_chain.c        # 错误链实现
├── CN_error_main.c         # 主框架实现
└── README.md              # 本文档
```

## 快速开始

### 1. 基本使用

```c
#include "CN_error.h"

// 初始化错误处理框架
CN_error_framework_init();

// 创建错误上下文
Stru_CN_ErrorContext_t error = CN_error_create_simple_context(
    Eum_CN_ERROR_INVALID_ARGUMENT,
    "参数无效");

// 报告错误
CN_error_report(&error);

// 检查是否有错误发生
if (CN_error_has_occurred()) {
    // 处理错误
}

// 清理框架
CN_error_framework_cleanup();
```

### 2. 使用错误链

```c
#include "CN_error.h"

// 创建错误链
Stru_CN_ErrorChain_t chain = CN_error_chain_create(10, true);

// 添加多个错误
CN_error_chain_add_new(&chain,
    Eum_CN_ERROR_INVALID_ARGUMENT,
    "第一个参数无效",
    "test.cn", 10, 5, NULL, NULL);

CN_error_chain_add_new(&chain,
    Eum_CN_ERROR_TYPE_MISMATCH,
    "类型不匹配",
    "test.cn", 15, 8, NULL, NULL);

// 格式化错误链
char buffer[1024];
CN_error_chain_format(&chain, buffer, sizeof(buffer), "\n");

// 输出错误链
printf("错误链:\n%s\n", buffer);

// 销毁错误链
CN_error_chain_destroy(&chain);
```

### 3. 全局错误处理

```c
#include "CN_error.h"

// 定义错误处理回调
void my_error_handler(const Stru_CN_ErrorContext_t* context, void* user_data) {
    printf("自定义错误处理: %s\n", CN_error_get_default_message(context));
}

int main() {
    // 初始化框架
    CN_error_framework_init();
    
    // 设置全局处理器
    CN_error_set_global_handler(my_error_handler, NULL);
    
    // 配置框架
    Stru_CN_ErrorConfig_t config = CN_error_get_config();
    config.verbose_logging = true;
    config.auto_report_to_stderr = true;
    CN_error_set_config(&config);
    
    // 使用框架...
    
    // 清理框架
    CN_error_framework_cleanup();
    return 0;
}
```

## API文档

### 核心头文件

#### `CN_error.h`
- `CN_error_framework_init()`: 初始化错误处理框架
- `CN_error_framework_cleanup()`: 清理错误处理框架
- `CN_error_report()`: 报告错误
- `CN_error_report_quick()`: 快速报告错误
- `CN_error_get_last_reported()`: 获取最后一个报告的错误
- `CN_error_has_occurred()`: 检查是否有错误发生
- `CN_error_get_severity()`: 获取错误严重性级别
- `CN_error_is_recoverable()`: 检查错误是否可恢复

#### `CN_error_codes.h`
- `Eum_CN_ErrorCode_t`: 错误码枚举类型
- `CN_error_get_description()`: 获取错误码描述

#### `CN_error_context.h`
- `Stru_CN_ErrorContext_t`: 错误上下文结构体
- `CN_error_create_context()`: 创建错误上下文
- `CN_error_create_simple_context()`: 创建简单错误上下文
- `CN_error_create_position_context()`: 创建带位置信息的错误上下文
- `CN_error_format_context()`: 格式化错误上下文为字符串
- `CN_error_is_context_valid()`: 检查错误上下文是否有效

#### `CN_error_chain.h`
- `Stru_CN_ErrorChain_t`: 错误链结构体
- `CN_error_chain_create()`: 创建错误链
- `CN_error_chain_destroy()`: 销毁错误链
- `CN_error_chain_add()`: 添加错误到错误链
- `CN_error_chain_get_first()`: 获取第一个错误
- `CN_error_chain_get_last()`: 获取最后一个错误
- `CN_error_chain_format()`: 格式化错误链为字符串
- `CN_error_chain_create_summary()`: 创建错误摘要

## 错误码分类

### 通用错误 (0x0000-0x0FFF)
- `Eum_CN_ERROR_SUCCESS`: 成功
- `Eum_CN_ERROR_OUT_OF_MEMORY`: 内存不足
- `Eum_CN_ERROR_INVALID_ARGUMENT`: 无效参数
- `Eum_CN_ERROR_NULL_POINTER`: 空指针
- `Eum_CN_ERROR_OUT_OF_BOUNDS`: 索引越界
- 等19个通用错误码

### 基础设施层错误 (0x1000-0x1FFF)
- **内存错误** (`0x1000-0x10FF`): 内存分配、释放、泄漏等
- **容器错误** (`0x1100-0x11FF`): 容器操作错误
- **字符串错误** (`0x1200-0x12FF`): 字符串处理错误
- **数学错误** (`0x1300-0x13FF`): 数学运算错误
- **文件系统错误** (`0x1400-0x14FF`): 文件操作错误
- **网络错误** (`0x1500-0x15FF`): 网络操作错误

### 核心层错误 (0x2000-0x2FFF)
- **词法分析错误** (`0x2000-0x20FF`): 词法分析相关错误
- **语法分析错误** (`0x2100-0x21FF`): 语法分析相关错误
- **语义分析错误** (`0x2200-0x22FF`): 语义分析相关错误
- **代码生成错误** (`0x2300-0x23FF`): 代码生成相关错误
- **运行时错误** (`0x2400-0x24FF`): 运行时相关错误

### 应用层错误 (0x3000-0x3FFF)
- **命令行界面错误** (`0x3000-0x30FF`): CLI相关错误
- **REPL错误** (`0x3100-0x31FF`): REPL环境错误
- **调试器错误** (`0x3200-0x32FF`): 调试器相关错误
- **IDE插件错误** (`0x3300-0x33FF`): IDE插件相关错误

### 用户自定义错误 (0x4000-0x4FFF)
- `Eum_CN_ERROR_USER_1` 到 `Eum_CN_ERROR_USER_6`: 用户自定义错误

## 设计原则

### 1. 单一职责原则
- 每个文件只负责一个功能领域
- 每个函数不超过50行
- 每个.c文件不超过500行

### 2. 开闭原则
- 通过接口支持扩展
- 错误码范围可扩展
- 配置选项可定制

### 3. 依赖倒置原则
- 高层模块定义接口
- 低层模块实现接口
- 通过依赖注入管理依赖

### 4. 接口隔离原则
- 为不同客户端提供专用接口
- 避免"胖接口"
- 通过接口组合提供灵活功能

## 性能考虑

### 内存使用
- 错误上下文结构体大小固定
- 错误链节点动态分配，支持自动释放
- 可配置最大错误链长度，防止内存泄漏

### 性能优化
- 错误码描述使用switch语句，O(1)复杂度
- 错误链操作使用双向链表，支持快速插入和删除
- 格式化函数使用缓冲区，避免多次分配

## 测试建议

### 单元测试
1. 测试每个错误码的描述是否正确
2. 测试错误上下文的创建和验证
3. 测试错误链的基本操作
4. 测试全局错误处理回调

### 集成测试
1. 测试错误处理框架与其他模块的集成
2. 测试多线程环境下的错误处理
3. 测试错误链的合并和格式化

### 性能测试
1. 测试大量错误报告的性能
2. 测试错误链操作的性能
3. 测试内存使用情况

## 兼容性

### 平台支持
- Windows (使用Windows API获取线程ID和时间戳)
- Linux/macOS (使用pthread和clock_gettime)

### 编译器支持
- 支持C99标准
- 使用标准库函数
- 无第三方依赖

## 版本历史

### v1.0.0 (2026-01-03)
- 初始版本
- 统一错误码系统
- 错误上下文传递
- 错误链支持
- 全局错误处理框架

## 贡献指南

1. 遵循项目编码规范
2. 添加新错误码时，确保在正确范围内
3. 更新相关文档
4. 添加相应的测试用例

## 许可证

MIT许可证
