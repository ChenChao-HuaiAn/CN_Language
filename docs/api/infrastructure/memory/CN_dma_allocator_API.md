# CN_dma_allocator API 文档

## 概述

`CN_dma_allocator` 模块提供DMA（直接内存访问）内存分配器接口，专门管理用于DMA传输的内存区域。本模块是CN_Language基础设施层的关键组件，遵循SOLID设计原则和项目架构规范，支持DMA特定功能如缓存一致性管理、对齐分配、分散-聚集支持等。

## 文件结构

```
src/infrastructure/memory/dma/
├── CN_dma_allocator.h      # 头文件 - 公共接口定义
├── CN_dma_allocator.c      # 主文件 - 模块化实现入口
├── CN_dma_core.c           # 核心分配器功能
├── CN_dma_bitmap.c         # 位图管理功能
├── CN_dma_tracking.c       # 分配跟踪功能
├── CN_dma_cache.c          # 缓存一致性管理
├── CN_dma_scatter_gather.c # 分散-聚集DMA支持
├── CN_dma_debug.c          # 调试和统计功能
├── CN_dma_internal.h       # 内部接口定义
└── README.md               # 模块详细文档
```

## 设计原则

### 单一职责原则
- 每个.c文件不超过500行
- 每个函数不超过50行
- 每个模块只负责一个功能领域

### 模块化设计
- 核心功能与辅助功能分离
- 位图管理独立于分配逻辑
- 调试功能可独立启用/禁用
- 分散-聚集支持可选

### DMA特定优化
- 缓存行对齐分配（默认64字节）
- 物理连续内存分配支持
- 缓存一致性自动管理
- 内存属性配置（不可缓存、设备内存等）

## 数据类型

### DMA内存属性枚举

```c
typedef enum Eum_CN_DmaMemoryAttribute_t
{
    Eum_DMA_ATTRIBUTE_NONE = 0,           /**< 无特殊属性，使用默认设置 */
    Eum_DMA_ATTRIBUTE_UNCACHED = 1,       /**< 不可缓存，用于设备直接访问 */
    Eum_DMA_ATTRIBUTE_WRITE_COMBINE = 2,  /**< 写合并，提高写入性能 */
    Eum_DMA_ATTRIBUTE_WRITE_THROUGH = 3,  /**< 写穿透，写入同时更新缓存和内存 */
    Eum_DMA_ATTRIBUTE_WRITE_BACK = 4,     /**< 写回，延迟写入内存 */
    Eum_DMA_ATTRIBUTE_DEVICE = 5,         /**< 设备内存，强排序，不可缓存 */
    Eum_DMA_ATTRIBUTE_NORMAL = 6          /**< 普通内存，可缓存 */
} Eum_CN_DmaMemoryAttribute_t;
```

### DMA方向枚举

```c
typedef enum Eum_CN_DmaDirection_t
{
    Eum_DMA_DIRECTION_TO_DEVICE = 0,      /**< 从内存到设备 */
    Eum_DMA_DIRECTION_FROM_DEVICE = 1,    /**< 从设备到内存 */
    Eum_DMA_DIRECTION_BIDIRECTIONAL = 2,  /**< 双向传输 */
    Eum_DMA_DIRECTION_NONE = 3            /**< 无DMA传输（仅分配） */
} Eum_CN_DmaDirection_t;
```

### DMA内存分配器配置结构体

```c
typedef struct Stru_CN_DmaConfig_t
{
    uintptr_t dma_region_start;          /**< DMA区域起始地址 */
    uintptr_t dma_region_end;            /**< DMA区域结束地址 */
    size_t page_size;                    /**< 页面大小（字节），默认4096 */
    size_t cache_line_size;              /**< 缓存行大小（字节），默认64 */
    bool enable_statistics;              /**< 是否启用统计信息 */
    bool enable_debug;                   /**< 是否启用调试模式 */
    bool zero_on_alloc;                  /**< 分配时是否清零内存 */
    bool track_allocations;              /**< 是否跟踪分配信息 */
    Eum_CN_DmaMemoryAttribute_t default_attribute; /**< 默认内存属性 */
    const char* name;                    /**< 分配器名称（用于调试） */
    bool support_scatter_gather;         /**< 是否支持分散-聚集DMA */
    bool require_physical_contiguous;    /**< 是否要求物理连续内存 */
    size_t max_scatter_elements;         /**< 最大分散元素数量（如果支持） */
} Stru_CN_DmaConfig_t;
```

### 默认配置宏

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

### DMA内存分配器统计信息结构体

