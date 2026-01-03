/******************************************************************************
 * 文件名: CN_virtual_memory_internal.h
 * 功能: CN_Language虚拟内存管理器内部实现细节
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建虚拟内存管理器内部头文件
 * 版权: MIT许可证
 * 
 * 注意: 此文件仅供虚拟内存管理器内部使用，不应被外部模块包含。
 ******************************************************************************/

#ifndef CN_VIRTUAL_MEMORY_INTERNAL_H
#define CN_VIRTUAL_MEMORY_INTERNAL_H

#include "CN_virtual_memory.h"
#include "../../containers/array/CN_array.h"
#include "../../containers/hash_table/CN_hash_table.h"
#include "../../utils/logs/CN_log.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 内部常量定义
// ============================================================================

/** @brief 最大页表级数 */
#define CN_MAX_PAGE_TABLE_LEVELS 4

/** @brief 每级页表项数量（假设9位索引） */
#define CN_PAGE_TABLE_ENTRIES_PER_LEVEL 512

/** @brief 页面表项大小（字节） */
#define CN_PAGE_TABLE_ENTRY_SIZE 8

/** @brief 默认虚拟地址空间大小（1GB） */
#define CN_DEFAULT_VIRTUAL_ADDRESS_SPACE_SIZE (1UL * 1024UL * 1024UL * 1024UL)

/** @brief 默认物理内存池大小（64MB） */
#define CN_DEFAULT_PHYSICAL_MEMORY_POOL_SIZE (64UL * 1024UL * 1024UL)

// ============================================================================
// 页表项结构
// ============================================================================

/**
 * @brief 页表项标志位定义
 */
typedef enum Eum_CN_PageTableEntryFlags_t
{
    Eum_PTE_PRESENT         = 0x01,   /**< 页面存在于内存中 */
    Eum_PTE_WRITABLE        = 0x02,   /**< 页面可写 */
    Eum_PTE_USER_ACCESSIBLE = 0x04,   /**< 用户模式可访问 */
    Eum_PTE_WRITE_THROUGH   = 0x08,   /**< 直写缓存 */
    Eum_PTE_CACHE_DISABLED  = 0x10,   /**< 禁用缓存 */
    Eum_PTE_ACCESSED        = 0x20,   /**< 页面已被访问 */
    Eum_PTE_DIRTY           = 0x40,   /**< 页面已被写入 */
    Eum_PTE_HUGE_PAGE       = 0x80,   /**< 大页面 */
    Eum_PTE_GLOBAL          = 0x100,  /**< 全局页面（TLB不刷新） */
    Eum_PTE_NO_EXECUTE      = 0x200,  /**< 禁止执行 */
} Eum_CN_PageTableEntryFlags_t;

/**
 * @brief 页表项结构
 * 
 * 表示单个页表项，包含物理地址和标志位。
 */
typedef struct Stru_CN_PageTableEntry_t
{
    uintptr_t physical_address;        /**< 物理地址（页面对齐） */
    uint32_t flags;                    /**< 页表项标志位 */
    uint32_t reserved;                 /**< 保留字段 */
} Stru_CN_PageTableEntry_t;

// ============================================================================
// 页表结构
// ============================================================================

/**
 * @brief 页表级别
 */
typedef enum Eum_CN_PageTableLevel_t
{
    Eum_PAGE_TABLE_LEVEL_1 = 0,   /**< 第1级页表（页目录指针表） */
    Eum_PAGE_TABLE_LEVEL_2 = 1,   /**< 第2级页表（页目录） */
    Eum_PAGE_TABLE_LEVEL_3 = 2,   /**< 第3级页表（页表） */
    Eum_PAGE_TABLE_LEVEL_4 = 3,   /**< 第4级页表（页表项） */
} Eum_CN_PageTableLevel_t;

/**
 * @brief 页表结构
 * 
 * 表示一个页表，包含多个页表项。
 */
