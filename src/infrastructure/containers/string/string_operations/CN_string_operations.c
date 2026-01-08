/**
 * @file CN_string_operations.c
 * @brief 字符串操作模块实现文件
 * 
 * 实现字符串的基本操作功能。
 * 包括字符串的追加、比较、复制、清空等操作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#define _POSIX_C_SOURCE 200809L  /* 启用strdup等POSIX函数 */

#include "CN_string_operations.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include <locale.h>

// 从核心模块导入内部函数
extern size_t utf8_strlen(const char* utf8_str);
extern size_t utf8_char_len(unsigned char first_byte);
extern bool ensure_capacity(Stru_String_t* str, size_t additional_bytes);

bool F_string_append(Stru_String_t* dest, Stru_String_t* src)
{
    if (dest == NULL || src == NULL)
    {
        return false;
    }
    
    return F_string_append_cstr(dest, src->data);
}

bool F_string_append_cstr(Stru_String_t* dest, const char* cstr)
{
    if (dest == NULL || cstr == NULL)
    {
        return false;
    }
    
    size_t cstr_len = strlen(cstr);
    size_t char_count = utf8_strlen(cstr);
    
    // 确保有足够容量
    if (!ensure_capacity(dest, cstr_len))
    {
        return false;
    }
    
    // 追加数据
    memcpy(dest->data + dest->byte_length, cstr, cstr_len);
    dest->byte_length += cstr_len;
    dest->length += char_count;
    dest->data[dest->byte_length] = '\0';
    
    return true;
}

bool F_string_append_char(Stru_String_t* str, char ch)
{
    if (str == NULL)
    {
        return false;
    }
    
    // 确保有足够容量
    if (!ensure_capacity(str, 1))
    {
        return false;
    }
    
    // 追加字符
    str->data[str->byte_length] = ch;
    str->byte_length++;
    str->length++; // 假设是单字节字符
    str->data[str->byte_length] = '\0';
    
    return true;
}

void F_string_clear(Stru_String_t* str)
{
    if (str == NULL)
    {
        return;
    }
    
    if (str->data != NULL && str->capacity > 0)
    {
        str->data[0] = '\0';
    }
    
    str->length = 0;
    str->byte_length = 0;
}

int F_string_compare(Stru_String_t* str1, Stru_String_t* str2)
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
    
    return strcmp(str1->data, str2->data);
}

int F_string_compare_case_insensitive(Stru_String_t* str1, Stru_String_t* str2)
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
    
    // 设置locale以支持大小写不敏感比较
    setlocale(LC_ALL, "en_US.UTF-8");
    
    // 跨平台实现：手动转换为小写后比较
    char* str1_lower = strdup(str1->data);
    char* str2_lower = strdup(str2->data);
    
    if (str1_lower == NULL || str2_lower == NULL)
    {
        if (str1_lower) free(str1_lower);
        if (str2_lower) free(str2_lower);
        return strcmp(str1->data, str2->data); // 回退到普通比较
    }
    
    // 转换为小写
    for (char* p = str1_lower; *p; p++)
    {
        *p = tolower((unsigned char)*p);
    }
    
    for (char* p = str2_lower; *p; p++)
    {
        *p = tolower((unsigned char)*p);
    }
    
    int result = strcmp(str1_lower, str2_lower);
    
    free(str1_lower);
    free(str2_lower);
    
    return result;
}

bool F_string_copy(Stru_String_t* dest, Stru_String_t* src)
{
    if (dest == NULL || src == NULL)
    {
        return false;
    }
    
    // 确保有足够容量
    if (!ensure_capacity(dest, src->byte_length - dest->byte_length))
    {
        return false;
    }
    
    // 复制数据
    memcpy(dest->data, src->data, src->byte_length + 1); // 包括null终止符
    dest->length = src->length;
    dest->byte_length = src->byte_length;
    
    return true;
}

bool F_string_copy_cstr(Stru_String_t* dest, const char* cstr)
{
    if (dest == NULL || cstr == NULL)
    {
        return false;
    }
    
    size_t cstr_len = strlen(cstr);
    size_t char_count = utf8_strlen(cstr);
    
    // 确保有足够容量
    if (!ensure_capacity(dest, cstr_len - dest->byte_length))
    {
        return false;
    }
    
    // 复制数据
    memcpy(dest->data, cstr, cstr_len + 1); // 包括null终止符
    dest->length = char_count;
    dest->byte_length = cstr_len;
    
    return true;
}

Stru_String_t* F_string_concat(Stru_String_t* str1, Stru_String_t* str2)
{
    if (str1 == NULL && str2 == NULL)
    {
        return NULL;
    }
    
    if (str1 == NULL)
    {
        return F_string_create_from_cstr(str2->data);
    }
    
    if (str2 == NULL)
    {
        return F_string_create_from_cstr(str1->data);
    }
    
    // 计算总长度
    size_t total_byte_length = str1->byte_length + str2->byte_length;
    
    // 创建新字符串
    Stru_String_t* result = F_string_create_with_capacity(total_byte_length + 1);
    if (result == NULL)
    {
        return NULL;
    }
    
    // 复制第一个字符串
    memcpy(result->data, str1->data, str1->byte_length);
    result->byte_length = str1->byte_length;
    result->length = str1->length;
    
    // 追加第二个字符串
    memcpy(result->data + result->byte_length, str2->data, str2->byte_length);
    result->byte_length += str2->byte_length;
    result->length += str2->length;
    result->data[result->byte_length] = '\0';
    
    return result;
}

Stru_String_t* F_string_concat_cstr(Stru_String_t* str, const char* cstr)
{
    if (str == NULL && cstr == NULL)
    {
        return NULL;
    }
    
    if (str == NULL)
    {
        return F_string_create_from_cstr(cstr);
    }
    
    if (cstr == NULL)
    {
        return F_string_create_from_cstr(str->data);
    }
    
    size_t cstr_len = strlen(cstr);
    size_t total_byte_length = str->byte_length + cstr_len;
    
    // 创建新字符串
    Stru_String_t* result = F_string_create_with_capacity(total_byte_length + 1);
    if (result == NULL)
    {
        return NULL;
    }
    
    // 复制第一个字符串
    memcpy(result->data, str->data, str->byte_length);
    result->byte_length = str->byte_length;
    result->length = str->length;
    
    // 追加C字符串
    memcpy(result->data + result->byte_length, cstr, cstr_len);
    result->byte_length += cstr_len;
    result->length += utf8_strlen(cstr);
    result->data[result->byte_length] = '\0';
    
    return result;
}
