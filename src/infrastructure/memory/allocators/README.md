# 内存分配器模块 (Allocators)

## 概述

内存分配器模块是CN_Language项目基础设施层的一部分，提供了多种内存分配策略的实现。该模块遵循单一职责原则，每个分配器专注于特定的内存管理策略，并通过统一的抽象接口提供一致的使用体验。

## 模块架构

```
allocators/
├── system/           # 系统分配器 - 基于标准C库
├── debug/            # 调试分配器 - 内存调试和错误检测
├── pool/             # 对象池分配器 - 固定大小对象高效分配
├── region/           # 区域分配器 - 区域式内存管理
└── factory/          # 分配器工厂 - 预留（未来扩展）
```

## 子模块详解

### 1. 系统分配器 (System Allocator)
- **路径**: `system/`
- **文件**: `CN_system_allocator.h`, `CN_system_allocator.c`
- **功能**: 基于标准C库的内存分配器，提供基本的malloc/free功能
- **特点**: 
  - 直接调用系统内存分配函数
  - 提供内存对齐分配支持
  - 支持内存统计功能
  - 高性能，适合生产环境

### 2. 调试分配器 (Debug Allocator)
- **路径**: `debug/`
- **文件**: `CN_debug_allocator.h`, `CN_debug_allocator.c`
- **功能**: 用于调试的内存分配器，检测内存错误
- **特点**:
  - 内存边界检查（前后哨兵值）
  - 内存泄漏检测
  - 双重释放检测
  - 未初始化内存检测
  - 魔术字验证

### 3. 对象池分配器 (Pool Allocator)
- **路径**: `pool/`
- **文件**: `CN_pool_allocator.h`, `CN_pool_allocator.c`
- **功能**: 固定大小对象的高效分配器
- **特点**:
  - 零内存碎片
  - O(1)时间复杂度的分配/释放
  - 对象重用机制
  - 线程安全支持（可选）

### 4. 区域分配器 (Region Allocator)
- **路径**: `region/`
- **文件**: `CN_region_allocator.h`, `CN_region_allocator.c`
- **功能**: 区域式内存管理，一次性分配和释放
- **特点**:
  - 一次性分配大块内存区域
  - 从区域中线性分配小内存块
  - 支持对齐分配
  - 支持区域扩展
  - 通过reset操作一次性释放所有内存
  - 适用于临时内存分配场景

### 5. 分配器工厂 (Allocator Factory) - 预留
- **路径**: `factory/`
- **状态**: 预留，未来扩展
- **功能**: 分配器创建和管理工厂

## 统一接口设计

### 抽象接口模式
所有分配器都实现统一的`Stru_MemoryAllocatorInterface_t`接口，支持运行时替换：

```c
typedef struct Stru_MemoryAllocatorInterface_t {
    void* (*allocate)(Stru_MemoryAllocatorInterface_t* allocator, 
                     size_t size, size_t alignment);
    void* (*reallocate)(Stru_MemoryAllocatorInterface_t* allocator,
                       void* ptr, size_t new_size);
    void (*deallocate)(Stru_MemoryAllocatorInterface_t* allocator, void* ptr);
    void (*get_statistics)(Stru_MemoryAllocatorInterface_t* allocator,
                          size_t* total_allocated, size_t* total_freed,
                          size_t* current_usage, size_t* allocation_count);
    bool (*validate)(Stru_MemoryAllocatorInterface_t* allocator, void* ptr);
    void (*cleanup)(Stru_MemoryAllocatorInterface_t* allocator);
    void* private_data;
} Stru_MemoryAllocatorInterface_t;
```

## 使用示例

### 1. 使用系统分配器
```c
#include "allocators/system/CN_system_allocator.h"

// 获取系统分配器接口
Stru_MemoryAllocatorInterface_t* allocator = F_create_system_allocator();

// 分配内存
void* memory = allocator->allocate(allocator, 1024, 8);

// 使用内存...

// 释放内存
allocator->deallocate(allocator, memory);
```

### 2. 使用调试分配器
```c
#include "allocators/debug/CN_debug_allocator.h"

// 获取调试分配器接口
Stru_MemoryAllocatorInterface_t* debug_allocator = 
    F_create_debug_allocator(NULL);

// 分配内存（带调试信息）
void* debug_memory = debug_allocator->allocate(debug_allocator, 512, 16);

// 验证内存有效性
bool valid = debug_allocator->validate(debug_allocator, debug_memory);

// 释放内存
debug_allocator->deallocate(debug_allocator, debug_memory);
```

### 3. 使用对象池分配器
```c
#include "allocators/pool/CN_pool_allocator.h"

// 创建对象池分配器（每个对象256字节，池大小100）
Stru_MemoryAllocatorInterface_t* pool_allocator = 
    F_create_pool_allocator(256, 100, NULL);

// 分配对象
void* obj = pool_allocator->allocate(pool_allocator, 256, 0);

// 使用对象...

// 释放对象
pool_allocator->deallocate(pool_allocator, obj);
```

### 4. 使用区域分配器
```c
#include "allocators/region/CN_region_allocator.h"

// 创建区域分配器（初始区域大小1MB，使用系统分配器作为父分配器）
Stru_MemoryAllocatorInterface_t* region_allocator = 
    F_create_region_allocator(1024 * 1024, NULL);

// 分配内存
void* memory1 = region_allocator->allocate(region_allocator, 256, 8);
void* memory2 = region_allocator->allocate(region_allocator, 512, 16);

// 使用内存...

// 重置区域（一次性释放所有内存）
region_allocator->cleanup(region_allocator);

// 重新分配内存
void* memory3 = region_allocator->allocate(region_allocator, 1024, 0);
```

