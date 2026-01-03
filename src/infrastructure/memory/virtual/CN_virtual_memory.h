/******************************************************************************
 * 文件名: CN_virtual_memory.h
 * 功能: CN_Language虚拟内存管理接口（分页、MMU抽象）
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建虚拟内存管理模块，支持分页和MMU抽象
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_VIRTUAL_MEMORY_H
#define CN_VIRTUAL_MEMORY_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 虚拟内存管理配置结构体
// ============================================================================

/**
 * @brief 虚拟内存管理器配置选项
 * 
 * 用于配置虚拟内存管理器的行为和性能参数。
 * 虚拟内存管理器负责管理虚拟地址空间、页表和MMU抽象。
 */
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

/**
 * @brief 默认虚拟内存管理器配置
 * 
 * 提供合理的默认配置值。
 */
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

// ============================================================================
// 页面大小常量
// ============================================================================

/** @brief 标准页面大小（4KB） */
#define CN_VIRTUAL_PAGE_SIZE_4KB    4096UL

/** @brief 大页面大小（2MB） */
#define CN_VIRTUAL_PAGE_SIZE_2MB    (2UL * 1024UL * 1024UL)

/** @brief 巨大页面大小（1GB） */
#define CN_VIRTUAL_PAGE_SIZE_1GB    (1024UL * 1024UL * 1024UL)

// ============================================================================
// 页面保护标志
// ============================================================================

/** @brief 页面保护标志位定义 */
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

// ============================================================================
// 页面映射标志
// ============================================================================

/** @brief 页面映射标志位定义 */
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

// ============================================================================
// MMU抽象接口
// ============================================================================

/**
 * @brief MMU（内存管理单元）抽象接口
 * 
 * 定义统一的MMU操作接口，支持不同架构的MMU实现。
 */
typedef struct Stru_CN_MMUInterface_t
{
    /**
     * @brief 初始化MMU
     * 
     * @param config 虚拟内存配置
     * @return 初始化成功返回true，失败返回false
     */
    bool (*initialize)(const Stru_CN_VirtualMemoryConfig_t* config);
    
    /**
     * @brief 启用MMU
     * 
     * @return 启用成功返回true，失败返回false
     */
    bool (*enable)(void);
    
    /**
     * @brief 禁用MMU
     */
    void (*disable)(void);
    
    /**
     * @brief 刷新TLB（转换后备缓冲区）
     * 
     * @param virtual_address 虚拟地址（0表示刷新全部）
     */
    void (*flush_tlb)(uintptr_t virtual_address);
    
    /**
     * @brief 设置页表基地址
     * 
     * @param page_table_base 页表基地址
     */
    void (*set_page_table_base)(uintptr_t page_table_base);
    
    /**
     * @brief 获取页表基地址
     * 
     * @return 当前页表基地址
     */
    uintptr_t (*get_page_table_base)(void);
    
    /**
     * @brief 获取架构信息
     * 
     * @param info_buffer 信息缓冲区
     * @param buffer_size 缓冲区大小
     */
    void (*get_architecture_info)(char* info_buffer, size_t buffer_size);
    
} Stru_CN_MMUInterface_t;

// ============================================================================
// 虚拟内存管理器统计信息
// ============================================================================

/**
 * @brief 虚拟内存管理器统计信息
 * 
 * 记录虚拟内存管理器的运行统计信息，用于性能监控和调试。
 */
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

// ============================================================================
// 页面错误信息
// ============================================================================

/**
 * @brief 页面错误信息结构体
 * 
 * 用于记录页面错误的详细信息。
 */
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

// ============================================================================
// 虚拟内存管理器句柄
// ============================================================================

/**
 * @brief 虚拟内存管理器句柄
 * 
 * 不透明指针，代表一个虚拟内存管理器实例。
 */
typedef struct Stru_CN_VirtualMemoryManager_t Stru_CN_VirtualMemoryManager_t;

// ============================================================================
// 虚拟内存管理器管理接口
// ============================================================================

/**
 * @brief 创建虚拟内存管理器
 * 
 * @param config 虚拟内存管理器配置
 * @param mmu_interface MMU接口（可为NULL，使用默认实现）
 * @return 虚拟内存管理器句柄，失败返回NULL
 */
Stru_CN_VirtualMemoryManager_t* CN_virtual_memory_create(
    const Stru_CN_VirtualMemoryConfig_t* config,
    const Stru_CN_MMUInterface_t* mmu_interface);

/**
 * @brief 销毁虚拟内存管理器
 * 
 * @param manager 虚拟内存管理器句柄
 */
void CN_virtual_memory_destroy(Stru_CN_VirtualMemoryManager_t* manager);

// ============================================================================
// 虚拟地址空间管理接口
// ============================================================================

