# 内存调试模块 (Memory Debug) - 版本 2.0.0

## 概述

内存调试模块提供了全面的内存调试和诊断功能，帮助开发者检测和修复内存相关的问题。该模块在开发和测试阶段特别有用，可以检测内存泄漏、缓冲区溢出、双重释放等常见内存错误。

## 模块化架构

本模块采用模块化设计，分为以下子模块：

### 1. 接口模块 (`interface/`)
- `CN_memory_debug_interface.h` - 内存调试器抽象接口定义
- `CN_memory_debug_factory.h` - 内存调试器工厂函数

### 2. 泄漏检测模块 (`leak_detection/`)
- `CN_memory_leak_detection.h` - 内存泄漏检测功能

### 3. 错误检测模块 (`error_detection/`)
- `CN_memory_error_detection.h` - 内存错误检测功能

### 4. 分析模块 (`analysis/`)
- `CN_memory_analysis.h` - 内存使用分析功能

### 5. 工具模块 (`tools/`)
- `CN_memory_debug_tools.h` - 内存调试工具

### 6. 实现模块 (`implementation/`)
- `CN_memory_debug_private.h/.c` - 私有数据结构和辅助函数
- `CN_memory_debug_core.h/.c` - 核心接口函数实现

### 7. 主入口点
- `CN_memory_debug.h` - 主头文件，包含所有子模块
- `CN_memory_debug.c` - 主实现文件（向后兼容）
- `CN_memory_debug_new.c` - 模块化实现（薄包装层）

## 核心功能

### 1. 内存泄漏检测
- **实时监控**: 监控所有内存分配和释放
- **泄漏报告**: 生成详细的内存泄漏报告
- **调用栈跟踪**: 记录分配点的调用栈信息

### 2. 内存错误检测
- **缓冲区溢出检测**: 检测读写越界
- **双重释放检测**: 检测重复释放同一内存块
- **野指针检测**: 检测使用已释放内存
- **未初始化内存检测**: 检测使用未初始化内存

### 3. 内存使用分析
- **内存使用统计**: 统计不同类型的内存使用
- **内存碎片分析**: 分析内存碎片程度
- **性能分析**: 分析内存分配性能

### 4. 调试工具
- **内存转储**: 转储内存内容用于调试
- **内存验证**: 验证内存块的完整性
- **调试信息**: 提供详细的调试信息

## 接口设计

### 核心接口
内存调试器采用抽象接口模式，通过`Stru_MemoryDebuggerInterface_t`结构体提供所有功能。接口函数通过函数指针实现，支持运行时配置和扩展。

### 工厂函数
模块提供了多种工厂函数来创建调试器实例：
- `F_create_default_memory_debugger()` - 创建默认配置的调试器
- `F_create_configured_memory_debugger()` - 创建自定义配置的调试器
- `F_get_memory_debugger_interface()` - 获取单例调试器接口（向后兼容）

### 向后兼容性
模块保持对旧接口`Stru_MemoryDebugInterface_t`的向后兼容性，通过适配器模式支持现有代码。

## 使用示例

### 1. 基本使用（新接口）
```c
#include "CN_memory_debug.h"

// 创建默认内存调试器
Stru_MemoryDebuggerInterface_t* debugger = F_create_default_memory_debugger();
if (debugger == NULL) {
    printf("无法创建内存调试器\n");
    return;
}

// 初始化调试器
if (!debugger->initialize(debugger)) {
    printf("调试器初始化失败\n");
    F_destroy_memory_debugger(debugger);
    return;
}

// 启用内存监控
debugger->enable_monitoring(debugger, true);

// 启用各种检查
debugger->enable_overflow_check(debugger, true);
debugger->enable_double_free_check(debugger, true);
debugger->enable_uninitialized_check(debugger, false); // 默认禁用

// 启用调用栈跟踪
debugger->enable_stack_trace(debugger, true);
debugger->set_stack_trace_depth(debugger, 10);

// 进行内存操作...
void* ptr1 = malloc(100);
void* ptr2 = calloc(10, 20);
free(ptr1);
free(ptr2);

// 检查内存泄漏
size_t leak_count = debugger->get_leak_count(debugger);
if (leak_count > 0) {
    printf("发现 %zu 个内存泄漏\n", leak_count);
    debugger->report_leaks(debugger);
}

// 清理
debugger->cleanup(debugger);
F_destroy_memory_debugger(debugger);
```

### 2. 向后兼容使用（旧接口）
```c
#include "CN_memory_debug.h"

// 获取内存调试器接口（向后兼容）
Stru_MemoryDebuggerInterface_t* debug_if = F_get_memory_debugger_interface();

// 初始化调试器
debug_if->initialize(debug_if);

// 启用内存监控
debug_if->enable_monitoring(debug_if, true);

// ... 其他操作与上面相同
```

