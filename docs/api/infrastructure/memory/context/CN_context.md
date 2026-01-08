# 内存上下文模块 API 文档

## 概述

内存上下文模块是CN_Language项目基础设施层的重要组成部分，提供层次化的内存管理机制。该模块允许将内存分配组织到逻辑上下文中，便于内存资源的统一管理和释放，特别适用于需要批量释放内存的场景。

## 核心概念

### 1. 内存上下文 (Memory Context)
内存上下文是一个逻辑容器，用于管理一组相关的内存分配。上下文可以形成层次结构，子上下文继承父上下文的内存管理策略。

### 2. 上下文生命周期
- **创建**: 创建新的内存上下文
- **切换**: 在当前线程中切换活动上下文
- **重置**: 释放上下文中的所有内存，但保留上下文本身
- **销毁**: 完全销毁上下文及其所有内存

### 3. 内存所有权
- 上下文拥有其分配的所有内存
- 上下文销毁时自动释放所有内存
- 支持内存所有权转移

## 模块架构

内存上下文模块采用模块化设计，分为多个子模块：

```
内存上下文模块
├── 核心接口 (core/)
│   ├── CN_context_interface.h - 核心接口定义
│   ├── CN_context_struct.h - 数据结构定义
│   └── CN_context_core.h - 核心功能声明
├── 管理子模块 (core/)
│   └── CN_context_management.c - 上下文创建、销毁、重置
├── 操作子模块 (core/)
│   └── CN_context_operations.c - 上下文切换、查询
├── 分配子模块 (core/)
│   └── CN_context_allocation.c - 内存分配、重新分配、释放
├── 统计子模块 (core/)
│   └── CN_context_statistics.c - 内存使用统计
└── 接口子模块 (core/)
    └── CN_context_interface.c - 全局状态管理和接口初始化
```

## 核心接口

### Stru_MemoryContextInterface_t

内存上下文核心接口，定义了完整的上下文管理操作集合：

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

## API 函数

### 全局管理函数

#### `F_get_memory_context_interface`

获取内存上下文接口实例。

```c
Stru_MemoryContextInterface_t* F_get_memory_context_interface(void);
```

**参数**: 无

**返回值**: 内存上下文接口指针

**说明**: 返回全局的内存上下文接口实例。如果接口未初始化，会自动初始化。

#### `F_initialize_memory_context_system`

初始化内存上下文系统。

```c
bool F_initialize_memory_context_system(void);
```

**参数**: 无

**返回值**: 初始化成功返回true，否则返回false

**说明**: 初始化内存上下文系统，创建默认的根上下文。如果系统已初始化，不执行任何操作。

#### `F_shutdown_memory_context_system`

关闭内存上下文系统。

```c
void F_shutdown_memory_context_system(void);
```

**参数**: 无

**返回值**: 无

**说明**: 关闭内存上下文系统，释放所有资源。调用后不能再使用任何上下文功能。

#### `F_get_root_context`

获取默认根上下文。

```c
Stru_MemoryContext_t* F_get_root_context(void);
```

**参数**: 无

**返回值**: 根上下文指针

**说明**: 获取系统默认的根上下文。如果系统未初始化，会自动初始化。

#### `F_set_root_context`

设置默认根上下文。

```c
void F_set_root_context(Stru_MemoryContext_t* root_context);
```

**参数**:
- `root_context`: 新的根上下文

**返回值**: 无

**说明**: 设置系统默认的根上下文。如果已有根上下文，会先销毁旧的。

#### `F_validate_context`

验证上下文是否有效。

