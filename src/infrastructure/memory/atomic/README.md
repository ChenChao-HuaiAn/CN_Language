# CN_Language 原子操作和内存屏障模块

## 概述

原子操作和内存屏障模块是CN_Language基础设施层的核心组件，提供跨平台的原子操作原语和内存屏障功能。本模块遵循现代内存模型（C11/C++11），支持多种内存顺序约束，确保多线程环境下的数据一致性和可见性。

## 功能特性

### 1. 原子操作支持
- **整数原子操作**：支持32位和64位整数的加载、存储、交换、比较交换、算术运算和位运算
- **指针原子操作**：支持指针的原子操作，包括指针算术
- **内存顺序控制**：支持6种内存顺序（Relaxed、Consume、Acquire、Release、Acq_Rel、Seq_Cst）

### 2. 内存屏障原语
- **线程间内存屏障**：确保内存操作在不同线程间的可见性和顺序
- **编译器内存屏障**：防止编译器重排优化
- **特定类型屏障**：加载-加载、存储-存储、加载-存储、存储-加载屏障
- **全内存屏障**：最强的顺序一致性屏障

### 3. 跨平台支持
- **GCC/Clang**：使用`__atomic`内置函数
- **MSVC**：使用`Interlocked`系列函数和内存屏障指令
- **回退实现**：在不支持原子操作的平台上提供非原子版本

## 架构设计

### 接口设计
```c
typedef struct Stru_CN_AtomicInterface_t
{
    // 32位整数操作
    int32_t (*load_i32)(const volatile int32_t* ptr, Eum_CN_MemoryOrder_t order);
    void (*store_i32)(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order);
    int32_t (*exchange_i32)(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order);
    bool (*compare_exchange_i32)(volatile int32_t* ptr, int32_t* expected, 
                                 int32_t desired, Eum_CN_MemoryOrder_t success_order,
                                 Eum_CN_MemoryOrder_t failure_order);
    // ... 更多操作
} Stru_CN_AtomicInterface_t;
```

### 内存顺序枚举
```c
typedef enum Eum_CN_MemoryOrder_t
{
    Eum_MEMORY_ORDER_RELAXED = 0,     // 无顺序约束，仅保证原子性
    Eum_MEMORY_ORDER_CONSUME = 1,     // 数据依赖顺序
    Eum_MEMORY_ORDER_ACQUIRE = 2,     // 获取顺序
    Eum_MEMORY_ORDER_RELEASE = 3,     // 释放顺序
    Eum_MEMORY_ORDER_ACQ_REL = 4,     // 获取-释放顺序
    Eum_MEMORY_ORDER_SEQ_CST = 5      // 顺序一致性
} Eum_CN_MemoryOrder_t;
```

## 使用示例

### 基本使用
```c
#include "CN_atomic.h"

// 原子计数器
static volatile int32_t g_counter = 0;

// 原子递增计数器
void increment_counter(void)
{
    CN_atomic_fetch_add_i32(&g_counter, 1);
}

// 原子加载计数器值
int32_t get_counter(void)
{
    return CN_atomic_load_i32(&g_counter);
}

// 使用内存屏障确保可见性
void publish_data(int32_t* data, int32_t value)
{
    *data = value;
    CN_atomic_full_memory_barrier(); // 确保写入对其他线程可见
}
```

### 高级使用（带内存顺序）
```c
#include "CN_atomic.h"

// 获取默认原子接口
const Stru_CN_AtomicInterface_t* atomic = CN_atomic_get_default();

// 使用特定内存顺序的原子操作
void advanced_atomic_operations(void)
{
    volatile int32_t shared_data = 0;
    
    // 使用获取顺序加载（确保后续读操作不会重排到之前）
    int32_t value = atomic->load_i32(&shared_data, Eum_MEMORY_ORDER_ACQUIRE);
    
    // 使用释放顺序存储（确保之前写操作不会重排到之后）
    atomic->store_i32(&shared_data, 42, Eum_MEMORY_ORDER_RELEASE);
    
    // 使用获取-释放顺序的交换
    int32_t old_value = atomic->exchange_i32(&shared_data, 100, Eum_MEMORY_ORDER_ACQ_REL);
    
    // 使用顺序一致性的比较交换
    int32_t expected = 100;
    bool success = atomic->compare_exchange_i32(&shared_data, &expected, 200,
                                               Eum_MEMORY_ORDER_SEQ_CST,
                                               Eum_MEMORY_ORDER_SEQ_CST);
}
```

### 内存屏障使用
```c
#include "CN_atomic.h"

// 编译器屏障（仅防止编译器重排）
void compiler_barrier_example(void)
{
    int32_t x = 1;
    int32_t y = 2;
    
    CN_atomic_compiler_barrier(); // 确保x和y的赋值不会重排
    
    // 一些操作...
}

// 线程间内存屏障
void thread_memory_barrier_example(void)
{
    volatile int32_t flag = 0;
    volatile int32_t data = 0;
    
    // 线程1：发布数据
    data = 42;
    CN_atomic_full_memory_barrier(); // 确保data写入在flag写入之前完成
    flag = 1;
    
    // 线程2：消费数据
    while (CN_atomic_load_i32(&flag) == 0) {
        // 等待数据就绪
    }
    CN_atomic_full_memory_barrier(); // 确保flag读取在data读取之前完成
    int32_t value = data; // 此时可以安全读取data
}
```

## API参考

### 核心管理函数