/**
 * @brief 分配虚拟地址空间
 * 
 * @param manager 虚拟内存管理器句柄
 * @param size 要分配的大小（字节）
 * @param alignment 对齐要求（必须是页面大小的倍数，0表示使用默认页面大小）
 * @param flags 映射标志
 * @param protection 页面保护标志
 * @return 分配的虚拟地址，失败返回0
 */
uintptr_t CN_virtual_memory_alloc(Stru_CN_VirtualMemoryManager_t* manager,
                                  size_t size,
                                  size_t alignment,
                                  Eum_CN_PageMappingFlags_t flags,
                                  Eum_CN_PageProtection_t protection);

/**
 * @brief 在指定地址分配虚拟地址空间
 * 
 * @param manager 虚拟内存管理器句柄
 * @param virtual_address 请求的虚拟地址（必须是页面对齐的）
 * @param size 要分配的大小（字节）
 * @param flags 映射标志
 * @param protection 页面保护标志
 * @return 分配的虚拟地址，失败返回0
 */
uintptr_t CN_virtual_memory_alloc_at(Stru_CN_VirtualMemoryManager_t* manager,
                                     uintptr_t virtual_address,
                                     size_t size,
                                     Eum_CN_PageMappingFlags_t flags,
                                     Eum_CN_PageProtection_t protection);

/**
 * @brief 释放虚拟地址空间
 * 
 * @param manager 虚拟内存管理器句柄
 * @param virtual_address 要释放的虚拟地址
 * @param size 要释放的大小（字节）
 */
void CN_virtual_memory_free(Stru_CN_VirtualMemoryManager_t* manager,
                            uintptr_t virtual_address,
                            size_t size);

/**
 * @brief 保留虚拟地址空间（不分配物理内存）
 * 
 * @param manager 虚拟内存管理器句柄
 * @param size 要保留的大小（字节）
 * @param alignment 对齐要求
 * @return 保留的虚拟地址，失败返回0
 */
uintptr_t CN_virtual_memory_reserve(Stru_CN_VirtualMemoryManager_t* manager,
                                    size_t size,
                                    size_t alignment);

/**
 * @brief 提交物理内存到虚拟地址空间
 * 
 * @param manager 虚拟内存管理器句柄
 * @param virtual_address 虚拟地址
 * @param size 要提交的大小（字节）
 * @param protection 页面保护标志
 * @return 提交成功返回true，失败返回false
 */
bool CN_virtual_memory_commit(Stru_CN_VirtualMemoryManager_t* manager,
                              uintptr_t virtual_address,
                              size_t size,
                              Eum_CN_PageProtection_t protection);

/**
 * @brief 取消提交物理内存（保留虚拟地址空间）
 * 
 * @param manager 虚拟内存管理器句柄
 * @param virtual_address 虚拟地址
 * @param size 要取消提交的大小（字节）
 */
void CN_virtual_memory_decommit(Stru_CN_VirtualMemoryManager_t* manager,
                                uintptr_t virtual_address,
                                size_t size);

// ============================================================================
// 页面映射接口
// ============================================================================

/**
 * @brief 映射物理内存到虚拟地址空间
 * 
 * @param manager 虚拟内存管理器句柄
 * @param virtual_address 虚拟地址
 * @param physical_address 物理地址
 * @param size 映射大小（字节）
 * @param protection 页面保护标志
 * @param flags 映射标志
 * @return 映射成功返回true，失败返回false
 */
bool CN_virtual_memory_map_physical(Stru_CN_VirtualMemoryManager_t* manager,
                                    uintptr_t virtual_address,
                                    uintptr_t physical_address,
                                    size_t size,
                                    Eum_CN_PageProtection_t protection,
                                    Eum_CN_PageMappingFlags_t flags);

/**
 * @brief 取消映射虚拟地址空间
 * 
 * @param manager 虚拟内存管理器句柄
 * @param virtual_address 虚拟地址
 * @param size 取消映射的大小（字节）
 */
void CN_virtual_memory_unmap(Stru_CN_VirtualMemoryManager_t* manager,
                             uintptr_t virtual_address,
                             size_t size);

/**
 * @brief 重新映射虚拟地址空间（更改保护标志）
 * 
 * @param manager 虚拟内存管理器句柄
 * @param virtual_address 虚拟地址
 * @param size 重新映射的大小（字节）
 * @param new_protection 新的页面保护标志
 * @return 重新映射成功返回true，失败返回false
 */
bool CN_virtual_memory_remap(Stru_CN_VirtualMemoryManager_t* manager,
                             uintptr_t virtual_address,
                             size_t size,
                             Eum_CN_PageProtection_t new_protection);

/**
 * @brief 查询虚拟地址的映射信息
 * 
 * @param manager 虚拟内存管理器句柄
 * @param virtual_address 虚拟地址
 * @param physical_address 输出物理地址（可为NULL）
 * @param protection 输出保护标志（可为NULL）
 * @param flags 输出映射标志（可为NULL）
 * @return 如果地址已映射返回true，否则返回false
 */
