# 分配器工厂模块 (Allocator Factory)

## 概述

分配器工厂模块是内存分配器模块的一部分，提供统一的分配器创建和管理接口。该模块遵循工厂模式，允许客户端通过统一的接口创建不同类型的分配器，而无需了解具体实现细节。

## 功能特性

### 1. 统一创建接口
- 提供统一的函数创建各种类型的分配器
- 支持通过配置参数定制分配器行为
- 隐藏具体分配器的实现细节

### 2. 分配器管理
- 跟踪已创建的分配器实例
- 提供分配器生命周期管理
- 支持分配器重用和池化

### 3. 配置管理
- 支持从配置文件创建分配器
- 提供默认配置和预设
- 支持运行时配置更新

### 4. 错误处理
- 统一的错误处理机制
- 详细的错误信息和诊断
- 优雅的失败处理

## 模块架构

```
factory/
├── CN_allocator_factory.h      # 工厂接口定义
├── CN_allocator_factory.c      # 工厂实现
├── CN_allocator_config.h       # 配置结构定义
├── CN_allocator_config.c       # 配置管理
└── README.md                   # 本文件
```

## 接口设计

### 分配器工厂接口

```c
/**
 * @brief 分配器工厂接口
 * 
 * 提供统一的分配器创建和管理功能。
 */
typedef struct Stru_AllocatorFactory_t Stru_AllocatorFactory_t;

struct Stru_AllocatorFactory_t {
    /**
     * @brief 创建系统分配器
     * 
     * @param factory 工厂实例
     * @param config 分配器配置（可为NULL）
     * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针
     */
    Stru_MemoryAllocatorInterface_t* (*create_system_allocator)(
        Stru_AllocatorFactory_t* factory,
        const Stru_AllocatorConfig_t* config);
    
    /**
     * @brief 创建调试分配器
     * 
     * @param factory 工厂实例
     * @param config 分配器配置（可为NULL）
     * @param parent_allocator 父分配器（可为NULL）
     * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针
     */
    Stru_MemoryAllocatorInterface_t* (*create_debug_allocator)(
        Stru_AllocatorFactory_t* factory,
        const Stru_AllocatorConfig_t* config,
        Stru_MemoryAllocatorInterface_t* parent_allocator);
    
    /**
     * @brief 创建对象池分配器
     * 
     * @param factory 工厂实例
     * @param config 分配器配置（可为NULL）
     * @param object_size 对象大小
     * @param pool_size 池大小
     * @param parent_allocator 父分配器（可为NULL）
     * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针
     */
    Stru_MemoryAllocatorInterface_t* (*create_pool_allocator)(
        Stru_AllocatorFactory_t* factory,
        const Stru_AllocatorConfig_t* config,
        size_t object_size,
        size_t pool_size,
        Stru_MemoryAllocatorInterface_t* parent_allocator);
    
    /**
     * @brief 创建区域分配器
     * 
     * @param factory 工厂实例
     * @param config 分配器配置（可为NULL）
     * @param region_size 区域大小
     * @param parent_allocator 父分配器（可为NULL）
     * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针
     */
    Stru_MemoryAllocatorInterface_t* (*create_region_allocator)(
        Stru_AllocatorFactory_t* factory,
        const Stru_AllocatorConfig_t* config,
        size_t region_size,
        Stru_MemoryAllocatorInterface_t* parent_allocator);
    
    /**
     * @brief 根据名称创建分配器
     * 
     * @param factory 工厂实例
     * @param allocator_type 分配器类型名称
     * @param config 分配器配置（可为NULL）
     * @param ... 可变参数（根据分配器类型不同）
     * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针
     */
    Stru_MemoryAllocatorInterface_t* (*create_allocator_by_name)(
        Stru_AllocatorFactory_t* factory,
        const char* allocator_type,
        const Stru_AllocatorConfig_t* config,
        ...);
    
    /**
     * @brief 销毁分配器
     * 
     * @param factory 工厂实例
     * @param allocator 要销毁的分配器
     */
    void (*destroy_allocator)(
        Stru_AllocatorFactory_t* factory,
        Stru_MemoryAllocatorInterface_t* allocator);
    
    /**
     * @brief 获取工厂统计信息
     * 
     * @param factory 工厂实例
     * @param total_created 输出：总创建的分配器数
     * @param active_allocators 输出：活动分配器数
     * @param memory_usage 输出：总内存使用量
     */
    void (*get_statistics)(
        Stru_AllocatorFactory_t* factory,
        size_t* total_created,
        size_t* active_allocators,
        size_t* memory_usage);
    
    /**
     * @brief 清理工厂资源
     * 
     * @param factory 工厂实例
     */
    void (*cleanup)(Stru_AllocatorFactory_t* factory);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};
```

