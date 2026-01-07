/**
 * @file CN_string_transform.c
 * @brief 字符串转换模块实现文件
 * 
 * 实现字符串的转换功能。
 * 包括大小写转换、空白处理、格式化等操作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_string_transform.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include <locale.h>

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

bool F_string_swap_case(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    // 设置locale以支持多字节字符
    setlocale(LC_ALL, "en_US.UTF-8");
    
    // 反转大小写（简单实现，仅处理ASCII）
    for (size_t i = 0; i < str->byte_length; i++)
    {
        if (str->data[i] >= 'a' && str->data[i] <= 'z')
        {
            str->data[i] = str->data[i] - 'a' + 'A';
        }
        else if (str->data[i] >= 'A' && str->data[i] <= 'Z')
        {
            str->data[i] = str->data[i] - 'A' + 'a';
        }
    }
    
    return true;
}

bool F_string_capitalize(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    if (str->byte_length == 0)
    {
        return true; // 空字符串，无需操作
    }
    
    // 设置locale以支持多字节字符
    setlocale(LC_ALL, "en_US.UTF-8");
    
    // 只将第一个字符大写，其他字符保持不变
    bool first_char_found = false;
    
    for (size_t i = 0; i < str->byte_length; i++)
    {
        if (!isspace((unsigned char)str->data[i]))
        {
            // 找到第一个非空白字符
            if (!first_char_found)
            {
                if (str->data[i] >= 'a' && str->data[i] <= 'z')
                {
                    str->data[i] = str->data[i] - 'a' + 'A';
                }
                first_char_found = true;
            }
            // 其他字符保持不变
        }
    }
    
    return true;
}

bool F_string_title(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    if (str->byte_length == 0)
    {
        return true; // 空字符串，无需操作
    }
    
    // 设置locale以支持多字节字符
    setlocale(LC_ALL, "en_US.UTF-8");
    
    bool new_word = true;
    
    for (size_t i = 0; i < str->byte_length; i++)
    {
        if (isspace((unsigned char)str->data[i]))
        {
            new_word = true;
        }
        else if (new_word)
        {
            // 单词首字母大写
            if (str->data[i] >= 'a' && str->data[i] <= 'z')
            {
                str->data[i] = str->data[i] - 'a' + 'A';
            }
            new_word = false;
        }
        else
        {
            // 单词非首字母小写
            if (str->data[i] >= 'A' && str->data[i] <= 'Z')
            {
                str->data[i] = str->data[i] - 'A' + 'a';
            }
        }
    }
    
    return true;
}

bool F_string_trim(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    if (str->byte_length == 0)
    {
        return true; // 空字符串，没有空白需要去除
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
    // 需要导入utf8_strlen函数
    extern size_t utf8_strlen(const char* utf8_str);
    str->length = utf8_strlen(str->data);
    
    return true;
}

bool F_string_ltrim(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    if (str->byte_length == 0)
    {
        return true; // 空字符串，没有空白需要去除
    }
    
    // 查找起始非空白字符
    size_t start = 0;
    while (start < str->byte_length && isspace((unsigned char)str->data[start]))
    {
        start++;
    }
    
    if (start == 0)
    {
        // 没有需要去除的空白字符
        return true;
    }
    
    // 计算新长度
    size_t new_byte_length = str->byte_length - start;
    
    // 移动数据
    memmove(str->data, str->data + start, new_byte_length);
    
    str->data[new_byte_length] = '\0';
    str->byte_length = new_byte_length;
    
    // 重新计算字符长度
    extern size_t utf8_strlen(const char* utf8_str);
    str->length = utf8_strlen(str->data);
    
    return true;
}

bool F_string_rtrim(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    if (str->byte_length == 0)
    {
        return true; // 空字符串，没有空白需要去除
    }
    
    // 查找结束非空白字符
    size_t end = str->byte_length;
    while (end > 0 && isspace((unsigned char)str->data[end - 1]))
    {
        end--;
    }
    
    if (end == str->byte_length)
    {
        // 没有需要去除的空白字符
        return true;
    }
    
    str->data[end] = '\0';
    str->byte_length = end;
    
    // 重新计算字符长度
    extern size_t utf8_strlen(const char* utf8_str);
    str->length = utf8_strlen(str->data);
    
    return true;
}

bool F_string_remove_whitespace(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    if (str->byte_length == 0)
    {
        return true; // 空字符串，没有空白需要去除
    }
    
    size_t write_pos = 0;
    
    for (size_t read_pos = 0; read_pos < str->byte_length; read_pos++)
    {
        if (!isspace((unsigned char)str->data[read_pos]))
        {
            str->data[write_pos] = str->data[read_pos];
            write_pos++;
        }
    }
    
    str->data[write_pos] = '\0';
    str->byte_length = write_pos;
    
    // 重新计算字符长度
    extern size_t utf8_strlen(const char* utf8_str);
    str->length = utf8_strlen(str->data);
    
    return true;
}

bool F_string_reverse(Stru_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    if (str->byte_length == 0)
    {
        return true; // 空字符串，无需操作
    }
    
    if (str->byte_length < 2)
    {
        return true; // 单字符字符串，无需操作
    }
    
    // 反转字符串（简单实现，不考虑UTF-8多字节字符）
    size_t left = 0;
    size_t right = str->byte_length - 1;
    
    while (left < right)
    {
        char temp = str->data[left];
        str->data[left] = str->data[right];
        str->data[right] = temp;
        
        left++;
        right--;
    }
    
    return true;
}

bool F_string_pad_left(Stru_String_t* str, size_t length, char fill_char)
{
    if (str == NULL)
    {
        return false;
    }
    
    if (str->length >= length)
    {
        return true; // 已经足够长
    }
    
    size_t padding_needed = length - str->length;
    
    // 确保有足够容量
    extern bool ensure_capacity(Stru_String_t* str, size_t additional_bytes);
    if (!ensure_capacity(str, padding_needed))
    {
        return false;
    }
    
    // 移动现有数据
    memmove(str->data + padding_needed, str->data, str->byte_length);
    
    // 填充字符
    for (size_t i = 0; i < padding_needed; i++)
    {
        str->data[i] = fill_char;
    }
    
    str->byte_length += padding_needed;
    str->length = length;
    str->data[str->byte_length] = '\0';
    
    return true;
}

bool F_string_pad_right(Stru_String_t* str, size_t length, char fill_char)
{
    if (str == NULL)
    {
        return false;
    }
    
    if (str->length >= length)
    {
        return true; // 已经足够长
    }
    
    size_t padding_needed = length - str->length;
    
    // 确保有足够容量
    extern bool ensure_capacity(Stru_String_t* str, size_t additional_bytes);
    if (!ensure_capacity(str, padding_needed))
    {
        return false;
    }
    
    // 填充字符
    for (size_t i = 0; i < padding_needed; i++)
    {
        str->data[str->byte_length + i] = fill_char;
    }
    
    str->byte_length += padding_needed;
    str->length = length;
    str->data[str->byte_length] = '\0';
    
    return true;
}

bool F_string_pad_both(Stru_String_t* str, size_t length, char fill_char)
{
    if (str == NULL)
    {
        return false;
    }
    
    if (str->length >= length)
    {
        return true; // 已经足够长
    }
    
    size_t padding_needed = length - str->length;
    size_t left_padding = padding_needed / 2;
    size_t right_padding = padding_needed - left_padding;
    
    // 确保有足够容量
    extern bool ensure_capacity(Stru_String_t* str, size_t additional_bytes);
    if (!ensure_capacity(str, padding_needed))
    {
        return false;
    }
    
    // 移动现有数据
    memmove(str->data + left_padding, str->data, str->byte_length);
    
    // 填充左侧字符
    for (size_t i = 0; i < left_padding; i++)
    {
        str->data[i] = fill_char;
    }
    
    // 填充右侧字符
    for (size_t i = 0; i < right_padding; i++)
    {
        str->data[str->byte_length + left_padding + i] = fill_char;
    }
    
    str->byte_length += padding_needed;
    str->length = length;
    str->data[str->byte_length] = '\0';
    
    return true;
}

bool F_string_to_ascii(Stru_String_t* str, char replacement)
{
    if (str == NULL)
    {
        return false;
    }
    
    // 导入UTF-8字符长度函数
    extern size_t utf8_char_len(unsigned char first_byte);
    
    size_t write_pos = 0;
    size_t read_pos = 0;
    
    while (read_pos < str->byte_length)
    {
        unsigned char first_byte = (unsigned char)str->data[read_pos];
        
        if (first_byte <= 127)
        {
            // ASCII字符，直接复制
            str->data[write_pos] = str->data[read_pos];
            write_pos++;
            read_pos++;
        }
        else
        {
            // 非ASCII字符（UTF-8多字节字符）
            // 用替换字符替换整个字符
            str->data[write_pos] = replacement;
            write_pos++;
            
            // 跳过整个UTF-8字符
            size_t char_len = utf8_char_len(first_byte);
            if (char_len == 0 || read_pos + char_len > str->byte_length)
            {
                // 无效的UTF-8字符，跳过1个字节
                read_pos++;
            }
            else
            {
                read_pos += char_len;
            }
        }
    }
    
    str->data[write_pos] = '\0';
    str->byte_length = write_pos;
    
    // 重新计算字符长度
    extern size_t utf8_strlen(const char* utf8_str);
    str->length = utf8_strlen(str->data);
    
    return true;
}
