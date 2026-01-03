# CN_physical_allocator API 文档

## 概述

`CN_physical_allocator` 模块提供物理内存页框分配功能，模拟操作系统内核中的物理内存管理。该模块是CN_Language基础设施层的关键组件，专门用于操作系统开发和嵌入式系统开发场景，支持页框分配、对齐分配、特定地址分配等功能。

## 文件结构

```
src/infrastructure/memory/physical/
├── CN_physical_allocator.h      # 头文件 - 公共接口定义
├── CN_physical_allocator.c      # 源文件 - 实现代码
└── README.md                    # 模块详细文档
```

## 设计原则

1. **模块化设计**：遵循单一职责原则，专注于物理内存管理
2. **高性能**：使用位图算法实现快速页面分配和释放
3. **可配置性**：支持多种配置选项，适应不同应用场景
4. **调试支持**：提供完整的调试和跟踪功能，便于问题排查
5. **内存安全**：实现内存泄漏检测和完整性验证

## 数据类型

### 物理内存分配器配置结构体

```c
typedef struct Stru_CN_PhysicalConfig_t
{
    uintptr_t memory_start;       /**< 物理内存起始地址 */
    uintptr_t memory_end;         /**< 物理内存结束地址 */
    size_t page_size;             /**< 页面大小（字节），默认4096 */
    bool enable_statistics;       /**< 是否启用统计信息 */
    bool enable_debug;            /**< 是否启用调试模式 */
    bool zero_on_alloc;           /**< 分配时是否清零页面 */
    bool track_allocations;       /**< 是否跟踪分配信息 */
    size_t reserved_pages;        /**< 保留的页面数量（用于内核等） */
    const char* name;             /**< 分配器名称（用于调试） */
} Stru_CN_PhysicalConfig_t;
```

### 默认配置宏

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

### 页面大小常量

```c
/** @brief 标准页面大小（4KB） */
#define CN_PAGE_SIZE_4KB    4096UL

/** @brief 大页面大小（2MB） */
#define CN_PAGE_SIZE_2MB    (2UL * 1024UL * 1024UL)

/** @brief 巨大页面大小（1GB） */
#define CN_PAGE_SIZE_1GB    (1024UL * 1024UL * 1024UL)
```

### 物理内存分配器统计信息结构体

```c
typedef struct Stru_CN_PhysicalStats_t
{
    size_t total_pages;           /**< 总页面数量 */
    size_t free_pages;            /**< 空闲页面数量 */
    size_t used_pages;            /**< 已使用页面数量 */
    size_t reserved_pages;        /**< 保留页面数量 */
    size_t allocation_count;      /**< 分配次数 */
    size_t free_count;            /**< 释放次数 */
    size_t allocation_failures;   /**< 分配失败次数 */
    size_t fragmentation;         /**< 内存碎片化程度（0-100） */
    uintptr_t largest_free_block; /**< 最大连续空闲块大小（页面数） */
} Stru_CN_PhysicalStats_t;
```

### 物理内存分配信息记录结构体

```c
typedef struct Stru_CN_PhysicalAllocationInfo_t
{
    uintptr_t physical_address;   /**< 分配的物理地址 */
    size_t page_count;            /**< 分配的页面数量 */
    const char* file;             /**< 分配所在的文件 */
    int line;                     /**< 分配所在的行号 */
    uint64_t timestamp;           /**< 分配时间戳 */
    const char* purpose;          /**< 分配目的（可选） */
} Stru_CN_PhysicalAllocationInfo_t;
```

### 物理内存分配器句柄

```c
typedef struct Stru_CN_PhysicalAllocator_t Stru_CN_PhysicalAllocator_t;
```

### 调试回调函数类型

```c
typedef void (*CN_PhysicalDebugCallback_t)(const char* message, void* user_data);
```

## 函数参考

### 分配器管理接口

#### `CN_physical_create`

```c
Stru_CN_PhysicalAllocator_t* CN_physical_create(const Stru_CN_PhysicalConfig_t* config);
```

创建物理内存分配器。

**参数：**
- `config`：物理内存分配器配置指针

**返回值：**
- 成功：物理内存分配器句柄
- 失败：NULL（配置无效或内存不足）

**前置条件：**
- `config` 不能为NULL
- `config->memory_start` 必须小于 `config->memory_end`
- `config->page_size` 必须大于0

**后置条件：**
- 返回的分配器已初始化完成
- 所有页面初始状态为空闲（除非配置了保留页面）

**示例：**
```c
Stru_CN_PhysicalConfig_t config = CN_PHYSICAL_CONFIG_DEFAULT;
config.memory_start = 0x100000;  // 1MB
config.memory_end = 0x200000;    // 2MB
config.page_size = CN_PAGE_SIZE_4KB;
config.name = "测试分配器";

Stru_CN_PhysicalAllocator_t* allocator = CN_physical_create(&config);
if (allocator == NULL) {
    fprintf(stderr, "创建物理内存分配器失败\n");
    return EXIT_FAILURE;
}
```

#### `CN_physical_destroy`

```c
void CN_physical_destroy(Stru_CN_PhysicalAllocator_t* allocator);
```

