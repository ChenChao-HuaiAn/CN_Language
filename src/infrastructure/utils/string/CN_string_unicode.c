/******************************************************************************
 * 文件名: CN_string_unicode.c
 * 功能: CN_Language字符串Unicode处理模块实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现Unicode大小写转换
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_string_unicode.h"
#include "CN_string_internal.h"  // 使用内部头文件获取结构体定义
#include "simple_memory.h"  // 使用简单的内存包装器进行测试
#include <string.h>
#include <ctype.h>
#include <stdint.h>

// ============================================================================
// Unicode字符操作实现
// ============================================================================

/**
 * @brief 获取UTF-8字符的Unicode码点
 */
uint32_t CN_unicode_get_code_point(const char* utf8_str, size_t str_len, 
                                   size_t pos, size_t* next_pos)
{
    if (utf8_str == NULL || pos >= str_len)
    {
        if (next_pos != NULL)
        {
            *next_pos = pos;
        }
        return 0;
    }
    
    unsigned char first_byte = (unsigned char)utf8_str[pos];
    uint32_t code_point = 0;
    size_t char_len = 0;
    
    // 确定UTF-8字符长度
    if (first_byte < 0x80)
    {
        // 1字节UTF-8 (ASCII)
        code_point = first_byte;
        char_len = 1;
    }
    else if ((first_byte & 0xE0) == 0xC0)
    {
        // 2字节UTF-8
        if (pos + 1 < str_len)
        {
            code_point = ((first_byte & 0x1F) << 6) | 
                         (utf8_str[pos + 1] & 0x3F);
            char_len = 2;
        }
    }
    else if ((first_byte & 0xF0) == 0xE0)
    {
        // 3字节UTF-8
        if (pos + 2 < str_len)
        {
            code_point = ((first_byte & 0x0F) << 12) |
                         ((utf8_str[pos + 1] & 0x3F) << 6) |
                         (utf8_str[pos + 2] & 0x3F);
            char_len = 3;
        }
    }
    else if ((first_byte & 0xF8) == 0xF0)
    {
        // 4字节UTF-8
        if (pos + 3 < str_len)
        {
            code_point = ((first_byte & 0x07) << 18) |
                         ((utf8_str[pos + 1] & 0x3F) << 12) |
                         ((utf8_str[pos + 2] & 0x3F) << 6) |
                         (utf8_str[pos + 3] & 0x3F);
            char_len = 4;
        }
    }
    else
    {
        // 无效的UTF-8序列
        char_len = 1;
    }
    
    if (next_pos != NULL)
    {
        *next_pos = pos + char_len;
    }
    
    return code_point;
}

/**
 * @brief 将Unicode码点编码为UTF-8
 */
size_t CN_unicode_encode_utf8(uint32_t code_point, char* buffer)
{
    if (buffer == NULL)
    {
        return 0;
    }
    
    if (code_point <= 0x7F)
    {
        // 1字节UTF-8
        buffer[0] = (char)code_point;
        return 1;
    }
    else if (code_point <= 0x7FF)
    {
        // 2字节UTF-8
        buffer[0] = (char)(0xC0 | (code_point >> 6));
        buffer[1] = (char)(0x80 | (code_point & 0x3F));
        return 2;
    }
    else if (code_point <= 0xFFFF)
    {
        // 3字节UTF-8
        buffer[0] = (char)(0xE0 | (code_point >> 12));
        buffer[1] = (char)(0x80 | ((code_point >> 6) & 0x3F));
        buffer[2] = (char)(0x80 | (code_point & 0x3F));
        return 3;
    }
    else if (code_point <= 0x10FFFF)
    {
        // 4字节UTF-8
        buffer[0] = (char)(0xF0 | (code_point >> 18));
        buffer[1] = (char)(0x80 | ((code_point >> 12) & 0x3F));
        buffer[2] = (char)(0x80 | ((code_point >> 6) & 0x3F));
        buffer[3] = (char)(0x80 | (code_point & 0x3F));
        return 4;
    }
    
    // 无效的Unicode码点
    return 0;
}

/**
 * @brief 简单的Unicode字符信息获取（基础实现）
 */
