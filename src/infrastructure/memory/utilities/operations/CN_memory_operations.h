/**
 * @file CN_memory_operations.h
 * @brief 内存操作函数接口
 * 
 * 提供高效、安全的内存操作函数，包括复制、移动、比较和设置。
 * 遵循单一职责原则，每个函数专注于特定的内存操作任务。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_OPERATIONS_H
#define CN_MEMORY_OPERATIONS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 内存操作接口结构体
 * 
 * 定义了内存操作的核心功能，支持高效的内存复制、移动、比较和设置。
 */
typedef struct Stru_MemoryOperationsInterface_t Stru_MemoryOperationsInterface_t;

/**
 * @brief 内存操作接口
 */
struct Stru_MemoryOperationsInterface_t
{
    /**
     * @brief 复制内存
     * 
     * 从源内存复制指定字节数到目标内存。
     * 如果dest和src重叠，行为未定义。
     * 
     * @param dest 目标内存
     * @param src 源内存
     * @param n 要复制的字节数
     * @return void* 目标内存指针
     */
    void* (*copy)(void* dest, const void* src, size_t n);
    
    /**
     * @brief 移动内存
     * 
     * 从源内存移动指定字节数到目标内存。
     * 正确处理重叠的内存区域。
     * 
     * @param dest 目标内存
     * @param src 源内存
     * @param n 要移动的字节数
     * @return void* 目标内存指针
     */
    void* (*move)(void* dest, const void* src, size_t n);
    
    /**
     * @brief 比较内存
     * 
     * 比较两个内存块的内容。
     * 返回0表示相等，负值表示ptr1 < ptr2，正值表示ptr1 > ptr2。
     * 
     * @param ptr1 第一个内存块
     * @param ptr2 第二个内存块
     * @param n 要比较的字节数
     * @return int 比较结果
     */
    int (*compare)(const void* ptr1, const void* ptr2, size_t n);
    
    /**
     * @brief 设置内存值
     * 
     * 将内存块的每个字节设置为指定值。
     * 
     * @param dest 目标内存
     * @param value 要设置的值
     * @param n 要设置的字节数
     * @return void* 目标内存指针
     */
    void* (*set)(void* dest, int value, size_t n);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

/**
 * @brief 创建内存操作接口实例
 * 
 * 创建默认的内存操作接口实例，使用标准C库实现。
 * 
 * @return Stru_MemoryOperationsInterface_t* 内存操作接口指针
 */
Stru_MemoryOperationsInterface_t* F_create_memory_operations(void);

/**
 * @brief 获取全局内存操作接口实例
 * 
 * 返回全局的内存操作接口单例实例。
 * 如果尚未创建，则创建新的实例。
 * 
 * @return Stru_MemoryOperationsInterface_t* 全局内存操作接口实例
 */
Stru_MemoryOperationsInterface_t* F_get_global_memory_operations(void);

/**
 * @brief 销毁内存操作接口实例
 * 
 * 释放内存操作接口实例占用的资源。
 * 
 * @param operations 要销毁的内存操作接口
 */
void F_destroy_memory_operations(Stru_MemoryOperationsInterface_t* operations);

#ifdef __cplusplus
}
#endif

#endif // CN_MEMORY_OPERATIONS_H
