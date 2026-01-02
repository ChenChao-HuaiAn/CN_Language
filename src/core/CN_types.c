/******************************************************************************
 * 文件名: CN_types.c
 * 功能: CN_Language公共数据类型内部辅助函数实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，实现数据类型相关函数
 *  2026-01-02: 重构文件，只保留内部辅助函数，其他函数拆分到专用模块
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_types.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 内部辅助函数实现
// ============================================================================

/**
 * @brief 安全分配内存
 * @param size 要分配的内存大小
 * @return 分配的内存指针，失败返回NULL
 */
void* safe_malloc(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }
    
    void* ptr = malloc(size);
    if (ptr == NULL)
    {
        // 内存分配失败，可以记录错误
        return NULL;
    }
    
    // 清零内存
    memset(ptr, 0, size);
    return ptr;
}

/**
 * @brief 安全重新分配内存
 * @param ptr 原内存指针
 * @param size 新的内存大小
 * @return 重新分配的内存指针，失败返回NULL
 */
void* safe_realloc(void* ptr, size_t size)
{
    if (size == 0)
    {
        free(ptr);
        return NULL;
    }
    
    void* new_ptr = realloc(ptr, size);
    if (new_ptr == NULL)
    {
        // 内存重新分配失败，可以记录错误
        return NULL;
    }
    
    return new_ptr;
}

/**
 * @brief 复制字符串
 * @param str 要复制的字符串
 * @param length 字符串长度
 * @return 新分配的字符串副本，失败返回NULL
 */
char* duplicate_string(const char* str, size_t length)
{
    if (str == NULL || length == 0)
    {
        return NULL;
    }
    
    // 计算实际长度
    size_t actual_length = length;
    if (actual_length == 0)
    {
        actual_length = strlen(str);
    }
    
    // 分配内存
    char* new_str = (char*)safe_malloc(actual_length + 1);
    if (new_str == NULL)
    {
        return NULL;
    }
    
    // 复制字符串
    memcpy(new_str, str, actual_length);
    new_str[actual_length] = '\0';
    
    return new_str;
}