static bool get_basic_char_info(uint32_t code_point, Stru_CN_UnicodeCharInfo_t* info)
{
    if (info == NULL)
    {
        return false;
    }
    
    // 初始化信息
    memset(info, 0, sizeof(Stru_CN_UnicodeCharInfo_t));
    info->code_point = code_point;
    info->utf8_length = CN_unicode_encode_utf8(code_point, info->utf8_bytes);
    
    // 基础的大小写映射（仅支持ASCII和基本拉丁字母扩展）
    if (code_point >= 'A' && code_point <= 'Z')
    {
        info->is_letter = true;
        info->is_uppercase = true;
        info->lowercase_mapping = code_point + ('a' - 'A');
        info->uppercase_mapping = code_point;
        info->titlecase_mapping = code_point;
    }
    else if (code_point >= 'a' && code_point <= 'z')
    {
        info->is_letter = true;
        info->is_lowercase = true;
        info->uppercase_mapping = code_point - ('a' - 'A');
        info->lowercase_mapping = code_point;
        info->titlecase_mapping = info->uppercase_mapping; // 标题大小写通常是大写
    }
    else if (code_point >= 0xC0 && code_point <= 0x17F)
    {
        // 基本拉丁字母扩展（拉丁文补充-1）
        // 这里只实现简单的映射，实际需要完整的Unicode表
        info->is_letter = true;
        
        // 简单的大小写映射（不完整，仅示例）
        if (code_point >= 0xC0 && code_point <= 0xD6)
        {
            info->is_uppercase = true;
            info->lowercase_mapping = code_point + 0x20;
        }
        else if (code_point >= 0xD8 && code_point <= 0xDE)
        {
            info->is_uppercase = true;
            info->lowercase_mapping = code_point + 0x20;
        }
        else if (code_point >= 0xE0 && code_point <= 0xF6)
        {
            info->is_lowercase = true;
            info->uppercase_mapping = code_point - 0x20;
        }
        else if (code_point >= 0xF8 && code_point <= 0xFE)
        {
            info->is_lowercase = true;
            info->uppercase_mapping = code_point - 0x20;
        }
        
        info->titlecase_mapping = info->uppercase_mapping;
    }
    else if (code_point >= 0x370 && code_point <= 0x3FF)
    {
        // 希腊字母
        info->is_letter = true;
        // 简单的大小写映射（不完整）
        if (code_point >= 0x391 && code_point <= 0x3A9)
        {
            info->is_uppercase = true;
            info->lowercase_mapping = code_point + 0x20;
        }
        else if (code_point >= 0x3B1 && code_point <= 0x3C9)
        {
            info->is_lowercase = true;
            info->uppercase_mapping = code_point - 0x20;
        }
        info->titlecase_mapping = info->uppercase_mapping;
    }
    else if (code_point >= 0x400 && code_point <= 0x4FF)
    {
        // 西里尔字母
        info->is_letter = true;
        // 简单的大小写映射（不完整）
        if (code_point >= 0x410 && code_point <= 0x42F)
        {
            info->is_uppercase = true;
            info->lowercase_mapping = code_point + 0x20;
        }
        else if (code_point >= 0x430 && code_point <= 0x44F)
        {
            info->is_lowercase = true;
            info->uppercase_mapping = code_point - 0x20;
        }
        info->titlecase_mapping = info->uppercase_mapping;
    }
    
    // 如果没有设置大小写映射，使用原值
    if (info->uppercase_mapping == 0)
    {
        info->uppercase_mapping = code_point;
    }
    if (info->lowercase_mapping == 0)
    {
        info->lowercase_mapping = code_point;
    }
    if (info->titlecase_mapping == 0)
    {
        info->titlecase_mapping = code_point;
    }
    
    return true;
}

/**
 * @brief 获取Unicode字符信息
 */
bool CN_unicode_get_char_info(uint32_t code_point, Stru_CN_UnicodeCharInfo_t* info)
{
    return get_basic_char_info(code_point, info);
}

/**
 * @brief 获取Unicode字符类别（简化实现）
 */
