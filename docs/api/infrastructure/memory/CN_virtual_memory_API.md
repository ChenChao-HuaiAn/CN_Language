# CN_Language 虚拟内存管理API文档

## 概述

本文档描述了CN_Language项目中虚拟内存管理模块的公共API。虚拟内存管理模块提供了分页机制和MMU（内存管理单元）抽象，支持虚拟地址空间管理、页表管理、页面保护等功能。

## 头文件

```c
#include "src/infrastructure/memory/virtual/CN_virtual_memory.h"
```

## 数据类型

### 配置结构体

#### Stru_CN_VirtualMemoryConfig_t

虚拟内存管理器配置选项。

```c
typedef struct Stru_CN_VirtualMemoryConfig_t
{
    uintptr_t virtual_address_start;   /**< 虚拟地址空间起始地址 */
    uintptr_t virtual_address_end;     /**< 虚拟地址空间结束地址 */
    size_t page_size;                  /**< 页面大小（字节），默认4096 */
    size_t page_table_levels;          /**< 页表级数（2-4级） */
    bool enable_huge_pages;            /**< 是否启用大页面支持 */
    bool enable_statistics;            /**< 是否启用统计信息 */
    bool enable_debug;                 /**< 是否启用调试模式 */
    bool enable_page_protection;       /**< 是否启用页面保护 */
    const char* name;                  /**< 虚拟内存管理器名称（用于调试） */
} Stru_CN_VirtualMemoryConfig_t;
```

**默认配置宏：**
```c
#define CN_VIRTUAL_MEMORY_CONFIG_DEFAULT \
    { \
        .virtual_address_start = 0x10000000,   /* 256MB处开始 */ \
        .virtual_address_end = 0x80000000,     /* 2GB处结束 */ \
        .page_size = 4096,                     /* 4KB页面 */ \
        .page_table_levels = 2,                /* 2级页表 */ \
        .enable_huge_pages = false,            /* 禁用大页面 */ \
        .enable_statistics = true,             /* 启用统计 */ \
        .enable_debug = false,                 /* 禁用调试 */ \
        .enable_page_protection = true,        /* 启用页面保护 */ \
        .name = "虚拟内存管理器"               /* 默认名称 */ \
    }
```

### 页面大小常量

```c
#define CN_VIRTUAL_PAGE_SIZE_4KB    4096UL     /**< 标准页面大小（4KB） */
#define CN_VIRTUAL_PAGE_SIZE_2MB    (2UL * 1024UL * 1024UL)  /**< 大页面大小（2MB） */
#define CN_VIRTUAL_PAGE_SIZE_1GB    (1024UL * 1024UL * 1024UL)  /**< 巨大页面大小（1GB） */
```

### 页面保护标志

#### Eum_CN_PageProtection_t

页面保护标志位定义。

```c
typedef enum Eum_CN_PageProtection_t
{
    Eum_PAGE_PROTECTION_NONE      = 0x00,   /**< 无保护（不可访问） */
    Eum_PAGE_PROTECTION_READ      = 0x01,   /**< 可读 */
    Eum_PAGE_PROTECTION_WRITE     = 0x02,   /**< 可写 */
    Eum_PAGE_PROTECTION_EXECUTE   = 0x04,   /**< 可执行 */
    Eum_PAGE_PROTECTION_USER      = 0x08,   /**< 用户模式可访问 */
    Eum_PAGE_PROTECTION_KERNEL    = 0x10,   /**< 内核模式可访问 */
    Eum_PAGE_PROTECTION_GUARD     = 0x20,   /**< 保护页（访问时触发异常） */
    Eum_PAGE_PROTECTION_NO_CACHE  = 0x40,   /**< 禁用缓存 */
    Eum_PAGE_PROTECTION_WRITE_THROUGH = 0x80, /**< 直写缓存 */
    
    /** @brief 标准组合标志 */
    Eum_PAGE_PROTECTION_READ_ONLY = Eum_PAGE_PROTECTION_READ,
    Eum_PAGE_PROTECTION_READ_WRITE = Eum_PAGE_PROTECTION_READ | Eum_PAGE_PROTECTION_WRITE,
    Eum_PAGE_PROTECTION_READ_EXECUTE = Eum_PAGE_PROTECTION_READ | Eum_PAGE_PROTECTION_EXECUTE,
    Eum_PAGE_PROTECTION_READ_WRITE_EXECUTE = Eum_PAGE_PROTECTION_READ | Eum_PAGE_PROTECTION_WRITE | Eum_PAGE_PROTECTION_EXECUTE,
    Eum_PAGE_PROTECTION_KERNEL_READ_WRITE = Eum_PAGE_PROTECTION_READ | Eum_PAGE_PROTECTION_WRITE | Eum_PAGE_PROTECTION_KERNEL,
    Eum_PAGE_PROTECTION_USER_READ_WRITE = Eum_PAGE_PROTECTION_READ | Eum_PAGE_PROTECTION_WRITE | Eum_PAGE_PROTECTION_USER,
} Eum_CN_PageProtection_t;
```

