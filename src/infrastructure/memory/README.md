# 内存管理模块

## 概述

内存管理模块是CN_Language项目的基础设施层核心组件，提供统一、可扩展的内存管理接口。模块遵循SOLID设计原则，支持多种内存分配策略，包括系统分配器、调试分配器等。

## 设计原则

1. **单一职责原则**：每个分配器只负责一种内存管理策略
2. **开闭原则**：通过抽象接口支持新的分配器类型
3. **依赖倒置原则**：高层模块通过接口使用内存服务
4. **接口隔离原则**：为不同客户端提供专用接口

## 模块结构

```
src/infrastructure/memory/
├── CN_memory_interface.h     # 内存管理接口定义
├── CN_memory_allocator.c     # 内存分配器实现
└── README.md                 # 模块文档
```

## 核心接口

### 内存分配器接口 (`Stru_MemoryAllocatorInterface_t`)

```c
typedef struct Stru_MemoryAllocatorInterface_t
{
    void* (*allocate)(Stru_MemoryAllocatorInterface_t* allocator, 
                     size_t size, size_t alignment);
    void* (*reallocate)(Stru_MemoryAllocatorInterface_t* allocator,
                       void* ptr, size_t new_size);
    void (*deallocate)(Stru_MemoryAllocatorInterface_t* allocator,
                      void* ptr);
    void (*get_statistics)(Stru_MemoryAllocatorInterface_t* allocator,
                          size_t* total_allocated, size_t* total_freed,
                          size_t* current_usage, size_t* allocation_count);
    bool (*validate)(Stru_MemoryAllocatorInterface_t* allocator,
                    void* ptr);
    void (*cleanup)(Stru_MemoryAllocatorInterface_t* allocator);
    void* private_data;
} Stru_MemoryAllocatorInterface_t;
```

### 内存调试接口 (`Stru_MemoryDebugInterface_t`)

提供内存调试功能，包括内存泄漏检测、越界检查等。

## 可用分配器

### 1. 系统分配器

使用标准C库的`malloc`/`free`函数，提供基本的内存管理功能。

```c
// 创建系统分配器
Stru_MemoryAllocatorInterface_t* allocator = F_create_system_allocator();

// 使用分配器
void* ptr = allocator->allocate(allocator, 1024, 0);
allocator->deallocate(allocator, ptr);
```

### 2. 调试分配器

在系统分配器基础上添加调试功能：
- 内存泄漏检测
- 缓冲区溢出检查
- 分配统计信息

```c
// 创建调试分配器
Stru_MemoryAllocatorInterface_t* debug_allocator = 
    F_create_debug_allocator(NULL);

// 使用调试分配器
void* ptr = debug_allocator->allocate(debug_allocator, 1024, 0);

// 获取统计信息
size_t total_allocated, current_usage;
debug_allocator->get_statistics(debug_allocator, &total_allocated, 
                               NULL, &current_usage, NULL);
```

## 便捷函数

模块提供`cn_`前缀的便捷函数，供项目其他部分使用：

```c
// 基本内存操作
void* cn_malloc(size_t size);
void* cn_realloc(void* ptr, size_t new_size);
void cn_free(void* ptr);

// 对齐内存分配
void* cn_malloc_aligned(size_t size, size_t alignment);

// 内存操作
void* cn_memcpy(void* dest, const void* src, size_t size);
void* cn_memset(void* ptr, int value, size_t size);
int cn_memcmp(const void* ptr1, const void* ptr2, size_t size);
```

## 使用示例

### 基本使用

```c
#include "CN_memory_interface.h"

// 创建分配器
Stru_MemoryAllocatorInterface_t* allocator = F_create_system_allocator();

// 分配内存
void* buffer = allocator->allocate(allocator, 1024, 0);
if (buffer == NULL) {
    // 处理分配失败
}

// 使用内存
// ...

// 释放内存
allocator->deallocate(allocator, buffer);

// 清理分配器
allocator->cleanup(allocator);
```

