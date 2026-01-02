/******************************************************************************
 * 文件名: CN_debug_internal.h
 * 功能: CN_Language调试分配器内部实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建调试分配器内部头文件
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_DEBUG_INTERNAL_H
#define CN_DEBUG_INTERNAL_H

#include "CN_debug_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 内部常量定义
// ============================================================================

/** 默认最大跟踪分配数量 */
#ifndef CN_DEBUG_MAX_TRACKED_ALLOCATIONS
#define CN_DEBUG_MAX_TRACKED_ALLOCATIONS 4096
#endif

/** 调试填充模式 */
#define CN_DEBUG_FILL_ALLOC          0xCC  /* 分配时填充模式 */
#define CN_DEBUG_FILL_FREE           0xDD  /* 释放时填充模式 */
#define CN_DEBUG_FILL_GUARD_BEFORE   0xAA  /* 前保护区域填充模式 */
#define CN_DEBUG_FILL_GUARD_AFTER    0xBB  /* 后保护区域填充模式 */

/** 魔术字定义 */
#define CN_DEBUG_MAGIC_NORMAL        0xDEADBEEF
#define CN_DEBUG_MAGIC_ALIGNED       0xCAFEBABE
#define CN_DEBUG_MAGIC_FREED         0xFEE1DEAD  /* 已释放块的魔术字 */

/** 内存块头部大小（用于跟踪分配大小） */
#define CN_DEBUG_BLOCK_HEADER_SIZE   sizeof(Stru_CN_DebugBlockHeader_t)

/** 对齐内存块头部大小 */
#define CN_DEBUG_ALIGNED_HEADER_SIZE sizeof(Stru_CN_DebugAlignedBlockHeader_t)

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief 调试分配器内部状态
 */
typedef struct Stru_CN_DebugAllocator_t
{
    // 配置信息
    Stru_CN_DebugConfig_t config;
    
    // 统计信息
    Stru_CN_DebugStats_t stats;
    
    // 分配跟踪信息
    Stru_CN_DebugAllocationInfo_t* allocations[CN_DEBUG_MAX_TRACKED_ALLOCATIONS];
    size_t allocation_count;
    
    // 分配ID计数器
    uint64_t next_allocation_id;
    
    // 调试回调
    CN_DebugCallback_t debug_callback;
    void* debug_user_data;
    
    // 互斥锁（如果支持线程安全）
    void* mutex;
    
} Stru_CN_DebugAllocator_t;

/**
 * @brief 调试内存块头部（用于跟踪分配大小和保护区域）
 */
typedef struct Stru_CN_DebugBlockHeader_t
{
    size_t size;                    /**< 用户请求的大小（不包括头部和保护区域） */
    size_t real_size;               /**< 实际分配的大小（包括头部和保护区域） */
    size_t magic;                   /**< 魔术字，用于验证 */
    const char* file;               /**< 分配所在的文件 */
    int line;                       /**< 分配所在的行号 */
    const char* purpose;            /**< 分配目的 */
    uint64_t timestamp;             /**< 分配时间戳 */
    uint64_t allocation_id;         /**< 分配ID（唯一标识符） */
    size_t guard_zone_size;         /**< 保护区域大小 */
} Stru_CN_DebugBlockHeader_t;

/**
 * @brief 调试对齐内存块头部
 */
typedef struct Stru_CN_DebugAlignedBlockHeader_t
{
    size_t size;                    /**< 用户请求的大小（不包括头部和保护区域） */
    size_t real_size;               /**< 实际分配的大小（包括头部和保护区域） */
    size_t alignment;               /**< 对齐要求 */
    size_t magic;                   /**< 魔术字，用于验证 */
    const char* file;               /**< 分配所在的文件 */
    int line;                       /**< 分配所在的行号 */
    const char* purpose;            /**< 分配目的 */
    uint64_t timestamp;             /**< 分配时间戳 */
    uint64_t allocation_id;         /**< 分配ID（唯一标识符） */
    size_t guard_zone_size;         /**< 保护区域大小 */
    void* original_pointer;         /**< 原始分配的指针 */
} Stru_CN_DebugAlignedBlockHeader_t;

