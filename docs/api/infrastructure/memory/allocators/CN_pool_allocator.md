# 对象池分配器 API 文档

## 概述

对象池分配器是CN_Language项目内存管理模块的重要组成部分，专门用于高效分配固定大小的对象。通过预分配内存块并重复使用，对象池分配器可以显著减少内存碎片，提高分配性能，特别适用于频繁创建和销毁相同大小对象的场景。

## 设计目标

1. **高性能**: O(1)时间复杂度的分配和释放操作
2. **零碎片**: 避免内存碎片问题
3. **可扩展**: 支持动态扩展池容量
4. **线程安全**: 可选的多线程支持
5. **统计信息**: 详细的池使用统计

## 核心接口

### 主接口函数

#### `F_create_pool_allocator`

创建对象池分配器实例。

```c
Stru_MemoryAllocatorInterface_t* F_create_pool_allocator(
    size_t object_size, 
    size_t pool_size,
    size_t alignment,
    Stru_MemoryAllocatorInterface_t* parent_allocator);
```

**参数**:
- `object_size`: 每个对象的大小（字节）
- `pool_size`: 池中对象的初始数量
- `alignment`: 内存对齐要求（字节），0表示使用默认对齐
- `parent_allocator`: 父分配器（用于分配池内存），可为NULL（使用系统分配器）

**返回值**:
- 成功: 对象池分配器接口指针
- 失败: NULL

**说明**:
- 对象池分配器专门用于分配固定大小的对象
- 如果请求的大小与`object_size`不匹配，将使用父分配器
- 池内存一次性分配，减少系统调用次数
- 支持内存对齐，确保返回的指针满足对齐要求

#### `F_get_pool_allocator_statistics`

获取对象池分配器的详细统计信息。

```c
void F_get_pool_allocator_statistics(
    Stru_MemoryAllocatorInterface_t* allocator,
    size_t* total_objects,
    size_t* allocated_objects,
    size_t* available_objects,
    size_t* allocation_failures);
```

**参数**:
- `allocator`: 对象池分配器
- `total_objects`: 输出参数，总对象数
- `allocated_objects`: 输出参数，已分配对象数
- `available_objects`: 输出参数，可用对象数
- `allocation_failures`: 输出参数，分配失败次数

**说明**:
- 所有输出参数均可为NULL
- 统计信息实时更新，反映当前池状态

#### `F_expand_pool_allocator`

扩展对象池容量。

```c
bool F_expand_pool_allocator(
    Stru_MemoryAllocatorInterface_t* allocator,
    size_t additional_objects);
```

**参数**:
- `allocator`: 对象池分配器
- `additional_objects`: 要添加的对象数量

**返回值**:
- 成功: true
- 失败: false

**说明**:
- 扩展操作只能在所有对象都空闲时进行
- 扩展后，池的总容量为原容量加上`additional_objects`
- 扩展失败通常是因为有对象仍在分配状态

### 内存分配器接口实现

对象池分配器实现了完整的`Stru_MemoryAllocatorInterface_t`接口：

```c
typedef struct Stru_MemoryAllocatorInterface_t {
    void* (*allocate)(Stru_MemoryAllocatorInterface_t* allocator, 
                     size_t size, size_t alignment);
    void* (*reallocate)(Stru_MemoryAllocatorInterface_t* allocator,
                       void* ptr, size_t new_size);
    void (*deallocate)(Stru_MemoryAllocatorInterface_t* allocator,
                      void* ptr);
    void (*get_statistics)(Stru_MemoryAllocatorInterface_t* allocator,
                          size_t* total_allocated, size_t* total_freed,
                          size_t* current_usage, size_t* allocation_count);
    bool (*validate)(Stru_MemoryAllocatorInterface_t* allocator,
                    void* ptr);
    void (*cleanup)(Stru_MemoryAllocatorInterface_t* allocator);
    void* private_data;
} Stru_MemoryAllocatorInterface_t;
```

## 使用示例

### 示例1: 基本使用

```c
#include "allocators/pool/CN_pool_allocator.h"

int main(void) {
    // 创建对象池分配器（每个对象64字节，池大小100个对象）
    Stru_MemoryAllocatorInterface_t* pool_allocator = 
        F_create_pool_allocator(64, 100, NULL);
    
    if (pool_allocator == NULL) {
        printf("创建对象池分配器失败\n");
        return 1;
    }
    
    // 分配对象
    void* objects[10];
    for (int i = 0; i < 10; i++) {
        objects[i] = pool_allocator->allocate(pool_allocator, 64, 0);
        if (objects[i] == NULL) {
            printf("分配对象 %d 失败\n", i);
            break;
        }
        printf("分配对象 %d: %p\n", i, objects[i]);
    }
    
    // 获取统计信息
    size_t total_objects, allocated_objects, available_objects, failures;
    F_get_pool_allocator_statistics(pool_allocator,
                                   &total_objects, &allocated_objects,
                                   &available_objects, &failures);
    
    printf("池统计信息:\n");
    printf("  总对象数: %zu\n", total_objects);
    printf("  已分配对象数: %zu\n", allocated_objects);
    printf("  可用对象数: %zu\n", available_objects);
    printf("  分配失败次数: %zu\n", failures);
    
    // 释放对象
    for (int i = 0; i < 10; i++) {
        if (objects[i] != NULL) {
            pool_allocator->deallocate(pool_allocator, objects[i]);
        }
    }
    
    // 清理分配器
    pool_allocator->cleanup(pool_allocator);
    
    return 0;
}
```