```c
typedef struct Stru_CN_DmaStats_t
{
    size_t total_pages;                  /**< 总页面数量 */
    size_t free_pages;                   /**< 空闲页面数量 */
    size_t used_pages;                   /**< 已使用页面数量 */
    size_t allocation_count;             /**< 分配次数 */
    size_t free_count;                   /**< 释放次数 */
    size_t allocation_failures;          /**< 分配失败次数 */
    size_t cache_flush_count;            /**< 缓存刷新次数 */
    size_t cache_invalidate_count;       /**< 缓存失效次数 */
    size_t sync_operations;              /**< 同步操作次数 */
    size_t fragmentation;                /**< 内存碎片化程度（0-100） */
    uintptr_t largest_free_block;        /**< 最大连续空闲块大小（页面数） */
} Stru_CN_DmaStats_t;
```

### DMA缓冲区描述符

```c
typedef struct Stru_CN_DmaBuffer_t
{
    uintptr_t physical_address;          /**< 物理地址（设备可见） */
    void* virtual_address;               /**< 虚拟地址（CPU可见） */
    size_t size;                         /**< 缓冲区大小（字节） */
    size_t alignment;                    /**< 对齐要求（字节） */
    Eum_CN_DmaMemoryAttribute_t attribute; /**< 内存属性 */
    Eum_CN_DmaDirection_t direction;     /**< DMA传输方向 */
    bool is_coherent;                    /**< 是否一致（缓存一致） */
    bool is_contiguous;                  /**< 是否物理连续 */
    uint32_t handle;                     /**< 缓冲区句柄（用于标识） */
} Stru_CN_DmaBuffer_t;
```

### DMA分散-聚集列表

```c
typedef struct Stru_CN_DmaScatterGatherElement_t
{
    uintptr_t physical_address;          /**< 物理地址 */
    size_t length;                       /**< 长度（字节） */
} Stru_CN_DmaScatterGatherElement_t;

typedef struct Stru_CN_DmaScatterGatherList_t
{
    Stru_CN_DmaScatterGatherElement_t* elements; /**< 元素数组 */
    size_t element_count;                /**< 元素数量 */
    size_t total_length;                 /**< 总长度（字节） */
    bool is_mapped;                      /**< 是否已映射到设备地址空间 */
} Stru_CN_DmaScatterGatherList_t;
```

## 函数参考

### 分配器管理接口

#### `CN_dma_create`

```c
Stru_CN_DmaAllocator_t* CN_dma_create(const Stru_CN_DmaConfig_t* config);
```

创建DMA内存分配器。

**参数：**
- `config`：DMA内存分配器配置

**返回值：**
- 成功：DMA内存分配器句柄
- 失败：NULL

**示例：**
```c
Stru_CN_DmaConfig_t config = CN_DMA_CONFIG_DEFAULT;
config.dma_region_start = 0x100000;  // 1MB
config.dma_region_end = 0x200000;    // 2MB
config.name = "测试DMA分配器";

Stru_CN_DmaAllocator_t* allocator = CN_dma_create(&config);
if (!allocator) {
    fprintf(stderr, "创建DMA分配器失败\n");
    return EXIT_FAILURE;
}
```

#### `CN_dma_destroy`

```c
void CN_dma_destroy(Stru_CN_DmaAllocator_t* allocator);
```

销毁DMA内存分配器。

**参数：**
- `allocator`：DMA内存分配器句柄

**注意：**
- 如果存在未释放的内存，会输出警告信息
- 分配器销毁后不能再使用

**示例：**
```c
CN_dma_destroy(allocator);
allocator = NULL; // 避免悬空指针
```

### DMA缓冲区分配接口

#### `CN_dma_alloc_buffer`

```c
Stru_CN_DmaBuffer_t* CN_dma_alloc_buffer(Stru_CN_DmaAllocator_t* allocator,
                                         size_t size,
                                         size_t alignment,
                                         Eum_CN_DmaMemoryAttribute_t attribute,
                                         const char* file, int line,
                                         const char* purpose);
```

分配DMA缓冲区（缓存行对齐）。

**参数：**
- `allocator`：DMA内存分配器句柄
- `size`：要分配的缓冲区大小（字节）
- `alignment`：对齐要求（必须是缓存行大小的倍数）
- `attribute`：内存属性
- `file`：调用文件名（用于调试，可为NULL）
- `line`：调用行号（用于调试）
- `purpose`：分配目的描述（可选，可为NULL）

**返回值：**
- 成功：DMA缓冲区描述符指针
- 失败：NULL

**示例：**
```c
Stru_CN_DmaBuffer_t* buffer = CN_dma_alloc_buffer(
    allocator,
    16 * 1024,                      // 16KB
    64,                             // 64字节对齐（缓存行）
    Eum_DMA_ATTRIBUTE_UNCACHED,     // 不可缓存
    __FILE__, __LINE__,
    "测试DMA缓冲区"
);

if (!buffer) {
    fprintf(stderr, "分配DMA缓冲区失败\n");
    return;
}
```

