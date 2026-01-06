# 内存管理模块 API 文档

## 概述

内存管理模块是CN_Language项目的基础设施层核心组件，提供统一、可扩展的内存管理接口。本模块遵循SOLID设计原则，支持多种内存分配策略，并包含调试功能。

## 设计原则

1. **单一职责原则**：每个分配器只负责一种内存管理策略
2. **开闭原则**：通过抽象接口支持新的分配器类型
3. **依赖倒置原则**：高层模块通过抽象接口使用内存服务
4. **接口隔离原则**：为不同客户端提供专用接口

## 核心接口

### Stru_MemoryAllocatorInterface_t

内存分配器抽象接口，定义了内存管理的基本操作。

```c
typedef struct Stru_MemoryAllocatorInterface_t Stru_MemoryAllocatorInterface_t;

struct Stru_MemoryAllocatorInterface_t
{
    void* (*allocate)(Stru_MemoryAllocatorInterface_t* allocator, size_t size, size_t alignment);
    void* (*reallocate)(Stru_MemoryAllocatorInterface_t* allocator, void* ptr, size_t new_size);
    void (*deallocate)(Stru_MemoryAllocatorInterface_t* allocator, void* ptr);
    void (*get_statistics)(Stru_MemoryAllocatorInterface_t* allocator,
                          size_t* total_allocated, size_t* total_freed, 
                          size_t* current_usage, size_t* allocation_count);
    bool (*validate)(Stru_MemoryAllocatorInterface_t* allocator, void* ptr);
    void (*cleanup)(Stru_MemoryAllocatorInterface_t* allocator);
    void* private_data;
};
```

### Stru_MemoryDebugInterface_t

内存调试接口，提供内存调试和诊断功能。

```c
typedef struct Stru_MemoryDebugInterface_t
{
    void (*enable_debugging)(bool enable_leak_detection, 
                            bool enable_bounds_checking, 
                            bool enable_tracking);
    size_t (*check_leaks)(void);
    void (*get_usage_report)(char* report_buffer, size_t buffer_size);
    void (*set_allocation_failure_callback)(void (*callback)(size_t requested_size));
    void (*dump_allocations)(void);
} Stru_MemoryDebugInterface_t;
```

### Stru_MemoryContext_t

内存管理上下文，包含完整的分配器和调试器。

```c
typedef struct Stru_MemoryContext_t
{
    Stru_MemoryAllocatorInterface_t* allocator;  ///< 内存分配器接口
    Stru_MemoryDebugInterface_t* debugger;       ///< 内存调试器接口
    void* private_data;                          ///< 私有数据
} Stru_MemoryContext_t;
```

## API 函数

### 分配器创建函数

#### `F_create_system_allocator`

创建标准系统内存分配器。

```c
Stru_MemoryAllocatorInterface_t* F_create_system_allocator(void);
```

**参数**：无

**返回值**：系统分配器接口指针

**说明**：使用标准C库的malloc/free/realloc实现

#### `F_create_debug_allocator`

创建调试内存分配器。

```c
Stru_MemoryAllocatorInterface_t* F_create_debug_allocator(
    Stru_MemoryAllocatorInterface_t* parent_allocator);
```

**参数**：
- `parent_allocator`：父分配器（可为NULL，使用系统分配器）

**返回值**：调试分配器接口指针

**说明**：添加魔术字检查和统计信息跟踪

#### `F_create_memory_debugger`

创建内存调试器。

```c
Stru_MemoryDebugInterface_t* F_create_memory_debugger(
    Stru_MemoryAllocatorInterface_t* allocator);
```

**参数**：
- `allocator`：要调试的分配器

**返回值**：调试器接口指针

#### `F_create_memory_context`

创建完整的内存管理上下文。

```c
Stru_MemoryContext_t* F_create_memory_context(bool use_debug_allocator);
```

**参数**：
- `use_debug_allocator`：是否使用调试分配器

**返回值**：内存上下文指针

#### `F_destroy_memory_context`

销毁内存管理上下文。

```c
void F_destroy_memory_context(Stru_MemoryContext_t* context);
```

**参数**：
- `context`：要销毁的上下文

### 便捷包装函数

#### `cn_malloc`

分配内存（使用默认分配器）。

```c
void* cn_malloc(size_t size);
```

**参数**：
- `size`：要分配的字节数

**返回值**：分配的内存指针，失败返回NULL

#### `cn_malloc_aligned`

分配对齐内存。

```c
void* cn_malloc_aligned(size_t size, size_t alignment);
```

**参数**：
- `size`：要分配的字节数
- `alignment`：对齐要求（必须是2的幂）

**返回值**：分配的内存指针，失败返回NULL

#### `cn_realloc`

