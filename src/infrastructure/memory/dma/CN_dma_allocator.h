/******************************************************************************
 * 文件名: CN_dma_allocator.h
 * 功能: CN_Language DMA内存分配器接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建DMA内存分配器，支持DMA内存区域管理
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_DMA_ALLOCATOR_H
#define CN_DMA_ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// DMA内存分配器配置结构体
// ============================================================================

/**
 * @brief DMA内存属性枚举
 * 
 * 定义DMA内存的不同属性，影响内存的缓存行为和访问特性。
 */
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

/**
 * @brief DMA方向枚举
 * 
 * 定义DMA传输的方向，影响内存的同步需求。
 */
typedef enum Eum_CN_DmaDirection_t
{
    Eum_DMA_DIRECTION_TO_DEVICE = 0,      /**< 从内存到设备 */
    Eum_DMA_DIRECTION_FROM_DEVICE = 1,    /**< 从设备到内存 */
    Eum_DMA_DIRECTION_BIDIRECTIONAL = 2,  /**< 双向传输 */
    Eum_DMA_DIRECTION_NONE = 3            /**< 无DMA传输（仅分配） */
} Eum_CN_DmaDirection_t;

/**
 * @brief DMA内存分配器配置选项
 * 
 * 用于配置DMA内存分配器的行为和性能参数。
 * DMA内存分配器专门管理用于DMA传输的内存区域。
 */
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

/**
 * @brief 默认DMA内存分配器配置
 * 
 * 提供合理的默认配置值。
 */
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

// ============================================================================
// DMA内存分配器统计信息
// ============================================================================

/**
 * @brief DMA内存分配器统计信息
 * 
 * 记录DMA内存分配器的运行统计信息，用于性能监控和调试。
 */
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

// ============================================================================
// DMA缓冲区描述符
// ============================================================================

/**
 * @brief DMA缓冲区描述符
 * 
 * 描述一个DMA缓冲区，包含地址、大小和属性信息。
 */
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

/**
 * @brief DMA分散-聚集列表元素
 * 
 * 用于分散-聚集DMA传输，描述不连续的内存区域。
 */
typedef struct Stru_CN_DmaScatterGatherElement_t
{
    uintptr_t physical_address;          /**< 物理地址 */
    size_t length;                       /**< 长度（字节） */
} Stru_CN_DmaScatterGatherElement_t;

/**
 * @brief DMA分散-聚集列表
 * 
 * 包含多个分散-聚集元素的列表，用于描述不连续的DMA缓冲区。
 */
typedef struct Stru_CN_DmaScatterGatherList_t
{
    Stru_CN_DmaScatterGatherElement_t* elements; /**< 元素数组 */
    size_t element_count;                /**< 元素数量 */
    size_t total_length;                 /**< 总长度（字节） */
    bool is_mapped;                      /**< 是否已映射到设备地址空间 */
} Stru_CN_DmaScatterGatherList_t;

// ============================================================================
// 分配信息结构体（用于跟踪）
// ============================================================================

/**
 * @brief DMA内存分配信息记录
 * 
 * 用于跟踪每次分配的信息，便于调试和泄漏检测。
 */
typedef struct Stru_CN_DmaAllocationInfo_t
{
    uintptr_t physical_address;          /**< 分配的物理地址 */
    void* virtual_address;               /**< 分配的虚拟地址 */
    size_t size;                         /**< 分配的大小（字节） */
    size_t alignment;                    /**< 对齐要求 */
    Eum_CN_DmaMemoryAttribute_t attribute; /**< 内存属性 */
    const char* file;                    /**< 分配所在的文件 */
    int line;                            /**< 分配所在的行号 */
    uint64_t timestamp;                  /**< 分配时间戳 */
    const char* purpose;                 /**< 分配目的（可选） */
    uint32_t handle;                     /**< 分配句柄 */
} Stru_CN_DmaAllocationInfo_t;

// ============================================================================
// DMA内存分配器句柄
// ============================================================================

/**
 * @brief DMA内存分配器句柄
 * 
 * 不透明指针，代表一个DMA内存分配器实例。
 */
typedef struct Stru_CN_DmaAllocator_t Stru_CN_DmaAllocator_t;

// ============================================================================
// DMA内存分配器管理接口
// ============================================================================

/**
 * @brief 创建DMA内存分配器
 * 
 * @param config DMA内存分配器配置
 * @return DMA内存分配器句柄，失败返回NULL
 */
Stru_CN_DmaAllocator_t* CN_dma_create(const Stru_CN_DmaConfig_t* config);

