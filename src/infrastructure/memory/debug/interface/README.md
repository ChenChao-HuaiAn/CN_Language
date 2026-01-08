# 接口模块

## 概述

接口模块定义了内存调试器的抽象接口和工厂函数。该模块提供了内存调试器的高级抽象，使得客户端代码可以通过统一的接口使用不同的内存调试功能，而不需要了解具体的实现细节。

## 文件说明

### 1. CN_memory_debug_interface.h
内存调试器抽象接口定义文件，定义了`Stru_MemoryDebuggerInterface_t`结构体，包含所有内存调试功能的函数指针。

### 2. CN_memory_debug_factory.h
内存调试器工厂函数声明文件，提供了创建和销毁内存调试器实例的工厂函数。

## 接口设计

### 抽象接口模式
内存调试器采用抽象接口模式设计，具有以下特点：

1. **接口与实现分离**: 接口定义与具体实现完全分离
2. **运行时多态**: 通过函数指针实现运行时多态
3. **可替换性**: 不同的实现可以无缝替换
4. **依赖倒置**: 高层模块依赖抽象接口，不依赖具体实现

### 接口结构
```c
typedef struct Stru_MemoryDebuggerInterface_t {
    // 初始化/清理
    bool (*initialize)(Stru_MemoryDebuggerInterface_t* self);
    void (*cleanup)(Stru_MemoryDebuggerInterface_t* self);
    
    // 内存监控
    void (*enable_monitoring)(Stru_MemoryDebuggerInterface_t* self, bool enable);
    bool (*is_monitoring_enabled)(Stru_MemoryDebuggerInterface_t* self);
    
    // 泄漏检测
    size_t (*get_leak_count)(Stru_MemoryDebuggerInterface_t* self);
    void (*report_leaks)(Stru_MemoryDebuggerInterface_t* self);
    void (*clear_leak_records)(Stru_MemoryDebuggerInterface_t* self);
    
    // 错误检测
    void (*enable_overflow_check)(Stru_MemoryDebuggerInterface_t* self, bool enable);
    void (*enable_double_free_check)(Stru_MemoryDebuggerInterface_t* self, bool enable);
    void (*enable_uninitialized_check)(Stru_MemoryDebuggerInterface_t* self, bool enable);
    
    // 内存验证
    bool (*validate_pointer)(Stru_MemoryDebuggerInterface_t* self, const void* ptr);
    bool (*validate_memory_range)(Stru_MemoryDebuggerInterface_t* self, 
                                 const void* ptr, size_t size);
    void (*check_all_allocations)(Stru_MemoryDebuggerInterface_t* self);
    
    // 调试信息
    void (*dump_memory_info)(Stru_MemoryDebuggerInterface_t* self);
    void (*dump_allocation_stats)(Stru_MemoryDebuggerInterface_t* self);
    void (*set_debug_output)(Stru_MemoryDebuggerInterface_t* self, 
                           void (*output_func)(const char*));
    
    // 调用栈跟踪
    void (*enable_stack_trace)(Stru_MemoryDebuggerInterface_t* self, bool enable);
    size_t (*get_stack_trace_depth)(Stru_MemoryDebuggerInterface_t* self);
    void (*set_stack_trace_depth)(Stru_MemoryDebuggerInterface_t* self, size_t depth);
    
    // 性能分析
    uint64_t (*get_total_allocations)(Stru_MemoryDebuggerInterface_t* self);
    uint64_t (*get_total_deallocations)(Stru_MemoryDebuggerInterface_t* self);
    size_t (*get_peak_memory_usage)(Stru_MemoryDebuggerInterface_t* self);
    size_t (*get_current_memory_usage)(Stru_MemoryDebuggerInterface_t* self);
    
    // 私有数据（实现细节）
    void* private_data;
} Stru_MemoryDebuggerInterface_t;
```

## 工厂函数

### 1. 创建默认内存调试器
```c
Stru_MemoryDebuggerInterface_t* F_create_default_memory_debugger(void);
```

创建具有默认配置的内存调试器实例。

### 2. 创建配置的内存调试器
```c
Stru_MemoryDebuggerInterface_t* F_create_configured_memory_debugger(
    bool enable_leak_detection,
    bool enable_overflow_check,
    bool enable_double_free_check,
    size_t stack_trace_depth);
```

创建具有特定配置的内存调试器实例。

### 3. 销毁内存调试器
```c
void F_destroy_memory_debugger(Stru_MemoryDebuggerInterface_t* debugger);
```

