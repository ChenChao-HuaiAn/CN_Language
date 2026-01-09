/**
 * @file CN_lexer_utf8.h
 * @brief CN_Language 词法分析器UTF-8编码支持模块
 * 
 * 提供完整的UTF-8编码支持，包括字符分类、编码验证和字符处理。
 * 支持中文字符的完整Unicode范围。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 */

#ifndef CN_LEXER_UTF8_H
#define CN_LEXER_UTF8_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief UTF-8字符信息结构体
 * 
 * 存储UTF-8字符的编码信息和Unicode码点。
 */
typedef struct Stru_UTF8CharInfo_t {
    uint32_t code_point;           ///< Unicode码点
    size_t byte_length;            ///< UTF-8编码字节长度（1-4字节）
    bool is_valid;                 ///< 是否为有效的UTF-8编码
    bool is_chinese;               ///< 是否为中文字符
    bool is_letter;                ///< 是否为字母（包括中文字符）
} Stru_UTF8CharInfo_t;

/**
 * @brief 获取UTF-8字符信息
 * 
 * 分析UTF-8编码的字符，返回字符的详细信息。
 * 
 * @param str UTF-8字符串
 * @param max_len 最大可读取长度
 * @return Stru_UTF8CharInfo_t UTF-8字符信息
 */
Stru_UTF8CharInfo_t F_get_utf8_char_info(const char* str, size_t max_len);

/**
 * @brief 检查字符是否为中文字符
 * 
 * 检查Unicode码点是否在中文字符范围内。
 * 
 * @param code_point Unicode码点
 * @return true 是中文字符
 * @return false 不是中文字符
 */
bool F_is_chinese_char(uint32_t code_point);

/**
 * @brief 检查字符是否为字母（包括中文字符）
 * 
 * 检查字符是否为字母，支持ASCII字母和中文字符。
 * 
 * @param str UTF-8字符串
 * @param max_len 最大可读取长度
 * @return true 是字母
 * @return false 不是字母
 */
bool F_is_alpha_utf8_complete(const char* str, size_t max_len);

/**
 * @brief 检查字符是否为数字
 * 
 * 检查字符是否为数字，支持全角数字。
 * 
 * @param str UTF-8字符串
 * @param max_len 最大可读取长度
 * @return true 是数字
 * @return false 不是数字
 */
bool F_is_digit_utf8(const char* str, size_t max_len);

/**
 * @brief 检查字符是否为字母或数字
 * 
 * 检查字符是否为字母或数字，支持Unicode字符。
 * 
 * @param str UTF-8字符串
 * @param max_len 最大可读取长度
 * @return true 是字母或数字
 * @return false 不是字母或数字
 */
bool F_is_alnum_utf8(const char* str, size_t max_len);

/**
 * @brief 检查UTF-8编码是否有效
 * 
 * 检查UTF-8编码序列是否有效。
 * 
 * @param str UTF-8字符串
 * @param length 字符串长度
 * @return true 编码有效
 * @return false 编码无效
 */
bool F_is_valid_utf8(const char* str, size_t length);

/**
 * @brief 获取UTF-8字符的字节长度
 * 
 * 根据UTF-8编码的第一个字节获取字符的字节长度。
 * 
 * @param first_byte UTF-8字符的第一个字节
 * @return size_t 字节长度（1-4），0表示无效编码
 */
size_t F_get_utf8_char_length(unsigned char first_byte);

/**
 * @brief 解码UTF-8字符
 * 
 * 解码UTF-8编码序列，返回Unicode码点。
 * 
 * @param str UTF-8字符串
 * @param length 可用的字节数
 * @param code_point 输出参数：Unicode码点
 * @return size_t 实际使用的字节数，0表示解码失败
 */
size_t F_decode_utf8_char(const char* str, size_t length, uint32_t* code_point);

/**
 * @brief 跳过UTF-8字符
 * 
 * 跳过指定数量的UTF-8字符。
 * 
 * @param str UTF-8字符串
 * @param length 字符串长度
 * @param char_count 要跳过的字符数
 * @return size_t 跳过的字节数
 */
size_t F_skip_utf8_chars(const char* str, size_t length, size_t char_count);

/**
 * @brief 获取UTF-8字符串的字符数
 * 
 * 计算UTF-8字符串中的字符数（不是字节数）。
 * 
 * @param str UTF-8字符串
 * @param length 字符串长度（字节数）
 * @return size_t 字符数
 */
size_t F_count_utf8_chars(const char* str, size_t length);

/**
 * @brief 检查字符是否为标识符起始字符
 * 
 * 检查字符是否可以作为标识符的起始字符。
 * 包括中文字符、ASCII字母和下划线。
 * 
 * @param str UTF-8字符串
 * @param max_len 最大可读取长度
 * @return true 可以作为标识符起始字符
 * @return false 不能作为标识符起始字符
 */
bool F_is_identifier_start_utf8(const char* str, size_t max_len);

/**
 * @brief 检查字符是否为标识符继续字符
 * 
 * 检查字符是否可以作为标识符的继续字符。
 * 包括中文字符、ASCII字母、数字和下划线。
 * 
 * @param str UTF-8字符串
 * @param max_len 最大可读取长度
 * @return true 可以作为标识符继续字符
 * @return false 不能作为标识符继续字符
 */
bool F_is_identifier_continue_utf8(const char* str, size_t max_len);

#endif // CN_LEXER_UTF8_H
