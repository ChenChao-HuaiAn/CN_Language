/**
 * @file CN_context_core.h
 * @brief 内存上下文核心模块头文件
 * 
 * 声明内存上下文核心模块的内部函数和数据结构。
 * 此文件仅供内部使用，不对外暴露。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_CONTEXT_CORE_H
#define CN_CONTEXT_CORE_H

#include "CN_context_interface.h"
#include "CN_context_struct.h"

// ============================================================================
// 全局变量声明（在CN_context_interface.c中定义）
// ============================================================================

/** 默认根上下文 */
extern Stru_MemoryContext_t* g_root_context;

/** 线程本地当前上下文 */
extern __declspec(thread) Stru_MemoryContext_t* g_thread_current_context;

/** 上下文ID计数器 */
extern uint64_t g_next_context_id;

/** 系统初始化标志 */
extern bool g_system_initialized;

// ============================================================================
// 内部函数声明（仅供核心模块使用）
// ============================================================================

/**
 * @brief 初始化内存上下文核心模块
 * 
 * 初始化核心模块的内部状态。
 * 此函数在系统初始化时自动调用。
 * 
 * @return bool 初始化成功返回true，否则返回false
 */
bool F_initialize_context_core(void);

/**
 * @brief 关闭内存上下文核心模块
 * 
 * 清理核心模块的内部状态。
 * 此函数在系统关闭时自动调用。
 */
void F_shutdown_context_core(void);

/**
 * @brief 获取默认根上下文
 * 
 * 返回系统的默认根上下文。
 * 如果根上下文不存在，会自动创建。
 * 
 * @return Stru_MemoryContext_t* 根上下文指针
 */
Stru_MemoryContext_t* F_get_root_context(void);

/**
 * @brief 设置默认根上下文
 * 
 * 设置系统的默认根上下文。
 * 注意：此函数应在系统初始化时调用。
 * 
 * @param root_context 新的根上下文
 */
void F_set_root_context(Stru_MemoryContext_t* root_context);

// ============================================================================
// 分配记录管理函数
// ============================================================================

/**
 * @brief 创建分配记录
 */
Stru_AllocationRecord_t* F_create_allocation_record(
    void* address, size_t size, size_t alignment, const char* file, int line);

/**
 * @brief 释放分配记录
 */
void F_free_allocation_record(Stru_AllocationRecord_t* record);

/**
 * @brief 添加分配记录到上下文
 */
void F_add_allocation_to_context(Stru_MemoryContext_t* context,
                                Stru_AllocationRecord_t* record);

/**
 * @brief 从上下文中移除分配记录
 */
Stru_AllocationRecord_t* F_remove_allocation_from_context(
    Stru_MemoryContext_t* context, void* address);

// ============================================================================
// 上下文管理函数
// ============================================================================

/**
 * @brief 创建新的内存上下文
 */
Stru_MemoryContext_t* F_context_create(const char* name,
                                      Stru_MemoryContext_t* parent);

/**
 * @brief 销毁内存上下文及其所有内存
 */
void F_context_destroy(Stru_MemoryContext_t* context);

/**
 * @brief 重置内存上下文
 */
void F_context_reset(Stru_MemoryContext_t* context);

// ============================================================================
// 上下文操作函数
// ============================================================================

/**
 * @brief 获取当前线程的活动上下文
 */
Stru_MemoryContext_t* F_context_get_current(void);

/**
 * @brief 设置当前线程的活动上下文
 */
void F_context_set_current(Stru_MemoryContext_t* context);

/**
 * @brief 获取上下文的父上下文
 */
Stru_MemoryContext_t* F_context_get_parent(Stru_MemoryContext_t* context);

/**
 * @brief 获取上下文的第一个子上下文
 */
Stru_MemoryContext_t* F_context_get_first_child(Stru_MemoryContext_t* context);

/**
 * @brief 获取上下文的下一个兄弟上下文
 */
