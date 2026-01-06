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
 * @brief 内存调试接口
 * 
 * 提供内存调试功能，包括内存泄漏检测、越界检查、使用分析等。
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
// 标准内存管理函数（遵循项目命名规范）
// ============================================================================

/**
 * @brief 创建标准系统内存分配器
 * 
 * @return Stru_MemoryAllocatorInterface_t* 分配器接口指针
 */
Stru_MemoryAllocatorInterface_t* F_create_system_allocator(void);

/**
 * @brief 创建调试内存分配器
 * 
 * @param parent_allocator 父分配器（可为NULL，使用系统分配器）
 * @return Stru_MemoryAllocatorInterface_t* 调试分配器接口指针
 */
Stru_MemoryAllocatorInterface_t* F_create_debug_allocator(
    Stru_MemoryAllocatorInterface_t* parent_allocator);

/**
 * @brief 创建内存调试器
 * 
 * @param allocator 要调试的分配器
 * @return Stru_MemoryDebugInterface_t* 调试器接口指针
 */
Stru_MemoryDebugInterface_t* F_create_memory_debugger(
    Stru_MemoryAllocatorInterface_t* allocator);

/**
 * @brief 创建完整的内存管理上下文
 * 
 * @param use_debug_allocator 是否使用调试分配器
 * @return Stru_MemoryContext_t* 内存上下文指针
 */
Stru_MemoryContext_t* F_create_memory_context(bool use_debug_allocator);

/**
 * @brief 销毁内存管理上下文
 * 
 * @param context 要销毁的上下文
 */
void F_destroy_memory_context(Stru_MemoryContext_t* context);

// ============================================================================
// 便捷包装函数（cn_前缀，供项目其他部分使用）
// ============================================================================

/**
 * @brief 分配内存（使用默认分配器）
 * 
 * @param size 要分配的字节数
 * @return void* 分配的内存指针
 */
void* cn_malloc(size_t size);

/**
 * @brief 分配对齐内存
 * 
 * @param size 要分配的字节数
 * @param alignment 对齐要求
 * @return void* 分配的内存指针
 */
void* cn_malloc_aligned(size_t size, size_t alignment);

/**
 * @brief 重新分配内存
 * 
 * @param ptr 原内存指针
 * @param new_size 新的字节数
 * @return void* 重新分配的内存指针
 */
void* cn_realloc(void* ptr, size_t new_size);

/**
 * @brief 释放内存
 * 
 * @param ptr 要释放的内存指针
 */
void cn_free(void* ptr);

/**
 * @brief 复制内存
 * 
 * @param dest 目标内存
 * @param src 源内存
 * @param size 要复制的字节数
 * @return void* 目标内存指针
 */
void* cn_memcpy(void* dest, const void* src, size_t size);

/**
 * @brief 设置内存值
 * 
 * @param ptr 内存指针
 * @param value 要设置的值
 * @param size 要设置的字节数
 * @return void* 内存指针
 */
void* cn_memset(void* ptr, int value, size_t size);

/**
 * @brief 比较内存
 * 
 * @param ptr1 第一个内存块
 * @param ptr2 第二个内存块
 * @param size 要比较的字节数
 * @return int 比较结果（0表示相等）
 */
int cn_memcmp(const void* ptr1, const void* ptr2, size_t size);

#endif // CN_MEMORY_INTERFACE_H
