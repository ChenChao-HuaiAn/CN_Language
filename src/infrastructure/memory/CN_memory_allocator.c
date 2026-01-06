/**
 * @file CN_memory_allocator.c
 * @brief 内存分配器实现
 * 
 * 实现了系统内存分配器和调试内存分配器，遵循单一职责原则，
 * 每个分配器独立实现，通过抽象接口提供服务。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_memory_interface.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// ============================================================================
// 系统分配器实现
// ============================================================================

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

// ============================================================================
// 调试分配器实现
// ============================================================================

/**
 * @brief 调试分配器私有数据
 */
typedef struct Stru_DebugAllocatorData_t
{
    Stru_MemoryAllocatorInterface_t* parent_allocator;  ///< 父分配器
    size_t total_allocated;     ///< 总分配字节数
    size_t total_freed;         ///< 总释放字节数
    size_t current_usage;       ///< 当前使用字节数
    size_t allocation_count;    ///< 分配次数
} Stru_DebugAllocatorData_t;

/**
 * @brief 调试分配器分配函数
 */
static void* debug_allocate(Stru_MemoryAllocatorInterface_t* allocator,
                           size_t size, size_t alignment)
{
    Stru_DebugAllocatorData_t* data = (Stru_DebugAllocatorData_t*)allocator->private_data;
    
    if (data == NULL || data->parent_allocator == NULL)
    {
        return NULL;
    }
    
    // 分配额外空间用于调试信息
    size_t total_size = size + sizeof(size_t) * 2;  // 大小 + 魔术字
    void* ptr = data->parent_allocator->allocate(data->parent_allocator, total_size, alignment);
    
    if (ptr != NULL)
    {
        // 存储分配大小和魔术字
        size_t* size_ptr = (size_t*)ptr;
        *size_ptr = size;
        
        size_t* magic_ptr = (size_t*)((uint8_t*)ptr + sizeof(size_t));
        *magic_ptr = 0xDEADBEEF;  // 魔术字
        
        // 更新统计信息
        data->total_allocated += size;
        data->current_usage += size;
        data->allocation_count++;
        
        // 返回用户可用空间
        return (uint8_t*)ptr + sizeof(size_t) * 2;
    }
    
    return NULL;
}

/**
 * @brief 调试分配器重新分配函数
 */
static void* debug_reallocate(Stru_MemoryAllocatorInterface_t* allocator,
                             void* ptr, size_t new_size)
{
    Stru_DebugAllocatorData_t* data = (Stru_DebugAllocatorData_t*)allocator->private_data;
    
    if (data == NULL || data->parent_allocator == NULL)
    {
        return NULL;
    }
    
    if (ptr == NULL)
    {
        return debug_allocate(allocator, new_size, 0);
    }
    
    // 获取原分配信息
    void* real_ptr = (uint8_t*)ptr - sizeof(size_t) * 2;
    size_t* old_size_ptr = (size_t*)real_ptr;
    size_t old_size = *old_size_ptr;
    
    // 重新分配
    size_t total_new_size = new_size + sizeof(size_t) * 2;
    void* new_real_ptr = data->parent_allocator->reallocate(
        data->parent_allocator, real_ptr, total_new_size);
    
    if (new_real_ptr != NULL)
    {
        // 更新大小和魔术字
        size_t* new_size_ptr = (size_t*)new_real_ptr;
        *new_size_ptr = new_size;
        
        size_t* magic_ptr = (size_t*)((uint8_t*)new_real_ptr + sizeof(size_t));
        *magic_ptr = 0xDEADBEEF;
        
        // 更新统计信息
        data->total_allocated += (new_size - old_size);
        data->current_usage += (new_size - old_size);
        
        // 返回用户可用空间
        return (uint8_t*)new_real_ptr + sizeof(size_t) * 2;
    }
    
    return NULL;
}

/**
 * @brief 调试分配器释放函数
 */
static void debug_deallocate(Stru_MemoryAllocatorInterface_t* allocator,
                            void* ptr)
{
    if (ptr == NULL)
    {
        return;
    }
    
    Stru_DebugAllocatorData_t* data = (Stru_DebugAllocatorData_t*)allocator->private_data;
    
    if (data == NULL || data->parent_allocator == NULL)
    {
        return;
    }
    
    // 获取分配信息
    void* real_ptr = (uint8_t*)ptr - sizeof(size_t) * 2;
    size_t* size_ptr = (size_t*)real_ptr;
    size_t allocated_size = *size_ptr;
    
    // 检查魔术字
    size_t* magic_ptr = (size_t*)((uint8_t*)real_ptr + sizeof(size_t));
    if (*magic_ptr != 0xDEADBEEF)
    {
        // 魔术字损坏，可能发生缓冲区溢出
        // 记录错误或触发断言
    }
    
    // 释放内存
    data->parent_allocator->deallocate(data->parent_allocator, real_ptr);
    
    // 更新统计信息
    data->total_freed += allocated_size;
    data->current_usage -= allocated_size;
}

