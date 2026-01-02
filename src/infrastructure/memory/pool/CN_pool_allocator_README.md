# CN_Language 对象池分配器模块

## 概述

对象池分配器是CN_Language内存管理模块的重要组成部分，专门用于高效管理固定大小对象的内存分配和释放。本模块遵循SOLID设计原则和项目架构规范，提供高性能、低碎片的内存管理方案。

## 设计理念

### 核心目标
- **高性能**：减少内存分配/释放的系统调用开销
- **低碎片**：固定大小对象，避免内存碎片
- **可配置**：灵活的配置选项适应不同场景
- **可监控**：详细的统计信息和调试支持

### 架构原则
- **单一职责**：专注于对象池管理，不涉及业务逻辑
- **接口隔离**：提供专用接口，避免"胖接口"
- **依赖倒置**：通过抽象接口与上层模块交互
- **开闭原则**：支持扩展而不修改核心逻辑

## 功能特性

### 1. 核心功能
- **固定大小对象池**：预分配和管理固定大小的对象
- **自动扩展**：根据需要自动扩展池容量
- **对象重用**：释放的对象返回池中供重用
- **内存清零**：可选的分配/释放时内存清零

### 2. 配置选项
- **对象大小**：池中每个对象的大小（字节）
- **初始容量**：池创建时的初始对象数量
- **最大容量**：池的最大对象数量限制（0表示无限制）
- **自动扩展**：是否在池满时自动扩展
- **扩展增量**：每次扩展增加的对象数量
- **线程安全**：是否支持多线程环境
- **内存清零**：分配/释放时是否清零内存

### 3. 统计监控
- **使用统计**：分配次数、释放次数、失败次数
- **容量统计**：总对象数、空闲对象数、已分配对象数
- **内存统计**：内存使用量、峰值使用量
- **性能统计**：池扩展次数、使用率

### 4. 调试支持
- **完整性验证**：验证池内部结构完整性
- **状态转储**：输出详细池状态信息
- **调试回调**：自定义调试信息输出
- **泄漏检测**：跟踪未释放对象

## 架构设计

### 模块结构
```
CN_pool_allocator/
├── CN_pool_allocator.h      # 公共接口定义
├── CN_pool_allocator.c      # 实现代码
└── CN_pool_allocator_README.md  # 模块文档
```

### 内部数据结构
```c
// 对象池块 - 管理一块连续内存中的多个对象
typedef struct Stru_CN_PoolBlock_t
{
    struct Stru_CN_PoolBlock_t* next;  // 下一个块
    void* memory;                      // 块内存起始地址
    size_t capacity;                   // 块容量（对象数量）
    size_t used_count;                 // 已使用对象数量
    unsigned char* usage_bitmap;       // 使用情况位图
} Stru_CN_PoolBlock_t;

// 对象池分配器 - 主结构
struct Stru_CN_PoolAllocator_t
{
    Stru_CN_PoolConfig_t config;       // 配置信息
    Stru_CN_PoolBlock_t* first_block;  // 内存块链表
    Stru_CN_PoolBlock_t* last_block;
    void* free_list;                   // 空闲对象链表
    Stru_CN_PoolStats_t stats;         // 统计信息
    // ... 其他内部状态
};
```

### 内存管理策略
1. **块分配**：以块为单位分配连续内存
2. **位图管理**：使用位图跟踪对象使用状态
3. **空闲链表**：维护最近释放的对象供快速重用
4. **嵌入链表**：在空闲对象中嵌入链表指针，减少内存开销

## 使用指南

### 基本使用
```c
#include "CN_pool_allocator.h"

// 1. 创建对象池配置
Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
config.object_size = sizeof(MyStruct);  // 设置对象大小
config.initial_capacity = 100;          // 初始100个对象
config.auto_expand = true;              // 允许自动扩展
config.expand_increment = 50;           // 每次扩展50个对象

// 2. 创建对象池
Stru_CN_PoolAllocator_t* pool = CN_pool_create(&config);
if (pool == NULL) {
    // 处理创建失败
}

// 3. 分配对象
MyStruct* obj1 = (MyStruct*)CN_pool_alloc(pool);
MyStruct* obj2 = (MyStruct*)CN_pool_alloc(pool);

// 4. 使用对象
obj1->value = 42;
obj2->value = 100;

// 5. 释放对象
CN_pool_free(pool, obj1);
CN_pool_free(pool, obj2);

// 6. 查看统计信息
Stru_CN_PoolStats_t stats;
CN_pool_get_stats(pool, &stats);
printf("使用率: %.2f%%\n", CN_pool_utilization(pool) * 100.0f);

// 7. 销毁对象池
CN_pool_destroy(pool);
```