```c
bool F_validate_context(Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 要验证的上下文

**返回值**: 有效返回true，否则返回false

#### `F_validate_context_name`

验证上下文名称是否有效。

```c
bool F_validate_context_name(const char* name);
```

**参数**:
- `name`: 上下文名称

**返回值**: 有效返回true，否则返回false

**说明**: 检查名称长度（1-256字符）和字符有效性（字母、数字、下划线、连字符、点号）。

#### `F_validate_memory_address`

验证内存地址是否属于指定上下文。

```c
bool F_validate_memory_address(Stru_MemoryContext_t* context, void* address);
```

**参数**:
- `context`: 上下文
- `address`: 内存地址

**返回值**: 属于该上下文返回true，否则返回false

### 上下文管理函数

#### `F_context_create`

创建新的内存上下文。

```c
Stru_MemoryContext_t* F_context_create(const char* name, Stru_MemoryContext_t* parent);
```

**参数**:
- `name`: 上下文名称（用于调试和识别）
- `parent`: 父上下文指针（NULL表示根上下文）

**返回值**: 新创建的上下文指针，失败返回NULL

**说明**: 创建新的内存上下文，可以指定父上下文形成层次结构。

#### `F_context_destroy`

销毁内存上下文及其所有内存。

```c
void F_context_destroy(Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 要销毁的上下文

**返回值**: 无

**说明**: 递归销毁上下文及其所有子上下文，释放所有分配的内存。

#### `F_context_reset`

重置内存上下文。

```c
void F_context_reset(Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 要重置的上下文

**返回值**: 无

**说明**: 释放上下文中的所有内存，但保留上下文本身。适用于需要重用上下文的场景。

### 上下文操作函数

#### `F_context_get_current`

获取当前线程的活动上下文。

```c
Stru_MemoryContext_t* F_context_get_current(void);
```

**参数**: 无

**返回值**: 当前活动上下文指针

**说明**: 返回线程本地的当前活动上下文。

#### `F_context_set_current`

设置当前线程的活动上下文。

```c
void F_context_set_current(Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 要设置为活动状态的上下文

**返回值**: 无

#### `F_context_get_parent`

获取上下文的父上下文。

```c
Stru_MemoryContext_t* F_context_get_parent(Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 要查询的上下文

**返回值**: 父上下文指针（NULL表示根上下文）

#### `F_context_get_first_child`

获取上下文的第一个子上下文。

```c
Stru_MemoryContext_t* F_context_get_first_child(Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 要查询的上下文

**返回值**: 第一个子上下文指针（NULL表示无子上下文）

#### `F_context_get_next_sibling`

获取上下文的下一个兄弟上下文。

```c
Stru_MemoryContext_t* F_context_get_next_sibling(Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 要查询的上下文

**返回值**: 下一个兄弟上下文指针（NULL表示无更多兄弟）

#### `F_context_get_name`

获取上下文名称。

```c
const char* F_context_get_name(Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 要查询的上下文

**返回值**: 上下文名称字符串

#### `F_context_get_id`

获取上下文唯一标识符。

```c
uint64_t F_context_get_id(Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 要查询的上下文

**返回值**: 上下文唯一ID

#### `F_context_is_valid`

检查上下文是否有效。

```c
bool F_context_is_valid(Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 要检查的上下文

**返回值**: 有效返回true，否则返回false

#### `F_context_get_creation_time`

获取上下文创建时间戳。

```c
uint64_t F_context_get_creation_time(Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 要查询的上下文

**返回值**: 创建时间戳（毫秒）

#### `F_context_get_depth`

获取上下文深度。

```c
size_t F_context_get_depth(Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 要查询的上下文

**返回值**: 上下文深度（根上下文为0）

### 内存分配函数

#### `F_context_allocate`

在指定上下文中分配内存。

```c
void* F_context_allocate(Stru_MemoryContext_t* context, size_t size, size_t alignment);
```

**参数**:
- `context`: 目标上下文（NULL表示使用当前上下文）
- `size`: 要分配的字节数
- `alignment`: 内存对齐要求（0表示使用默认对齐）

**返回值**: 分配的内存指针，失败返回NULL

#### `F_context_reallocate`

在指定上下文中重新分配内存。

```c
void* F_context_reallocate(Stru_MemoryContext_t* context, void* ptr, size_t new_size);
```

**参数**:
- `context`: 目标上下文（NULL表示使用当前上下文）
- `ptr`: 原内存指针
- `new_size`: 新的字节数

**返回值**: 重新分配的内存指针，失败返回NULL

#### `F_context_deallocate`

释放指定上下文中的内存。

```c
void F_context_deallocate(Stru_MemoryContext_t* context, void* ptr);
```

**参数**:
- `context`: 目标上下文（NULL表示使用当前上下文）
- `ptr`: 要释放的内存指针

**返回值**: 无

### 内存管理函数

#### `F_context_get_allocated_size`

获取内存块的实际分配大小。

```c
size_t F_context_get_allocated_size(Stru_MemoryContext_t* context, void* ptr);
```

**参数**:
- `context`: 内存块所属的上下文
- `ptr`: 内存块指针

**返回值**: 实际分配的字节数（0表示无效指针）

#### `F_context_get_total_allocated`

获取上下文的总分配内存。

```c
size_t F_context_get_total_allocated(Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 要查询的上下文

**返回值**: 总分配字节数

#### `F_context_get_peak_allocated`

获取上下文的峰值分配内存。

```c
size_t F_context_get_peak_allocated(Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 要查询的上下文

**返回值**: 峰值分配字节数

#### `F_context_get_current_usage`

获取上下文的当前使用内存。

```c
size_t F_context_get_current_usage(Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 要查询的上下文

**返回值**: 当前使用字节数

### 向后兼容性函数

#### `F_create_memory_context`

创建完整的内存管理上下文。

```c
Stru_MemoryContext_t* F_create_memory_context(bool use_debug_allocator);
```

**参数**:
- `use_debug_allocator`: 是否使用调试分配器

**返回值**: 内存上下文指针

**说明**: 创建包含分配器和调试器的内存管理上下文。如果use_debug_allocator为true，使用调试分配器；否则使用系统分配器。

#### `F_create_custom_memory_context`

创建自定义内存管理上下文。

```c
Stru_MemoryContext_t* F_create_custom_memory_context(
    Stru_MemoryAllocatorInterface_t* allocator,
    Stru_MemoryDebugInterface_t* debugger);
```

**参数**:
- `allocator`: 自定义分配器
- `debugger`: 自定义调试器

**返回值**: 内存上下文指针

**说明**: 使用指定的分配器和调试器创建内存管理上下文。如果allocator为NULL，使用系统分配器。如果debugger为NULL，不启用调试功能。

#### `F_destroy_memory_context`

销毁内存管理上下文。

```c
void F_destroy_memory_context(Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 要销毁的上下文

**返回值**: 无

**说明**: 销毁内存管理上下文，释放所有相关资源。如果context为NULL，不执行任何操作。

#### `F_get_context_allocator`

从上下文获取分配器。

```c
Stru_MemoryAllocatorInterface_t* F_get_context_allocator(
    Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 内存上下文

**返回值**: 分配器接口指针

**说明**: 从内存上下文中获取分配器接口。如果context为NULL，返回系统分配器。

#### `F_get_context_debugger`

从上下文获取调试器。

```c
Stru_MemoryDebugInterface_t* F_get_context_debugger(
    Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 内存上下文

**返回值**: 调试器接口指针

**说明**: 从内存上下文中获取调试器接口。如果context为NULL或未启用调试，返回NULL。

#### `F_set_context_private_data`

设置上下文私有数据。

```c
void F_set_context_private_data(Stru_MemoryContext_t* context,
                               void* private_data);
```

**参数**:
- `context`: 内存上下文
- `private_data`: 私有数据指针

**返回值**: 无

**说明**: 设置内存上下文的私有数据。私有数据用于存储上下文特定的信息。

#### `F_get_context_private_data`

获取上下文私有数据。

```c
void* F_get_context_private_data(Stru_MemoryContext_t* context);
```

**参数**:
- `context`: 内存上下文

**返回值**: 私有数据指针

## 使用示例

### 1. 基本使用

```c
#include "CN_memory_context.h"

// 获取内存上下文接口
Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();

// 创建根上下文
Stru_MemoryContext_t* root_ctx = ctx_if->create("RootContext", NULL);

// 创建子上下文
Stru_MemoryContext_t* child_ctx = ctx_if->create("ChildContext", root_ctx);

// 在子上下文中分配内存
void* memory = ctx_if->allocate(child_ctx, 1024, 8);

// 使用内存...

// 获取内存统计
size_t total_allocated = ctx_if->get_total_allocated(child_ctx);
size_t peak_allocated = ctx_if->get_peak_allocated(child_ctx);

// 销毁子上下文（自动释放所有内存）
ctx_if->destroy(child_ctx);

// 销毁根上下文
ctx_if->destroy(root_ctx);
```

### 2. 上下文切换

```c
#include "CN_memory_context.h"

// 获取内存上下文接口
Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();

// 创建多个上下文
Stru_MemoryContext_t* ctx1 = ctx_if->create("Context1", NULL);
Stru_MemoryContext_t* ctx2 = ctx_if->create("Context2", NULL);

// 切换到上下文1
ctx_if->set_current(ctx1);
void* mem1 = ctx_if->allocate(NULL, 512, 16); // 在当前上下文分配

// 切换到上下文2
ctx_if->set_current(ctx2);
void* mem2 = ctx_if->allocate(NULL, 256, 8); // 在当前上下文分配

// 获取当前上下文
Stru_MemoryContext_t* current = ctx_if->get_current();

// 清理
ctx_if->destroy(ctx1);
ctx_if->destroy(ctx2);
```

### 3. 内存统计

```c
#include "CN_memory_context.h"
#include <stdio.h>

// 获取内存上下文接口
Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();

// 创建上下文
Stru_MemoryContext_t* ctx = ctx_if->create("StatsContext", NULL);

// 分配一些内存
void* mem1 = ctx_if->allocate(ctx, 100, 8);
void* mem2 = ctx_if->allocate(ctx, 200, 16);
void* mem3 = ctx_if->allocate(ctx, 300, 32);

// 获取统计信息
size_t total = ctx_if->get_total_allocated(ctx);
size_t peak = ctx_if->get_peak_allocated(ctx);
size_t current = ctx_if->get_current_usage(ctx);

printf("内存统计:\n");
printf("  总分配: %zu bytes\n", total);
printf("  峰值分配: %zu bytes\n", peak);
printf("  当前使用: %zu bytes\n", current);

// 清理
ctx_if->destroy(ctx);
```

### 4. 使用向后兼容性函数

```c
#include "CN_memory_context.h"

// 创建带调试功能的内存上下文
Stru_MemoryContext_t* debug_ctx = F_create_memory_context(true);

// 获取上下文中的分配器
Stru_MemoryAllocatorInterface_t* allocator = F_get_context_allocator(debug_ctx);

// 使用分配器分配内存
void* memory = allocator->allocate(allocator, 1024, 8);

// 设置上下文私有数据
int user_data = 42;
F_set_context_private_data(debug_ctx, &user_data);

// 获取上下文私有数据
int* retrieved_data = (int*)F_get_context_private_data(debug_ctx);

// 清理
F_destroy_memory_context(debug_ctx);
```

### 5. 上下文层次结构

```c
#include "CN_memory_context.h"
#include <stdio.h>

// 获取内存上下文接口
Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();

// 创建层次化上下文结构
Stru_MemoryContext_t* root = ctx_if->create("Root", NULL);
Stru_MemoryContext_t* child1 = ctx_if->create("Child1", root);
Stru_MemoryContext_t* child2 = ctx_if->create("Child2", root);
Stru_MemoryContext_t* grandchild = ctx_if->create("Grandchild", child1);

// 遍历上下文层次结构
printf("上下文层次结构:\n");
Stru_MemoryContext_t* current = root;
while (current != NULL)
{
    // 打印当前上下文信息
    printf("  %*s%s (ID: %llu, 深度: %zu)\n", 
           (int)ctx_if->get_depth(current) * 2, "",
           ctx_if->get_name(current),
           ctx_if->get_id(current),
           ctx_if->get_depth(current));
    
    // 如果有子上下文，先处理子上下文
    if (ctx_if->get_first_child(current) != NULL)
    {
        current = ctx_if->get_first_child(current);
    }
    else
    {
        // 处理兄弟上下文
        while (current != NULL && ctx_if->get_next_sibling(current) == NULL)
        {
            current = ctx_if->get_parent(current);
        }
        if (current != NULL)
        {
            current = ctx_if->get_next_sibling(current);
        }
    }
}

// 清理
ctx_if->destroy(root);
```

## 错误处理

内存上下文函数通过返回值指示错误：
- 创建函数返回NULL表示创建失败
- 分配函数返回NULL表示分配失败
- 验证函数返回false表示无效
- 统计函数通过输出参数返回信息

建议的错误处理模式：

```c
#include "CN_memory_context.h"

// 获取接口
Stru_MemoryContextInterface_t* ctx_if = F_get_memory_context_interface();
if (ctx_if == NULL)
{
    // 处理接口获取失败
    return;
}

// 创建上下文
Stru_MemoryContext_t* ctx = ctx_if->create("MyContext", NULL);
if (ctx == NULL)
{
    // 处理上下文创建失败
    return;
}

// 分配内存
void* memory = ctx_if->allocate(ctx, 1024, 8);
if (memory == NULL)
{
    // 处理内存分配失败
    ctx_if->destroy(ctx);
    return;
}

// 使用内存...

// 验证内存有效性
if (!ctx_if->is_valid(ctx))
{
    // 处理无效上下文
    return;
}

// 清理
ctx_if->destroy(ctx);
```

## 性能考虑

### 1. 上下文切换开销
- 上下文切换有轻微性能开销
- 避免频繁的上下文切换
- 批量操作在同一上下文中进行

### 2. 内存统计开销
- 内存统计有轻微性能开销
- 生产环境可禁用统计功能
- 统计信息通过配置选项控制

### 3. 层次结构深度
- 过深的层次结构可能影响性能
- 建议层次深度不超过10级
- 扁平化结构可提高性能

## 线程安全

### 1. 上下文线程安全
- 内存上下文不是线程安全的
- 每个线程应使用独立的上下文
- 上下文切换是线程本地的

### 2. 接口线程安全
- 全局接口函数是线程安全的
- 接口实例在多线程环境下需要外部同步
- 建议每个线程获取独立的接口实例

### 3. 内存分配线程安全
- 内存分配操作需要外部同步
- 同一上下文在多线程环境下需要锁保护
- 建议使用线程本地上下文

## 设计原则

### 1. 单一职责原则
- 每个子模块只负责一个功能领域
- 函数功能单一明确
- 清晰的职责分离

### 2. 开闭原则
- 通过抽象接口支持新的上下文类型
- 现有代码对扩展开放，对修改封闭
- 可替换的实现

### 3. 里氏替换原则
- 所有上下文实现可互换使用
- 接口契约一致
- 行为可预测

### 4. 接口隔离原则
- 为不同客户端提供专用接口
- 避免"胖接口"
- 通过接口组合提供灵活的功能集

### 5. 依赖倒置原则
- 高层模块定义接口
- 低层模块实现接口
- 通过依赖注入管理依赖关系

## 扩展指南

### 1. 添加新的上下文类型
1. 实现`Stru_MemoryContextInterface_t`接口
2. 提供专用的创建函数
3. 添加单元测试
4. 更新文档

### 2. 添加新的内存分配策略
1. 扩展上下文结构体支持自定义分配器
2. 提供分配器配置接口
3. 添加性能测试
4. 更新API文档

### 3. 添加新的统计功能
1. 扩展统计接口
2. 提供统计配置选项
3. 添加性能基准
4. 更新使用示例

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-01-08 | 初始版本，基本上下文管理 |
| 2.0.0 | 2026-01-08 | 模块化重构，分为多个子模块 |
| 2.1.0 | 2026-01-08 | 完善API文档，添加完整示例 |

## 相关文件

### 源代码文件
- `src/infrastructure/memory/context/CN_memory_context.h` - 主接口文件
- `src/infrastructure/memory/context/core/CN_context_interface.h` - 核心接口定义
- `src/infrastructure/memory/context/core/CN_context_struct.h` - 数据结构定义
- `src/infrastructure/memory/context/core/CN_context_core.h` - 核心功能声明
- `src/infrastructure/memory/context/core/CN_context_management.c` - 上下文管理实现
- `src/infrastructure/memory/context/core/CN_context_operations.c` - 上下文操作实现
- `src/infrastructure/memory/context/core/CN_context_allocation.c` - 内存分配实现
- `src/infrastructure/memory/context/core/CN_context_statistics.c` - 统计功能实现
- `src/infrastructure/memory/context/core/CN_context_interface.c` - 接口初始化实现

### 文档文件
- `src/infrastructure/memory/context/README.md` - 模块主文档
- `docs/api/infrastructure/memory/context/CN_context.md` - API文档（本文档）

### 测试文件
- `tests/infrastructure/memory/context/test_context.h` - 测试头文件
- `tests/infrastructure/memory/context/test_context_utils.c` - 测试辅助函数
- `tests/infrastructure/memory/context/test_context_management.c` - 上下文管理测试
- `tests/infrastructure/memory/context/test_context_operations.c` - 上下文操作测试
- `tests/infrastructure/memory/context/test_context_allocation.c` - 内存分配测试
- `tests/infrastructure/memory/context/test_context_statistics.c` - 统计测试
- `tests/infrastructure/memory/context/test_context_runner.c` - 测试运行器

## 维护者

CN_Language架构委员会