### 2. 内存验证
```c
#include "CN_memory_debug.h"

// 获取内存调试器接口
Stru_MemoryDebuggerInterface_t* debug_if = F_get_memory_debugger_interface();

// 初始化
debug_if->initialize();
debug_if->enable_monitoring(true);

// 分配内存
void* memory = malloc(256);

// 验证指针
bool valid = debug_if->validate_pointer(memory);
if (!valid) {
    printf("无效指针!\n");
}

// 验证内存范围
valid = debug_if->validate_memory_range(memory, 256);
if (!valid) {
    printf("内存范围无效!\n");
}

// 检查所有分配
debug_if->check_all_allocations();

// 转储内存信息
debug_if->dump_memory_info();

// 清理
free(memory);
debug_if->cleanup();
```

### 3. 性能分析
```c
#include "CN_memory_debug.h"
#include <stdio.h>

// 获取内存调试器接口
Stru_MemoryDebuggerInterface_t* debug_if = F_get_memory_debugger_interface();

// 初始化
debug_if->initialize();
debug_if->enable_monitoring(true);

// 模拟大量内存操作
for (int i = 0; i < 1000; i++) {
    void* ptr = malloc(i * 10);
    // 使用内存...
    free(ptr);
}

// 获取性能统计
uint64_t total_allocs = debug_if->get_total_allocations();
uint64_t total_frees = debug_if->get_total_deallocations();
size_t peak_usage = debug_if->get_peak_memory_usage();
size_t current_usage = debug_if->get_current_memory_usage();

printf("性能统计:\n");
printf("  总分配次数: %llu\n", total_allocs);
printf("  总释放次数: %llu\n", total_frees);
printf("  峰值内存使用: %zu bytes\n", peak_usage);
printf("  当前内存使用: %zu bytes\n", current_usage);

// 转储分配统计
debug_if->dump_allocation_stats();

// 清理
debug_if->cleanup();
```

## 依赖关系

- **内部依赖**: 内存分配器模块
- **外部依赖**: C标准库 (stdlib.h, string.h, stdint.h)
- **可选依赖**: 调用栈跟踪库（如libunwind）

## 设计原则

### 1. 单一职责原则
- 调试器只负责调试功能
- 与分配器功能分离
- 清晰的接口边界

### 2. 开闭原则
- 通过接口支持新的调试功能
- 现有代码对扩展开放
- 插件式架构

### 3. 接口隔离原则
- 为不同调试场景提供专用接口
- 避免功能耦合
- 最小化依赖

## 性能考虑

### 1. 运行时开销
- 调试功能有性能开销
- 可选择性启用功能
- 生产环境建议禁用

### 2. 内存开销
- 调试信息占用额外内存
- 哨兵值增加内存使用
- 调用栈跟踪增加开销

### 3. 优化策略
- 条件编译禁用调试
- 运行时动态启用/禁用
- 采样式监控减少开销

## 安全特性

### 1. 内存保护
- 哨兵值保护内存边界
- 内存填充检测越界访问
- 释放后填充防止误用

### 2. 错误检测
- 实时错误检测
- 详细错误报告
- 错误上下文信息

### 3. 数据完整性
- 内存内容验证
- 结构完整性检查
- 一致性验证

## 使用场景

### 1. 开发阶段
- 早期发现内存错误
- 调试复杂内存问题
- 性能优化分析

### 2. 测试阶段
- 自动化内存测试
- 压力测试监控
- 回归测试验证

### 3. 生产调试
- 现场问题诊断
- 内存泄漏调查
- 性能问题分析

## 扩展性

### 1. 自定义检测器
- 添加新的错误检测类型
- 自定义验证规则
- 特定领域内存检查

### 2. 输出格式
- 支持多种输出格式
- 自定义报告生成
- 日志系统集成

### 3. 平台适配
- 不同平台调用栈跟踪
- 平台特定内存检查
- 操作系统集成

## 注意事项

1. 调试功能有显著性能开销
2. 生产环境应禁用调试功能
3. 调用栈跟踪需要额外库支持
4. 内存使用统计可能不精确

## 版本历史

- v1.0.0: 初始版本，基本内存调试功能
- v1.1.0: 添加调用栈跟踪支持
- v1.2.0: 添加性能分析功能
- v1.3.0: 优化内存使用，减少开销
- v2.0.0: 模块化重构，遵循单一职责原则
  - 拆分大文件为多个小文件
  - 每个.c文件不超过500行
  - 每个函数不超过50行
  - 清晰的模块边界
  - 改进的接口设计

## 维护者

CN_Language架构委员会
