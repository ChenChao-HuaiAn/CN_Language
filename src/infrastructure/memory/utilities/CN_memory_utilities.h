/**
 * @file CN_memory_utilities.h
 * @brief 内存工具函数接口
 * 
 * 提供便捷的内存操作函数，供项目其他部分使用。
 * 遵循项目命名规范，使用cn_前缀。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_UTILITIES_H
#define CN_MEMORY_UTILITIES_H

#include <stddef.h>

// 包含子模块接口
#include "operations/CN_memory_operations.h"
#include "safety/CN_memory_safety.h"
#include "alignment/CN_memory_alignment.h"
#include "statistics/CN_memory_statistics.h"

/**
 * @brief 分配内存（使用默认分配器）
 * 
 * 使用默认分配器分配指定大小的内存块。
 * 如果分配失败，返回NULL。
 * 
 * @param size 要分配的字节数
 * @return void* 分配的内存指针
 */
void* cn_malloc(size_t size);

/**
 * @brief 分配对齐内存
 * 
 * 使用默认分配器分配指定大小和对齐要求的内存块。
 * 如果分配失败，返回NULL。
 * 
 * @param size 要分配的字节数
 * @param alignment 对齐要求
 * @return void* 分配的内存指针
 */
void* cn_malloc_aligned(size_t size, size_t alignment);

/**
 * @brief 重新分配内存
 * 
 * 使用默认分配器重新分配内存块。
 * 如果ptr为NULL，等同于cn_malloc(new_size)。
 * 如果new_size为0，等同于cn_free(ptr)。
 * 
 * @param ptr 原内存指针
 * @param new_size 新的字节数
 * @return void* 重新分配的内存指针
 */
void* cn_realloc(void* ptr, size_t new_size);

/**
 * @brief 释放内存
 * 
 * 使用默认分配器释放内存块。
 * 如果ptr为NULL，不执行任何操作。
 * 
 * @param ptr 要释放的内存指针
 */
void cn_free(void* ptr);

/**
 * @brief 复制内存
 * 
 * 从源内存复制指定字节数到目标内存。
 * 如果dest和src重叠，行为未定义。
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
 * 将内存块的每个字节设置为指定值。
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
 * 比较两个内存块的内容。
 * 返回0表示相等，负值表示ptr1 < ptr2，正值表示ptr1 > ptr2。
 * 
 * @param ptr1 第一个内存块
 * @param ptr2 第二个内存块
 * @param size 要比较的字节数
 * @return int 比较结果
 */
int cn_memcmp(const void* ptr1, const void* ptr2, size_t size);

/**
 * @brief 移动内存
 * 
 * 从源内存移动指定字节数到目标内存。
 * 正确处理重叠的内存区域。
 * 
 * @param dest 目标内存
 * @param src 源内存
 * @param size 要移动的字节数
 * @return void* 目标内存指针
 */
void* cn_memmove(void* dest, const void* src, size_t size);

/**
 * @brief 分配并清零内存
 * 
 * 分配指定大小的内存块，并将其内容清零。
 * 如果分配失败，返回NULL。
 * 
 * @param size 要分配的字节数
 * @return void* 分配并清零的内存指针
 */
void* cn_calloc(size_t num, size_t size);

/**
 * @brief 设置默认分配器
 * 
 * 设置全局默认分配器。
 * 如果allocator为NULL，使用系统分配器。
 * 
 * @param allocator 要设置为默认的分配器
 */
void cn_set_default_allocator(void* allocator);

/**
 * @brief 获取默认分配器
 * 
 * 获取当前全局默认分配器。
 * 
 * @return void* 当前默认分配器
 */
void* cn_get_default_allocator(void);

#endif // CN_MEMORY_UTILITIES_H
