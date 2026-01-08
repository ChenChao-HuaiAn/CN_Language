/**
 * @file CN_lexer_utils.h
 * @brief CN_Language 词法分析器工具函数模块
 * 
 * 提供词法分析器所需的通用工具函数，包括字符分类、字符串处理等。
 * 这些函数被多个子模块共享使用。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#ifndef CN_LEXER_UTILS_H
#define CN_LEXER_UTILS_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief 检查字符是否为空白字符
 * 
 * 检查字符是否为空白字符（空格、制表符、换行符、回车符）。
 * 
 * @param c 要检查的字符
 * @return true 是空白字符
 * @return false 不是空白字符
 */
bool F_is_whitespace(char c);

/**
 * @brief 检查字符是否为字母（包括中文字符）
 * 
 * 检查字符是否为字母，支持ASCII字母和中文字符。
 * 中文字符通过检查UTF-8编码的第一个字节来识别。
 * 
 * @param c 要检查的字符
 * @return true 是字母
 * @return false 不是字母
 */
bool F_is_alpha_utf8(char c);

/**
 * @brief 检查字符是否为数字
 * 
 * 检查字符是否为数字（0-9）。
 * 
 * @param c 要检查的字符
 * @return true 是数字
 * @return false 不是数字
 */
bool F_is_digit_simple(char c);

/**
 * @brief 检查字符是否为十六进制数字
 * 
 * 检查字符是否为十六进制数字（0-9, a-f, A-F）。
 * 
 * @param c 要检查的字符
 * @return true 是十六进制数字
 * @return false 不是十六进制数字
 */
bool F_is_hex_digit(char c);

/**
 * @brief 检查字符是否为字母或数字
 * 
 * 检查字符是否为字母或数字。
 * 
 * @param c 要检查的字符
 * @return true 是字母或数字
 * @return false 不是字母或数字
 */
bool F_is_alnum(char c);

/**
 * @brief 检查字符是否为运算符字符
 * 
 * 检查字符是否为运算符字符。
 * 
 * @param c 要检查的字符
 * @return true 是运算符字符
 * @return false 不是运算符字符
 */
bool F_is_operator_char_simple(char c);

/**
 * @brief 检查字符是否为分隔符字符
 * 
 * 检查字符是否为分隔符字符。
 * 
 * @param c 要检查的字符
 * @return true 是分隔符字符
 * @return false 不是分隔符字符
 */
bool F_is_delimiter_char_simple(char c);

/**
 * @brief 安全复制字符串
 * 
 * 安全地将源字符串复制到目标缓冲区，确保不会溢出。
 * 
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @param dest_size 目标缓冲区大小
 * @return size_t 复制的字符数（不包括终止符）
 */
size_t F_safe_strcpy(char* dest, const char* src, size_t dest_size);

/**
 * @brief 安全连接字符串
 * 
 * 安全地将源字符串连接到目标缓冲区，确保不会溢出。
 * 
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @param dest_size 目标缓冲区大小
 * @return size_t 连接后的字符串长度
 */
size_t F_safe_strcat(char* dest, const char* src, size_t dest_size);

/**
 * @brief 检查字符串是否以指定前缀开头
 * 
 * 检查字符串是否以指定的前缀开头。
 * 
 * @param str 要检查的字符串
 * @param prefix 前缀
 * @return true 以指定前缀开头
 * @return false 不以指定前缀开头
 */
bool F_starts_with(const char* str, const char* prefix);

/**
 * @brief 检查字符串是否以指定后缀结尾
 * 
 * 检查字符串是否以指定的后缀结尾。
 * 
 * @param str 要检查的字符串
 * @param suffix 后缀
 * @return true 以指定后缀结尾
 * @return false 不以指定后缀结尾
 */
bool F_ends_with(const char* str, const char* suffix);

/**
 * @brief 跳过字符串中的空白字符
 * 
 * 返回跳过空白字符后的字符串指针。
 * 
 * @param str 输入字符串
 * @return const char* 跳过空白字符后的字符串指针
 */
const char* F_skip_whitespace_str(const char* str);

/**
 * @brief 计算字符串长度（安全版本）
 * 
 * 安全地计算字符串长度，限制最大长度。
 * 
 * @param str 输入字符串
 * @param max_len 最大长度限制
 * @return size_t 字符串长度（不超过max_len）
 */
size_t F_safe_strlen(const char* str, size_t max_len);

#endif // CN_LEXER_UTILS_H
