/**
 * @file CN_memory_utilities.c
 * @brief 内存工具函数实现
 * 
 * 实现了便捷的内存操作函数，供项目其他部分使用。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_memory_utilities.h"
#include "../allocators/CN_system_allocator.h"
#include <string.h>

// 全局默认分配器
static Stru_MemoryAllocatorInterface_t* g_default_allocator = NULL;

/**
 * @brief 获取或创建默认分配器
 */
static Stru_MemoryAllocatorInterface_t* get_default_allocator(void)
{
    if (g_default_allocator == NULL)
    {
        g_default_allocator = F_create_system_allocator();
    }
    return g_default_allocator;
}

void* cn_malloc(size_t size)
{
    Stru_MemoryAllocatorInterface_t* allocator = get_default_allocator();
    return allocator->allocate(allocator, size, 0);
}

void* cn_malloc_aligned(size_t size, size_t alignment)
{
    Stru_MemoryAllocatorInterface_t* allocator = get_default_allocator();
    return allocator->allocate(allocator, size, alignment);
}

void* cn_realloc(void* ptr, size_t new_size)
{
    Stru_MemoryAllocatorInterface_t* allocator = get_default_allocator();
    return allocator->reallocate(allocator, ptr, new_size);
}

void cn_free(void* ptr)
{
    Stru_MemoryAllocatorInterface_t* allocator = get_default_allocator();
    allocator->deallocate(allocator, ptr);
}

void* cn_memcpy(void* dest, const void* src, size_t size)
{
    return memcpy(dest, src, size);
}

void* cn_memset(void* ptr, int value, size_t size)
{
    return memset(ptr, value, size);
}

int cn_memcmp(const void* ptr1, const void* ptr2, size_t size)
{
    return memcmp(ptr1, ptr2, size);
}

void* cn_memmove(void* dest, const void* src, size_t size)
{
    return memmove(dest, src, size);
}

void* cn_calloc(size_t num, size_t size)
{
    size_t total_size = num * size;
    void* ptr = cn_malloc(total_size);
    if (ptr != NULL)
    {
        cn_memset(ptr, 0, total_size);
    }
    return ptr;
}

void cn_set_default_allocator(void* allocator)
{
    if (allocator == NULL)
    {
        g_default_allocator = F_create_system_allocator();
    }
    else
    {
        g_default_allocator = (Stru_MemoryAllocatorInterface_t*)allocator;
    }
}

void* cn_get_default_allocator(void)
{
    return get_default_allocator();
}