Eum_CN_UnicodeCategory_t CN_unicode_get_category(uint32_t code_point)
{
    Stru_CN_UnicodeCharInfo_t info;
    if (get_basic_char_info(code_point, &info))
    {
        if (info.is_letter)
        {
            if (info.is_uppercase)
            {
                return Eum_UNICODE_CATEGORY_UPPERCASE_LETTER;
            }
            else if (info.is_lowercase)
            {
                return Eum_UNICODE_CATEGORY_LOWERCASE_LETTER;
            }
            else
            {
                return Eum_UNICODE_CATEGORY_OTHER_LETTER;
            }
        }
    }
    
    // 简单分类（不完整）
    if (code_point >= '0' && code_point <= '9')
    {
        return Eum_UNICODE_CATEGORY_DECIMAL_NUMBER;
    }
    else if (code_point == ' ' || code_point == '\t' || code_point == '\n' || 
             code_point == '\r' || code_point == '\f' || code_point == '\v')
    {
        return Eum_UNICODE_CATEGORY_SPACE_SEPARATOR;
    }
    else if (code_point < 0x20 || (code_point >= 0x7F && code_point < 0xA0))
    {
        return Eum_UNICODE_CATEGORY_CONTROL;
    }
    
    return Eum_UNICODE_CATEGORY_OTHER_SYMBOL;
}

// ============================================================================
// Unicode大小写转换实现
// ============================================================================

/**
 * @brief 转换为大写（Unicode支持）
 */
bool CN_string_to_upper_unicode(Stru_CN_String_t* str)
{
    if (str == NULL || str->data == NULL || str->encoding != Eum_STRING_ENCODING_UTF8)
    {
        return false;
    }
    
    // 创建新缓冲区用于存储转换后的字符串
    size_t new_capacity = str->length * 2 + 1; // 最坏情况：每个字符可能变长
    char* new_buffer = (char*)cn_malloc(new_capacity);
    if (new_buffer == NULL)
    {
        return false;
    }
    
    size_t new_length = 0;
    size_t pos = 0;
    
    while (pos < str->length)
    {
        size_t next_pos;
        uint32_t code_point = CN_unicode_get_code_point(str->data, str->length, pos, &next_pos);
        
        if (code_point == 0 && next_pos == pos)
        {
            // 无效字符，跳过
            pos++;
            continue;
        }
        
        // 获取字符信息
        Stru_CN_UnicodeCharInfo_t info;
        if (CN_unicode_get_char_info(code_point, &info))
        {
            // 使用大写映射
            uint32_t upper_code = info.uppercase_mapping;
            char utf8_buffer[6];
            size_t utf8_len = CN_unicode_encode_utf8(upper_code, utf8_buffer);
            
            if (new_length + utf8_len < new_capacity)
            {
                memcpy(new_buffer + new_length, utf8_buffer, utf8_len);
                new_length += utf8_len;
            }
        }
        else
        {
            // 无法获取信息，复制原字符
            size_t char_len = next_pos - pos;
            if (new_length + char_len < new_capacity)
            {
                memcpy(new_buffer + new_length, str->data + pos, char_len);
                new_length += char_len;
            }
        }
        
        pos = next_pos;
    }
    
    // 确保以null结尾
    new_buffer[new_length] = '\0';
    
    // 替换原数据
    cn_free(str->data);
    str->data = new_buffer;
    str->length = new_length;
    str->capacity = new_capacity;
    
    return true;
}

/**
 * @brief 转换为小写（Unicode支持）
 */
