# 内存上下文接口模块

## 概述

内存上下文接口模块定义了内存上下文管理的抽象接口，遵循SOLID设计原则和接口隔离原则。该模块提供了统一的接口规范，使得不同的内存上下文实现可以互换使用。

## 核心接口

### Stru_MemoryContextInterface_t

内存上下文的核心接口，定义了完整的操作集合：

```c
typedef struct Stru_MemoryContextInterface_t
{
    // 上下文管理
    Stru_MemoryContext_t* (*create)(const char* name, Stru_MemoryContext_t* parent);
    void (*destroy)(Stru_MemoryContext_t* context);
    void (*reset)(Stru_MemoryContext_t* context);
    
    // 上下文操作
    Stru_MemoryContext_t* (*get_current)(void);
    void (*set_current)(Stru_MemoryContext_t* context);
    Stru_MemoryContext_t* (*get_parent)(Stru_MemoryContext_t* context);
    Stru_MemoryContext_t* (*get_first_child)(Stru_MemoryContext_t* context);
    Stru_MemoryContext_t* (*get_next_sibling)(Stru_MemoryContext_t* context);
    
    // 内存分配
    void* (*allocate)(Stru_MemoryContext_t* context, size_t size, size_t alignment);
    void* (*reallocate)(Stru_MemoryContext_t* context, void* ptr, size_t new_size);
    void (*deallocate)(Stru_MemoryContext_t* context, void* ptr);
    
    // 内存管理
    size_t (*get_allocated_size)(Stru_MemoryContext_t* context, void* ptr);
    size_t (*get_total_allocated)(Stru_MemoryContext_t* context);
    size_t (*get_peak_allocated)(Stru_MemoryContext_t* context);
    size_t (*get_current_usage)(Stru_MemoryContext_t* context);
    
    // 上下文信息
    const char* (*get_name)(Stru_MemoryContext_t* context);
    uint64_t (*get_id)(Stru_MemoryContext_t* context);
    bool (*is_valid)(Stru_MemoryContext_t* context);
    uint64_t (*get_creation_time)(Stru_MemoryContext_t* context);
    size_t (*get_depth)(Stru_MemoryContext_t* context);
    
    // 资源管理
    void (*cleanup)(void);
    void* private_data;
} Stru_MemoryContextInterface_t;
```

## 文件说明

### CN_context_interface.h
- **功能**: 定义内存上下文接口的数据结构和函数声明
- **设计原则**: 接口隔离原则，为不同客户端提供专用接口
- **依赖**: 无外部依赖，只包含标准库头文件

### CN_context_interface.c
- **功能**: 实现接口的初始化和全局函数
- **包含**: 全局状态管理、接口实例创建、系统初始化/关闭
- **设计原则**: 单一职责原则，只负责接口管理

## 设计原则

### 1. 接口隔离原则
- 为内存管理、上下文操作、统计查询等不同功能提供专用接口
- 避免"胖接口"，客户端只依赖它们需要的接口

### 2. 依赖倒置原则
- 高层模块定义抽象接口
- 低层模块实现具体功能
- 通过接口指针进行通信，减少编译时依赖

### 3. 开闭原则
- 接口设计支持扩展新的上下文类型
- 现有代码对扩展开放，对修改封闭

## 使用示例

### 获取接口实例
```c
#include "CN_context_interface.h"

// 获取内存上下文接口
Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
if (ctx_if == NULL) {
    // 处理错误
}
```

### 系统初始化和关闭
```c
// 初始化内存上下文系统
if (!F_initialize_memory_context_system()) {
    // 处理初始化失败
}

// 使用系统...

// 关闭内存上下文系统
F_shutdown_memory_context_system();
```

## 依赖关系

- **内部依赖**: 核心模块 (CN_context_core.h, CN_context_struct.h)
- **外部依赖**: C标准库 (stdlib.h, string.h, stdint.h)
- **架构层**: 基础设施层 - 内存管理模块

## 注意事项

1. **线程安全**: 接口本身不保证线程安全，具体实现在核心模块中处理
2. **初始化顺序**: 必须先调用`F_initialize_memory_context_system()`或`F_get_memory_context_interface()`初始化系统
3. **资源管理**: 使用`F_shutdown_memory_context_system()`清理所有资源
4. **错误处理**: 所有函数都提供明确的错误返回值，调用者应检查

## 扩展性

### 添加新接口
1. 在`CN_context_interface.h`中定义新的接口函数
2. 在`CN_context_interface.c`中实现接口初始化
3. 在核心模块中实现具体功能
4. 更新测试确保兼容性

### 替换实现
1. 实现新的`Stru_MemoryContextInterface_t`接口
2. 通过依赖注入替换默认实现
3. 保持接口契约不变

## 版本兼容性

- **主版本变更**: 不兼容的接口修改
- **次版本变更**: 向下兼容的功能新增
- **修订版本**: 向下兼容的问题修正

## 维护者

CN_Language架构委员会
