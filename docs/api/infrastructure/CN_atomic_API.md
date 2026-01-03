# CN_atomic API 文档

## 概述

`CN_atomic`模块提供跨平台的原子操作和内存屏障原语，支持现代内存模型（C11/C++11）。本模块是CN_Language基础设施层的一部分，用于实现线程安全的并发操作。

## 头文件

```c
#include "src/infrastructure/memory/atomic/CN_atomic.h"
```

## 数据类型

### 内存顺序枚举

```c
typedef enum Eum_CN_MemoryOrder_t
{
    Eum_MEMORY_ORDER_RELAXED = 0,     /**< 无顺序约束，仅保证原子性 */
    Eum_MEMORY_ORDER_CONSUME = 1,     /**< 数据依赖顺序（C++11 consume，C11 acquire） */
    Eum_MEMORY_ORDER_ACQUIRE = 2,     /**< 获取顺序，确保后续读/写不会重排到之前 */
    Eum_MEMORY_ORDER_RELEASE = 3,     /**< 释放顺序，确保之前读/写不会重排到之后 */
    Eum_MEMORY_ORDER_ACQ_REL = 4,     /**< 获取-释放顺序，同时具有acquire和release语义 */
    Eum_MEMORY_ORDER_SEQ_CST = 5      /**< 顺序一致性，最强内存顺序 */
} Eum_CN_MemoryOrder_t;
```

### 原子操作接口结构体

```c
typedef struct Stru_CN_AtomicInterface_t
{
    // 32位整数原子操作
    int32_t (*load_i32)(const volatile int32_t* ptr, Eum_CN_MemoryOrder_t order);
    void (*store_i32)(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order);
    int32_t (*exchange_i32)(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order);
    bool (*compare_exchange_i32)(volatile int32_t* ptr, int32_t* expected, 
                                 int32_t desired, Eum_CN_MemoryOrder_t success_order,
                                 Eum_CN_MemoryOrder_t failure_order);
    int32_t (*fetch_add_i32)(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order);
    int32_t (*fetch_sub_i32)(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order);
    int32_t (*fetch_and_i32)(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order);
    int32_t (*fetch_or_i32)(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order);
    int32_t (*fetch_xor_i32)(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order);
    
    // 64位整数原子操作
    int64_t (*load_i64)(const volatile int64_t* ptr, Eum_CN_MemoryOrder_t order);
    void (*store_i64)(volatile int64_t* ptr, int64_t value, Eum_CN_MemoryOrder_t order);
    int64_t (*exchange_i64)(volatile int64_t* ptr, int64_t value, Eum_CN_MemoryOrder_t order);
    bool (*compare_exchange_i64)(volatile int64_t* ptr, int64_t* expected, 
                                 int64_t desired, Eum_CN_MemoryOrder_t success_order,
                                 Eum_CN_MemoryOrder_t failure_order);
    int64_t (*fetch_add_i64)(volatile int64_t* ptr, int64_t value, Eum_CN_MemoryOrder_t order);
    int64_t (*fetch_sub_i64)(volatile int64_t* ptr, int64_t value, Eum_CN_MemoryOrder_t order);
    
    // 指针原子操作
    void* (*load_ptr)(void* const volatile* ptr, Eum_CN_MemoryOrder_t order);
    void (*store_ptr)(void* volatile* ptr, void* value, Eum_CN_MemoryOrder_t order);
    void* (*exchange_ptr)(void* volatile* ptr, void* value, Eum_CN_MemoryOrder_t order);
    bool (*compare_exchange_ptr)(void* volatile* ptr, void** expected, 
                                 void* desired, Eum_CN_MemoryOrder_t success_order,
                                 Eum_CN_MemoryOrder_t failure_order);
    void* (*fetch_add_ptr)(void* volatile* ptr, ptrdiff_t offset, Eum_CN_MemoryOrder_t order);
    void* (*fetch_sub_ptr)(void* volatile* ptr, ptrdiff_t offset, Eum_CN_MemoryOrder_t order);
    
    // 内存屏障操作
    void (*thread_fence)(Eum_CN_MemoryOrder_t order);
    void (*signal_fence)(Eum_CN_MemoryOrder_t order);
    void (*compiler_barrier)(void);
    void (*full_memory_barrier)(void);
    void (*load_load_barrier)(void);
    void (*store_store_barrier)(void);
    void (*load_store_barrier)(void);
    void (*store_load_barrier)(void);
} Stru_CN_AtomicInterface_t;
```

## 函数参考

### 管理函数

#### CN_atomic_get_default

```c
const Stru_CN_AtomicInterface_t* CN_atomic_get_default(void);
```

**描述**：获取默认原子操作接口。

**参数**：无

**返回值**：指向默认原子操作接口的指针。如果初始化失败，返回NULL。

**注意**：首次调用时会初始化默认接口。如果已设置自定义接口，则返回自定义接口。

#### CN_atomic_create_custom

```c
Stru_CN_AtomicInterface_t* CN_atomic_create_custom(const Stru_CN_AtomicInterface_t* interface);
```

**描述**：创建自定义原子操作接口。

