# 内存统计工具模块

## 概述

内存统计工具模块是CN_Language项目基础设施层内存管理模块的一部分，专注于提供内存使用统计和分析功能。本模块实现了`Stru_MemoryStatisticsInterface_t`接口，提供全面的内存使用统计、性能分析和碎片分析功能，帮助开发者优化内存使用和检测内存问题。

## 设计原则

1. **全面性**：提供全面的内存使用统计信息
2. **低开销**：统计收集对性能影响最小
3. **实时性**：支持实时统计查询和报告
4. **可配置性**：允许按需启用或禁用统计功能

## 接口定义

### Stru_MemoryStatisticsInterface_t

内存统计接口定义：

```c
typedef struct Stru_MemoryStatisticsInterface_t {
    // 内存使用统计
    size_t (*get_total_allocated)(void);
    size_t (*get_total_freed)(void);
    size_t (*get_current_usage)(void);
    size_t (*get_peak_usage)(void);
    
    // 分配统计
    uint64_t (*get_allocation_count)(void);
    uint64_t (*get_deallocation_count)(void);
    uint64_t (*get_failed_allocation_count)(void);
    
    // 性能统计
    double (*get_average_allocation_time)(void);
    double (*get_average_deallocation_time)(void);
    uint64_t (*get_total_allocation_time)(void);
    uint64_t (*get_total_deallocation_time)(void);
    
    // 碎片分析
    size_t (*get_fragmentation_ratio)(void);
    size_t (*get_largest_free_block)(void);
    size_t (*get_smallest_free_block)(void);
    
    // 统计控制
    void (*reset_statistics)(void);
    void (*enable_statistics)(bool enable);
    bool (*is_statistics_enabled)(void);
    
    // 统计报告
    void (*print_statistics_report)(void);
    void (*get_statistics_report)(char* buffer, size_t buffer_size);
} Stru_MemoryStatisticsInterface_t;
```

## API 函数

### 内存使用统计函数

#### `get_total_allocated`

获取总分配内存大小。

```c
size_t get_total_allocated(void);
```

**返回值**：自统计开始以来分配的总字节数

**说明**：
- 包括所有成功分配的内存
- 不包括分配失败的内存
- 统计在启用统计功能后开始

#### `get_total_freed`

获取总释放内存大小。

```c
size_t get_total_freed(void);
```

**返回值**：自统计开始以来释放的总字节数

**说明**：
- 包括所有成功释放的内存
- 统计在启用统计功能后开始

#### `get_current_usage`

获取当前内存使用量。

```c
size_t get_current_usage(void);
```

**返回值**：当前正在使用的内存字节数

**说明**：
- 计算方式：总分配 - 总释放
- 反映当前内存压力
- 可用于内存泄漏检测

#### `get_peak_usage`

获取峰值内存使用量。

```c
size_t get_peak_usage(void);
```

**返回值**：自统计开始以来的峰值内存使用字节数

**说明**：
- 记录历史最高内存使用量
- 用于评估内存需求
- 帮助优化内存分配策略

### 分配统计函数

#### `get_allocation_count`

获取分配次数。

```c
uint64_t get_allocation_count(void);
```

**返回值**：自统计开始以来的分配次数

**说明**：
- 包括所有成功和失败的分配
- 反映内存分配频率
- 用于性能分析

#### `get_deallocation_count`

获取释放次数。

```c
uint64_t get_deallocation_count(void);
```

**返回值**：自统计开始以来的释放次数

**说明**：
- 反映内存释放频率
- 与分配次数比较可检测内存泄漏

#### `get_failed_allocation_count`

获取分配失败次数。

```c
uint64_t get_failed_allocation_count(void);
```

**返回值**：自统计开始以来的分配失败次数

**说明**：
- 反映内存压力情况
- 帮助识别内存不足问题
- 可用于自动调整内存策略

### 性能统计函数

#### `get_average_allocation_time`

获取平均分配时间。

```c
double get_average_allocation_time(void);
```

**返回值**：平均分配时间（微秒）

**说明**：
- 反映内存分配性能
- 用于性能优化
- 受系统负载影响

#### `get_average_deallocation_time`

获取平均释放时间。

```c
double get_average_deallocation_time(void);
```

**返回值**：平均释放时间（微秒）

**说明**：
- 反映内存释放性能
- 用于性能优化
- 通常比分配时间短

#### `get_total_allocation_time`

获取总分配时间。

```c
uint64_t get_total_allocation_time(void);
```

**返回值**：总分配时间（微秒）

**说明**：
- 所有分配操作的总耗时
- 用于性能分析
- 帮助识别性能瓶颈

#### `get_total_deallocation_time`

获取总释放时间。

```c
uint64_t get_total_deallocation_time(void);
```

**返回值**：总释放时间（微秒）

**说明**：
- 所有释放操作的总耗时
- 用于性能分析
- 通常比总分配时间短

### 碎片分析函数

#### `get_fragmentation_ratio`

获取内存碎片率。

```c
size_t get_fragmentation_ratio(void);
```

**返回值**：内存碎片率（百分比）

