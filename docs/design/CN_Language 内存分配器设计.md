# CN Language 内存分配器

## 概述

CN Language 内存分配器是一个高级的内存管理系统，提供多种分配策略、内存池管理和内存泄漏检测功能。它专为系统编程和高性能应用设计。

## 主要特性

### 1. 多种分配策略

- **首次适应 (First Fit)**: 使用第一个足够大的空闲块，速度快
- **最佳适应 (Best Fit)**: 使用最小的足够大的空闲块，节省空间
- **最差适应 (Worst Fit)**: 使用最大的空闲块，减少碎片
- **下次适应 (Next Fit)**: 从上次分配位置继续查找

### 2. 内存池管理

- 支持创建多个独立的内存池
- 每个池可以使用不同的分配策略
- 池可以命名和查找
- 支持池重置和复用

### 3. 内存泄漏检测

- 自动跟踪所有分配和释放
- 详细的泄漏报告（包含文件名、行号、时间戳）
- 可以在开发和生产环境中启用/禁用

### 4. 性能统计

- 实时统计内存使用情况
- 记录峰值使用量
- 统计分配/释放次数
- 内存碎片分析

### 5. 内存整理

- 自动合并相邻空闲块
- 减少内存碎片
- 提高分配效率

## 使用方法

### 基本使用

```c
#include "cnlang/runtime/allocator.h"

// 1. 创建分配器
CnAllocator *allocator = cn_allocator_create(
    1024 * 1024,                    // 默认池大小 (1MB)
    CN_ALLOC_STRATEGY_FIRST_FIT     // 默认策略
);

// 2. 创建内存池
CnMemPool *pool = cn_pool_create(
    allocator,                      // 分配器
    "my_pool",                      // 池名称
    256 * 1024,                     // 池大小 (256KB)
    CN_ALLOC_STRATEGY_BEST_FIT      // 分配策略
);

// 3. 分配内存
void *ptr = cn_pool_alloc(pool, 100);

// 4. 使用内存
// ... your code ...

// 5. 释放内存
cn_pool_free(pool, ptr);

// 6. 销毁分配器（自动销毁所有池）
cn_allocator_destroy(allocator);
```

### 内存泄漏检测

```c
// 启用泄漏检测
cn_allocator_set_leak_detection(allocator, true);

// 分配内存
void *ptr1 = cn_pool_alloc(pool, 100);
void *ptr2 = cn_pool_alloc(pool, 200);

// 只释放一部分
cn_pool_free(pool, ptr1);
// ptr2 未释放，会被检测为泄漏

// 检查泄漏
size_t leak_count = cn_allocator_check_leaks(allocator);
printf("泄漏数量: %zu\n", leak_count);

// 打印详细泄漏报告
cn_allocator_print_leak_report(allocator);
```

### 统计信息

```c
// 获取池统计
size_t total, used, free, blocks;
cn_pool_get_stats(pool, &total, &used, &free, &blocks);

// 打印池统计
cn_pool_print_stats(pool);

// 打印分配器统计（所有池）
cn_allocator_print_stats(allocator);
```

### 内存整理

```c
// 合并空闲块
size_t merged = cn_pool_merge_free_blocks(pool);
printf("合并了 %zu 个块\n", merged);

// 完整的碎片整理
bool success = cn_pool_defragment(pool);
```

## 分配策略选择指南

| 策略 | 优点 | 缺点 | 适用场景 |
|------|------|------|----------|
| 首次适应 | 速度快 | 可能产生碎片 | 通用场景、性能优先 |
| 最佳适应 | 节省空间 | 搜索慢 | 内存受限场景 |
| 最差适应 | 减少碎片 | 大块消耗快 | 大小对象混合 |
| 下次适应 | 平衡性能 | 局部性较差 | 顺序分配场景 |

## 性能考虑

### 时间复杂度

- **首次适应**: O(n) - 线性搜索
- **最佳适应**: O(n) - 需要遍历所有块
- **最差适应**: O(n) - 需要遍历所有块
- **下次适应**: O(n) - 但平均更快

### 空间复杂度

每个分配块需要额外的元数据（约40-50字节）：
- 指针信息
- 大小信息
- 调试信息（文件名、行号、时间戳）

### 优化建议

1. **选择合适的池大小**: 避免频繁创建新池
2. **定期整理碎片**: 在非关键路径调用 `cn_pool_merge_free_blocks`
3. **批量分配**: 对于相同大小的对象，考虑批量分配
4. **重用池**: 使用 `cn_pool_reset` 重置池而不是销毁再创建

## 测试

运行测试套件:

```bash
# 构建测试
cmake --build build --target allocator_test

# 运行测试
ctest -C Debug -R allocator_test --output-on-failure
```

测试覆盖:
- ✓ 所有分配策略的正确性
- ✓ 内存泄漏检测
- ✓ 边界条件（零大小、超大分配等）
- ✓ 内存碎片整理
- ✓ 统计功能
- ✓ 性能基准测试
- ✓ 压力测试

## 示例程序

查看完整示例: [examples/allocator_example.c](../../examples/allocator_example.c)

编译运行示例:

```bash
gcc -o allocator_example \
    examples/allocator_example.c \
    src/runtime/memory/allocator.c \
    -Iinclude \
    -O2

./allocator_example
```

## API 参考

详细的 API 文档请参考: [include/cnlang/runtime/allocator.h](../../include/cnlang/runtime/allocator.h)

主要接口:

### 分配器管理
- `cn_allocator_create()` - 创建分配器
- `cn_allocator_destroy()` - 销毁分配器
- `cn_allocator_set_strategy()` - 设置默认策略

### 内存池管理
- `cn_pool_create()` - 创建内存池
- `cn_pool_destroy()` - 销毁内存池
- `cn_pool_find()` - 查找内存池
- `cn_pool_reset()` - 重置内存池

### 内存分配
- `cn_pool_alloc()` - 从池分配
- `cn_pool_free()` - 释放到池
- `cn_allocator_alloc()` - 从分配器分配（自动选池）
- `cn_allocator_free()` - 释放到分配器

### 泄漏检测
- `cn_allocator_set_leak_detection()` - 启用/禁用检测
- `cn_allocator_check_leaks()` - 检查泄漏
- `cn_allocator_print_leak_report()` - 打印报告

### 统计诊断
- `cn_pool_get_stats()` - 获取池统计
- `cn_pool_print_stats()` - 打印池统计
- `cn_allocator_get_stats()` - 获取分配器统计
- `cn_allocator_print_stats()` - 打印分配器统计

## 设计原则

1. **简单易用**: 简洁的 API 设计
2. **灵活可配置**: 支持多种策略和配置
3. **高性能**: 优化的分配算法
4. **可调试**: 详细的调试信息和统计
5. **内存安全**: 自动检测泄漏和错误

## 限制和注意事项

1. **不是线程安全的**: 多线程使用需要外部同步
2. **元数据开销**: 每个块有40-50字节的元数据
3. **碎片整理限制**: 当前实现不移动已分配的内存
4. **调试信息**: 启用调试信息会增加内存开销

## 未来改进

- [ ] 线程安全版本
- [ ] 更高级的碎片整理（移动已分配内存）
- [ ] 内存压缩
- [ ] 更多的分配策略（Buddy System、Slab等）
- [ ] 可视化工具
- [ ] 更详细的性能分析

## 许可证

本项目遵循 CN Language 项目的许可证。

## 贡献

欢迎提交问题报告和改进建议！