**参数**：
- `interface`：自定义接口实现的指针。如果为NULL，函数返回NULL。

**返回值**：新创建的自定义接口指针。如果内存分配失败，返回NULL。

**注意**：创建自定义接口后，`CN_atomic_get_default()`将返回此自定义接口。如果已存在自定义接口，它将被替换。

#### CN_atomic_destroy

```c
void CN_atomic_destroy(Stru_CN_AtomicInterface_t* interface);
```

**描述**：销毁原子操作接口。

**参数**：
- `interface`：要销毁的原子操作接口指针。

**返回值**：无

**注意**：只能销毁通过`CN_atomic_create_custom()`创建的自定义接口。销毁默认接口是无效操作。

### 便捷函数（顺序一致性）

以下函数使用顺序一致性内存顺序（`Eum_MEMORY_ORDER_SEQ_CST`），提供简化的接口。

#### CN_atomic_load_i32

```c
int32_t CN_atomic_load_i32(const volatile int32_t* ptr);
```

**描述**：原子加载32位整数。

**参数**：
- `ptr`：指向原子变量的指针。

**返回值**：加载的值。

#### CN_atomic_store_i32

```c
void CN_atomic_store_i32(volatile int32_t* ptr, int32_t value);
```

**描述**：原子存储32位整数。

**参数**：
- `ptr`：指向原子变量的指针。
- `value`：要存储的值。

**返回值**：无

#### CN_atomic_exchange_i32

```c
int32_t CN_atomic_exchange_i32(volatile int32_t* ptr, int32_t value);
```

**描述**：原子交换32位整数。

**参数**：
- `ptr`：指向原子变量的指针。
- `value`：新值。

**返回值**：旧值。

#### CN_atomic_compare_exchange_i32

```c
bool CN_atomic_compare_exchange_i32(volatile int32_t* ptr, int32_t* expected, int32_t desired);
```

**描述**：原子比较并交换32位整数。

**参数**：
- `ptr`：指向原子变量的指针。
- `expected`：指向期望值的指针。如果比较成功，`*expected`保持不变；如果失败，`*expected`被更新为当前值。
- `desired`：期望的新值。

**返回值**：如果交换成功返回`true`，否则返回`false`。

#### CN_atomic_fetch_add_i32

```c
int32_t CN_atomic_fetch_add_i32(volatile int32_t* ptr, int32_t value);
```

**描述**：原子加法（返回旧值）。

**参数**：
- `ptr`：指向原子变量的指针。
- `value`：要加的值。

**返回值**：加法前的值。

#### CN_atomic_fetch_sub_i32

```c
int32_t CN_atomic_fetch_sub_i32(volatile int32_t* ptr, int32_t value);
```

**描述**：原子减法（返回旧值）。

**参数**：
- `ptr`：指向原子变量的指针。
- `value`：要减的值。

**返回值**：减法前的值。

#### CN_atomic_fetch_increment_i32

```c
int32_t CN_atomic_fetch_increment_i32(volatile int32_t* ptr);
```

**描述**：原子递增（返回旧值）。

**参数**：
- `ptr`：指向原子变量的指针。

**返回值**：递增前的值。

**注意**：等价于`CN_atomic_fetch_add_i32(ptr, 1)`。

#### CN_atomic_fetch_decrement_i32

```c
int32_t CN_atomic_fetch_decrement_i32(volatile int32_t* ptr);
```

**描述**：原子递减（返回旧值）。

**参数**：
- `ptr`：指向原子变量的指针。

**返回值**：递减前的值。

**注意**：等价于`CN_atomic_fetch_sub_i32(ptr, 1)`。

#### CN_atomic_full_memory_barrier

```c
void CN_atomic_full_memory_barrier(void);
```

**描述**：全内存屏障（顺序一致性）。

**参数**：无

**返回值**：无

**注意**：这是最强的内存屏障，确保所有内存操作按程序顺序执行。

#### CN_atomic_compiler_barrier

```c
void CN_atomic_compiler_barrier(void);
```

**描述**：编译器内存屏障。

**参数**：无

**返回值**：无

**注意**：仅防止编译器重排，不生成CPU指令。适用于硬件寄存器访问等场景。

## 内存顺序语义

### Relaxed（宽松顺序）
- 仅保证原子性，无顺序约束
- 适用于计数器等不需要同步的场景
- 性能最好

### Acquire（获取顺序）
- 确保后续的读/写操作不会重排到屏障之前
- 适用于读取共享数据后的同步
- 典型模式：加载（acquire）→ 使用数据

### Release（释放顺序）
- 确保之前的读/写操作不会重排到屏障之后
- 适用于发布共享数据前的同步
- 典型模式：准备数据 → 存储（release）

### Acquire-Release（获取-释放顺序）
- 同时具有acquire和release语义
- 适用于锁的实现
- 典型模式：锁获取（acquire）→ 临界区 → 锁释放（release）

### Sequential Consistency（顺序一致性）
- 最强的内存顺序
- 所有操作按程序顺序执行
- 性能最差，但最容易理解

## 使用示例

### 示例1：原子计数器

