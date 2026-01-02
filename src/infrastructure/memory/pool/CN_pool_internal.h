/******************************************************************************
 * 文件名: CN_pool_internal.h
 * 功能: CN_Language对象池分配器内部接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建内部接口，支持模块拆分
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_POOL_INTERNAL_H
#define CN_POOL_INTERNAL_H

#include "CN_pool_allocator.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 内部数据结构（与CN_pool_core.c共享）
// ============================================================================

/**
 * @brief 对象池块结构
 * 
 * 管理一块连续内存中的多个对象。
 */
typedef struct Stru_CN_PoolBlock_t
{
    struct Stru_CN_PoolBlock_t* next;  /**< 下一个块 */
    void* memory;                      /**< 块内存起始地址 */
    size_t capacity;                   /**< 块容量（对象数量） */
    size_t used_count;                 /**< 已使用对象数量 */
    unsigned char* usage_bitmap;       /**< 使用情况位图 */
} Stru_CN_PoolBlock_t;

/**
 * @brief 对象池分配器内部结构
 */
struct Stru_CN_PoolAllocator_t
{
    // 配置信息
    Stru_CN_PoolConfig_t config;
    
    // 内存块链表
    Stru_CN_PoolBlock_t* first_block;
    Stru_CN_PoolBlock_t* last_block;
    
    // 空闲对象链表（使用嵌入链表技术）
    void* free_list;
    
    // 统计信息
    Stru_CN_PoolStats_t stats;
    
    // 调试支持
    CN_PoolDebugCallback_t debug_callback;
    void* debug_user_data;
    
    // 内部状态
    bool initialized;
    size_t block_count;
};

// ============================================================================
// 内部辅助函数声明
// ============================================================================

/**
 * @brief 调试日志输出
 */
void pool_debug_log(Stru_CN_PoolAllocator_t* pool, const char* format, ...);

/**
 * @brief 在块中分配对象
 */
void* allocate_from_block(Stru_CN_PoolBlock_t* block, size_t object_size);

/**
 * @brief 释放块中的对象
 */
bool free_in_block(Stru_CN_PoolBlock_t* block, void* ptr, size_t object_size);

/**
 * @brief 扩展对象池
 */
bool expand_pool(Stru_CN_PoolAllocator_t* pool);

#ifdef __cplusplus
}
#endif

#endif // CN_POOL_INTERNAL_H