销毁物理内存分配器，释放所有资源。

**参数：**
- `allocator`：物理内存分配器句柄

**注意：**
- 如果 `allocator` 为NULL，函数什么都不做
- 销毁时会检查内存泄漏并输出警告
- 所有通过该分配器分配的内存都会被标记为未分配

**示例：**
```c
Stru_CN_PhysicalAllocator_t* allocator = CN_physical_create(&config);
// 使用分配器...
CN_physical_destroy(allocator);
allocator = NULL; // 避免悬空指针
```

### 页面分配接口

#### `CN_physical_alloc_pages`

```c
uintptr_t CN_physical_alloc_pages(Stru_CN_PhysicalAllocator_t* allocator, 
                                  size_t page_count, 
                                  const char* file, int line, 
                                  const char* purpose);
```

从物理内存分配器分配连续页面。

**参数：**
- `allocator`：物理内存分配器句柄
- `page_count`：要分配的页面数量
- `file`：调用文件名（用于调试，可为NULL）
- `line`：调用行号（用于调试）
- `purpose`：分配目的描述（可选，可为NULL）

**返回值：**
- 成功：分配的物理地址（页面对齐）
- 失败：0（内存不足或参数无效）

**错误条件：**
- `allocator` 为NULL或未初始化
- `page_count` 为0或大于空闲页面数
- 找不到足够的连续空闲页面

**示例：**
```c
// 分配4个页面（16KB）
uintptr_t address = CN_physical_alloc_pages(allocator, 4, __FILE__, __LINE__, "缓冲区");
if (address == 0) {
    fprintf(stderr, "页面分配失败\n");
    return;
}

printf("分配成功：物理地址=0x%llx\n", (unsigned long long)address);
```

#### `CN_physical_alloc_pages_aligned`

```c
uintptr_t CN_physical_alloc_pages_aligned(Stru_CN_PhysicalAllocator_t* allocator, 
                                          size_t page_count, 
                                          size_t alignment,
                                          const char* file, int line, 
                                          const char* purpose);
```

从物理内存分配器分配对齐的连续页面。

**参数：**
- `allocator`：物理内存分配器句柄
- `page_count`：要分配的页面数量
- `alignment`：对齐要求（必须是页面大小的倍数）
- `file`：调用文件名（用于调试，可为NULL）
- `line`：调用行号（用于调试）
- `purpose`：分配目的描述（可选，可为NULL）

**返回值：**
- 成功：分配的物理地址（满足对齐要求）
- 失败：0（内存不足、参数无效或找不到对齐的页面）

**特殊要求：**
- `alignment` 必须是 `allocator->config.page_size` 的倍数

**示例：**
```c
// 分配对齐到1MB边界的2个页面
uintptr_t address = CN_physical_alloc_pages_aligned(
    allocator, 
    2,                      // 2个页面
    1024 * 1024,            // 1MB对齐
    __FILE__, __LINE__,
    "DMA缓冲区"
);

if (address != 0) {
    printf("对齐分配成功：地址=0x%llx\n", (unsigned long long)address);
    // 地址是1MB对齐的
    assert((address & (1024 * 1024 - 1)) == 0);
}
```

#### `CN_physical_alloc_pages_at`

```c
uintptr_t CN_physical_alloc_pages_at(Stru_CN_PhysicalAllocator_t* allocator, 
                                     uintptr_t physical_address,
                                     size_t page_count, 
                                     const char* file, int line, 
                                     const char* purpose);
```

从物理内存分配器分配特定地址的页面。

**参数：**
- `allocator`：物理内存分配器句柄
- `physical_address`：请求的物理地址（必须是页面对齐的）
- `page_count`：要分配的页面数量
- `file`：调用文件名（用于调试，可为NULL）
- `line`：调用行号（用于调试）
- `purpose`：分配目的描述（可选，可为NULL）

**返回值：**
- 成功：分配的物理地址（与请求地址相同）
- 失败：0（地址已分配、参数无效或地址不在范围内）

**特殊要求：**
- `physical_address` 必须是页面对齐的
- 请求的地址范围必须完全空闲

**示例：**
```c
// 在特定地址0x100000分配2个页面
uintptr_t address = CN_physical_alloc_pages_at(
    allocator,
    0x100000,               // 特定地址
    2,                      // 2个页面
    __FILE__, __LINE__,
    "固定地址设备映射"
);

if (address != 0) {
    printf("特定地址分配成功：地址=0x%llx\n", (unsigned long long)address);
    assert(address == 0x100000); // 地址与请求相同
}
```

#### `CN_physical_free_pages`

```c
void CN_physical_free_pages(Stru_CN_PhysicalAllocator_t* allocator, 
                            uintptr_t physical_address,
                            size_t page_count, 
                            const char* file, int line);
```

释放物理内存分配器分配的页面。

**参数：**
- `allocator`：物理内存分配器句柄
- `physical_address`：要释放的物理地址
- `page_count`：要释放的页面数量
- `file`：调用文件名（用于调试，可为NULL）
- `line`：调用行号（用于调试）

