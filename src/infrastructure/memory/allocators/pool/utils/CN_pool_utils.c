/**
 * @file CN_pool_utils.c
 * @brief 对象池分配器工具函数
 * 
 * 实现了对象池分配器的工具函数，包括内存对齐计算等。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-12
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_pool_utils.h"
#include <stdint.h>
#include <stdlib.h>

#ifdef _WIN32
#include <malloc.h>
#else
#include <stdlib.h>
#endif

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
size_t F_calculate_aligned_size(size_t size, size_t alignment)
{
    if (alignment == 0)
    {
        return size;
    }
    
    // 确保对齐是2的幂
    if ((alignment & (alignment - 1)) != 0)
    {
        // 如果不是2的幂，向上取整到最近的2的幂
        alignment--;
        alignment |= alignment >> 1;
        alignment |= alignment >> 2;
        alignment |= alignment >> 4;
        alignment |= alignment >> 8;
        alignment |= alignment >> 16;
        alignment++;
    }
    
    // 计算对齐后的大小
    return ((size + alignment - 1) / alignment) * alignment;
}

/**
 * @brief 分配对齐内存
 * 
 * 跨平台的对齐内存分配函数。
 * 
 * @param size 要分配的大小
 * @param alignment 对齐要求
 * @return void* 分配的内存指针，失败返回NULL
 */
void* F_allocate_aligned_memory(size_t size, size_t alignment)
{
    if (size == 0)
    {
        return NULL;
    }
    
    void* mem = NULL;
    
    #ifdef _WIN32
        mem = _aligned_malloc(size, alignment > 0 ? alignment : 16);
    #else
        if (alignment > 0)
        {
            if (posix_memalign(&mem, alignment, size) != 0)
            {
                mem = NULL;
            }
        }
        else
        {
            mem = malloc(size);
        }
    #endif
    
    return mem;
}

/**
 * @brief 释放对齐内存
 * 
 * 跨平台的对齐内存释放函数。
 * 
 * @param ptr 要释放的内存指针
 */
void F_free_aligned_memory(void* ptr)
{
    if (ptr == NULL)
    {
        return;
    }
    
    #ifdef _WIN32
        _aligned_free(ptr);
    #else
        free(ptr);
    #endif
}

#ifdef __cplusplus
}
#endif