### 分配器配置结构

```c
/**
 * @brief 分配器配置结构
 * 
 * 用于配置分配器的各种参数。
 */
typedef struct Stru_AllocatorConfig_t {
    /**
     * @brief 分配器名称（用于调试和日志）
     */
    const char* name;
    
    /**
     * @brief 是否启用线程安全
     */
    bool thread_safe;
    
    /**
     * @brief 是否启用统计信息收集
     */
    bool enable_statistics;
    
    /**
     * @brief 是否启用调试功能
     */
    bool enable_debugging;
    
    /**
     * @brief 内存对齐要求（0表示使用默认）
     */
    size_t alignment;
    
    /**
     * @brief 最大分配大小限制（0表示无限制）
     */
    size_t max_allocation_size;
    
    /**
     * @brief 分配失败回调函数
     */
    void (*allocation_failure_callback)(size_t requested_size);
    
    /**
     * @brief 自定义配置数据
     */
    void* custom_data;
    
    /**
     * @brief 自定义配置数据大小
     */
    size_t custom_data_size;
} Stru_AllocatorConfig_t;
```

## 使用示例

### 1. 创建工厂实例

```c
#include "allocators/factory/CN_allocator_factory.h"

// 创建分配器工厂
Stru_AllocatorFactory_t* factory = F_create_allocator_factory();
if (factory == NULL) {
    // 处理错误
    return;
}
```

### 2. 使用工厂创建分配器

```c
// 创建系统分配器
Stru_MemoryAllocatorInterface_t* system_allocator = 
    factory->create_system_allocator(factory, NULL);

// 创建调试分配器（使用系统分配器作为父分配器）
Stru_MemoryAllocatorInterface_t* debug_allocator = 
    factory->create_debug_allocator(factory, NULL, system_allocator);

// 创建对象池分配器
Stru_MemoryAllocatorInterface_t* pool_allocator = 
    factory->create_pool_allocator(factory, NULL, 256, 100, system_allocator);

// 创建区域分配器
Stru_MemoryAllocatorInterface_t* region_allocator = 
    factory->create_region_allocator(factory, NULL, 1024 * 1024, system_allocator);
```

### 3. 使用配置创建分配器

```c
// 创建配置
Stru_AllocatorConfig_t config = {
    .name = "MyDebugAllocator",
    .thread_safe = true,
    .enable_statistics = true,
    .enable_debugging = true,
    .alignment = 16,
    .max_allocation_size = 1024 * 1024, // 1MB限制
    .allocation_failure_callback = my_failure_callback,
    .custom_data = NULL,
    .custom_data_size = 0
};

// 使用配置创建分配器
Stru_MemoryAllocatorInterface_t* custom_allocator = 
    factory->create_debug_allocator(factory, &config, system_allocator);
```

### 4. 根据名称创建分配器