#### `CN_dma_alloc_contiguous`

```c
Stru_CN_DmaBuffer_t* CN_dma_alloc_contiguous(Stru_CN_DmaAllocator_t* allocator,
                                             size_t size,
                                             size_t alignment,
                                             Eum_CN_DmaMemoryAttribute_t attribute,
                                             const char* file, int line,
                                             const char* purpose);
```

分配物理连续的DMA缓冲区。

**参数：**
- `allocator`：DMA内存分配器句柄
- `size`：要分配的缓冲区大小（字节）
- `alignment`：对齐要求（必须是页面大小的倍数）
- `attribute`：内存属性
- `file`：调用文件名（用于调试，可为NULL）
- `line`：调用行号（用于调试）
- `purpose`：分配目的描述（可选，可为NULL）

**返回值：**
- 成功：DMA缓冲区描述符指针
- 失败：NULL

**注意：**
- 物理连续内存对于某些DMA设备是必需的
- 分配失败的可能性比普通分配更高

**示例：**
```c
Stru_CN_DmaBuffer_t* contiguous_buffer = CN_dma_alloc_contiguous(
    allocator,
    64 * 1024,                      // 64KB
    4096,                           // 4KB页面对齐
    Eum_DMA_ATTRIBUTE_DEVICE,       // 设备内存属性
    __FILE__, __LINE__,
    "物理连续DMA缓冲区"
);
```

#### `CN_dma_alloc_scatter_gather`

```c
Stru_CN_DmaScatterGatherList_t* CN_dma_alloc_scatter_gather(
    Stru_CN_DmaAllocator_t* allocator,
    size_t total_size,
    size_t max_elements,
    Eum_CN_DmaMemoryAttribute_t attribute,
    const char* file, int line,
    const char* purpose);
```

分配分散-聚集DMA缓冲区。

**参数：**
- `allocator`：DMA内存分配器句柄
- `total_size`：总缓冲区大小（字节）
- `max_elements`：最大分散元素数量
- `attribute`：内存属性
- `file`：调用文件名（用于调试，可为NULL）
- `line`：调用行号（用于调试）
- `purpose`：分配目的描述（可选，可为NULL）

**返回值：**
- 成功：分散-聚集列表指针
- 失败：NULL

**注意：**
- 需要配置中启用`support_scatter_gather`
- 适用于不连续内存区域的DMA传输

**示例：**
```c
Stru_CN_DmaScatterGatherList_t* sg_list = CN_dma_alloc_scatter_gather(
    allocator,
    128 * 1024,                     // 128KB
    8,                              // 最多8个分散元素
    Eum_DMA_ATTRIBUTE_UNCACHED,
    __FILE__, __LINE__,
    "分散-聚集DMA缓冲区"
);
```

#### `CN_dma_free_buffer`

```c
void CN_dma_free_buffer(Stru_CN_DmaAllocator_t* allocator,
                        Stru_CN_DmaBuffer_t* buffer,
                        const char* file, int line);
```

释放DMA缓冲区。

**参数：**
- `allocator`：DMA内存分配器句柄
- `buffer`：要释放的DMA缓冲区
- `file`：调用文件名（用于调试，可为NULL）
- `line`：调用行号（用于调试）

**示例：**
```c
CN_dma_free_buffer(allocator, buffer, __FILE__, __LINE__);
buffer = NULL; // 避免悬空指针
```

#### `CN_dma_free_scatter_gather`

```c
void CN_dma_free_scatter_gather(Stru_CN_DmaAllocator_t* allocator,
                                Stru_CN_DmaScatterGatherList_t* sg_list,
                                const char* file, int line);
```

释放分散-聚集DMA缓冲区。

**参数：**
- `allocator`：DMA内存分配器句柄
- `sg_list`：要释放的分散-聚集列表
- `file`：调用文件名（用于调试，可为NULL）
- `line`：调用行号（用于调试）

**示例：**
```c
CN_dma_free_scatter_gather(allocator, sg_list, __FILE__, __LINE__);
sg_list = NULL;
```

### DMA缓冲区同步接口

#### `CN_dma_sync_to_device`

```c
void CN_dma_sync_to_device(Stru_CN_DmaBuffer_t* buffer,
                           size_t offset, size_t size);
```

同步DMA缓冲区（CPU到设备）。

**参数：**
- `buffer`：DMA缓冲区
- `offset`：偏移量（字节）
- `size`：同步大小（字节），0表示整个缓冲区

**注意：**
- 在CPU写入数据后调用，确保数据对设备可见
- 对于不可缓存内存，可能不需要同步