bool CN_virtual_memory_query(Stru_CN_VirtualMemoryManager_t* manager,
                             uintptr_t virtual_address,
                             uintptr_t* physical_address,
                             Eum_CN_PageProtection_t* protection,
                             Eum_CN_PageMappingFlags_t* flags);

// ============================================================================
// 页表管理接口
// ============================================================================

/**
 * @brief 获取页表基地址
 * 
 * @param manager 虚拟内存管理器句柄
 * @return 页表基地址
 */
uintptr_t CN_virtual_memory_get_page_table_base(Stru_CN_VirtualMemoryManager_t* manager);

/**
 * @brief 设置页表基地址
 * 
 * @param manager 虚拟内存管理器句柄
 * @param page_table_base 页表基地址
 */
void CN_virtual_memory_set_page_table_base(Stru_CN_VirtualMemoryManager_t* manager,
                                           uintptr_t page_table_base);

/**
 * @brief 刷新TLB（转换后备缓冲区）
 * 
 * @param manager 虚拟内存管理器句柄
 * @param virtual_address 虚拟地址（0表示刷新全部）
 */
void CN_virtual_memory_flush_tlb(Stru_CN_VirtualMemoryManager_t* manager,
                                 uintptr_t virtual_address);

// ============================================================================
// 统计和调试接口
// ============================================================================

/**
 * @brief 获取虚拟内存管理器统计信息
 * 
 * @param manager 虚拟内存管理器句柄
 * @param stats 输出统计信息
 * @return 获取成功返回true，失败返回false
 */
bool CN_virtual_memory_get_stats(Stru_CN_VirtualMemoryManager_t* manager,
                                 Stru_CN_VirtualMemoryStats_t* stats);

/**
 * @brief 重置虚拟内存管理器统计信息
 * 
 * @param manager 虚拟内存管理器句柄
 */
void CN_virtual_memory_reset_stats(Stru_CN_VirtualMemoryManager_t* manager);

/**
 * @brief 启用/禁用调试模式
 * 
 * @param manager 虚拟内存管理器句柄
 * @param enable 是否启用调试
 */
void CN_virtual_memory_enable_debug(Stru_CN_VirtualMemoryManager_t* manager, bool enable);

/**
 * @brief 验证虚拟内存管理器完整性
 * 
 * @param manager 虚拟内存管理器句柄
 * @return 如果管理器完整返回true，否则返回false
 */
bool CN_virtual_memory_validate(Stru_CN_VirtualMemoryManager_t* manager);

/**
 * @brief 转储虚拟内存管理器状态到标准输出
 * 
 * @param manager 虚拟内存管理器句柄
 */
void CN_virtual_memory_dump(Stru_CN_VirtualMemoryManager_t* manager);

/**
 * @brief 转储所有未释放的映射信息
 * 
 * @param manager 虚拟内存管理器句柄
 */
void CN_virtual_memory_dump_leaks(Stru_CN_VirtualMemoryManager_t* manager);

/**
 * @brief 处理页面错误
 * 
 * @param manager 虚拟内存管理器句柄
 * @param fault_info 页面错误信息
 * @return 处理成功返回true，失败返回false
 */
bool CN_virtual_memory_handle_page_fault(Stru_CN_VirtualMemoryManager_t* manager,
                                         const Stru_CN_PageFaultInfo_t* fault_info);

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 将字节大小转换为页面数量
 * 
 * @param size 字节大小
 * @param page_size 页面大小
 * @return 页面数量（向上取整）
 */
static inline size_t CN_virtual_memory_size_to_pages(size_t size, size_t page_size)
{
    return (size + page_size - 1) / page_size;
}

/**
 * @brief 将页面数量转换为字节大小
 * 
 * @param page_count 页面数量
 * @param page_size 页面大小
 * @return 字节大小
 */
static inline size_t CN_virtual_memory_pages_to_size(size_t page_count, size_t page_size)
{
    return page_count * page_size;
}

/**
 * @brief 对齐地址到页面边界
 * 
 * @param address 地址
 * @param page_size 页面大小
 * @return 对齐后的地址
 */
static inline uintptr_t CN_virtual_memory_align_to_page(uintptr_t address, size_t page_size)
{
    return (address + page_size - 1) & ~(page_size - 1);
}

/**
 * @brief 检查地址是否页面对齐
 * 
 * @param address 地址
 * @param page_size 页面大小
 * @return 如果对齐返回true，否则返回false
 */
static inline bool CN_virtual_memory_is_page_aligned(uintptr_t address, size_t page_size)
{
    return (address & (page_size - 1)) == 0;
}

#ifdef __cplusplus
}
#endif

#endif // CN_VIRTUAL_MEMORY_H
