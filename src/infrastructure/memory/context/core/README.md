# 内存上下文核心模块

## 概述

内存上下文核心模块是内存上下文模块的核心实现部分，包含所有子模块的具体实现。本模块遵循单一职责原则，将功能分解为多个专注的子模块，每个子模块负责特定的功能领域。

## 模块结构

核心模块采用模块化设计，分为以下子模块：

### 1. 接口定义模块 (`CN_context_interface.h`)
- 定义内存上下文的核心接口 `Stru_MemoryContextInterface_t`
- 提供全局函数声明
- 遵循接口隔离原则

### 2. 数据结构模块 (`CN_context_struct.h`)
- 定义内存上下文的核心数据结构
- 包含分配记录、上下文状态等内部结构
- 隐藏实现细节，只暴露必要接口

### 3. 核心功能模块 (`CN_context_core.h`)
- 声明核心功能函数
- 提供模块间函数声明
- 遵循依赖倒置原则

### 4. 上下文管理模块 (`CN_context_management.c`)
- 实现上下文的创建、销毁和重置功能
- 管理上下文层次结构
- 处理父子关系

### 5. 上下文操作模块 (`CN_context_operations.c`)
- 实现上下文切换和查询功能
- 提供线程本地上下文支持
- 实现上下文遍历功能

### 6. 内存分配模块 (`CN_context_allocation.c`)
- 实现上下文感知的内存分配
- 支持对齐内存分配
- 管理分配记录

### 7. 统计模块 (`CN_context_statistics.c`)
- 实现内存使用统计功能
- 跟踪总分配、峰值分配和当前使用
- 提供性能监控

### 8. 接口初始化模块 (`CN_context_interface.c`)
- 初始化全局接口实例
- 管理全局状态
- 提供系统初始化和关闭功能

## 设计原则

### 1. 单一职责原则
- 每个.c文件不超过500行
- 每个函数不超过50行
- 每个模块只负责一个功能领域

### 2. 开闭原则
- 通过抽象接口支持扩展
- 现有代码对修改封闭
- 支持可替换的实现

### 3. 里氏替换原则
- 所有实现可互换使用
- 接口契约一致
- 行为可预测

### 4. 接口隔离原则
- 为不同客户端提供专用接口
- 避免"胖接口"
- 通过接口组合提供灵活功能

### 5. 依赖倒置原则
- 高层模块定义接口
- 低层模块实现接口
- 通过依赖注入管理依赖

## 文件说明

### `CN_context_interface.h`
```c
// 核心接口定义
typedef struct Stru_MemoryContextInterface_t {
    // 上下文管理函数指针
    Stru_MemoryContext_t* (*create)(const char* name, Stru_MemoryContext_t* parent);
    void (*destroy)(Stru_MemoryContext_t* context);
    void (*reset)(Stru_MemoryContext_t* context);
    
    // ... 其他函数指针
} Stru_MemoryContextInterface_t;

// 全局函数声明
Stru_MemoryContextInterface_t* F_get_memory_context_interface(void);
bool F_initialize_memory_context_system(void);
void F_shutdown_memory_context_system(void);
```

### `CN_context_struct.h`
```c
// 分配记录结构
typedef struct Stru_AllocationRecord_t {
    void* address;
    size_t size;
    size_t alignment;
    uint64_t timestamp;
    struct Stru_AllocationRecord_t* next;
} Stru_AllocationRecord_t;

// 内存上下文结构
typedef struct Stru_MemoryContext_t {
    char* name;
    uint64_t id;
    uint64_t creation_time;
    size_t depth;
    
    // 层次结构
    Stru_MemoryContext_t* parent;
    Stru_MemoryContext_t* first_child;
    Stru_MemoryContext_t* next_sibling;
    
    // 内存管理
    Stru_AllocationRecord_t* allocations;
    size_t total_allocated;
    size_t peak_allocated;
    size_t current_usage;
    size_t allocation_count;
    
    // ... 其他字段
} Stru_MemoryContext_t;
```

