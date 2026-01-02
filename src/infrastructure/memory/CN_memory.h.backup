/******************************************************************************
 * 文件名: CN_memory.h
 * 功能: CN_Language内存管理接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，定义统一内存管理接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_MEMORY_H
#define CN_MEMORY_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief 内存分配器类型枚举
 * 
 * 定义了CN_Language支持的不同内存分配策略。
 */
typedef enum Eum_CN_AllocatorType_t
{
    Eum_ALLOCATOR_SYSTEM = 0,      /**< 系统分配器（使用malloc/free） */
    Eum_ALLOCATOR_POOL = 1,        /**< 对象池分配器 */
    Eum_ALLOCATOR_ARENA = 2,       /**< 区域分配器 */
    Eum_ALLOCATOR_DEBUG = 3        /**< 调试分配器（检测内存错误） */
} Eum_CN_AllocatorType_t;

/**
 * @brief 内存分配统计信息
 */
typedef struct Stru_CN_MemoryStats_t
{
    size_t total_allocated;        /**< 总分配字节数 */
    size_t total_freed;            /**< 总释放字节数 */
    size_t current_usage;          /**< 当前使用字节数 */
    size_t peak_usage;             /**< 峰值使用字节数 */
    size_t allocation_count;       /**< 分配次数 */
    size_t free_count;             /**< 释放次数 */
    size_t leak_count;             /**< 内存泄漏次数 */
} Stru_CN_MemoryStats_t;

/**
 * @brief 内存分配器接口结构体
 */
typedef struct Stru_CN_AllocatorInterface_t
{
    // 内存分配函数
    void* (*allocate)(size_t size, const char* purpose);
    void* (*allocate_aligned)(size_t size, size_t alignment, const char* purpose);
    void* (*reallocate)(void* ptr, size_t new_size, const char* purpose);
    
    // 内存释放函数
    void (*deallocate)(void* ptr);
    
    // 统计信息函数
    Stru_CN_MemoryStats_t (*get_stats)(void);
    void (*reset_stats)(void);
    
    // 分配器管理函数
    bool (*initialize)(void);
    void (*shutdown)(void);
} Stru_CN_AllocatorInterface_t;

// ============================================================================
// 公共内存管理API
// ============================================================================

/**
 * @brief 初始化内存管理系统
 * 
 * @param allocator_type 分配器类型
 * @return 初始化成功返回true，失败返回false
 */
bool CN_memory_init(Eum_CN_AllocatorType_t allocator_type);

/**
 * @brief 关闭内存管理系统
 */
void CN_memory_shutdown(void);

/**
 * @brief 分配内存
 * 
 * @param size 要分配的内存大小
 * @param purpose 分配目的描述（用于调试）
 * @return 分配的内存指针，失败返回NULL
 */
void* cn_malloc(size_t size, const char* purpose);

/**
 * @brief 分配对齐内存
 * 
 * @param size 要分配的内存大小
 * @param alignment 对齐要求（必须是2的幂）
 * @param purpose 分配目的描述
 * @return 分配的内存指针，失败返回NULL
 */
void* cn_malloc_aligned(size_t size, size_t alignment, const char* purpose);

/**
 * @brief 分配并清零内存
 * 
 * @param count 元素数量
 * @param size 每个元素大小
 * @param purpose 分配目的描述
 * @return 分配的内存指针，失败返回NULL
 */
void* cn_calloc(size_t count, size_t size, const char* purpose);

/**
 * @brief 重新分配内存
 * 
 * @param ptr 原内存指针
 * @param new_size 新的内存大小
 * @param purpose 重新分配目的描述
 * @return 重新分配的内存指针，失败返回NULL
 */
void* cn_realloc(void* ptr, size_t new_size, const char* purpose);

/**
 * @brief 释放内存
 * 
 * @param ptr 要释放的内存指针
 */
void cn_free(void* ptr);

/**
 * @brief 设置当前分配器
 * 
 * @param allocator_type 分配器类型
 * @return 设置成功返回true，失败返回false
 */
bool cn_set_allocator(Eum_CN_AllocatorType_t allocator_type);

/**
 * @brief 获取当前分配器类型
 * 
 * @return 当前分配器类型
 */
Eum_CN_AllocatorType_t cn_get_allocator_type(void);

/**
 * @brief 获取内存统计信息
 * 
 * @return 内存统计信息
 */
Stru_CN_MemoryStats_t cn_get_memory_stats(void);

/**
 * @brief 重置内存统计信息
 */
void cn_reset_memory_stats(void);

/**
 * @brief 检查内存泄漏
 * 
 * @return 如果检测到内存泄漏返回true，否则返回false
 */
bool cn_check_memory_leaks(void);

/**
 * @brief 转储内存统计信息到控制台
 */
void cn_dump_memory_stats(void);

// ============================================================================
// 内存调试工具
// ============================================================================

/**
 * @brief 启用内存调试
 * 
 * @param track_allocations 是否跟踪每次分配
 * @param fill_pattern 填充模式（用于检测未初始化内存）
 */
void cn_enable_memory_debug(bool track_allocations, unsigned char fill_pattern);

/**
 * @brief 禁用内存调试
 */
void cn_disable_memory_debug(void);

/**
 * @brief 验证堆完整性
 * 
 * @return 如果堆完整返回true，否则返回false
 */
bool cn_validate_heap(void);

/**
 * @brief 设置内存分配失败模拟（用于测试）
 * 
 * @param fail_after 在第几次分配后失败（0表示不模拟失败）
 */
void cn_set_allocation_failure_simulation(size_t fail_after);

#endif // CN_MEMORY_H