/**
 * @brief 销毁DMA内存分配器
 * 
 * @param allocator DMA内存分配器句柄
 */
void CN_dma_destroy(Stru_CN_DmaAllocator_t* allocator);

// ============================================================================
// DMA缓冲区分配接口
// ============================================================================

/**
 * @brief 分配DMA缓冲区
 * 
 * @param allocator DMA内存分配器句柄
 * @param size 要分配的缓冲区大小（字节）
 * @param alignment 对齐要求（必须是缓存行大小的倍数）
 * @param attribute 内存属性
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 * @param purpose 分配目的描述（可选，可为NULL）
 * @return DMA缓冲区描述符指针，失败返回NULL
 */
Stru_CN_DmaBuffer_t* CN_dma_alloc_buffer(Stru_CN_DmaAllocator_t* allocator,
                                         size_t size,
                                         size_t alignment,
                                         Eum_CN_DmaMemoryAttribute_t attribute,
                                         const char* file, int line,
                                         const char* purpose);

/**
 * @brief 分配物理连续的DMA缓冲区
 * 
 * @param allocator DMA内存分配器句柄
 * @param size 要分配的缓冲区大小（字节）
 * @param alignment 对齐要求（必须是页面大小的倍数）
 * @param attribute 内存属性
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 * @param purpose 分配目的描述（可选，可为NULL）
 * @return DMA缓冲区描述符指针，失败返回NULL
 */
Stru_CN_DmaBuffer_t* CN_dma_alloc_contiguous(Stru_CN_DmaAllocator_t* allocator,
                                             size_t size,
                                             size_t alignment,
                                             Eum_CN_DmaMemoryAttribute_t attribute,
                                             const char* file, int line,
                                             const char* purpose);

/**
 * @brief 分配分散-聚集DMA缓冲区
 * 
 * @param allocator DMA内存分配器句柄
 * @param total_size 总缓冲区大小（字节）
 * @param max_elements 最大分散元素数量
 * @param attribute 内存属性
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 * @param purpose 分配目的描述（可选，可为NULL）
 * @return 分散-聚集列表指针，失败返回NULL
 */
Stru_CN_DmaScatterGatherList_t* CN_dma_alloc_scatter_gather(
    Stru_CN_DmaAllocator_t* allocator,
    size_t total_size,
    size_t max_elements,
    Eum_CN_DmaMemoryAttribute_t attribute,
    const char* file, int line,
    const char* purpose);

/**
 * @brief 释放DMA缓冲区
 * 
 * @param allocator DMA内存分配器句柄
 * @param buffer 要释放的DMA缓冲区
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 */
void CN_dma_free_buffer(Stru_CN_DmaAllocator_t* allocator,
                        Stru_CN_DmaBuffer_t* buffer,
                        const char* file, int line);

/**
 * @brief 释放分散-聚集DMA缓冲区
 * 
 * @param allocator DMA内存分配器句柄
 * @param sg_list 要释放的分散-聚集列表
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 */
void CN_dma_free_scatter_gather(Stru_CN_DmaAllocator_t* allocator,
                                Stru_CN_DmaScatterGatherList_t* sg_list,
                                const char* file, int line);

// ============================================================================
// DMA缓冲区同步接口
// ============================================================================

/**
 * @brief 同步DMA缓冲区（CPU到设备）
 * 
 * 在CPU写入数据后调用，确保数据对设备可见。
 * 
 * @param buffer DMA缓冲区
 * @param offset 偏移量（字节）
 * @param size 同步大小（字节），0表示整个缓冲区
 */
void CN_dma_sync_to_device(Stru_CN_DmaBuffer_t* buffer,
                           size_t offset, size_t size);

/**
 * @brief 同步DMA缓冲区（设备到CPU）
 * 
 * 在设备写入数据后调用，确保数据对CPU可见。
 * 
 * @param buffer DMA缓冲区
 * @param offset 偏移量（字节）
 * @param size 同步大小（字节），0表示整个缓冲区
 */
void CN_dma_sync_from_device(Stru_CN_DmaBuffer_t* buffer,
                             size_t offset, size_t size);

/**
 * @brief 同步分散-聚集DMA缓冲区（CPU到设备）
 * 
 * @param sg_list 分散-聚集列表
 */
void CN_dma_sync_sg_to_device(Stru_CN_DmaScatterGatherList_t* sg_list);

/**
 * @brief 同步分散-聚集DMA缓冲区（设备到CPU）
 * 
 * @param sg_list 分散-聚集列表
 */
void CN_dma_sync_sg_from_device(Stru_CN_DmaScatterGatherList_t* sg_list);

// ============================================================================
// DMA内存分配器查询接口
// ============================================================================

