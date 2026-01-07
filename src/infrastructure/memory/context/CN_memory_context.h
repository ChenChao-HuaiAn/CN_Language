/**
 * @file CN_memory_context.h
 * @brief 内存上下文管理接口
 * 
 * 提供内存上下文管理功能，简化多个分配器的使用。
 * 内存上下文包含分配器和调试器的完整配置。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_CONTEXT_H
#define CN_MEMORY_CONTEXT_H

#include "../CN_memory_interface.h"

/**
 * @brief 创建完整的内存管理上下文
 * 
 * 创建包含分配器和调试器的内存管理上下文。
 * 如果use_debug_allocator为true，使用调试分配器；
 * 否则使用系统分配器。
 * 
 * @param use_debug_allocator 是否使用调试分配器
 * @return Stru_MemoryContext_t* 内存上下文指针
 */
Stru_MemoryContext_t* F_create_memory_context(bool use_debug_allocator);

/**
 * @brief 创建自定义内存管理上下文
 * 
 * 使用指定的分配器和调试器创建内存管理上下文。
 * 如果allocator为NULL，使用系统分配器。
 * 如果debugger为NULL，不启用调试功能。
 * 
 * @param allocator 自定义分配器
 * @param debugger 自定义调试器
 * @return Stru_MemoryContext_t* 内存上下文指针
 */
Stru_MemoryContext_t* F_create_custom_memory_context(
    Stru_MemoryAllocatorInterface_t* allocator,
    Stru_MemoryDebugInterface_t* debugger);

/**
 * @brief 销毁内存管理上下文
 * 
 * 销毁内存管理上下文，释放所有相关资源。
 * 如果context为NULL，不执行任何操作。
 * 
 * @param context 要销毁的上下文
 */
void F_destroy_memory_context(Stru_MemoryContext_t* context);

/**
 * @brief 从上下文获取分配器
 * 
 * 从内存上下文中获取分配器接口。
 * 如果context为NULL，返回系统分配器。
 * 
 * @param context 内存上下文
 * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针
 */
Stru_MemoryAllocatorInterface_t* F_get_context_allocator(
    Stru_MemoryContext_t* context);

/**
 * @brief 从上下文获取调试器
 * 
 * 从内存上下文中获取调试器接口。
 * 如果context为NULL或未启用调试，返回NULL。
 * 
 * @param context 内存上下文
 * @return Stru_MemoryDebugInterface_t* 调试器接口指针
 */
Stru_MemoryDebugInterface_t* F_get_context_debugger(
    Stru_MemoryContext_t* context);

/**
 * @brief 设置上下文私有数据
 * 
 * 设置内存上下文的私有数据。
 * 私有数据用于存储上下文特定的信息。
 * 
 * @param context 内存上下文
 * @param private_data 私有数据指针
 */
void F_set_context_private_data(Stru_MemoryContext_t* context,
                               void* private_data);

/**
 * @brief 获取上下文私有数据
 * 
 * 获取内存上下文的私有数据。
 * 
 * @param context 内存上下文
 * @return void* 私有数据指针
 */
void* F_get_context_private_data(Stru_MemoryContext_t* context);

#endif // CN_MEMORY_CONTEXT_H