### 页面映射标志

#### Eum_CN_PageMappingFlags_t

页面映射标志位定义。

```c
typedef enum Eum_CN_PageMappingFlags_t
{
    Eum_PAGE_MAPPING_FIXED        = 0x01,   /**< 固定映射（指定虚拟地址） */
    Eum_PAGE_MAPPING_COMMIT       = 0x02,   /**< 立即提交物理内存 */
    Eum_PAGE_MAPPING_RESERVE      = 0x04,   /**< 仅保留虚拟地址空间 */
    Eum_PAGE_MAPPING_SHARED       = 0x08,   /**< 共享内存映射 */
    Eum_PAGE_MAPPING_COPY_ON_WRITE = 0x10,  /**< 写时复制 */
    Eum_PAGE_MAPPING_LAZY         = 0x20,   /**< 延迟分配（按需分页） */
    Eum_PAGE_MAPPING_GUARD        = 0x40,   /**< 保护页映射 */
    Eum_PAGE_MAPPING_NO_ACCESS    = 0x80,   /**< 禁止访问 */
} Eum_CN_PageMappingFlags_t;
```

### MMU抽象接口

#### Stru_CN_MMUInterface_t

MMU（内存管理单元）抽象接口。

```c
typedef struct Stru_CN_MMUInterface_t
{
    bool (*initialize)(const Stru_CN_VirtualMemoryConfig_t* config);
    bool (*enable)(void);
    void (*disable)(void);
    void (*flush_tlb)(uintptr_t virtual_address);
    void (*set_page_table_base)(uintptr_t page_table_base);
    uintptr_t (*get_page_table_base)(void);
    void (*get_architecture_info)(char* info_buffer, size_t buffer_size);
} Stru_CN_MMUInterface_t;
```

### 统计信息结构体

#### Stru_CN_VirtualMemoryStats_t

虚拟内存管理器统计信息。

```c
typedef struct Stru_CN_VirtualMemoryStats_t
{
    size_t total_virtual_pages;          /**< 总虚拟页面数量 */
    size_t mapped_pages;                 /**< 已映射页面数量 */
    size_t reserved_pages;               /**< 保留页面数量 */
    size_t committed_pages;              /**< 已提交页面数量 */
    size_t page_faults;                  /**< 页面错误次数 */
    size_t tlb_misses;                   /**< TLB未命中次数 */
    size_t tlb_flushes;                  /**< TLB刷新次数 */
    size_t page_table_allocations;       /**< 页表分配次数 */
    size_t page_table_frees;             /**< 页表释放次数 */
    size_t protection_violations;        /**< 保护违规次数 */
    size_t shared_mappings;              /**< 共享映射数量 */
    size_t private_mappings;             /**< 私有映射数量 */
} Stru_CN_VirtualMemoryStats_t;
```

### 页面错误信息结构体

#### Stru_CN_PageFaultInfo_t

页面错误信息。

