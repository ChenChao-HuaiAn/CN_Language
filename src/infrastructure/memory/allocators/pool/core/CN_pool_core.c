/**
 * @file CN_pool_core.c
 * @brief 对象池分配器核心实现
 * 
 * 实现了对象池分配器的核心功能，包括指针检查和验证。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-12
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_pool_core.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 检查指针是否在对象池范围内
 * 
 * 检查给定的指针是否在对象池的内存块范围内。
 * 
 * @param pool_data 对象池数据
 * @param ptr 要检查的指针
 * @return bool 如果在范围内返回true，否则返回false
 */
bool F_is_pointer_in_pool_range(Stru_PoolAllocatorData_t* pool_data, void* ptr)
{
    if (pool_data == NULL || pool_data->memory_block == NULL || ptr == NULL)
    {
        return false;
    }
    
    return (ptr >= pool_data->memory_block && 
            ptr < (char*)pool_data->memory_block + pool_data->pool_size * pool_data->aligned_object_size);
}

/**
 * @brief 检查指针是否对齐到对象大小
 * 
 * 检查给定的指针是否对齐到对象池的对象大小。
 * 
 * @param pool_data 对象池数据
 * @param ptr 要检查的指针
 * @return bool 如果对齐返回true，否则返回false
 */
bool F_is_pointer_aligned_to_object(Stru_PoolAllocatorData_t* pool_data, void* ptr)
{
    if (pool_data == NULL || pool_data->memory_block == NULL || ptr == NULL)
    {
        return false;
    }
    
    size_t offset = (char*)ptr - (char*)pool_data->memory_block;
    return (offset % pool_data->aligned_object_size == 0);
}

#ifdef __cplusplus
}
#endif
