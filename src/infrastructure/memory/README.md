# CN_Language 内存管理模块

## 概述

内存管理模块是CN_Language基础设施层的核心组件，提供统一的内存管理接口和多种分配器策略。本模块遵循SOLID设计原则和项目架构规范，确保内存操作的安全性和高效性。

## 功能特性

### 1. 统一内存管理接口
- 提供标准化的内存分配、释放和重新分配接口
- 支持文件行号跟踪，便于调试和错误定位
- 遵循接口隔离原则，最小化接口依赖

### 2. 多种分配器策略
- **系统分配器**：包装标准库的malloc/free/realloc
- **调试分配器**：提供边界检查、泄漏检测和调试信息输出
- **对象池分配器**：固定大小对象重用，减少内存碎片
- **区域分配器**：批量分配，统一释放，适合临时数据

### 3. 内存安全特性
- 内存统计信息跟踪（分配次数、使用量、峰值等）
- 调试模式支持（内存填充、分配跟踪）
- 泄漏检测功能
- 堆完整性验证（简化版）

## 架构设计

### 接口设计
```c
typedef struct Stru_CN_AllocatorInterface_t
{
    void* (*allocate)(size_t size, const char* file, int line);
    void (*deallocate)(void* ptr, const char* file, int line);
    void* (*reallocate)(void* ptr, size_t new_size, const char* file, int line);
} Stru_CN_AllocatorInterface_t;
```

### 分配器类型
```c
typedef enum Eum_CN_AllocatorType_t
{
    Eum_ALLOCATOR_SYSTEM = 0,      // 系统分配器
    Eum_ALLOCATOR_DEBUG = 1,       // 调试分配器
    Eum_ALLOCATOR_POOL = 2,        // 对象池分配器
    Eum_ALLOCATOR_ARENA = 3        // 区域分配器
} Eum_CN_AllocatorType_t;
```

## 使用示例

### 基本使用
```c
#include "CN_memory.h"

// 初始化内存系统（使用系统分配器）
CN_memory_init(Eum_ALLOCATOR_SYSTEM);

// 分配内存
int* data = (int*)cn_malloc(10 * sizeof(int));

// 使用内存
for (int i = 0; i < 10; i++) {
    data[i] = i * i;
}

// 释放内存
cn_free(data);

// 关闭内存系统
CN_memory_shutdown();
```

### 切换分配器
```c
// 切换到调试分配器
cn_set_allocator(Eum_ALLOCATOR_DEBUG);

// 启用调试输出
cn_enable_debug(true);

// 现在所有分配都会输出调试信息
void* ptr = cn_malloc(100);
cn_free(ptr);
```

### 内存统计
```c
// 分配一些内存
void* p1 = cn_malloc(100);
void* p2 = cn_malloc(200);
void* p3 = cn_calloc(10, sizeof(int));

// 查看统计信息
cn_dump_stats();

// 检查泄漏
if (cn_check_leaks()) {
    printf("检测到内存泄漏！\n");
}

// 清理
cn_free(p1);
cn_free(p2);
cn_free(p3);
```

## API参考

### 核心函数

| 函数 | 描述 |
|------|------|
| `CN_memory_init()` | 初始化内存管理系统 |
| `CN_memory_shutdown()` | 关闭内存管理系统 |
| `cn_malloc()` | 分配内存 |
| `cn_free()` | 释放内存 |
| `cn_realloc()` | 重新分配内存 |
| `cn_calloc()` | 分配并清零内存 |

### 分配器管理

| 函数 | 描述 |
|------|------|
| `cn_set_allocator()` | 设置当前分配器类型 |
| `cn_get_allocator_type()` | 获取当前分配器类型 |
| `cn_get_allocator()` | 获取当前分配器接口 |

### 调试和统计

| 函数 | 描述 |
|------|------|
| `cn_enable_debug()` | 启用/禁用调试模式 |
| `cn_check_leaks()` | 检查内存泄漏 |
| `cn_dump_stats()` | 转储统计信息 |
| `cn_validate_heap()` | 验证堆完整性 |

## 实现细节

### 系统分配器
- 直接包装标准库函数
- 提供基本的内存统计
- 支持调试模式下的内存填充

### 调试分配器
- 输出详细的分配/释放信息
- 包含文件名和行号
- 便于追踪内存问题

### 对象池分配器
- 预分配固定大小的对象池
- 重用已释放的对象，减少分配开销
- 适合频繁创建销毁的小对象

### 区域分配器
- 一次性分配大块内存
- 从区域中分配小内存块
- 区域销毁时统一释放所有内存
- 适合临时数据或同一生命周期的对象

## 性能考虑

1. **系统分配器**：性能与标准库相当，适合通用场景
2. **调试分配器**：有额外开销，仅用于调试
3. **对象池分配器**：减少碎片，提高小对象分配速度
4. **区域分配器**：批量操作，减少系统调用

## 内存安全

1. **边界检查**：调试模式下检测越界访问
2. **泄漏检测**：跟踪未释放的内存
3. **统计信息**：监控内存使用情况
4. **调试支持**：便于定位内存问题

## 测试覆盖

- 单元测试覆盖所有公共API
- 集成测试验证分配器协作
- 压力测试确保长时间运行稳定性
- 内存安全测试验证边界条件

## 依赖关系

- C标准库（stdlib.h, string.h, stdio.h）
- 无其他项目模块依赖（基础设施层特性）

## 编码规范

- 遵循项目命名规范（Stru_前缀，Eum_前缀）
- 每个函数不超过50行
- 文件总行数不超过500行
- 完整的Doxygen风格注释

## 未来扩展

1. **垃圾回收**：添加引用计数或标记清除GC
2. **内存分析**：更详细的内存使用分析
3. **多线程支持**：线程安全的分配器实现
4. **自定义分配器**：允许用户提供自定义分配策略

## 版本历史

- v1.0.0 (2026-01-02): 初始版本，实现统一内存管理接口
- v1.1.0 (计划): 添加完整的对象池和区域分配器实现
- v1.2.0 (计划): 添加多线程支持和性能优化

## 贡献指南

1. 遵循项目编码规范和架构原则
2. 为新功能添加完整的单元测试
3. 更新API文档和README
4. 通过代码审查和CI测试

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件