### 高级功能
```c
// 预分配对象（提高首次分配性能）
CN_pool_prealloc(pool, 1000);

// 验证池完整性
if (!CN_pool_validate(pool)) {
    printf("对象池完整性验证失败！\n");
}

// 转储池状态（调试用）
CN_pool_dump(pool);

// 设置调试回调
void debug_callback(const char* message, void* user_data) {
    printf("[POOL_DEBUG] %s\n", message);
}
CN_pool_set_debug_callback(pool, debug_callback, NULL);

// 检查指针是否属于对象池
if (CN_pool_contains(pool, some_ptr)) {
    printf("指针属于对象池\n");
}

// 清空对象池（重置所有对象为空闲状态）
CN_pool_clear(pool);
```

### 集成到CN_memory系统
```c
#include "CN_memory.h"

// 使用对象池分配器作为全局内存分配器
CN_memory_init(Eum_ALLOCATOR_POOL);

// 现在所有cn_malloc/cn_free调用都使用对象池
void* ptr1 = cn_malloc(64);  // 使用对象池（如果大小匹配）
void* ptr2 = cn_malloc(128); // 使用系统分配器（大小不匹配）

cn_free(ptr1);
cn_free(ptr2);

CN_memory_shutdown();
```

## 性能优化

### 1. 配置优化
```c
// 高性能配置（适合频繁分配释放）
Stru_CN_PoolConfig_t perf_config = {
    .object_size = 64,           // 匹配常用对象大小
    .initial_capacity = 1024,    // 较大的初始容量
    .max_capacity = 0,           // 无限制
    .auto_expand = true,         // 自动扩展
    .expand_increment = 512,     // 较大的扩展增量
    .thread_safe = false,        // 单线程环境
    .zero_on_alloc = false,      // 不清零（性能优先）
    .zero_on_free = false        // 不清零
};

// 安全配置（适合调试和测试）
Stru_CN_PoolConfig_t safe_config = {
    .object_size = 128,
    .initial_capacity = 256,
    .max_capacity = 4096,        // 限制最大容量
    .auto_expand = true,
    .expand_increment = 128,
    .thread_safe = true,         // 线程安全
    .zero_on_alloc = true,       // 分配时清零
    .zero_on_free = true         // 释放时清零
};
```

### 2. 使用模式建议
- **批量操作**：预分配大量对象减少扩展开销
- **大小匹配**：确保对象大小与池配置匹配
- **及时释放**：不再使用的对象及时释放回池
- **监控使用率**：定期检查使用率，调整配置

### 3. 性能监控
```c
// 定期监控性能
void monitor_pool_performance(Stru_CN_PoolAllocator_t* pool) {
    Stru_CN_PoolStats_t stats;
    CN_pool_get_stats(pool, &stats);
    
    float utilization = CN_pool_utilization(pool);
    printf("使用率: %.2f%%, 分配次数: %zu, 失败次数: %zu\n",
           utilization * 100.0f, stats.total_allocations, stats.allocation_failures);
    
    if (utilization > 0.9f) {
        printf("警告：对象池使用率过高，考虑增加容量\n");
    }
    
    if (stats.allocation_failures > 0) {
        printf("警告：有分配失败，可能需要调整配置\n");
    }
}
```

## 错误处理

### 常见错误
```c
// 1. 创建失败
Stru_CN_PoolAllocator_t* pool = CN_pool_create(&config);
if (pool == NULL) {
    // 可能原因：内存不足、无效配置
    fprintf(stderr, "对象池创建失败\n");
    return;
}

// 2. 分配失败
void* ptr = CN_pool_alloc(pool);
if (ptr == NULL) {
    // 可能原因：池已满且无法扩展、内存不足
    fprintf(stderr, "对象分配失败\n");
    
    // 可以尝试系统分配作为后备
    ptr = malloc(desired_size);
    if (ptr == NULL) {
        // 真正的内存不足
        fprintf(stderr, "系统内存不足\n");
        return;
    }
}

// 3. 释放失败
if (!CN_pool_free(pool, ptr)) {
    // 可能原因：指针不属于对象池、重复释放
    fprintf(stderr, "对象释放失败，使用系统释放\n");
    free(ptr);  // 后备方案
}
```

### 调试技巧
```c
// 启用详细调试
void debug_callback(const char* message, void* user_data) {
    FILE* log_file = (FILE*)user_data;
    fprintf(log_file, "[%s] %s\n", timestamp(), message);
}

FILE* log_file = fopen("pool_debug.log", "w");
CN_pool_set_debug_callback(pool, debug_callback, log_file);

// 定期验证完整性
if (!CN_pool_validate(pool)) {
    CN_pool_dump(pool);  // 输出详细状态
    // 采取恢复措施或报告错误
}

// 程序退出前检查
CN_pool_dump(pool);  // 输出最终状态
fclose(log_file);
```

## 线程安全

### 单线程模式
- 默认配置，性能最高
- 适合单线程应用或每个线程独立池

### 多线程模式
```c
// 启用线程安全
Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
config.thread_safe = true;

// 使用注意事项：
// 1. 性能会有一定下降
// 2. 确保正确初始化
// 3. 避免竞争条件
```

### 最佳实践
1. **线程局部池**：每个线程使用独立的对象池
2. **生产者-消费者**：专用池用于特定数据流
3. **锁粒度**：根据访问模式选择合适的锁策略

