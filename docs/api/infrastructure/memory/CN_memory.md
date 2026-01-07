# 内存管理模块 API 文档

## 概述

内存管理模块是CN_Language项目的基础设施层核心组件，提供统一、可扩展的内存管理接口。本模块采用模块化设计，分为四个子模块，每个子模块专注于特定的功能领域。模块遵循SOLID设计原则，支持多种内存分配策略，并包含完整的调试功能。

## 模块化架构

内存管理模块采用分层模块化设计：

```
内存管理模块
├── 分配器子模块 (allocators/)
│   ├── 系统分配器
│   └── 调试分配器
├── 工具函数子模块 (utilities/)
│   ├── 内存操作函数
│   ├── 内存安全函数
│   └── 内存对齐工具
├── 上下文子模块 (context/)
│   ├── 内存上下文管理
│   ├── 上下文切换
│   └── 批量内存管理
└── 调试子模块 (debug/)
    ├── 内存泄漏检测
    ├── 错误检测
    └── 性能分析
```

每个子模块提供独立的接口，遵循接口隔离原则，支持按需使用。

## 设计原则

1. **单一职责原则**：每个分配器只负责一种内存管理策略
2. **开闭原则**：通过抽象接口支持新的分配器类型
3. **依赖倒置原则**：高层模块通过抽象接口使用内存服务
4. **接口隔离原则**：为不同客户端提供专用接口

## 核心接口

### 主接口文件 (`CN_memory_interface.h`)

主接口文件提供统一的访问点，包含所有子模块的接口定义：

```c
// 包含所有子模块的头文件
#include "allocators/CN_system_allocator.h"
#include "allocators/CN_debug_allocator.h"
#include "utilities/CN_memory_utilities.h"
#include "context/CN_memory_context.h"
#include "debug/CN_memory_debug.h"

// 便捷函数（向后兼容）
void* cn_malloc(size_t size);
void* cn_realloc(void* ptr, size_t new_size);
void cn_free(void* ptr);
void* cn_malloc_aligned(size_t size, size_t alignment);
void* cn_memcpy(void* dest, const void* src, size_t size);
void* cn_memset(void* ptr, int value, size_t size);
int cn_memcmp(const void* ptr1, const void* ptr2, size_t size);
```

### 分配器接口 (`allocators/`)

#### Stru_AllocatorInterface_t

通用内存分配器接口，所有分配器都实现此接口：

```c
typedef struct Stru_AllocatorInterface_t {
    void* (*allocate)(size_t size, size_t alignment);
    void* (*reallocate)(void* ptr, size_t new_size);
    void (*deallocate)(void* ptr);
    size_t (*get_allocated_size)(void* ptr);
    bool (*validate)(void* ptr);
} Stru_AllocatorInterface_t;
```

### 工具函数接口 (`utilities/`)

#### Stru_MemoryOperationsInterface_t

内存操作接口：

```c
typedef struct Stru_MemoryOperationsInterface_t {
    void* (*copy)(void* dest, const void* src, size_t n);
    void* (*move)(void* dest, const void* src, size_t n);
    int (*compare)(const void* ptr1, const void* ptr2, size_t n);
    void* (*set)(void* dest, int value, size_t n);
} Stru_MemoryOperationsInterface_t;
```

#### Stru_MemorySafetyInterface_t

内存安全接口：

```c
typedef struct Stru_MemorySafetyInterface_t {
    void (*secure_zero)(void* ptr, size_t n);
    bool (*validate_range)(const void* ptr, size_t n);
    void (*initialize_with_pattern)(void* ptr, size_t n, uint8_t pattern);
} Stru_MemorySafetyInterface_t;
```

### 上下文接口 (`context/`)

#### Stru_MemoryContextInterface_t

内存上下文管理接口：

