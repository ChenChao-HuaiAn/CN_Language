/******************************************************************************
 * 文件名: CN_system_internal.h
 * 功能: CN_Language系统分配器内部实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建系统分配器内部头文件
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_SYSTEM_INTERNAL_H
#define CN_SYSTEM_INTERNAL_H

#include "CN_system_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 内部常量定义
// ============================================================================

/** 最大跟踪的分配数量 */
#define CN_SYSTEM_MAX_TRACKED_ALLOCATIONS 1024

/** 调试填充模式 */
#define CN_SYSTEM_DEBUG_FILL_ALLOC 0xCC
#define CN_SYSTEM_DEBUG_FILL_FREE  0xDD

/** 内存块头部大小（用于跟踪分配大小） */
#define CN_SYSTEM_BLOCK_HEADER_SIZE sizeof(size_t)

/** 对齐内存块头部大小 */
#define CN_SYSTEM_ALIGNED_HEADER_SIZE (sizeof(size_t) + sizeof(size_t))

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief 系统分配器内部状态
 */
typedef struct Stru_CN_SystemAllocator_t
{
    // 配置信息
    Stru_CN_SystemConfig_t config;
    
    // 统计信息
    Stru_CN_SystemStats_t stats;
    
    // 分配跟踪信息
    Stru_CN_AllocationInfo_t* allocations[CN_SYSTEM_MAX_TRACKED_ALLOCATIONS];
    size_t allocation_count;
    
    // 调试回调
    CN_SystemDebugCallback_t debug_callback;
    void* debug_user_data;
    
    // 互斥锁（如果支持线程安全）
    void* mutex;
    
} Stru_CN_SystemAllocator_t;

/**
 * @brief 内存块头部（用于跟踪分配大小）
 */
typedef struct Stru_CN_BlockHeader_t
{
    size_t size;                    /**< 分配的大小（不包括头部） */
    size_t magic;                   /**< 魔术字，用于验证 */
    const char* file;               /**< 分配所在的文件 */
    int line;                       /**< 分配所在的行号 */
    const char* purpose;            /**< 分配目的 */
    uint64_t timestamp;             /**< 分配时间戳 */
} Stru_CN_BlockHeader_t;

/**
 * @brief 对齐内存块头部
 */
typedef struct Stru_CN_AlignedBlockHeader_t
{
    size_t size;                    /**< 分配的大小（不包括头部） */
    size_t alignment;               /**< 对齐要求 */
    size_t magic;                   /**< 魔术字，用于验证 */
    const char* file;               /**< 分配所在的文件 */
    int line;                       /**< 分配所在的行号 */
    const char* purpose;            /**< 分配目的 */
    uint64_t timestamp;             /**< 分配时间戳 */
    void* original_pointer;         /**< 原始分配的指针 */
} Stru_CN_AlignedBlockHeader_t;


// ============================================================================
// 内部辅助函数声明
// ============================================================================

/**
 * @brief 锁定分配器（如果支持线程安全）
 */
void system_lock(Stru_CN_SystemAllocator_t* allocator);

/**
 * @brief 解锁分配器（如果支持线程安全）
 */
void system_unlock(Stru_CN_SystemAllocator_t* allocator);

/**
 * @brief 获取当前时间戳
 */
uint64_t system_get_timestamp(void);

/**
 * @brief 调试输出
 */
void system_debug_output(Stru_CN_SystemAllocator_t* allocator,
                         const char* format, ...);

/**
 * @brief 填充内存（调试用）
 */
void system_fill_memory(void* ptr, size_t size, unsigned char pattern);

/**
 * @brief 验证分配大小
 */
bool system_validate_size(Stru_CN_SystemAllocator_t* allocator,
                          size_t size);

/**
 * @brief 验证内存块头部
 */
bool system_validate_block_header(const Stru_CN_BlockHeader_t* header);

/**
 * @brief 验证对齐内存块头部
 */
bool system_validate_aligned_block_header(
    const Stru_CN_AlignedBlockHeader_t* header);

/**
 * @brief 记录分配信息
 */
bool system_record_allocation(Stru_CN_SystemAllocator_t* allocator,
                              void* address, size_t size,
                              const char* file, int line,
                              const char* purpose);

/**
 * @brief 移除分配记录
 */
bool system_remove_allocation(Stru_CN_SystemAllocator_t* allocator,
                              void* address);

/**
 * @brief 获取分配信息
 */
Stru_CN_AllocationInfo_t* system_find_allocation(
    Stru_CN_SystemAllocator_t* allocator, void* address);

/**
 * @brief 更新统计信息（分配）
 */
void system_update_stats_on_alloc(Stru_CN_SystemAllocator_t* allocator,
                                  size_t size);

/**
 * @brief 更新统计信息（释放）
 */
void system_update_stats_on_free(Stru_CN_SystemAllocator_t* allocator,
                                 size_t size);

// ============================================================================
// 魔术字定义
// ============================================================================

#define CN_SYSTEM_MAGIC_NORMAL     0xDEADBEEF
#define CN_SYSTEM_MAGIC_ALIGNED    0xCAFEBABE

#ifdef __cplusplus
}
#endif

#endif // CN_SYSTEM_INTERNAL_H
