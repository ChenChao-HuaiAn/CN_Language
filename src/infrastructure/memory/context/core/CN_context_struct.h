/**
 * @file CN_context_struct.h
 * @brief 内存上下文结构体定义
 * 
 * 定义了内存上下文的核心数据结构。
 * 遵循数据封装原则，结构体定义对外部模块不可见。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_CONTEXT_STRUCT_H
#define CN_CONTEXT_STRUCT_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 内存分配记录
 * 
 * 记录内存分配的信息，用于跟踪和统计。
 */
typedef struct Stru_AllocationRecord_t
{
    void* address;          ///< 分配的内存地址
    size_t size;           ///< 分配的大小
    size_t alignment;      ///< 对齐要求
    uint64_t timestamp;    ///< 分配时间戳
    const char* file;      ///< 分配所在的源文件
    int line;              ///< 分配所在的行号
    struct Stru_AllocationRecord_t* next; ///< 下一个分配记录
} Stru_AllocationRecord_t;

/**
 * @brief 内存上下文结构体
 * 
 * 内存上下文的核心数据结构，包含上下文的所有状态信息。
 * 注意：此结构体定义在内部使用，外部模块应通过接口访问。
 */
struct Stru_MemoryContext_t
{
    // 基本信息
    char* name;                     ///< 上下文名称
    uint64_t id;                    ///< 上下文唯一标识符
    uint64_t creation_time;         ///< 创建时间戳
    
    // 层次结构
    struct Stru_MemoryContext_t* parent;      ///< 父上下文
    struct Stru_MemoryContext_t* first_child; ///< 第一个子上下文
    struct Stru_MemoryContext_t* next_sibling;///< 下一个兄弟上下文
    size_t depth;                   ///< 上下文深度
    
    // 内存管理
    Stru_AllocationRecord_t* allocations;     ///< 分配记录链表
    size_t total_allocated;        ///< 总分配字节数
    size_t peak_allocated;         ///< 峰值分配字节数
    size_t current_usage;          ///< 当前使用字节数
    size_t allocation_count;       ///< 分配次数
    
    // 分配器接口
    void* (*allocate_func)(void* allocator_data, size_t size, size_t alignment);
    void* (*reallocate_func)(void* allocator_data, void* ptr, size_t new_size);
    void (*deallocate_func)(void* allocator_data, void* ptr);
    void* allocator_data;          ///< 分配器私有数据
    
    // 线程本地存储
    bool is_thread_local;          ///< 是否为线程本地上下文
    void* thread_data;             ///< 线程相关数据
    
    // 统计和调试
    bool enable_statistics;        ///< 是否启用统计
    bool enable_debugging;         ///< 是否启用调试
    void* statistics_data;         ///< 统计相关数据
    void* debug_data;              ///< 调试相关数据
    
    // 引用计数
    size_t reference_count;        ///< 引用计数（用于共享上下文）
    
    // 状态标志
    bool is_valid;                 ///< 上下文是否有效
    bool is_destroying;            ///< 是否正在销毁
    bool is_resetting;             ///< 是否正在重置
};

#endif // CN_CONTEXT_STRUCT_H