**示例：**
```c
// CPU写入数据
memset(buffer->virtual_address, 0xAA, buffer->size);

// 同步到设备
CN_dma_sync_to_device(buffer, 0, 0); // 同步整个缓冲区
```

#### `CN_dma_sync_from_device`

```c
void CN_dma_sync_from_device(Stru_CN_DmaBuffer_t* buffer,
                             size_t offset, size_t size);
```

同步DMA缓冲区（设备到CPU）。

**参数：**
- `buffer`：DMA缓冲区
- `offset`：偏移量（字节）
- `size`：同步大小（字节），0表示整个缓冲区

**注意：**
- 在设备写入数据后调用，确保数据对CPU可见
- 对于不可缓存内存，可能不需要同步

**示例：**
```c
// 设备进行DMA传输...

// 从设备同步
CN_dma_sync_from_device(buffer, 0, 0);

// CPU读取数据
uint8_t* data = (uint8_t*)buffer->virtual_address;
```

#### `CN_dma_sync_sg_to_device`

```c
void CN_dma_sync_sg_to_device(Stru_CN_DmaScatterGatherList_t* sg_list);
```

同步分散-聚集DMA缓冲区（CPU到设备）。

**参数：**
- `sg_list`：分散-聚集列表

**示例：**
```c
CN_dma_sync_sg_to_device(sg_list);
```

#### `CN_dma_sync_sg_from_device`

```c
void CN_dma_sync_sg_from_device(Stru_CN_DmaScatterGatherList_t* sg_list);
```

同步分散-聚集DMA缓冲区（设备到CPU）。

**参数：**
- `sg_list`：分散-聚集列表

**示例：**
```c
CN_dma_sync_sg_from_device(sg_list);
```

### DMA内存分配器查询接口

#### `CN_dma_get_config`

```c
bool CN_dma_get_config(Stru_CN_DmaAllocator_t* allocator,
                       Stru_CN_DmaConfig_t* config);
```

获取DMA内存分配器配置。

**参数：**
- `allocator`：DMA内存分配器句柄
- `config`：输出配置信息

**返回值：**
- 获取成功返回true
- 失败返回false

**示例：**
```c
Stru_CN_DmaConfig_t config;
if (CN_dma_get_config(allocator, &config)) {
    printf("分配器名称：%s\n", config.name);
    printf("页面大小：%zu 字节\n", config.page_size);
}
```

#### `CN_dma_get_stats`

```c
bool CN_dma_get_stats(Stru_CN_DmaAllocator_t* allocator,
                      Stru_CN_DmaStats_t* stats);
```

获取DMA内存分配器统计信息。

**参数：**
- `allocator`：DMA内存分配器句柄
- `stats`：输出统计信息

**返回值：**
- 获取成功返回true
- 失败返回false

**示例：**
```c
Stru_CN_DmaStats_t stats;
if (CN_dma_get_stats(allocator, &stats)) {
    printf("总页面数：%zu\n", stats.total_pages);
    printf("已使用页面：%zu\n", stats.used_pages);
    printf("空闲页面：%zu\n", stats.free_pages);
}
```

#### `CN_dma_reset_stats`

```c
void CN_dma_reset_stats(Stru_CN_DmaAllocator_t* allocator);
```

重置DMA内存分配器统计信息。

**参数：**
- `allocator`：DMA内存分配器句柄

**示例：**
```c
// 重置统计信息，开始新的性能测量
CN_dma_reset_stats(allocator);
```

#### `CN_dma_total_pages`

```c
size_t CN_dma_total_pages(Stru_CN_DmaAllocator_t* allocator);
```

获取总页面数量。

**参数：**
- `allocator`：DMA内存分配器句柄

**返回值：**
总页面数量

**示例：**
```c
size_t total = CN_dma_total_pages(allocator);
printf("总页面数：%zu\n", total);
```

#### `CN_dma_get_free_pages`

```c
size_t CN_dma_get_free_pages(Stru_CN_DmaAllocator_t* allocator);
```

获取空闲页面数量。

**参数：**
- `allocator`：DMA内存分配器句柄

**返回值：**
空闲页面数量

**示例：**
```c
size_t free_pages = CN_dma_get_free_pages(allocator);
printf("空闲页面数：%zu\n", free_pages);
```

#### `CN_dma_used_pages`

```c
size_t CN_dma_used_pages(Stru_CN_DmaAllocator_t* allocator);
```

获取已使用页面数量。

**参数：**
- `allocator`：DMA内存分配器句柄

**返回值：**
已使用页面数量

**示例：**
```c
size_t used_pages = CN_dma_used_pages(allocator);
printf("已使用页面数：%zu\n", used_pages);
```

#### `CN_dma_largest_free_block`

```c
size_t CN_dma_largest_free_block(Stru_CN_DmaAllocator_t* allocator);
```

获取最大连续空闲页面数量。

