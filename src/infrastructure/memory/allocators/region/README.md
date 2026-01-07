# 区域分配器 (Region Allocator)

## 概述

区域分配器是一种特殊的内存分配器，它一次性分配一大块内存（区域），然后从该区域中分配小内存块。这种分配器适用于临时内存分配场景，可以显著提高内存分配效率，减少内存碎片。

## 设计原理

### 区域式内存管理

区域分配器采用区域式内存管理策略：
1. **一次性分配**：在创建时分配一大块连续内存
2. **线性分配**：从区域起始位置开始线性分配内存
3. **批量释放**：通过重置操作一次性释放所有内存
4. **高效重用**：重置后可以重用整个区域

### 适用场景

- **临时内存分配**：函数调用期间的临时变量
- **批量操作**：需要大量临时内存的操作
- **性能关键路径**：需要快速内存分配的场景
- **内存池**：作为其他分配器的底层实现

## 接口设计

### 核心接口

```c
// 创建区域分配器
Stru_MemoryAllocatorInterface_t* F_create_region_allocator(
    size_t region_size,
    Stru_MemoryAllocatorInterface_t* parent_allocator);

// 获取统计信息
void F_get_region_allocator_statistics(
    Stru_MemoryAllocatorInterface_t* allocator,
    size_t* total_size, size_t* used_size,
    size_t* available_size, size_t* allocation_count);

// 重置区域分配器
void F_reset_region_allocator(Stru_MemoryAllocatorInterface_t* allocator);

// 扩展区域容量
bool F_expand_region_allocator(
    Stru_MemoryAllocatorInterface_t* allocator,
    size_t additional_size);
```

### 接口特点

1. **不支持单独释放**：区域分配器不支持单独释放内存块，只能通过`reset`操作一次性释放所有内存
2. **支持对齐分配**：支持任意对齐要求的分配
3. **自动扩展**：当区域空间不足时，可以自动扩展区域容量
4. **统计信息**：提供详细的统计信息，包括总大小、已使用大小、可用大小等

## 使用示例

### 基本使用

```c
#include "CN_region_allocator.h"
#include "allocators/system/CN_system_allocator.h"

// 创建系统分配器作为父分配器
Stru_MemoryAllocatorInterface_t* system_allocator = F_create_system_allocator();

// 创建区域分配器（区域大小1MB）
Stru_MemoryAllocatorInterface_t* region_allocator = F_create_region_allocator(
    1024 * 1024, system_allocator);

// 分配内存
void* ptr1 = region_allocator->allocate(region_allocator, 100, 0);
void* ptr2 = region_allocator->allocate(region_allocator, 200, 8); // 8字节对齐

// 获取统计信息
size_t total_size = 0, used_size = 0, available_size = 0, allocation_count = 0;
F_get_region_allocator_statistics(region_allocator, &total_size, &used_size,
                                 &available_size, &allocation_count);

printf("区域统计:\n");
printf("  总大小: %zu bytes\n", total_size);
printf("  已使用: %zu bytes\n", used_size);
printf("  可用:   %zu bytes\n", available_size);
printf("  分配次数: %zu\n", allocation_count);

// 重置区域（释放所有内存）
F_reset_region_allocator(region_allocator);

// 扩展区域容量
bool expanded = F_expand_region_allocator(region_allocator, 512 * 1024);

// 清理分配器
region_allocator->cleanup(region_allocator);
```

### 嵌套使用

```c
// 区域分配器可以作为其他分配器的父分配器
Stru_MemoryAllocatorInterface_t* region_allocator = F_create_region_allocator(
    1024 * 1024, NULL);  // 使用系统默认分配器

// 创建调试分配器，使用区域分配器作为父分配器
Stru_MemoryAllocatorInterface_t* debug_allocator = F_create_debug_allocator(
    region_allocator);

// 现在所有调试分配器的内存都来自区域分配器
```

## 性能特点

### 优势

