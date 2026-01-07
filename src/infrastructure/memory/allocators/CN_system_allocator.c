/**
 * @file CN_system_allocator.c
 * @brief 系统内存分配器实现
 * 
 * 实现了基于标准C库的系统内存分配器。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_system_allocator.h"
#include <stdlib.h>
#include <stdbool.h>

/**
 * @brief 系统分配器分配函数
 */
static void* system_allocate(Stru_MemoryAllocatorInterface_t* allocator, 
                            size_t size, size_t alignment)
{
    // 忽略对齐参数，使用系统默认对齐
    (void)alignment;
    (void)allocator;
    return malloc(size);
}

/**
 * @brief 系统分配器重新分配函数
 */
static void* system_reallocate(Stru_MemoryAllocatorInterface_t* allocator,
                              void* ptr, size_t new_size)
{
    (void)allocator;
    return realloc(ptr, new_size);
}

/**
 * @brief 系统分配器释放函数
 */
static void system_deallocate(Stru_MemoryAllocatorInterface_t* allocator,
                             void* ptr)
{
    (void)allocator;
    free(ptr);
}

/**
 * @brief 系统分配器获取统计信息函数
 */
static void system_get_statistics(Stru_MemoryAllocatorInterface_t* allocator,
                                 size_t* total_allocated, size_t* total_freed,
                                 size_t* current_usage, size_t* allocation_count)
{
    // 简化实现：系统分配器不跟踪统计信息
    (void)allocator;
    if (total_allocated != NULL) *total_allocated = 0;
    if (total_freed != NULL) *total_freed = 0;
    if (current_usage != NULL) *current_usage = 0;
    if (allocation_count != NULL) *allocation_count = 0;
}

/**
 * @brief 系统分配器验证函数
 */
static bool system_validate(Stru_MemoryAllocatorInterface_t* allocator,
                           void* ptr)
{
    (void)allocator;
    // 简单验证：检查指针是否非NULL
    return ptr != NULL;
}

/**
 * @brief 系统分配器清理函数
 */
static void system_cleanup(Stru_MemoryAllocatorInterface_t* allocator)
{
    // 系统分配器无需清理
    (void)allocator;
}

// 系统分配器接口实例
static Stru_MemoryAllocatorInterface_t g_system_allocator_instance = {
    .allocate = system_allocate,
    .reallocate = system_reallocate,
    .deallocate = system_deallocate,
    .get_statistics = system_get_statistics,
    .validate = system_validate,
    .cleanup = system_cleanup,
    .private_data = NULL
};

/**
 * @brief 创建标准系统内存分配器
 */
Stru_MemoryAllocatorInterface_t* F_create_system_allocator(void)
{
    return &g_system_allocator_instance;
}

/**
 * @brief 获取全局系统分配器实例
 */
Stru_MemoryAllocatorInterface_t* F_get_global_system_allocator(void)
{
    return &g_system_allocator_instance;
}