// ============================================================================
// 内部辅助函数声明
// ============================================================================

/**
 * @brief 锁定分配器（如果支持线程安全）
 */
void debug_lock(Stru_CN_DebugAllocator_t* allocator);

/**
 * @brief 解锁分配器（如果支持线程安全）
 */
void debug_unlock(Stru_CN_DebugAllocator_t* allocator);

/**
 * @brief 获取当前时间戳
 */
uint64_t debug_get_timestamp(void);

/**
 * @brief 调试输出
 */
void debug_output(Stru_CN_DebugAllocator_t* allocator,
                  const char* format, ...);

/**
 * @brief 填充内存（调试用）
 */
void debug_fill_memory(void* ptr, size_t size, unsigned char pattern);

/**
 * @brief 验证分配大小
 */
bool debug_validate_size(Stru_CN_DebugAllocator_t* allocator,
                         size_t size);

/**
 * @brief 验证内存块头部
 */
bool debug_validate_block_header(const Stru_CN_DebugBlockHeader_t* header);

/**
 * @brief 验证对齐内存块头部
 */
bool debug_validate_aligned_block_header(
    const Stru_CN_DebugAlignedBlockHeader_t* header);

/**
 * @brief 记录分配信息
 */
bool debug_record_allocation(Stru_CN_DebugAllocator_t* allocator,
                             void* address, void* real_address,
                             size_t size, size_t real_size,
                             const char* file, int line,
                             const char* purpose, uint64_t allocation_id);

/**
 * @brief 标记分配为已释放
 */
bool debug_mark_allocation_freed(Stru_CN_DebugAllocator_t* allocator,
                                 void* address);

/**
 * @brief 获取分配信息
 */
Stru_CN_DebugAllocationInfo_t* debug_find_allocation(
    Stru_CN_DebugAllocator_t* allocator, void* address);

/**
 * @brief 检查保护区域完整性
 */
bool debug_check_guard_zones(Stru_CN_DebugAllocator_t* allocator,
                             const Stru_CN_DebugBlockHeader_t* header);

/**
 * @brief 检查对齐保护区域完整性
 */
bool debug_check_aligned_guard_zones(Stru_CN_DebugAllocator_t* allocator,
                                     const Stru_CN_DebugAlignedBlockHeader_t* header);

/**
 * @brief 更新统计信息（分配）
 */
void debug_update_stats_on_alloc(Stru_CN_DebugAllocator_t* allocator,
                                 size_t size, size_t real_size);

/**
 * @brief 更新统计信息（释放）
 */
void debug_update_stats_on_free(Stru_CN_DebugAllocator_t* allocator,
                                size_t size);

/**
 * @brief 更新统计信息（边界违规）
 */
void debug_update_stats_on_boundary_violation(Stru_CN_DebugAllocator_t* allocator);

/**
 * @brief 更新统计信息（双重释放）
 */
void debug_update_stats_on_double_free(Stru_CN_DebugAllocator_t* allocator);

/**
 * @brief 更新统计信息（内存损坏）
 */
void debug_update_stats_on_corruption(Stru_CN_DebugAllocator_t* allocator);

/**
 * @brief 计算实际分配大小（包括头部和保护区域）
 */
size_t debug_calculate_real_size(Stru_CN_DebugAllocator_t* allocator,
                                 size_t size);

/**
 * @brief 获取用户指针（从头部指针）
 */
void* debug_get_user_pointer(Stru_CN_DebugBlockHeader_t* header);

/**
 * @brief 获取头部指针（从用户指针）
 */
Stru_CN_DebugBlockHeader_t* debug_get_header_pointer(void* user_ptr);

#ifdef __cplusplus
}
#endif

#endif // CN_DEBUG_INTERNAL_H
