/**
 * @file CN_string_search.c
 * @brief 字符串搜索模块实现文件
 * 
 * 实现字符串的搜索和替换功能。
 * 包括子字符串提取、查找、替换等操作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_string_search.h"
#include "../string_operations/CN_string_operations.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// 从核心模块导入内部函数
extern size_t utf8_strlen(const char* utf8_str);
extern size_t utf8_char_len(unsigned char first_byte);
extern bool ensure_capacity(Stru_String_t* str, size_t additional_bytes);

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
    Stru_String_t* substring = F_string_create_with_capacity(substring_byte_len + 1);
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

Stru_String_t* F_string_substring_bytes(Stru_String_t* str, size_t byte_start, size_t byte_length)
{
    if (str == NULL || byte_start >= str->byte_length || byte_length == 0)
    {
        return NULL;
    }
    
    // 计算实际要复制的字节数
    size_t actual_byte_length = byte_length;
    if (byte_start + byte_length > str->byte_length)
    {
        actual_byte_length = str->byte_length - byte_start;
    }
    
    // 创建新字符串
    Stru_String_t* substring = F_string_create_with_capacity(actual_byte_length + 1);
    if (substring == NULL)
    {
        return NULL;
    }
    
    // 复制子字符串数据
    memcpy(substring->data, str->data + byte_start, actual_byte_length);
    substring->data[actual_byte_length] = '\0';
    substring->byte_length = actual_byte_length;
    
    // 计算字符长度
    substring->length = utf8_strlen(substring->data);
    
    return substring;
}

Stru_String_t* F_string_left(Stru_String_t* str, size_t length)
{
    return F_string_substring(str, 0, length);
}

Stru_String_t* F_string_right(Stru_String_t* str, size_t length)
{
    if (str == NULL || length == 0 || str->length == 0)
    {
        return NULL;
    }
    
    size_t start = (length >= str->length) ? 0 : str->length - length;
    size_t actual_length = (length >= str->length) ? str->length : length;
    
    return F_string_substring(str, start, actual_length);
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
    
    // 计算起始字节位置
    size_t start_byte_pos = 0;
    size_t char_pos = 0;
    while (char_pos < start_pos && start_byte_pos < str->byte_length)
    {
        size_t char_len = utf8_char_len((unsigned char)str->data[start_byte_pos]);
        start_byte_pos += char_len;
        char_pos++;
    }
    
    // 从起始字节位置开始查找子字符串
    const char* found = strstr(str_cstr + start_byte_pos, substr_cstr);
    if (found == NULL)
    {
        return -1;
    }
    
    // 计算字符位置（不是字节位置）
    size_t byte_offset = found - str_cstr;
    char_pos = 0;
    size_t byte_pos = 0;
    
    while (byte_pos < byte_offset && byte_pos < str->byte_length)
    {
        size_t char_len = utf8_char_len((unsigned char)str->data[byte_pos]);
        byte_pos += char_len;
        char_pos++;
    }
    
    return (int64_t)char_pos;
}

int64_t F_string_find_cstr(Stru_String_t* str, const char* csubstr, size_t start_pos)
{
    if (str == NULL || csubstr == NULL || start_pos >= str->length)
    {
        return -1;
    }
    
    // 转换为C字符串以便使用strstr
    const char* str_cstr = F_string_cstr(str);
    
    if (str_cstr == NULL)
    {
        return -1;
    }
    
    // 计算起始字节位置
    size_t start_byte_pos = 0;
    size_t char_pos = 0;
    while (char_pos < start_pos && start_byte_pos < str->byte_length)
    {
        size_t char_len = utf8_char_len((unsigned char)str->data[start_byte_pos]);
        start_byte_pos += char_len;
        char_pos++;
    }
    
    // 从起始字节位置开始查找子字符串
    const char* found = strstr(str_cstr + start_byte_pos, csubstr);
    if (found == NULL)
    {
        return -1;
    }
    
    // 计算字符位置（不是字节位置）
    size_t byte_offset = found - str_cstr;
    char_pos = 0;
    size_t byte_pos = 0;
    
    while (byte_pos < byte_offset && byte_pos < str->byte_length)
    {
        size_t char_len = utf8_char_len((unsigned char)str->data[byte_pos]);
        byte_pos += char_len;
        char_pos++;
    }
    
    return (int64_t)char_pos;
}

int64_t F_string_rfind(Stru_String_t* str, Stru_String_t* substr, size_t start_pos)
{
    if (str == NULL || substr == NULL || str->length == 0 || substr->length == 0)
    {
        return -1;
    }
    
    // 简单实现：从后向前查找
    // 注意：这不是最优实现，但对于大多数情况足够
    const char* str_cstr = F_string_cstr(str);
    const char* substr_cstr = F_string_cstr(substr);
    
    if (str_cstr == NULL || substr_cstr == NULL)
    {
        return -1;
    }
    
    size_t str_len = strlen(str_cstr);
    size_t substr_len = strlen(substr_cstr);
    
    if (substr_len > str_len)
    {
        return -1;
    }
    
    // 处理start_pos参数（目前未使用，保持接口兼容性）
    (void)start_pos; // 标记为未使用以避免警告
    
    // 从后向前查找
    // 注意：使用int64_t来处理i可能为负数的情况
    for (int64_t i = (int64_t)(str_len - substr_len); i >= 0; i--)
    {
        if (strncmp(str_cstr + i, substr_cstr, substr_len) == 0)
        {
            // 计算字符位置
            size_t char_pos = 0;
            size_t byte_pos = 0;
            
            while (byte_pos < (size_t)i && byte_pos < str->byte_length)
            {
                size_t char_len = utf8_char_len((unsigned char)str->data[byte_pos]);
                byte_pos += char_len;
                char_pos++;
            }
            
            return (int64_t)char_pos;
        }
    }
    
    return -1;
}

int64_t F_string_find_char(Stru_String_t* str, char ch, size_t start_pos)
{
    if (str == NULL || start_pos >= str->length)
    {
        return -1;
    }
    
    // 查找起始字节位置
    size_t byte_pos = 0;
    size_t char_pos = 0;
    while (char_pos < start_pos && byte_pos < str->byte_length)
    {
        size_t char_len = utf8_char_len((unsigned char)str->data[byte_pos]);
        byte_pos += char_len;
        char_pos++;
    }
    
    // 查找字符
    while (byte_pos < str->byte_length)
    {
        if (str->data[byte_pos] == ch)
        {
            return (int64_t)char_pos;
        }
        
        size_t char_len = utf8_char_len((unsigned char)str->data[byte_pos]);
        byte_pos += char_len;
        char_pos++;
    }
    
    return -1;
}

int64_t F_string_rfind_char(Stru_String_t* str, char ch, size_t start_pos)
{
    if (str == NULL || str->length == 0)
    {
        return -1;
    }
    
    const char* str_cstr = F_string_cstr(str);
    
    if (str_cstr == NULL)
    {
        return -1;
    }
    
    // 处理start_pos参数：如果start_pos >= str->length，从末尾开始
    // 注意：start_pos是字符位置，不是字节位置
    size_t start_char_pos = start_pos;
    if (start_pos >= str->length || start_pos == (size_t)-1)
    {
        start_char_pos = str->length - 1;
    }
    
    // 计算起始字节位置
    size_t start_byte_pos = 0;
    size_t char_pos = 0;
    while (char_pos < start_char_pos && start_byte_pos < str->byte_length)
    {
        size_t char_len = utf8_char_len((unsigned char)str->data[start_byte_pos]);
        start_byte_pos += char_len;
        char_pos++;
    }
    
    // 从起始字节位置向前查找
    // 注意：使用int64_t来处理i可能为负数的情况
    for (int64_t i = (int64_t)start_byte_pos; i >= 0; i--)
    {
        if (str_cstr[i] == ch)
        {
            // 计算字符位置
            char_pos = 0;
            size_t byte_pos = 0;
            
            while (byte_pos < (size_t)i && byte_pos < str->byte_length)
            {
                size_t char_len = utf8_char_len((unsigned char)str->data[byte_pos]);
                byte_pos += char_len;
                char_pos++;
            }
            
            return (int64_t)char_pos;
        }
    }
    
    return -1;
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
    F_string_destroy(before);
    if (after != NULL)
    {
        F_string_destroy(after);
    }
    
    return success;
}

bool F_string_replace_cstr(Stru_String_t* str, const char* old_csubstr, const char* new_csubstr)
{
    if (str == NULL || old_csubstr == NULL || new_csubstr == NULL)
    {
        return false;
    }
    
    // 创建临时字符串
    Stru_String_t* old_substr = F_string_create_from_cstr(old_csubstr);
    Stru_String_t* new_substr = F_string_create_from_cstr(new_csubstr);
    
    if (old_substr == NULL || new_substr == NULL)
    {
        if (old_substr != NULL) F_string_destroy(old_substr);
        if (new_substr != NULL) F_string_destroy(new_substr);
        return false;
    }
    
    // 调用字符串版本
    bool result = F_string_replace(str, old_substr, new_substr);
    
    // 清理
    F_string_destroy(old_substr);
    F_string_destroy(new_substr);
    
    return result;
}

int64_t F_string_replace_all(Stru_String_t* str, Stru_String_t* old_substr, Stru_String_t* new_substr)
{
    if (str == NULL || old_substr == NULL || new_substr == NULL)
    {
        return -1;
    }
    
    // 如果旧子字符串为空或原字符串为空，直接返回0
    if (old_substr->length == 0 || str->length == 0)
    {
        return 0;
    }
    
    // 创建结果字符串
    Stru_String_t* result = F_string_create_empty();
    if (result == NULL)
    {
        return -1;
    }
    
    int64_t replace_count = 0;
    size_t current_pos = 0;
    
    while (current_pos < str->length)
    {
        // 从当前位置查找
        int64_t pos = F_string_find(str, old_substr, current_pos);
        if (pos < 0)
        {
            // 没有更多匹配，复制剩余部分
            Stru_String_t* remaining = F_string_substring(str, current_pos, str->length - current_pos);
            if (remaining != NULL)
            {
                F_string_append(result, remaining);
                F_string_destroy(remaining);
            }
            break;
        }
        
        // 复制当前位置到匹配位置之间的部分
        if ((size_t)pos > current_pos)
        {
            Stru_String_t* between = F_string_substring(str, current_pos, (size_t)pos - current_pos);
            if (between != NULL)
            {
                F_string_append(result, between);
                F_string_destroy(between);
            }
        }
        
        // 添加新子字符串
        F_string_append(result, new_substr);
        replace_count++;
        
        // 跳过旧子字符串
        current_pos = (size_t)pos + old_substr->length;
    }
    
    // 如果整个字符串都没有匹配，复制整个字符串
    if (replace_count == 0 && current_pos == 0)
    {
        F_string_append(result, str);
    }
    
    // 用结果替换原字符串
    F_string_clear(str);
    bool success = F_string_append(str, result);
    
    // 清理
    F_string_destroy(result);
    
    return success ? replace_count : -1;
}

int64_t F_string_replace_all_cstr(Stru_String_t* str, const char* old_csubstr, const char* new_csubstr)
{
    if (str == NULL || old_csubstr == NULL || new_csubstr == NULL)
    {
        return -1;
    }
    
    // 创建临时字符串
    Stru_String_t* old_substr = F_string_create_from_cstr(old_csubstr);
    Stru_String_t* new_substr = F_string_create_from_cstr(new_csubstr);
    
    if (old_substr == NULL || new_substr == NULL)
    {
        if (old_substr != NULL) F_string_destroy(old_substr);
        if (new_substr != NULL) F_string_destroy(new_substr);
        return -1;
    }
    
    // 调用字符串版本
    int64_t result = F_string_replace_all(str, old_substr, new_substr);
    
    // 清理
    F_string_destroy(old_substr);
    F_string_destroy(new_substr);
    
    return result;
}
