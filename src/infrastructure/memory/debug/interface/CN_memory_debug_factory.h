/**
 * @file CN_memory_debug_factory.h
 * @brief 内存调试器工厂函数
 * 
 * 本文件提供了创建内存调试器实例的工厂函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_DEBUG_FACTORY_H
#define CN_MEMORY_DEBUG_FACTORY_H

#include "CN_memory_debug_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建默认内存调试器
 * 
 * 创建一个具有默认配置的内存调试器实例。
 * 
 * @return Stru_MemoryDebuggerInterface_t* 调试器接口指针，失败返回NULL
 */
Stru_MemoryDebuggerInterface_t* F_create_default_memory_debugger(void);

/**
 * @brief 创建具有特定配置的内存调试器
 * 
 * 创建一个具有指定配置的内存调试器实例。
 * 
 * @param enable_leak_detection 是否启用内存泄漏检测
 * @param enable_overflow_check 是否启用缓冲区溢出检查
 * @param enable_double_free_check 是否启用双重释放检查
 * @param stack_trace_depth 调用栈跟踪深度（0表示禁用）
 * @return Stru_MemoryDebuggerInterface_t* 调试器接口指针，失败返回NULL
 */
Stru_MemoryDebuggerInterface_t* F_create_configured_memory_debugger(
    bool enable_leak_detection,
    bool enable_overflow_check,
    bool enable_double_free_check,
    size_t stack_trace_depth);

/**
 * @brief 销毁内存调试器
 * 
 * 销毁内存调试器实例，释放相关资源。
 * 
 * @param debugger 要销毁的调试器
 */
void F_destroy_memory_debugger(Stru_MemoryDebuggerInterface_t* debugger);

/**
 * @brief 获取内存调试器接口（向后兼容）
 * 
 * 获取内存调试器接口，用于向后兼容旧代码。
 * 
 * @return Stru_MemoryDebuggerInterface_t* 调试器接口指针
 */
Stru_MemoryDebuggerInterface_t* F_get_memory_debugger_interface(void);

#ifdef __cplusplus
}
#endif

#endif // CN_MEMORY_DEBUG_FACTORY_H