```c
// 根据名称创建分配器（支持可变参数）
Stru_MemoryAllocatorInterface_t* allocator1 = 
    factory->create_allocator_by_name(factory, "system", NULL);

Stru_MemoryAllocatorInterface_t* allocator2 = 
    factory->create_allocator_by_name(factory, "pool", NULL, 256, 100, system_allocator);

Stru_MemoryAllocatorInterface_t* allocator3 = 
    factory->create_allocator_by_name(factory, "region", NULL, 1024 * 1024, system_allocator);
```

### 5. 清理资源

```c
// 销毁分配器
factory->destroy_allocator(factory, system_allocator);
factory->destroy_allocator(factory, debug_allocator);
factory->destroy_allocator(factory, pool_allocator);
factory->destroy_allocator(factory, region_allocator);

// 清理工厂
factory->cleanup(factory);
```

## 设计原则

### 1. 开闭原则
- 工厂接口对扩展开放，对修改封闭
- 可以添加新的分配器类型而不修改现有代码
- 通过注册机制支持插件式架构

### 2. 单一职责原则
- 工厂只负责分配器的创建和管理
- 配置管理由专门的配置模块处理
- 错误处理由专门的错误处理模块处理

### 3. 依赖倒置原则
- 高层模块通过抽象接口使用分配器
- 工厂返回抽象接口，隐藏具体实现
- 客户端代码不依赖具体分配器实现

### 4. 接口隔离原则
- 为不同客户端提供专用接口
- 工厂接口与分配器接口分离
- 配置接口与工厂接口分离

## 性能考虑

### 1. 创建性能
- 工厂缓存常用分配器配置
- 支持分配器实例池化
- 延迟初始化策略

### 2. 内存开销
- 工厂本身内存开销小
- 配置数据共享机制
- 轻量级跟踪机制

### 3. 线程安全
- 可选线程安全实现
- 细粒度锁策略
- 无锁数据结构支持

## 扩展性

### 1. 添加新的分配器类型
可以通过注册机制添加新的分配器类型：

```c
// 注册新的分配器类型
bool register_allocator_type(
    Stru_AllocatorFactory_t* factory,
    const char* type_name,
    Stru_MemoryAllocatorInterface_t* (*creator)(const Stru_AllocatorConfig_t* config, ...));
```

### 2. 自定义配置
支持自定义配置扩展：

```c
// 定义自定义配置
typedef struct {
    bool custom_feature;
    size_t custom_limit;
} MyCustomConfig;

// 使用自定义配置
Stru_AllocatorConfig_t config = {
    .name = "CustomAllocator",
    .custom_data = &my_custom_config,
    .custom_data_size = sizeof(MyCustomConfig)
};
```

### 3. 插件架构
支持动态加载分配器插件：

```c
// 加载分配器插件
bool load_allocator_plugin(
    Stru_AllocatorFactory_t* factory,
    const char* plugin_path);
```

## 测试策略

### 1. 单元测试
- 工厂创建和销毁测试
- 各种分配器创建测试
- 配置管理测试
- 错误处理测试

### 2. 集成测试
- 工厂与分配器集成测试
- 多分配器协作测试
- 配置持久化测试

### 3. 性能测试
- 工厂创建性能测试
- 多线程创建测试
- 内存泄漏测试

## 注意事项

1. **线程安全**: 默认情况下工厂不是线程安全的，需要显式启用
2. **内存管理**: 工厂创建的所有分配器必须通过工厂销毁
3. **配置生命周期**: 配置对象在创建分配器后可以被销毁
4. **错误处理**: 所有创建函数都可能返回NULL，需要检查返回值
5. **资源清理**: 必须正确清理工厂和所有分配的分配器

## 版本历史

- **v1.0.0**: 预留模块，基础结构定义
- **计划**: 未来版本将实现完整功能

## 维护者

CN_Language架构委员会

## 参考文档

- [系统分配器文档](../system/README.md)
- [调试分配器文档](../debug/README.md)
- [对象池分配器文档](../pool/README.md)
- [区域分配器文档](../region/README.md)
- [内存管理接口文档](../../CN_memory_interface.h)
