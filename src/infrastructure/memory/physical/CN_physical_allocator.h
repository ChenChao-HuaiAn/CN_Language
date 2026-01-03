/******************************************************************************
 * 文件名: CN_physical_allocator.h
 * 功能: CN_Language物理内存分配器接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建物理内存分配器，支持页框分配
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_PHYSICAL_ALLOCATOR_H
#define CN_PHYSICAL_ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 物理内存分配器配置结构体
// ============================================================================

/**
 * @brief 物理内存分配器配置选项
 * 
 * 用于配置物理内存分配器的行为和性能参数。
 * 物理内存分配器管理物理内存页框，通常用于操作系统内核开发。
 */
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

/**
 * @brief 默认物理内存分配器配置
 * 
 * 提供合理的默认配置值。
 */
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

// ============================================================================
// 页面大小常量
// ============================================================================

/** @brief 标准页面大小（4KB） */
#define CN_PAGE_SIZE_4KB    4096UL

/** @brief 大页面大小（2MB） */
#define CN_PAGE_SIZE_2MB    (2UL * 1024UL * 1024UL)

/** @brief 巨大页面大小（1GB） */
#define CN_PAGE_SIZE_1GB    (1024UL * 1024UL * 1024UL)

// ============================================================================
// 物理内存分配器统计信息
// ============================================================================

/**
 * @brief 物理内存分配器统计信息
 * 
 * 记录物理内存分配器的运行统计信息，用于性能监控和调试。
 */
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

// ============================================================================
// 分配信息结构体（用于跟踪）
// ============================================================================

/**
 * @brief 物理内存分配信息记录
 * 
 * 用于跟踪每次分配的信息，便于调试和泄漏检测。
 */
typedef struct Stru_CN_PhysicalAllocationInfo_t
{
    uintptr_t physical_address;   /**< 分配的物理地址 */
    size_t page_count;            /**< 分配的页面数量 */
    const char* file;             /**< 分配所在的文件 */
    int line;                     /**< 分配所在的行号 */
    uint64_t timestamp;           /**< 分配时间戳 */
    const char* purpose;          /**< 分配目的（可选） */
} Stru_CN_PhysicalAllocationInfo_t;

// ============================================================================
// 物理内存分配器句柄
// ============================================================================

/**
 * @brief 物理内存分配器句柄
 * 
 * 不透明指针，代表一个物理内存分配器实例。
 */
typedef struct Stru_CN_PhysicalAllocator_t Stru_CN_PhysicalAllocator_t;

// ============================================================================
// 物理内存分配器管理接口
// ============================================================================

/**
 * @brief 创建物理内存分配器
 * 
 * @param config 物理内存分配器配置
 * @return 物理内存分配器句柄，失败返回NULL
 */
Stru_CN_PhysicalAllocator_t* CN_physical_create(const Stru_CN_PhysicalConfig_t* config);

/**
 * @brief 销毁物理内存分配器
 * 
 * @param allocator 物理内存分配器句柄
 */
void CN_physical_destroy(Stru_CN_PhysicalAllocator_t* allocator);

/**
 * @brief 从物理内存分配器分配页面
 * 
 * @param allocator 物理内存分配器句柄
 * @param page_count 要分配的页面数量
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 * @param purpose 分配目的描述（可选，可为NULL）
 * @return 分配的物理地址，失败返回0
 */
uintptr_t CN_physical_alloc_pages(Stru_CN_PhysicalAllocator_t* allocator, 
                                  size_t page_count, 
                                  const char* file, int line, 
                                  const char* purpose);

/**
 * @brief 从物理内存分配器分配对齐的页面
 * 
 * @param allocator 物理内存分配器句柄
 * @param page_count 要分配的页面数量
 * @param alignment 对齐要求（必须是页面大小的倍数）
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 * @param purpose 分配目的描述（可选，可为NULL）
 * @return 分配的物理地址，失败返回0
 */
uintptr_t CN_physical_alloc_pages_aligned(Stru_CN_PhysicalAllocator_t* allocator, 
                                          size_t page_count, 
                                          size_t alignment,
                                          const char* file, int line, 
                                          const char* purpose);

/**
 * @brief 从物理内存分配器分配特定地址的页面
 * 
 * @param allocator 物理内存分配器句柄
 * @param physical_address 请求的物理地址（必须是页面对齐的）
 * @param page_count 要分配的页面数量
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 * @param purpose 分配目的描述（可选，可为NULL）
 * @return 分配的物理地址，失败返回0
 */
uintptr_t CN_physical_alloc_pages_at(Stru_CN_PhysicalAllocator_t* allocator, 
                                     uintptr_t physical_address,
                                     size_t page_count, 
                                     const char* file, int line, 
                                     const char* purpose);

