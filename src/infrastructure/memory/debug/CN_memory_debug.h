/**
 * @file CN_memory_debug.h
 * @brief 内存调试器接口
 * 
 * 提供内存调试功能，包括内存泄漏检测、越界检查等。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_DEBUG_H
#define CN_MEMORY_DEBUG_H

#include "../CN_memory_interface.h"

/**
 * @brief 创建内存调试器
 * 
 * 创建内存调试器实例，用于调试内存分配和释放。
 * 调试器提供以下功能：
 * - 内存泄漏检测
 * - 越界检查
 * - 分配跟踪
 * - 使用报告
 * 
 * @param allocator 要调试的分配器
 * @return Stru_MemoryDebugInterface_t* 调试器接口指针
 */
Stru_MemoryDebugInterface_t* F_create_memory_debugger(
    Stru_MemoryAllocatorInterface_t* allocator);

/**
 * @brief 销毁内存调试器
 * 
 * 销毁内存调试器实例，释放相关资源。
 * 
 * @param debugger 要销毁的调试器
 */
void F_destroy_memory_debugger(Stru_MemoryDebugInterface_t* debugger);

/**
 * @brief 启用详细调试输出
 * 
 * 启用或禁用详细的调试输出。
 * 当启用时，调试器会输出详细的分配和释放信息。
 * 
 * @param debugger 调试器
 * @param enable 是否启用详细输出
 */
void F_enable_verbose_debugging(Stru_MemoryDebugInterface_t* debugger,
                               bool enable);

/**
 * @brief 设置调试输出回调
 * 
 * 设置调试输出回调函数，用于自定义调试信息的输出。
 * 如果callback为NULL，使用默认输出（printf）。
 * 
 * @param debugger 调试器
 * @param callback 输出回调函数
 */
void F_set_debug_output_callback(Stru_MemoryDebugInterface_t* debugger,
                                void (*callback)(const char* message));

#endif // CN_MEMORY_DEBUG_H