**注意：**
- 如果 `physical_address` 为0或 `page_count` 为0，函数什么都不做
- 地址必须是页面对齐的
- 释放未分配或已释放的页面是安全的（会输出调试信息）

**示例：**
```c
// 分配页面
uintptr_t address = CN_physical_alloc_pages(allocator, 4, __FILE__, __LINE__, "临时缓冲区");

// 使用页面...

// 释放页面
CN_physical_free_pages(allocator, address, 4, __FILE__, __LINE__);
```

### 查询接口

#### `CN_physical_get_config`

```c
bool CN_physical_get_config(Stru_CN_PhysicalAllocator_t* allocator, 
                            Stru_CN_PhysicalConfig_t* config);
```

获取物理内存分配器配置。

**参数：**
- `allocator`：物理内存分配器句柄
- `config`：输出配置信息

**返回值：**
- `true`：获取成功
- `false`：获取失败（参数无效）

**示例：**
```c
Stru_CN_PhysicalConfig_t config;
if (CN_physical_get_config(allocator, &config)) {
    printf("分配器配置：\n");
    printf("  名称：%s\n", config.name);
    printf("  页面大小：%zu 字节\n", config.page_size);
    printf("  内存范围：0x%llx - 0x%llx\n", 
           (unsigned long long)config.memory_start,
           (unsigned long long)config.memory_end);
}
```

#### `CN_physical_get_stats`

```c
bool CN_physical_get_stats(Stru_CN_PhysicalAllocator_t* allocator, 
                           Stru_CN_PhysicalStats_t* stats);
```

获取物理内存分配器统计信息。

**参数：**
- `allocator`：物理内存分配器句柄
- `stats`：输出统计信息

**返回值：**
- `true`：获取成功
- `false`：获取失败（参数无效）

**统计信息说明：**
- `total_pages`：总页面数量
- `free_pages`：当前空闲页面数量
- `used_pages`：当前使用页面数量
- `reserved_pages`：保留页面数量
- `allocation_count`：分配次数
- `free_count`：释放次数
- `allocation_failures`：分配失败次数
- `fragmentation`：内存碎片化程度（0-100，0表示无碎片）
- `largest_free_block`：最大连续空闲块大小（页面数）

**示例：**
```c
Stru_CN_PhysicalStats_t stats;
if (CN_physical_get_stats(allocator, &stats)) {
    printf("分配器统计：\n");
    printf("  总页面：%zu\n", stats.total_pages);
    printf("  空闲页面：%zu\n", stats.free_pages);
    printf("  使用页面：%zu\n", stats.used_pages);
    printf("  碎片化：%zu%%\n", stats.fragmentation);
    printf("  最大连续空闲块：%zu 页面\n", stats.largest_free_block);
}
```

#### `CN_physical_reset_stats`

```c
void CN_physical_reset_stats(Stru_CN_PhysicalAllocator_t* allocator);
```

重置物理内存分配器统计信息。

**参数：**
- `allocator`：物理内存分配器句柄

**注意：**
- 只重置计数统计（分配次数、释放次数、失败次数、碎片化）
- 不重置页面统计（总页面、空闲页面、使用页面、保留页面）

**示例：**
```c
// 运行一段时间后重置统计
CN_physical_reset_stats(allocator);
printf("统计信息已重置\n");
```

#### `CN_physical_is_allocated`

```c
bool CN_physical_is_allocated(Stru_CN_PhysicalAllocator_t* allocator, 
                              uintptr_t physical_address);
```

检查物理地址是否已分配。

**参数：**
- `allocator`：物理内存分配器句柄
- `physical_address`：要检查的物理地址

**返回值：**
- `true`：地址已分配
- `false`：地址未分配或参数无效

**注意：**
- 地址不需要页面对齐，函数会检查地址所在的页面

**示例：**
```c
uintptr_t address = CN_physical_alloc_pages(allocator, 1, __FILE__, __LINE__, "测试");

if (CN_physical_is_allocated(allocator, address)) {
    printf("地址 0x%llx 已分配\n", (unsigned long long)address);
} else {
    printf("地址 0x%llx 未分配\n", (unsigned long long)address);
}
```

#### `CN_physical_total_pages`

```c
size_t CN_physical_total_pages(Stru_CN_PhysicalAllocator_t* allocator);
```

获取总页面数量。

**参数：**
- `allocator`：物理内存分配器句柄

**返回值：**
- 总页面数量，失败返回0

**示例：**
```c
size_t total = CN_physical_total_pages(allocator);
printf("总页面数量：%zu\n", total);
```

#### `CN_physical_get_free_pages`

```c
size_t CN_physical_get_free_pages(Stru_CN_PhysicalAllocator_t* allocator);
```

获取空闲页面数量。

**参数：**
- `allocator`：物理内存分配器句柄

**返回值：**
- 空闲页面数量，失败返回0

**示例：**
```c
size_t free_pages = CN_physical_get_free_pages(allocator);
printf("空闲页面数量：%zu\n", free_pages);
```

#### `CN_physical_used_pages`

```c
size_t CN_physical_used_pages(Stru_CN_PhysicalAllocator_t* allocator);
```

获取已使用页面数量。

**参数：**
- `allocator`：物理内存分配器句柄