/**
 * @brief 调试分配器获取统计信息函数
 */
static void debug_get_statistics(Stru_MemoryAllocatorInterface_t* allocator,
                                size_t* total_allocated, size_t* total_freed,
                                size_t* current_usage, size_t* allocation_count)
{
    Stru_DebugAllocatorData_t* data = (Stru_DebugAllocatorData_t*)allocator->private_data;
    
    if (data != NULL)
    {
        if (total_allocated != NULL) *total_allocated = data->total_allocated;
        if (total_freed != NULL) *total_freed = data->total_freed;
        if (current_usage != NULL) *current_usage = data->current_usage;
        if (allocation_count != NULL) *allocation_count = data->allocation_count;
    }
}

/**
 * @brief 调试分配器验证函数
 */
static bool debug_validate(Stru_MemoryAllocatorInterface_t* allocator,
                          void* ptr)
{
    (void)allocator;
    
    if (ptr == NULL)
    {
        return true;
    }
    
    void* real_ptr = (uint8_t*)ptr - sizeof(size_t) * 2;
    size_t* magic_ptr = (size_t*)((uint8_t*)real_ptr + sizeof(size_t));
    
    return *magic_ptr == 0xDEADBEEF;
}

/**
 * @brief 调试分配器清理函数
 */
static void debug_cleanup(Stru_MemoryAllocatorInterface_t* allocator)
{
    Stru_DebugAllocatorData_t* data = (Stru_DebugAllocatorData_t*)allocator->private_data;
    
    if (data != NULL)
    {
        if (data->parent_allocator != NULL)
        {
            data->parent_allocator->cleanup(data->parent_allocator);
        }
        
        free(data);
        allocator->private_data = NULL;
    }
}

/**
 * @brief 创建调试内存分配器
 */
Stru_MemoryAllocatorInterface_t* F_create_debug_allocator(
    Stru_MemoryAllocatorInterface_t* parent_allocator)
{
    // 创建调试分配器实例
    Stru_MemoryAllocatorInterface_t* allocator = 
        (Stru_MemoryAllocatorInterface_t*)malloc(sizeof(Stru_MemoryAllocatorInterface_t));
    
    if (allocator == NULL)
    {
        return NULL;
    }
    
    // 创建私有数据
    Stru_DebugAllocatorData_t* data = 
        (Stru_DebugAllocatorData_t*)malloc(sizeof(Stru_DebugAllocatorData_t));
    
    if (data == NULL)
    {
        free(allocator);
        return NULL;
    }
    
    // 初始化私有数据
    data->parent_allocator = parent_allocator ? parent_allocator : F_create_system_allocator();
    data->total_allocated = 0;
    data->total_freed = 0;
    data->current_usage = 0;
    data->allocation_count = 0;
    
    // 设置接口函数
    allocator->allocate = debug_allocate;
    allocator->reallocate = debug_reallocate;
    allocator->deallocate = debug_deallocate;
    allocator->get_statistics = debug_get_statistics;
    allocator->validate = debug_validate;
    allocator->cleanup = debug_cleanup;
    allocator->private_data = data;
    
    return allocator;
}

// ============================================================================
// 便捷包装函数实现
// ============================================================================

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

// ============================================================================
// 其他接口函数实现
// ============================================================================

Stru_MemoryDebugInterface_t* F_create_memory_debugger(
    Stru_MemoryAllocatorInterface_t* allocator)
{
    // 简化实现：返回NULL，实际项目需要完整实现
    (void)allocator;
    return NULL;
}

Stru_MemoryContext_t* F_create_memory_context(bool use_debug_allocator)
{
    Stru_MemoryContext_t* context = 
        (Stru_MemoryContext_t*)malloc(sizeof(Stru_MemoryContext_t));
    
    if (context == NULL)
    {
        return NULL;
    }
    
    if (use_debug_allocator)
    {
        context->allocator = F_create_debug_allocator(NULL);
    }
    else
    {
        context->allocator = F_create_system_allocator();
    }
    
    context->debugger = NULL;
    context->private_data = NULL;
    
    return context;
}

void F_destroy_memory_context(Stru_MemoryContext_t* context)
{
    if (context != NULL)
    {
        if (context->allocator != NULL)
        {
            context->allocator->cleanup(context->allocator);
        }
        
        free(context);
    }
}
