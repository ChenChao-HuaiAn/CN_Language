# 内存分配模块

## 概述

内存分配模块实现了内存上下文中的内存分配、重新分配和释放功能。该模块负责管理内存块的分配记录，跟踪内存使用情况，并提供内存验证功能。

## 核心功能

### 1. 内存分配
- **基本分配**: 在指定上下文中分配对齐的内存块
- **重新分配**: 调整已分配内存块的大小
- **释放**: 释放内存块并更新分配记录

### 2. 分配记录管理
- **记录创建**: 为每次分配创建详细的记录
- **记录跟踪**: 维护分配记录的链表结构
- **记录清理**: 在上下文重置或销毁时清理所有记录

### 3. 内存验证
- **地址验证**: 检查内存地址是否属于指定上下文
- **完整性检查**: 验证内存块的完整性
- **边界检查**: 检测内存越界访问

## 文件说明

### CN_context_allocation.c
- **功能**: 实现内存分配、重新分配、释放和分配记录管理
- **行数**: 320行（符合单一职责原则）
- **设计原则**: 单一职责原则，只负责内存分配相关功能

## 核心函数

### 分配记录管理
```c
// 创建分配记录
Stru_AllocationRecord_t* F_create_allocation_record(
    void* address, size_t size, size_t alignment, 
    const char* file, int line);

// 释放分配记录
void F_free_allocation_record(Stru_AllocationRecord_t* record);

// 添加分配记录到上下文
void F_add_allocation_to_context(Stru_MemoryContext_t* context,
                                Stru_AllocationRecord_t* record);

// 从上下文中移除分配记录
Stru_AllocationRecord_t* F_remove_allocation_from_context(
    Stru_MemoryContext_t* context, void* address);
```

### 内存分配操作
```c
// 在指定上下文中分配内存
void* F_context_allocate(Stru_MemoryContext_t* context,
                        size_t size, size_t alignment);

// 在指定上下文中重新分配内存
void* F_context_reallocate(Stru_MemoryContext_t* context,
                          void* ptr, size_t new_size);

// 释放指定上下文中的内存
void F_context_deallocate(Stru_MemoryContext_t* context, void* ptr);
```

## 设计原则

### 1. 单一职责原则
- 模块只负责内存分配功能
- 分配记录管理独立于其他功能
- 每个函数不超过50行

### 2. 开闭原则
- 支持不同的分配策略
- 通过接口扩展新的分配算法
- 现有代码对修改封闭

### 3. 里氏替换原则
- 所有分配函数遵循相同的契约
- 可替换不同的分配实现
- 行为可预测

## 内存管理策略

### 1. 分配记录结构
```c
typedef struct Stru_AllocationRecord_t
{
    void* address;          // 分配的内存地址
    size_t size;           // 分配的大小
    size_t alignment;      // 对齐要求
    uint64_t timestamp;    // 分配时间戳
    const char* file;      // 分配所在的源文件
    int line;              // 分配所在的行号
    struct Stru_AllocationRecord_t* next; // 下一个分配记录
} Stru_AllocationRecord_t;
```

### 2. 内存跟踪
- **完整跟踪**: 记录所有分配的详细信息
- **调试支持**: 支持源文件和行号跟踪
- **时间戳**: 记录分配时间，支持性能分析

### 3. 错误处理
- **NULL检查**: 对所有输入参数进行验证
- **错误恢复**: 分配失败时清理已分配的资源
- **状态一致性**: 确保上下文状态的一致性

## 使用示例

### 基本内存分配
```c
#include "CN_context_core.h"

// 创建上下文
Stru_MemoryContext_t* ctx = F_context_create("TestContext", NULL);

// 分配内存
void* memory = F_context_allocate(ctx, 1024, 8);
if (memory == NULL) {
    // 处理分配失败
}

// 使用内存...

// 重新分配内存
memory = F_context_reallocate(ctx, memory, 2048);

// 释放内存
F_context_deallocate(ctx, memory);

// 销毁上下文
F_context_destroy(ctx);
```

### 分配记录管理
```c
#include "CN_context_core.h"

// 创建分配记录
Stru_AllocationRecord_t* record = F_create_allocation_record(
    memory, 1024, 8, __FILE__, __LINE__);

// 添加到上下文
F_add_allocation_to_context(ctx, record);

// 从上下文中查找和移除记录
Stru_AllocationRecord_t* found = F_remove_allocation_from_context(ctx, memory);

// 释放记录
F_free_allocation_record(found);
```

## 性能考虑

### 1. 快速路径优化
- **常见大小缓存**: 缓存常见大小的内存块
- **批量操作**: 支持批量分配和释放
- **内存池**: 使用内存池减少系统调用

### 2. 内存碎片管理
- **合并空闲块**: 自动合并相邻的空闲内存块
- **大小分类**: 按大小分类管理内存块
- **碎片整理**: 定期整理内存碎片

### 3. 并发性能
- **线程本地分配**: 支持线程本地内存分配
- **无锁数据结构**: 使用无锁链表管理分配记录
- **细粒度锁**: 对关键部分使用细粒度锁

## 安全特性

### 1. 内存安全
- **边界检查**: 检测内存越界访问
- **双重释放检测**: 防止同一内存块被多次释放
- **未初始化内存检测**: 检测使用未初始化的内存

### 2. 错误检测
- **内存泄漏检测**: 跟踪未释放的内存
- **无效指针检测**: 检测无效的内存访问
- **分配失败处理**: 优雅处理内存分配失败

### 3. 调试支持
- **详细日志**: 记录所有分配和释放操作
- **堆栈跟踪**: 记录分配时的调用堆栈
- **内存转储**: 支持内存状态转储

## 依赖关系

- **内部依赖**: 核心数据结构 (CN_context_struct.h)
- **外部依赖**: C标准库 (stdlib.h, string.h, stdint.h)
- **架构层**: 基础设施层 - 内存管理模块

## 测试策略

### 1. 单元测试
- **分配测试**: 测试各种大小的内存分配
- **重新分配测试**: 测试内存重新分配
- **释放测试**: 测试内存释放和清理

### 2. 集成测试
- **上下文集成**: 测试与上下文管理的集成
- **统计集成**: 测试与统计模块的集成
- **错误处理集成**: 测试错误处理流程

### 3. 性能测试
- **分配性能**: 测试分配操作的性能
- **内存使用**: 测试内存使用效率
- **并发性能**: 测试多线程下的性能

## 维护者

CN_Language架构委员会