| 函数 | 描述 |
|------|------|
| `CN_atomic_get_default()` | 获取默认原子操作接口 |
| `CN_atomic_create_custom()` | 创建自定义原子操作接口 |
| `CN_atomic_destroy()` | 销毁原子操作接口 |

### 便捷函数（顺序一致性）

| 函数 | 描述 |
|------|------|
| `CN_atomic_load_i32()` | 原子加载32位整数 |
| `CN_atomic_store_i32()` | 原子存储32位整数 |
| `CN_atomic_exchange_i32()` | 原子交换32位整数 |
| `CN_atomic_compare_exchange_i32()` | 原子比较并交换32位整数 |
| `CN_atomic_fetch_add_i32()` | 原子加法（返回旧值） |
| `CN_atomic_fetch_sub_i32()` | 原子减法（返回旧值） |
| `CN_atomic_fetch_increment_i32()` | 原子递增（返回旧值） |
| `CN_atomic_fetch_decrement_i32()` | 原子递减（返回旧值） |
| `CN_atomic_full_memory_barrier()` | 全内存屏障 |
| `CN_atomic_compiler_barrier()` | 编译器内存屏障 |

### 内存屏障类型

| 屏障类型 | 描述 | 典型用途 |
|----------|------|----------|
| 编译器屏障 | 仅防止编译器重排 | 硬件寄存器访问、内存映射IO |
| 加载-加载屏障 | 确保屏障前的加载在屏障后的加载之前完成 | 读取标志后读取数据 |
| 存储-存储屏障 | 确保屏障前的存储在屏障后的存储之前完成 | 初始化数据后设置就绪标志 |
| 加载-存储屏障 | 确保屏障前的加载在屏障后的存储之前完成 | 读取配置后执行操作 |
| 存储-加载屏障 | 确保屏障前的存储在屏障后的加载之前完成 | 发布数据后通知其他线程 |
| 全内存屏障 | 最强的屏障，确保所有操作顺序 | 线程间同步、锁实现 |

## 实现细节

### 平台适配策略

#### GCC/Clang实现
- 使用`__atomic`内置函数系列
- 支持完整的内存顺序语义
- 支持32位和64位原子操作

#### MSVC实现
- 使用`Interlocked`系列函数进行原子操作
- 使用`MemoryBarrier()`、`_ReadBarrier()`、`_WriteBarrier()`进行内存屏障
- 在x64平台上支持64位原子操作

#### 回退实现
- 在不支持原子操作的平台上提供非原子版本
- 使用`volatile`关键字提供基本的内存可见性保证
- 适用于单线程环境或不需要原子性的场景

### 性能考虑

1. **内存顺序选择**：
   - **Relaxed**：性能最好，仅保证原子性
   - **Acquire/Release**：中等性能，适合锁和同步原语
   - **Seq_Cst**：性能最差，但提供最强的顺序保证

2. **平台差异**：
   - x86/x64架构：大多数操作具有隐式的内存屏障
   - ARM/PowerPC架构：需要显式的内存屏障指令
   - 嵌入式系统：可能不支持某些原子操作

3. **编译器优化**：
   - 使用编译器内置函数避免函数调用开销
   - 内联小函数提高性能
   - 避免不必要的内存屏障

## 内存安全

### 原子性保证
- 所有原子操作保证在并发访问下的原子性
- 即使在不支持硬件原子操作的平台上，也通过软件锁提供原子性保证

### 内存顺序保证
- 遵循C11/C++11内存模型
- 提供6种内存顺序选项，满足不同场景需求
- 确保多处理器环境下的内存可见性

### 类型安全
- 使用类型明确的函数接口
- 避免类型转换错误
- 提供编译时类型检查

## 测试覆盖

### 单元测试
- 测试所有原子操作的正确性
- 验证内存屏障的效果
- 测试并发场景下的数据一致性

### 集成测试
- 测试与线程模块的集成
- 验证在多线程环境下的正确性
- 测试性能基准

### 平台测试
- 在不同平台（Windows、Linux、macOS）上测试
- 验证跨平台一致性
- 测试边缘情况和错误处理

## 依赖关系

- C标准库（stdlib.h, string.h）
- 平台相关头文件（windows.h, stdatomic.h等）
- 无其他项目模块依赖（基础设施层特性）

## 编码规范

- 遵循项目命名规范（Stru_前缀，Eum_前缀）
- 每个函数不超过50行
- 文件总行数不超过500行
- 完整的Doxygen风格注释
- 错误处理使用返回值而非异常

## 未来扩展

1. **更多数据类型**：支持8位、16位、128位原子操作
2. **浮点原子操作**：支持浮点数的原子操作
3. **事务内存**：支持硬件事务内存（如果可用）
4. **原子引用计数**：提供原子引用计数工具
5. **无锁数据结构**：基于原子操作实现无锁队列、栈等数据结构

## 版本历史

- v1.0.0 (2026-01-03): 初始版本，实现基本原子操作和内存屏障
- v1.1.0 (计划): 添加64位原子操作和指针操作
- v1.2.0 (计划): 添加浮点数原子操作和事务内存支持

## 贡献指南

1. 遵循项目编码规范和架构原则
2. 为新功能添加完整的单元测试
3. 更新API文档和README
4. 通过代码审查和CI测试
5. 确保跨平台兼容性

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 参考文档

- [C11标准 - 原子操作](https://en.cppreference.com/w/c/atomic)
- [C++11内存模型](https://en.cppreference.com/w/cpp/atomic/memory_order)
- [GCC原子内置函数](https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html)
- [MSVC Interlocked函数](https://docs.microsoft.com/en-us/windows/win32/sync/interlocked-variable-access)
