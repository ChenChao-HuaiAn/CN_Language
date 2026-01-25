#include "cnlang/runtime/allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/*
 * CN Language 内存分配器实现
 * 提供多种分配策略和内存管理功能
 */

// =============================================================================
// 内部辅助函数
// =============================================================================

// 获取时间戳（毫秒）
static uint64_t get_timestamp_ms(void)
{
    return (uint64_t)(clock() * 1000 / CLOCKS_PER_SEC);
}

// 对齐大小到8字节边界
static inline size_t align_size(size_t size)
{
    return (size + 7) & ~7;
}

// 创建内存块描述符
static CnMemBlock *create_block_descriptor(void *ptr, size_t size, bool is_free,
                                           const char *file, int line)
{
    CnMemBlock *block = (CnMemBlock *)malloc(sizeof(CnMemBlock));
    if (!block) {
        return NULL;
    }
    
    block->ptr = ptr;
    block->size = size;
    block->is_free = is_free;
    block->next = NULL;
    block->prev = NULL;
    block->file = file;
    block->line = line;
    block->alloc_time = get_timestamp_ms();
    
    return block;
}

// 释放块描述符
static void free_block_descriptor(CnMemBlock *block)
{
    if (block) {
        free(block);
    }
}

// =============================================================================
// 分配算法实现
// =============================================================================

// 首次适应算法：查找第一个足够大的空闲块
static CnMemBlock *find_block_first_fit(CnMemPool *pool, size_t size)
{
    CnMemBlock *block = pool->first_block;
    
    while (block) {
        if (block->is_free && block->size >= size) {
            return block;
        }
        block = block->next;
    }
    
    return NULL;
}

// 最佳适应算法：查找最小的足够大的空闲块
static CnMemBlock *find_block_best_fit(CnMemPool *pool, size_t size)
{
    CnMemBlock *block = pool->first_block;
    CnMemBlock *best_block = NULL;
    size_t best_size = (size_t)-1;
    
    while (block) {
        if (block->is_free && block->size >= size) {
            if (block->size < best_size) {
                best_block = block;
                best_size = block->size;
            }
        }
        block = block->next;
    }
    
    return best_block;
}

// 最差适应算法：查找最大的空闲块
static CnMemBlock *find_block_worst_fit(CnMemPool *pool, size_t size)
{
    CnMemBlock *block = pool->first_block;
    CnMemBlock *worst_block = NULL;
    size_t worst_size = 0;
    
    while (block) {
        if (block->is_free && block->size >= size) {
            if (block->size > worst_size) {
                worst_block = block;
                worst_size = block->size;
            }
        }
        block = block->next;
    }
    
    return worst_block;
}

// 下次适应算法：从上次分配位置开始查找
static CnMemBlock *find_block_next_fit(CnMemPool *pool, size_t size)
{
    CnMemBlock *start = pool->last_alloc ? pool->last_alloc : pool->first_block;
    CnMemBlock *block = start;
    
    // 从上次位置向后查找
    while (block) {
        if (block->is_free && block->size >= size) {
            return block;
        }
        block = block->next;
    }
    
    // 如果没找到，从头开始查找到上次位置
    block = pool->first_block;
    while (block && block != start) {
        if (block->is_free && block->size >= size) {
            return block;
        }
        block = block->next;
    }
    
    return NULL;
}

// 根据策略查找合适的块
static CnMemBlock *find_block_by_strategy(CnMemPool *pool, size_t size)
{
    switch (pool->strategy) {
        case CN_ALLOC_STRATEGY_FIRST_FIT:
            return find_block_first_fit(pool, size);
        case CN_ALLOC_STRATEGY_BEST_FIT:
            return find_block_best_fit(pool, size);
        case CN_ALLOC_STRATEGY_WORST_FIT:
            return find_block_worst_fit(pool, size);
        case CN_ALLOC_STRATEGY_NEXT_FIT:
            return find_block_next_fit(pool, size);
        default:
            return find_block_first_fit(pool, size);
    }
}

