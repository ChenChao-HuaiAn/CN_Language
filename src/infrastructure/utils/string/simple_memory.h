/******************************************************************************
 * 文件名: simple_memory.h
 * 功能: 简单的内存管理包装器，用于字符串模块测试
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef SIMPLE_MEMORY_H
#define SIMPLE_MEMORY_H

#include <stdlib.h>
#include <string.h>

// 简单的内存管理函数，直接包装标准库
static inline void* cn_malloc(size_t size)
{
    return malloc(size);
}

static inline void cn_free(void* ptr)
{
    free(ptr);
}

static inline void* cn_realloc(void* ptr, size_t new_size)
{
    return realloc(ptr, new_size);
}

static inline void* cn_calloc(size_t count, size_t size)
{
    return calloc(count, size);
}

#endif // SIMPLE_MEMORY_H
