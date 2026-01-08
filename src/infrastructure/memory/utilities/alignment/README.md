# 内存对齐工具模块

## 概述

内存对齐工具模块是CN_Language项目基础设施层内存管理模块的一部分，专注于提供内存对齐相关的工具函数。本模块实现了`Stru_MemoryAlignmentInterface_t`接口，提供内存对齐计算、对齐检查和指针对齐等功能，帮助开发者处理内存对齐问题，提高内存访问性能。

## 设计原则

1. **性能优化**：对齐的内存访问通常更快，本模块帮助实现最佳对齐
2. **平台兼容性**：处理不同平台的对齐要求和特性
3. **易用性**：提供简单直观的API处理复杂的对齐问题
4. **安全性**：确保对齐操作不会导致内存错误

## 接口定义

### Stru_MemoryAlignmentInterface_t

内存对齐接口定义：

```c
typedef struct Stru_MemoryAlignmentInterface_t {
    size_t (*align_up)(size_t size, size_t alignment);
    size_t (*align_down)(size_t size, size_t alignment);
    bool (*is_aligned)(const void* ptr, size_t alignment);
    size_t (*alignment_of)(const void* ptr);
    void* (*align_pointer)(void* ptr, size_t alignment);
    size_t (*calculate_padding)(size_t size, size_t alignment);
} Stru_MemoryAlignmentInterface_t;
```

## API 函数

### `align_up`

向上对齐到指定的对齐边界。

```c
size_t align_up(size_t size, size_t alignment);
```

**参数**：
- `size`：要对齐的大小
- `alignment`：对齐边界（必须是2的幂）

**返回值**：向上对齐后的大小

**说明**：
- 如果`size`已经是对齐的，返回原值
- 对齐边界必须是2的幂，否则行为未定义
- 用于计算分配内存时所需的大小

### `align_down`

向下对齐到指定的对齐边界。

```c
size_t align_down(size_t size, size_t alignment);
```

**参数**：
- `size`：要对齐的大小
- `alignment`：对齐边界（必须是2的幂）

**返回值**：向下对齐后的大小

**说明**：
- 如果`size`已经是对齐的，返回原值
- 对齐边界必须是2的幂，否则行为未定义
- 用于计算内存块内的可用空间

### `is_aligned`

检查指针是否按指定对齐方式对齐。

```c
bool is_aligned(const void* ptr, size_t alignment);
```

**参数**：
- `ptr`：要检查的指针
- `alignment`：对齐边界（必须是2的幂）

**返回值**：
- `true`：指针已对齐
- `false`：指针未对齐或参数无效

**说明**：
- 检查指针地址是否是对齐边界的倍数
- 用于验证内存对齐要求

### `alignment_of`

获取指针的自然对齐方式。

```c
size_t alignment_of(const void* ptr);
```

**参数**：
- `ptr`：要检查的指针

**返回值**：指针的自然对齐方式（2的幂）

**说明**：
- 返回指针地址的最大2的幂因子
- 用于确定指针的最佳对齐方式
- 如果指针为NULL，返回0

### `align_pointer`

将指针对齐到指定的边界。

```c
void* align_pointer(void* ptr, size_t alignment);
```

**参数**：
- `ptr`：要对齐的指针
- `alignment`：对齐边界（必须是2的幂）

**返回值**：对齐后的指针

**说明**：
- 返回大于等于`ptr`的第一个对齐地址
- 用于在内存块内创建对齐的子分配
- 需要确保有足够的空间进行对齐

### `calculate_padding`

计算达到指定对齐所需的填充字节数。

```c
size_t calculate_padding(size_t size, size_t alignment);
```

**参数**：
- `size`：当前大小
- `alignment`：目标对齐边界（必须是2的幂）

**返回值**：达到对齐所需的填充字节数

**说明**：
- 如果`size`已经对齐，返回0
- 用于计算内存分配中的填充大小
- 帮助优化内存布局

## 使用示例

### 示例1：计算对齐的内存大小

```c
#include "CN_memory_utilities.h"

// 获取内存对齐接口
Stru_MemoryAlignmentInterface_t* align = F_get_memory_alignment();

// 计算需要分配的内存大小（对齐到16字节）
size_t data_size = 100;
size_t aligned_size = align->align_up(data_size, 16);
printf("原始大小: %zu, 对齐后大小: %zu\n", data_size, aligned_size);

// 分配对齐的内存
void* memory = malloc(aligned_size);
if (memory == NULL) {
    // 处理分配失败
}

// 检查内存是否对齐
bool is_aligned = align->is_aligned(memory, 16);
if (is_aligned) {
    printf("内存已正确对齐到16字节\n");
}

// 使用内存...

// 释放内存
free(memory);
```

### 示例2：在内存块内创建对齐的子分配

```c
#include "CN_memory_utilities.h"

// 获取内存对齐接口
Stru_MemoryAlignmentInterface_t* align = F_get_memory_alignment();

// 分配一个大内存块
size_t block_size = 4096;
void* block = malloc(block_size);
if (block == NULL) {
    // 处理分配失败
}

// 在块内创建对齐到64字节的子分配
void* aligned_ptr = align->align_pointer(block, 64);

// 计算填充大小
size_t padding = (size_t)((uintptr_t)aligned_ptr - (uintptr_t)block);
printf("原始指针: %p, 对齐指针: %p, 填充: %zu字节\n", 
       block, aligned_ptr, padding);

// 计算可用空间
size_t available_size = block_size - padding;

// 使用对齐的内存...

// 释放整个内存块
free(block);
```