Stru_MemoryContext_t* F_context_get_next_sibling(Stru_MemoryContext_t* context);

/**
 * @brief 获取上下文名称
 */
const char* F_context_get_name(Stru_MemoryContext_t* context);

/**
 * @brief 获取上下文唯一标识符
 */
uint64_t F_context_get_id(Stru_MemoryContext_t* context);

/**
 * @brief 检查上下文是否有效
 */
bool F_context_is_valid(Stru_MemoryContext_t* context);

/**
 * @brief 获取上下文创建时间戳
 */
uint64_t F_context_get_creation_time(Stru_MemoryContext_t* context);

/**
 * @brief 获取上下文深度（距离根上下文的层级数）
 */
size_t F_context_get_depth(Stru_MemoryContext_t* context);

// ============================================================================
// 内存分配函数
// ============================================================================

/**
 * @brief 在指定上下文中分配内存
 */
void* F_context_allocate(Stru_MemoryContext_t* context,
                        size_t size, size_t alignment);

/**
 * @brief 在指定上下文中重新分配内存
 */
void* F_context_reallocate(Stru_MemoryContext_t* context,
                          void* ptr, size_t new_size);

/**
 * @brief 释放指定上下文中的内存
 */
void F_context_deallocate(Stru_MemoryContext_t* context, void* ptr);

// ============================================================================
// 内存统计函数
// ============================================================================

/**
 * @brief 获取内存块的实际分配大小
 */
size_t F_context_get_allocated_size(Stru_MemoryContext_t* context,
                                   void* ptr);

/**
 * @brief 获取上下文的总分配内存
 */
size_t F_context_get_total_allocated(Stru_MemoryContext_t* context);

/**
 * @brief 获取上下文的峰值分配内存
 */
size_t F_context_get_peak_allocated(Stru_MemoryContext_t* context);

/**
 * @brief 获取上下文的当前使用内存
 */
size_t F_context_get_current_usage(Stru_MemoryContext_t* context);

/**
 * @brief 获取上下文的分配次数
 */
size_t F_context_get_allocation_count(Stru_MemoryContext_t* context);

/**
 * @brief 启用或禁用上下文统计
 */
void F_context_enable_statistics(Stru_MemoryContext_t* context, bool enable);

/**
 * @brief 检查是否启用了上下文统计
 */
bool F_context_is_statistics_enabled(Stru_MemoryContext_t* context);

/**
 * @brief 启用或禁用上下文调试
 */
void F_context_enable_debugging(Stru_MemoryContext_t* context, bool enable);

/**
 * @brief 检查是否启用了上下文调试
 */
bool F_context_is_debugging_enabled(Stru_MemoryContext_t* context);

/**
 * @brief 获取上下文的引用计数
 */
size_t F_context_get_reference_count(Stru_MemoryContext_t* context);

/**
 * @brief 增加上下文的引用计数
 */
void F_context_increment_reference(Stru_MemoryContext_t* context);

/**
 * @brief 减少上下文的引用计数
 */
void F_context_decrement_reference(Stru_MemoryContext_t* context);

// ============================================================================
// 上下文验证函数
// ============================================================================

/**
 * @brief 验证上下文是否有效
 * 
 * @param context 要验证的上下文
 * @return bool 有效返回true，否则返回false
 */
bool F_validate_context(Stru_MemoryContext_t* context);

/**
 * @brief 验证上下文名称是否有效
 * 
 * @param name 上下文名称
 * @return bool 有效返回true，否则返回false
 */
bool F_validate_context_name(const char* name);

/**
 * @brief 验证内存地址是否属于指定上下文
 * 
 * @param context 目标上下文
 * @param address 内存地址
 * @return bool 属于返回true，否则返回false
 */
bool F_validate_memory_address(Stru_MemoryContext_t* context, void* address);

#endif // CN_CONTEXT_CORE_H