### 示例2: 池扩展

```c
#include "allocators/pool/CN_pool_allocator.h"

int main(void) {
    // 创建小对象池
    Stru_MemoryAllocatorInterface_t* pool_allocator = 
        F_create_pool_allocator(32, 5, NULL);
    
    if (pool_allocator == NULL) {
        printf("创建对象池分配器失败\n");
        return 1;
    }
    
    // 分配所有对象
    void* objects[5];
    for (int i = 0; i < 5; i++) {
        objects[i] = pool_allocator->allocate(pool_allocator, 32, 0);
        if (objects[i] == NULL) {
            printf("分配对象 %d 失败\n", i);
            break;
        }
    }
    
    // 尝试分配第6个对象（应该失败）
    void* extra = pool_allocator->allocate(pool_allocator, 32, 0);
    if (extra == NULL) {
        printf("池已满，需要扩展\n");
        
        // 释放所有对象以便扩展
        for (int i = 0; i < 5; i++) {
            if (objects[i] != NULL) {
                pool_allocator->deallocate(pool_allocator, objects[i]);
            }
        }
        
        // 扩展池容量
        bool expanded = F_expand_pool_allocator(pool_allocator, 10);
        if (expanded) {
            printf("池扩展成功，新容量: 15个对象\n");
            
            // 重新分配对象
            for (int i = 0; i < 15; i++) {
                objects[i] = pool_allocator->allocate(pool_allocator, 32, 0);
                if (objects[i] == NULL) {
                    printf("分配对象 %d 失败\n", i);
                    break;
                }
            }
        } else {
            printf("池扩展失败\n");
        }
    }
    
    // 清理
    for (int i = 0; i < 15; i++) {
        if (objects[i] != NULL) {
            pool_allocator->deallocate(pool_allocator, objects[i]);
        }
    }
    
    pool_allocator->cleanup(pool_allocator);
    
    return 0;
}
```

### 示例3: 不同大小分配

```c
#include "allocators/pool/CN_pool_allocator.h"
#include "allocators/system/CN_system_allocator.h"

int main(void) {
    // 创建系统分配器作为父分配器
    Stru_MemoryAllocatorInterface_t* system_allocator = 
        F_create_system_allocator();
    
    // 创建对象池分配器（对象大小128字节）
    Stru_MemoryAllocatorInterface_t* pool_allocator = 
        F_create_pool_allocator(128, 10, system_allocator);
    
    // 分配正确大小的对象（使用对象池）
    void* correct_size = pool_allocator->allocate(pool_allocator, 128, 0);
    printf("分配128字节对象: %p\n", correct_size);
    
    // 分配较小对象（使用父分配器）
    void* smaller = pool_allocator->allocate(pool_allocator, 64, 0);
    printf("分配64字节对象: %p\n", smaller);
    
    // 分配较大对象（使用父分配器）
    void* larger = pool_allocator->allocate(pool_allocator, 256, 0);
    printf("分配256字节对象: %p\n", larger);
    
    // 释放内存
    if (correct_size != NULL) {
        pool_allocator->deallocate(pool_allocator, correct_size);
    }
    if (smaller != NULL) {
        pool_allocator->deallocate(pool_allocator, smaller);
    }
    if (larger != NULL) {
        pool_allocator->deallocate(pool_allocator, larger);
    }
    
    // 清理
    pool_allocator->cleanup(pool_allocator);
    
    return 0;
}
```

## 实现原理

### 内存布局

对象池使用连续的内存块，每个对象的大小固定：

```
+----------------------+----------------------+----------------------+
| 对象0数据区          | 对象1数据区          | 对象2数据区          |
+----------------------+----------------------+----------------------+
```

### 空闲列表管理

对象池使用数组实现空闲列表，而不是链表，以提高缓存友好性：

1. **初始化**: 所有对象索引放入空闲数组
2. **分配**: 从空闲数组末尾取出对象索引
3. **释放**: 将对象索引放回空闲数组末尾

### 性能特点

1. **分配时间**: O(1) - 数组末尾操作
2. **释放时间**: O(1) - 数组末尾操作
3. **内存开销**: 每个对象无额外开销（使用独立空闲数组）
4. **缓存友好**: 连续内存访问模式