**参数：**
- `allocator`：DMA内存分配器句柄

**返回值：**
最大连续空闲页面数量

**示例：**
```c
size_t largest_block = CN_dma_largest_free_block(allocator);
printf("最大连续空闲块：%zu 页面\n", largest_block);
```

### DMA内存分配器调试接口

#### `CN_dma_enable_debug`

```c
void CN_dma_enable_debug(Stru_CN_DmaAllocator_t* allocator, bool enable);
```

启用/禁用调试模式。

**参数：**
- `allocator`：DMA内存分配器句柄
- `enable`：是否启用调试

**示例：**
```c
// 启用调试模式
CN_dma_enable_debug(allocator, true);

// 分配会输出调试信息
Stru_CN_DmaBuffer_t* buffer = CN_dma_alloc_buffer(allocator, 4096, 64, 
    Eum_DMA_ATTRIBUTE_UNCACHED, __FILE__, __LINE__, "调试测试");

// 禁用调试模式
CN_dma_enable_debug(allocator, false);
```

#### `CN_dma_validate`

```c
bool CN_dma_validate(Stru_CN_DmaAllocator_t* allocator);
```

验证DMA内存分配器完整性。

**参数：**
- `allocator`：DMA内存分配器句柄

**返回值：**
- 如果分配器完整返回true
- 否则返回false

**示例：**
```c
if (!CN_dma_validate(allocator)) {
    fprintf(stderr, "DMA分配器完整性验证失败\n");
    CN_dma_dump(allocator); // 转储状态以便调试
}
```

#### `CN_dma_dump`

```c
void CN_dma_dump(Stru_CN_DmaAllocator_t* allocator);
```

转储DMA内存分配器状态到标准输出。

**参数：**
- `allocator`：DMA内存分配器句柄

**示例：**
```c
// 转储分配器状态
CN_dma_dump(allocator);

// 输出示例：
// === DMA内存分配器状态 ===
// 名称：测试DMA分配器
// DMA区域：0x100000 - 0x200000
// 页面大小：4096 字节
// 总页面数：256
// 已使用页面：128
// 空闲页面：128
// 最大连续空闲块：64 页面
// ===========================
```

#### `CN_dma_dump_leaks`

```c
void CN_dma_dump_leaks(Stru_CN_DmaAllocator_t* allocator);
```

转储所有未释放的分配信息。

**参数：**
- `allocator`：DMA内存分配器句柄

**示例：**
```c
// 程序结束前检查泄漏
CN_dma_dump_leaks(allocator);

// 输出示例：
// === 内存泄漏检测 ===
// 泄漏数量：2
// 1. 地址：0x100000，大小：4096，文件：test.c，行：10，目的：测试缓冲区1
// 2. 地址：0x101000，大小：8192，文件：test.c，行：15，目的：测试缓冲区2
// =====================
```

#### `CN_dma_set_debug_callback`

```c
typedef void (*CN_DmaDebugCallback_t)(const char* message, void* user_data);
void CN_dma_set_debug_callback(Stru_CN_DmaAllocator_t* allocator,
                               CN_DmaDebugCallback_t callback,
                               void* user_data);
```

设置DMA内存分配器调试回调。

**参数：**
- `allocator`：DMA内存分配器句柄
- `callback`：调试回调函数
- `user_data`：用户数据

**示例：**
```c
void dma_debug_callback(const char* message, void* user_data) {
    printf("[DMA分配器] %s\n", message);
}

// 设置调试回调
CN_dma_set_debug_callback(allocator, dma_debug_callback, NULL);

// 现在所有调试信息都会通过回调输出
```

### 辅助函数

#### `CN_dma_size_to_pages`

```c
static inline size_t CN_dma_size_to_pages(size_t size, size_t page_size)
{
    return (size + page_size - 1) / page_size;
}
```

将字节大小转换为页面数量。

**参数：**
- `size`：字节大小
- `page_size`：页面大小

**返回值：**
页面数量（向上取整）

**示例：**
```c
size_t page_count = CN_dma_size_to_pages(5000, 4096); // 返回2
```

#### `CN_dma_pages_to_size`

```c
static inline size_t CN_dma_pages_to_size(size_t page_count, size_t page_size)
{
    return page_count * page_size;
}
```

将页面数量转换为字节大小。

**参数：**
- `page_count`：页面数量
- `page_size`：页面大小

**返回值：**
字节大小

**示例：**
```c
size_t byte_size = CN_dma_pages_to_size(3, 4096); // 返回12288
```

#### `CN_dma_align_to_page`

```c
static inline uintptr_t CN_dma_align_to_page(uintptr_t address, size_t page_size)
{
    return (address + page_size - 1) & ~(page_size - 1);
}
```