**返回值：**
- 已使用页面数量，失败返回0

**示例：**
```c
size_t used_pages = CN_physical_used_pages(allocator);
printf("已使用页面数量：%zu\n", used_pages);
```

#### `CN_physical_largest_free_block`

```c
size_t CN_physical_largest_free_block(Stru_CN_PhysicalAllocator_t* allocator);
```

获取最大连续空闲页面数量。

**参数：**
- `allocator`：物理内存分配器句柄

**返回值：**
- 最大连续空闲页面数量，失败返回0

**注意：**
- 这个值反映了内存碎片化程度
- 值越小表示碎片化越严重

**示例：**
```c
size_t largest_block = CN_physical_largest_free_block(allocator);
printf("最大连续空闲块：%zu 页面\n", largest_block);
```

### 调试接口

#### `CN_physical_enable_debug`

```c
void CN_physical_enable_debug(Stru_CN_PhysicalAllocator_t* allocator, bool enable);
```

启用或禁用调试模式。

**参数：**
- `allocator`：物理内存分配器句柄
- `enable`：true启用调试，false禁用调试

**调试模式功能：**
- 输出详细的分配/释放信息
- 记录调试日志
- 启用调试回调

**示例：**
```c
// 启用调试模式
CN_physical_enable_debug(allocator, true);

// 现在所有操作都会输出调试信息
uintptr_t addr = CN_physical_alloc_pages(allocator, 2, __FILE__, __LINE__, "调试测试");

// 禁用调试模式
CN_physical_enable_debug(allocator, false);
```

#### `CN_physical_validate`

```c
bool CN_physical_validate(Stru_CN_PhysicalAllocator_t* allocator);
```

验证物理内存分配器完整性。

**参数：**
- `allocator`：物理内存分配器句柄

**返回值：**
- `true`：分配器完整
- `false`：分配器损坏或参数无效

**验证内容：**
1. 分配器状态验证
2. 位图一致性检查
3. 统计信息一致性检查
4. 内存范围验证

**示例：**
```c
if (!CN_physical_validate(allocator)) {
    fprintf(stderr, "物理内存分配器完整性验证失败\n");
    CN_physical_dump(allocator); // 转储状态以便调试
    return ERROR_CORRUPTED_ALLOCATOR;
}
```

#### `CN_physical_dump`

```c
void CN_physical_dump(Stru_CN_PhysicalAllocator_t* allocator);
```

转储物理内存分配器状态到标准输出。

**参数：**
- `allocator`：物理内存分配器句柄

**输出内容：**
- 分配器基本信息
- 内存范围信息
- 页面统计信息
- 操作统计信息
- 碎片化信息
- 调试和跟踪状态

**示例：**
```c
// 分配一些内存后查看状态
CN_physical_alloc_pages(allocator, 4, __FILE__, __LINE__, "测试1");
CN_physical_alloc_pages(allocator, 2, __FILE__, __LINE__, "测试2");

// 转储状态
CN_physical_dump(allocator);

// 输出示例：
// === 物理内存分配器状态：测试分配器 ===
// 内存范围: 0x100000 - 0x200000
// 页面大小: 4096 字节
// 总页面数: 64
// 空闲页面: 58
// 使用页面: 6
// 保留页面: 0
// 分配次数: 2
// 释放次数: 0
// 分配失败: 0
// 碎片化程度: 5%
// 最大连续空闲块: 58 页面
// 调试模式: 禁用
// 分配跟踪: 启用 (2/1024)
// ==========================================
```

#### `CN_physical_dump_leaks`

```c
void CN_physical_dump_leaks(Stru_CN_PhysicalAllocator_t* allocator);
```

转储所有未释放的分配信息（内存泄漏检测）。

**参数：**
- `allocator`：物理内存分配器句柄

**前提条件：**
- 分配器必须启用分配跟踪（`config.track_allocations = true`）

**输出内容：**
- 泄漏数量
- 每个泄漏的详细信息（地址、大小、位置、目的）

**示例：**
```c
// 启用分配跟踪
Stru_CN_PhysicalConfig_t config = CN_PHYSICAL_CONFIG_DEFAULT;
config.track_allocations = true;
Stru_CN_PhysicalAllocator_t* allocator = CN_physical_create(&config);

// 分配但不释放（模拟泄漏）
CN_physical_alloc_pages(allocator, 2, "test.c", 10, "泄漏测试1");
CN_physical_alloc_pages(allocator, 1, "test.c", 20, "泄漏测试2");

// 销毁分配器前检查泄漏
CN_physical_dump_leaks(allocator);

// 输出示例：
// === 检测到内存泄漏：2 个未释放分配 ===
// 泄漏 #1:
//   地址: 0x100000
//   页面数: 2
//   大小: 8192 字节
//   位置: test.c:10
//   目的: 泄漏测试1
// 
// 泄漏 #2:
//   地址: 0x100800
//   页面数: 1
//   大小: 4096 字节
//   位置: test.c:20
//   目的: 泄漏测试2
// ==========================================
```

#### `CN_physical_set_debug_callback`

