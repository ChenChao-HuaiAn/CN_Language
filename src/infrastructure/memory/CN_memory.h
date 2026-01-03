/******************************************************************************
 * 文件名: CN_memory.h
 * 功能: CN_Language统一内存管理接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 重构为统一内存管理接口，支持多种分配器策略
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_MEMORY_H
#define CN_MEMORY_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 内存分配器接口
 * 
 * 定义统一的内存分配器接口，支持多种分配策略。
 * 遵循接口隔离原则，提供最小化接口。
 */
typedef struct Stru_CN_AllocatorInterface_t
{
    /**
     * @brief 分配内存
     * 
     * @param size 要分配的内存大小（字节）
     * @param file 调用文件名（用于调试）
     * @param line 调用行号（用于调试）
     * @return 分配的内存指针，失败返回NULL
     */
    void* (*allocate)(size_t size, const char* file, int line);
    
    /**
     * @brief 释放内存
     * 
     * @param ptr 要释放的内存指针
     * @param file 调用文件名（用于调试）
     * @param line 调用行号（用于调试）
     */
    void (*deallocate)(void* ptr, const char* file, int line);
    
    /**
     * @brief 重新分配内存
     * 
     * @param ptr 原内存指针
     * @param new_size 新的内存大小
     * @param file 调用文件名（用于调试）
     * @param line 调用行号（用于调试）
     * @return 重新分配的内存指针，失败返回NULL
     */
    void* (*reallocate)(void* ptr, size_t new_size, const char* file, int line);
    
} Stru_CN_AllocatorInterface_t;

/**
 * @brief 分配器类型枚举
 */
typedef enum Eum_CN_AllocatorType_t
{
    Eum_ALLOCATOR_SYSTEM = 0,      /**< 系统分配器（包装malloc/free） */
    Eum_ALLOCATOR_DEBUG = 1,       /**< 调试分配器（边界检查、泄漏检测） */
    Eum_ALLOCATOR_POOL = 2,        /**< 对象池分配器（固定大小对象重用） */
    Eum_ALLOCATOR_ARENA = 3,       /**< 区域分配器（批量分配，统一释放） */
    Eum_ALLOCATOR_PHYSICAL = 4     /**< 物理内存分配器（页框分配，用于操作系统开发） */
} Eum_CN_AllocatorType_t;

// ============================================================================
// 全局分配器管理函数
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
 * @brief 获取当前分配器接口
 * 
 * @return 当前分配器接口指针
 */
const Stru_CN_AllocatorInterface_t* cn_get_allocator(void);

// ============================================================================
// 统一内存分配包装函数
// ============================================================================

/**
 * @brief 分配内存
 * 
 * @param size 要分配的内存大小
 * @return 分配的内存指针，失败返回NULL
 */
void* cn_malloc(size_t size);

/**
 * @brief 释放内存
 * 
 * @param ptr 要释放的内存指针
 */
void cn_free(void* ptr);

/**
 * @brief 重新分配内存
 * 
 * @param ptr 原内存指针
 * @param new_size 新的内存大小
 * @return 重新分配的内存指针，失败返回NULL
 */
void* cn_realloc(void* ptr, size_t new_size);

/**
 * @brief 分配并清零内存
 * 
 * @param count 元素数量
 * @param size 每个元素大小
 * @return 分配的内存指针，失败返回NULL
 */
void* cn_calloc(size_t count, size_t size);

// ============================================================================
// 调试和统计函数
// ============================================================================

/**
 * @brief 启用内存调试
 * 
 * @param enable 是否启用调试
 */
void cn_enable_debug(bool enable);

/**
 * @brief 检查内存泄漏
 * 
 * @return 如果检测到内存泄漏返回true，否则返回false
 */
bool cn_check_leaks(void);

/**
 * @brief 转储内存统计信息
 */
void cn_dump_stats(void);

/**
 * @brief 验证堆完整性
 * 
 * @return 如果堆完整返回true，否则返回false
 */
bool cn_validate_heap(void);

#ifdef __cplusplus
}
#endif

#endif // CN_MEMORY_H
