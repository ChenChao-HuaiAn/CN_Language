/******************************************************************************
 * 文件名: CN_debug_allocator.h
 * 功能: CN_Language调试分配器接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建独立的调试分配器模块，专注于边界检查和泄漏检测
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_DEBUG_ALLOCATOR_H
#define CN_DEBUG_ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 调试分配器配置结构体
// ============================================================================

/**
 * @brief 调试分配器配置选项
 * 
 * 用于配置调试分配器的行为和调试参数。
 * 调试分配器专注于内存安全检测，包括边界检查、泄漏检测、
 * 双重释放检测和内存损坏检测。
 */
typedef struct Stru_CN_DebugConfig_t
{
    bool enable_boundary_check;    /**< 是否启用边界检查（添加保护区域） */
    bool enable_leak_detection;    /**< 是否启用泄漏检测 */
    bool enable_double_free_check; /**< 是否启用双重释放检测 */
    bool enable_memory_corruption_check; /**< 是否启用内存损坏检测 */
    bool enable_statistics;        /**< 是否启用统计信息 */
    bool zero_on_alloc;            /**< 分配时是否清零内存 */
    bool fill_on_alloc;            /**< 分配时是否填充特定模式（调试用） */
    bool fill_on_free;             /**< 释放时是否填充特定模式（调试用） */
    bool track_allocations;        /**< 是否跟踪分配信息（文件、行号） */
    size_t guard_zone_size;        /**< 保护区域大小（字节） */
    size_t max_tracked_allocations;/**< 最大跟踪分配数量 */
    size_t max_single_alloc;       /**< 单次分配最大大小（0表示无限制） */
    size_t max_total_alloc;        /**< 总分配最大大小（0表示无限制） */
} Stru_CN_DebugConfig_t;

/**
 * @brief 默认调试分配器配置
 * 
 * 提供合理的默认配置值，启用所有调试功能。
 */
#define CN_DEBUG_CONFIG_DEFAULT \
    { \
        .enable_boundary_check = true,         /* 启用边界检查 */ \
        .enable_leak_detection = true,         /* 启用泄漏检测 */ \
        .enable_double_free_check = true,      /* 启用双重释放检测 */ \
        .enable_memory_corruption_check = true,/* 启用内存损坏检测 */ \
        .enable_statistics = true,             /* 启用统计信息 */ \
        .zero_on_alloc = false,                /* 不清零内存 */ \
        .fill_on_alloc = true,                 /* 分配时填充模式 */ \
        .fill_on_free = true,                  /* 释放时填充模式 */ \
        .track_allocations = true,             /* 跟踪分配信息 */ \
        .guard_zone_size = 16,                 /* 16字节保护区域 */ \
        .max_tracked_allocations = 4096,       /* 最大跟踪4096个分配 */ \
        .max_single_alloc = 0,                 /* 无单次分配限制 */ \
        .max_total_alloc = 0                   /* 无总分配限制 */ \
    }

// ============================================================================
// 调试分配器统计信息
// ============================================================================

/**
 * @brief 调试分配器统计信息
 * 
 * 记录调试分配器的运行统计信息，用于性能监控和调试。
 */
typedef struct Stru_CN_DebugStats_t
{
    size_t total_allocated;           /**< 总分配字节数 */
    size_t total_freed;               /**< 总释放字节数 */
    size_t current_usage;             /**< 当前使用字节数 */
    size_t peak_usage;                /**< 峰值使用字节数 */
    size_t allocation_count;          /**< 分配次数 */
    size_t free_count;                /**< 释放次数 */
    size_t realloc_count;             /**< 重新分配次数 */
    size_t calloc_count;              /**< 清零分配次数 */
    size_t allocation_failures;       /**< 分配失败次数 */
    size_t memory_overhead;           /**< 内存开销（元数据、保护区域等） */
    size_t boundary_violations;       /**< 边界违规次数 */
    size_t double_free_detections;    /**< 双重释放检测次数 */
    size_t corruption_detections;     /**< 内存损坏检测次数 */
    size_t leak_count;                /**< 泄漏次数（当前未释放的分配） */
} Stru_CN_DebugStats_t;

