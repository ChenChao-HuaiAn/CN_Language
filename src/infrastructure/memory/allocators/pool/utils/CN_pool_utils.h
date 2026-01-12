/**
 * @file CN_pool_utils.h
 * @brief 对象池分配器工具函数接口
 * 
 * 提供对象池分配器的工具函数接口，包括内存对齐计算等。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-12
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_POOL_UTILS_H
#define CN_POOL_UTILS_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 计算对齐后的大小
 * 
 * 计算满足指定对齐要求的内存大小。
 * 
 * @param size 原始大小
 * @param alignment 对齐要求（字节），0表示使用默认对齐
 * @return size_t 对齐后的大小
 */
size_t F_calculate_aligned_size(size_t size, size_t alignment);

/**
 * @brief 分配对齐内存
 * 
 * 跨平台的对齐内存分配函数。
 * 
 * @param size 要分配的大小
 * @param alignment 对齐要求
 * @return void* 分配的内存指针，失败返回NULL
 */
void* F_allocate_aligned_memory(size_t size, size_t alignment);

/**
 * @brief 释放对齐内存
 * 
 * 跨平台的对齐内存释放函数。
 * 
 * @param ptr 要释放的内存指针
 */
void F_free_aligned_memory(void* ptr);

#ifdef __cplusplus
}
#endif

#endif // CN_POOL_UTILS_H