/**
 * @brief 释放物理内存分配器分配的页面
 * 
 * @param allocator 物理内存分配器句柄
 * @param physical_address 要释放的物理地址
 * @param page_count 要释放的页面数量
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 */
void CN_physical_free_pages(Stru_CN_PhysicalAllocator_t* allocator, 
                            uintptr_t physical_address,
                            size_t page_count, 
                            const char* file, int line);

// ============================================================================
// 物理内存分配器查询接口
// ============================================================================

/**
 * @brief 获取物理内存分配器配置
 * 
 * @param allocator 物理内存分配器句柄
 * @param config 输出配置信息
 * @return 获取成功返回true，失败返回false
 */
bool CN_physical_get_config(Stru_CN_PhysicalAllocator_t* allocator, 
                            Stru_CN_PhysicalConfig_t* config);

/**
 * @brief 获取物理内存分配器统计信息
 * 
 * @param allocator 物理内存分配器句柄
 * @param stats 输出统计信息
 * @return 获取成功返回true，失败返回false
 */
bool CN_physical_get_stats(Stru_CN_PhysicalAllocator_t* allocator, 
                           Stru_CN_PhysicalStats_t* stats);

/**
 * @brief 重置物理内存分配器统计信息
 * 
 * @param allocator 物理内存分配器句柄
 */
void CN_physical_reset_stats(Stru_CN_PhysicalAllocator_t* allocator);

/**
 * @brief 检查物理地址是否已分配
 * 
 * @param allocator 物理内存分配器句柄
 * @param physical_address 要检查的物理地址
 * @return 如果地址已分配返回true，否则返回false
 */
bool CN_physical_is_allocated(Stru_CN_PhysicalAllocator_t* allocator, 
                              uintptr_t physical_address);

/**
 * @brief 获取总页面数量
 * 
 * @param allocator 物理内存分配器句柄
 * @return 总页面数量
 */
size_t CN_physical_total_pages(Stru_CN_PhysicalAllocator_t* allocator);

/**
 * @brief 获取空闲页面数量
 * 
 * @param allocator 物理内存分配器句柄
 * @return 空闲页面数量
 */
size_t CN_physical_get_free_pages(Stru_CN_PhysicalAllocator_t* allocator);

/**
 * @brief 获取已使用页面数量
 * 
 * @param allocator 物理内存分配器句柄
 * @return 已使用页面数量
 */
size_t CN_physical_used_pages(Stru_CN_PhysicalAllocator_t* allocator);

/**
 * @brief 获取最大连续空闲页面数量
 * 
 * @param allocator 物理内存分配器句柄
 * @return 最大连续空闲页面数量
 */
size_t CN_physical_largest_free_block(Stru_CN_PhysicalAllocator_t* allocator);

// ============================================================================
// 物理内存分配器调试接口
// ============================================================================

/**
 * @brief 启用/禁用调试模式
 * 
 * @param allocator 物理内存分配器句柄
 * @param enable 是否启用调试
 */
void CN_physical_enable_debug(Stru_CN_PhysicalAllocator_t* allocator, bool enable);

/**
 * @brief 验证物理内存分配器完整性
 * 
 * @param allocator 物理内存分配器句柄
 * @return 如果分配器完整返回true，否则返回false
 */
bool CN_physical_validate(Stru_CN_PhysicalAllocator_t* allocator);

/**
 * @brief 转储物理内存分配器状态到标准输出
 * 
 * @param allocator 物理内存分配器句柄
 */
void CN_physical_dump(Stru_CN_PhysicalAllocator_t* allocator);

/**
 * @brief 转储所有未释放的分配信息
 * 
 * @param allocator 物理内存分配器句柄
 */
void CN_physical_dump_leaks(Stru_CN_PhysicalAllocator_t* allocator);

/**
 * @brief 设置物理内存分配器调试回调
 * 
 * @param allocator 物理内存分配器句柄
 * @param callback 调试回调函数
 * @param user_data 用户数据
 */
typedef void (*CN_PhysicalDebugCallback_t)(const char* message, void* user_data);
void CN_physical_set_debug_callback(Stru_CN_PhysicalAllocator_t* allocator, 
                                    CN_PhysicalDebugCallback_t callback, 
                                    void* user_data);

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
static inline size_t CN_physical_size_to_pages(size_t size, size_t page_size)
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
static inline size_t CN_physical_pages_to_size(size_t page_count, size_t page_size)
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
static inline uintptr_t CN_physical_align_to_page(uintptr_t address, size_t page_size)
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
static inline bool CN_physical_is_page_aligned(uintptr_t address, size_t page_size)
{
    return (address & (page_size - 1)) == 0;
}

#ifdef __cplusplus
}
#endif

#endif // CN_PHYSICAL_ALLOCATOR_H