```c
void CN_physical_set_debug_callback(Stru_CN_PhysicalAllocator_t* allocator, 
                                    CN_PhysicalDebugCallback_t callback, 
                                    void* user_data);
```

设置物理内存分配器调试回调函数。

**参数：**
- `allocator`：物理内存分配器句柄
- `callback`：调试回调函数
- `user_data`：用户数据（传递给回调函数）

**回调函数原型：**
```c
typedef void (*CN_PhysicalDebugCallback_t)(const char* message, void* user_data);
```

**示例：**
```c
// 定义调试回调函数
void my_debug_callback(const char* message, void* user_data) {
    FILE* log_file = (FILE*)user_data;
    fprintf(log_file, "[物理内存分配器] %s\n", message);
}

int main() {
    // 打开日志文件
    FILE* log_file = fopen("memory.log", "w");
    if (!log_file) {
        perror("打开日志文件失败");
        return EXIT_FAILURE;
    }
    
    // 创建分配器
    Stru_CN_PhysicalAllocator_t* allocator = CN_physical_create(&config);
    
    // 设置调试回调
    CN_physical_set_debug_callback(allocator, my_debug_callback, log_file);
    
    // 启用调试模式
    CN_physical_enable_debug(allocator, true);
    
    // 现在所有操作都会通过回调记录到文件
    CN_physical_alloc_pages(allocator, 4, __FILE__, __LINE__, "测试");
    
    // 清理
    CN_physical_destroy(allocator);
    fclose(log_file);
    
    return EXIT_SUCCESS;
}
```

### 辅助函数

#### `CN_physical_size_to_pages`

```c
static inline size_t CN_physical_size_to_pages(size_t size, size_t page_size)
```

将字节大小转换为页面数量（向上取整）。

**参数：**
- `size`：字节大小
- `page_size`：页面大小

**返回值：**
- 页面数量（向上取整）

**示例：**
```c
size_t page_size = 4096;
size_t buffer_size = 10000; // 需要10000字节

// 计算需要多少页面
size_t pages_needed = CN_physical_size_to_pages(buffer_size, page_size);
printf("需要 %zu 个页面（%zu 字节）\n", pages_needed, pages_needed * page_size);
// 输出：需要 3 个页面（12288 字节）
```

#### `CN_physical_pages_to_size`

```c
static inline size_t CN_physical_pages_to_size(size_t page_count, size_t page_size)
```

将页面数量转换为字节大小。

**参数：**
- `page_count`：页面数量
- `page_size`：页面大小

**返回值：**
- 字节大小

**示例：**
```c
size_t page_size = 4096;
size_t page_count = 5;

size_t total_size = CN_physical_pages_to_size(page_count, page_size);
printf("%zu 个页面 = %zu 字节\n", page_count, total_size);
// 输出：5 个页面 = 20480 字节
```

#### `CN_physical_align_to_page`

```c
static inline uintptr_t CN_physical_align_to_page(uintptr_t address, size_t page_size)
```

对齐地址到页面边界（向上取整）。

**参数：**
- `address`：地址
- `page_size`：页面大小

**返回值：**
- 对齐后的地址

**示例：**
```c
uintptr_t address = 0x100123;
size_t page_size = 4096;

uintptr_t aligned = CN_physical_align_to_page(address, page_size);
printf("地址 0x%llx 对齐到 0x%llx\n", 
       (unsigned long long)address, 
       (unsigned long long)aligned);
// 输出：地址 0x100123 对齐到 0x101000
```

#### `CN_physical_is_page_aligned`

```c
static inline bool CN_physical_is_page_aligned(uintptr_t address, size_t page_size)
```

检查地址是否页面对齐。

**参数：**
- `address`：地址
- `page_size`：页面大小

**返回值：**
- `true`：地址页面对齐
- `false`：地址未对齐

**示例：**
```c
uintptr_t address1 = 0x100000; // 1MB，4KB对齐
uintptr_t address2 = 0x100123; // 不对齐
size_t page_size = 4096;

bool aligned1 = CN_physical_is_page_aligned(address1, page_size);
bool aligned2 = CN_physical_is_page_aligned(address2, page_size);

printf("地址1 对齐: %s\n", aligned1 ? "是" : "否"); // 是
printf("地址2 对齐: %s\n", aligned2 ? "是" : "否"); // 否
```

## 使用模式

### 基本使用模式