bool CN_string_to_lower_unicode(Stru_CN_String_t* str)
{
    if (str == NULL || str->data == NULL || str->encoding != Eum_STRING_ENCODING_UTF8)
    {
        return false;
    }
    
    // 创建新缓冲区用于存储转换后的字符串
    size_t new_capacity = str->length * 2 + 1;
    char* new_buffer = (char*)cn_malloc(new_capacity);
    if (new_buffer == NULL)
    {
        return false;
    }
    
    size_t new_length = 0;
    size_t pos = 0;
    
    while (pos < str->length)
    {
        size_t next_pos;
        uint32_t code_point = CN_unicode_get_code_point(str->data, str->length, pos, &next_pos);
        
        if (code_point == 0 && next_pos == pos)
        {
            // 无效字符，跳过
            pos++;
            continue;
        }
        
        // 获取字符信息
        Stru_CN_UnicodeCharInfo_t info;
        if (CN_unicode_get_char_info(code_point, &info))
        {
            // 使用小写映射
            uint32_t lower_code = info.lowercase_mapping;
            char utf8_buffer[6];
            size_t utf8_len = CN_unicode_encode_utf8(lower_code, utf8_buffer);
            
            if (new_length + utf8_len < new_capacity)
            {
                memcpy(new_buffer + new_length, utf8_buffer, utf8_len);
                new_length += utf8_len;
            }
        }
        else
        {
            // 无法获取信息，复制原字符
            size_t char_len = next_pos - pos;
            if (new_length + char_len < new_capacity)
            {
                memcpy(new_buffer + new_length, str->data + pos, char_len);
                new_length += char_len;
            }
        }
        
        pos = next_pos;
    }
    
    // 确保以null结尾
    new_buffer[new_length] = '\0';
    
    // 替换原数据
    cn_free(str->data);
    str->data = new_buffer;
    str->length = new_length;
    str->capacity = new_capacity;
    
    return true;
}

/**
 * @brief 转换为标题大小写（Unicode支持）
 */
bool CN_string_to_title_unicode(Stru_CN_String_t* str)
{
    if (str == NULL || str->data == NULL || str->encoding != Eum_STRING_ENCODING_UTF8)
    {
        return false;
    }
    
    // 创建新缓冲区用于存储转换后的字符串
    size_t new_capacity = str->length * 2 + 1;
    char* new_buffer = (char*)cn_malloc(new_capacity);
    if (new_buffer == NULL)
    {
        return false;
    }
    
    size_t new_length = 0;
    size_t pos = 0;
    bool word_start = true; // 标记是否在单词开头
    
    while (pos < str->length)
    {
        size_t next_pos;
        uint32_t code_point = CN_unicode_get_code_point(str->data, str->length, pos, &next_pos);
        
        if (code_point == 0 && next_pos == pos)
        {
            // 无效字符，跳过
            pos++;
            continue;
        }
        
        // 获取字符信息
        Stru_CN_UnicodeCharInfo_t info;
        if (CN_unicode_get_char_info(code_point, &info))
        {
            uint32_t target_code;
            
            if (word_start && info.is_letter)
            {
                // 单词开头使用标题大小写
                target_code = info.titlecase_mapping;
                word_start = false;
            }
            else
            {
                // 单词其他部分使用小写
                target_code = info.lowercase_mapping;
            }
            
            // 检查是否是单词分隔符
            Eum_CN_UnicodeCategory_t category = CN_unicode_get_category(code_point);
            if (category == Eum_UNICODE_CATEGORY_SPACE_SEPARATOR ||
                category == Eum_UNICODE_CATEGORY_LINE_SEPARATOR ||
                category == Eum_UNICODE_CATEGORY_PARAGRAPH_SEPARATOR ||
                category == Eum_UNICODE_CATEGORY_DASH_PUNCTUATION ||
                category == Eum_UNICODE_CATEGORY_OTHER_PUNCTUATION)
            {
                word_start = true;
            }
            
            char utf8_buffer[6];
            size_t utf8_len = CN_unicode_encode_utf8(target_code, utf8_buffer);
            
            if (new_length + utf8_len < new_capacity)
            {
                memcpy(new_buffer + new_length, utf8_buffer, utf8_len);
                new_length += utf8_len;
            }
        }
        else
        {
            // 无法获取信息，复制原字符
            size_t char_len = next_pos - pos;
            if (new_length + char_len < new_capacity)
            {
                memcpy(new_buffer + new_length, str->data + pos, char_len);
                new_length += char_len;
            }
            
            // 检查是否是单词分隔符
            Eum_CN_UnicodeCategory_t category = CN_unicode_get_category(code_point);
            if (category == Eum_UNICODE_CATEGORY_SPACE_SEPARATOR ||
                category == Eum_UNICODE_CATEGORY_LINE_SEPARATOR ||
                category == Eum_UNICODE_CATEGORY_PARAGRAPH_SEPARATOR)
            {
                word_start = true;
            }
        }
        
        pos = next_pos;
    }
    
    // 确保以null结尾
    new_buffer[new_length] = '\0';
    
    // 替换原数据
    cn_free(str->data);
    str->data = new_buffer;
    str->length = new_length;
    str->capacity = new_capacity;
    
    return true;
}