**说明**：
- 反映内存碎片程度
- 0%表示无碎片，100%表示完全碎片化
- 帮助优化内存分配策略

#### `get_largest_free_block`

获取最大空闲内存块大小。

```c
size_t get_largest_free_block(void);
```

**返回值**：最大连续空闲内存块字节数

**说明**：
- 反映可用内存的连续性
- 影响大内存分配的成功率
- 帮助评估内存健康状况

#### `get_smallest_free_block`

获取最小空闲内存块大小。

```c
size_t get_smallest_free_block(void);
```

**返回值**：最小空闲内存块字节数

**说明**：
- 反映内存碎片细节
- 帮助识别内存浪费
- 用于优化内存分配算法

### 统计控制函数

#### `reset_statistics`

重置所有统计信息。

```c
void reset_statistics(void);
```

**说明**：
- 将所有统计计数器重置为零
- 不影响已分配的内存
- 用于开始新的统计周期

#### `enable_statistics`

启用或禁用统计功能。

```c
void enable_statistics(bool enable);
```

**参数**：
- `enable`：true启用统计，false禁用统计

**说明**：
- 禁用统计可减少性能开销
- 启用时开始收集统计信息
- 禁用时统计信息保持不变

#### `is_statistics_enabled`

检查统计功能是否启用。

```c
bool is_statistics_enabled(void);
```

**返回值**：
- `true`：统计功能已启用
- `false`：统计功能已禁用

**说明**：
- 用于检查当前统计状态
- 帮助调试和监控

### 统计报告函数

#### `print_statistics_report`

打印统计报告到标准输出。

```c
void print_statistics_report(void);
```

**说明**：
- 打印格式化的统计报告
- 包含所有统计信息
- 便于快速查看和分析

#### `get_statistics_report`

获取统计报告字符串。

```c
void get_statistics_report(char* buffer, size_t buffer_size);
```

**参数**：
- `buffer`：输出缓冲区
- `buffer_size`：缓冲区大小

**说明**：
- 将统计报告写入缓冲区
- 便于程序化处理
- 需要确保缓冲区足够大

## 使用示例

### 示例1：基本内存统计

```c
#include "CN_memory_utilities.h"

// 获取内存统计接口
Stru_MemoryStatisticsInterface_t* stats = F_get_memory_statistics();

// 启用统计
stats->enable_statistics(true);

// 进行一些内存操作
void* ptr1 = malloc(1024);
void* ptr2 = malloc(2048);
free(ptr1);
void* ptr3 = malloc(4096);

// 获取统计信息
size_t current_usage = stats->get_current_usage();
size_t peak_usage = stats->get_peak_usage();
uint64_t alloc_count = stats->get_allocation_count();
uint64_t dealloc_count = stats->get_deallocation_count();

printf("当前内存使用: %zu 字节\n", current_usage);
printf("峰值内存使用: %zu 字节\n", peak_usage);
printf("分配次数: %llu\n", alloc_count);
printf("释放次数: %llu\n", dealloc_count);

// 打印完整报告
stats->print_statistics_report();

// 清理
free(ptr2);
free(ptr3);

// 禁用统计
stats->enable_statistics(false);
```

### 示例2：性能分析

```c
#include "CN_memory_utilities.h"
#include <time.h>

// 获取内存统计接口
Stru_MemoryStatisticsInterface_t* stats = F_get_memory_statistics();

// 启用统计
stats->enable_statistics(true);

// 重置统计
stats->reset_statistics();

// 执行性能测试
clock_t start = clock();

// 执行大量内存操作
for (int i = 0; i < 10000; i++) {
    void* ptr = malloc(rand() % 1024 + 1);
    if (ptr != NULL) {
        free(ptr);
    }
}

clock_t end = clock();
double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

// 获取性能统计
double avg_alloc_time = stats->get_average_allocation_time();
double avg_dealloc_time = stats->get_average_deallocation_time();
uint64_t total_alloc_time = stats->get_total_allocation_time();
uint64_t total_dealloc_time = stats->get_total_deallocation_time();

printf("性能测试结果:\n");
printf("  CPU时间: %.6f 秒\n", cpu_time_used);
printf("  平均分配时间: %.3f 微秒\n", avg_alloc_time);
printf("  平均释放时间: %.3f 微秒\n", avg_dealloc_time);
printf("  总分配时间: %llu 微秒\n", total_alloc_time);
printf("  总释放时间: %llu 微秒\n", total_dealloc_time);

// 禁用统计
stats->enable_statistics(false);
```

### 示例3：内存碎片分析

