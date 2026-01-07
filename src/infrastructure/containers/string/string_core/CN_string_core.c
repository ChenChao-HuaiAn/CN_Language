/**
 * @file CN_string_core.c
 * @brief 字符串核心模块实现文件
 * 
 * 实现字符串数据结构的核心功能。
 * 包括字符串的创建、销毁、基本属性和容量管理。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_string_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 默认初始容量
#define CN_STRING_INITIAL_CAPACITY 32
// 扩容因子
#define CN_STRING_GROWTH_FACTOR 2

/**
 * @brief 确保字符串有足够容量
 * 
 * 检查字符串是否有足够容量容纳新内容，如果没有则扩容。
 * 
 * @param str 字符串指针
 * @param additional_bytes 需要额外增加的字节数
 * @return 容量足够或扩容成功返回true，失败返回false
 */
bool ensure_capacity(Stru_String_t* str, size_t additional_bytes)
{
    if (str == NULL)
    {
        return false;
    }
    
    size_t required_capacity = str->byte_length + additional_bytes + 1; // +1 for null terminator
    
    if (required_capacity > str->capacity)
    {
        size_t new_capacity = str->capacity * CN_STRING_GROWTH_FACTOR;
        while (new_capacity < required_capacity)
        {
            new_capacity *= CN_STRING_GROWTH_FACTOR;
        }
        
        char* new_data = (char*)realloc(str->data, new_capacity);
        if (new_data == NULL)
        {
            return false;
        }
        
        str->data = new_data;
        str->capacity = new_capacity;
    }
    
    return true;
}

/**
 * @brief 计算UTF-8字符串长度
 * 
 * 计算UTF-8编码字符串的字符数（不是字节数）。
 * 
 * @param utf8_str UTF-8字符串
 * @return 字符数
 */
size_t utf8_strlen(const char* utf8_str)
{
    if (utf8_str == NULL)
    {
        return 0;
    }
    
    size_t char_count = 0;
    while (*utf8_str)
    {
        if ((*utf8_str & 0xC0) != 0x80) // 不是UTF-8多字节序列的后续字节
        {
            char_count++;
        }
        utf8_str++;
    }
    
    return char_count;
}

/**
 * @brief 获取UTF-8字符的字节长度
 * 
 * 获取UTF-8字符的字节长度。
 * 
 * @param first_byte UTF-8字符的第一个字节
 * @return 字符的字节长度（1-4）
 */
size_t utf8_char_len(unsigned char first_byte)
{
    if ((first_byte & 0x80) == 0x00) // 0xxxxxxx
    {
        return 1;
    }
    else if ((first_byte & 0xE0) == 0xC0) // 110xxxxx
    {
        return 2;
    }
    else if ((first_byte & 0xF0) == 0xE0) // 1110xxxx
    {
        return 3;
    }
    else if ((first_byte & 0xF8) == 0xF0) // 11110xxx
    {
        return 4;
    }
    
    return 1; // 无效的UTF-8序列，当作单字节处理
}

Stru_String_t* F_string_create_empty(void)
{
    return F_string_create_with_capacity(CN_STRING_INITIAL_CAPACITY);
}

Stru_String_t* F_string_create_with_capacity(size_t initial_capacity)
{
    // 确保最小容量
    if (initial_capacity < CN_STRING_INITIAL_CAPACITY)
    {
        initial_capacity = CN_STRING_INITIAL_CAPACITY;
    }
    
    // 分配字符串结构体
    Stru_String_t* str = (Stru_String_t*)malloc(sizeof(Stru_String_t));
    if (str == NULL)
    {
        return NULL;
    }
    
    // 分配字符串数据
    str->data = (char*)malloc(initial_capacity);
    if (str->data == NULL)
    {
        free(str);
        return NULL;
    }
    
    // 初始化字符串
    str->data[0] = '\0';
    str->length = 0;
    str->byte_length = 0;
    str->capacity = initial_capacity;
    
    return str;
}

Stru_String_t* F_string_create_from_cstr(const char* cstr)
{
    if (cstr == NULL)
    {
        return NULL;
    }
    
    size_t cstr_len = strlen(cstr);
    size_t char_count = utf8_strlen(cstr);
    
    // 创建字符串
    Stru_String_t* str = F_string_create_with_capacity(cstr_len + 1); // +1 for null terminator
    if (str == NULL)
    {
        return NULL;
    }
    
    // 复制数据
    memcpy(str->data, cstr, cstr_len + 1); // 包括null终止符
    str->byte_length = cstr_len;
    str->length = char_count;
    
    return str;
}

Stru_String_t* F_string_create_copy(Stru_String_t* str)
{
    if (str == NULL)
    {
        return NULL;
    }
    
    Stru_String_t* copy = F_string_create_with_capacity(str->capacity);
    if (copy == NULL)
    {
        return NULL;
    }
    
    // 复制数据
    memcpy(copy->data, str->data, str->byte_length + 1); // 包括null终止符
    copy->byte_length = str->byte_length;
    copy->length = str->length;
    
    return copy;
}

void F_string_destroy(Stru_String_t* str)
{
    if (str == NULL)
    {
        return;
    }
    
    if (str->data != NULL)
    {
        free(str->data);
        str->data = NULL;
    }
    
    free(str);
}

bool F_string_is_empty(Stru_String_t* str)
{
    if (str == NULL)
    {
        return true;
    }
    
    return str->length == 0;
}

size_t F_string_length(Stru_String_t* str)
{
    if (str == NULL)
    {
        return 0;
    }
    
    return str->length;
}

size_t F_string_byte_length(Stru_String_t* str)
{
    if (str == NULL)
    {
        return 0;
    }
    
    return str->byte_length;
}

const char* F_string_cstr(Stru_String_t* str)
{
    if (str == NULL || str->data == NULL)
    {
        return NULL;
    }
    
    // 确保以null结尾
    if (str->byte_length >= str->capacity)
    {
        // 这不应该发生，但为了安全起见
        if (!ensure_capacity(str, 1))
        {
            return NULL;
        }
    }
    
    str->data[str->byte_length] = '\0';
    return str->data;
}

size_t F_string_capacity(Stru_String_t* str)
{
    if (str == NULL)
    {
        return 0;
    }
    
    return str->capacity;
}

bool F_string_ensure_capacity(Stru_String_t* str, size_t required_bytes)
{
    return ensure_capacity(str, required_bytes);
}

bool F_string_reserve(Stru_String_t* str, size_t new_capacity)
{
    if (str == NULL)
    {
        return false;
    }
    
    if (new_capacity < str->byte_length)
    {
        return false; // 新容量不能小于当前内容长度
    }
    
    if (new_capacity <= str->capacity)
    {
        return true; // 已经足够大
    }
    
    char* new_data = (char*)realloc(str->data, new_capacity);
    if (new_data == NULL)
    {
        return false;
    }
    
    str->data = new_data;
    str->capacity = new_capacity;
    
    return true;
}

bool F_string_shrink_to_fit(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    size_t required_capacity = str->byte_length + 1; // +1 for null terminator
    
    if (required_capacity >= str->capacity)
    {
        return true; // 已经是最小容量
    }
    
    char* new_data = (char*)realloc(str->data, required_capacity);
    if (new_data == NULL)
    {
        return false;
    }
    
    str->data = new_data;
    str->capacity = required_capacity;
    
    return true;
}