对齐地址到页面边界。

**参数：**
- `address`：地址
- `page_size`：页面大小

**返回值：**
对齐后的地址

**示例：**
```c
uintptr_t aligned = CN_dma_align_to_page(0x100123, 4096); // 返回0x101000
```

#### `CN_dma_is_page_aligned`

```c
static inline bool CN_dma_is_page_aligned(uintptr_t address, size_t page_size)
{
    return (address & (page_size - 1)) == 0;
}
```

检查地址是否页面对齐。

**参数：**
- `address`：地址
- `page_size`：页面大小

**返回值：**
- 如果对齐返回true
- 否则返回false

**示例：**
```c
bool aligned = CN_dma_is_page_aligned(0x100000, 4096); // 返回true
aligned = CN_dma_is_page_aligned(0x100123, 4096);      // 返回false
```

#### `CN_dma_align_to_cache_line`

```c
static inline uintptr_t CN_dma_align_to_cache_line(uintptr_t address, size_t cache_line_size)
{
    return (address + cache_line_size - 1) & ~(cache_line_size - 1);
}
```

对齐地址到缓存行边界。

**参数：**
- `address`：地址
- `cache_line_size`：缓存行大小

**返回值：**
对齐后的地址

**示例：**
```c
uintptr_t aligned = CN_dma_align_to_cache_line(0x100123, 64); // 返回0x100140
```

#### `CN_dma_is_cache_line_aligned`

```c
static inline bool CN_dma_is_cache_line_aligned(uintptr_t address, size_t cache_line_size)
{
    return (address & (cache_line_size - 1)) == 0;
}
```

检查地址是否缓存行对齐。

**参数：**
- `address`：地址
- `cache_line_size`：缓存行大小

**返回值：**
- 如果对齐返回true
- 否则返回false

**示例：**
```c
bool aligned = CN_dma_is_cache_line_aligned(0x100140, 64); // 返回true
aligned = CN_dma_is_cache_line_aligned(0x100123, 64);      // 返回false
```

## 使用示例

### 示例1：基本DMA内存分配

```c
#include "CN_dma_allocator.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    // 配置DMA内存分配器
    Stru_CN_DmaConfig_t config = CN_DMA_CONFIG_DEFAULT;
    config.dma_region_start = 0x100000;  // 1MB
    config.dma_region_end = 0x200000;    // 2MB
    config.name = "基本DMA分配器示例";
    
    // 创建分配器
    Stru_CN_DmaAllocator_t* allocator = CN_dma_create(&config);
    if (!allocator) {
        fprintf(stderr, "创建DMA分配器失败\n");
        return EXIT_FAILURE;
    }
    
    // 分配16KB DMA缓冲区
    Stru_CN_DmaBuffer_t* buffer = CN_dma_alloc_buffer(
        allocator,
        16 * 1024,                      // 16KB
        64,                             // 64字节对齐（缓存行）
        Eum_DMA_ATTRIBUTE_UNCACHED,     // 不可缓存
        __FILE__, __LINE__,
        "示例缓冲区"
    );
    
    if (!buffer) {
        fprintf(stderr, "分配DMA缓冲区失败\n");
        CN_dma_destroy(allocator);
        return EXIT_FAILURE;
    }
    
    printf("分配成功：\n");
    printf("  物理地址：0x%llx\n", (unsigned long long)buffer->physical_address);
    printf("  虚拟地址：%p\n", buffer->virtual_address);
    printf("  大小：%zu 字节\n", buffer->size);
    printf("  对齐：%zu 字节\n", buffer->alignment);
    printf("  是否连续：%s\n", buffer->is_contiguous ? "是" : "否");
    
    // 使用DMA缓冲区
    // 1. CPU写入数据
    memset(buffer->virtual_address, 0xAA, buffer->size);
    
    // 2. 同步到设备
    CN_dma_sync_to_device(buffer, 0, 0);
    
    // 3. 模拟设备DMA传输...
    
    // 4. 从设备同步
    CN_dma_sync_from_device(buffer, 0, 0);
    
    // 5. CPU读取数据
    // uint8_t* data = (uint8_t*)buffer->virtual_address;
    
    // 释放DMA缓冲区
    CN_dma_free_buffer(allocator, buffer, __FILE__, __LINE__);
    
    // 销毁分配器
    CN_dma_destroy(allocator);
    
    return EXIT_SUCCESS;
}
```

### 示例2：物理连续DMA缓冲区