```c
#include "CN_memory_utilities.h"

// 获取内存统计接口
Stru_MemoryStatisticsInterface_t* stats = F_get_memory_statistics();

// 启用统计
stats->enable_statistics(true);

// 模拟内存碎片场景
void* blocks[100];
for (int i = 0; i < 100; i++) {
    blocks[i] = malloc(rand() % 128 + 1);
}

// 释放部分内存块（创建碎片）
for (int i = 0; i < 100; i += 2) {
    free(blocks[i]);
    blocks[i] = NULL;
}

// 分析内存碎片
size_t fragmentation = stats->get_fragmentation_ratio();
size_t largest_free = stats->get_largest_free_block();
size_t smallest_free = stats->get_smallest_free_block();

printf("内存碎片分析:\n");
printf("  碎片率: %zu%%\n", fragmentation);
printf("  最大空闲块: %zu 字节\n", largest_free);
printf("  最小空闲块: %zu 字节\n", smallest_free);

// 清理剩余内存
for (int i = 0; i < 100; i++) {
    if (blocks[i] != NULL) {
        free(blocks[i]);
    }
}

// 禁用统计
stats->enable_statistics(false);
```

## 实现细节

### 统计收集实现

统计收集使用原子操作确保线程安全：

```c
// 统计数据结构
typedef struct {
    _Atomic size_t total_allocated;
    _Atomic size_t total_freed;
    _Atomic size_t peak_usage;
    _Atomic uint64_t allocation_count;
    _Atomic uint64_t deallocation_count;
    _Atomic uint64_t failed_allocation_count;
    _Atomic uint64_t total_allocation_time;
    _Atomic uint64_t total_deallocation_time;
    bool enabled;
} MemoryStatistics;
```

### 性能测量实现

性能测量使用高精度计时器：

```c
uint64_t F_get_current_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

// 在分配开始时记录时间
uint64_t start_time = F_get_current_time_us();

// 执行分配操作
void* ptr = malloc(size);

// 在分配结束时计算耗时
uint64_t end_time = F_get_current_time_us();
uint64_t duration = end_time - start_time;

// 更新统计（原子操作）
atomic_fetch_add(&stats.total_allocation_time, duration);
```

### 碎片分析实现

碎片分析通过扫描内存管理器实现：

```c
size_t F_get_fragmentation_ratio(void) {
    if (stats.total_allocated == 0) {
        return 0;
    }
    
    size_t free_memory = get_total_free_memory();
    size_t largest_free = get_largest_free_block();
    
    if (free_memory == 0 || largest_free == 0) {
        return 0;
    }
    
    // 碎片率 = (1 - 最大空闲块/总空闲内存) * 100
    size_t ratio = 100 - (largest_free * 100 / free_memory);
    return ratio;
}
```

## 性能考虑

### 统计开销

- **原子操作**：使用原子操作确保线程安全，有轻微性能开销
- **选择性启用**：允许按需启用统计，减少不必要开销
- **采样统计**：支持采样模式，进一步减少开销

### 内存开销

- **固定大小结构**：统计数据结构大小固定，内存开销可预测
- **无额外分配**：统计不分配额外内存
- **可配置精度**：支持调整统计精度平衡内存和准确性

### 实时性

- **低延迟查询**：统计查询操作快速完成
- **增量更新**：统计信息增量更新，避免批量处理延迟
- **缓存友好**：统计数据结构紧凑，提高缓存命中率

## 线程安全

所有内存统计函数都是线程安全的：

1. **原子操作**：使用原子操作更新统计计数器
2. **无竞争条件**：精心设计的数据访问模式避免竞争
3. **可重入**：函数可被多个线程同时调用

## 错误处理

### 参数验证

所有函数都对输入参数进行验证：

1. **缓冲区检查**：确保输出缓冲区有效
2. **状态检查**：检查统计功能是否启用
3. **边界检查**：防止统计计数器溢出

### 错误返回值

- 统计函数返回合理的默认值（如0）而不是失败
- 报告函数确保缓冲区不溢出
- 控制函数验证输入参数

## 测试覆盖

### 单元测试

1. **基本统计测试**：
   - 测试内存分配和释放统计
   - 测试峰值使用量记录
   - 测试统计重置功能

2. **性能统计测试**：
   - 测试时间测量准确性
   - 测试平均时间计算
   - 测试总时间累计

3. **碎片分析测试**：
   - 测试碎片率计算
   - 测试空闲块大小检测
   - 测试边界情况

### 集成测试

1. **与内存分配器集成**：
   - 测试实际内存操作的统计
   - 测试多线程环境下的统计
   - 测试长时间运行的统计稳定性

2. **性能测试**：
   - 测试统计功能对性能的影响
   - 测试不同负载下的统计准确性
   - 测试统计查询的性能

3. **压力测试**：
   - 测试高并发下的统计正确性
   - 测试内存压力下的统计功能
   - 测试长时间运行的统计可靠性

## 相关文件

### 源代码文件
- `src/infrastructure/memory/utilities/statistics/CN_memory_statistics.h` - 头文件
- `src/infrastructure/memory/utilities/statistics/CN_memory_statistics.c` - 实现文件

### 测试文件
- `tests/infrastructure/memory/utilities/statistics/test_memory_statistics.c` - 单元测试

### 文档文件
- `docs/api/infrastructure/memory/CN_memory.md` - API文档

## 维护者

CN_Language架构委员会

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-01-08 | 初始版本，包含基本统计功能 |
| 1.1.0 | 2026-01-08 | 添加性能优化和平台特定实现 |
| 1.2.0 | 2026-01-08 | 添加碎片分析和高级统计功能 |
