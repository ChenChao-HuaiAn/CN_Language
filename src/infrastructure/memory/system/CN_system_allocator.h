/******************************************************************************
 * 文件名: CN_system_allocator.h
 * 功能: CN_Language系统分配器接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建独立的系统分配器模块
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_SYSTEM_ALLOCATOR_H
#define CN_SYSTEM_ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 系统分配器配置结构体
// ============================================================================

/**
 * @brief 系统分配器配置选项
 * 
 * 用于配置系统分配器的行为和性能参数。
 * 系统分配器是标准库malloc/free/realloc的包装器，
 * 提供额外的统计、调试和安全功能。
 */
typedef struct Stru_CN_SystemConfig_t
{
    bool enable_statistics;       /**< 是否启用统计信息 */
    bool enable_debug;            /**< 是否启用调试模式 */
    bool zero_on_alloc;           /**< 分配时是否清零内存 */
    bool fill_on_alloc;           /**< 分配时是否填充特定模式（调试用） */
    bool fill_on_free;            /**< 释放时是否填充特定模式（调试用） */
    bool track_allocations;       /**< 是否跟踪分配信息（文件、行号） */
    bool detect_leaks;            /**< 是否检测内存泄漏 */
    bool validate_heap;           /**< 是否验证堆完整性 */
    size_t max_single_alloc;      /**< 单次分配最大大小（0表示无限制） */
    size_t max_total_alloc;       /**< 总分配最大大小（0表示无限制） */
} Stru_CN_SystemConfig_t;

/**
 * @brief 默认系统分配器配置
 * 
 * 提供合理的默认配置值。
 */
#define CN_SYSTEM_CONFIG_DEFAULT \
    { \
        .enable_statistics = true,    /* 启用统计 */ \
        .enable_debug = false,        /* 禁用调试 */ \
        .zero_on_alloc = false,       /* 不清零内存 */ \
        .fill_on_alloc = false,       /* 不填充模式 */ \
        .fill_on_free = false,        /* 不填充模式 */ \
        .track_allocations = true,    /* 跟踪分配信息 */ \
        .detect_leaks = true,         /* 检测泄漏 */ \
        .validate_heap = false,       /* 不验证堆 */ \
        .max_single_alloc = 0,        /* 无单次分配限制 */ \
        .max_total_alloc = 0          /* 无总分配限制 */ \
    }

// ============================================================================
// 系统分配器统计信息
// ============================================================================

/**
 * @brief 系统分配器统计信息
 * 
 * 记录系统分配器的运行统计信息，用于性能监控和调试。
 */
typedef struct Stru_CN_SystemStats_t
{
    size_t total_allocated;       /**< 总分配字节数 */
    size_t total_freed;           /**< 总释放字节数 */
    size_t current_usage;         /**< 当前使用字节数 */
    size_t peak_usage;            /**< 峰值使用字节数 */
    size_t allocation_count;      /**< 分配次数 */
    size_t free_count;            /**< 释放次数 */
    size_t realloc_count;         /**< 重新分配次数 */
    size_t calloc_count;          /**< 清零分配次数 */
    size_t allocation_failures;   /**< 分配失败次数 */
    size_t memory_overhead;       /**< 内存开销（元数据等） */
} Stru_CN_SystemStats_t;

// ============================================================================
// 分配信息结构体（用于跟踪）
// ============================================================================

/**
 * @brief 分配信息记录
 * 
 * 用于跟踪每次分配的信息，便于调试和泄漏检测。
 */
typedef struct Stru_CN_AllocationInfo_t
{
    void* address;                /**< 分配的内存地址 */
    size_t size;                  /**< 分配的大小 */
    const char* file;             /**< 分配所在的文件 */
    int line;                     /**< 分配所在的行号 */
    uint64_t timestamp;           /**< 分配时间戳 */
    const char* purpose;          /**< 分配目的（可选） */
} Stru_CN_AllocationInfo_t;

// ============================================================================
// 系统分配器句柄
// ============================================================================

/**
 * @brief 系统分配器句柄
 * 
 * 不透明指针，代表一个系统分配器实例。
 */
typedef struct Stru_CN_SystemAllocator_t Stru_CN_SystemAllocator_t;

// ============================================================================
// 系统分配器管理接口
// ============================================================================

/**
 * @brief 创建系统分配器
 * 
 * @param config 系统分配器配置
 * @return 系统分配器句柄，失败返回NULL
 */
Stru_CN_SystemAllocator_t* CN_system_create(const Stru_CN_SystemConfig_t* config);

/**
 * @brief 销毁系统分配器
 * 
 * @param allocator 系统分配器句柄
 */
void CN_system_destroy(Stru_CN_SystemAllocator_t* allocator);

/**
 * @brief 从系统分配器分配内存
 * 
 * @param allocator 系统分配器句柄
 * @param size 要分配的内存大小（字节）
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 * @param purpose 分配目的描述（可选，可为NULL）
 * @return 分配的内存指针，失败返回NULL
 */
void* CN_system_alloc(Stru_CN_SystemAllocator_t* allocator, size_t size, 
                      const char* file, int line, const char* purpose);

/**
 * @brief 从系统分配器分配对齐内存
 * 
 * @param allocator 系统分配器句柄
 * @param size 要分配的内存大小（字节）
 * @param alignment 对齐要求（必须是2的幂）
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 * @param purpose 分配目的描述（可选，可为NULL）
 * @return 分配的内存指针，失败返回NULL
 */
