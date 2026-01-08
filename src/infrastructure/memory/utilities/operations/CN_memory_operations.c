/**
 * @file CN_memory_operations.c
 * @brief 内存操作函数实现
 * 
 * 实现了高效、安全的内存操作函数，包括复制、移动、比较和设置。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_memory_operations.h"
#include <string.h>
#include <stdlib.h>

/**
 * @brief 内存操作接口实现结构体
 */
typedef struct
{
    Stru_MemoryOperationsInterface_t interface;  ///< 接口定义
    // 可以添加私有数据字段
} MemoryOperationsImpl;

/**
 * @brief 内存复制实现
 */
static void* memory_copy_impl(void* dest, const void* src, size_t n)
{
    return memcpy(dest, src, n);
}

/**
 * @brief 内存移动实现
 */
static void* memory_move_impl(void* dest, const void* src, size_t n)
{
    return memmove(dest, src, n);
}

/**
 * @brief 内存比较实现
 */
static int memory_compare_impl(const void* ptr1, const void* ptr2, size_t n)
{
    return memcmp(ptr1, ptr2, n);
}

/**
 * @brief 内存设置实现
 */
static void* memory_set_impl(void* dest, int value, size_t n)
{
    return memset(dest, value, n);
}

/**
 * @brief 创建内存操作接口实例
 */
Stru_MemoryOperationsInterface_t* F_create_memory_operations(void)
{
    MemoryOperationsImpl* impl = (MemoryOperationsImpl*)malloc(sizeof(MemoryOperationsImpl));
    if (impl == NULL)
    {
        return NULL;
    }
    
    // 初始化接口函数指针
    impl->interface.copy = memory_copy_impl;
    impl->interface.move = memory_move_impl;
    impl->interface.compare = memory_compare_impl;
    impl->interface.set = memory_set_impl;
    impl->interface.private_data = NULL;
    
    return (Stru_MemoryOperationsInterface_t*)impl;
}

/**
 * @brief 全局内存操作接口实例
 */
static Stru_MemoryOperationsInterface_t* g_global_memory_operations = NULL;

/**
 * @brief 获取全局内存操作接口实例
 */
Stru_MemoryOperationsInterface_t* F_get_global_memory_operations(void)
{
    if (g_global_memory_operations == NULL)
    {
        g_global_memory_operations = F_create_memory_operations();
    }
    return g_global_memory_operations;
}

/**
 * @brief 销毁内存操作接口实例
 */
void F_destroy_memory_operations(Stru_MemoryOperationsInterface_t* operations)
{
    if (operations != NULL)
    {
        free(operations);
        
        // 如果是全局实例，重置指针
        if (operations == g_global_memory_operations)
        {
            g_global_memory_operations = NULL;
        }
    }
}
