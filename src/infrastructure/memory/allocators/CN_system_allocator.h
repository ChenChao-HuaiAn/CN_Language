/**
 * @file CN_system_allocator.h
 * @brief 系统内存分配器接口
 * 
 * 提供基于标准C库的系统内存分配器实现。
 * 遵循项目命名规范，使用Stru_前缀和F_前缀函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_SYSTEM_ALLOCATOR_H
#define CN_SYSTEM_ALLOCATOR_H

#include "../CN_memory_interface.h"

/**
 * @brief 创建标准系统内存分配器
 * 
 * 创建使用标准C库malloc/free/realloc函数的系统分配器。
 * 此分配器性能最高，适合生产环境使用。
 * 
 * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针
 */
Stru_MemoryAllocatorInterface_t* F_create_system_allocator(void);

/**
 * @brief 获取全局系统分配器实例
 * 
 * 返回全局的系统分配器单例实例。
 * 如果尚未创建，则创建新的实例。
 * 
 * @return Stru_MemoryAllocatorInterface_t* 全局系统分配器实例
 */
Stru_MemoryAllocatorInterface_t* F_get_global_system_allocator(void);

#endif // CN_SYSTEM_ALLOCATOR_H
