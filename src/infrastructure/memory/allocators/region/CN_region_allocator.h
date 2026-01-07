/**
 * @file CN_region_allocator.h
 * @brief 区域内存分配器接口
 * 
 * 提供区域式内存管理，一次性分配和释放内存区域。
 * 适用于临时内存分配场景，提高内存分配效率。
 * 遵循项目命名规范，使用Stru_前缀和F_前缀函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_REGION_ALLOCATOR_H
#define CN_REGION_ALLOCATOR_H

#include "../../CN_memory_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 区域分配器私有数据结构
 */
typedef struct Stru_RegionAllocatorData_t Stru_RegionAllocatorData_t;

/**
 * @brief 创建区域内存分配器
 * 
 * 创建区域式内存分配器，一次性分配大块内存，
 * 然后从该区域中分配小内存块。
 * 
 * @param region_size 区域大小（字节）
 * @param parent_allocator 父分配器（用于分配区域内存）
 * @return Stru_MemoryAllocatorInterface_t* 区域分配器接口指针
 */
Stru_MemoryAllocatorInterface_t* F_create_region_allocator(
    size_t region_size,
    Stru_MemoryAllocatorInterface_t* parent_allocator);

/**
 * @brief 获取区域分配器的统计信息
 * 
 * 获取区域分配器的详细统计信息，包括：
 * - 区域总大小
 * - 已使用字节数
 * - 可用字节数
 * - 分配次数
 * 
 * @param allocator 区域分配器
 * @param total_size 输出参数：区域总大小
 * @param used_size 输出参数：已使用字节数
 * @param available_size 输出参数：可用字节数
 * @param allocation_count 输出参数：分配次数
 */
void F_get_region_allocator_statistics(
    Stru_MemoryAllocatorInterface_t* allocator,
    size_t* total_size, size_t* used_size,
    size_t* available_size, size_t* allocation_count);

/**
 * @brief 重置区域分配器
 * 
 * 重置区域分配器，释放所有已分配的内存块，
 * 但保留区域内存本身，以便重用。
 * 
 * @param allocator 区域分配器
 */
void F_reset_region_allocator(Stru_MemoryAllocatorInterface_t* allocator);

/**
 * @brief 扩展区域容量
 * 
 * 扩展区域分配器的容量，增加可用内存。
 * 
 * @param allocator 区域分配器
 * @param additional_size 要添加的字节数
 * @return bool 扩展成功返回true，失败返回false
 */
bool F_expand_region_allocator(
    Stru_MemoryAllocatorInterface_t* allocator,
    size_t additional_size);

#ifdef __cplusplus
}
#endif

#endif // CN_REGION_ALLOCATOR_H
