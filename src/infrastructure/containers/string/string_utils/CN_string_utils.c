/**
 * @file CN_string_utils.c
 * @brief 字符串工具模块实现文件
 * 
 * 实现字符串的工具函数。
 * 包括UTF-8处理、字符分类、格式化等辅助功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_string_utils.h"
#include "../string_operations/CN_string_operations.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

// 从核心模块导入内部函数
extern size_t utf8_strlen(const char* utf8_str);
extern size_t utf8_char_len(unsigned char first_byte);

size_t F_utf8_strlen(const char* utf8_str)
{
    return utf8_strlen(utf8_str);
}

size_t F_utf8_char_len(unsigned char first_byte)
{
    return utf8_char_len(first_byte);
}

bool F_string_validate_utf8(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    return F_cstr_validate_utf8(str->data, str->byte_length);
}

bool F_cstr_validate_utf8(const char* cstr, size_t length)
{
    if (cstr == NULL)
    {
        return false;
    }
    
    if (length == 0)
    {
        length = strlen(cstr);
    }
    
    for (size_t i = 0; i < length;)
    {
        unsigned char ch = (unsigned char)cstr[i];
        
        if (ch <= 0x7F)
        {
            // ASCII字符
            i++;
        }
        else if ((ch & 0xE0) == 0xC0)
        {
            // 2字节UTF-8字符
            if (i + 1 >= length) return false;
            if ((cstr[i+1] & 0xC0) != 0x80) return false;
            
            // 检查过度编码：2字节UTF-8字符的值必须 >= 0x80
            // 0xC0 0x80 是过度的NUL字符编码，无效
            unsigned char byte2 = (unsigned char)cstr[i+1];
            unsigned int code_point = ((ch & 0x1F) << 6) | (byte2 & 0x3F);
            if (code_point < 0x80) return false;
            
            i += 2;
        }
        else if ((ch & 0xF0) == 0xE0)
        {
            // 3字节UTF-8字符
            if (i + 2 >= length) return false;
            if ((cstr[i+1] & 0xC0) != 0x80) return false;
            if ((cstr[i+2] & 0xC0) != 0x80) return false;
            
            // 检查过度编码：3字节UTF-8字符的值必须 >= 0x800
            unsigned char byte2 = (unsigned char)cstr[i+1];
            unsigned char byte3 = (unsigned char)cstr[i+2];
            unsigned int code_point = ((ch & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F);
            if (code_point < 0x800) return false;
            
            i += 3;
        }
        else if ((ch & 0xF8) == 0xF0)
        {
            // 4字节UTF-8字符
            if (i + 3 >= length) return false;
            if ((cstr[i+1] & 0xC0) != 0x80) return false;
            if ((cstr[i+2] & 0xC0) != 0x80) return false;
            if ((cstr[i+3] & 0xC0) != 0x80) return false;
            
            // 检查过度编码：4字节UTF-8字符的值必须 >= 0x10000
            unsigned char byte2 = (unsigned char)cstr[i+1];
            unsigned char byte3 = (unsigned char)cstr[i+2];
            unsigned char byte4 = (unsigned char)cstr[i+3];
            unsigned int code_point = ((ch & 0x07) << 18) | ((byte2 & 0x3F) << 12) | 
                                     ((byte3 & 0x3F) << 6) | (byte4 & 0x3F);
            if (code_point < 0x10000) return false;
            
            i += 4;
        }
        else
        {
            // 无效的UTF-8序列
            return false;
        }
    }
    
    return true;
}

int64_t F_string_utf8_char_position(Stru_String_t* str, size_t char_index)
{
    if (str == NULL || char_index >= str->length)
    {
        return -1;
    }
    
    size_t byte_pos = 0;
    size_t current_char = 0;
    
    while (current_char < char_index && byte_pos < str->byte_length)
    {
        size_t char_len = utf8_char_len((unsigned char)str->data[byte_pos]);
        byte_pos += char_len;
        current_char++;
    }
    
    if (current_char != char_index)
    {
        return -1;
    }
    
    return (int64_t)byte_pos;
}

bool F_string_is_alpha(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    // 空字符串视为只包含字母
    if (str->byte_length == 0)
    {
        return true;
    }
    
    for (size_t i = 0; i < str->byte_length; i++)
    {
        if (!isalpha((unsigned char)str->data[i]))
        {
            return false;
        }
    }
    
    return true;
}

bool F_string_is_digit(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    // 空字符串视为只包含数字
    if (str->byte_length == 0)
    {
        return true;
    }
    
    for (size_t i = 0; i < str->byte_length; i++)
    {
        if (!isdigit((unsigned char)str->data[i]))
        {
            return false;
        }
    }
    
    return true;
}

bool F_string_is_alnum(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    // 空字符串视为只包含字母数字
    if (str->byte_length == 0)
    {
        return true;
    }
    
    for (size_t i = 0; i < str->byte_length; i++)
    {
        if (!isalnum((unsigned char)str->data[i]))
        {
            return false;
        }
    }
    
    return true;
}

bool F_string_is_ascii(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    // 空字符串视为只包含ASCII字符
    if (str->byte_length == 0)
    {
        return true;
    }
    
    for (size_t i = 0; i < str->byte_length; i++)
    {
        if ((unsigned char)str->data[i] > 127)
        {
            return false;
        }
    }
    
    return true;
}

bool F_string_is_space(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    // 空字符串视为只包含空白
    if (str->byte_length == 0)
    {
        return true;
    }
    
    for (size_t i = 0; i < str->byte_length; i++)
    {
        if (!isspace((unsigned char)str->data[i]))
        {
            return false;
        }
    }
    
    return true;
}

bool F_string_is_lower(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    // 空字符串视为只包含小写字母
    if (str->byte_length == 0)
    {
        return true;
    }
    
    for (size_t i = 0; i < str->byte_length; i++)
    {
        unsigned char ch = (unsigned char)str->data[i];
        if (isalpha(ch) && !islower(ch))
        {
            return false;
        }
    }
    
    return true;
}

bool F_string_is_upper(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    // 空字符串视为只包含大写字母
    if (str->byte_length == 0)
    {
        return true;
    }
    
    for (size_t i = 0; i < str->byte_length; i++)
    {
        unsigned char ch = (unsigned char)str->data[i];
        if (isalpha(ch) && !isupper(ch))
        {
            return false;
        }
    }
    
    return true;
}

Stru_String_t* F_string_format(const char* format, ...)
{
    if (format == NULL)
    {
        return NULL;
    }
    
    va_list args;
    va_start(args, format);
    
    // 第一次调用获取所需长度
    va_list args_copy;
    va_copy(args_copy, args);
    int length = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    
    if (length < 0)
    {
        va_end(args);
        return NULL;
    }
    
    // 分配缓冲区
    size_t buffer_size = (size_t)length + 1;
    char* buffer = (char*)malloc(buffer_size);
    if (buffer == NULL)
    {
        va_end(args);
        return NULL;
    }
    
    // 格式化字符串
    vsnprintf(buffer, buffer_size, format, args);
    va_end(args);
    
    // 创建字符串
    Stru_String_t* str = F_string_create_from_cstr(buffer);
    free(buffer);
    
    return str;
}

bool F_string_format_to(Stru_String_t* str, const char* format, ...)
{
    if (str == NULL || format == NULL)
    {
        return false;
    }
    
    va_list args;
    va_start(args, format);
    
    // 第一次调用获取所需长度
    va_list args_copy;
    va_copy(args_copy, args);
    int length = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    
    if (length < 0)
    {
        va_end(args);
        return false;
    }
    
    // 确保有足够容量
    extern bool ensure_capacity(Stru_String_t* str, size_t additional_bytes);
    if (!ensure_capacity(str, (size_t)length - str->byte_length))
    {
        va_end(args);
        return false;
    }
    
    // 格式化字符串
    vsnprintf(str->data, str->capacity, format, args);
    va_end(args);
    
    // 更新长度
    str->byte_length = (size_t)length;
    str->length = utf8_strlen(str->data);
    str->data[str->byte_length] = '\0';
    
    return true;
}

bool F_string_to_int(Stru_String_t* str, int* value, int base)
{
    if (str == NULL || value == NULL)
    {
        return false;
    }
    
    const char* cstr = F_string_cstr(str);
    if (cstr == NULL)
    {
        return false;
    }
    
    char* endptr;
    errno = 0;
    long result = strtol(cstr, &endptr, base);
    
    if (errno != 0 || *endptr != '\0')
    {
        return false;
    }
    
    // 检查是否溢出
    if (result > INT_MAX || result < INT_MIN)
    {
        return false;
    }
    
    *value = (int)result;
    return true;
}

bool F_string_to_double(Stru_String_t* str, double* value)
{
    if (str == NULL || value == NULL)
    {
        return false;
    }
    
    const char* cstr = F_string_cstr(str);
    if (cstr == NULL)
    {
        return false;
    }
    
    char* endptr;
    errno = 0;
    double result = strtod(cstr, &endptr);
    
    if (errno != 0 || *endptr != '\0')
    {
        return false;
    }
    
    // 检查是否溢出
    if (result == HUGE_VAL || result == -HUGE_VAL)
    {
        return false;
    }
    
    *value = result;
    return true;
}

Stru_String_t** F_string_split(Stru_String_t* str, char delimiter, size_t* count)
{
    if (str == NULL || count == NULL)
    {
        return NULL;
    }
    
    return F_cstr_split(str->data, delimiter, count);
}

Stru_String_t** F_cstr_split(const char* cstr, char delimiter, size_t* count)
{
    if (cstr == NULL || count == NULL)
    {
        return NULL;
    }
    
    // 计算分割数量
    size_t segment_count = 1;
    for (const char* p = cstr; *p != '\0'; p++)
    {
        if (*p == delimiter)
        {
            segment_count++;
        }
    }
    
    // 分配数组
    Stru_String_t** segments = (Stru_String_t**)malloc(segment_count * sizeof(Stru_String_t*));
    if (segments == NULL)
    {
        return NULL;
    }
    
    // 分割字符串
    const char* start = cstr;
    size_t segment_index = 0;
    
    for (const char* p = cstr; ; p++)
    {
        if (*p == delimiter || *p == '\0')
        {
            // 计算子字符串长度
            size_t length = p - start;
            
            // 创建子字符串
            if (length > 0)
            {
                char* temp = (char*)malloc(length + 1);
                if (temp == NULL)
                {
                    // 清理已分配的内存
                    for (size_t i = 0; i < segment_index; i++)
                    {
                        F_string_destroy(segments[i]);
                    }
                    free(segments);
                    return NULL;
                }
                
                memcpy(temp, start, length);
                temp[length] = '\0';
                
                segments[segment_index] = F_string_create_from_cstr(temp);
                free(temp);
            }
            else
            {
                segments[segment_index] = F_string_create_from_cstr("");
            }
            
            if (segments[segment_index] == NULL)
            {
                // 清理已分配的内存
                for (size_t i = 0; i <= segment_index; i++)
                {
                    if (segments[i] != NULL)
                    {
                        F_string_destroy(segments[i]);
                    }
                }
                free(segments);
                return NULL;
            }
            
            segment_index++;
            
            if (*p == '\0')
            {
                break;
            }
            
            start = p + 1;
        }
    }
    
    *count = segment_count;
    return segments;
}

Stru_String_t* F_string_join(Stru_String_t** strings, size_t count, const char* delimiter)
{
    if (delimiter == NULL)
    {
        return NULL;
    }
    
    // 处理空数组的情况：返回空字符串
    if (count == 0)
    {
        return F_string_create_from_cstr("");
    }
    
    // 如果count > 0但strings为NULL，则返回NULL
    if (strings == NULL)
    {
        return NULL;
    }
    
    // 计算总长度
    size_t total_length = 0;
    size_t delimiter_length = strlen(delimiter);
    
    for (size_t i = 0; i < count; i++)
    {
        if (strings[i] != NULL)
        {
            total_length += strings[i]->byte_length;
            if (i < count - 1)
            {
                total_length += delimiter_length;
            }
        }
    }
    
    // 创建结果字符串
    Stru_String_t* result = F_string_create_with_capacity(total_length + 1);
    if (result == NULL)
    {
        return NULL;
    }
    
    // 连接字符串
    for (size_t i = 0; i < count; i++)
    {
        if (strings[i] != NULL)
        {
            F_string_append_cstr(result, strings[i]->data);
        }
        
        if (i < count - 1)
        {
            F_string_append_cstr(result, delimiter);
        }
    }
    
    return result;
}