### `CN_context_core.h`
```c
// 核心函数声明（在其他模块中实现）
extern Stru_MemoryContext_t* F_context_create(const char* name, Stru_MemoryContext_t* parent);
extern void F_context_destroy(Stru_MemoryContext_t* context);
extern void F_context_reset(Stru_MemoryContext_t* context);
// ... 其他函数声明
```

## 模块间依赖关系

```
CN_context_interface.c
    ├── CN_context_core.h (函数声明)
    ├── CN_context_interface.h (接口定义)
    └── CN_context_struct.h (数据结构)

CN_context_management.c
    ├── CN_context_core.h (函数声明)
    ├── CN_context_struct.h (数据结构)
    └── 标准库头文件

CN_context_operations.c
    ├── CN_context_core.h (函数声明)
    ├── CN_context_struct.h (数据结构)
    └── 标准库头文件

CN_context_allocation.c
    ├── CN_context_core.h (函数声明)
    ├── CN_context_struct.h (数据结构)
    └── 标准库头文件

CN_context_statistics.c
    ├── CN_context_core.h (函数声明)
    ├── CN_context_struct.h (数据结构)
    └── 标准库头文件
```

## 编译和链接

### 独立编译
每个.c文件可以独立编译，减少编译依赖：

```bash
# 编译单个模块
gcc -c CN_context_management.c -o CN_context_management.o
gcc -c CN_context_operations.c -o CN_context_operations.o
gcc -c CN_context_allocation.c -o CN_context_allocation.o
gcc -c CN_context_statistics.c -o CN_context_statistics.o
gcc -c CN_context_interface.c -o CN_context_interface.o

# 链接所有模块
gcc CN_context_management.o CN_context_operations.o \
    CN_context_allocation.o CN_context_statistics.o \
    CN_context_interface.o -o libcontext.a
```

### 依赖管理
- 只依赖C标准库
- 不依赖项目其他层（遵循分层架构）
- 通过接口减少编译时依赖

## 测试策略

### 单元测试
每个子模块有独立的单元测试：
- `test_context_management.c` - 测试上下文管理功能
- `test_context_operations.c` - 测试上下文操作功能
- `test_context_allocation.c` - 测试内存分配功能
- `test_context_statistics.c` - 测试统计功能

### 集成测试
- `test_context_runner.c` - 集成测试运行器
- `test_context_utils.c` - 测试辅助函数

### 测试覆盖率目标
- 语句覆盖率 ≥85%
- 分支覆盖率 ≥70%
- 函数覆盖率 ≥90%
- 行覆盖率 ≥85%

## 性能考虑

### 1. 内存分配优化
- 预分配内存池减少系统调用
- 内存重用减少碎片
- 批量操作优化

### 2. 上下文切换优化
- 线程本地存储减少锁竞争
- 快速路径优化常见操作
- 缓存友好数据结构

### 3. 统计开销控制
- 可配置的统计功能
- 运行时启用/禁用
- 最小化性能影响

## 错误处理

### 1. 参数验证
- 所有公共函数验证输入参数
- 提供清晰的错误信息
- 防止无效操作

### 2. 资源管理
- 谁创建谁销毁原则
- 自动资源清理
- 防止资源泄漏

### 3. 状态一致性
- 事务性操作
- 错误回滚
- 状态恢复

## 扩展指南

### 1. 添加新功能
1. 在`CN_context_interface.h`中扩展接口
2. 在`CN_context_struct.h`中扩展数据结构（如果需要）
3. 在适当的子模块中实现功能
4. 添加单元测试
5. 更新文档

### 2. 优化现有功能
1. 性能分析和基准测试
2. 识别性能瓶颈
3. 实现优化
4. 验证正确性
5. 更新性能文档

### 3. 平台适配
1. 识别平台差异
2. 实现平台特定优化
3. 条件编译
4. 平台测试

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-01-08 | 初始版本，基本功能 |
| 2.0.0 | 2026-01-08 | 模块化重构，分为多个子模块 |
| 2.1.0 | 2026-01-08 | 完善文档和测试 |

## 维护者

CN_Language架构委员会