### 示例3：处理结构体对齐

```c
#include "CN_memory_utilities.h"
#include <stddef.h>

// 获取内存对齐接口
Stru_MemoryAlignmentInterface_t* align = F_get_memory_alignment();

// 定义结构体
typedef struct {
    char a;
    double b;
    int c;
} MyStruct;

// 计算结构体的自然对齐
MyStruct instance;
size_t struct_alignment = align->alignment_of(&instance);
printf("结构体自然对齐: %zu字节\n", struct_alignment);

// 计算结构体大小和对齐
size_t struct_size = sizeof(MyStruct);
size_t aligned_struct_size = align->align_up(struct_size, struct_alignment);

// 分配对齐的结构体数组
size_t array_count = 10;
size_t total_size = aligned_struct_size * array_count;
void* array = malloc(total_size);
if (array == NULL) {
    // 处理分配失败
}

// 检查数组是否对齐
bool array_aligned = align->is_aligned(array, struct_alignment);
if (array_aligned) {
    printf("结构体数组已正确对齐\n");
}

// 使用结构体数组...

// 释放内存
free(array);
```

## 实现细节

### 对齐计算实现

对齐计算使用位操作实现，效率高：

```c
size_t F_align_up(size_t size, size_t alignment) {
    // 确保alignment是2的幂
    if ((alignment & (alignment - 1)) != 0) {
        // 处理无效对齐参数
        return size;
    }
    
    return (size + alignment - 1) & ~(alignment - 1);
}

size_t F_align_down(size_t size, size_t alignment) {
    // 确保alignment是2的幂
    if ((alignment & (alignment - 1)) != 0) {
        // 处理无效对齐参数
        return size;
    }
    
    return size & ~(alignment - 1);
}
```

### 对齐检查实现

对齐检查使用模运算的位操作优化：

```c
bool F_is_aligned(const void* ptr, size_t alignment) {
    if (ptr == NULL || alignment == 0) {
        return false;
    }
    
    // 确保alignment是2的幂
    if ((alignment & (alignment - 1)) != 0) {
        return false;
    }
    
    return ((uintptr_t)ptr & (alignment - 1)) == 0;
}
```

### 指针对齐实现

指针对齐计算下一个对齐地址：

```c
void* F_align_pointer(void* ptr, size_t alignment) {
    if (ptr == NULL || alignment == 0) {
        return ptr;
    }
    
    // 确保alignment是2的幂
    if ((alignment & (alignment - 1)) != 0) {
        return ptr;
    }
    
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    return (void*)aligned_addr;
}
```

## 性能考虑

### 计算性能

- **位操作优化**：所有对齐计算使用位操作，避免除法和模运算
- **编译时常量**：对齐边界通常是编译时常量，编译器可以优化
- **内联函数**：小函数适合内联，减少函数调用开销

### 内存性能

- **缓存友好**：对齐的内存访问提高缓存性能
- **SIMD优化**：对齐的内存支持SIMD指令
- **减少碎片**：合理的对齐减少内存碎片

### 平台特定优化

- **硬件支持**：利用硬件对齐检查指令
- **编译器提示**：使用编译器属性提示对齐
- **运行时检测**：检测平台的最佳对齐方式

## 线程安全

所有内存对齐函数都是线程安全的：

1. **无状态操作**：函数不维护内部状态
2. **纯函数**：输出只依赖于输入，没有副作用
3. **可重入**：函数可被多个线程同时调用

## 错误处理

### 参数验证

所有函数都对输入参数进行验证：

1. **对齐边界检查**：确保对齐边界是2的幂
2. **指针有效性**：检查指针是否为NULL
3. **大小检查**：检查大小参数是否合理

### 错误返回值

- `align_up`/`align_down`：如果对齐边界无效，返回原值
- `is_aligned`：如果参数无效，返回false
- `alignment_of`：如果指针为NULL，返回0
- `align_pointer`：如果参数无效，返回原指针
- `calculate_padding`：如果对齐边界无效，返回0

## 测试覆盖

### 单元测试

1. **对齐计算测试**：
   - 测试各种大小的对齐计算
   - 测试边界情况（已经对齐的大小）
   - 测试无效对齐参数

2. **对齐检查测试**：
   - 测试对齐和未对齐的指针
   - 测试不同对齐边界
   - 测试无效参数

3. **指针对齐测试**：
   - 测试指针对齐计算
   - 测试填充计算
   - 测试边界情况

### 集成测试

1. **与内存分配器集成**：
   - 测试对齐的内存分配
   - 测试内存块内的对齐子分配
   - 测试对齐对性能的影响

2. **与数据结构集成**：
   - 测试结构体对齐
   - 测试数组对齐
   - 测试缓存行对齐

3. **性能测试**：
   - 测试对齐操作的速度
   - 测试对齐内存访问的性能提升
   - 测试多线程环境下的性能

## 相关文件

### 源代码文件
- `src/infrastructure/memory/utilities/alignment/CN_memory_alignment.h` - 头文件
- `src/infrastructure/memory/utilities/alignment/CN_memory_alignment.c` - 实现文件

### 测试文件
- `tests/infrastructure/memory/utilities/alignment/test_memory_alignment.c` - 单元测试

### 文档文件
- `docs/api/infrastructure/memory/CN_memory.md` - API文档

## 维护者

CN_Language架构委员会

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-01-08 | 初始版本，包含基本对齐函数 |
| 1.1.0 | 2026-01-08 | 添加性能优化和平台特定实现 |
