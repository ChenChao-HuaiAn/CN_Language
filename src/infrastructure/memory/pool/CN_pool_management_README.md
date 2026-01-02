# CN_pool_management 模块

## 概述

`CN_pool_management.c` 是CN_Language对象池分配器的管理模块，负责对象池的配置管理、统计查询和批量操作。

## 功能职责

### 配置管理
1. **配置获取**
   - `CN_pool_get_config()`: 获取对象池当前配置
   - 验证参数有效性
   - 返回完整的配置信息

2. **配置验证**
   - 内部配置一致性检查
   - 运行时配置验证

### 统计管理
1. **统计查询**
   - `CN_pool_get_stats()`: 获取对象池统计信息
   - 包括容量、使用率、性能指标

2. **统计重置**
   - `CN_pool_reset_stats()`: 重置计数统计
   - 保留容量和内存使用统计

3. **使用率计算**
   - `CN_pool_utilization()`: 计算对象池使用率
   - 返回0.0到1.0之间的浮点数

### 批量操作
1. **预分配**
   - `CN_pool_prealloc()`: 预分配多个对象
   - 提高首次分配性能
   - 支持自动扩展

2. **池清空**
   - `CN_pool_clear()`: 清空对象池
   - 重置所有空闲对象
   - 不影响已分配对象

3. **池收缩**
   - `CN_pool_shrink()`: 收缩对象池（占位函数）
   - 当前版本未实现
   - 预留未来扩展接口

### 对象验证
- `CN_pool_contains()`: 检查指针是否属于对象池
- 支持已分配和空闲对象检查
- 无效指针检测

## 设计原则

### 单一职责
本模块专注于对象池的管理功能，不涉及具体的分配/释放操作。

### 数据一致性
1. **实时统计**: 统计信息实时更新
2. **配置同步**: 配置信息与运行时状态一致
3. **错误处理**: 完整的参数验证和错误返回

### 可扩展性
1. **模块化设计**: 独立的管理功能模块
2. **接口清晰**: 明确的输入输出契约
3. **未来扩展**: 预留收缩功能接口

## 实现细节

### 统计信息维护
```c
// 统计信息实时更新
- 分配时: allocated_objects++, free_objects--, total_allocations++
- 释放时: allocated_objects--, free_objects++, total_deallocations++
- 扩展时: total_objects += new_capacity, free_objects += new_capacity
```

### 使用率计算
```c
// 使用率 = 已分配对象数 / 总对象数
float utilization = (float)allocated_objects / (float)total_objects;
// 特殊处理: 总对象数为0时返回-1.0
```

### 预分配算法
```c
1. 计算需要预分配的数量
2. 检查当前空闲对象是否足够
3. 如果不足，计算需要扩展的数量
4. 循环扩展直到满足需求
```

## 依赖关系

### 内部依赖
- `CN_pool_internal.h`: 内部数据结构定义
- `CN_pool_core.c`: 核心数据结构和扩展功能

### 外部依赖
- C标准库: `stdlib.h`, `string.h`
- 标准头文件: `stdbool.h`

## 使用示例

```c
#include "CN_pool_allocator.h"

// 获取配置
Stru_CN_PoolConfig_t config;
if (CN_pool_get_config(pool, &config)) {
    printf("对象大小: %zu\n", config.object_size);
}

// 获取统计
Stru_CN_PoolStats_t stats;
if (CN_pool_get_stats(pool, &stats)) {
    printf("使用率: %.2f%%\n", CN_pool_utilization(pool) * 100.0f);
}

// 预分配对象
if (CN_pool_prealloc(pool, 1000)) {
    printf("预分配成功\n");
}

// 检查指针
if (CN_pool_contains(pool, some_ptr)) {
    printf("指针属于对象池\n");
}
```

## 性能特性

### 时间复杂度
- 配置获取: O(1)
- 统计获取: O(1)
- 使用率计算: O(1)
- 预分配: O(n) - n为需要扩展的块数
- 指针检查: O(n) - n为块数

### 空间开销
- 统计信息: 固定大小结构体
- 配置信息: 固定大小结构体

## 测试覆盖

### 单元测试
- 配置管理测试
- 统计功能测试
- 批量操作测试

### 集成测试
- 与核心模块集成测试
- 与操作模块集成测试
- 端到端功能测试

## 维护说明

### 代码规范
- 每个函数功能单一明确
- 完整的参数验证
- 清晰的错误返回

### 修改记录
- 2026-01-02: 从CN_pool_allocator.c拆分，创建管理模块
- 2026-01-02: 优化统计信息更新逻辑

## 相关文档

- [CN_pool_core_README.md](./CN_pool_core_README.md) - 核心模块文档
- [CN_pool_operations_README.md](./CN_pool_operations_README.md) - 操作模块文档
- [CN_pool_allocator_README.md](./CN_pool_allocator_README.md) - 对象池总览

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件
