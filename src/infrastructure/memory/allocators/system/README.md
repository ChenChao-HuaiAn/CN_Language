# 系统分配器模块 (System Allocator)

## 概述

系统分配器是基于标准C库的内存分配器实现，提供基本的malloc/free/realloc功能。这是性能最高的分配器，适合生产环境使用。

## 功能特性

- **标准兼容**: 直接调用系统内存分配函数
- **内存对齐**: 支持任意对齐要求的内存分配
- **性能优化**: 无额外开销，性能接近原生系统调用
- **线程安全**: 使用系统提供的线程安全机制
- **统计信息**: 提供基本的内存使用统计

## 接口设计

### 主要接口函数

```c
/**
 * @brief 创建标准系统内存分配器
 * 
 * 创建使用标准C库malloc/free/realloc函数的系统分配器。
 * 此分配器性能最高，适合生产环境使用。
 * 
 * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针
 */
Stru_MemoryAllocatorInterface_t* F_create_system_allocator(void);

/**
 * @brief 获取全局系统分配器实例
 * 
 * 返回全局的系统分配器单例实例。
 * 如果尚未创建，则创建新的实例。
 * 
 * @return Stru_MemoryAllocatorInterface_t* 全局系统分配器实例
 */
Stru_MemoryAllocatorInterface_t* F_get_global_system_allocator(void);
```

### 接口实现

系统分配器实现了完整的`Stru_MemoryAllocatorInterface_t`接口：

1. **allocate**: 分配指定大小和对齐的内存
2. **reallocate**: 重新分配内存块
3. **deallocate**: 释放内存块
4. **get_statistics**: 获取内存使用统计信息
5. **validate**: 验证内存块有效性
6. **cleanup**: 清理分配器资源

## 使用示例

### 基本使用

```c
#include "allocators/system/CN_system_allocator.h"

// 获取系统分配器
Stru_MemoryAllocatorInterface_t* allocator = F_create_system_allocator();

// 分配内存
void* memory = allocator->allocate(allocator, 1024, 8);

// 使用内存...

// 重新分配内存
memory = allocator->reallocate(allocator, memory, 2048);

// 释放内存
allocator->deallocate(allocator, memory);
```

### 使用全局实例

```c
#include "allocators/system/CN_system_allocator.h"

// 获取全局系统分配器
Stru_MemoryAllocatorInterface_t* global_allocator = F_get_global_system_allocator();

// 使用全局分配器...
```

## 性能特点

### 优点
1. **高性能**: 直接系统调用，无额外开销
2. **稳定可靠**: 使用经过充分测试的系统内存管理
3. **兼容性好**: 支持所有标准C环境
4. **线程安全**: 系统级线程安全保证

### 局限性
1. **无调试功能**: 不提供内存泄漏检测等调试功能
2. **碎片问题**: 可能存在内存碎片问题
3. **统计有限**: 只能提供基本的统计信息

## 内存对齐

系统分配器支持任意对齐要求：

```c
// 分配16字节对齐的内存
void* aligned_memory = allocator->allocate(allocator, 256, 16);

// 分配默认对齐的内存（系统默认）
void* default_memory = allocator->allocate(allocator, 256, 0);
```

## 统计信息

系统分配器提供基本的内存使用统计：

```c
size_t total_allocated = 0;
size_t total_freed = 0;
size_t current_usage = 0;
size_t allocation_count = 0;

allocator->get_statistics(allocator, 
                         &total_allocated, &total_freed,
                         &current_usage, &allocation_count);
```

## 依赖关系

- **内部依赖**: 无
- **外部依赖**: C标准库 (stdlib.h)
- **架构层**: 基础设施层 - 内存管理模块

## 设计原则

### 单一职责原则
- 只负责系统内存分配功能
- 不包含调试或特殊功能

### 开闭原则
- 通过抽象接口支持扩展
- 实现稳定，接口开放

### 依赖倒置原则
- 实现标准内存分配器接口
- 高层模块通过接口使用

## 测试策略

- 单元测试覆盖所有接口函数
- 内存边界测试
- 对齐要求测试
- 多线程安全性测试
- 性能基准测试

## 注意事项

1. 生产环境推荐使用系统分配器
2. 调试阶段建议使用调试分配器
3. 确保分配和释放成对出现
4. 注意内存对齐对性能的影响

## 版本历史

- v1.0.0: 初始版本，基于标准C库实现
- v1.1.0: 添加内存对齐支持
- v1.2.0: 添加全局实例支持

## 维护者

CN_Language架构委员会