```c
#include "CN_physical_allocator.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    // 1. 配置物理内存分配器
    Stru_CN_PhysicalConfig_t config = CN_PHYSICAL_CONFIG_DEFAULT;
    config.memory_start = 0x100000;  // 1MB
    config.memory_end = 0x200000;    // 2MB (1MB内存空间)
    config.page_size = CN_PAGE_SIZE_4KB;
    config.name = "示例分配器";
    
    // 2. 创建分配器
    Stru_CN_PhysicalAllocator_t* allocator = CN_physical_create(&config);
    if (allocator == NULL) {
        fprintf(stderr, "创建分配器失败\n");
        return EXIT_FAILURE;
    }
    
    // 3. 分配内存
    uintptr_t addr1 = CN_physical_alloc_pages(allocator, 2, __FILE__, __LINE__, "缓冲区1");
    uintptr_t addr2 = CN_physical_alloc_pages(allocator, 1, __FILE__, __LINE__, "缓冲区2");
    
    if (addr1 == 0 || addr2 == 0) {
        fprintf(stderr, "内存分配失败\n");
        CN_physical_destroy(allocator);
        return EXIT_FAILURE;
    }
    
    printf("分配成功：\n");
    printf("  缓冲区1: 0x%llx (2页面)\n", (unsigned long long)addr1);
    printf("  缓冲区2: 0x%llx (1页面)\n", (unsigned long long)addr2);
    
    // 4. 查询统计信息
    Stru_CN_PhysicalStats_t stats;
    if (CN_physical_get_stats(allocator, &stats)) {
        printf("\n统计信息：\n");
        printf("  总页面：%zu\n", stats.total_pages);
        printf("  空闲页面：%zu\n", stats.free_pages);
        printf("  使用页面：%zu\n", stats.used_pages);
        printf("  碎片化：%zu%%\n", stats.fragmentation);
    }
    
    // 5. 释放内存
    CN_physical_free_pages(allocator, addr1, 2, __FILE__, __LINE__);
    CN_physical_free_pages(allocator, addr2, 1, __FILE__, __LINE__);
    
    // 6. 销毁分配器
    CN_physical_destroy(allocator);
    
    return EXIT_SUCCESS;
}
```

### 对齐分配模式（用于DMA等）

```c
#include "CN_physical_allocator.h"

void dma_example(void) {
    Stru_CN_PhysicalConfig_t config = CN_PHYSICAL_CONFIG_DEFAULT;
    config.memory_start = 0;
    config.memory_end = 16 * 1024 * 1024; // 16MB
    config.page_size = CN_PAGE_SIZE_4KB;
    
    Stru_CN_PhysicalAllocator_t* allocator = CN_physical_create(&config);
    if (!allocator) return;
    
    // DMA需要64KB对齐的缓冲区
    uintptr_t dma_buffer = CN_physical_alloc_pages_aligned(
        allocator,
        16,                    // 16个页面 = 64KB
        64 * 1024,             // 64KB对齐
        __FILE__, __LINE__,
        "DMA传输缓冲区"
    );
    
    if (dma_buffer != 0) {
        // 验证对齐
        assert((dma_buffer & (64 * 1024 - 1)) == 0);
        
        // 使用DMA缓冲区...
        printf("DMA缓冲区地址: 0x%llx\n", (unsigned long long)dma_buffer);
        
        // 释放
        CN_physical_free_pages(allocator, dma_buffer, 16, __FILE__, __LINE__);
    }
    
    CN_physical_destroy(allocator);
}
```

### 调试和跟踪模式

```c
#include "CN_physical_allocator.h"

void debug_example(void) {
    // 配置启用调试和跟踪
    Stru_CN_PhysicalConfig_t config = CN_PHYSICAL_CONFIG_DEFAULT;
    config.memory_start = 0x100000;
    config.memory_end = 0x200000;
    config.enable_debug = true;
    config.track_allocations = true;
    config.name = "调试分配器";
    
    Stru_CN_PhysicalAllocator_t* allocator = CN_physical_create(&config);
    if (!allocator) return;
    
    // 设置调试回调
    void debug_callback(const char* msg, void* user_data) {
        printf("[DEBUG] %s\n", msg);
    }
    CN_physical_set_debug_callback(allocator, debug_callback, NULL);
    
    // 分配内存（会触发调试输出）
    uintptr_t addr1 = CN_physical_alloc_pages(allocator, 2, "app.c", 100, "临时数据");
    uintptr_t addr2 = CN_physical_alloc_pages(allocator, 1, "app.c", 105, "配置数据");
    
    // 故意不释放addr2，模拟泄漏
    
    // 检查泄漏
    printf("\n=== 泄漏检查 ===\n");
    CN_physical_dump_leaks(allocator);
    
    // 转储完整状态
    printf("\n=== 分配器状态 ===\n");
    CN_physical_dump(allocator);
    
    // 验证完整性
    if (!CN_physical_validate(allocator)) {
        printf("警告：分配器完整性验证失败\n");
    }
    
    // 清理（会输出泄漏警告）
    CN_physical_destroy(allocator);
}
```

### 特定硬件地址分配模式

```c
#include "CN_physical_allocator.h"

void hardware_mapping_example(void) {
    // 假设硬件设备寄存器在0xF0000000
    Stru_CN_PhysicalConfig_t config = CN_PHYSICAL_CONFIG_DEFAULT;
    config.memory_start = 0xF0000000;
    config.memory_end = 0xF0100000; // 1MB硬件地址空间
    config.page_size = CN_PAGE_SIZE_4KB;
    config.name = "硬件映射分配器";
    
    Stru_CN_PhysicalAllocator_t* allocator = CN_physical_create(&config);
    if (!allocator) return;
    
    // 分配特定硬件地址（设备寄存器区域）
    uintptr_t hw_registers = CN_physical_alloc_pages_at(
        allocator,
        0xF0000000,              // 硬件寄存器地址
        4,                       // 4个页面 = 16KB
        __FILE__, __LINE__,
        "硬件设备寄存器"
    );
    
    if (hw_registers != 0) {
        printf("硬件寄存器映射成功：地址=0x%llx\n", (unsigned long long)hw_registers);
        
        // 注意：在实际系统中，这里需要将物理地址映射到虚拟地址才能访问
        // void* virtual_addr = map_physical_to_virtual(hw_registers, 4 * 4096);
        
        // 使用硬件寄存器...
        
        // 释放（在实际系统中可能不需要释放硬件映射）
        // CN_physical_free_pages(allocator, hw_registers, 4, __FILE__, __LINE__);
    }
    
    CN_physical_destroy(allocator);
}
```