```c
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

### 调试接口 (`debug/`)

#### Stru_MemoryDebuggerInterface_t

内存调试器接口：

```c
typedef struct Stru_MemoryDebuggerInterface_t {
    // 初始化/清理
    bool (*initialize)(void);
    void (*cleanup)(void);
    
    // 内存监控
    void (*enable_monitoring)(bool enable);
    bool (*is_monitoring_enabled)(void);
    
    // 泄漏检测
    size_t (*get_leak_count)(void);
    void (*report_leaks)(void);
    void (*clear_leak_records)(void);
    
    // 错误检测
    void (*enable_overflow_check)(bool enable);
    void (*enable_double_free_check)(bool enable);
    void (*enable_uninitialized_check)(bool enable);
    
    // 内存验证
    bool (*validate_pointer)(const void* ptr);
    bool (*validate_memory_range)(const void* ptr, size_t size);
    void (*check_all_allocations)(void);
    
    // 调试信息
    void (*dump_memory_info)(void);
    void (*dump_allocation_stats)(void);
    void (*set_debug_output)(void (*output_func)(const char*));
    
    // 调用栈跟踪
    void (*enable_stack_trace)(bool enable);
    size_t (*get_stack_trace_depth)(void);
    void (*set_stack_trace_depth)(size_t depth);
    
    // 性能分析
    uint64_t (*get_total_allocations)(void);
    uint64_t (*get_total_deallocations)(void);
    size_t (*get_peak_memory_usage)(void);
    size_t (*get_current_memory_usage)(void);
} Stru_MemoryDebuggerInterface_t;
```

## API 函数

### 分配器子模块函数

#### `F_get_system_allocator`

获取系统分配器接口。

```c
Stru_AllocatorInterface_t* F_get_system_allocator(void);
```

**参数**：无

**返回值**：系统分配器接口指针

**说明**：基于标准C库的高性能分配器

#### `F_get_debug_allocator`

获取调试分配器接口。

```c
Stru_AllocatorInterface_t* F_get_debug_allocator(void);
```

**参数**：无

**返回值**：调试分配器接口指针

**说明**：带完整调试功能的分配器

### 工具函数子模块函数

#### `F_get_memory_operations`

获取内存操作接口。

```c
Stru_MemoryOperationsInterface_t* F_get_memory_operations(void);
```

**参数**：无

**返回值**：内存操作接口指针

#### `F_get_memory_safety`

获取内存安全接口。

```c
Stru_MemorySafetyInterface_t* F_get_memory_safety(void);
```

**参数**：无

**返回值**：内存安全接口指针

### 上下文子模块函数

#### `F_get_memory_context_interface`

获取内存上下文接口。

```c
Stru_MemoryContextInterface_t* F_get_memory_context_interface(void);
```

**参数**：无

**返回值**：内存上下文接口指针

### 调试子模块函数

#### `F_get_memory_debugger_interface`

获取内存调试器接口。

```c
Stru_MemoryDebuggerInterface_t* F_get_memory_debugger_interface(void);
```

**参数**：无

**返回值**：内存调试器接口指针

### 便捷函数（向后兼容）

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

### 1. 基本使用（向后兼容）

```c
#include "CN_memory_interface.h"

// 使用便捷函数（向后兼容）
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

### 2. 使用系统分配器

```c
#include "allocators/CN_system_allocator.h"

// 获取系统分配器接口
Stru_AllocatorInterface_t* allocator = F_get_system_allocator();

// 分配内存
void* memory = allocator->allocate(1024, 8);

// 使用内存...

// 释放内存
allocator->deallocate(memory);
```

### 3. 使用调试分配器

```c
#include "allocators/CN_debug_allocator.h"

// 获取调试分配器接口
Stru_AllocatorInterface_t* debug_allocator = F_get_debug_allocator();

// 分配内存（带调试信息）
void* debug_memory = debug_allocator->allocate(512, 16);

// 验证内存有效性
bool valid = debug_allocator->validate(debug_memory);

// 获取分配大小
size_t allocated_size = debug_allocator->get_allocated_size(debug_memory);

// 释放内存
debug_allocator->deallocate(debug_memory);
```

### 4. 使用内存工具函数

```c
#include "utilities/CN_memory_utilities.h"

// 获取内存操作接口
Stru_MemoryOperationsInterface_t* ops = F_get_memory_operations();

// 安全内存复制
char src[100] = "Hello, World!";
char dest[100];
ops->copy(dest, src, strlen(src) + 1);

// 内存比较
int result = ops->compare(src, dest, strlen(src) + 1);

// 获取内存安全接口
Stru_MemorySafetyInterface_t* safety = F_get_memory_safety();

// 安全清零敏感数据
char sensitive_data[256];
safety->secure_zero(sensitive_data, sizeof(sensitive_data));
```

### 5. 使用内存上下文

```c
#include "context/CN_memory_context.h"

// 获取内存上下文接口
Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();

// 创建根上下文
Stru_MemoryContext_t* root_ctx = ctx_if->create("RootContext", NULL);

// 创建子上下文
Stru_MemoryContext_t* child_ctx = ctx_if->create("ChildContext", root_ctx);

// 在子上下文中分配内存
void* memory = ctx_if->allocate(child_ctx, 1024, 8);

// 使用内存...

// 获取内存统计
size_t total_allocated = ctx_if->get_total_allocated(child_ctx);
size_t peak_allocated = ctx_if->get_peak_allocated(child_ctx);

// 销毁子上下文（自动释放所有内存）
ctx_if->destroy(child_ctx);

// 销毁根上下文
ctx_if->destroy(root_ctx);
```

### 6. 使用内存调试器