```c
typedef struct Stru_CN_PageFaultInfo_t
{
    uintptr_t virtual_address;           /**< 引发错误的虚拟地址 */
    uintptr_t instruction_pointer;       /**< 引发错误的指令指针 */
    bool is_write;                       /**< 是否是写操作 */
    bool is_user_mode;                   /**< 是否是用户模式 */
    bool is_present;                     /**< 页面是否存在 */
    bool is_protection_violation;        /**< 是否是保护违规 */
    uint32_t error_code;                 /**< 错误代码（架构相关） */
} Stru_CN_PageFaultInfo_t;
```

### 虚拟内存管理器句柄

#### Stru_CN_VirtualMemoryManager_t

虚拟内存管理器句柄（不透明指针）。

```c
typedef struct Stru_CN_VirtualMemoryManager_t Stru_CN_VirtualMemoryManager_t;
```

## API函数

### 虚拟内存管理器管理接口

#### CN_virtual_memory_create

创建虚拟内存管理器。

```c
Stru_CN_VirtualMemoryManager_t* CN_virtual_memory_create(
    const Stru_CN_VirtualMemoryConfig_t* config,
    const Stru_CN_MMUInterface_t* mmu_interface);
```

**参数：**
- `config`：虚拟内存管理器配置（可为NULL，使用默认配置）
- `mmu_interface`：MMU接口（可为NULL，使用默认实现）

**返回值：**
- 成功：虚拟内存管理器句柄
- 失败：NULL

**示例：**
```c
Stru_CN_VirtualMemoryConfig_t config = CN_VIRTUAL_MEMORY_CONFIG_DEFAULT;
Stru_CN_VirtualMemoryManager_t* manager = CN_virtual_memory_create(&config, NULL);
```

#### CN_virtual_memory_destroy

销毁虚拟内存管理器。

```c
void CN_virtual_memory_destroy(Stru_CN_VirtualMemoryManager_t* manager);
```

**参数：**
- `manager`：虚拟内存管理器句柄

**注意：** 会释放所有分配的资源，包括虚拟地址空间和物理内存。

### 虚拟地址空间管理接口

#### CN_virtual_memory_alloc

分配虚拟地址空间。

```c
uintptr_t CN_virtual_memory_alloc(Stru_CN_VirtualMemoryManager_t* manager,
                                  size_t size,
                                  size_t alignment,
                                  Eum_CN_PageMappingFlags_t flags,
                                  Eum_CN_PageProtection_t protection);
```

**参数：**
- `manager`：虚拟内存管理器句柄
- `size`：要分配的大小（字节）
- `alignment`：对齐要求（必须是页面大小的倍数，0表示使用默认页面大小）
- `flags`：映射标志
- `protection`：页面保护标志

**返回值：**
- 成功：分配的虚拟地址
- 失败：0

**示例：**
```c
uintptr_t addr = CN_virtual_memory_alloc(manager, 4096, 0, 
                                         Eum_PAGE_MAPPING_COMMIT,
                                         Eum_PAGE_PROTECTION_READ_WRITE);
```

#### CN_virtual_memory_alloc_at

在指定地址分配虚拟地址空间。

```c
uintptr_t CN_virtual_memory_alloc_at(Stru_CN_VirtualMemoryManager_t* manager,
                                     uintptr_t virtual_address,
                                     size_t size,
                                     Eum_CN_PageMappingFlags_t flags,
                                     Eum_CN_PageProtection_t protection);
```

**参数：**
- `manager`：虚拟内存管理器句柄
- `virtual_address`：请求的虚拟地址（必须是页面对齐的）
- `size`：要分配的大小（字节）
- `flags`：映射标志
- `protection`：页面保护标志

**返回值：**
- 成功：分配的虚拟地址
- 失败：0

#### CN_virtual_memory_free

释放虚拟地址空间。

```c
void CN_virtual_memory_free(Stru_CN_VirtualMemoryManager_t* manager,
                            uintptr_t virtual_address,
                            size_t size);
```

**参数：**
- `manager`：虚拟内存管理器句柄
- `virtual_address`：要释放的虚拟地址
- `size`：要释放的大小（字节）

#### CN_virtual_memory_reserve

保留虚拟地址空间（不分配物理内存）。

```c
uintptr_t CN_virtual_memory_reserve(Stru_CN_VirtualMemoryManager_t* manager,
                                    size_t size,
                                    size_t alignment);
```