## 错误处理

### 常见错误及处理

#### 内存不足错误

```c
uintptr_t address = CN_physical_alloc_pages(allocator, large_page_count, 
                                           __FILE__, __LINE__, "大缓冲区");
if (address == 0) {
    // 检查具体原因
    Stru_CN_PhysicalStats_t stats;
    if (CN_physical_get_stats(allocator, &stats)) {
        if (large_page_count > stats.free_pages) {
            fprintf(stderr, "错误：请求 %zu 页面，但只有 %zu 空闲页面\n",
                    large_page_count, stats.free_pages);
        } else {
            // 可能是碎片化问题
            size_t largest = CN_physical_largest_free_block(allocator);
            if (largest < large_page_count) {
                fprintf(stderr, "错误：内存碎片化，最大连续块只有 %zu 页面\n", largest);
            }
        }
    }
    
    // 处理策略：尝试分配较小的块或整理内存
    return ERROR_OUT_OF_MEMORY;
}
```

#### 参数错误

```c
// 检查地址是否页面对齐
if (!CN_physical_is_page_aligned(address, allocator->config.page_size)) {
    fprintf(stderr, "错误：地址 0x%llx 不是页面对齐的（页面大小：%zu）\n",
            (unsigned long long)address, allocator->config.page_size);
    
    // 自动对齐
    uintptr_t aligned_address = CN_physical_align_to_page(address, 
                                                         allocator->config.page_size);
    fprintf(stderr, "建议使用对齐地址：0x%llx\n", (unsigned long long)aligned_address);
    
    return ERROR_INVALID_PARAMETER;
}
```

#### 分配器状态错误

```c
// 在关键操作前验证分配器状态
if (!CN_physical_validate(allocator)) {
    fprintf(stderr, "错误：物理内存分配器状态异常\n");
    
    // 尝试恢复或安全退出
    CN_physical_dump(allocator);  // 输出状态以便调试
    
    // 根据应用场景决定：继续运行、重启分配器或退出
    return ERROR_CORRUPTED_ALLOCATOR;
}
```

## 性能优化建议

### 1. 选择合适的页面大小

```c
// 根据应用需求选择页面大小
Stru_CN_PhysicalConfig_t config = CN_PHYSICAL_CONFIG_DEFAULT;

// 场景1：通用内存分配
config.page_size = CN_PAGE_SIZE_4KB;  // 标准页面

// 场景2：大内存分配（数据库缓冲区等）
config.page_size = CN_PAGE_SIZE_2MB;  // 大页面，减少TLB压力

// 场景3：巨型内存分配（科学计算等）
config.page_size = CN_PAGE_SIZE_1GB;  // 巨大页面
```

### 2. 批量分配减少碎片

```c
// 不好的做法：频繁分配小内存块
for (int i = 0; i < 100; i++) {
    uintptr_t addr = CN_physical_alloc_pages(allocator, 1, __FILE__, __LINE__, "小块");
    // 使用...
    CN_physical_free_pages(allocator, addr, 1, __FILE__, __LINE__);
}

// 好的做法：批量分配
uintptr_t large_block = CN_physical_alloc_pages(allocator, 100, __FILE__, __LINE__, "批量分配");
if (large_block != 0) {
    // 从大块中分配小内存（需要应用层管理）
    for (int i = 0; i < 100; i++) {
        uintptr_t small_addr = large_block + i * 4096;
        // 使用小内存...
    }
    CN_physical_free_pages(allocator, large_block, 100, __FILE__, __LINE__);
}
```

### 3. 使用对齐分配优化性能

```c
// 对于需要特定对齐的硬件操作，使用对齐分配
uintptr_t aligned_buffer = CN_physical_alloc_pages_aligned(
    allocator,
    page_count,
    required_alignment,  // 如64KB、1MB等
    __FILE__, __LINE__,
    "性能关键缓冲区"
);

// 对齐分配可以减少内存访问延迟
// 特别是对于DMA、GPU操作等
```

## 内存安全最佳实践

### 1. 启用调试和跟踪

```c
// 在开发阶段启用完整调试
Stru_CN_PhysicalConfig_t dev_config = CN_PHYSICAL_CONFIG_DEFAULT;
dev_config.enable_debug = true;
dev_config.track_allocations = true;
dev_config.name = "开发分配器";

// 在生产阶段可以关闭调试以减少开销
Stru_CN_PhysicalConfig_t prod_config = CN_PHYSICAL_CONFIG_DEFAULT;
prod_config.enable_debug = false;
prod_config.track_allocations = false;  // 除非需要泄漏检测
prod_config.name = "生产分配器";
```

