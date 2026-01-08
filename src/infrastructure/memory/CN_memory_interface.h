/**
 * @file CN_memory_interface.h
 * @brief 内存管理抽象接口定义
 * 
 * 本文件定义了CN_Language项目的统一内存管理接口，遵循SOLID设计原则，
 * 提供可扩展、可替换的内存管理策略。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_INTERFACE_H
#define CN_MEMORY_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief 内存分配器抽象接口
 * 
 * 定义了内存管理的基本操作，支持多种分配策略（系统分配器、对象池、区域分配器、调试分配器）。
 * 遵循依赖倒置原则，高层模块通过此接口使用内存服务。
 */
typedef struct Stru_MemoryAllocatorInterface_t Stru_MemoryAllocatorInterface_t;

/**
 * @brief 内存分配器操作接口
 */
struct Stru_MemoryAllocatorInterface_t
{
    /**
     * @brief 分配指定大小的内存块
     * 
     * @param allocator 分配器实例
     * @param size 要分配的字节数
     * @param alignment 内存对齐要求（0表示使用默认对齐）
     * @return void* 分配的内存指针，失败返回NULL
     */
    void* (*allocate)(Stru_MemoryAllocatorInterface_t* allocator, size_t size, size_t alignment);
    
    /**
     * @brief 重新分配内存块
     * 
     * @param allocator 分配器实例
     * @param ptr 原内存指针
     * @param new_size 新的字节数
     * @return void* 重新分配的内存指针，失败返回NULL
     */
    void* (*reallocate)(Stru_MemoryAllocatorInterface_t* allocator, void* ptr, size_t new_size);
    
    /**
     * @brief 释放内存块
     * 
     * @param allocator 分配器实例
     * @param ptr 要释放的内存指针
     */
    void (*deallocate)(Stru_MemoryAllocatorInterface_t* allocator, void* ptr);
    
    /**
     * @brief 获取分配器统计信息
     * 
     * @param allocator 分配器实例
     * @param total_allocated 输出参数：总分配字节数
     * @param total_freed 输出参数：总释放字节数
     * @param current_usage 输出参数：当前使用字节数
     * @param allocation_count 输出参数：分配次数
     */
    void (*get_statistics)(Stru_MemoryAllocatorInterface_t* allocator,
                          size_t* total_allocated, size_t* total_freed, 
                          size_t* current_usage, size_t* allocation_count);
    
    /**
     * @brief 验证内存块完整性
     * 
     * @param allocator 分配器实例
     * @param ptr 要验证的内存指针
     * @return bool 验证通过返回true，否则返回false
     */
    bool (*validate)(Stru_MemoryAllocatorInterface_t* allocator, void* ptr);
    
    /**
     * @brief 清理分配器资源
     * 
     * @param allocator 分配器实例
     */
    void (*cleanup)(Stru_MemoryAllocatorInterface_t* allocator);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

/**
 * @brief 内存调试接口（旧版本，保持向后兼容）
 * 
 * 提供内存调试功能，包括内存泄漏检测、越界检查、使用分析等。
 * 
 * @deprecated 请使用新的Stru_MemoryDebuggerInterface_t接口
 */
typedef struct Stru_MemoryDebugInterface_t
{
    /**
     * @brief 启用内存调试
     * 
     * @param enable_leak_detection 是否启用内存泄漏检测
     * @param enable_bounds_checking 是否启用边界检查
     * @param enable_tracking 是否启用分配跟踪
     */
    void (*enable_debugging)(bool enable_leak_detection, 
                            bool enable_bounds_checking, 
                            bool enable_tracking);
    
    /**
     * @brief 检查内存泄漏
     * 
     * @return size_t 泄漏的字节数，0表示无泄漏
     */
    size_t (*check_leaks)(void);
    
    /**
     * @brief 获取内存使用报告
     * 
     * @param report_buffer 报告缓冲区
     * @param buffer_size 缓冲区大小
     */
    void (*get_usage_report)(char* report_buffer, size_t buffer_size);
    
    /**
     * @brief 设置内存分配失败回调
     * 
     * @param callback 回调函数，当内存分配失败时调用
     */
    void (*set_allocation_failure_callback)(void (*callback)(size_t requested_size));
    
    /**
     * @brief 转储所有活动内存分配
     */
    void (*dump_allocations)(void);
    
} Stru_MemoryDebugInterface_t;

/**
 * @brief 内存调试器抽象接口（新版本）
 * 
 * 定义了内存调试的基本操作，支持多种调试功能：
 * - 内存泄漏检测
 * - 内存错误检测（越界、双重释放、野指针等）
 * - 内存使用分析
 * - 调试工具（内存转储、验证等）
 * 
 * 遵循依赖倒置原则，高层模块通过此接口使用内存调试服务。
 */
typedef struct Stru_MemoryDebuggerInterface_t Stru_MemoryDebuggerInterface_t;

/**
 * @brief 内存管理上下文
 * 
 * 包含内存分配器和调试器的完整上下文。
 */
typedef struct Stru_MemoryContext_t
{
    Stru_MemoryAllocatorInterface_t* allocator;  ///< 内存分配器接口
    Stru_MemoryDebugInterface_t* debugger;       ///< 内存调试器接口
    void* private_data;                          ///< 私有数据
} Stru_MemoryContext_t;

// ============================================================================
// 子模块包含（模块化设计）
// ============================================================================

// 分配器模块 - 系统分配器
#include "allocators/system/CN_system_allocator.h"

// 分配器模块 - 调试分配器
#include "allocators/debug/CN_debug_allocator.h"

// 分配器模块 - 对象池分配器
#include "allocators/pool/CN_pool_allocator.h"

// 分配器模块 - 区域分配器
#include "allocators/region/CN_region_allocator.h"

// 分配器模块 - 分配器工厂
#include "allocators/factory/CN_allocator_factory.h"

// 工具函数模块
#include "utilities/CN_memory_utilities.h"

// 上下文管理模块
#include "context/public/CN_memory_context.h"

// 调试模块
#include "debug/CN_memory_debug.h"

// 调试模块 - 新接口
#include "debug/interface/CN_memory_debug_interface.h"

// ============================================================================
// 向后兼容性（保持现有代码可用）
// ============================================================================

// 注意：所有函数现在在各自的子模块中定义
// 为了向后兼容，我们通过包含子模块头文件来提供它们
// 实际实现在子模块中

#endif // CN_MEMORY_INTERFACE_H
