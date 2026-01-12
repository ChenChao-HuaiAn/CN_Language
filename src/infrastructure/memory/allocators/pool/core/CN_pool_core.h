/**
 * @file CN_pool_core.h
 * @brief 对象池分配器核心数据结构
 * 
 * 定义对象池分配器的核心数据结构和内部函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-12
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_POOL_CORE_H
#define CN_POOL_CORE_H

#include <stddef.h>
#include <stdbool.h>

// 前向声明
typedef struct Stru_MemoryAllocatorInterface_t Stru_MemoryAllocatorInterface_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 对象池分配器私有数据结构
 */
typedef struct Stru_PoolAllocatorData_t
{
    size_t object_size;                         ///< 每个对象的大小（字节）
    size_t aligned_object_size;                 ///< 对齐后的对象大小
    size_t pool_size;                           ///< 池中对象的总数
    size_t allocated_objects;                   ///< 已分配的对象数
    size_t allocation_failures;                 ///< 分配失败次数
    void* memory_block;                         ///< 内存块指针
    void** free_list;                           ///< 空闲对象链表
    size_t free_count;                          ///< 空闲对象数量
    Stru_MemoryAllocatorInterface_t* parent_allocator; ///< 父分配器
} Stru_PoolAllocatorData_t;

/**
 * @brief 检查指针是否在对象池范围内
 * 
 * 检查给定的指针是否在对象池的内存块范围内。
 * 
 * @param pool_data 对象池数据
 * @param ptr 要检查的指针
 * @return bool 如果在范围内返回true，否则返回false
 */
bool F_is_pointer_in_pool_range(Stru_PoolAllocatorData_t* pool_data, void* ptr);

/**
 * @brief 检查指针是否对齐到对象大小
 * 
 * 检查给定的指针是否对齐到对象池的对象大小。
 * 
 * @param pool_data 对象池数据
 * @param ptr 要检查的指针
 * @return bool 如果对齐返回true，否则返回false
 */
bool F_is_pointer_aligned_to_object(Stru_PoolAllocatorData_t* pool_data, void* ptr);

#ifdef __cplusplus
}
#endif

#endif // CN_POOL_CORE_H
