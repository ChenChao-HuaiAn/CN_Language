/******************************************************************************
 * 文件名: CN_arena_internal.h
 * 功能: CN_Language区域分配器内部数据结构
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建区域分配器内部数据结构
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_ARENA_INTERNAL_H
#define CN_ARENA_INTERNAL_H

#include "CN_arena_allocator.h"

// ============================================================================
// 区域分配器内部数据结构
// ============================================================================

/**
 * @brief 区域分配器内部结构
 * 
 * 注意：这是内部实现细节，不应在模块外部直接访问。
 */
struct Stru_CN_ArenaAllocator_t
{
    // 配置信息
    Stru_CN_ArenaConfig_t config;
    
    // 内存管理
    void* memory;                   /**< 当前内存块指针 */
    size_t current_offset;          /**< 当前偏移量（字节） */
    size_t current_block_size;      /**< 当前块大小（字节） */
    size_t total_allocated_size;    /**< 总分配大小（所有块总和） */
    
    // 线程安全
    void* lock;                     /**< 锁对象（如果启用线程安全） */
    
    // 统计信息
    Stru_CN_ArenaStats_t stats;     /**< 统计信息 */
    
    // 调试支持
    CN_ArenaDebugCallback_t debug_callback;  /**< 调试回调函数 */
    void* debug_user_data;                   /**< 调试用户数据 */
};

#endif // CN_ARENA_INTERNAL_H
