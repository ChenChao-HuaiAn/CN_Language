/**
 * @file CN_debug_allocator.h
 * @brief 调试内存分配器接口
 * 
 * 提供调试功能的内存分配器，包括内存泄漏检测、缓冲区溢出检查等。
 * 遵循项目命名规范，使用Stru_前缀和F_前缀函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_DEBUG_ALLOCATOR_H
#define CN_DEBUG_ALLOCATOR_H

#include "../CN_memory_interface.h"

/**
 * @brief 调试分配器私有数据结构
 */
typedef struct Stru_DebugAllocatorData_t Stru_DebugAllocatorData_t;

/**
 * @brief 创建调试内存分配器
 * 
 * 创建具有调试功能的内存分配器，包括：
 * - 内存泄漏检测
 * - 缓冲区溢出检查
 * - 分配统计信息
 * - 魔术字验证
 * 
 * @param parent_allocator 父分配器（可为NULL，使用系统分配器）
 * @return Stru_MemoryAllocatorInterface_t* 调试分配器接口指针
 */
Stru_MemoryAllocatorInterface_t* F_create_debug_allocator(
    Stru_MemoryAllocatorInterface_t* parent_allocator);

/**
 * @brief 获取调试分配器的统计信息
 * 
 * 获取调试分配器的详细统计信息，包括：
 * - 总分配字节数
 * - 总释放字节数
 * - 当前使用字节数
 * - 分配次数
 * 
 * @param allocator 调试分配器
 * @param total_allocated 输出参数：总分配字节数
 * @param total_freed 输出参数：总释放字节数
 * @param current_usage 输出参数：当前使用字节数
 * @param allocation_count 输出参数：分配次数
 */
void F_get_debug_allocator_statistics(
    Stru_MemoryAllocatorInterface_t* allocator,
    size_t* total_allocated, size_t* total_freed,
    size_t* current_usage, size_t* allocation_count);

/**
 * @brief 检查调试分配器中的内存泄漏
 * 
 * 检查调试分配器中是否存在未释放的内存。
 * 返回泄漏的字节数，0表示无泄漏。
 * 
 * @param allocator 调试分配器
 * @return size_t 泄漏的字节数
 */
size_t F_check_debug_allocator_leaks(Stru_MemoryAllocatorInterface_t* allocator);

#endif // CN_DEBUG_ALLOCATOR_H
