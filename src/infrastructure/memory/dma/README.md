# CN_Language DMA内存分配器模块

## 概述

DMA内存分配器是CN_Language项目基础设施层的关键组件，专门用于管理DMA（直接内存访问）内存区域的分配和释放。该模块提供DMA特定的功能，如缓存一致性管理、对齐分配、分散-聚集支持等，适用于设备驱动开发和嵌入式系统开发场景。

## 设计目标

1. **DMA特定功能**：支持DMA内存的特殊需求，如缓存一致性、对齐要求
2. **高性能**：使用位图算法实现快速页面分配和释放
3. **可配置性**：支持多种DMA内存属性和配置选项
4. **调试支持**：提供完整的调试和跟踪功能，便于问题排查
5. **内存安全**：实现内存泄漏检测和完整性验证

## 架构设计

### 核心数据结构

1. **Stru_CN_DmaAllocator_t**：DMA内存分配器句柄（不透明指针）
2. **Stru_CN_DmaConfig_t**：DMA分配器配置结构
3. **Stru_CN_DmaStats_t**：DMA分配器统计信息结构
4. **Stru_CN_DmaBuffer_t**：DMA缓冲区描述符
5. **Stru_CN_DmaScatterGatherList_t**：分散-聚集列表结构

### DMA特定功能

- **内存属性管理**：支持不可缓存、写合并、写穿透、写回、设备内存等属性
- **缓存一致性**：自动管理缓存刷新和失效
- **对齐分配**：支持缓存行对齐和页面对齐
- **分散-聚集支持**：支持不连续内存区域的DMA传输

## 功能特性

### 1. 基本分配功能
- 创建和销毁DMA内存分配器
- 分配和释放DMA内存缓冲区
- 支持标准页面（4KB）和自定义页面大小

### 2. DMA特定功能
- 缓存行对齐分配（默认64字节对齐）
- 物理连续内存分配（用于需要连续物理内存的DMA设备）
- 分散-聚集缓冲区分配（用于不连续内存区域的DMA传输）
- 内存属性管理（不可缓存、设备内存等）

### 3. 同步功能
- CPU到设备同步（确保CPU写入的数据对设备可见）
- 设备到CPU同步（确保设备写入的数据对CPU可见）
- 分散-聚集缓冲区同步

### 4. 查询和统计功能
- 获取分配器配置和统计信息
- 查询内存分配状态
- 计算最大连续空闲块
- 内存碎片化分析
- DMA操作统计（缓存刷新、失效、同步等）

### 5. 调试和跟踪功能
- 调试模式开关
- 分配跟踪（记录分配位置和目的）
- 内存泄漏检测
- 分配器完整性验证

## API接口

### 分配器管理接口
- `CN_dma_create()` - 创建DMA内存分配器
- `CN_dma_destroy()` - 销毁DMA内存分配器

### DMA缓冲区分配接口
- `CN_dma_alloc_buffer()` - 分配DMA缓冲区（缓存行对齐）
- `CN_dma_alloc_contiguous()` - 分配物理连续的DMA缓冲区
- `CN_dma_alloc_scatter_gather()` - 分配分散-聚集DMA缓冲区
- `CN_dma_free_buffer()` - 释放DMA缓冲区
- `CN_dma_free_scatter_gather()` - 释放分散-聚集DMA缓冲区

### DMA缓冲区同步接口
- `CN_dma_sync_to_device()` - 同步DMA缓冲区（CPU到设备）
- `CN_dma_sync_from_device()` - 同步DMA缓冲区（设备到CPU）
- `CN_dma_sync_sg_to_device()` - 同步分散-聚集缓冲区（CPU到设备）
- `CN_dma_sync_sg_from_device()` - 同步分散-聚集缓冲区（设备到CPU）

### 查询接口
- `CN_dma_get_config()` - 获取分配器配置
- `CN_dma_get_stats()` - 获取分配器统计信息
- `CN_dma_total_pages()` - 获取总页面数量
- `CN_dma_get_free_pages()` - 获取空闲页面数量
- `CN_dma_used_pages()` - 获取已使用页面数量
- `CN_dma_largest_free_block()` - 获取最大连续空闲块

### 调试接口
- `CN_dma_enable_debug()` - 启用/禁用调试模式
- `CN_dma_validate()` - 验证分配器完整性
- `CN_dma_dump()` - 转储分配器状态
- `CN_dma_dump_leaks()` - 转储内存泄漏信息
- `CN_dma_set_debug_callback()` - 设置调试回调函数

### 辅助函数
- `CN_dma_size_to_pages()` - 字节大小转换为页面数量
- `CN_dma_pages_to_size()` - 页面数量转换为字节大小
- `CN_dma_align_to_page()` - 对齐地址到页面边界
- `CN_dma_is_page_aligned()` - 检查地址是否页面对齐
- `CN_dma_align_to_cache_line()` - 对齐地址到缓存行边界
- `CN_dma_is_cache_line_aligned()` - 检查地址是否缓存行对齐