// 分割块：如果块太大，分割成两个块
static void split_block(CnMemPool *pool, CnMemBlock *block, size_t size)
{
    if (block->size >= size + sizeof(CnMemBlock) + 64) {
        // 创建新的空闲块
        void *new_ptr = (char *)block->ptr + size;
        size_t new_size = block->size - size;
        
        CnMemBlock *new_block = create_block_descriptor(new_ptr, new_size, true, NULL, 0);
        if (new_block) {
            // 插入到链表中
            new_block->next = block->next;
            new_block->prev = block;
            
            if (block->next) {
                block->next->prev = new_block;
            }
            block->next = new_block;
            
            // 更新当前块大小
            block->size = size;
            pool->block_count++;
        }
    }
}

// =============================================================================
// 分配器管理接口实现
// =============================================================================

CnAllocator *cn_allocator_create(size_t default_pool_size, CnAllocStrategy strategy)
{
    CnAllocator *allocator = (CnAllocator *)malloc(sizeof(CnAllocator));
    if (!allocator) {
        return NULL;
    }
    
    memset(allocator, 0, sizeof(CnAllocator));
    allocator->default_strategy = strategy;
    allocator->default_pool_size = default_pool_size > 0 ? default_pool_size : (1024 * 1024); // 默认1MB
    allocator->leak_detection_enabled = true;
    
    return allocator;
}

void cn_allocator_destroy(CnAllocator *allocator)
{
    if (!allocator) {
        return;
    }
    
    // 销毁所有内存池
    CnMemPool *pool = allocator->pools;
    while (pool) {
        CnMemPool *next = pool->next;
        cn_pool_destroy(allocator, pool);
        pool = next;
    }
    
    // 清理泄漏记录
    cn_allocator_clear_leak_records(allocator);
    
    free(allocator);
}

void cn_allocator_set_strategy(CnAllocator *allocator, CnAllocStrategy strategy)
{
    if (allocator) {
        allocator->default_strategy = strategy;
    }
}

// =============================================================================
// 内存池管理接口实现
// =============================================================================

CnMemPool *cn_pool_create(CnAllocator *allocator, const char *name, 
                          size_t size, CnAllocStrategy strategy)
{
    if (!allocator || size == 0) {
        return NULL;
    }
    
    // 分配内存池结构
    CnMemPool *pool = (CnMemPool *)malloc(sizeof(CnMemPool));
    if (!pool) {
        return NULL;
    }
    
    // 分配内存池空间
    void *base = malloc(size);
    if (!base) {
        free(pool);
        return NULL;
    }
    
    memset(pool, 0, sizeof(CnMemPool));
    
    // 设置内存池属性
    pool->name = name ? strdup(name) : NULL;
    pool->base = base;
    pool->total_size = size;
    pool->free_size = size;
    pool->used_size = 0;
    pool->strategy = strategy;
    
    // 创建初始空闲块
    CnMemBlock *initial_block = create_block_descriptor(base, size, true, NULL, 0);
    if (!initial_block) {
        free(base);
        if (pool->name) free(pool->name);
        free(pool);
        return NULL;
    }
    
    pool->first_block = initial_block;
    pool->last_alloc = NULL;
    pool->block_count = 1;
    
    // 添加到分配器的池链表
    pool->next = allocator->pools;
    allocator->pools = pool;
    allocator->pool_count++;
    
    return pool;
}

void cn_pool_destroy(CnAllocator *allocator, CnMemPool *pool)
{
    if (!pool) {
        return;
    }
    
    // 从分配器链表中移除
    if (allocator) {
        CnMemPool *prev = NULL;
        CnMemPool *curr = allocator->pools;
        
        while (curr) {
            if (curr == pool) {
                if (prev) {
                    prev->next = curr->next;
                } else {
                    allocator->pools = curr->next;
                }
                allocator->pool_count--;
                break;
            }
            prev = curr;
            curr = curr->next;
        }
    }
    
    // 释放所有块描述符
    CnMemBlock *block = pool->first_block;
    while (block) {
        CnMemBlock *next = block->next;
        free_block_descriptor(block);
        block = next;
    }
    
    // 释放内存池空间
    free(pool->base);
    if (pool->name) {
        free(pool->name);
    }
    free(pool);
}

CnMemPool *cn_pool_find(CnAllocator *allocator, const char *name)
{
    if (!allocator || !name) {
        return NULL;
    }
    
    CnMemPool *pool = allocator->pools;
    while (pool) {
        if (pool->name && strcmp(pool->name, name) == 0) {
            return pool;
        }
        pool = pool->next;
    }
    
    return NULL;
}