/**
 * @brief 获取DMA内存分配器配置
 * 
 * @param allocator DMA内存分配器句柄
 * @param config 输出配置信息
 * @return 获取成功返回true，失败返回false
 */
bool CN_dma_get_config(Stru_CN_DmaAllocator_t* allocator,
                       Stru_CN_DmaConfig_t* config);

/**
 * @brief 获取DMA内存分配器统计信息
 * 
 * @param allocator DMA内存分配器句柄
 * @param stats 输出统计信息
 * @return 获取成功返回true，失败返回false
 */
bool CN_dma_get_stats(Stru_CN_DmaAllocator_t* allocator,
                      Stru_CN_DmaStats_t* stats);

/**
 * @brief 重置DMA内存分配器统计信息
 * 
 * @param allocator DMA内存分配器句柄
 */
void CN_dma_reset_stats(Stru_CN_DmaAllocator_t* allocator);

/**
 * @brief 获取总页面数量
 * 
 * @param allocator DMA内存分配器句柄
 * @return 总页面数量
 */
size_t CN_dma_total_pages(Stru_CN_DmaAllocator_t* allocator);

/**
 * @brief 获取空闲页面数量
 * 
 * @param allocator DMA内存分配器句柄
 * @return 空闲页面数量
 */
size_t CN_dma_get_free_pages(Stru_CN_DmaAllocator_t* allocator);

/**
 * @brief 获取已使用页面数量
 * 
 * @param allocator DMA内存分配器句柄
 * @return 已使用页面数量
 */
size_t CN_dma_used_pages(Stru_CN_DmaAllocator_t* allocator);

/**
 * @brief 获取最大连续空闲页面数量
 * 
 * @param allocator DMA内存分配器句柄
 * @return 最大连续空闲页面数量
 */
size_t CN_dma_largest_free_block(Stru_CN_DmaAllocator_t* allocator);

// ============================================================================
// DMA内存分配器调试接口
// ============================================================================

/**
 * @brief 启用/禁用调试模式
 * 
 * @param allocator DMA内存分配器句柄
 * @param enable 是否启用调试
 */
void CN_dma_enable_debug(Stru_CN_DmaAllocator_t* allocator, bool enable);

/**
 * @brief 验证DMA内存分配器完整性
 * 
 * @param allocator DMA内存分配器句柄
 * @return 如果分配器完整返回true，否则返回false
 */
bool CN_dma_validate(Stru_CN_DmaAllocator_t* allocator);

/**
 * @brief 转储DMA内存分配器状态到标准输出
 * 
 * @param allocator DMA内存分配器句柄
 */
void CN_dma_dump(Stru_CN_DmaAllocator_t* allocator);

/**
 * @brief 转储所有未释放的分配信息
 * 
 * @param allocator DMA内存分配器句柄
 */
void CN_dma_dump_leaks(Stru_CN_DmaAllocator_t* allocator);

/**
 * @brief 设置DMA内存分配器调试回调
 * 
 * @param allocator DMA内存分配器句柄
 * @param callback 调试回调函数
 * @param user_data 用户数据
 */
typedef void (*CN_DmaDebugCallback_t)(const char* message, void* user_data);
void CN_dma_set_debug_callback(Stru_CN_DmaAllocator_t* allocator,
                               CN_DmaDebugCallback_t callback,
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
static inline size_t CN_dma_size_to_pages(size_t size, size_t page_size)
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
static inline size_t CN_dma_pages_to_size(size_t page_count, size_t page_size)
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
static inline uintptr_t CN_dma_align_to_page(uintptr_t address, size_t page_size)
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
static inline bool CN_dma_is_page_aligned(uintptr_t address, size_t page_size)
{
    return (address & (page_size - 1)) == 0;
}

/**
 * @brief 对齐地址到缓存行边界
 * 
 * @param address 地址
 * @param cache_line_size 缓存行大小
 * @return 对齐后的地址
 */
static inline uintptr_t CN_dma_align_to_cache_line(uintptr_t address, size_t cache_line_size)
{
    return (address + cache_line_size - 1) & ~(cache_line_size - 1);
}

/**
 * @brief 检查地址是否缓存行对齐
 * 
 * @param address 地址
 * @param cache_line_size 缓存行大小
 * @return 如果对齐返回true，否则返回false
 */
static inline bool CN_dma_is_cache_line_aligned(uintptr_t address, size_t cache_line_size)
{
    return (address & (cache_line_size - 1)) == 0;
}

#ifdef __cplusplus
}
#endif

#endif // CN_DMA_ALLOCATOR_H