```c
#include "CN_dma_allocator.h"

void contiguous_dma_example(void)
{
    Stru_CN_DmaConfig_t config = CN_DMA_CONFIG_DEFAULT;
    config.dma_region_start = 0x200000;
    config.dma_region_end = 0x400000;
    config.require_physical_contiguous = true;
    config.name = "物理连续DMA示例";
    
    Stru_CN_DmaAllocator_t* allocator = CN_dma_create(&config);
    if (!allocator) {
        return;
    }
    
    // 分配物理连续的DMA缓冲区
    Stru_CN_DmaBuffer_t* contiguous_buffer = CN_dma_alloc_contiguous(
        allocator,
        64 * 1024,                      // 64KB
        4096,                           // 4KB页面对齐
        Eum_DMA_ATTRIBUTE_DEVICE,       // 设备内存属性
        __FILE__, __LINE__,
        "物理连续缓冲区"
    );
    
    if (contiguous_buffer) {
        printf("物理连续缓冲区分配成功：\n");
        printf("  物理地址：0x%llx\n", 
               (unsigned long long)contiguous_buffer->physical_address);
        printf("  是否连续：%s\n", 
               contiguous_buffer->is_contiguous ? "是" : "否");
        
        // 使用物理连续缓冲区...
        
        CN_dma_free_buffer(allocator, contiguous_buffer, __FILE__, __LINE__);
    }
    
    CN_dma_destroy(allocator);
}
```

### 示例3：调试和跟踪

```c
#include "CN_dma_allocator.h"

void debug_and_tracking_example(void)
{
    Stru_CN_DmaConfig_t config = CN_DMA_CONFIG_DEFAULT;
    config.dma_region_start = 0x300000;
    config.dma_region_end = 0x500000;
    config.enable_debug = true;
    config.track_allocations = true;
    config.name = "调试和跟踪示例";
    
    Stru_CN_DmaAllocator_t* allocator = CN_dma_create(&config);
    if (!allocator) {
        return;
    }
    
    // 设置调试回调
    void debug_callback(const char* message, void* user_data) {
        printf("[DMA调试] %s\n", message);
    }
    
    CN_dma_set_debug_callback(allocator, debug_callback, NULL);
    
    // 启用调试模式
    CN_dma_enable_debug(allocator, true);
    
    // 分配几个缓冲区
    Stru_CN_DmaBuffer_t* buffer1 = CN_dma_alloc_buffer(
        allocator, 4096, 64, Eum_DMA_ATTRIBUTE_UNCACHED,
        __FILE__, __LINE__, "缓冲区1");
    
    Stru_CN_DmaBuffer_t* buffer2 = CN_dma_alloc_buffer(
        allocator, 8192, 64, Eum_DMA_ATTRIBUTE_UNCACHED,
        __FILE__, __LINE__, "缓冲区2");
    
    // 验证分配器完整性
    if (!CN_dma_validate(allocator)) {
        printf("分配器验证失败\n");
    }
    
    // 转储分配器状态
    CN_dma_dump(allocator);
    
    // 释放缓冲区（故意留下一个泄漏用于演示）
    CN_dma_free_buffer(allocator, buffer1, __FILE__, __LINE__);
    // buffer2 故意不释放，用于演示泄漏检测
    
    // 检查泄漏
    printf("\n=== 内存泄漏检测 ===\n");
    CN_dma_dump_leaks(allocator);
    
    // 禁用调试模式
    CN_dma_enable_debug(allocator, false);
    
    // 销毁分配器
    CN_dma_destroy(allocator);
}
```

## 错误处理

### 内存分配失败

```c
Stru_CN_DmaBuffer_t* buffer = CN_dma_alloc_buffer(
    allocator,
    very_large_size,
    64,
    Eum_DMA_ATTRIBUTE_UNCACHED,
    __FILE__, __LINE__,
    "大缓冲区"
);

if (buffer == NULL) {
    // 处理内存不足
    fprintf(stderr, "DMA内存分配失败，请求大小: %zu\n", very_large_size);
    
    // 获取统计信息了解当前状态
    Stru_CN_DmaStats_t stats;
    if (CN_dma_get_stats(allocator, &stats)) {
        fprintf(stderr, "当前状态: 总页面=%zu, 已使用=%zu, 空闲=%zu, 最大连续块=%zu\n",
                stats.total_pages, stats.used_pages, stats.free_pages, 
                stats.largest_free_block);
    }
    
    // 尝试释放一些内存后重试
    free_some_dma_memory();
    buffer = CN_dma_alloc_buffer(allocator, very_large_size, 64, 
                                 Eum_DMA_ATTRIBUTE_UNCACHED,
                                 __FILE__, __LINE__, "大缓冲区");
    
    if (buffer == NULL) {
        // 仍然失败，采取降级策略或退出
        return ERROR_OUT_OF_DMA_MEMORY;
    }
}
```

### 配置错误处理