void* CN_system_alloc_aligned(Stru_CN_SystemAllocator_t* allocator, size_t size, 
                              size_t alignment, const char* file, int line, 
                              const char* purpose);

/**
 * @brief 释放系统分配器分配的内存
 * 
 * @param allocator 系统分配器句柄
 * @param ptr 要释放的内存指针
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 */
void CN_system_free(Stru_CN_SystemAllocator_t* allocator, void* ptr, 
                    const char* file, int line);

/**
 * @brief 重新分配系统分配器分配的内存
 * 
 * @param allocator 系统分配器句柄
 * @param ptr 原内存指针
 * @param new_size 新的内存大小
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 * @param purpose 重新分配目的描述（可选，可为NULL）
 * @return 重新分配的内存指针，失败返回NULL
 */
void* CN_system_realloc(Stru_CN_SystemAllocator_t* allocator, void* ptr, 
                        size_t new_size, const char* file, int line, 
                        const char* purpose);

/**
 * @brief 分配并清零内存
 * 
 * @param allocator 系统分配器句柄
 * @param count 元素数量
 * @param size 每个元素大小
 * @param file 调用文件名（用于调试，可为NULL）
 * @param line 调用行号（用于调试）
 * @param purpose 分配目的描述（可选，可为NULL）
 * @return 分配的内存指针，失败返回NULL
 */
void* CN_system_calloc(Stru_CN_SystemAllocator_t* allocator, size_t count, 
                       size_t size, const char* file, int line, 
                       const char* purpose);

// ============================================================================
// 系统分配器查询接口
// ============================================================================

/**
 * @brief 获取系统分配器配置
 * 
 * @param allocator 系统分配器句柄
 * @param config 输出配置信息
 * @return 获取成功返回true，失败返回false
 */
bool CN_system_get_config(Stru_CN_SystemAllocator_t* allocator, 
                          Stru_CN_SystemConfig_t* config);

/**
 * @brief 获取系统分配器统计信息
 * 
 * @param allocator 系统分配器句柄
 * @param stats 输出统计信息
 * @return 获取成功返回true，失败返回false
 */
bool CN_system_get_stats(Stru_CN_SystemAllocator_t* allocator, 
                         Stru_CN_SystemStats_t* stats);

/**
 * @brief 重置系统分配器统计信息
 * 
 * @param allocator 系统分配器句柄
 */
void CN_system_reset_stats(Stru_CN_SystemAllocator_t* allocator);

/**
 * @brief 检查内存泄漏
 * 
 * @param allocator 系统分配器句柄
 * @return 如果检测到内存泄漏返回true，否则返回false
 */
bool CN_system_check_leaks(Stru_CN_SystemAllocator_t* allocator);

/**
 * @brief 获取当前内存使用量
 * 
 * @param allocator 系统分配器句柄
 * @return 当前使用字节数
 */
size_t CN_system_current_usage(Stru_CN_SystemAllocator_t* allocator);

/**
 * @brief 获取峰值内存使用量
 * 
 * @param allocator 系统分配器句柄
 * @return 峰值使用字节数
 */
size_t CN_system_peak_usage(Stru_CN_SystemAllocator_t* allocator);

// ============================================================================
// 系统分配器调试接口
// ============================================================================

/**
 * @brief 启用/禁用调试模式
 * 
 * @param allocator 系统分配器句柄
 * @param enable 是否启用调试
 */
void CN_system_enable_debug(Stru_CN_SystemAllocator_t* allocator, bool enable);

/**
 * @brief 验证堆完整性
 * 
 * @param allocator 系统分配器句柄
 * @return 如果堆完整返回true，否则返回false
 */
bool CN_system_validate_heap(Stru_CN_SystemAllocator_t* allocator);

/**
 * @brief 转储系统分配器状态到标准输出
 * 
 * @param allocator 系统分配器句柄
 */
void CN_system_dump(Stru_CN_SystemAllocator_t* allocator);

/**
 * @brief 转储所有未释放的分配信息
 * 
 * @param allocator 系统分配器句柄
 */
void CN_system_dump_leaks(Stru_CN_SystemAllocator_t* allocator);

/**
 * @brief 设置系统分配器调试回调
 * 
 * @param allocator 系统分配器句柄
 * @param callback 调试回调函数
 * @param user_data 用户数据
 */
typedef void (*CN_SystemDebugCallback_t)(const char* message, void* user_data);
void CN_system_set_debug_callback(Stru_CN_SystemAllocator_t* allocator, 
                                  CN_SystemDebugCallback_t callback, 
                                  void* user_data);

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 获取默认系统分配器（全局单例）
 * 
 * 注意：这个函数返回全局默认系统分配器，适合大多数使用场景。
 * 如果需要自定义配置，请使用CN_system_create创建独立的分配器。
 * 
 * @return 默认系统分配器句柄
 */
Stru_CN_SystemAllocator_t* CN_system_get_default(void);

/**
 * @brief 释放默认系统分配器
 * 
 * 在程序退出时调用，确保所有资源被正确释放。
 */
void CN_system_release_default(void);

#ifdef __cplusplus
}
#endif

#endif // CN_SYSTEM_ALLOCATOR_H