重新分配内存。

```c
void* cn_realloc(void* ptr, size_t new_size);
```

**参数**：
- `ptr`：原内存指针
- `new_size`：新的字节数

**返回值**：重新分配的内存指针，失败返回NULL

#### `cn_free`

释放内存。

```c
void cn_free(void* ptr);
```

**参数**：
- `ptr`：要释放的内存指针

#### `cn_memcpy`

复制内存。

```c
void* cn_memcpy(void* dest, const void* src, size_t size);
```

**参数**：
- `dest`：目标内存
- `src`：源内存
- `size`：要复制的字节数

**返回值**：目标内存指针

#### `cn_memset`

设置内存值。

```c
void* cn_memset(void* ptr, int value, size_t size);
```

**参数**：
- `ptr`：内存指针
- `value`：要设置的值
- `size`：要设置的字节数

**返回值**：内存指针

#### `cn_memcmp`

比较内存。

```c
int cn_memcmp(const void* ptr1, const void* ptr2, size_t size);
```

**参数**：
- `ptr1`：第一个内存块
- `ptr2`：第二个内存块
- `size`：要比较的字节数

**返回值**：比较结果（0表示相等）

## 使用示例

### 基本使用

```c
#include "CN_memory_interface.h"

// 使用默认分配器
void* data = cn_malloc(1024);
if (data != NULL) {
    // 使用内存
    cn_memset(data, 0, 1024);
    cn_free(data);
}

// 使用对齐内存
void* aligned_data = cn_malloc_aligned(256, 16);
if (aligned_data != NULL) {
    cn_free(aligned_data);
}
```

### 使用特定分配器

```c
#include "CN_memory_interface.h"

// 创建系统分配器
Stru_MemoryAllocatorInterface_t* system_allocator = F_create_system_allocator();

// 使用分配器
void* data = system_allocator->allocate(system_allocator, 512, 0);
if (data != NULL) {
    // 使用内存
    system_allocator->deallocate(system_allocator, data);
}

// 清理分配器
system_allocator->cleanup(system_allocator);
```

### 使用调试分配器

```c
#include "CN_memory_interface.h"

// 创建调试分配器
Stru_MemoryAllocatorInterface_t* debug_allocator = F_create_debug_allocator(NULL);

// 分配内存
void* debug_data = debug_allocator->allocate(debug_allocator, 256, 0);

// 获取统计信息
size_t total_allocated, total_freed, current_usage, allocation_count;
debug_allocator->get_statistics(debug_allocator, 
                               &total_allocated, &total_freed, 
                               &current_usage, &allocation_count);

// 验证内存
bool is_valid = debug_allocator->validate(debug_allocator, debug_data);

// 释放内存
debug_allocator->deallocate(debug_allocator, debug_data);

// 清理分配器
debug_allocator->cleanup(debug_allocator);
```

### 使用内存上下文

```c
#include "CN_memory_interface.h"

// 创建带调试功能的内存上下文
Stru_MemoryContext_t* context = F_create_memory_context(true);

if (context != NULL && context->allocator != NULL) {
    // 使用上下文中的分配器
    void* data = context->allocator->allocate(context->allocator, 1024, 0);
    
    if (data != NULL) {
        // 使用内存
        context->allocator->deallocate(context->allocator, data);
    }
    
    // 销毁上下文
    F_destroy_memory_context(context);
}
```

## 错误处理

内存管理函数通过返回值指示错误：
- 分配函数返回NULL表示分配失败
- 验证函数返回false表示内存损坏
- 统计函数通过输出参数返回信息

## 性能考虑

1. **系统分配器**：性能最高，但缺少调试功能
2. **调试分配器**：性能较低，但提供完整的调试信息
3. **对齐分配**：对齐要求必须是2的幂，否则可能影响性能

## 线程安全

- 默认分配器是线程安全的（使用全局静态实例）
- 多个线程可以同时使用不同的分配器实例
- 同一分配器实例在多线程环境下使用时需要外部同步

## 内存调试功能

调试分配器提供以下功能：

1. **魔术字检查**：检测缓冲区溢出
2. **统计信息**：跟踪内存使用情况
3. **泄漏检测**：检查未释放的内存
4. **边界检查**：检测越界访问

## 扩展指南

要添加新的分配器类型：

1. 实现`Stru_MemoryAllocatorInterface_t`接口的所有函数
2. 创建分配器创建函数
3. 在模块文档中记录新分配器的特性

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，包含系统分配器和调试分配器 |

## 相关文件

- `src/infrastructure/memory/CN_memory_interface.h` - 接口定义
- `src/infrastructure/memory/CN_memory_allocator.c` - 实现文件
- `src/infrastructure/memory/README.md` - 模块文档
