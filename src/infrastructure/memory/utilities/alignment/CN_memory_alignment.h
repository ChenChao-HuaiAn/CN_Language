/**
 * @file CN_memory_alignment.h
 * @brief 内存对齐工具接口
 * 
 * 提供内存对齐相关的工具函数，包括对齐计算、对齐检查和辅助函数。
 * 支持多种对齐要求，提高内存访问性能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_ALIGNMENT_H
#define CN_MEMORY_ALIGNMENT_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 内存对齐接口结构体
 * 
 * 定义了内存对齐的核心功能，支持对齐计算、检查和辅助函数。
 */
typedef struct Stru_MemoryAlignmentInterface_t Stru_MemoryAlignmentInterface_t;

/**
 * @brief 内存对齐接口
 */
struct Stru_MemoryAlignmentInterface_t
{
    /**
     * @brief 向上对齐
     * 
     * 计算向上对齐到指定对齐要求的大小。
     * 
     * @param size 原始大小
     * @param alignment 对齐要求（必须是2的幂）
     * @return size_t 对齐后的大小
     */
    size_t (*align_up)(size_t size, size_t alignment);
    
    /**
     * @brief 向下对齐
     * 
     * 计算向下对齐到指定对齐要求的大小。
     * 
     * @param size 原始大小
     * @param alignment 对齐要求（必须是2的幂）
     * @return size_t 对齐后的大小
     */
    size_t (*align_down)(size_t size, size_t alignment);
    
    /**
     * @brief 检查对齐状态
     * 
     * 检查指针是否满足指定的对齐要求。
     * 
     * @param ptr 内存指针
     * @param alignment 对齐要求（必须是2的幂）
     * @return bool 满足对齐要求返回true，否则返回false
     */
    bool (*is_aligned)(const void* ptr, size_t alignment);
    
    /**
     * @brief 对齐分配辅助函数
     * 
     * 辅助函数，简化对齐内存分配。
     * 注意：调用者负责释放返回的内存。
     * 
     * @param size 要分配的大小
     * @param alignment 对齐要求（必须是2的幂）
     * @return void* 对齐的内存指针，失败返回NULL
     */
    void* (*aligned_alloc_helper)(size_t size, size_t alignment);
    
    /**
     * @brief 计算填充大小
     * 
     * 计算需要多少填充才能满足对齐要求。
     * 
     * @param size 原始大小
     * @param alignment 对齐要求（必须是2的幂）
     * @return size_t 需要的填充大小
     */
    size_t (*padding_size)(size_t size, size_t alignment);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

/**
 * @brief 创建内存对齐接口实例
 * 
 * 创建默认的内存对齐接口实例。
 * 
 * @return Stru_MemoryAlignmentInterface_t* 内存对齐接口指针
 */
Stru_MemoryAlignmentInterface_t* F_create_memory_alignment(void);

/**
 * @brief 获取全局内存对齐接口实例
 * 
 * 返回全局的内存对齐接口单例实例。
 * 如果尚未创建，则创建新的实例。
 * 
 * @return Stru_MemoryAlignmentInterface_t* 全局内存对齐接口实例
 */
Stru_MemoryAlignmentInterface_t* F_get_global_memory_alignment(void);

/**
 * @brief 销毁内存对齐接口实例
 * 
 * 释放内存对齐接口实例占用的资源。
 * 
 * @param alignment 要销毁的内存对齐接口
 */
void F_destroy_memory_alignment(Stru_MemoryAlignmentInterface_t* alignment);

#ifdef __cplusplus
}
#endif

#endif // CN_MEMORY_ALIGNMENT_H