### 使用便捷函数

```c
#include "CN_memory_interface.h"

// 分配内存
int* numbers = (int*)cn_malloc(10 * sizeof(int));
if (numbers == NULL) {
    // 处理分配失败
}

// 初始化内存
cn_memset(numbers, 0, 10 * sizeof(int));

// 使用内存
for (int i = 0; i < 10; i++) {
    numbers[i] = i * i;
}

// 重新分配内存
numbers = (int*)cn_realloc(numbers, 20 * sizeof(int));

// 释放内存
cn_free(numbers);
```

### 调试内存使用

```c
#include "CN_memory_interface.h"

// 创建调试分配器
Stru_MemoryAllocatorInterface_t* debug_allocator = 
    F_create_debug_allocator(NULL);

// 分配内存
char* str = (char*)debug_allocator->allocate(debug_allocator, 100, 0);

// 检查内存有效性
bool valid = debug_allocator->validate(debug_allocator, str);
if (!valid) {
    // 内存损坏
}

// 获取统计信息
size_t total_allocated, current_usage, allocation_count;
debug_allocator->get_statistics(debug_allocator, &total_allocated, 
                               NULL, &current_usage, &allocation_count);

printf("Total allocated: %zu bytes\n", total_allocated);
printf("Current usage: %zu bytes\n", current_usage);
printf("Allocation count: %zu\n", allocation_count);

// 释放内存
debug_allocator->deallocate(debug_allocator, str);

// 清理分配器
debug_allocator->cleanup(debug_allocator);
```

## 内存上下文

模块提供内存上下文管理，简化多个分配器的使用：

```c
#include "CN_memory_interface.h"

// 创建内存上下文（使用调试分配器）
Stru_MemoryContext_t* context = F_create_memory_context(true);

if (context != NULL) {
    // 使用上下文中的分配器
    void* ptr = context->allocator->allocate(context->allocator, 1024, 0);
    
    // 使用内存
    // ...
    
    // 释放内存
    context->allocator->deallocate(context->allocator, ptr);
    
    // 销毁上下文
    F_destroy_memory_context(context);
}
```

## 性能考虑

1. **系统分配器**：性能最高，适合生产环境
2. **调试分配器**：性能较低，适合开发和测试环境
3. **内存对齐**：使用`cn_malloc_aligned`进行对齐内存分配，提高访问性能

## 线程安全

- 分配器实例本身不是线程安全的
- 每个线程应使用独立的分配器实例
- 全局便捷函数使用线程安全的默认分配器

## 错误处理

内存分配失败时返回`NULL`，调用者应检查返回值：

```c
void* ptr = cn_malloc(size);
if (ptr == NULL) {
    // 处理内存分配失败
    cn_set_error(Eum_ERROR_OUT_OF_MEMORY, 
                "Failed to allocate memory", 
                __FILE__, __LINE__, NULL);
    return false;
}
```

## 扩展指南

要添加新的分配器类型：

1. 实现分配器接口函数
2. 创建分配器创建函数
3. 更新接口文档
4. 添加单元测试

示例：实现池分配器

```c
// 池分配器私有数据
typedef struct Stru_PoolAllocatorData_t {
    // 池配置和数据
} Stru_PoolAllocatorData_t;

// 实现接口函数
static void* pool_allocate(Stru_MemoryAllocatorInterface_t* allocator,
                          size_t size, size_t alignment) {
    // 池分配实现
}

// 创建函数
Stru_MemoryAllocatorInterface_t* F_create_pool_allocator(/* 参数 */) {
    // 创建池分配器
}
```

## 相关文档

- [API文档](../../docs/api/infrastructure/memory/API.md)
- [设计文档](../../docs/design/infrastructure/memory/DESIGN.md)
- [测试文档](../../tests/unit/infrastructure/memory/README.md)

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现系统分配器和调试分配器 |

## 维护者

CN_Language架构委员会