1. **高性能**：分配操作是O(1)时间复杂度，只需要移动指针
2. **低碎片**：内存连续分配，减少内存碎片
3. **缓存友好**：连续内存布局有利于CPU缓存
4. **批量释放**：一次性释放所有内存，避免逐个释放的开销

### 限制

1. **不支持单独释放**：不能单独释放某个内存块
2. **内存浪费**：如果分配大小变化大，可能会有内部碎片
3. **固定生命周期**：所有内存块的生命周期相同

## 实现细节

### 内存布局

```
+-------------------+-------------------+-------------------+
| 已分配内存块1     | 已分配内存块2     | 空闲空间          |
+-------------------+-------------------+-------------------+
^                   ^                   ^                   ^
起始位置            偏移1               偏移2               区域结束
```

### 对齐处理

区域分配器支持任意对齐要求：
1. 计算当前偏移的对齐偏移
2. 确保对齐后的偏移满足对齐要求
3. 从对齐偏移处开始分配内存

### 扩展机制

当区域空间不足时：
1. 尝试扩展区域容量
2. 重新分配更大的内存块
3. 复制原有数据（如果需要）
4. 更新内部指针

## 测试策略

### 单元测试

1. **基本分配测试**：测试分配功能是否正常
2. **对齐分配测试**：测试不同对齐要求的分配
3. **统计信息测试**：测试统计信息是否正确
4. **重置功能测试**：测试重置功能是否正常
5. **扩展功能测试**：测试扩展功能是否正常

### 集成测试

1. **与其他分配器集成**：测试作为其他分配器的父分配器
2. **性能测试**：测试分配性能是否符合预期
3. **内存泄漏测试**：确保没有内存泄漏

## 配置选项

### 编译选项

```makefile
# 启用区域分配器
ENABLE_REGION_ALLOCATOR = 1

# 默认区域大小（字节）
DEFAULT_REGION_SIZE = 1048576  # 1MB

# 扩展因子（当空间不足时扩展的比例）
REGION_EXPANSION_FACTOR = 2.0
```

### 运行时配置

```c
// 创建时指定区域大小
Stru_MemoryAllocatorInterface_t* allocator = F_create_region_allocator(
    custom_size, parent_allocator);

// 运行时扩展
F_expand_region_allocator(allocator, additional_size);
```

## 最佳实践

### 使用建议

1. **合理设置区域大小**：根据实际需求设置合适的区域大小
2. **批量操作**：适合批量分配和释放的场景
3. **临时内存**：适合函数调用期间的临时内存分配
4. **性能关键路径**：在性能关键路径上使用区域分配器

### 避免的用法

1. **长期内存**：不适合长期持有的内存分配
2. **单独释放**：需要单独释放内存的场景
3. **大小变化大**：分配大小变化很大的场景

## 扩展性

### 自定义扩展

可以通过以下方式扩展区域分配器：

1. **自定义内存管理策略**：实现不同的内存分配算法
2. **添加调试功能**：添加内存调试和错误检测
3. **支持多线程**：添加线程安全支持
4. **集成性能监控**：添加性能监控和统计

### 插件架构

区域分配器支持插件架构：
1. **可替换的父分配器**：可以使用任何实现了`Stru_MemoryAllocatorInterface_t`接口的分配器作为父分配器
2. **可扩展的统计信息**：可以添加更多的统计信息
3. **可配置的行为**：可以通过配置改变分配器的行为

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-01-07 | 初始版本，基本区域分配功能 |
| 1.1.0 | 2026-01-07 | 添加对齐支持和扩展功能 |

## 相关文档

- [内存管理模块总文档](../README.md)
- [系统分配器文档](../system/README.md)
- [调试分配器文档](../debug/README.md)
- [对象池分配器文档](../pool/README.md)
- [API文档](../../../../docs/api/infrastructure/memory/CN_memory.md)

## 维护者

CN_Language架构委员会
