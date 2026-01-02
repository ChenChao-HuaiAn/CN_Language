/******************************************************************************
 * 文件名: CN_arena_allocator.h
 * 功能: CN_Language区域分配器接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建完整的区域分配器模块
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_ARENA_ALLOCATOR_H
#define CN_ARENA_ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 区域分配器配置结构体
// ============================================================================

/**
 * @brief 区域分配器配置选项
 * 
 * 用于配置区域分配器的行为和性能参数。
 * 区域分配器（Arena Allocator）也称为区域分配器或线性分配器，
 * 特点是在一个连续内存区域中线性分配，统一释放。
 */
typedef struct Stru_CN_ArenaConfig_t
{
    size_t initial_size;          /**< 初始区域大小（字节） */
    size_t max_size;              /**< 最大区域大小（0表示无限制） */
    size_t alignment;             /**< 内存对齐要求（字节） */
    bool auto_expand;             /**< 是否自动扩展区域 */
    size_t expand_increment;      /**< 自动扩展时的增量（字节） */
    bool zero_on_alloc;           /**< 分配时是否清零内存 */
    bool thread_safe;             /**< 是否线程安全 */
    bool enable_statistics;       /**< 是否启用统计信息 */
} Stru_CN_ArenaConfig_t;

/**
 * @brief 默认区域分配器配置
 * 
 * 提供合理的默认配置值。
 */
#define CN_ARENA_CONFIG_DEFAULT \
    { \
        .initial_size = 65536,    /* 64KB初始大小 */ \
        .max_size = 0,            /* 无限制 */ \
        .alignment = 8,           /* 8字节对齐 */ \
        .auto_expand = true,      /* 允许自动扩展 */ \
        .expand_increment = 65536, /* 64KB扩展增量 */ \
        .zero_on_alloc = false,   /* 不清零内存 */ \
        .thread_safe = false,     /* 单线程模式 */ \
        .enable_statistics = true /* 启用统计 */ \
    }

// ============================================================================
// 区域分配器统计信息
// ============================================================================

/**
 * @brief 区域分配器统计信息
 * 
 * 记录区域分配器的运行统计信息，用于性能监控和调试。
 */
typedef struct Stru_CN_ArenaStats_t
{
    size_t total_allocated;       /**< 总分配字节数 */
    size_t total_freed;           /**< 总释放字节数（区域重置时） */
    size_t current_usage;         /**< 当前使用字节数 */
    size_t peak_usage;            /**< 峰值使用字节数 */
    size_t allocation_count;      /**< 分配次数 */
    size_t arena_expansions;      /**< 区域扩展次数 */
    size_t arena_resets;          /**< 区域重置次数 */
    size_t allocation_failures;   /**< 分配失败次数 */
    size_t wasted_space;          /**< 浪费的空间（对齐填充） */
} Stru_CN_ArenaStats_t;

// ============================================================================
// 区域分配器句柄
// ============================================================================

/**
 * @brief 区域分配器句柄
 * 
 * 不透明指针，代表一个区域分配器实例。
 */
typedef struct Stru_CN_ArenaAllocator_t Stru_CN_ArenaAllocator_t;

// ============================================================================
// 区域分配器管理接口
// ============================================================================

/**
 * @brief 创建区域分配器
 * 
 * @param config 区域分配器配置
 * @return 区域分配器句柄，失败返回NULL
 */
Stru_CN_ArenaAllocator_t* CN_arena_create(const Stru_CN_ArenaConfig_t* config);

/**
 * @brief 销毁区域分配器
 * 
 * @param arena 区域分配器句柄
 */
void CN_arena_destroy(Stru_CN_ArenaAllocator_t* arena);

/**
 * @brief 从区域分配器分配内存
 * 
 * @param arena 区域分配器句柄
 * @param size 要分配的内存大小（字节）
 * @return 分配的内存指针，失败返回NULL
 */
void* CN_arena_alloc(Stru_CN_ArenaAllocator_t* arena, size_t size);

/**
 * @brief 从区域分配器分配对齐内存
 * 
 * @param arena 区域分配器句柄
 * @param size 要分配的内存大小（字节）
 * @param alignment 对齐要求（必须是2的幂）
 * @return 分配的内存指针，失败返回NULL
 */
void* CN_arena_alloc_aligned(Stru_CN_ArenaAllocator_t* arena, size_t size, size_t alignment);

/**
 * @brief 重置区域分配器
 * 
 * 释放区域中所有分配的内存，但不释放区域本身。
 * 这是区域分配器的核心特性：批量释放。
 * 
 * @param arena 区域分配器句柄
 * @param zero_memory 是否清零区域内存
 */
