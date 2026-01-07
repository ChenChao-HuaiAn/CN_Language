# 调试分配器模块 (Debug Allocator)

## 概述

调试分配器是具有调试功能的内存分配器，提供内存泄漏检测、缓冲区溢出检查、分配跟踪等调试功能。适合开发和测试阶段使用。

## 功能特性

- **内存泄漏检测**: 自动跟踪所有分配，检测未释放的内存
- **缓冲区溢出检查**: 使用哨兵值检测缓冲区溢出
- **分配统计**: 详细的分配统计信息
- **魔术字验证**: 使用魔术字验证内存块完整性
- **双重释放检测**: 检测重复释放同一内存块
- **未初始化内存检测**: 检测使用未初始化的内存

## 接口设计

### 主要接口函数

```c
/**
 * @brief 创建调试内存分配器
 * 
 * 创建具有调试功能的内存分配器，包括：
 * - 内存泄漏检测
 * - 缓冲区溢出检查
 * - 分配统计信息
 * - 魔术字验证
 * 
 * @param parent_allocator 父分配器（可为NULL，使用系统分配器）
 * @return Stru_MemoryAllocatorInterface_t* 调试分配器接口指针
 */
Stru_MemoryAllocatorInterface_t* F_create_debug_allocator(
    Stru_MemoryAllocatorInterface_t* parent_allocator);

/**
 * @brief 获取调试分配器的统计信息
 * 
 * 获取调试分配器的详细统计信息，包括：
 * - 总分配字节数
 * - 总释放字节数
 * - 当前使用字节数
 * - 分配次数
 * 
 * @param allocator 调试分配器
 * @param total_allocated 输出参数：总分配字节数
 * @param total_freed 输出参数：总释放字节数
 * @param current_usage 输出参数：当前使用字节数
 * @param allocation_count 输出参数：分配次数
 */
void F_get_debug_allocator_statistics(
    Stru_MemoryAllocatorInterface_t* allocator,
    size_t* total_allocated, size_t* total_freed,
    size_t* current_usage, size_t* allocation_count);

/**
 * @brief 检查调试分配器中的内存泄漏
 * 
 * 检查调试分配器中是否存在未释放的内存。
 * 返回泄漏的字节数，0表示无泄漏。
 * 
 * @param allocator 调试分配器
 * @return size_t 泄漏的字节数
 */
size_t F_check_debug_allocator_leaks(Stru_MemoryAllocatorInterface_t* allocator);
```

### 接口实现

调试分配器实现了完整的`Stru_MemoryAllocatorInterface_t`接口，并添加了额外的调试功能。

## 使用示例

### 基本使用

```c
#include "allocators/debug/CN_debug_allocator.h"

// 创建调试分配器（使用系统分配器作为父分配器）
Stru_MemoryAllocatorInterface_t* debug_allocator = 
    F_create_debug_allocator(NULL);

// 分配内存（带调试信息）
void* memory = debug_allocator->allocate(debug_allocator, 512, 8);

// 验证内存有效性
bool valid = debug_allocator->validate(debug_allocator, memory);

// 使用内存...

// 检查内存泄漏
size_t leaks = F_check_debug_allocator_leaks(debug_allocator);
if (leaks > 0) {
    printf("检测到内存泄漏: %zu 字节\n", leaks);
}

// 释放内存
debug_allocator->deallocate(debug_allocator, memory);

// 清理分配器
debug_allocator->cleanup(debug_allocator);
```

### 使用自定义父分配器

```c
#include "allocators/debug/CN_debug_allocator.h"
#include "allocators/system/CN_system_allocator.h"

// 创建系统分配器
Stru_MemoryAllocatorInterface_t* system_allocator = 
    F_create_system_allocator();

// 创建调试分配器，包装系统分配器
Stru_MemoryAllocatorInterface_t* debug_allocator = 
    F_create_debug_allocator(system_allocator);

// 使用调试分配器...
```

## 调试功能详解

### 内存布局

调试分配器为每个分配添加额外的调试信息：

```
+----------------------+
| 分配大小 (size_t)    |  <- 存储原始分配大小
+----------------------+
| 魔术字 (size_t)      |  <- 0xDEADBEEF，用于验证
+----------------------+
| 用户数据区域         |  <- 返回给用户的指针
+----------------------+
| 哨兵值 (可选)        |  <- 检测缓冲区溢出
+----------------------+
```

### 内存泄漏检测

调试分配器跟踪所有活动分配，可以检测：

1. **未释放的内存**: 程序退出时仍有分配未释放
2. **分配统计**: 每个分配的大小、位置等信息
3. **泄漏报告**: 生成详细的泄漏报告

### 缓冲区溢出检查

在每个分配前后添加哨兵值，检测：

1. **前溢出**: 在分配开始前写入数据
2. **后溢出**: 在分配结束后写入数据
3. **哨兵值损坏**: 哨兵值被意外修改

## 性能特点

### 优点
1. **全面的调试功能**: 提供完整的调试信息
2. **错误检测**: 自动检测常见内存错误
3. **诊断信息**: 详细的错误报告和诊断
4. **可配置性**: 可以启用/禁用特定调试功能

### 局限性
1. **性能开销**: 额外的内存和时间开销
2. **内存使用**: 每个分配需要额外空间存储调试信息
3. **生产环境不适用**: 仅用于开发和测试

## 配置选项

调试分配器支持多种配置选项：

```c
// 未来扩展：可以通过配置结构启用/禁用特定功能
typedef struct {
    bool enable_leak_detection;      // 启用内存泄漏检测
    bool enable_bounds_checking;     // 启用边界检查
    bool enable_magic_validation;    // 启用魔术字验证
    bool enable_statistics;          // 启用统计信息
    size_t guard_zone_size;          // 哨兵区域大小
} DebugAllocatorConfig;
```

## 依赖关系

- **内部依赖**: 系统分配器模块（作为默认父分配器）
- **外部依赖**: C标准库 (stdlib.h, string.h)
- **架构层**: 基础设施层 - 内存管理模块

## 设计原则

### 单一职责原则
- 只负责内存调试功能
- 不包含实际的内存分配逻辑（委托给父分配器）

### 开闭原则
- 通过装饰器模式扩展分配器功能
- 支持添加新的调试功能

### 依赖倒置原则
- 通过接口依赖父分配器
- 支持任意类型的父分配器

## 测试策略

- 单元测试覆盖所有接口函数
- 内存泄漏检测测试
- 缓冲区溢出检测测试
- 魔术字验证测试
- 性能影响测试

## 注意事项

1. 仅用于开发和测试环境
2. 生产环境应使用系统分配器
3. 调试功能会增加内存使用
4. 可能影响程序性能

## 版本历史

- v1.0.0: 初始版本，基本调试功能
- v1.1.0: 添加内存泄漏检测
- v1.2.0: 添加缓冲区溢出检查
- v1.3.0: 添加分配统计功能

## 维护者

CN_Language架构委员会