## 内存安全

### 安全特性
1. **边界检查**：验证指针是否属于对象池
2. **重复释放检测**：防止同一指针多次释放
3. **内存清零**：可选的分配/释放时清零
4. **完整性验证**：定期检查内部数据结构

### 安全配置
```c
// 安全优先的配置
Stru_CN_PoolConfig_t safe_config = {
    .object_size = 256,
    .initial_capacity = 128,
    .max_capacity = 2048,        // 限制最大容量防止失控
    .auto_expand = true,
    .expand_increment = 64,
    .thread_safe = true,         // 线程安全
    .zero_on_alloc = true,       // 分配时清零（防止信息泄漏）
    .zero_on_free = true         // 释放时清零（防止悬空指针）
};
```

## 测试策略

### 单元测试
```c
// 测试创建和销毁
void test_create_destroy(void) {
    Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
    config.object_size = 64;
    
    Stru_CN_PoolAllocator_t* pool = CN_pool_create(&config);
    assert(pool != NULL);
    
    CN_pool_destroy(pool);
    printf("测试通过：创建和销毁\n");
}

// 测试分配和释放
void test_alloc_free(void) {
    Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
    config.object_size = 128;
    
    Stru_CN_PoolAllocator_t* pool = CN_pool_create(&config);
    
    void* ptrs[100];
    for (int i = 0; i < 100; i++) {
        ptrs[i] = CN_pool_alloc(pool);
        assert(ptrs[i] != NULL);
    }
    
    for (int i = 0; i < 100; i++) {
        assert(CN_pool_free(pool, ptrs[i]));
    }
    
    CN_pool_destroy(pool);
    printf("测试通过：分配和释放\n");
}
```

### 压力测试
```c
// 长时间运行测试
void stress_test(void) {
    Stru_CN_PoolConfig_t config = CN_POOL_CONFIG_DEFAULT;
    config.object_size = 256;
    config.initial_capacity = 1000;
    config.auto_expand = true;
    
    Stru_CN_PoolAllocator_t* pool = CN_pool_create(&config);
    
    // 运行多次分配释放循环
    for (int cycle = 0; cycle < 10000; cycle++) {
        void* objects[100];
        for (int i = 0; i < 100; i++) {
            objects[i] = CN_pool_alloc(pool);
            assert(objects[i] != NULL);
            
            // 使用内存
            memset(objects[i], cycle % 256, config.object_size);
        }
        
        for (int i = 0; i < 100; i++) {
            assert(CN_pool_free(pool, objects[i]));
        }
        
        // 每100周期验证一次
        if (cycle % 100 == 0) {
            assert(CN_pool_validate(pool));
        }
    }
    
    CN_pool_destroy(pool);
    printf("压力测试通过\n");
}
```

## 兼容性说明

### 与标准库兼容
- 可以替代malloc/free用于固定大小对象
- 提供类似的接口模式
- 支持与标准库混合使用（通过后备机制）

### 平台支持
- 纯C实现，无平台依赖
- 使用标准库函数
- 可移植到任何支持C的平台

### 版本兼容性
- 保持API向后兼容
- 新增功能通过新接口提供
- 废弃功能有明确标记和替代方案

## 扩展计划

### 短期改进（v1.1）
1. **多尺寸支持**：单个池支持多种对象大小
2. **智能扩展**：根据使用模式动态调整扩展策略
3. **性能分析**：更详细的性能统计和瓶颈分析

### 中期改进（v1.2）
1. **分层池**：支持多级对象池 hierarchy
2. **内存压缩**：自动合并空闲块减少碎片
3. **NUMA优化**：针对NUMA架构的优化

### 长期愿景（v2.0）
1. **分布式池**：支持跨进程/机器的对象池
2. **持久化池**：支持池状态保存和恢复
3. **智能预测**：基于机器学习的分配预测

## 贡献指南

### 代码规范
1. 遵循项目编码规范和命名约定
2. 每个函数不超过50行
3. 每个文件不超过500行
4. 完整的Doxygen风格注释

### 测试要求
1. 为新功能添加单元测试
2. 确保测试覆盖率≥85%
3. 包含边界条件测试
4. 包含性能基准测试

### 文档要求
1. 更新API文档
2. 更新使用示例
3. 记录架构决策
4. 维护变更日志

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 版本历史

### v1.0.0 (2026-01-02)
- 初始版本，完整对象池分配器实现
- 支持基本配置和统计功能
- 集成到CN_memory系统

### v1.0.1 (计划)
- 性能优化和bug修复
- 增强调试功能
- 改进文档和示例

## 参考资源

- [CN_memory模块文档](../CN_memory_API.md)
- [架构设计原则](../../../docs/architecture/架构设计原则.md)
- [编码规范](../../../docs/specifications/CN_Language项目%20技术规范和编码标准.md)
- [性能优化指南](../../../docs/design/中文编程语言CN_Language开发规划.md)
