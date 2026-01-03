# CN_Language 物理内存分配器模块

## 概述

物理内存分配器是CN_Language项目基础设施层的关键组件，专门用于管理物理内存页框的分配和释放。该模块模拟操作系统内核中的物理内存管理功能，支持页框分配、对齐分配、特定地址分配等功能，适用于操作系统开发和嵌入式系统开发场景。

## 设计目标

1. **模块化设计**：遵循单一职责原则，专注于物理内存管理
2. **高性能**：使用位图算法实现快速页面分配和释放
3. **可配置性**：支持多种配置选项，适应不同应用场景
4. **调试支持**：提供完整的调试和跟踪功能，便于问题排查
5. **内存安全**：实现内存泄漏检测和完整性验证

## 架构设计

### 核心数据结构

1. **Stru_CN_PhysicalAllocator_t**：物理内存分配器句柄（不透明指针）
2. **Stru_CN_PhysicalConfig_t**：分配器配置结构
3. **Stru_CN_PhysicalStats_t**：分配器统计信息结构
4. **Stru_CN_PhysicalAllocationInfo_t**：分配跟踪信息结构

### 算法设计

- **位图管理**：使用位图跟踪每个页面的分配状态（1=已分配，0=空闲）
- **首次适应算法**：从上次搜索位置开始查找连续空闲页面
- **对齐分配**：支持页面对齐的内存分配
- **特定地址分配**：支持在指定物理地址分配页面

## 功能特性

### 1. 基本分配功能
- 创建和销毁物理内存分配器
- 分配和释放物理内存页面
- 支持标准页面（4KB）、大页面（2MB）和巨大页面（1GB）

### 2. 高级分配功能
- 对齐页面分配（支持任意对齐要求）
- 特定地址页面分配
- 保留页面设置（用于内核等特殊用途）

### 3. 查询和统计功能
- 获取分配器配置和统计信息
- 查询页面分配状态
- 计算最大连续空闲块
- 内存碎片化分析

### 4. 调试和跟踪功能
- 调试模式开关
- 分配跟踪（记录分配位置和目的）
- 内存泄漏检测
- 分配器完整性验证

## API接口

### 分配器管理接口
- `CN_physical_create()` - 创建物理内存分配器
- `CN_physical_destroy()` - 销毁物理内存分配器

### 页面分配接口
- `CN_physical_alloc_pages()` - 分配连续页面
- `CN_physical_alloc_pages_aligned()` - 分配对齐的连续页面
- `CN_physical_alloc_pages_at()` - 在特定地址分配页面
- `CN_physical_free_pages()` - 释放分配的页面

### 查询接口
- `CN_physical_get_config()` - 获取分配器配置
- `CN_physical_get_stats()` - 获取分配器统计信息
- `CN_physical_is_allocated()` - 检查地址是否已分配
- `CN_physical_total_pages()` - 获取总页面数量
- `CN_physical_get_free_pages()` - 获取空闲页面数量
- `CN_physical_used_pages()` - 获取已使用页面数量
- `CN_physical_largest_free_block()` - 获取最大连续空闲块

### 调试接口
- `CN_physical_enable_debug()` - 启用/禁用调试模式
- `CN_physical_validate()` - 验证分配器完整性
- `CN_physical_dump()` - 转储分配器状态
- `CN_physical_dump_leaks()` - 转储内存泄漏信息
- `CN_physical_set_debug_callback()` - 设置调试回调函数

### 辅助函数
- `CN_physical_size_to_pages()` - 字节大小转换为页面数量
- `CN_physical_pages_to_size()` - 页面数量转换为字节大小
- `CN_physical_align_to_page()` - 对齐地址到页面边界
- `CN_physical_is_page_aligned()` - 检查地址是否页面对齐

## 使用示例

### 示例1：创建和使用物理内存分配器

```c
#include "CN_physical_allocator.h"

int main() {
    // 配置物理内存分配器
    Stru_CN_PhysicalConfig_t config = CN_PHYSICAL_CONFIG_DEFAULT;
    config.memory_start = 0x100000;  // 1MB
    config.memory_end = 0x200000;    // 2MB
    config.page_size = CN_PAGE_SIZE_4KB;
    config.name = "测试分配器";
    
    // 创建分配器
    Stru_CN_PhysicalAllocator_t* allocator = CN_physical_create(&config);
    if (!allocator) {
        printf("创建分配器失败\n");
        return -1;
    }
    
    // 分配4个页面（16KB）
    uintptr_t addr = CN_physical_alloc_pages(allocator, 4, __FILE__, __LINE__, "测试分配");
    if (addr == 0) {
        printf("分配失败\n");
        CN_physical_destroy(allocator);
        return -1;
    }
    
    printf("分配成功：地址=0x%llx\n", (unsigned long long)addr);
    
    // 获取统计信息
    Stru_CN_PhysicalStats_t stats;
    if (CN_physical_get_stats(allocator, &stats)) {
        printf("总页面：%zu，空闲页面：%zu，使用页面：%zu\n",
               stats.total_pages, stats.free_pages, stats.used_pages);
    }
    
    // 释放页面
    CN_physical_free_pages(allocator, addr, 4, __FILE__, __LINE__);
    
    // 销毁分配器
    CN_physical_destroy(allocator);
    
    return 0;
}
```

### 示例2：对齐页面分配