**参数：**
- `manager`：虚拟内存管理器句柄
- `size`：要保留的大小（字节）
- `alignment`：对齐要求

**返回值：**
- 成功：保留的虚拟地址
- 失败：0

#### CN_virtual_memory_commit

提交物理内存到虚拟地址空间。

```c
bool CN_virtual_memory_commit(Stru_CN_VirtualMemoryManager_t* manager,
                              uintptr_t virtual_address,
                              size_t size,
                              Eum_CN_PageProtection_t protection);
```

**参数：**
- `manager`：虚拟内存管理器句柄
- `virtual_address`：虚拟地址
- `size`：要提交的大小（字节）
- `protection`：页面保护标志

**返回值：**
- 成功：true
- 失败：false

#### CN_virtual_memory_decommit

取消提交物理内存（保留虚拟地址空间）。

```c
void CN_virtual_memory_decommit(Stru_CN_VirtualMemoryManager_t* manager,
                                uintptr_t virtual_address,
                                size_t size);
```

**参数：**
- `manager`：虚拟内存管理器句柄
- `virtual_address`：虚拟地址
- `size`：要取消提交的大小（字节）

### 页面映射接口

#### CN_virtual_memory_map_physical

映射物理内存到虚拟地址空间。

```c
bool CN_virtual_memory_map_physical(Stru_CN_VirtualMemoryManager_t* manager,
                                    uintptr_t virtual_address,
                                    uintptr_t physical_address,
                                    size_t size,
                                    Eum_CN_PageProtection_t protection,
                                    Eum_CN_PageMappingFlags_t flags);
```

**参数：**
- `manager`：虚拟内存管理器句柄
- `virtual_address`：虚拟地址
- `physical_address`：物理地址
- `size`：映射大小（字节）
- `protection`：页面保护标志
- `flags`：映射标志

**返回值：**
- 成功：true
- 失败：false

**示例：**
```c
bool success = CN_virtual_memory_map_physical(manager, 0x10000000, 0x20000000,
                                              4096, Eum_PAGE_PROTECTION_READ_WRITE,
                                              Eum_PAGE_MAPPING_FIXED);
```

#### CN_virtual_memory_unmap

取消映射虚拟地址空间。

```c
void CN_virtual_memory_unmap(Stru_CN_VirtualMemoryManager_t* manager,
                             uintptr_t virtual_address,
                             size_t size);
```

**参数：**
- `manager`：虚拟内存管理器句柄
- `virtual_address`：虚拟地址
- `size`：取消映射的大小（字节）

#### CN_virtual_memory_remap

重新映射虚拟地址空间（更改保护标志）。

```c
bool CN_virtual_memory_remap(Stru_CN_VirtualMemoryManager_t* manager,
                             uintptr_t virtual_address,
                             size_t size,
                             Eum_CN_PageProtection_t new_protection);
```

**参数：**
- `manager`：虚拟内存管理器句柄
- `virtual_address`：虚拟地址
- `size`：重新映射的大小（字节）
- `new_protection`：新的页面保护标志

**返回值：**
- 成功：true
- 失败：false

#### CN_virtual_memory_query

查询虚拟地址的映射信息。

```c
bool CN_virtual_memory_query(Stru_CN_VirtualMemoryManager_t* manager,
                             uintptr_t virtual_address,
                             uintptr_t* physical_address,
                             Eum_CN_PageProtection_t* protection,
                             Eum_CN_PageMappingFlags_t* flags);
```

**参数：**
- `manager`：虚拟内存管理器句柄
- `virtual_address`：虚拟地址
- `physical_address`：输出物理地址（可为NULL）
- `protection`：输出保护标志（可为NULL）
- `flags`：输出映射标志（可为NULL）

**返回值：**
- 如果地址已映射：true
- 如果地址未映射：false

### 页表管理接口

#### CN_virtual_memory_get_page_table_base

获取页表基地址。

```c
uintptr_t CN_virtual_memory_get_page_table_base(Stru_CN_VirtualMemoryManager_t* manager);
```

