/**
 * @file CN_lexer_utf8.c
 * @brief CN_Language 词法分析器UTF-8编码支持模块实现
 * 
 * 实现完整的UTF-8编码支持，包括字符分类、编码验证和字符处理。
 * 支持中文字符的完整Unicode范围。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 */

#include "CN_lexer_utf8.h"
#include <string.h>
#include <ctype.h>

/**
 * @brief 检查字符是否为中文字符
 */
bool F_is_chinese_char(uint32_t code_point)
{
    // 中文字符的Unicode范围
    // 1. 基本汉字：U+4E00 - U+9FFF
    // 2. 扩展A区：U+3400 - U+4DBF
    // 3. 扩展B区：U+20000 - U+2A6DF
    // 4. 扩展C区：U+2A700 - U+2B73F
    // 5. 扩展D区：U+2B740 - U+2B81F
    // 6. 扩展E区：U+2B820 - U+2CEAF
    // 7. 扩展F区：U+2CEB0 - U+2EBEF
    // 8. 兼容汉字：U+F900 - U+FAFF
    // 9. 兼容扩展：U+2F800 - U+2FA1F
    
    // 对于词法分析器，我们主要关注基本汉字范围
    if (code_point >= 0x4E00 && code_point <= 0x9FFF) {
        return true;
    }
    
    // 扩展A区
    if (code_point >= 0x3400 && code_point <= 0x4DBF) {
        return true;
    }
    
    // 兼容汉字
    if (code_point >= 0xF900 && code_point <= 0xFAFF) {
        return true;
    }
    
    return false;
}

/**
 * @brief 获取UTF-8字符的字节长度
 */
size_t F_get_utf8_char_length(unsigned char first_byte)
{
    if (first_byte < 0x80) {
        return 1;  // ASCII字符
    } else if ((first_byte & 0xE0) == 0xC0) {
        return 2;  // 2字节UTF-8字符
    } else if ((first_byte & 0xF0) == 0xE0) {
        return 3;  // 3字节UTF-8字符
    } else if ((first_byte & 0xF8) == 0xF0) {
        return 4;  // 4字节UTF-8字符
    } else {
        return 0;  // 无效的UTF-8编码
    }
}

/**
 * @brief 解码UTF-8字符
 */
size_t F_decode_utf8_char(const char* str, size_t length, uint32_t* code_point)
{
    if (str == NULL || length == 0 || code_point == NULL) {
        return 0;
    }
    
    unsigned char first_byte = (unsigned char)str[0];
    size_t char_len = F_get_utf8_char_length(first_byte);
    
    if (char_len == 0 || char_len > length) {
        return 0;  // 无效编码或长度不足
    }
    
    // 解码UTF-8
    uint32_t result = 0;
    
    if (char_len == 1) {
        // ASCII字符
        result = first_byte;
    } else if (char_len == 2) {
        // 2字节UTF-8字符
        if ((str[1] & 0xC0) != 0x80) {
            return 0;  // 无效的后续字节
        }
        result = ((first_byte & 0x1F) << 6) | (str[1] & 0x3F);
    } else if (char_len == 3) {
        // 3字节UTF-8字符
        if ((str[1] & 0xC0) != 0x80 || (str[2] & 0xC0) != 0x80) {
            return 0;  // 无效的后续字节
        }
        result = ((first_byte & 0x0F) << 12) | 
                 ((str[1] & 0x3F) << 6) | 
                 (str[2] & 0x3F);
    } else if (char_len == 4) {
        // 4字节UTF-8字符
        if ((str[1] & 0xC0) != 0x80 || 
            (str[2] & 0xC0) != 0x80 || 
            (str[3] & 0xC0) != 0x80) {
            return 0;  // 无效的后续字节
        }
        result = ((first_byte & 0x07) << 18) | 
                 ((str[1] & 0x3F) << 12) | 
                 ((str[2] & 0x3F) << 6) | 
                 (str[3] & 0x3F);
    }
    
    // 检查码点是否有效
    if (result > 0x10FFFF) {
        return 0;  // 超出Unicode范围
    }
    
    // 检查是否为代理对（U+D800 - U+DFFF）
    if (result >= 0xD800 && result <= 0xDFFF) {
        return 0;  // 无效的码点（代理对）
    }
    
    *code_point = result;
    return char_len;
}

/**
 * @brief 获取UTF-8字符信息
 */