void cn_pool_reset(CnMemPool *pool)
{
    if (!pool) {
        return;
    }
    
    // 释放所有块描述符
    CnMemBlock *block = pool->first_block;
    while (block) {
        CnMemBlock *next = block->next;
        free_block_descriptor(block);
        block = next;
    }
    
    // 创建新的初始空闲块
    CnMemBlock *initial_block = create_block_descriptor(pool->base, pool->total_size, true, NULL, 0);
    if (initial_block) {
        pool->first_block = initial_block;
        pool->last_alloc = NULL;
        pool->block_count = 1;
        pool->used_size = 0;
        pool->free_size = pool->total_size;
        pool->alloc_count = 0;
        pool->free_count = 0;
        pool->merge_count = 0;
    }
}

// =============================================================================
// 内存分配接口实现
// =============================================================================

void *cn_pool_alloc_debug(CnMemPool *pool, size_t size, const char *file, int line)
{
    if (!pool || size == 0) {
        return NULL;
    }
    
    size_t aligned_size = align_size(size);
    
    // 查找合适的块
    CnMemBlock *block = find_block_by_strategy(pool, aligned_size);
    if (!block) {
        return NULL;
    }
    
    // 分割块
    split_block(pool, block, aligned_size);
    
    // 标记为已使用
    block->is_free = false;
    block->file = file;
    block->line = line;
    block->alloc_time = get_timestamp_ms();
    
    // 更新统计
    pool->used_size += block->size;
    pool->free_size -= block->size;
    pool->alloc_count++;
    pool->last_alloc = block;
    
    return block->ptr;
}

void cn_pool_free(CnMemPool *pool, void *ptr)
{
    if (!pool || !ptr) {
        return;
    }
    
    // 查找对应的块
    CnMemBlock *block = pool->first_block;
    while (block) {
        if (block->ptr == ptr) {
            if (block->is_free) {
                fprintf(stderr, "警告: 重复释放内存 %p\n", ptr);
                return;
            }
            
            // 标记为空闲
            block->is_free = true;
            block->file = NULL;
            block->line = 0;
            
            // 更新统计
            pool->used_size -= block->size;
            pool->free_size += block->size;
            pool->free_count++;
            
            return;
        }
        block = block->next;
    }
    
    fprintf(stderr, "警告: 释放无效的内存指针 %p\n", ptr);
}

void *cn_allocator_alloc_debug(CnAllocator *allocator, size_t size, 
                               const char *file, int line)
{
    if (!allocator || size == 0) {
        return NULL;
    }
    
    // 尝试从现有池分配
    CnMemPool *pool = allocator->pools;
    while (pool) {
        if (pool->free_size >= size) {
            void *ptr = cn_pool_alloc_debug(pool, size, file, line);
            if (ptr) {
                allocator->total_allocated += size;
                allocator->current_usage += size;
                if (allocator->current_usage > allocator->peak_usage) {
                    allocator->peak_usage = allocator->current_usage;
                }
                
                // 泄漏检测：记录分配
                if (allocator->leak_detection_enabled) {
                    // 简化版本：仅统计计数
                    allocator->leak_count++;
                }
                
                return ptr;
            }
        }
        pool = pool->next;
    }
    
    // 没有合适的池，创建新池
    size_t new_pool_size = (size > allocator->default_pool_size) ? 
                           size * 2 : allocator->default_pool_size;
    
    pool = cn_pool_create(allocator, "auto_pool", new_pool_size, 
                         allocator->default_strategy);
    if (!pool) {
        return NULL;
    }
    
    return cn_allocator_alloc_debug(allocator, size, file, line);
}

void cn_allocator_free(CnAllocator *allocator, void *ptr)
{
    if (!allocator || !ptr) {
        return;
    }
    
    // 在所有池中查找并释放
    CnMemPool *pool = allocator->pools;
    while (pool) {
        CnMemBlock *block = pool->first_block;
        while (block) {
            if (block->ptr == ptr) {
                size_t size = block->size;
                cn_pool_free(pool, ptr);
                
                allocator->total_freed += size;
                allocator->current_usage -= size;
                
                // 泄漏检测：记录释放
                if (allocator->leak_detection_enabled && allocator->leak_count > 0) {
                    allocator->leak_count--;
                }
                
                return;
            }
            block = block->next;
        }
        pool = pool->next;
    }
    
    fprintf(stderr, "警告: 释放无效的内存指针 %p（不属于任何池）\n", ptr);
}