```c
#include "debug/CN_memory_debug.h"

// 获取内存调试器接口
Stru_MemoryDebuggerInterface_t* debug_if = F_get_memory_debugger_interface();

// 初始化调试器
debug_if->initialize();

// 启用内存监控
debug_if->enable_monitoring(true);

// 启用各种检查
debug_if->enable_overflow_check(true);
debug_if->enable_double_free_check(true);
debug_if->enable_uninitialized_check(true);

// 启用调用栈跟踪
debug_if->enable_stack_trace(true);
debug_if->set_stack_trace_depth(10);

// 进行内存操作...
void* ptr1 = malloc(100);
void* ptr2 = calloc(10, 20);
free(ptr1);
free(ptr2);

// 检查内存泄漏
size_t leak_count = debug_if->get_leak_count();
if (leak_count > 0) {
    printf("发现 %zu 个内存泄漏\n", leak_count);
    debug_if->report_leaks();
}

// 获取性能统计
uint64_t total_allocs = debug_if->get_total_allocations();
uint64_t total_frees = debug_if->get_total_deallocations();
size_t peak_usage = debug_if->get_peak_memory_usage();

printf("性能统计:\n");
printf("  总分配次数: %llu\n", total_allocs);
printf("  总释放次数: %llu\n", total_frees);
printf("  峰值内存使用: %zu bytes\n", peak_usage);

// 清理调试器
debug_if->cleanup();
```

## 错误处理

内存管理函数通过返回值指示错误：
- 分配函数返回NULL表示分配失败
- 验证函数返回false表示内存损坏或无效
- 统计函数通过输出参数返回信息
- 上下文操作返回NULL表示操作失败

## 性能考虑

### 1. 分配器性能
- **系统分配器**：最高性能，直接系统调用
- **调试分配器**：较低性能，但提供完整的调试信息
- **对齐分配**：对齐要求必须是2的幂，否则可能影响性能

### 2. 模块化开销
- 接口调用有轻微性能开销
- 模块化设计便于性能优化
- 可选择性启用功能减少开销

### 3. 调试开销
- 调试功能有显著性能开销
- 生产环境建议禁用调试功能
- 运行时动态启用/禁用

## 线程安全

### 1. 分配器线程安全
- 默认分配器是线程安全的（使用全局静态实例）
- 多个线程可以同时使用不同的分配器实例
- 同一分配器实例在多线程环境下使用时需要外部同步

### 2. 上下文线程安全
- 内存上下文不是线程安全的
- 每个线程应使用独立的上下文
- 上下文切换是线程本地的

### 3. 调试器线程安全
- 调试器提供基本的线程安全保护
- 高并发环境下可能需要额外同步
- 性能统计可能不完全准确

## 模块化设计优势

### 1. 独立编译
- 每个子模块可独立编译和测试
- 减少不必要的编译依赖
- 加快编译速度

### 2. 最小依赖
- 子模块间通过接口通信
- 减少编译时依赖
- 便于代码维护

### 3. 灵活组合
- 可根据需要选择使用哪些子模块
- 支持运行时模块替换
- 便于功能扩展

### 4. 易于测试
- 每个子模块可独立测试
- 测试覆盖更全面
- 便于自动化测试

## 扩展指南

### 添加新的分配器类型
1. 在`allocators/`目录中创建新的分配器文件
2. 实现`Stru_AllocatorInterface_t`接口
3. 提供分配器获取函数（如`F_get_pool_allocator()`）
4. 更新`allocators/README.md`文档
5. 添加单元测试

### 添加新的工具函数
1. 在`utilities/`目录中扩展现有接口或创建新接口
2. 遵循接口隔离原则
3. 提供完整的参数验证
4. 添加单元测试

### 添加新的调试功能
1. 在`debug/`目录中扩展现有接口
2. 确保性能开销可控
3. 提供详细的调试信息
4. 添加测试用例

### 添加新的上下文类型
1. 在`context/`目录中创建新的上下文实现
2. 实现`Stru_MemoryContextInterface_t`接口
3. 提供上下文管理函数
4. 添加单元测试

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，包含系统分配器和调试分配器 |
| 2.0.0 | 2026-01-07 | 模块化重构，分为四个子模块 |
| 2.1.0 | 2026-01-07 | 完善子模块文档和API文档 |

## 相关文件

### 源代码文件
- `src/infrastructure/memory/CN_memory_interface.h` - 主接口定义
- `src/infrastructure/memory/allocators/` - 分配器子模块
- `src/infrastructure/memory/utilities/` - 工具函数子模块
- `src/infrastructure/memory/context/` - 上下文子模块
- `src/infrastructure/memory/debug/` - 调试子模块

### 文档文件
- `src/infrastructure/memory/README.md` - 模块主文档
- `src/infrastructure/memory/allocators/README.md` - 分配器子模块文档
- `src/infrastructure/memory/utilities/README.md` - 工具函数子模块文档
- `src/infrastructure/memory/context/README.md` - 上下文子模块文档
- `src/infrastructure/memory/debug/README.md` - 调试子模块文档
- `docs/api/infrastructure/memory/CN_memory.md` - API文档（本文档）

### 测试文件
- `tests/infrastructure/memory/` - 内存模块测试代码

## 维护者

CN_Language架构委员会