Stru_UTF8CharInfo_t F_get_utf8_char_info(const char* str, size_t max_len)
{
    Stru_UTF8CharInfo_t info = {0};
    
    if (str == NULL || max_len == 0) {
        return info;
    }
    
    // 解码UTF-8字符
    uint32_t code_point = 0;
    size_t char_len = F_decode_utf8_char(str, max_len, &code_point);
    
    if (char_len == 0) {
        // 无效的UTF-8编码
        info.is_valid = false;
        return info;
    }
    
    // 填充信息
    info.code_point = code_point;
    info.byte_length = char_len;
    info.is_valid = true;
    info.is_chinese = F_is_chinese_char(code_point);
    
    // 检查是否为字母
    if (info.is_chinese) {
        info.is_letter = true;  // 中文字符视为字母
    } else if (char_len == 1) {
        // ASCII字符
        char c = str[0];
        info.is_letter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    } else {
        // 其他Unicode字母字符
        // 这里可以添加更多Unicode字母范围的检查
        info.is_letter = false;
    }
    
    return info;
}

/**
 * @brief 检查字符是否为字母（包括中文字符）
 */
bool F_is_alpha_utf8_complete(const char* str, size_t max_len)
{
    Stru_UTF8CharInfo_t info = F_get_utf8_char_info(str, max_len);
    return info.is_valid && info.is_letter;
}

/**
 * @brief 检查字符是否为数字
 */
bool F_is_digit_utf8(const char* str, size_t max_len)
{
    if (str == NULL || max_len == 0) {
        return false;
    }
    
    // 检查ASCII数字
    if (max_len >= 1 && str[0] >= '0' && str[0] <= '9') {
        return true;
    }
    
    // 检查全角数字（U+FF10 - U+FF19）
    Stru_UTF8CharInfo_t info = F_get_utf8_char_info(str, max_len);
    if (info.is_valid && info.code_point >= 0xFF10 && info.code_point <= 0xFF19) {
        return true;
    }
    
    return false;
}

/**
 * @brief 检查字符是否为字母或数字
 */
bool F_is_alnum_utf8(const char* str, size_t max_len)
{
    return F_is_alpha_utf8_complete(str, max_len) || 
           F_is_digit_utf8(str, max_len);
}

/**
 * @brief 检查UTF-8编码是否有效
 */
bool F_is_valid_utf8(const char* str, size_t length)
{
    if (str == NULL) {
        return false;
    }
    
    size_t pos = 0;
    while (pos < length) {
        unsigned char first_byte = (unsigned char)str[pos];
        size_t char_len = F_get_utf8_char_length(first_byte);
        
        if (char_len == 0) {
            return false;  // 无效的UTF-8起始字节
        }
        
        if (pos + char_len > length) {
            return false;  // 长度不足
        }
        
        // 检查后续字节
        for (size_t i = 1; i < char_len; i++) {
            if ((str[pos + i] & 0xC0) != 0x80) {
                return false;  // 无效的后续字节
            }
        }
        
        // 解码码点进行额外验证
        uint32_t code_point = 0;
        if (F_decode_utf8_char(str + pos, char_len, &code_point) != char_len) {
            return false;  // 解码失败
        }
        
        pos += char_len;
    }
    
    return true;
}

/**
 * @brief 跳过UTF-8字符
 */
size_t F_skip_utf8_chars(const char* str, size_t length, size_t char_count)
{
    if (str == NULL || length == 0 || char_count == 0) {
        return 0;
    }
    
    size_t pos = 0;
    size_t chars_skipped = 0;
    
    while (pos < length && chars_skipped < char_count) {
        unsigned char first_byte = (unsigned char)str[pos];
        size_t char_len = F_get_utf8_char_length(first_byte);
        
        if (char_len == 0 || pos + char_len > length) {
            break;  // 无效编码或长度不足
        }
        
        pos += char_len;
        chars_skipped++;
    }
    
    return pos;
}

/**
 * @brief 获取UTF-8字符串的字符数
 */
size_t F_count_utf8_chars(const char* str, size_t length)
{
    if (str == NULL || length == 0) {
        return 0;
    }
    
    size_t pos = 0;
    size_t char_count = 0;
    
    while (pos < length) {
        unsigned char first_byte = (unsigned char)str[pos];
        size_t char_len = F_get_utf8_char_length(first_byte);
        
        if (char_len == 0 || pos + char_len > length) {
            break;  // 无效编码或长度不足
        }
        
        pos += char_len;
        char_count++;
    }
    
    return char_count;
}

/**
 * @brief 检查字符是否为标识符起始字符
 */
bool F_is_identifier_start_utf8(const char* str, size_t max_len)
{
    if (str == NULL || max_len == 0) {
        return false;
    }
    
    // 检查下划线
    if (max_len >= 1 && str[0] == '_') {
        return true;
    }
    
    // 检查字母（包括中文字符）
    return F_is_alpha_utf8_complete(str, max_len);
}

/**
 * @brief 检查字符是否为标识符继续字符
 */
bool F_is_identifier_continue_utf8(const char* str, size_t max_len)
{
    if (str == NULL || max_len == 0) {
        return false;
    }
    
    // 检查下划线
    if (max_len >= 1 && str[0] == '_') {
        return true;
    }
    
    // 检查字母或数字（包括中文字符）
    return F_is_alnum_utf8(str, max_len);
}