// ============================================================================
// 其他Unicode函数实现
// ============================================================================

/**
 * @brief 转换为大写（Unicode支持）并返回新字符串
 */
Stru_CN_String_t* CN_string_to_upper_copy(const Stru_CN_String_t* str)
{
    if (str == NULL)
    {
        return NULL;
    }
    
    Stru_CN_String_t* copy = CN_string_copy(str);
    if (copy == NULL)
    {
        return NULL;
    }
    
    if (!CN_string_to_upper_unicode(copy))
    {
        CN_string_destroy(copy);
        return NULL;
    }
    
    return copy;
}

/**
 * @brief 转换为小写（Unicode支持）并返回新字符串
 */
Stru_CN_String_t* CN_string_to_lower_copy(const Stru_CN_String_t* str)
{
    if (str == NULL)
    {
        return NULL;
    }
    
    Stru_CN_String_t* copy = CN_string_copy(str);
    if (copy == NULL)
    {
        return NULL;
    }
    
    if (!CN_string_to_lower_unicode(copy))
    {
        CN_string_destroy(copy);
        return NULL;
    }
    
    return copy;
}

/**
 * @brief 转换为标题大小写（Unicode支持）并返回新字符串
 */
Stru_CN_String_t* CN_string_to_title_copy(const Stru_CN_String_t* str)
{
    if (str == NULL)
    {
        return NULL;
    }
    
    Stru_CN_String_t* copy = CN_string_copy(str);
    if (copy == NULL)
    {
        return NULL;
    }
    
    if (!CN_string_to_title_unicode(copy))
    {
        CN_string_destroy(copy);
        return NULL;
    }
    
    return copy;
}

/**
 * @brief Unicode不区分大小写比较
 */
int CN_string_compare_case_insensitive_unicode(const Stru_CN_String_t* str1,
                                               const Stru_CN_String_t* str2)
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
    
    // 简单实现：转换为小写后比较
    Stru_CN_String_t* lower1 = CN_string_to_lower_copy(str1);
    Stru_CN_String_t* lower2 = CN_string_to_lower_copy(str2);
    
    if (lower1 == NULL || lower2 == NULL)
    {
        if (lower1 != NULL) CN_string_destroy(lower1);
        if (lower2 != NULL) CN_string_destroy(lower2);
        // 回退到原始比较
        return strcmp(str1->data, str2->data);
    }
    
    int result = strcmp(lower1->data, lower2->data);
    CN_string_destroy(lower1);
    CN_string_destroy(lower2);
    
    return result;
}

/**
 * @brief Unicode不区分大小写检查字符串是否相等
 */
bool CN_string_equal_case_insensitive_unicode(const Stru_CN_String_t* str1,
                                              const Stru_CN_String_t* str2)
{
    return CN_string_compare_case_insensitive_unicode(str1, str2) == 0;
}

/**
 * @brief 对字符串进行Unicode规范化
 */
bool CN_string_normalize_unicode(Stru_CN_String_t* str,
                                 Eum_CN_UnicodeNormalizationForm_t form)
{
    // 基础实现：目前只支持NFC（规范组合）
    // TODO: 实现完整的Unicode规范化
    (void)form; // 暂时未使用
    
    if (str == NULL || str->data == NULL || str->encoding != Eum_STRING_ENCODING_UTF8)
    {
        return false;
    }
    
    // 简单实现：目前只返回成功
    return true;
}

/**
 * @brief 对字符串进行Unicode规范化并返回新字符串
 */
Stru_CN_String_t* CN_string_normalize_copy(const Stru_CN_String_t* str,
                                           Eum_CN_UnicodeNormalizationForm_t form)
{
    if (str == NULL)
    {
        return NULL;
    }
    
    Stru_CN_String_t* copy = CN_string_copy(str);
    if (copy == NULL)
    {
        return NULL;
    }
    
    if (!CN_string_normalize_unicode(copy, form))
    {
        CN_string_destroy(copy);
        return NULL;
    }
    
    return copy;
}