typedef struct Stru_CN_PageTable_t
{
    Stru_CN_PageTableEntry_t* entries;     /**< 页表项数组 */
    size_t entry_count;                    /**< 页表项数量 */
    Eum_CN_PageTableLevel_t level;         /**< 页表级别 */
    uintptr_t physical_address;            /**< 页表的物理地址 */
    bool is_allocated;                     /**< 页表是否已分配 */
} Stru_CN_PageTable_t;

// ============================================================================
// 虚拟地址映射信息
// ============================================================================

/**
 * @brief 虚拟地址映射信息
 * 
 * 记录虚拟地址到物理地址的映射关系。
 */
typedef struct Stru_CN_VirtualMapping_t
{
    uintptr_t virtual_address;             /**< 虚拟地址（页面对齐） */
    uintptr_t physical_address;            /**< 物理地址（页面对齐） */
    size_t page_count;                     /**< 映射的页面数量 */
    Eum_CN_PageProtection_t protection;    /**< 页面保护标志 */
    Eum_CN_PageMappingFlags_t flags;       /**< 映射标志 */
    uint64_t timestamp;                    /**< 映射时间戳 */
    const char* purpose;                   /**< 映射目的（可选） */
} Stru_CN_VirtualMapping_t;

// ============================================================================
// 虚拟内存管理器内部结构
// ============================================================================

/**
 * @brief 虚拟内存管理器内部结构
 * 
 * 包含虚拟内存管理器的所有内部状态和数据。
 */
typedef struct Stru_CN_VirtualMemoryManager_t
{
    // 配置信息
    Stru_CN_VirtualMemoryConfig_t config;          /**< 配置信息 */
    
    // MMU接口
    const Stru_CN_MMUInterface_t* mmu_interface;   /**< MMU接口 */
    Stru_CN_MMUInterface_t default_mmu_interface;  /**< 默认MMU接口 */
    
    // 页表管理
    Stru_CN_PageTable_t* page_tables;              /**< 页表数组 */
    size_t page_table_count;                       /**< 页表数量 */
    uintptr_t page_table_base;                     /**< 页表基地址 */
    
    // 虚拟地址空间管理
    Stru_CN_Array_t* virtual_mappings;             /**< 虚拟映射数组 */
    Stru_CN_HashTable_t* virtual_address_map;      /**< 虚拟地址哈希表（快速查找） */
    uintptr_t next_virtual_address;                /**< 下一个可分配的虚拟地址 */
    
    // 物理内存管理
    Stru_CN_Array_t* physical_memory_pool;         /**< 物理内存池 */
    size_t total_physical_pages;                   /**< 总物理页面数量 */
    size_t free_physical_pages;                    /**< 空闲物理页面数量 */
    
    // 统计信息
    Stru_CN_VirtualMemoryStats_t stats;            /**< 统计信息 */
    
    // 调试和状态
    bool is_initialized;                           /**< 是否已初始化 */
    bool debug_enabled;                            /**< 调试是否启用 */
    Eum_CN_LogLevel_t log_level;                   /**< 日志级别 */
    
    // 同步和锁（未来扩展）
    void* lock;                                    /**< 锁对象（未来使用） */
    
} Stru_CN_VirtualMemoryManager_t;

// ============================================================================
// 内部辅助函数声明
// ============================================================================

/**
 * @brief 初始化页表
 * 
 * @param manager 虚拟内存管理器
 * @return 初始化成功返回true，失败返回false
 */
bool CN_virtual_memory_init_page_tables(Stru_CN_VirtualMemoryManager_t* manager);

/**
 * @brief 销毁页表
 * 
 * @param manager 虚拟内存管理器
 */
void CN_virtual_memory_destroy_page_tables(Stru_CN_VirtualMemoryManager_t* manager);

/**
 * @brief 分配物理页面
 * 
 * @param manager 虚拟内存管理器
 * @param page_count 要分配的页面数量
 * @return 分配的物理地址，失败返回0
 */