// ============================================================================
// 分配信息结构体（用于跟踪和调试）
// ============================================================================

/**
 * @brief 调试分配信息记录
 * 
 * 用于跟踪每次分配的详细信息，便于调试和泄漏检测。
 */
typedef struct Stru_CN_DebugAllocationInfo_t
{
    void* address;                    /**< 分配的内存地址（用户可见地址） */
    void* real_address;               /**< 实际分配的内存地址（包含元数据） */
    size_t size;                      /**< 用户请求的大小 */
    size_t real_size;                 /**< 实际分配的大小（包括元数据和保护区域） */
    const char* file;                 /**< 分配所在的文件 */
    int line;                         /**< 分配所在的行号 */
    uint64_t timestamp;               /**< 分配时间戳 */
    const char* purpose;              /**< 分配目的（可选） */
    uint64_t allocation_id;           /**< 分配ID（唯一标识符） */
    bool is_freed;                    /**< 是否已释放 */
} Stru_CN_DebugAllocationInfo_t;

// ============================================================================
// 调试分配器句柄
// ============================================================================

/**
 * @brief 调试分配器句柄
 * 
 * 不透明指针，代表一个调试分配器实例。
 */
typedef struct Stru_CN_DebugAllocator_t Stru_CN_DebugAllocator_t;

// ============================================================================
// 调试分配器管理接口
// ============================================================================

/**
 * @brief 创建调试分配器
 * 
 * @param config 调试分配器配置
 * @return 调试分配器句柄，失败返回NULL
 */
Stru_CN_DebugAllocator_t* CN_debug_create(const Stru_CN_DebugConfig_t* config);

/**
 * @brief 销毁调试分配器
 * 
 * @param allocator 调试分配器句柄
 */
void CN_debug_destroy(Stru_CN_DebugAllocator_t* allocator);

/**
 * @brief 从调试分配器分配内存
 * 
 * @param allocator 调试分配器句柄
 * @param size 要分配的内存大小（字节）
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 * @param purpose 分配目的描述（可选，可为NULL）
 * @return 分配的内存指针，失败返回NULL
 */
void* CN_debug_alloc(Stru_CN_DebugAllocator_t* allocator, size_t size, 
                     const char* file, int line, const char* purpose);

/**
 * @brief 从调试分配器分配对齐内存
 * 
 * @param allocator 调试分配器句柄
 * @param size 要分配的内存大小（字节）
 * @param alignment 对齐要求（必须是2的幂）
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 * @param purpose 分配目的描述（可选，可为NULL）
 * @return 分配的内存指针，失败返回NULL
 */
void* CN_debug_alloc_aligned(Stru_CN_DebugAllocator_t* allocator, size_t size, 
                             size_t alignment, const char* file, int line, 
                             const char* purpose);

/**
 * @brief 释放调试分配器分配的内存
 * 
 * @param allocator 调试分配器句柄
 * @param ptr 要释放的内存指针
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 */
void CN_debug_free(Stru_CN_DebugAllocator_t* allocator, void* ptr, 
                   const char* file, int line);

/**
 * @brief 重新分配调试分配器分配的内存
 * 
 * @param allocator 调试分配器句柄
 * @param ptr 原内存指针
 * @param new_size 新的内存大小
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 * @param purpose 重新分配目的描述（可选，可为NULL）
 * @return 重新分配的内存指针，失败返回NULL
 */
void* CN_debug_realloc(Stru_CN_DebugAllocator_t* allocator, void* ptr, 
                       size_t new_size, const char* file, int line, 
                       const char* purpose);

/**
 * @brief 分配并清零内存
 * 
 * @param allocator 调试分配器句柄
 * @param count 元素数量
 * @param size 每个元素大小
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 * @param purpose 分配目的描述（可选，可为NULL）
 * @return 分配的内存指针，失败返回NULL
 */
void* CN_debug_calloc(Stru_CN_DebugAllocator_t* allocator, size_t count, 
                      size_t size, const char* file, int line, 
                      const char* purpose);

