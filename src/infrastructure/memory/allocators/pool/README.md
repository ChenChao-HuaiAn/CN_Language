# 对象池分配器模块 (Pool Allocator)

## 概述

对象池分配器是一种高效的内存分配策略，专门用于分配固定大小的对象。通过预分配内存块并重复使用，可以显著减少内存碎片和提高分配性能。

## 功能特性

- **固定大小分配**: 专门分配特定大小的对象
- **高效重用**: 释放的对象可以立即被重用
- **零内存碎片**: 避免内存碎片问题
- **快速分配/释放**: O(1)时间复杂度的分配和释放
- **线程安全**: 支持多线程环境（可选）
- **统计信息**: 详细的池使用统计

## 接口设计

### 主要接口函数

```c
/**
 * @brief 创建对象池分配器
 * 
 * 创建用于分配固定大小对象的对象池分配器。
 * 
 * @param object_size 每个对象的大小（字节）
 * @param pool_size 池中对象的数量
 * @param parent_allocator 父分配器（用于分配池内存）
 * @return Stru_MemoryAllocatorInterface_t* 对象池分配器接口指针
 */
Stru_MemoryAllocatorInterface_t* F_create_pool_allocator(
    size_t object_size, size_t pool_size,
    Stru_MemoryAllocatorInterface_t* parent_allocator);

/**
 * @brief 获取对象池分配器的统计信息
 * 
 * 获取对象池的详细统计信息，包括：
 * - 总对象数
 * - 已分配对象数
 * - 可用对象数
 * - 分配失败次数
 * 
 * @param allocator 对象池分配器
 * @param total_objects 输出参数：总对象数
 * @param allocated_objects 输出参数：已分配对象数
 * @param available_objects 输出参数：可用对象数
 * @param allocation_failures 输出参数：分配失败次数
 */
void F_get_pool_allocator_statistics(
    Stru_MemoryAllocatorInterface_t* allocator,
    size_t* total_objects, size_t* allocated_objects,
    size_t* available_objects, size_t* allocation_failures);

/**
 * @brief 扩展对象池容量
 * 
 * 扩展对象池的容量，增加可用对象数量。
 * 
 * @param allocator 对象池分配器
 * @param additional_objects 要添加的对象数量
 * @return bool 扩展成功返回true，失败返回false
 */
bool F_expand_pool_allocator(
    Stru_MemoryAllocatorInterface_t* allocator,
    size_t additional_objects);
```

### 接口实现

对象池分配器实现了完整的`Stru_MemoryAllocatorInterface_t`接口，专门优化固定大小对象的分配。

## 使用示例

### 基本使用

```c
#include "allocators/pool/CN_pool_allocator.h"

// 创建对象池分配器（每个对象256字节，池大小100个对象）
Stru_MemoryAllocatorInterface_t* pool_allocator = 
    F_create_pool_allocator(256, 100, NULL);

// 分配对象
void* obj1 = pool_allocator->allocate(pool_allocator, 256, 0);
void* obj2 = pool_allocator->allocate(pool_allocator, 256, 0);

// 使用对象...

// 释放对象
pool_allocator->deallocate(pool_allocator, obj1);
pool_allocator->deallocate(pool_allocator, obj2);

// 获取统计信息
size_t total_objects, allocated_objects, available_objects, failures;
F_get_pool_allocator_statistics(pool_allocator,
                               &total_objects, &allocated_objects,
                               &available_objects, &failures);

// 清理分配器
pool_allocator->cleanup(pool_allocator);
```

### 扩展池容量

```c
#include "allocators/pool/CN_pool_allocator.h"

// 创建对象池
Stru_MemoryAllocatorInterface_t* pool_allocator = 
    F_create_pool_allocator(128, 50, NULL);

// 当池满时扩展容量
if (!pool_allocator->allocate(pool_allocator, 128, 0)) {
    // 池已满，扩展50个对象
    F_expand_pool_allocator(pool_allocator, 50);
}
```

## 实现原理

### 内存布局

对象池使用连续的内存块，每个对象包含一个链表指针用于空闲列表：

```
+----------------------+
| 对象数据区域         |  <- 用户数据
+----------------------+
| 下一个空闲对象指针   |  <- 空闲链表（仅空闲对象使用）
+----------------------+
```

### 空闲列表管理

对象池维护一个空闲对象链表：
- **初始化**: 所有对象链接到空闲列表
- **分配**: 从空闲列表头部取出对象
- **释放**: 将对象放回空闲列表头部

### 性能特点

1. **分配**: O(1) - 从空闲列表头部取出
2. **释放**: O(1) - 放回空闲列表头部
3. **内存使用**: 固定大小，无外部碎片
4. **缓存友好**: 连续内存布局

## 配置选项

对象池分配器支持多种配置：

```c
// 配置结构（未来扩展）
typedef struct {
    size_t object_size;          // 对象大小
    size_t initial_pool_size;    // 初始池大小
    size_t expansion_size;       // 每次扩展的大小
    bool thread_safe;            // 是否线程安全
    bool zero_on_alloc;          // 分配时清零内存
    bool zero_on_free;           // 释放时清零内存
} PoolAllocatorConfig;
```

## 适用场景

### 适合使用对象池的场景
1. **频繁创建/销毁相同大小的对象**
2. **实时系统需要确定性的分配时间**
3. **避免内存碎片的场景**
4. **性能关键的分配操作**

### 不适合使用对象池的场景
1. **对象大小不固定**
2. **对象生命周期很长**
3. **内存使用模式不可预测**
4. **对象大小差异很大**

## 依赖关系

- **内部依赖**: 系统分配器模块（用于分配池内存）
- **外部依赖**: C标准库 (stdlib.h)
- **架构层**: 基础设施层 - 内存管理模块

## 设计原则

### 单一职责原则
- 只负责固定大小对象的分配
- 不处理变长内存分配

### 开闭原则
- 通过配置支持不同的池策略
- 支持扩展和收缩

### 依赖倒置原则
- 通过接口依赖父分配器
- 支持任意内存来源

## 测试策略

- 单元测试覆盖所有接口函数
- 性能基准测试
- 多线程安全性测试
- 内存边界测试
- 池扩展/收缩测试

## 注意事项

1. 对象池只适合固定大小的对象
2. 池大小需要合理预估
3. 对象释放后不会立即返回系统
4. 可能造成内存浪费（未使用的池空间）

## 性能优化

### 内存对齐
对象池自动处理内存对齐，确保每个对象正确对齐：

```c
// 自动计算对齐后的对象大小
size_t aligned_size = (object_size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
```

### 缓存优化
- 连续内存布局提高缓存命中率
- 预取机制减少缓存未命中
- 对象重用保持缓存热度

### 线程安全
可选线程安全实现：
- 使用互斥锁保护空闲列表
- 无锁实现（CAS操作）
- 线程本地缓存减少锁竞争

## 版本历史

- v1.0.0: 初始版本，基本对象池功能
- v1.1.0: 添加池扩展功能
- v1.2.0: 添加统计信息
- v1.3.0: 添加线程安全支持

## 维护者

CN_Language架构委员会
