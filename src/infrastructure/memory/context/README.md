# 内存上下文模块 (Memory Context)

## 概述

内存上下文模块提供了一种层次化的内存管理机制，允许将内存分配组织到逻辑上下文中。这种设计便于内存资源的统一管理和释放，特别适用于需要批量释放内存的场景。

## 核心概念

### 1. 内存上下文 (Memory Context)
内存上下文是一个逻辑容器，用于管理一组相关的内存分配。上下文可以形成层次结构，子上下文继承父上下文的内存管理策略。

### 2. 上下文生命周期
- **创建**: 创建新的内存上下文
- **切换**: 在当前线程中切换活动上下文
- **重置**: 释放上下文中的所有内存，但保留上下文本身
- **销毁**: 完全销毁上下文及其所有内存

### 3. 内存所有权
- 上下文拥有其分配的所有内存
- 上下文销毁时自动释放所有内存
- 支持内存所有权转移

## 模块功能

### 1. 上下文管理
- **上下文创建和销毁**: 创建新的内存上下文，安全销毁上下文
- **上下文切换**: 线程本地上下文切换
- **上下文查询**: 获取当前活动上下文信息

### 2. 内存分配
- **上下文感知分配**: 在特定上下文中分配内存
- **对齐分配**: 支持内存对齐的分配
- **批量分配**: 高效批量内存分配

### 3. 内存管理
- **内存释放**: 释放单个内存块或整个上下文
- **内存重置**: 快速重置上下文，重用内存池
- **内存统计**: 统计上下文内存使用情况

### 4. 上下文关系
- **父子关系**: 建立上下文层次结构
- **兄弟关系**: 同级上下文管理
- **上下文继承**: 子上下文继承父上下文属性

## 接口设计

### 核心接口
```c
// 内存上下文接口
typedef struct Stru_MemoryContextInterface_t {
    // 上下文管理
    Stru_MemoryContext_t* (*create)(const char* name, Stru_MemoryContext_t* parent);
    void (*destroy)(Stru_MemoryContext_t* context);
    void (*reset)(Stru_MemoryContext_t* context);
    
    // 上下文操作
    Stru_MemoryContext_t* (*get_current)(void);
    void (*set_current)(Stru_MemoryContext_t* context);
    Stru_MemoryContext_t* (*get_parent)(Stru_MemoryContext_t* context);
    
    // 内存分配
    void* (*allocate)(Stru_MemoryContext_t* context, size_t size, size_t alignment);
    void* (*reallocate)(Stru_MemoryContext_t* context, void* ptr, size_t new_size);
    void (*deallocate)(Stru_MemoryContext_t* context, void* ptr);
    
    // 内存管理
    size_t (*get_allocated_size)(Stru_MemoryContext_t* context, void* ptr);
    size_t (*get_total_allocated)(Stru_MemoryContext_t* context);
    size_t (*get_peak_allocated)(Stru_MemoryContext_t* context);
    
    // 上下文信息
    const char* (*get_name)(Stru_MemoryContext_t* context);
    uint64_t (*get_id)(Stru_MemoryContext_t* context);
    bool (*is_valid)(Stru_MemoryContext_t* context);
} Stru_MemoryContextInterface_t;
```

## 使用示例

### 1. 基本使用
```c
#include "CN_memory_context.h"

// 获取内存上下文接口
Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();

// 创建根上下文
Stru_MemoryContext_t* root_ctx = ctx_if->create("RootContext", NULL);

// 创建子上下文
Stru_MemoryContext_t* child_ctx = ctx_if->create("ChildContext", root_ctx);

// 在子上下文中分配内存
void* memory = ctx_if->allocate(child_ctx, 1024, 8);

// 使用内存...

// 销毁子上下文（自动释放所有内存）
ctx_if->destroy(child_ctx);

// 销毁根上下文
ctx_if->destroy(root_ctx);
```

