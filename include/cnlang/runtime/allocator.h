#ifndef CNLANG_RUNTIME_ALLOCATOR_H
#define CNLANG_RUNTIME_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * CN Language 内存分配器系统
 * 提供多种分配策略、内存池管理和内存泄漏检测功能
 * 
 * 主要特性:
 * - 支持首次适应(First Fit)、最佳适应(Best Fit)、最差适应(Worst Fit)算法
 * - 支持多内存池管理和分池策略
 * - 提供内存泄漏检测和分析功能
 * - 支持内存使用统计和性能监控
 */

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// 分配策略枚举
// =============================================================================
typedef enum {
    CN_ALLOC_STRATEGY_FIRST_FIT,  // 首次适应：使用第一个足够大的块
    CN_ALLOC_STRATEGY_BEST_FIT,   // 最佳适应：使用最小的足够大的块
    CN_ALLOC_STRATEGY_WORST_FIT,  // 最差适应：使用最大的可用块
    CN_ALLOC_STRATEGY_NEXT_FIT,   // 下次适应：从上次分配位置继续查找
} CnAllocStrategy;

// =============================================================================
// 内存块结构
// =============================================================================
typedef struct CnMemBlock {
    void *ptr;                    // 内存块指针
    size_t size;                  // 块大小
    bool is_free;                 // 是否空闲
    struct CnMemBlock *next;      // 下一个块
    struct CnMemBlock *prev;      // 上一个块
    
    // 调试信息
    const char *file;             // 分配位置文件名
    int line;                     // 分配位置行号
    uint64_t alloc_time;          // 分配时间戳
} CnMemBlock;

// =============================================================================
// 内存池结构
// =============================================================================
typedef struct CnMemPool {
    char *name;                   // 内存池名称
    void *base;                   // 内存池基址
    size_t total_size;            // 总大小
    size_t used_size;             // 已使用大小
    size_t free_size;             // 空闲大小
    
    CnMemBlock *first_block;      // 第一个块
    CnMemBlock *last_alloc;       // 上次分配位置(用于Next Fit)
    
    CnAllocStrategy strategy;     // 分配策略
    size_t block_count;           // 块数量
    size_t alloc_count;           // 分配次数
    size_t free_count;            // 释放次数
    size_t merge_count;           // 合并次数
    
    struct CnMemPool *next;       // 下一个内存池
} CnMemPool;

// =============================================================================
// 内存分配器结构
// =============================================================================
typedef struct CnAllocator {
    CnMemPool *pools;             // 内存池链表
    size_t pool_count;            // 内存池数量
    
    // 全局统计
    size_t total_allocated;       // 总分配字节数
    size_t total_freed;           // 总释放字节数
    size_t peak_usage;            // 峰值使用量
    size_t current_usage;         // 当前使用量
    
    // 泄漏检测
    bool leak_detection_enabled;  // 是否启用泄漏检测
    CnMemBlock *leak_list;        // 泄漏块链表
    size_t leak_count;            // 泄漏块数量
    
    // 默认配置
    CnAllocStrategy default_strategy;
    size_t default_pool_size;
} CnAllocator;

// =============================================================================
// 分配器管理接口
// =============================================================================

/**
 * 创建内存分配器
 * @param default_pool_size 默认内存池大小
 * @param strategy 默认分配策略
 * @return 分配器指针，失败返回NULL
 */
CnAllocator *cn_allocator_create(size_t default_pool_size, CnAllocStrategy strategy);

/**
 * 销毁内存分配器
 * @param allocator 分配器指针
 */
void cn_allocator_destroy(CnAllocator *allocator);

/**
 * 设置分配器的默认策略
 * @param allocator 分配器指针
 * @param strategy 分配策略
 */
void cn_allocator_set_strategy(CnAllocator *allocator, CnAllocStrategy strategy);

// =============================================================================
// 内存池管理接口
// =============================================================================

/**
 * 创建内存池
 * @param allocator 分配器指针
 * @param name 内存池名称
 * @param size 内存池大小
 * @param strategy 分配策略
 * @return 内存池指针，失败返回NULL
 */
CnMemPool *cn_pool_create(CnAllocator *allocator, const char *name, 
                          size_t size, CnAllocStrategy strategy);

/**
 * 销毁内存池
 * @param allocator 分配器指针
 * @param pool 内存池指针
 */
void cn_pool_destroy(CnAllocator *allocator, CnMemPool *pool);