// =============================================================================
// 内存泄漏检测接口实现
// =============================================================================

void cn_allocator_set_leak_detection(CnAllocator *allocator, bool enabled)
{
    if (allocator) {
        allocator->leak_detection_enabled = enabled;
    }
}

size_t cn_allocator_check_leaks(CnAllocator *allocator)
{
    if (!allocator) {
        return 0;
    }
    
    size_t leak_count = 0;
    
    // 遍历所有池，查找未释放的块
    CnMemPool *pool = allocator->pools;
    while (pool) {
        CnMemBlock *block = pool->first_block;
        while (block) {
            if (!block->is_free) {
                leak_count++;
            }
            block = block->next;
        }
        pool = pool->next;
    }
    
    return leak_count;
}

void cn_allocator_print_leak_report(CnAllocator *allocator)
{
    if (!allocator) {
        return;
    }
    
    printf("=== 内存泄漏检测报告 ===\n");
    
    size_t total_leak_size = 0;
    size_t leak_count = 0;
    
    // 遍历所有池
    CnMemPool *pool = allocator->pools;
    while (pool) {
        CnMemBlock *block = pool->first_block;
        while (block) {
            if (!block->is_free) {
                leak_count++;
                total_leak_size += block->size;
                
                printf("泄漏 #%zu:\n", leak_count);
                printf("  地址: %p\n", block->ptr);
                printf("  大小: %zu 字节\n", block->size);
                printf("  分配位置: %s:%d\n", 
                       block->file ? block->file : "未知",
                       block->line);
                printf("  分配时间: %llu ms\n", 
                       (unsigned long long)block->alloc_time);
                printf("  所属池: %s\n\n", 
                       pool->name ? pool->name : "匿名");
            }
            block = block->next;
        }
        pool = pool->next;
    }
    
    printf("总计: %zu 个泄漏, 泄漏 %zu 字节\n", leak_count, total_leak_size);
    printf("=======================\n");
}

void cn_allocator_clear_leak_records(CnAllocator *allocator)
{
    if (!allocator) {
        return;
    }
    
    // 清理泄漏链表
    CnMemBlock *block = allocator->leak_list;
    while (block) {
        CnMemBlock *next = block->next;
        free_block_descriptor(block);
        block = next;
    }
    
    allocator->leak_list = NULL;
    allocator->leak_count = 0;
}

// =============================================================================
// 统计与诊断接口实现
// =============================================================================

void cn_pool_get_stats(CnMemPool *pool, size_t *total_size, size_t *used_size,
                       size_t *free_size, size_t *block_count)
{
    if (!pool) {
        if (total_size) *total_size = 0;
        if (used_size) *used_size = 0;
        if (free_size) *free_size = 0;
        if (block_count) *block_count = 0;
        return;
    }
    
    if (total_size) *total_size = pool->total_size;
    if (used_size) *used_size = pool->used_size;
    if (free_size) *free_size = pool->free_size;
    if (block_count) *block_count = pool->block_count;
}

void cn_pool_print_stats(CnMemPool *pool)
{
    if (!pool) {
        return;
    }
    
    printf("=== 内存池统计信息 ===\n");
    printf("名称: %s\n", pool->name ? pool->name : "匿名");
    printf("总大小: %zu 字节 (%.2f MB)\n", 
           pool->total_size, pool->total_size / (1024.0 * 1024.0));
    printf("已使用: %zu 字节 (%.2f%%)\n", 
           pool->used_size, (pool->used_size * 100.0) / pool->total_size);
    printf("空闲: %zu 字节 (%.2f%%)\n", 
           pool->free_size, (pool->free_size * 100.0) / pool->total_size);
    printf("块数量: %zu\n", pool->block_count);
    printf("分配次数: %zu\n", pool->alloc_count);
    printf("释放次数: %zu\n", pool->free_count);
    printf("合并次数: %zu\n", pool->merge_count);
    
    const char *strategy_names[] = {
        "首次适应", "最佳适应", "最差适应", "下次适应"
    };
    printf("分配策略: %s\n", strategy_names[pool->strategy]);
    printf("=====================\n");
}