## 使用示例

### 示例1：创建和使用DMA内存分配器

```c
#include "CN_dma_allocator.h"

int main() {
    // 配置DMA内存分配器
    Stru_CN_DmaConfig_t config = CN_DMA_CONFIG_DEFAULT;
    config.dma_region_start = 0x100000;  // 1MB
    config.dma_region_end = 0x200000;    // 2MB
    config.page_size = 4096;
    config.cache_line_size = 64;
    config.name = "测试DMA分配器";
    
    // 创建分配器
    Stru_CN_DmaAllocator_t* allocator = CN_dma_create(&config);
    if (!allocator) {
        printf("创建DMA分配器失败\n");
        return -1;
    }
    
    // 分配16KB DMA缓冲区（缓存行对齐）
    Stru_CN_DmaBuffer_t* buffer = CN_dma_alloc_buffer(
        allocator,
        16 * 1024,                      // 16KB
        64,                             // 64字节对齐（缓存行）
        Eum_DMA_ATTRIBUTE_UNCACHED,     // 不可缓存
        __FILE__, __LINE__,
        "测试DMA缓冲区"
    );
    
    if (!buffer) {
        printf("分配DMA缓冲区失败\n");
        CN_dma_destroy(allocator);
        return -1;
    }
    
    printf("分配成功：物理地址=0x%llx，虚拟地址=%p，大小=%zu\n",
           (unsigned long long)buffer->physical_address,
           buffer->virtual_address,
           buffer->size);
    
    // 使用DMA缓冲区...
    // 1. CPU写入数据
    memset(buffer->virtual_address, 0xAA, buffer->size);
    
    // 2. 同步到设备（确保数据对设备可见）
    CN_dma_sync_to_device(buffer, 0, 0); // 0,0表示整个缓冲区
    
    // 3. 设备进行DMA传输...
    
    // 4. 从设备同步（确保设备写入的数据对CPU可见）
    CN_dma_sync_from_device(buffer, 0, 0);
    
    // 5. CPU读取数据
    // uint8_t* data = (uint8_t*)buffer->virtual_address;
    
    // 释放DMA缓冲区
    CN_dma_free_buffer(allocator, buffer, __FILE__, __LINE__);
    
    // 销毁分配器
    CN_dma_destroy(allocator);
    
    return 0;
}
```

### 示例2：物理连续DMA缓冲区分配

```c
// 分配物理连续的DMA缓冲区（用于需要连续物理内存的设备）
Stru_CN_DmaBuffer_t* contiguous_buffer = CN_dma_alloc_contiguous(
    allocator,
    64 * 1024,                      // 64KB
    4096,                           // 4KB页面对齐
    Eum_DMA_ATTRIBUTE_DEVICE,       // 设备内存属性
    __FILE__, __LINE__,
    "物理连续DMA缓冲区"
);

if (contiguous_buffer) {
    printf("物理连续缓冲区分配成功：地址=0x%llx，连续=%s\n",
           (unsigned long long)contiguous_buffer->physical_address,
           contiguous_buffer->is_contiguous ? "是" : "否");
    
    // 使用物理连续缓冲区...
    CN_dma_free_buffer(allocator, contiguous_buffer, __FILE__, __LINE__);
}
```

### 示例3：调试和跟踪

```c
// 启用调试模式
CN_dma_enable_debug(allocator, true);

// 设置调试回调
void dma_debug_callback(const char* message, void* user_data) {
    printf("[DMA分配器] %s\n", message);
}

CN_dma_set_debug_callback(allocator, dma_debug_callback, NULL);

// 验证分配器完整性
if (!CN_dma_validate(allocator)) {
    printf("DMA分配器验证失败\n");
}

// 转储分配器状态
CN_dma_dump(allocator);

// 检查内存泄漏
CN_dma_dump_leaks(allocator);
```

## 配置选项

### 默认配置
```c
#define CN_DMA_CONFIG_DEFAULT \
    { \
        .dma_region_start = 0,           /* 需要运行时设置 */ \
        .dma_region_end = 0,             /* 需要运行时设置 */ \
        .page_size = 4096,               /* 4KB页面 */ \
        .cache_line_size = 64,           /* 64字节缓存行 */ \
        .enable_statistics = true,       /* 启用统计 */ \
        .enable_debug = false,           /* 禁用调试 */ \
        .zero_on_alloc = true,           /* 分配时清零内存（DMA安全） */ \
        .track_allocations = true,       /* 跟踪分配信息 */ \
        .default_attribute = Eum_DMA_ATTRIBUTE_UNCACHED, /* 默认不可缓存 */ \
        .name = "DMA内存分配器",         /* 默认名称 */ \
        .support_scatter_gather = false, /* 默认不支持分散-聚集 */ \
        .require_physical_contiguous = true, /* 默认要求物理连续 */ \
        .max_scatter_elements = 16       /* 默认最大分散元素数 */ \
    }
```