```c
// 分配对齐到1MB边界的2个页面
uintptr_t aligned_addr = CN_physical_alloc_pages_aligned(
    allocator, 
    2,                      // 2个页面
    1024 * 1024,            // 1MB对齐
    __FILE__, __LINE__,
    "对齐分配测试"
);

if (aligned_addr != 0) {
    printf("对齐分配成功：地址=0x%llx\n", (unsigned long long)aligned_addr);
    // 使用对齐的内存...
    CN_physical_free_pages(allocator, aligned_addr, 2, __FILE__, __LINE__);
}
```

### 示例3：调试和跟踪

```c
// 启用调试模式
CN_physical_enable_debug(allocator, true);

// 设置调试回调
void debug_callback(const char* message, void* user_data) {
    printf("[物理内存分配器] %s\n", message);
}

CN_physical_set_debug_callback(allocator, debug_callback, NULL);

// 验证分配器完整性
if (!CN_physical_validate(allocator)) {
    printf("分配器验证失败\n");
}

// 转储分配器状态
CN_physical_dump(allocator);

// 检查内存泄漏
CN_physical_dump_leaks(allocator);
```

## 配置选项

### 默认配置
```c
#define CN_PHYSICAL_CONFIG_DEFAULT \
    { \
        .memory_start = 0,            /* 需要运行时设置 */ \
        .memory_end = 0,              /* 需要运行时设置 */ \
        .page_size = 4096,            /* 4KB页面 */ \
        .enable_statistics = true,    /* 启用统计 */ \
        .enable_debug = false,        /* 禁用调试 */ \
        .zero_on_alloc = false,       /* 不清零页面 */ \
        .track_allocations = true,    /* 跟踪分配信息 */ \
        .reserved_pages = 0,          /* 无保留页面 */ \
        .name = "物理内存分配器"      /* 默认名称 */ \
    }
```

### 配置参数说明
- `memory_start`：物理内存起始地址
- `memory_end`：物理内存结束地址
- `page_size`：页面大小（字节），建议使用`CN_PAGE_SIZE_4KB`、`CN_PAGE_SIZE_2MB`、`CN_PAGE_SIZE_1GB`
- `enable_statistics`：是否启用统计信息收集
- `enable_debug`：是否启用调试模式
- `zero_on_alloc`：分配时是否清零页面内容
- `track_allocations`：是否跟踪分配信息（用于调试）
- `reserved_pages`：保留的页面数量（用于内核等特殊用途）
- `name`：分配器名称（用于调试输出）

## 性能考虑

### 时间复杂度
- 页面分配：O(n)，其中n为总页面数（使用优化搜索算法）
- 页面释放：O(1)
- 查询操作：O(1) 或 O(n)（取决于具体操作）

### 空间复杂度
- 位图大小：总页面数/8 字节
- 分配跟踪：每个跟踪记录约40字节

### 优化策略
1. **搜索优化**：记录上次搜索位置，减少搜索时间
2. **位图压缩**：使用位图而不是数组跟踪页面状态
3. **缓存友好**：位图数据连续存储，提高缓存命中率

## 内存管理

### 内部内存分配
- 分配器结构：使用系统malloc分配
- 位图内存：使用系统calloc分配（初始化为0）
- 分配跟踪数组：使用系统malloc分配（如果启用跟踪）

### 内存安全
- 边界检查：所有操作都进行边界检查
- 状态验证：关键操作前验证分配器状态
- 泄漏检测：销毁时检查未释放的内存

## 错误处理

### 错误类型
1. **配置错误**：无效的配置参数
2. **分配失败**：内存不足或碎片化严重
3. **参数错误**：无效的地址或大小
4. **状态错误**：分配器未初始化或已损坏

### 错误处理策略
- 返回错误码或NULL表示失败
- 提供调试信息帮助问题定位
- 支持完整性验证

## 测试策略

### 单元测试
- 基本分配和释放测试
- 边界条件测试
- 错误处理测试

### 集成测试
- 与内存管理框架集成测试
- 与容器模块集成测试

### 性能测试
- 分配性能测试
- 内存碎片化测试
- 并发访问测试（如果支持）

## 依赖关系

### 编译时依赖
- C标准库：stdlib.h, string.h, stdio.h, stdarg.h
- 项目内部：无其他模块依赖（遵循基础设施层设计原则）

### 运行时依赖
- 无特殊运行时依赖

## 平台兼容性

### 支持平台
- Windows（通过MinGW/GCC）
- Linux（通过GCC）
- 其他支持C99标准的平台

### 平台特定考虑
- 地址大小：支持32位和64位地址
- 字节序：独立于字节序
- 对齐要求：遵循平台对齐要求

## 版本历史

### v1.0.0 (2026-01-03)
- 初始版本发布
- 实现基本物理内存分配功能
- 支持对齐分配和特定地址分配
- 提供完整的调试和跟踪功能

## 维护指南

### 代码规范
- 遵循CN_Language项目编码标准
- 每个函数不超过50行
- 每个文件不超过500行
- 使用中文注释说明复杂逻辑

### 扩展建议
1. **性能优化**：实现更高效的搜索算法
2. **功能扩展**：支持NUMA架构
3. **安全增强**：添加内存保护功能
4. **监控集成**：与系统监控工具集成

## 相关文档

- [API文档](../docs/api/infrastructure/memory/CN_physical_allocator_API.md)
- [架构设计原则](../../../../docs/architecture/架构设计原则.md)
- [内存管理框架文档](../CN_memory_README.md)

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language开发团队

## 最后更新

2026年1月3日