### 2. 上下文切换
```c
#include "CN_memory_context.h"

// 获取内存上下文接口
Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();

// 创建多个上下文
Stru_MemoryContext_t* ctx1 = ctx_if->create("Context1", NULL);
Stru_MemoryContext_t* ctx2 = ctx_if->create("Context2", NULL);

// 切换到上下文1
ctx_if->set_current(ctx1);
void* mem1 = ctx_if->allocate(NULL, 512, 16); // 在当前上下文分配

// 切换到上下文2
ctx_if->set_current(ctx2);
void* mem2 = ctx_if->allocate(NULL, 256, 8); // 在当前上下文分配

// 获取当前上下文
Stru_MemoryContext_t* current = ctx_if->get_current();

// 清理
ctx_if->destroy(ctx1);
ctx_if->destroy(ctx2);
```

### 3. 内存统计
```c
#include "CN_memory_context.h"
#include <stdio.h>

// 获取内存上下文接口
Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();

// 创建上下文
Stru_MemoryContext_t* ctx = ctx_if->create("StatsContext", NULL);

// 分配一些内存
void* mem1 = ctx_if->allocate(ctx, 100, 8);
void* mem2 = ctx_if->allocate(ctx, 200, 16);
void* mem3 = ctx_if->allocate(ctx, 300, 32);

// 获取统计信息
size_t total = ctx_if->get_total_allocated(ctx);
size_t peak = ctx_if->get_peak_allocated(ctx);

printf("Total allocated: %zu bytes\n", total);
printf("Peak allocated: %zu bytes\n", peak);

// 清理
ctx_if->destroy(ctx);
```

## 依赖关系

- **内部依赖**: 内存分配器模块
- **外部依赖**: C标准库 (stdlib.h, string.h, stdint.h)
- **架构层**: 基础设施层

## 设计原则

### 1. 单一职责原则
- 上下文只负责内存管理
- 分配器负责实际内存分配
- 清晰的职责分离

### 2. 开闭原则
- 通过接口支持新的上下文类型
- 现有代码对扩展开放，对修改封闭

### 3. 里氏替换原则
- 所有上下文实现可互换使用
- 接口契约一致
- 行为可预测

## 性能考虑

### 1. 快速路径优化
- 常见操作路径优化
- 内联小型函数
- 缓存友好数据结构

### 2. 内存池管理
- 预分配内存池减少系统调用
- 内存重用减少碎片
- 批量操作优化

### 3. 线程安全性
- 线程本地上下文
- 无锁数据结构
- 细粒度锁策略

## 安全特性

### 1. 内存安全
- 边界检查防止溢出
- 双重释放检测
- 无效指针检测

### 2. 上下文隔离
- 上下文间内存隔离
- 访问权限控制
- 内存泄漏检测

### 3. 错误恢复
- 优雅的错误处理
- 资源清理保证
- 状态一致性维护

## 使用场景

### 1. 临时内存管理
- 函数调用期间的临时分配
- 栈式内存管理
- 自动清理

### 2. 对象生命周期管理
- 相关对象的统一管理
- 批量创建和销毁
- 内存所有权明确

### 3. 资源受限环境
- 内存使用限制
- 内存使用监控
- 内存回收策略

## 扩展性

### 1. 自定义分配策略
- 可替换分配器实现
- 自定义内存池
- 特殊内存类型支持

### 2. 监控和调试
- 内存使用监控
- 泄漏检测集成
- 性能分析支持

### 3. 平台适配
- 不同平台优化
- 特殊硬件支持
- 操作系统集成

## 注意事项

1. 上下文销毁会释放所有内存
2. 线程本地上下文需要线程安全考虑
3. 上下文层次不宜过深
4. 内存统计可能有性能开销

## 版本历史

- v1.0.0: 初始版本，基本上下文管理
- v1.1.0: 添加线程本地上下文支持
- v1.2.0: 添加内存统计功能
- v1.3.0: 优化性能，添加内存池

## 维护者

CN_Language架构委员会