uintptr_t CN_virtual_memory_alloc_physical_pages(Stru_CN_VirtualMemoryManager_t* manager,
                                                 size_t page_count);

/**
 * @brief 释放物理页面
 * 
 * @param manager 虚拟内存管理器
 * @param physical_address 物理地址
 * @param page_count 页面数量
 */
void CN_virtual_memory_free_physical_pages(Stru_CN_VirtualMemoryManager_t* manager,
                                           uintptr_t physical_address,
                                           size_t page_count);

/**
 * @brief 查找虚拟地址映射
 * 
 * @param manager 虚拟内存管理器
 * @param virtual_address 虚拟地址
 * @return 映射信息索引，未找到返回-1
 */
int CN_virtual_memory_find_mapping(Stru_CN_VirtualMemoryManager_t* manager,
                                   uintptr_t virtual_address);

/**
 * @brief 添加虚拟地址映射
 * 
 * @param manager 虚拟内存管理器
 * @param mapping 映射信息
 * @return 添加成功返回true，失败返回false
 */
bool CN_virtual_memory_add_mapping(Stru_CN_VirtualMemoryManager_t* manager,
                                   const Stru_CN_VirtualMapping_t* mapping);

/**
 * @brief 移除虚拟地址映射
 * 
 * @param manager 虚拟内存管理器
 * @param virtual_address 虚拟地址
 * @param page_count 页面数量
 * @return 移除成功返回true，失败返回false
 */
bool CN_virtual_memory_remove_mapping(Stru_CN_VirtualMemoryManager_t* manager,
                                      uintptr_t virtual_address,
                                      size_t page_count);

/**
 * @brief 更新页表项
 * 
 * @param manager 虚拟内存管理器
 * @param virtual_address 虚拟地址
 * @param physical_address 物理地址
 * @param protection 保护标志
 * @param flags 映射标志
 * @return 更新成功返回true，失败返回false
 */
bool CN_virtual_memory_update_page_table_entry(Stru_CN_VirtualMemoryManager_t* manager,
                                               uintptr_t virtual_address,
                                               uintptr_t physical_address,
                                               Eum_CN_PageProtection_t protection,
                                               Eum_CN_PageMappingFlags_t flags);

/**
 * @brief 清除页表项
 * 
 * @param manager 虚拟内存管理器
 * @param virtual_address 虚拟地址
 */
void CN_virtual_memory_clear_page_table_entry(Stru_CN_VirtualMemoryManager_t* manager,
                                              uintptr_t virtual_address);

/**
 * @brief 虚拟地址分解
 * 
 * @param virtual_address 虚拟地址
 * @param page_size 页面大小
 * @param levels 页表级数
 * @param indices 输出索引数组（必须至少有levels个元素）
 */
void CN_virtual_memory_decompose_address(uintptr_t virtual_address,
                                         size_t page_size,
                                         size_t levels,
                                         size_t* indices);

/**
 * @brief 保护标志转换为页表项标志
 * 
 * @param protection 保护标志
 * @param flags 映射标志
 * @return 页表项标志
 */
uint32_t CN_virtual_memory_protection_to_pte_flags(Eum_CN_PageProtection_t protection,
                                                   Eum_CN_PageMappingFlags_t flags);

/**
 * @brief 页表项标志转换为保护标志
 * 
 * @param pte_flags 页表项标志
 * @param protection 输出保护标志
 * @param flags 输出映射标志
 */
void CN_virtual_memory_pte_flags_to_protection(uint32_t pte_flags,
                                               Eum_CN_PageProtection_t* protection,
                                               Eum_CN_PageMappingFlags_t* flags);

/**
 * @brief 默认MMU接口实现
 */
extern const Stru_CN_MMUInterface_t CN_default_mmu_interface;

#ifdef __cplusplus
}
#endif

#endif // CN_VIRTUAL_MEMORY_INTERNAL_H