### 2. 定期验证完整性

```c
// 在关键点验证分配器完整性
void critical_operation(Stru_CN_PhysicalAllocator_t* allocator) {
    // 操作前验证
    if (!CN_physical_validate(allocator)) {
        handle_corruption(allocator);
        return;
    }
    
    // 执行操作...
    
    // 操作后验证
    if (!CN_physical_validate(allocator)) {
        // 操作导致分配器损坏
        log_error("操作导致分配器损坏");
        recover_or_restart(allocator);
    }
}
```

### 3. 内存泄漏检测

```c
// 程序退出前检查泄漏
void cleanup(Stru_CN_PhysicalAllocator_t* allocator) {
    // 检查泄漏
    if (allocator->config.track_allocations) {
        size_t leaks = allocator->current_allocations;
        if (leaks > 0) {
            fprintf(stderr, "警告：检测到 %zu 个内存泄漏\n", leaks);
            CN_physical_dump_leaks(allocator);
        }
    }
    
    // 销毁分配器
    CN_physical_destroy(allocator);
}
```

## 平台特定注意事项

### Windows平台

```c
// Windows可能需要特殊处理
#ifdef _WIN32
    // Windows使用不同的内存管理API
    // 物理内存分配器在Windows上通常用于模拟或测试
    
    // 注意：Windows虚拟地址空间布局
    config.memory_start = 0x100000;  // 1MB，避开低端内存
    config.memory_end = 0x7FFFFFFF;  // 用户空间上限（32位）
#endif
```

### Linux平台

```c
// Linux平台注意事项
#ifdef __linux__
    // Linux有更灵活的内存管理
    // 物理内存分配器可以模拟内核行为
    
    // 注意页面大小可能因架构而异
    // x86: 通常4KB，但支持大页面
    // ARM: 可能4KB或64KB
    config.page_size = sysconf(_SC_PAGESIZE);  // 获取系统页面大小
#endif
```

### 嵌入式平台

```c
// 嵌入式系统注意事项
#ifdef __EMBEDDED__
    // 内存有限，需要精确配置
    config.memory_start = 0;  // 从起始地址开始
    config.memory_end = 0x100000;  // 1MB内存
    
    // 可能使用较小的页面大小
    config.page_size = 1024;  // 1KB页面，减少内部碎片
    
    // 禁用调试以减少开销
    config.enable_debug = false;
    config.track_allocations = false;
#endif
```

## 测试策略

### 单元测试示例

```c
#include "CN_physical_allocator.h"
#include <assert.h>

void test_basic_allocation(void) {
    Stru_CN_PhysicalConfig_t config = CN_PHYSICAL_CONFIG_DEFAULT;
    config.memory_start = 0x100000;
    config.memory_end = 0x110000;  // 64KB内存
    config.page_size = 4096;
    
    Stru_CN_PhysicalAllocator_t* allocator = CN_physical_create(&config);
    assert(allocator != NULL);
    
    // 测试1：基本分配
    uintptr_t addr = CN_physical_alloc_pages(allocator, 2, __FILE__, __LINE__, "测试");
    assert(addr != 0);
    assert(CN_physical_is_allocated(allocator, addr));
    
    // 测试2：释放
    CN_physical_free_pages(allocator, addr, 2, __FILE__, __LINE__);
    assert(!CN_physical_is_allocated(allocator, addr));
    
    // 测试3：统计信息
    Stru_CN_PhysicalStats_t stats;
    assert(CN_physical_get_stats(allocator, &stats));
    assert(stats.used_pages == 0);
    assert(stats.free_pages == stats.total_pages);
    
    CN_physical_destroy(allocator);
    printf("基本分配测试通过\n");
}

void test_aligned_allocation(void) {
    // 测试对齐分配...
}

void test_specific_address_allocation(void) {
    // 测试特定地址分配...
}

int main(void) {
    test_basic_allocation();
    test_aligned_allocation();
    test_specific_address_allocation();
    
    printf("所有测试通过\n");
    return 0;
}
```

## 版本兼容性

### 向后兼容性保证

1. **API稳定性**：公共API一旦发布，保持向后兼容
2. **数据结构**：核心数据结构布局不变
3. **行为一致性**：函数行为保持一致

### 升级指南

从旧版本升级时：
1. 检查配置结构是否有新字段
2. 验证统计信息结构兼容性
3. 测试现有代码与新版本的兼容性

## 相关文档

- [物理内存分配器README](../../../src/infrastructure/memory/physical/README.md)
- [内存管理框架API](../CN_memory_API.md)
- [架构设计原则](../../../architecture/架构设计原则.md)
- [编码规范](../../../specifications/CN_Language项目%20技术规范和编码标准.md)

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-03 | 初始版本，物理内存页框分配器 |
| 1.0.1 | 计划 | 性能优化和错误修复 |
| 1.1.0 | 计划 | 添加NUMA支持和高级碎片整理 |

## 作者

CN_Language开发团队

## 最后更新

2026年1月3日
