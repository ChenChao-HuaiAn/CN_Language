# 内存管理模块

## 概述

内存管理模块是CN_Language项目的基础设施层核心组件，提供统一、可扩展的内存管理接口。模块遵循SOLID设计原则，支持多种内存分配策略，包括系统分配器、调试分配器等。

## 设计原则

1. **单一职责原则**：每个分配器只负责一种内存管理策略
2. **开闭原则**：通过抽象接口支持新的分配器类型
3. **依赖倒置原则**：高层模块通过接口使用内存服务
4. **接口隔离原则**：为不同客户端提供专用接口

## 模块化结构

内存管理模块采用模块化设计，分为四个子模块，每个子模块专注于特定的功能领域：

```
src/infrastructure/memory/
├── CN_memory_interface.h          # 内存管理主接口定义
├── README.md                      # 模块主文档
│
├── allocators/                    # 内存分配器子模块
│   ├── CN_system_allocator.h      # 系统分配器接口
│   ├── CN_system_allocator.c      # 系统分配器实现
│   ├── CN_debug_allocator.h       # 调试分配器接口
│   ├── CN_debug_allocator.c       # 调试分配器实现
│   └── README.md                  # 分配器子模块文档
│
├── utilities/                     # 内存工具函数子模块
│   ├── CN_memory_utilities.h      # 内存工具函数接口
│   ├── CN_memory_utilities.c      # 内存工具函数实现
│   └── README.md                  # 工具函数子模块文档
│
├── context/                       # 内存上下文子模块
│   ├── CN_memory_context.h        # 内存上下文接口
│   ├── CN_memory_context.c        # 内存上下文实现
│   └── README.md                  # 上下文子模块文档
│
└── debug/                         # 内存调试子模块
    ├── CN_memory_debug.h          # 内存调试器接口
    ├── CN_memory_debug.c          # 内存调试器实现
    └── README.md                  # 调试子模块文档
```

### 子模块职责

1. **allocators/**: 提供多种内存分配策略的实现
   - 系统分配器：基于标准C库的内存分配
   - 调试分配器：带调试功能的内存分配

2. **utilities/**: 提供内存操作辅助函数
   - 安全内存操作函数
   - 内存对齐工具
   - 内存统计功能

3. **context/**: 提供层次化内存管理
   - 内存上下文创建和管理
   - 上下文感知的内存分配
   - 批量内存管理

4. **debug/**: 提供内存调试和诊断功能
   - 内存泄漏检测
   - 内存错误检测
   - 性能分析工具

## 核心接口

### 主接口文件 (`CN_memory_interface.h`)

主接口文件提供了统一的访问点，包含所有子模块的接口定义：

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

### 子模块接口

每个子模块提供专门的接口，遵循接口隔离原则：

1. **分配器接口** (`allocators/`): 提供内存分配策略
2. **工具函数接口** (`utilities/`): 提供内存操作辅助
3. **上下文接口** (`context/`): 提供层次化内存管理
4. **调试接口** (`debug/`): 提供调试和诊断功能

详细接口定义请参考各子模块的文档。

## 子模块功能概述

### 1. 分配器子模块 (`allocators/`)
提供多种内存分配策略：

- **系统分配器**: 基于标准C库的高性能分配器
- **调试分配器**: 带完整调试功能的分配器

### 2. 工具函数子模块 (`utilities/`)
提供内存操作辅助函数：

- **安全内存操作**: 带边界检查的内存函数
- **内存对齐工具**: 对齐计算和检查
- **内存统计**: 使用情况统计和分析

### 3. 上下文子模块 (`context/`)
提供层次化内存管理：

- **内存上下文**: 逻辑内存容器
- **上下文切换**: 线程本地上下文管理
- **批量管理**: 相关内存的统一管理

### 4. 调试子模块 (`debug/`)
提供全面的调试功能：

- **内存泄漏检测**: 实时监控和报告
- **错误检测**: 缓冲区溢出、双重释放等
- **性能分析**: 分配性能统计和分析

详细使用方法和示例请参考各子模块的README文档。

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

### 3. 使用内存上下文

```c
#include "context/CN_memory_context.h"

// 获取内存上下文接口
Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();

// 创建上下文
Stru_MemoryContext_t* ctx = ctx_if->create("MyContext", NULL);

// 在上下文中分配内存
void* memory = ctx_if->allocate(ctx, 1024, 16);

// 使用内存...

// 销毁上下文（自动释放所有内存）
ctx_if->destroy(ctx);
```

### 4. 使用内存调试器

```c
#include "debug/CN_memory_debug.h"

// 获取内存调试器接口
Stru_MemoryDebuggerInterface_t* debug_if = F_get_memory_debugger_interface();

// 初始化调试器
debug_if->initialize();
debug_if->enable_monitoring(true);

// 进行内存操作...

// 检查内存泄漏
size_t leak_count = debug_if->get_leak_count();
if (leak_count > 0) {
    debug_if->report_leaks();
}

// 清理调试器
debug_if->cleanup();
```

详细示例请参考各子模块的README文档。

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

### 模块化设计优势

1. **独立编译**: 每个子模块可独立编译和测试
2. **最小依赖**: 减少不必要的编译依赖
3. **易于维护**: 小文件易于理解和修改
4. **灵活组合**: 可根据需要选择使用哪些子模块

## 相关文档

### 子模块文档
- [分配器子模块](./allocators/README.md)
- [工具函数子模块](./utilities/README.md)
- [上下文子模块](./context/README.md)
- [调试子模块](./debug/README.md)

### API文档
- [内存管理API文档](../../docs/api/infrastructure/memory/CN_memory.md)

### 测试文档
- [内存模块测试文档](../../tests/infrastructure/memory/README.md)

### 架构文档
- [CN_Language开发规划](../../docs/architecture/001-中文编程语言CN_Language开发规划.md)
- [技术规范和编码标准](../../docs/specifications/CN_Language项目%20技术规范和编码标准.md)

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现系统分配器和调试分配器 |
| 2.0.0 | 2026-01-07 | 模块化重构，分为四个子模块 |
| 2.1.0 | 2026-01-07 | 添加内存上下文和调试子模块 |

## 维护者

CN_Language架构委员会