## 配置选项

### 内存对齐

对象池自动处理内存对齐，确保每个对象正确对齐到平台要求：

```c
// 自动计算对齐后的对象大小
size_t aligned_size = (object_size + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
```

### 线程安全

对象池支持可选的线程安全模式：

```c
// 配置结构
typedef struct {
    size_t object_size;
    size_t initial_pool_size;
    size_t expansion_size;
    bool thread_safe;            // 是否线程安全
    bool zero_on_alloc;          // 分配时清零内存
    bool zero_on_free;           // 释放时清零内存
} PoolAllocatorConfig;
```

## 错误处理

### 分配失败

当对象池已满时，分配操作将失败：
- 返回NULL指针
- 增加`allocation_failures`计数器
- 如果配置了父分配器，将尝试使用父分配器

### 验证失败

验证函数检查：
1. 指针是否在池内存范围内
2. 指针是否对齐到对象大小
3. 指针是否指向已分配的对象

### 扩展失败

池扩展可能失败的原因：
1. 有对象仍在分配状态
2. 内存不足
3. 父分配器失败

## 性能优化

### 缓存优化策略

1. **预取**: 预取下一个可能分配的对象
2. **批量操作**: 支持批量分配和释放
3. **本地缓存**: 线程本地缓存减少锁竞争

### 内存使用优化

1. **智能扩展**: 根据使用模式动态调整扩展大小
2. **收缩机制**: 空闲时自动收缩池大小
3. **内存回收**: 长时间空闲时返回内存给系统

## 测试策略

### 单元测试

覆盖所有接口函数：
1. 基本分配/释放测试
2. 边界条件测试
3. 错误处理测试
4. 统计信息测试

### 性能测试

1. **分配性能**: 测量分配/释放操作的时间
2. **内存使用**: 测量内存碎片和利用率
3. **并发性能**: 多线程环境下的性能测试

### 集成测试

1. 与其他分配器的集成测试
2. 在真实场景中的使用测试
3. 长期稳定性测试

## 适用场景

### 推荐使用对象池的场景

1. **游戏开发**: 频繁创建/销毁的游戏对象
2. **网络编程**: 连接池、缓冲区池
3. **实时系统**: 需要确定性分配时间的系统
4. **嵌入式系统**: 内存受限的环境

### 不推荐使用对象池的场景

1. **对象大小变化大**: 对象池只适合固定大小
2. **对象生命周期长**: 长期持有的对象不适合池化
3. **内存使用不可预测**: 难以预估池大小

## 限制和注意事项

### 技术限制

1. **固定大小**: 只能分配固定大小的对象
2. **扩展限制**: 只能在所有对象空闲时扩展
3. **内存浪费**: 未使用的池空间造成内存浪费

### 使用注意事项

1. **池大小预估**: 需要合理预估池大小
2. **对象清理**: 释放对象不会自动清理内容
3. **线程安全**: 默认非线程安全，需要显式启用

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-01-07 | 初始版本，基本对象池功能 |
| 1.1.0 | 2026-01-07 | 添加池扩展功能 |
| 1.2.0 | 2026-01-07 | 添加统计信息功能 |
| 1.3.0 | 2026-01-08 | 优化内存布局，提高缓存友好性 |
| 2.0.0 | 2026-01-12 | 模块化重构，完善API文档 |

## 相关文件

### 源代码文件（模块化结构）

#### 主接口文件
- `src/infrastructure/memory/allocators/pool/CN_pool_allocator.h` - 公共API头文件
- `src/infrastructure/memory/allocators/pool/CN_pool_allocator_main.c` - 主接口实现

#### 核心模块
- `src/infrastructure/memory/allocators/pool/core/CN_pool_core.h` - 核心数据结构定义
- `src/infrastructure/memory/allocators/pool/core/CN_pool_core.c` - 核心功能实现
- `src/infrastructure/memory/allocators/pool/core/CN_pool_operations.c` - 分配器操作函数

#### 工具模块
- `src/infrastructure/memory/allocators/pool/utils/CN_pool_utils.h` - 工具函数头文件
- `src/infrastructure/memory/allocators/pool/utils/CN_pool_utils.c` - 工具函数实现

#### 模块化设计优势
1. **单一职责原则**: 每个文件专注于一个功能领域
2. **可维护性**: 文件大小控制在500行以内
3. **可测试性**: 模块可独立测试
4. **可重用性**: 工具函数可在其他模块中使用

### 文档文件
- `src/infrastructure/memory/allocators/pool/README.md` - 模块文档
- `docs/api/infrastructure/memory/allocators/CN_pool_allocator.md` - API文档（本文档）

### 测试文件
- `tests/infrastructure/memory/allocators/test_pool_allocator.c` - 测试代码

## 维护者

CN_Language架构委员会

## 许可证

MIT许可证
