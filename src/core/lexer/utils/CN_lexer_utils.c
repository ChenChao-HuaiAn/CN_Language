/**
 * @file CN_lexer_utils.c
 * @brief CN_Language 词法分析器工具函数实现
 * 
 * 实现词法分析器所需的通用工具函数，包括字符分类、字符串处理等。
 * 这些函数被多个子模块共享使用。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include "CN_lexer_utils.h"
#include <string.h>
#include <ctype.h>

/**
 * @brief 检查字符是否为空白字符
 */
bool F_is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/**
 * @brief 检查字符是否为字母（包括中文字符）
 */
bool F_is_alpha_utf8(char c)
{
    // 检查ASCII字母
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
    {
        return true;
    }
    
    // 检查中文字符（UTF-8编码的第一个字节）
    // 中文字符的UTF-8编码第一个字节在0xE0-0xEF范围内
    if ((unsigned char)c >= 0xE0 && (unsigned char)c <= 0xEF)
    {
        return true;
    }
    
    return false;
}

/**
 * @brief 检查字符是否为数字
 */
bool F_is_digit_simple(char c)
{
    return c >= '0' && c <= '9';
}

/**
 * @brief 检查字符是否为十六进制数字
 */
bool F_is_hex_digit(char c)
{
    return (c >= '0' && c <= '9') || 
           (c >= 'a' && c <= 'f') || 
           (c >= 'A' && c <= 'F');
}

/**
 * @brief 检查字符是否为字母或数字
 */
bool F_is_alnum(char c)
{
    return F_is_alpha_utf8(c) || F_is_digit_simple(c);
}

/**
 * @brief 检查字符是否为运算符字符
 */
bool F_is_operator_char_simple(char c)
{
    return strchr("+-*/%=!<>", c) != NULL;
}

/**
 * @brief 检查字符是否为分隔符字符
 */
bool F_is_delimiter_char_simple(char c)
{
    return strchr(",;:(){}[]", c) != NULL;
}

/**
 * @brief 安全复制字符串
 */
size_t F_safe_strcpy(char* dest, const char* src, size_t dest_size)
{
    if (dest == NULL || src == NULL || dest_size == 0)
    {
        return 0;
    }
    
    size_t i = 0;
    while (i < dest_size - 1 && src[i] != '\0')
    {
        dest[i] = src[i];
        i++;
    }
    
    dest[i] = '\0';
    return i;
}

/**
 * @brief 安全连接字符串
 */
size_t F_safe_strcat(char* dest, const char* src, size_t dest_size)
{
    if (dest == NULL || src == NULL || dest_size == 0)
    {
        return 0;
    }
    
    // 找到目标字符串的结尾
    size_t dest_len = 0;
    while (dest_len < dest_size && dest[dest_len] != '\0')
    {
        dest_len++;
    }
    
    if (dest_len >= dest_size)
    {
        return dest_len;
    }
    
    // 连接源字符串
    size_t i = 0;
    while (dest_len + i < dest_size - 1 && src[i] != '\0')
    {
        dest[dest_len + i] = src[i];
        i++;
    }
    
    dest[dest_len + i] = '\0';
    return dest_len + i;
}

/**
 * @brief 检查字符串是否以指定前缀开头
 */
bool F_starts_with(const char* str, const char* prefix)
{
    if (str == NULL || prefix == NULL)
    {
        return false;
    }
    
    while (*prefix != '\0')
    {
        if (*str != *prefix)
        {
            return false;
        }
        str++;
        prefix++;
    }
    
    return true;
}

/**
 * @brief 检查字符串是否以指定后缀结尾
 */
bool F_ends_with(const char* str, const char* suffix)
{
    if (str == NULL || suffix == NULL)
    {
        return false;
    }
    
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    
    if (suffix_len > str_len)
    {
        return false;
    }
    
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

/**
 * @brief 跳过字符串中的空白字符
 */
const char* F_skip_whitespace_str(const char* str)
{
    if (str == NULL)
    {
        return NULL;
    }
    
    while (*str != '\0' && F_is_whitespace(*str))
    {
        str++;
    }
    
    return str;
}

/**
 * @brief 计算字符串长度（安全版本）
 */
size_t F_safe_strlen(const char* str, size_t max_len)
{
    if (str == NULL)
    {
        return 0;
    }
    
    size_t len = 0;
    while (len < max_len && str[len] != '\0')
    {
        len++;
    }
    
    return len;
}