// ============================================================================
// 调试分配器查询接口
// ============================================================================

/**
 * @brief 获取调试分配器配置
 * 
 * @param allocator 调试分配器句柄
 * @param config 输出配置信息
 * @return 获取成功返回true，失败返回false
 */
bool CN_debug_get_config(Stru_CN_DebugAllocator_t* allocator, 
                         Stru_CN_DebugConfig_t* config);

/**
 * @brief 获取调试分配器统计信息
 * 
 * @param allocator 调试分配器句柄
 * @param stats 输出统计信息
 * @return 获取成功返回true，失败返回false
 */
bool CN_debug_get_stats(Stru_CN_DebugAllocator_t* allocator, 
                        Stru_CN_DebugStats_t* stats);

/**
 * @brief 重置调试分配器统计信息
 * 
 * @param allocator 调试分配器句柄
 */
void CN_debug_reset_stats(Stru_CN_DebugAllocator_t* allocator);

/**
 * @brief 检查内存泄漏
 * 
 * @param allocator 调试分配器句柄
 * @return 如果检测到内存泄漏返回true，否则返回false
 */
bool CN_debug_check_leaks(Stru_CN_DebugAllocator_t* allocator);

/**
 * @brief 获取当前内存使用量
 * 
 * @param allocator 调试分配器句柄
 * @return 当前使用字节数
 */
size_t CN_debug_current_usage(Stru_CN_DebugAllocator_t* allocator);

/**
 * @brief 获取峰值内存使用量
 * 
 * @param allocator 调试分配器句柄
 * @return 峰值使用字节数
 */
size_t CN_debug_peak_usage(Stru_CN_DebugAllocator_t* allocator);

// ============================================================================
// 调试分配器调试接口
// ============================================================================

/**
 * @brief 验证堆完整性
 * 
 * @param allocator 调试分配器句柄
 * @return 如果堆完整返回true，否则返回false
 */
bool CN_debug_validate_heap(Stru_CN_DebugAllocator_t* allocator);

/**
 * @brief 验证内存块完整性
 * 
 * @param allocator 调试分配器句柄
 * @param ptr 要验证的内存指针
 * @return 如果内存块完整返回true，否则返回false
 */
bool CN_debug_validate_block(Stru_CN_DebugAllocator_t* allocator, void* ptr);

/**
 * @brief 转储调试分配器状态到标准输出
 * 
 * @param allocator 调试分配器句柄
 */
void CN_debug_dump(Stru_CN_DebugAllocator_t* allocator);

/**
 * @brief 转储所有未释放的分配信息
 * 
 * @param allocator 调试分配器句柄
 */
void CN_debug_dump_leaks(Stru_CN_DebugAllocator_t* allocator);

/**
 * @brief 转储所有分配信息（包括已释放的）
 * 
 * @param allocator 调试分配器句柄
 */
void CN_debug_dump_all_allocations(Stru_CN_DebugAllocator_t* allocator);

/**
 * @brief 设置调试分配器调试回调
 * 
 * @param allocator 调试分配器句柄
 * @param callback 调试回调函数
 * @param user_data 用户数据
 */
typedef void (*CN_DebugCallback_t)(const char* message, void* user_data);
void CN_debug_set_debug_callback(Stru_CN_DebugAllocator_t* allocator, 
                                 CN_DebugCallback_t callback, 
                                 void* user_data);

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 获取默认调试分配器（全局单例）
 * 
 * 注意：这个函数返回全局默认调试分配器，适合大多数调试场景。
 * 如果需要自定义配置，请使用CN_debug_create创建独立的分配器。
 * 
 * @return 默认调试分配器句柄
 */
Stru_CN_DebugAllocator_t* CN_debug_get_default(void);

/**
 * @brief 释放默认调试分配器
 * 
 * 在程序退出时调用，确保所有资源被正确释放并报告泄漏。
 */
void CN_debug_release_default(void);

#ifdef __cplusplus
}
#endif

#endif // CN_DEBUG_ALLOCATOR_H