```c
#include "CN_atomic.h"

// 全局原子计数器
static volatile int32_t g_counter = 0;

// 线程安全的计数器递增
void increment_counter(void)
{
    CN_atomic_fetch_add_i32(&g_counter, 1);
}

// 获取计数器值
int32_t get_counter(void)
{
    return CN_atomic_load_i32(&g_counter);
}

// 重置计数器
void reset_counter(int32_t value)
{
    CN_atomic_store_i32(&g_counter, value);
}
```

### 示例2：自旋锁实现

```c
#include "CN_atomic.h"

// 简单的自旋锁
typedef struct {
    volatile int32_t lock;
} SpinLock;

// 初始化锁
void spinlock_init(SpinLock* lock)
{
    CN_atomic_store_i32(&lock->lock, 0);
}

// 获取锁
void spinlock_lock(SpinLock* lock)
{
    int32_t expected = 0;
    int32_t desired = 1;
    
    // 使用CAS实现锁获取
    while (!CN_atomic_compare_exchange_i32(&lock->lock, &expected, desired)) {
        expected = 0; // 重置期望值
        // 可选：添加退避策略
    }
    
    // 获取顺序屏障，确保临界区内的操作不会重排到锁获取之前
    CN_atomic_full_memory_barrier();
}

// 释放锁
void spinlock_unlock(SpinLock* lock)
{
    // 释放顺序屏障，确保临界区内的操作不会重排到锁释放之后
    CN_atomic_full_memory_barrier();
    
    // 释放锁
    CN_atomic_store_i32(&lock->lock, 0);
}
```

### 示例3：发布-订阅模式

```c
#include "CN_atomic.h"

// 共享数据结构
typedef struct {
    volatile int32_t ready;
    int32_t data[100];
} SharedData;

// 发布者线程
void publisher(SharedData* shared)
{
    // 准备数据
    for (int i = 0; i < 100; i++) {
        shared->data[i] = i * i;
    }
    
    // 发布数据（使用释放顺序）
    CN_atomic_full_memory_barrier(); // 确保数据写入在标志写入之前完成
    CN_atomic_store_i32(&shared->ready, 1);
}

// 订阅者线程
void subscriber(SharedData* shared)
{
    // 等待数据就绪
    while (CN_atomic_load_i32(&shared->ready) == 0) {
        // 忙等待或让出CPU
    }
    
    // 获取顺序屏障，确保标志读取在数据读取之前完成
    CN_atomic_full_memory_barrier();
    
    // 安全地读取数据
    for (int i = 0; i < 100; i++) {
        printf("data[%d] = %d\n", i, shared->data[i]);
    }
}
```

## 平台支持

### 支持的平台
- **Windows**：使用Interlocked系列函数和内存屏障指令
- **Linux/macOS**：使用GCC/Clang的`__atomic`内置函数
- **其他平台**：提供回退实现（非原子版本）

### 编译器要求
- GCC 4.7+ 或 Clang 3.0+：支持完整的`__atomic`内置函数
- MSVC 2012+：支持Interlocked函数和内存屏障
- 其他编译器：使用回退实现

## 错误处理

### 返回值错误
- 所有返回布尔值的函数：`true`表示成功，`false`表示失败
- 指针返回函数：NULL表示失败
- 数值返回函数：无错误返回值（使用前应验证参数）

### 参数验证
- 空指针检查：部分函数检查NULL指针
- 内存对齐：原子操作要求适当的内存对齐
- 平台限制：某些平台可能不支持64位原子操作

## 性能考虑

### 内存顺序影响
- **Relaxed**：最快，适合不需要同步的场景
- **Acquire/Release**：中等，适合锁和同步原语
- **Seq_Cst**：最慢，但提供最强的保证

### 平台差异
- x86/x64：大多数操作具有隐式内存屏障
- ARM/PowerPC：需要显式内存屏障指令
- 嵌入式系统：可能不支持某些原子操作

### 优化建议
1. 使用最弱的内存顺序满足需求
2. 避免不必要的内存屏障
3. 批量操作减少原子操作次数
4. 使用缓存友好的数据结构

## 线程安全

### 原子性保证
- 所有原子操作保证在并发访问下的原子性
- 即使在不支持硬件原子操作的平台上，也通过软件锁提供原子性保证

### 内存可见性
- 内存屏障确保操作在不同线程间的可见性
- 遵循C11/C++11内存模型

### 重排序限制
- 内存顺序约束限制编译器和CPU的重排序
- 确保多线程程序的正确性

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| v1.0.0 | 2026-01-03 | 初始版本，实现基本原子操作和内存屏障 |
| v1.1.0 | 计划 | 添加64位原子操作和指针操作 |
| v1.2.0 | 计划 | 添加浮点数原子操作和事务内存支持 |

## 相关文档

- [CN_Language架构设计原则](../architecture/架构设计原则.md)
- [CN_memory API文档](./CN_memory_API.md)
- [C11原子操作标准](https://en.cppreference.com/w/c/atomic)
- [C++11内存模型](https://en.cppreference.com/w/cpp/atomic/memory_order)

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件