/**
 * 根据名称查找内存池
 * @param allocator 分配器指针
 * @param name 内存池名称
 * @return 内存池指针，未找到返回NULL
 */
CnMemPool *cn_pool_find(CnAllocator *allocator, const char *name);

/**
 * 重置内存池（释放所有分配，但保留池本身）
 * @param pool 内存池指针
 */
void cn_pool_reset(CnMemPool *pool);

// =============================================================================
// 内存分配接口
// =============================================================================

/**
 * 从内存池分配内存
 * @param pool 内存池指针
 * @param size 分配大小
 * @param file 文件名（用于调试）
 * @param line 行号（用于调试）
 * @return 分配的内存指针，失败返回NULL
 */
void *cn_pool_alloc_debug(CnMemPool *pool, size_t size, const char *file, int line);

/**
 * 释放内存池中的内存
 * @param pool 内存池指针
 * @param ptr 内存指针
 */
void cn_pool_free(CnMemPool *pool, void *ptr);

/**
 * 从分配器分配内存（自动选择合适的池）
 * @param allocator 分配器指针
 * @param size 分配大小
 * @param file 文件名（用于调试）
 * @param line 行号（用于调试）
 * @return 分配的内存指针，失败返回NULL
 */
void *cn_allocator_alloc_debug(CnAllocator *allocator, size_t size, 
                               const char *file, int line);

/**
 * 释放分配器中的内存
 * @param allocator 分配器指针
 * @param ptr 内存指针
 */
void cn_allocator_free(CnAllocator *allocator, void *ptr);

// 便利宏：自动填充文件名和行号
#define cn_pool_alloc(pool, size) \
    cn_pool_alloc_debug((pool), (size), __FILE__, __LINE__)

#define cn_allocator_alloc(allocator, size) \
    cn_allocator_alloc_debug((allocator), (size), __FILE__, __LINE__)

// =============================================================================
// 内存泄漏检测接口
// =============================================================================

/**
 * 启用/禁用内存泄漏检测
 * @param allocator 分配器指针
 * @param enabled 是否启用
 */
void cn_allocator_set_leak_detection(CnAllocator *allocator, bool enabled);

/**
 * 检查内存泄漏
 * @param allocator 分配器指针
 * @return 泄漏的块数量
 */
size_t cn_allocator_check_leaks(CnAllocator *allocator);

/**
 * 打印内存泄漏报告
 * @param allocator 分配器指针
 */
void cn_allocator_print_leak_report(CnAllocator *allocator);

/**
 * 清除泄漏检测记录（不释放内存）
 * @param allocator 分配器指针
 */
void cn_allocator_clear_leak_records(CnAllocator *allocator);

// =============================================================================
// 统计与诊断接口
// =============================================================================

/**
 * 获取内存池统计信息
 * @param pool 内存池指针
 * @param total_size 输出总大小
 * @param used_size 输出已使用大小
 * @param free_size 输出空闲大小
 * @param block_count 输出块数量
 */
void cn_pool_get_stats(CnMemPool *pool, size_t *total_size, size_t *used_size,
                       size_t *free_size, size_t *block_count);

/**
 * 打印内存池统计信息
 * @param pool 内存池指针
 */
void cn_pool_print_stats(CnMemPool *pool);

/**
 * 获取分配器统计信息
 * @param allocator 分配器指针
 * @param total_allocated 输出总分配字节数
 * @param total_freed 输出总释放字节数
 * @param current_usage 输出当前使用量
 * @param peak_usage 输出峰值使用量
 */
void cn_allocator_get_stats(CnAllocator *allocator, size_t *total_allocated,
                            size_t *total_freed, size_t *current_usage,
                            size_t *peak_usage);

/**
 * 打印分配器统计信息
 * @param allocator 分配器指针
 */
void cn_allocator_print_stats(CnAllocator *allocator);

/**
 * 打印内存池详细信息（包括所有块）
 * @param pool 内存池指针
 */
void cn_pool_dump(CnMemPool *pool);

// =============================================================================
// 内存整理接口
// =============================================================================

/**
 * 合并内存池中的相邻空闲块
 * @param pool 内存池指针
 * @return 合并的块数量
 */
size_t cn_pool_merge_free_blocks(CnMemPool *pool);

/**
 * 内存碎片整理（可选功能）
 * @param pool 内存池指针
 * @return 整理是否成功
 */
bool cn_pool_defragment(CnMemPool *pool);

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_RUNTIME_ALLOCATOR_H */