void CN_arena_reset(Stru_CN_ArenaAllocator_t* arena, bool zero_memory);

// ============================================================================
// 区域分配器查询接口
// ============================================================================

/**
 * @brief 获取区域分配器配置
 * 
 * @param arena 区域分配器句柄
 * @param config 输出配置信息
 * @return 获取成功返回true，失败返回false
 */
bool CN_arena_get_config(Stru_CN_ArenaAllocator_t* arena, Stru_CN_ArenaConfig_t* config);

/**
 * @brief 获取区域分配器统计信息
 * 
 * @param arena 区域分配器句柄
 * @param stats 输出统计信息
 * @return 获取成功返回true，失败返回false
 */
bool CN_arena_get_stats(Stru_CN_ArenaAllocator_t* arena, Stru_CN_ArenaStats_t* stats);

/**
 * @brief 重置区域分配器统计信息
 * 
 * @param arena 区域分配器句柄
 */
void CN_arena_reset_stats(Stru_CN_ArenaAllocator_t* arena);

/**
 * @brief 获取区域分配器使用率（0.0到1.0）
 * 
 * @param arena 区域分配器句柄
 * @return 使用率，失败返回-1.0
 */
float CN_arena_utilization(Stru_CN_ArenaAllocator_t* arena);

/**
 * @brief 获取区域分配器剩余空间
 * 
 * @param arena 区域分配器句柄
 * @return 剩余空间字节数
 */
size_t CN_arena_remaining(Stru_CN_ArenaAllocator_t* arena);

/**
 * @brief 获取区域分配器总大小
 * 
 * @param arena 区域分配器句柄
 * @return 总大小字节数
 */
size_t CN_arena_total_size(Stru_CN_ArenaAllocator_t* arena);

// ============================================================================
// 区域分配器批量操作接口
// ============================================================================

/**
 * @brief 预分配区域空间
 * 
 * 确保区域有足够的连续空间，避免后续分配失败。
 * 
 * @param arena 区域分配器句柄
 * @param size 要预分配的空间大小
 * @return 预分配成功返回true，失败返回false
 */
bool CN_arena_reserve(Stru_CN_ArenaAllocator_t* arena, size_t size);

/**
 * @brief 扩展区域
 * 
 * 手动扩展区域大小。
 * 
 * @param arena 区域分配器句柄
 * @param additional_size 要增加的大小
 * @return 扩展成功返回true，失败返回false
 */
bool CN_arena_expand(Stru_CN_ArenaAllocator_t* arena, size_t additional_size);

/**
 * @brief 收缩区域
 * 
 * 释放多余内存，减少区域大小。
 * 
 * @param arena 区域分配器句柄
 * @return 收缩成功返回true，失败返回false
 */
bool CN_arena_shrink(Stru_CN_ArenaAllocator_t* arena);

// ============================================================================
// 区域分配器调试接口
// ============================================================================

/**
 * @brief 验证区域分配器完整性
 * 
 * @param arena 区域分配器句柄
 * @return 如果区域分配器完整返回true，否则返回false
 */
bool CN_arena_validate(Stru_CN_ArenaAllocator_t* arena);

/**
 * @brief 转储区域分配器状态到标准输出
 * 
 * @param arena 区域分配器句柄
 */
void CN_arena_dump(Stru_CN_ArenaAllocator_t* arena);

/**
 * @brief 设置区域分配器调试回调
 * 
 * @param arena 区域分配器句柄
 * @param callback 调试回调函数
 * @param user_data 用户数据
 */
typedef void (*CN_ArenaDebugCallback_t)(const char* message, void* user_data);
void CN_arena_set_debug_callback(Stru_CN_ArenaAllocator_t* arena, 
                                 CN_ArenaDebugCallback_t callback, 
                                 void* user_data);

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 计算对齐后的地址
 * 
 * @param address 原始地址
 * @param alignment 对齐要求（必须是2的幂）
 * @return 对齐后的地址
 */
static inline uintptr_t CN_arena_align_address(uintptr_t address, size_t alignment)
{
    return (address + (alignment - 1)) & ~(alignment - 1);
}

/**
 * @brief 计算对齐填充大小
 * 
 * @param address 原始地址
 * @param alignment 对齐要求（必须是2的幂）
 * @return 需要的填充字节数
 */
static inline size_t CN_arena_padding_size(uintptr_t address, size_t alignment)
{
    return CN_arena_align_address(address, alignment) - address;
}

#ifdef __cplusplus
}
#endif

#endif // CN_ARENA_ALLOCATOR_H