**参数：**
- `manager`：虚拟内存管理器句柄

**返回值：**
- 页表基地址

#### CN_virtual_memory_set_page_table_base

设置页表基地址。

```c
void CN_virtual_memory_set_page_table_base(Stru_CN_VirtualMemoryManager_t* manager,
                                           uintptr_t page_table_base);
```

**参数：**
- `manager`：虚拟内存管理器句柄
- `page_table_base`：页表基地址

#### CN_virtual_memory_flush_tlb

刷新TLB（转换后备缓冲区）。

```c
void CN_virtual_memory_flush_tlb(Stru_CN_VirtualMemoryManager_t* manager,
                                 uintptr_t virtual_address);
```

**参数：**
- `manager`：虚拟内存管理器句柄
- `virtual_address`：虚拟地址（0表示刷新全部）

### 统计和调试接口

#### CN_virtual_memory_get_stats

获取虚拟内存管理器统计信息。

```c
bool CN_virtual_memory_get_stats(Stru_CN_VirtualMemoryManager_t* manager,
                                 Stru_CN_VirtualMemoryStats_t* stats);
```

**参数：**
- `manager`：虚拟内存管理器句柄
- `stats`：输出统计信息

**返回值：**
- 成功：true
- 失败：false

#### CN_virtual_memory_reset_stats

重置虚拟内存管理器统计信息。

```c
void CN_virtual_memory_reset_stats(Stru_CN_VirtualMemoryManager_t* manager);
```

**参数：**
- `manager`：虚拟内存管理器句柄

#### CN_virtual_memory_enable_debug

启用/禁用调试模式。

```c
void CN_virtual_memory_enable_debug(Stru_CN_VirtualMemoryManager_t* manager, bool enable);
```

**参数：**
- `manager`：虚拟内存管理器句柄
- `enable`：是否启用调试

#### CN_virtual_memory_validate

验证虚拟内存管理器完整性。

```c
bool CN_virtual_memory_validate(Stru_CN_VirtualMemoryManager_t* manager);
```

**参数：**
- `manager`：虚拟内存管理器句柄

**返回值：**
- 如果管理器完整：true
- 如果管理器不完整：false

#### CN_virtual_memory_dump

转储虚拟内存管理器状态到标准输出。

```c
void CN_virtual_memory_dump(Stru_CN_VirtualMemoryManager_t* manager);
```

**参数：**
- `manager`：虚拟内存管理器句柄

#### CN_virtual_memory_dump_leaks

转储所有未释放的映射信息。

```c
void CN_virtual_memory_dump_leaks(Stru_CN_VirtualMemoryManager_t* manager);
```

**参数：**
- `manager`：虚拟内存管理器句柄

#### CN_virtual_memory_handle_page_fault

处理页面错误。

```c
bool CN_virtual_memory_handle_page_fault(Stru_CN_VirtualMemoryManager_t* manager,
                                         const Stru_CN_PageFaultInfo_t* fault_info);
```

**参数：**
- `manager`：虚拟内存管理器句柄
- `fault_info`：页面错误信息

**返回值：**
- 处理成功：true
- 处理失败：false

### 辅助函数

#### CN_virtual_memory_size_to_pages

将字节大小转换为页面数量。

```c
static inline size_t CN_virtual_memory_size_to_pages(size_t size, size_t page_size)
{
    return (size + page_size - 1) / page_size;
}
```

**参数：**
- `size`：字节大小
- `page_size`：页面大小

**返回值：**
- 页面数量（向上取整）

#### CN_virtual_memory_pages_to_size

将页面数量转换为字节大小。

```c
static inline size_t CN_virtual_memory_pages_to_size(size_t page_count, size_t page_size)
{
    return page_count * page_size;
}
```

**参数：**
- `page_count`：页面数量
- `page_size`：页面大小

**返回值：**
- 字节大小

#### CN_virtual_memory_align_to_page

对齐地址到页面边界。

```c
static inline uintptr_t CN_virtual_memory_align_to_page(uintptr_t address, size_t page_size)
{
    return (address + page_size - 1) & ~(page_size - 1);
}
```

