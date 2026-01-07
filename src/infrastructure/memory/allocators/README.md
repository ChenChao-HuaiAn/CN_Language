# 内存分配器模块 (Allocators)

## 概述

内存分配器模块是CN_Language项目基础设施层的一部分，提供了多种内存分配策略的实现。该模块遵循单一职责原则，每个分配器专注于特定的内存管理策略。

## 模块结构

### 1. 系统分配器 (System Allocator)
- **文件**: `CN_system_allocator.h`, `CN_system_allocator.c`
- **功能**: 基于标准C库的内存分配器，提供基本的malloc/free功能
- **特点**: 
  - 直接调用系统内存分配函数
  - 提供内存对齐分配支持
  - 支持内存统计功能

### 2. 调试分配器 (Debug Allocator)
- **文件**: `CN_debug_allocator.h`, `CN_debug_allocator.c`
- **功能**: 用于调试的内存分配器，检测内存错误
- **特点**:
  - 内存边界检查（前后哨兵值）
  - 内存泄漏检测
  - 双重释放检测
  - 未初始化内存检测

## 接口设计

### 抽象接口模式
所有分配器都实现统一的`Stru_AllocatorInterface_t`接口，支持运行时替换：

```c
typedef struct Stru_AllocatorInterface_t {
    void* (*allocate)(size_t size, size_t alignment);
    void* (*reallocate)(void* ptr, size_t new_size);
    void (*deallocate)(void* ptr);
    size_t (*get_allocated_size)(void* ptr);
    bool (*validate)(void* ptr);
} Stru_AllocatorInterface_t;
```

## 使用示例

### 1. 使用系统分配器
```c
#include "CN_system_allocator.h"

// 获取系统分配器接口
Stru_AllocatorInterface_t* allocator = F_get_system_allocator();

// 分配内存
void* memory = allocator->allocate(1024, 8);

// 使用内存...

// 释放内存
allocator->deallocate(memory);
```

### 2. 使用调试分配器
```c
#include "CN_debug_allocator.h"

// 获取调试分配器接口
Stru_AllocatorInterface_t* debug_allocator = F_get_debug_allocator();

// 分配内存（带调试信息）
void* debug_memory = debug_allocator->allocate(512, 16);

// 验证内存有效性
bool valid = debug_allocator->validate(debug_memory);

// 释放内存
debug_allocator->deallocate(debug_memory);
```

## 依赖关系

- **内部依赖**: 无
- **外部依赖**: C标准库 (stdlib.h, string.h)
- **架构层**: 基础设施层

## 设计原则

### 1. 单一职责原则
- 每个分配器只负责一种内存分配策略
- 接口清晰，功能明确

### 2. 开闭原则
- 通过抽象接口支持新的分配器类型
- 现有代码对扩展开放，对修改封闭

### 3. 依赖倒置原则
- 高层模块定义接口
- 低层模块实现接口
- 通过依赖注入配置分配器

## 性能考虑

1. **系统分配器**: 高性能，直接系统调用
2. **调试分配器**: 较低性能，但提供全面的错误检测
3. **内存对齐**: 支持任意对齐要求
4. **内存碎片**: 提供内存碎片统计

## 测试策略

- 单元测试覆盖所有接口函数
- 内存边界测试
- 压力测试（大量分配/释放）
- 多线程安全性测试

## 扩展性

可以通过实现`Stru_AllocatorInterface_t`接口来添加新的分配器类型：
- 对象池分配器
- 区域分配器
- 线程本地分配器
- 自定义内存管理策略

## 注意事项

1. 调试分配器会显著增加内存开销
2. 生产环境建议使用系统分配器
3. 调试分配器仅用于开发和测试阶段
4. 确保分配和释放成对出现

## 版本历史

- v1.0.0: 初始版本，包含系统和调试分配器
- v1.1.0: 添加内存对齐支持
- v1.2.0: 添加内存统计功能

## 维护者

CN_Language架构委员会
