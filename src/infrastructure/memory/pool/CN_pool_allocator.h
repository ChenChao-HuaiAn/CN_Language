/******************************************************************************
 * 文件名: CN_pool_allocator.h
 * 功能: CN_Language对象池分配器接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建独立的对象池分配器模块
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_POOL_ALLOCATOR_H
#define CN_POOL_ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 对象池配置结构体
// ============================================================================

/**
 * @brief 对象池配置选项
 * 
 * 用于配置对象池的行为和性能参数。
 */
typedef struct Stru_CN_PoolConfig_t
{
    size_t object_size;           /**< 对象大小（字节） */
    size_t initial_capacity;      /**< 初始容量（对象数量） */
    size_t max_capacity;          /**< 最大容量（0表示无限制） */
    bool auto_expand;             /**< 是否自动扩展 */
    size_t expand_increment;      /**< 自动扩展时的增量 */
    bool thread_safe;             /**< 是否线程安全 */
    bool zero_on_alloc;           /**< 分配时是否清零内存 */
    bool zero_on_free;            /**< 释放时是否清零内存 */
} Stru_CN_PoolConfig_t;

/**
 * @brief 默认对象池配置
 * 
 * 提供合理的默认配置值。
 */
#define CN_POOL_CONFIG_DEFAULT \
    { \
        .object_size = 0, \
        .initial_capacity = 64, \
        .max_capacity = 0, \
        .auto_expand = true, \
        .expand_increment = 64, \
        .thread_safe = false, \
        .zero_on_alloc = false, \
        .zero_on_free = false \
    }

// ============================================================================
// 对象池统计信息
// ============================================================================

/**
 * @brief 对象池统计信息
 * 
 * 记录对象池的运行统计信息，用于性能监控和调试。
 */
typedef struct Stru_CN_PoolStats_t
{
    size_t total_objects;         /**< 总对象数量（当前在池中+已分配） */
    size_t free_objects;          /**< 空闲对象数量 */
    size_t allocated_objects;     /**< 已分配对象数量 */
    size_t total_allocations;     /**< 总分配次数 */
    size_t total_deallocations;   /**< 总释放次数 */
    size_t pool_expansions;       /**< 池扩展次数 */
    size_t memory_usage;          /**< 内存使用量（字节） */
    size_t peak_memory_usage;     /**< 峰值内存使用量（字节） */
    size_t allocation_failures;   /**< 分配失败次数 */
} Stru_CN_PoolStats_t;

// ============================================================================
// 对象池句柄
// ============================================================================

/**
 * @brief 对象池句柄
 * 
 * 不透明指针，代表一个对象池实例。
 */
typedef struct Stru_CN_PoolAllocator_t Stru_CN_PoolAllocator_t;

// ============================================================================
// 对象池管理接口
// ============================================================================

/**
 * @brief 创建对象池
 * 
 * @param config 对象池配置
 * @return 对象池句柄，失败返回NULL
 */
Stru_CN_PoolAllocator_t* CN_pool_create(const Stru_CN_PoolConfig_t* config);

/**
 * @brief 销毁对象池
 * 
 * @param pool 对象池句柄
 */
void CN_pool_destroy(Stru_CN_PoolAllocator_t* pool);

/**
 * @brief 从对象池分配对象
 * 
 * @param pool 对象池句柄
 * @return 分配的对象指针，失败返回NULL
 */
void* CN_pool_alloc(Stru_CN_PoolAllocator_t* pool);

/**
 * @brief 释放对象到对象池
 * 
 * @param pool 对象池句柄
 * @param ptr 要释放的对象指针
 * @return 释放成功返回true，失败返回false
 */
bool CN_pool_free(Stru_CN_PoolAllocator_t* pool, void* ptr);

/**
 * @brief 重新分配对象（对象池不支持，提供兼容接口）
 * 
 * @param pool 对象池句柄
 * @param ptr 原对象指针
 * @param new_size 新大小（必须等于对象池对象大小）
 * @return 重新分配的对象指针，失败返回NULL
 */
void* CN_pool_realloc(Stru_CN_PoolAllocator_t* pool, void* ptr, size_t new_size);

// ============================================================================
// 对象池查询接口
// ============================================================================

/**
 * @brief 获取对象池配置
 * 
 * @param pool 对象池句柄
 * @param config 输出配置信息
 * @return 获取成功返回true，失败返回false
 */
bool CN_pool_get_config(Stru_CN_PoolAllocator_t* pool, Stru_CN_PoolConfig_t* config);

/**
 * @brief 获取对象池统计信息
 * 
 * @param pool 对象池句柄
 * @param stats 输出统计信息
 * @return 获取成功返回true，失败返回false
 */
bool CN_pool_get_stats(Stru_CN_PoolAllocator_t* pool, Stru_CN_PoolStats_t* stats);

/**
 * @brief 重置对象池统计信息
 * 
 * @param pool 对象池句柄
 */
void CN_pool_reset_stats(Stru_CN_PoolAllocator_t* pool);

/**
 * @brief 检查对象指针是否属于对象池
 * 
 * @param pool 对象池句柄
 * @param ptr 要检查的指针
 * @return 如果指针属于对象池返回true，否则返回false
 */
bool CN_pool_contains(Stru_CN_PoolAllocator_t* pool, const void* ptr);

/**
 * @brief 获取对象池使用率（0.0到1.0）
 * 
 * @param pool 对象池句柄
 * @return 使用率，失败返回-1.0
 */
float CN_pool_utilization(Stru_CN_PoolAllocator_t* pool);

// ============================================================================
// 对象池批量操作接口
// ============================================================================

/**
 * @brief 预分配多个对象
 * 
 * @param pool 对象池句柄
 * @param count 要预分配的对象数量
 * @return 预分配成功返回true，失败返回false
 */
bool CN_pool_prealloc(Stru_CN_PoolAllocator_t* pool, size_t count);

/**
 * @brief 清空对象池（释放所有空闲对象）
 * 
 * @param pool 对象池句柄
 * @return 清空成功返回true，失败返回false
 */
bool CN_pool_clear(Stru_CN_PoolAllocator_t* pool);

/**
 * @brief 收缩对象池（释放多余内存）
 * 
 * @param pool 对象池句柄
 * @return 收缩成功返回true，失败返回false
 */
bool CN_pool_shrink(Stru_CN_PoolAllocator_t* pool);

// ============================================================================
// 对象池调试接口
// ============================================================================

/**
 * @brief 验证对象池完整性
 * 
 * @param pool 对象池句柄
 * @return 如果对象池完整返回true，否则返回false
 */
bool CN_pool_validate(Stru_CN_PoolAllocator_t* pool);

/**
 * @brief 转储对象池状态到标准输出
 * 
 * @param pool 对象池句柄
 */
void CN_pool_dump(Stru_CN_PoolAllocator_t* pool);

/**
 * @brief 设置对象池调试回调
 * 
 * @param pool 对象池句柄
 * @param callback 调试回调函数
 * @param user_data 用户数据
 */
typedef void (*CN_PoolDebugCallback_t)(const char* message, void* user_data);
void CN_pool_set_debug_callback(Stru_CN_PoolAllocator_t* pool, 
                                CN_PoolDebugCallback_t callback, 
                                void* user_data);

#ifdef __cplusplus
}
#endif

#endif // CN_POOL_ALLOCATOR_H