## 依赖关系

### 内部依赖
- **系统分配器**: 无内部依赖
- **调试分配器**: 依赖系统分配器（作为父分配器）
- **对象池分配器**: 依赖系统分配器（用于分配池内存）
- **区域分配器**: 依赖系统分配器（用于分配区域内存）

### 外部依赖
- C标准库 (stdlib.h, string.h)
- 标准类型定义 (stddef.h, stdbool.h)

### 架构层
- **层级**: 基础设施层
- **依赖方向**: 单向依赖（上层模块可依赖本模块）

## 设计原则

### 1. 单一职责原则
- 每个分配器只负责一种内存分配策略
- 接口清晰，功能明确
- 每个.c文件不超过500行，每个函数不超过50行

### 2. 开闭原则
- 通过抽象接口支持新的分配器类型
- 现有代码对扩展开放，对修改封闭
- 支持插件式架构

### 3. 依赖倒置原则
- 高层模块定义接口
- 低层模块实现接口
- 通过依赖注入配置分配器

### 4. 接口隔离原则
- 为不同客户端提供专用接口
- 避免"胖接口"
- 通过接口组合提供灵活的功能集

### 5. 里氏替换原则
- 接口实现必须完全遵守契约
- 子类可无缝替换父类
- 通过单元测试验证替换性

## 性能考虑

### 分配器性能对比
| 分配器类型 | 分配性能 | 释放性能 | 内存开销 | 适用场景 |
|------------|----------|----------|----------|----------|
| 系统分配器 | 高 | 高 | 低 | 生产环境，通用分配 |
| 调试分配器 | 中 | 中 | 高 | 开发测试，错误检测 |
| 对象池分配器 | 极高 | 极高 | 中 | 固定大小对象，频繁分配 |
| 区域分配器 | 极高 | 极高（reset） | 低 | 临时内存，批量分配 |

### 内存对齐
所有分配器都支持内存对齐要求：
- 系统分配器：使用系统默认对齐
- 调试分配器：支持任意对齐
- 对象池分配器：自动处理对象对齐
- 区域分配器：支持任意对齐要求

### 线程安全
- 系统分配器：依赖系统线程安全机制
- 调试分配器：可选线程安全实现
- 对象池分配器：可选线程安全实现
- 区域分配器：单线程使用（区域内部状态不保证线程安全）

## 测试策略

### 测试金字塔
```
        ┌─────────────────┐
        │   端到端测试     │ (少量)
        └─────────────────┘
                │
        ┌─────────────────┐
        │   集成测试       │ (中等)
        └─────────────────┘
                │
        ┌─────────────────┐
        │   单元测试       │ (大量)
        └─────────────────┘
```

### 测试覆盖率目标
- **语句覆盖率**: ≥85%
- **分支覆盖率**: ≥70%
- **函数覆盖率**: ≥90%
- **行覆盖率**: ≥85%

### 测试类型
1. **单元测试**: 覆盖所有公开接口，使用Unity测试框架
2. **集成测试**: 测试分配器组合和协作
3. **性能测试**: 建立关键路径的性能基准
4. **压力测试**: 大量分配/释放操作测试
5. **多线程测试**: 并发环境下的正确性测试

## 扩展性

### 添加新的分配器类型
可以通过实现`Stru_MemoryAllocatorInterface_t`接口来添加新的分配器类型：

1. **创建新的子模块目录**
2. **实现接口函数**
3. **编写单元测试**
4. **更新文档**
5. **集成到构建系统**

### 支持的扩展类型
- 线程本地分配器
- 堆栈分配器（stack allocator）
- 自定义内存管理策略
- 内存映射文件分配器
- 分配器工厂（统一创建和管理）

## 构建和编译

### 编译选项
```makefile
# 启用调试分配器
ENABLE_DEBUG_ALLOCATOR = 1

# 启用对象池分配器
ENABLE_POOL_ALLOCATOR = 1

# 启用区域分配器
ENABLE_REGION_ALLOCATOR = 1

# 启用线程安全
ENABLE_THREAD_SAFE = 0
```

### 对象文件位置
所有编译中途产生的.o文件放在build目录里对应的文件夹里：
```
build/
├── infrastructure/
│   └── memory/
│       └── allocators/
│           ├── system/
│           ├── debug/
│           ├── pool/
│           └── region/
```

## 文档要求

每个子模块必须包含：
1. `README.md` - 模块概述和使用说明
2. 代码中的Doxygen注释 - API文档
3. 示例代码 - 使用示例

## 版本管理

### 语义化版本
- **主版本号**: 不兼容的API修改
- **次版本号**: 向下兼容的功能性新增
- **修订号**: 向下兼容的问题修正

### 兼容性保证
- 公共API一旦发布，保持向后兼容
- 废弃的API需要标记，至少保留两个主版本
- 使用版本锁定文件确保构建一致性

## 注意事项

1. **调试分配器**会显著增加内存开销，仅用于开发和测试阶段
2. **对象池分配器**只适合固定大小的对象
3. **生产环境**建议使用系统分配器
4. 确保分配和释放成对出现
5. 注意内存对齐对性能的影响

## 版本历史

- **v1.0.0**: 初始版本，包含系统和调试分配器
- **v1.1.0**: 模块化重构，添加子模块结构
- **v1.2.0**: 添加对象池分配器
- **v1.3.0**: 添加区域分配器，完善文档和测试

## 维护者

CN_Language架构委员会

## 参考文档

- [系统分配器文档](system/README.md)
- [调试分配器文档](debug/README.md)
- [对象池分配器文档](pool/README.md)
- [API文档](../../../docs/api/infrastructure/memory/CN_memory.md)
