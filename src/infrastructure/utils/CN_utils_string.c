/**
 * @file CN_utils_string.c
 * @brief 字符串处理函数实现
 * 
 * 实现了安全的字符串处理函数，包括复制、连接、比较等操作。
 * 所有函数都包含边界检查，防止缓冲区溢出。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_utils_interface.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

// ============================================================================
// 字符串处理函数实现
// ============================================================================

char* cn_strcpy(char* dest, const char* src, size_t max_len)
{
    if (dest == NULL || src == NULL || max_len == 0)
    {
        return NULL;
    }
    
    size_t i = 0;
    while (i < max_len - 1 && src[i] != '\0')
    {
        dest[i] = src[i];
        i++;
    }
    
    // 确保字符串以空字符结尾
    dest[i] = '\0';
    
    return dest;
}

char* cn_strcat(char* dest, const char* src, size_t max_len)
{
    if (dest == NULL || src == NULL || max_len == 0)
    {
        return NULL;
    }
    
    // 找到目标字符串的结尾
    size_t dest_len = 0;
    while (dest_len < max_len && dest[dest_len] != '\0')
    {
        dest_len++;
    }
    
    if (dest_len >= max_len)
    {
        // 目标字符串没有空字符，不安全
        return NULL;
    }
    
    // 计算剩余空间
    size_t remaining = max_len - dest_len - 1;
    
    // 复制源字符串
    size_t i = 0;
    while (i < remaining && src[i] != '\0')
    {
        dest[dest_len + i] = src[i];
        i++;
    }
    
    // 确保字符串以空字符结尾
    dest[dest_len + i] = '\0';
    
    return dest;
}

int cn_strcmp(const char* str1, const char* str2)
{
    if (str1 == NULL && str2 == NULL)
    {
        return 0;
    }
    
    if (str1 == NULL)
    {
        return -1;
    }
    
    if (str2 == NULL)
    {
        return 1;
    }
    
    return strcmp(str1, str2);
}

int cn_strncmp(const char* str1, const char* str2, size_t n)
{
    if (str1 == NULL && str2 == NULL)
    {
        return 0;
    }
    
    if (str1 == NULL)
    {
        return -1;
    }
    
    if (str2 == NULL)
    {
        return 1;
    }
    
    return strncmp(str1, str2, n);
}

size_t cn_strlen(const char* str)
{
    if (str == NULL)
    {
        return 0;
    }
    
    return strlen(str);
}

char* cn_strchr(const char* str, int ch)
{
    if (str == NULL)
    {
        return NULL;
    }
    
    return strchr(str, ch);
}

char* cn_strstr(const char* haystack, const char* needle)
{
    if (haystack == NULL || needle == NULL)
    {
        return NULL;
    }
    
    return strstr(haystack, needle);
}

char* cn_strncpy(char* dest, const char* src, size_t n)
{
    if (dest == NULL || src == NULL || n == 0)
    {
        return NULL;
    }
    
    return strncpy(dest, src, n);
}

// ============================================================================
// 内存操作函数实现
// ============================================================================

void* cn_memmove(void* dest, const void* src, size_t size)
{
    if (dest == NULL || src == NULL || size == 0)
    {
        return NULL;
    }
    
    return memmove(dest, src, size);
}

void* cn_memchr(const void* ptr, int value, size_t size)
{
    if (ptr == NULL || size == 0)
    {
        return NULL;
    }
    
    return memchr(ptr, value, size);
}