销毁内存调试器实例，释放相关资源。

### 4. 获取内存调试器接口（向后兼容）
```c
Stru_MemoryDebuggerInterface_t* F_get_memory_debugger_interface(void);
```

获取单例内存调试器接口，用于向后兼容。

## 使用示例

### 基本使用
```c
#include "CN_memory_debug_interface.h"
#include "CN_memory_debug_factory.h"

int main() {
    // 创建内存调试器
    Stru_MemoryDebuggerInterface_t* debugger = F_create_default_memory_debugger();
    if (!debugger) {
        return 1;
    }
    
    // 初始化
    if (!debugger->initialize(debugger)) {
        F_destroy_memory_debugger(debugger);
        return 1;
    }
    
    // 使用调试功能
    debugger->enable_monitoring(debugger, true);
    debugger->enable_overflow_check(debugger, true);
    
    // 进行内存操作...
    
    // 检查泄漏
    size_t leaks = debugger->get_leak_count(debugger);
    if (leaks > 0) {
        debugger->report_leaks(debugger);
    }
    
    // 清理
    debugger->cleanup(debugger);
    F_destroy_memory_debugger(debugger);
    
    return 0;
}
```

### 自定义配置
```c
// 创建具有特定配置的调试器
Stru_MemoryDebuggerInterface_t* debugger = F_create_configured_memory_debugger(
    true,   // 启用泄漏检测
    true,   // 启用溢出检查
    false,  // 禁用双重释放检查
    8       // 调用栈深度
);
```

## 设计原则

### 1. 单一职责原则
- 接口只定义契约，不包含实现
- 工厂函数只负责创建实例
- 每个函数有明确的职责

### 2. 开闭原则
- 接口对扩展开放
- 现有代码对修改封闭
- 支持添加新的调试功能

### 3. 里氏替换原则
- 所有实现必须遵守接口契约
- 实现可以无缝替换
- 通过接口测试验证替换性

### 4. 接口隔离原则
- 为不同客户端提供专用接口
- 避免"胖接口"
- 通过接口组合提供灵活功能

### 5. 依赖倒置原则
- 高层模块依赖抽象接口
- 低层模块实现抽象接口
- 通过工厂函数管理依赖

## 向后兼容性

### 1. 旧接口支持
模块保持对旧接口`Stru_MemoryDebugInterface_t`的向后兼容性，通过适配器模式支持现有代码。

### 2. 迁移路径
- 新代码使用新接口`Stru_MemoryDebuggerInterface_t`
- 旧代码可以继续使用旧接口
- 提供迁移工具和示例

### 3. 版本管理
- 接口版本号遵循语义化版本
- 废弃接口标记为已废弃
- 提供至少两个主版本的兼容性

## 扩展指南

### 1. 添加新功能
1. 在接口中添加新的函数指针
2. 在工厂函数中添加配置选项
3. 在实现模块中实现新功能
4. 更新文档和测试

### 2. 创建新实现
1. 实现所有接口函数
2. 提供工厂函数创建实例
3. 确保遵守接口契约
4. 提供完整的测试覆盖

### 3. 自定义配置
1. 扩展工厂函数参数
2. 添加配置数据结构
3. 实现配置验证
4. 提供配置示例

## 测试策略

### 1. 接口测试
- 验证接口函数指针不为NULL
- 测试接口契约遵守情况
- 验证错误处理行为

### 2. 工厂测试
- 测试工厂函数创建实例
- 验证配置参数有效性
- 测试资源管理正确性

### 3. 集成测试
- 测试接口与实现的集成
- 验证多实现兼容性
- 测试向后兼容性

## 性能考虑

### 1. 虚函数调用开销
- 函数指针调用有轻微开销
- 通过内联优化减少开销
- 关键路径考虑直接调用

### 2. 内存开销
- 接口结构占用固定内存
- 函数指针表增加内存使用
- 私有数据需要额外内存

### 3. 优化建议
- 生产环境使用轻量级实现
- 按需启用调试功能
- 使用条件编译优化

## 相关模块

### 1. 依赖模块
- 基础类型定义模块
- 错误处理模块
- 配置管理模块

### 2. 实现模块
- 泄漏检测实现模块
- 错误检测实现模块
- 分析实现模块

### 3. 工具模块
- 测试框架模块
- 性能分析模块
- 文档生成模块

---

*最后更新: 2026-01-08*
*版本: 2.0.0*
*维护者: CN_Language架构委员会*