### 配置参数说明
- `dma_region_start`：DMA区域起始地址
- `dma_region_end`：DMA区域结束地址
- `page_size`：页面大小（字节），建议使用4096（4KB）
- `cache_line_size`：缓存行大小（字节），通常为64
- `enable_statistics`：是否启用统计信息收集
- `enable_debug`：是否启用调试模式
- `zero_on_alloc`：分配时是否清零内存内容（DMA安全）
- `track_allocations`：是否跟踪分配信息（用于调试）
- `default_attribute`：默认内存属性
- `name`：分配器名称（用于调试输出）
- `support_scatter_gather`：是否支持分散-聚集DMA
- `require_physical_contiguous`：是否要求物理连续内存
- `max_scatter_elements`：最大分散元素数量（如果支持分散-聚集）

## 内存属性说明

### Eum_CN_DmaMemoryAttribute_t
- `Eum_DMA_ATTRIBUTE_NONE`：无特殊属性，使用默认设置
- `Eum_DMA_ATTRIBUTE_UNCACHED`：不可缓存，用于设备直接访问
- `Eum_DMA_ATTRIBUTE_WRITE_COMBINE`：写合并，提高写入性能
- `Eum_DMA_ATTRIBUTE_WRITE_THROUGH`：写穿透，写入同时更新缓存和内存
- `Eum_DMA_ATTRIBUTE_WRITE_BACK`：写回，延迟写入内存
- `Eum_DMA_ATTRIBUTE_DEVICE`：设备内存，强排序，不可缓存
- `Eum_DMA_ATTRIBUTE_NORMAL`：普通内存，可缓存

## 性能考虑

### 时间复杂度
- 页面分配：O(n)，其中n为总页面数（使用优化搜索算法）
- 页面释放：O(1)
- 同步操作：O(1) 或 O(n)（取决于缓冲区大小）

### 空间复杂度
- 位图大小：总页面数/8 字节
- 分配跟踪：每个跟踪记录约64字节
- DMA缓冲区描述符：每个缓冲区约48字节

### 优化策略
1. **搜索优化**：记录上次搜索位置，减少搜索时间
2. **位图压缩**：使用位图而不是数组跟踪页面状态
3. **缓存友好**：位图数据连续存储，提高缓存命中率
4. **对齐优化**：预计算对齐页面，减少运行时计算

## 内存管理

### 内部内存分配
- 分配器结构：使用系统malloc分配
- 位图内存：使用系统calloc分配（初始化为0）
- 分配跟踪数组：使用系统malloc分配（如果启用跟踪）
- DMA缓冲区描述符：使用系统malloc分配

### 内存安全
- 边界检查：所有操作都进行边界检查
- 状态验证：关键操作前验证分配器状态
- 泄漏检测：销毁时检查未释放的内存
- 缓存一致性：自动管理缓存刷新和失效

## 错误处理

### 错误类型
1. **配置错误**：无效的配置参数
2. **分配失败**：内存不足或碎片化严重
3. **参数错误**：无效的地址、大小或对齐要求
4. **状态错误**：分配器未初始化或已损坏
5. **同步错误**：缓存操作失败

### 错误处理策略
- 返回错误码或NULL表示失败
- 提供调试信息帮助问题定位
- 支持完整性验证
- 记录错误统计信息

## 测试策略

### 单元测试
- 基本分配和释放测试
- 对齐分配测试
- 缓存一致性测试
- 错误处理测试

### 集成测试
- 与设备驱动集成测试
- 与操作系统DMA API集成测试
- 性能测试

### 性能测试
- 分配性能测试
- 同步操作性能测试
- 内存碎片化测试
- 并发访问测试（如果支持）

## 依赖关系

### 编译时依赖
- C标准库：stdlib.h, string.h, stdio.h, stdarg.h
- 项目内部：无其他模块依赖（遵循基础设施层设计原则）

### 运行时依赖
- 无特殊运行时依赖
- 可能需要平台特定的缓存操作指令

## 平台兼容性

### 支持平台
- Windows（通过MinGW/GCC）
- Linux（通过GCC）
- 其他支持C99标准的平台

### 平台特定考虑
- 缓存操作：需要平台特定的缓存刷新指令
- 内存属性：需要平台特定的内存属性设置
- 地址映射：需要平台特定的物理-虚拟地址映射

## 版本历史

### v1.0.0 (2026-01-03)
- 初始版本发布
- 实现基本DMA内存分配功能
- 支持缓存一致性管理
- 支持对齐分配和物理连续分配
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
5. **平台适配**：添加更多平台特定的优化

## 相关文档

- [API文档](../../../docs/api/infrastructure/memory/CN_dma_allocator_API.md)
- [架构设计原则](../../../../docs/architecture/架构设计原则.md)
- [物理内存分配器文档](../physical/README.md)
- [内存管理框架文档](../CN_memory_README.md)

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language开发团队

## 最后更新

2026年1月3日