**参数：**
- `address`：地址
- `page_size`：页面大小

**返回值：**
- 对齐后的地址

#### CN_virtual_memory_is_page_aligned

检查地址是否页面对齐。

```c
static inline bool CN_virtual_memory_is_page_aligned(uintptr_t address, size_t page_size)
{
    return (address & (page_size - 1)) == 0;
}
```

**参数：**
- `address`：地址
- `page_size`：页面大小

**返回值：**
- 如果对齐：true
- 如果未对齐：false

## 错误处理

虚拟内存管理API使用以下错误处理机制：

1. **返回值检查**：大多数函数通过返回值指示成功或失败
2. **日志输出**：使用CN_LOG宏输出错误信息
3. **统计信息**：记录错误次数到统计信息中

## 使用示例

### 完整示例：创建管理器并分配内存

```c
#include "CN_virtual_memory.h"
#include <stdio.h>

int main()
{
    // 创建虚拟内存管理器
    Stru_CN_VirtualMemoryConfig_t config = CN_VIRTUAL_MEMORY_CONFIG_DEFAULT;
    Stru_CN_VirtualMemoryManager_t* manager = CN_virtual_memory_create(&config, NULL);
    
    if (!manager) {
        printf("创建虚拟内存管理器失败\n");
        return 1;
    }
    
    // 分配虚拟地址空间
    uintptr_t addr1 = CN_virtual_memory_alloc(manager, 4096, 0, 
                                             Eum_PAGE_MAPPING_COMMIT,
                                             Eum_PAGE_PROTECTION_READ_WRITE);
    
    if (addr1 == 0) {
        printf("分配虚拟地址空间失败\n");
        CN_virtual_memory_destroy(manager);
        return 1;
    }
    
    printf("分配的虚拟地址: 0x%p\n", (void*)addr1);
    
    // 映射物理内存
    bool success = CN_virtual_memory_map_physical(manager, 0x20000000, 0x30000000,
                                                  4096, Eum_PAGE_PROTECTION_READ_WRITE,
                                                  Eum_PAGE_MAPPING_FIXED);
    
    if (!success) {
        printf("映射物理内存失败\n");
    }
    
    // 获取统计信息
    Stru_CN_VirtualMemoryStats_t stats;
    if (CN_virtual_memory_get_stats(manager, &stats)) {
        printf("已映射页面: %zu\n", stats.mapped_pages);
        printf("保留页面: %zu\n", stats.reserved_pages);
    }
    
    // 清理
    CN_virtual_memory_free(manager, addr1, 4096);
    CN_virtual_memory_destroy(manager);
    
    return 0;
}
```

## 性能建议

1. **页面大小选择**：根据应用需求选择合适的页面大小
   - 小页面（4KB）：内存利用率高，适合通用应用
   - 大页面（2MB/1GB）：TLB效率高，适合大数据处理

2. **按需分页**：使用`Eum_PAGE_MAPPING_LAZY`标志延迟分配物理内存

3. **TLB管理**：合理使用`CN_virtual_memory_flush_tlb`函数

4. **统计监控**：定期检查统计信息，优化内存使用

## 线程安全

当前版本的虚拟内存管理API**不是线程安全的**。如果需要在多线程环境中使用，需要：

1. 使用外部锁机制保护对管理器的访问
2. 避免并发调用可能修改内部状态的函数

## 平台兼容性

虚拟内存管理模块设计为平台无关的，但需要注意：

1. **MMU实现**：需要为不同平台提供相应的MMU接口实现
2. **页面大小**：不同平台的默认页面大小可能不同
3. **地址空间**：虚拟地址空间范围可能受平台限制

## 版本历史

- **v1.0.0** (2026-01-03): 初始版本
  - 基本虚拟内存管理功能
  - 分页机制和MMU抽象
  - 完整的API文档

## 相关文档

- [虚拟内存管理模块README](../src/infrastructure/memory/virtual/README.md)
- [CN_Language项目架构设计原则](../../../docs/architecture/架构设计原则.md)
- [内存管理模块API文档](./CN_memory_API.md)

## 许可证

MIT许可证
