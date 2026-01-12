/**
 * @file CN_pool_allocator.h
 * @brief 对象池分配器接口
 * 
 * 提供固定大小对象的高效内存分配器，通过对象池减少内存碎片。
 * 遵循项目命名规范，使用Stru_前缀和F_前缀函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_POOL_ALLOCATOR_H
#define CN_POOL_ALLOCATOR_H

#include "../../CN_memory_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 对象池分配器私有数据结构
 */
typedef struct Stru_PoolAllocatorData_t Stru_PoolAllocatorData_t;

/**
 * @brief 创建对象池分配器
 * 
 * 创建用于分配固定大小对象的对象池分配器。
 * 
 * @param object_size 每个对象的大小（字节）
 * @param pool_size 池中对象的数量
 * @param alignment 内存对齐要求（字节），0表示使用默认对齐
 * @param parent_allocator 父分配器（用于分配池内存）
 * @return Stru_MemoryAllocatorInterface_t* 对象池分配器接口指针
 */
Stru_MemoryAllocatorInterface_t* F_create_pool_allocator(
    size_t object_size, size_t pool_size, size_t alignment,
    Stru_MemoryAllocatorInterface_t* parent_allocator);

/**
 * @brief 获取对象池分配器的统计信息
 * 
 * 获取对象池的详细统计信息，包括：
 * - 总对象数
 * - 已分配对象数
 * - 可用对象数
 * - 分配失败次数
 * 
 * @param allocator 对象池分配器
 * @param total_objects 输出参数：总对象数
 * @param allocated_objects 输出参数：已分配对象数
 * @param available_objects 输出参数：可用对象数
 * @param allocation_failures 输出参数：分配失败次数
 */
void F_get_pool_allocator_statistics(
    Stru_MemoryAllocatorInterface_t* allocator,
    size_t* total_objects, size_t* allocated_objects,
    size_t* available_objects, size_t* allocation_failures);

/**
 * @brief 扩展对象池容量
 * 
 * 扩展对象池的容量，增加可用对象数量。
 * 
 * @param allocator 对象池分配器
 * @param additional_objects 要添加的对象数量
 * @return bool 扩展成功返回true，失败返回false
 */
bool F_expand_pool_allocator(
    Stru_MemoryAllocatorInterface_t* allocator,
    size_t additional_objects);

#ifdef __cplusplus
}
#endif

#endif // CN_POOL_ALLOCATOR_H