void cn_allocator_get_stats(CnAllocator *allocator, size_t *total_allocated,
                            size_t *total_freed, size_t *current_usage,
                            size_t *peak_usage)
{
    if (!allocator) {
        if (total_allocated) *total_allocated = 0;
        if (total_freed) *total_freed = 0;
        if (current_usage) *current_usage = 0;
        if (peak_usage) *peak_usage = 0;
        return;
    }
    
    if (total_allocated) *total_allocated = allocator->total_allocated;
    if (total_freed) *total_freed = allocator->total_freed;
    if (current_usage) *current_usage = allocator->current_usage;
    if (peak_usage) *peak_usage = allocator->peak_usage;
}

void cn_allocator_print_stats(CnAllocator *allocator)
{
    if (!allocator) {
        return;
    }
    
    printf("=== 分配器统计信息 ===\n");
    printf("内存池数量: %zu\n", allocator->pool_count);
    printf("总分配: %zu 字节 (%.2f MB)\n", 
           allocator->total_allocated, 
           allocator->total_allocated / (1024.0 * 1024.0));
    printf("总释放: %zu 字节 (%.2f MB)\n", 
           allocator->total_freed,
           allocator->total_freed / (1024.0 * 1024.0));
    printf("当前使用: %zu 字节 (%.2f MB)\n", 
           allocator->current_usage,
           allocator->current_usage / (1024.0 * 1024.0));
    printf("峰值使用: %zu 字节 (%.2f MB)\n", 
           allocator->peak_usage,
           allocator->peak_usage / (1024.0 * 1024.0));
    printf("泄漏检测: %s\n", 
           allocator->leak_detection_enabled ? "启用" : "禁用");
    
    if (allocator->leak_detection_enabled) {
        size_t leaks = cn_allocator_check_leaks(allocator);
        printf("未释放块: %zu\n", leaks);
    }
    
    printf("=====================\n");
    
    // 打印每个池的信息
    printf("\n");
    CnMemPool *pool = allocator->pools;
    int pool_index = 1;
    while (pool) {
        printf("内存池 #%d:\n", pool_index++);
        cn_pool_print_stats(pool);
        printf("\n");
        pool = pool->next;
    }
}

void cn_pool_dump(CnMemPool *pool)
{
    if (!pool) {
        return;
    }
    
    printf("=== 内存池详细信息 ===\n");
    printf("名称: %s\n", pool->name ? pool->name : "匿名");
    printf("基址: %p\n", pool->base);
    printf("总大小: %zu 字节\n\n", pool->total_size);
    
    printf("块列表:\n");
    CnMemBlock *block = pool->first_block;
    int block_index = 1;
    
    while (block) {
        printf("  块 #%d:\n", block_index++);
        printf("    地址: %p\n", block->ptr);
        printf("    大小: %zu 字节\n", block->size);
        printf("    状态: %s\n", block->is_free ? "空闲" : "已使用");
        
        if (!block->is_free) {
            printf("    分配位置: %s:%d\n", 
                   block->file ? block->file : "未知",
                   block->line);
            printf("    分配时间: %llu ms\n", 
                   (unsigned long long)block->alloc_time);
        }
        
        printf("\n");
        block = block->next;
    }
    
    printf("=====================\n");
}

// =============================================================================
// 内存整理接口实现
// =============================================================================

size_t cn_pool_merge_free_blocks(CnMemPool *pool)
{
    if (!pool) {
        return 0;
    }
    
    size_t merge_count = 0;
    CnMemBlock *block = pool->first_block;
    
    while (block && block->next) {
        // 如果当前块和下一个块都是空闲的，且地址连续，则合并
        if (block->is_free && block->next->is_free) {
            char *block_end = (char *)block->ptr + block->size;
            if (block_end == (char *)block->next->ptr) {
                // 合并块
                CnMemBlock *next = block->next;
                block->size += next->size;
                block->next = next->next;
                
                if (next->next) {
                    next->next->prev = block;
                }
                
                free_block_descriptor(next);
                pool->block_count--;
                merge_count++;
                pool->merge_count++;
                
                // 不移动到下一个块，继续尝试合并
                continue;
            }
        }
        
        block = block->next;
    }
    
    return merge_count;
}

bool cn_pool_defragment(CnMemPool *pool)
{
    if (!pool) {
        return false;
    }
    
    // 简单实现：仅合并相邻空闲块
    // 完整的碎片整理需要移动已分配的内存，较为复杂
    size_t merged = cn_pool_merge_free_blocks(pool);
    
    return merged > 0;
}