```c
Stru_CN_DmaConfig_t config = CN_DMA_CONFIG_DEFAULT;
config.dma_region_start = 0x100000;
config.dma_region_end = 0x0FFFFF; // 错误：结束地址小于起始地址

Stru_CN_DmaAllocator_t* allocator = CN_dma_create(&config);
if (allocator == NULL) {
    fprintf(stderr, "DMA分配器创建失败：无效的配置参数\n");
    
    // 验证配置参数
    if (config.dma_region_start >= config.dma_region_end) {
        fprintf(stderr, "错误：DMA区域起始地址(0x%llx)必须小于结束地址(0x%llx)\n",
                (unsigned long long)config.dma_region_start,
                (unsigned long long)config.dma_region_end);
    }
    
    if (config.page_size == 0) {
        fprintf(stderr, "错误：页面大小不能为0\n");
    }
    
    return EXIT_FAILURE;
}
```

### 同步操作错误

```c
// 同步操作前验证缓冲区
if (buffer == NULL || buffer->virtual_address == NULL) {
    fprintf(stderr, "错误：无效的DMA缓冲区\n");
    return;
}

// 验证同步范围
if (offset >= buffer->size) {
    fprintf(stderr, "错误：偏移量(%zu)超出缓冲区大小(%zu)\n", 
            offset, buffer->size);
    return;
}

// 计算实际同步大小
size_t actual_size = size;
if (size == 0) {
    actual_size = buffer->size - offset;
} else if (offset + size > buffer->size) {
    fprintf(stderr, "警告：同步范围超出缓冲区边界，调整为有效范围\n");
    actual_size = buffer->size - offset;
}

// 执行同步
CN_dma_sync_to_device(buffer, offset, actual_size);
```

## 最佳实践

### 1. 配置管理
- 明确设置DMA区域范围，避免与其他内存冲突
- 根据硬件特性设置合适的页面大小和缓存行大小
- 在生产环境中禁用调试模式以提高性能
- 为分配器设置有意义的名称便于调试

### 2. 内存分配
- 根据DMA设备要求选择合适的内存属性
- 对于需要物理连续内存的设备，使用`CN_dma_alloc_contiguous`
- 对齐要求应符合硬件规范（通常是缓存行大小）
- 分配时提供有意义的分配目的描述

### 3. 同步操作
- 只在必要时进行同步操作
- 对于不可缓存内存，可能不需要同步
- 批量操作时考虑同步开销
- 使用合适的同步方向（CPU到设备或设备到CPU）

### 4. 调试和监控
- 开发阶段启用分配跟踪
- 定期验证分配器完整性
- 监控内存碎片化程度
- 使用统计信息优化内存使用

### 5. 资源管理
- 谁分配谁释放，保持所有权清晰
- 在程序关键点检查内存泄漏
- 避免在DMA传输过程中释放缓冲区
- 分配器销毁前确保所有缓冲区已释放

## 性能考虑

### 时间复杂度
- 页面分配：O(n)，使用优化搜索算法
- 页面释放：O(1)
- 同步操作：O(1) 或 O(n)（取决于缓冲区大小）
- 统计查询：O(1)

### 空间复杂度
- 位图大小：总页面数/8 字节
- 分配跟踪：每个跟踪记录约64字节
- DMA缓冲区描述符：每个缓冲区约48字节

### 优化策略
1. **搜索优化**：记录上次搜索位置，减少搜索时间
2. **位图压缩**：使用位图而不是数组跟踪页面状态
3. **缓存友好**：位图数据连续存储，提高缓存命中率
4. **对齐优化**：预计算对齐页面，减少运行时计算

## 兼容性说明

### 平台依赖性
- 缓存操作需要平台特定的指令
- 内存属性设置可能因平台而异
- 物理-虚拟地址映射机制平台相关

### 线程安全性
当前实现不是线程安全的。如果需要在多线程环境中使用：
1. 每个线程使用独立的DMA分配器
2. 或添加互斥锁保护关键操作
3. 或使用线程本地存储

### 与标准库的兼容性
- 使用标准C库，无特殊依赖
- 调试信息使用`__FILE__`和`__LINE__`宏
- 内存分配使用系统malloc/free

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-03 | 初始版本，支持基本DMA内存分配 |
| 1.0.1 | 计划 | 修复已知问题，优化性能 |
| 1.1.0 | 计划 | 添加完整分散-聚集支持 |
| 1.2.0 | 计划 | 添加平台特定优化 |

## 相关文档

- [DMA内存分配器模块README](../../../../src/infrastructure/memory/dma/README.md)
- [内存管理框架API文档](../CN_memory_API.md)
- [物理内存分配器API文档](../CN_physical_allocator_API.md)
- [架构设计原则](../../../architecture/架构设计原则.md)
- [编码规范](../../../specifications/CN_Language项目%20技术规范和编码标准.md)

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language开发团队

## 最后更新

2026年1月3日
