/**
 * @file CN_string.c
 * @brief 字符串模块实现文件
 * 
 * 实现字符串数据结构的核心功能。
 * 包括创建、销毁、追加、比较、查找、替换等操作。
 * 支持UTF-8编码的字符串处理。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_string.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include <locale.h>
#include <stdio.h>

// 默认初始容量
#define CN_STRING_INITIAL_CAPACITY 32
// 扩容因子
#define CN_STRING_GROWTH_FACTOR 2

/**
 * @brief 内部函数：确保字符串有足够容量
 * 
 * 检查字符串是否有足够容量容纳新内容，如果没有则扩容。
 * 
 * @param str 字符串指针
 * @param additional_bytes 需要额外增加的字节数
 * @return 容量足够或扩容成功返回true，失败返回false
 */
static bool ensure_capacity(Stru_String_t* str, size_t additional_bytes)
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
 * @brief 内部函数：计算UTF-8字符串长度
 * 
 * 计算UTF-8编码字符串的字符数（不是字节数）。
 * 
 * @param utf8_str UTF-8字符串
 * @return 字符数
 */
static size_t utf8_strlen(const char* utf8_str)
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
 * @brief 内部函数：获取UTF-8字符的字节长度
 * 
 * 获取UTF-8字符的字节长度。
 * 
 * @param first_byte UTF-8字符的第一个字节
 * @return 字符的字节长度（1-4）
 */
static size_t utf8_char_len(unsigned char first_byte)
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

Stru_String_t* F_create_string(size_t initial_capacity)
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

Stru_String_t* F_create_string_from_cstr(const char* cstr)
{
    if (cstr == NULL)
    {
        return NULL;
    }
    
    size_t cstr_len = strlen(cstr);
    size_t char_count = utf8_strlen(cstr);
    
    // 创建字符串
    Stru_String_t* str = F_create_string(cstr_len + 1); // +1 for null terminator
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

void F_destroy_string(Stru_String_t* str)
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

Stru_String_t* F_string_substring(Stru_String_t* str, size_t start, size_t length)
{
    if (str == NULL || start >= str->length || length == 0)
    {
        return NULL;
    }
    
    // 计算实际要复制的字符数
    size_t actual_length = length;
    if (start + length > str->length)
    {
        actual_length = str->length - start;
    }
    
    // 查找起始字节位置
    size_t byte_pos = 0;
    size_t char_pos = 0;
    while (char_pos < start && byte_pos < str->byte_length)
    {
        size_t char_len = utf8_char_len((unsigned char)str->data[byte_pos]);
        byte_pos += char_len;
        char_pos++;
    }
    
    // 查找结束字节位置
    size_t end_byte_pos = byte_pos;
    size_t end_char_pos = char_pos;
    while (end_char_pos < start + actual_length && end_byte_pos < str->byte_length)
    {
        size_t char_len = utf8_char_len((unsigned char)str->data[end_byte_pos]);
        end_byte_pos += char_len;
        end_char_pos++;
    }
    
    // 计算子字符串字节长度
    size_t substring_byte_len = end_byte_pos - byte_pos;
    
    // 创建新字符串
    Stru_String_t* substring = F_create_string(substring_byte_len + 1);
    if (substring == NULL)
    {
        return NULL;
    }
    
    // 复制子字符串数据
    memcpy(substring->data, str->data + byte_pos, substring_byte_len);
    substring->data[substring_byte_len] = '\0';
    substring->byte_length = substring_byte_len;
    substring->length = actual_length;
    
    return substring;
}

int64_t F_string_find(Stru_String_t* str, Stru_String_t* substr, size_t start_pos)
{
    if (str == NULL || substr == NULL || start_pos >= str->length)
    {
        return -1;
    }
    
    // 转换为C字符串以便使用strstr
    const char* str_cstr = F_string_cstr(str);
    const char* substr_cstr = F_string_cstr(substr);
    
    if (str_cstr == NULL || substr_cstr == NULL)
    {
        return -1;
    }
    
    // 查找子字符串
    const char* found = strstr(str_cstr, substr_cstr);
    if (found == NULL)
    {
        return -1;
    }
    
    // 计算字符位置（不是字节位置）
    size_t byte_offset = found - str_cstr;
    size_t char_pos = 0;
    size_t byte_pos = 0;
    
    while (byte_pos < byte_offset && byte_pos < str->byte_length)
    {
        size_t char_len = utf8_char_len((unsigned char)str->data[byte_pos]);
        byte_pos += char_len;
        char_pos++;
    }
    
    return (int64_t)char_pos;
}

bool F_string_replace(Stru_String_t* str, Stru_String_t* old_substr, Stru_String_t* new_substr)
{
    if (str == NULL || old_substr == NULL || new_substr == NULL)
    {
        return false;
    }
    
    // 查找旧子字符串位置
    int64_t pos = F_string_find(str, old_substr, 0);
    if (pos < 0)
    {
        return false; // 未找到
    }
    
    // 获取子字符串
    Stru_String_t* before = F_string_substring(str, 0, (size_t)pos);
    if (before == NULL)
    {
        return false;
    }
    
    size_t after_start = (size_t)pos + old_substr->length;
    size_t after_length = str->length - after_start;
    Stru_String_t* after = F_string_substring(str, after_start, after_length);
    
    // 构建新字符串
    F_string_clear(str);
    bool success = F_string_append(str, before) &&
                   F_string_append(str, new_substr) &&
                   (after == NULL || F_string_append(str, after));
    
    // 清理
    F_destroy_string(before);
    if (after != NULL)
    {
        F_destroy_string(after);
    }
    
    return success;
}

bool F_string_to_upper(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    // 设置locale以支持多字节字符
    setlocale(LC_ALL, "en_US.UTF-8");
    
    // 转换为大写（简单实现，仅处理ASCII）
    for (size_t i = 0; i < str->byte_length; i++)
    {
        if (str->data[i] >= 'a' && str->data[i] <= 'z')
        {
            str->data[i] = str->data[i] - 'a' + 'A';
        }
    }
    
    return true;
}

bool F_string_to_lower(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    // 设置locale以支持多字节字符
    setlocale(LC_ALL, "en_US.UTF-8");
    
    // 转换为小写（简单实现，仅处理ASCII）
    for (size_t i = 0; i < str->byte_length; i++)
    {
        if (str->data[i] >= 'A' && str->data[i] <= 'Z')
        {
            str->data[i] = str->data[i] - 'A' + 'a';
        }
    }
    
    return true;
}

bool F_string_trim(Stru_String_t* str)
{
    if (str == NULL || str->byte_length == 0)
    {
        return false;
    }
    
    // 查找起始非空白字符
    size_t start = 0;
    while (start < str->byte_length && isspace((unsigned char)str->data[start]))
    {
        start++;
    }
    
    // 查找结束非空白字符
    size_t end = str->byte_length;
    while (end > start && isspace((unsigned char)str->data[end - 1]))
    {
        end--;
    }
    
    if (start == 0 && end == str->byte_length)
    {
        // 没有需要去除的空白字符
        return true;
    }
    
    // 计算新长度
    size_t new_byte_length = end - start;
    
    // 移动数据
    if (start > 0)
    {
        memmove(str->data, str->data + start, new_byte_length);
    }
    
    str->data[new_byte_length] = '\0';
    str->byte_length = new_byte_length;
    
    // 重新计算字符长度
    str->length = utf8_strlen(str->data);
    
    return true;
}
