/**
 * @file CN_memory_alignment.c
 * @brief 内存对齐工具实现
 * 
 * 实现了内存对齐相关的工具函数，包括对齐计算、对齐检查和辅助函数。
 * 支持多种对齐要求，提高内存访问性能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_memory_alignment.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief 内存对齐接口实现结构体
 */
typedef struct
{
    Stru_MemoryAlignmentInterface_t interface;  ///< 接口定义
    // 可以添加私有数据字段
} MemoryAlignmentImpl;

/**
 * @brief 检查对齐值是否为2的幂
 */
static bool is_power_of_two(size_t n)
{
    return (n != 0) && ((n & (n - 1)) == 0);
}

/**
 * @brief 向上对齐实现
 */
static size_t align_up_impl(size_t size, size_t alignment)
{
    if (!is_power_of_two(alignment) || alignment == 0)
    {
        // 无效的对齐要求，返回原始大小
        return size;
    }
    
    return (size + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief 向下对齐实现
 */
static size_t align_down_impl(size_t size, size_t alignment)
{
    if (!is_power_of_two(alignment) || alignment == 0)
    {
        // 无效的对齐要求，返回原始大小
        return size;
    }
    
    return size & ~(alignment - 1);
}

/**
 * @brief 检查对齐状态实现
 */
static bool is_aligned_impl(const void* ptr, size_t alignment)
{
    if (!is_power_of_two(alignment) || alignment == 0 || ptr == NULL)
    {
        return false;
    }
    
    return ((uintptr_t)ptr & (alignment - 1)) == 0;
}

/**
 * @brief 对齐分配辅助函数实现
 */
static void* aligned_alloc_helper_impl(size_t size, size_t alignment)
{
    if (!is_power_of_two(alignment) || alignment == 0 || size == 0)
    {
        return NULL;
    }
    
    // 使用标准C11的aligned_alloc函数
    #ifdef __STDC_VERSION__
    #if __STDC_VERSION__ >= 201112L
    return aligned_alloc(alignment, align_up_impl(size, alignment));
    #endif
    #endif
    
    // 回退到posix_memalign
    #ifdef _POSIX_C_SOURCE
    #if _POSIX_C_SOURCE >= 200112L
    void* ptr = NULL;
    if (posix_memalign(&ptr, alignment, align_up_impl(size, alignment)) == 0)
    {
        return ptr;
    }
    #endif
    #endif
    
    // 最后回退到手动对齐分配
    size_t aligned_size = align_up_impl(size, alignment);
    size_t total_size = aligned_size + alignment - 1 + sizeof(void*);
    
    void* original_ptr = malloc(total_size);
    if (original_ptr == NULL)
    {
        return NULL;
    }
    
    void* aligned_ptr = (void*)align_up_impl((uintptr_t)original_ptr + sizeof(void*), alignment);
    
    // 在aligned_ptr之前存储original_ptr以便释放
    ((void**)aligned_ptr)[-1] = original_ptr;
    
    return aligned_ptr;
}

/**
 * @brief 计算填充大小实现
 */
static size_t padding_size_impl(size_t size, size_t alignment)
{
    if (!is_power_of_two(alignment) || alignment == 0)
    {
        return 0;
    }
    
    size_t aligned_size = align_up_impl(size, alignment);
    return aligned_size - size;
}

/**
 * @brief 创建内存对齐接口实例
 */
Stru_MemoryAlignmentInterface_t* F_create_memory_alignment(void)
{
    MemoryAlignmentImpl* impl = (MemoryAlignmentImpl*)malloc(sizeof(MemoryAlignmentImpl));
    if (impl == NULL)
    {
        return NULL;
    }
    
    // 初始化接口函数指针
    impl->interface.align_up = align_up_impl;
    impl->interface.align_down = align_down_impl;
    impl->interface.is_aligned = is_aligned_impl;
    impl->interface.aligned_alloc_helper = aligned_alloc_helper_impl;
    impl->interface.padding_size = padding_size_impl;
    impl->interface.private_data = NULL;
    
    return (Stru_MemoryAlignmentInterface_t*)impl;
}

/**
 * @brief 全局内存对齐接口实例
 */
static Stru_MemoryAlignmentInterface_t* g_global_memory_alignment = NULL;

/**
 * @brief 获取全局内存对齐接口实例
 */
Stru_MemoryAlignmentInterface_t* F_get_global_memory_alignment(void)
{
    if (g_global_memory_alignment == NULL)
    {
        g_global_memory_alignment = F_create_memory_alignment();
    }
    return g_global_memory_alignment;
}

/**
 * @brief 销毁内存对齐接口实例
 */
void F_destroy_memory_alignment(Stru_MemoryAlignmentInterface_t* alignment)
{
    if (alignment != NULL)
    {
        free(alignment);
        
        // 如果是全局实例，重置指针
        if (alignment == g_global_memory_alignment)
        {
            g_global_memory_alignment = NULL;
        }
    }
}
