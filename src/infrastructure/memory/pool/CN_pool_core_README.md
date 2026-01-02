# CN_pool_core 模块

## 概述

`CN_pool_core.c` 是CN_Language对象池分配器的核心模块，负责对象池的基本数据结构和生命周期管理。

## 功能职责

### 核心数据结构
- `Stru_CN_PoolBlock_t`: 对象池内存块结构
- `Stru_CN_PoolAllocator_t`: 对象池分配器主结构

### 核心功能
1. **对象池创建和销毁**
   - `CN_pool_create()`: 创建对象池实例
   - `CN_pool_destroy()`: 销毁对象池实例

2. **内存块管理**
   - `create_pool_block()`: 创建新的内存块
   - `destroy_pool_block()`: 销毁内存块
   - `expand_pool()`: 扩展对象池容量

3. **内部辅助函数**
   - `pool_debug_log()`: 调试日志输出
   - `allocate_from_block()`: 在块中分配对象
   - `free_in_block()`: 释放块中的对象

## 设计原则

### 单一职责
本模块专注于对象池的核心数据结构和生命周期管理，不涉及具体的分配/释放业务逻辑。

### 模块化设计
- 独立编译单元，可单独测试
- 通过`CN_pool_internal.h`共享内部数据结构
- 清晰的接口边界

### 内存安全
- 完整的错误检查
- 资源泄漏防护
- 边界条件处理

## 依赖关系

### 内部依赖
- `CN_pool_internal.h`: 内部数据结构定义
- `CN_pool_allocator.h`: 公共接口定义

### 外部依赖
- C标准库: `stdlib.h`, `string.h`, `stdio.h`
- 标准头文件: `stdarg.h`, `assert.h`

## 使用示例

```c
#include "CN_pool_allocator.h"

// 创建对象池
Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
config.object_size = sizeof(MyData);
config.initial_capacity = 100;

Stru_CN_PoolAllocator_t* pool = CN_pool_create(&config);
if (pool == NULL) {
    // 处理错误
}

// 使用对象池...

// 销毁对象池
CN_pool_destroy(pool);
```

## 性能特性

### 时间复杂度
- 创建: O(1)
- 销毁: O(n) - n为内存块数量
- 扩展: O(1)

### 空间复杂度
- 固定开销: 每个对象池约100字节
- 动态开销: 每个内存块有少量管理开销

## 测试覆盖

### 单元测试
- 创建/销毁测试
- 内存块管理测试
- 边界条件测试

### 集成测试
- 与操作模块集成测试
- 与统计模块集成测试

## 维护说明

### 代码规范
- 遵循项目编码标准
- 完整的Doxygen注释
- 错误处理一致

### 修改记录
- 2026-01-02: 从CN_pool_allocator.c拆分，创建核心模块
- 2026-01-02: 优化内存块管理算法

## 相关文档

- [CN_pool_allocator_README.md](./CN_pool_allocator_README.md) - 对象池总览
- [CN_pool_allocator_API.md](../../../docs/api/CN_pool_allocator_API.md) - API文档
- [架构设计原则](../../../../docs/architecture/架构设计原则.md) - 项目架构

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件
