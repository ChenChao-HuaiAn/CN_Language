/**
 * @file CN_string_utils.h
 * @brief 字符串工具模块头文件
 * 
 * 提供字符串的工具函数接口声明。
 * 包括UTF-8处理、字符分类、格式化等辅助功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_STRING_UTILS_H
#define CN_STRING_UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "../string_core/CN_string_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== UTF-8处理 ==================== */

/**
 * @brief 计算UTF-8字符串长度
 * 
 * 计算UTF-8编码字符串的字符数（不是字节数）。
 * 
 * @param utf8_str UTF-8字符串
 * @return 字符数，如果utf8_str为NULL返回0
 */
size_t F_utf8_strlen(const char* utf8_str);

/**
 * @brief 获取UTF-8字符的字节长度
 * 
 * 获取UTF-8字符的字节长度。
 * 
 * @param first_byte UTF-8字符的第一个字节
 * @return 字符的字节长度（1-4）
 */
size_t F_utf8_char_len(unsigned char first_byte);

/**
 * @brief 验证UTF-8字符串
 * 
 * 验证字符串是否为有效的UTF-8编码。
 * 
 * @param str 字符串指针
 * @return 有效返回true，无效返回false
 */
bool F_string_validate_utf8(Stru_String_t* str);

/**
 * @brief 验证C字符串UTF-8编码
 * 
 * 验证C风格字符串是否为有效的UTF-8编码。
 * 
 * @param cstr C风格字符串
 * @param length 字符串长度（字节数），0表示自动计算
 * @return 有效返回true，无效返回false
 */
bool F_cstr_validate_utf8(const char* cstr, size_t length);

/**
 * @brief 获取UTF-8字符在字符串中的位置
 * 
 * 获取第n个UTF-8字符在字符串中的字节位置。
 * 
 * @param str 字符串指针
 * @param char_index 字符索引
 * @return 字节位置，如果索引超出范围返回-1
 */
int64_t F_string_utf8_char_position(Stru_String_t* str, size_t char_index);

/* ==================== 字符分类 ==================== */

/**
 * @brief 检查字符串是否只包含字母
 * 
 * 检查字符串是否只包含字母字符。
 * 
 * @param str 字符串指针
 * @return 只包含字母返回true，否则返回false
 */
bool F_string_is_alpha(Stru_String_t* str);

/**
 * @brief 检查字符串是否只包含数字
 * 
 * 检查字符串是否只包含数字字符。
 * 
 * @param str 字符串指针
 * @return 只包含数字返回true，否则返回false
 */
bool F_string_is_digit(Stru_String_t* str);

/**
 * @brief 检查字符串是否只包含字母和数字
 * 
 * 检查字符串是否只包含字母和数字字符。
 * 
 * @param str 字符串指针
 * @return 只包含字母和数字返回true，否则返回false
 */
bool F_string_is_alnum(Stru_String_t* str);

/**
 * @brief 检查字符串是否只包含ASCII字符
 * 
 * 检查字符串是否只包含ASCII字符（0-127）。
 * 
 * @param str 字符串指针
 * @return 只包含ASCII字符返回true，否则返回false
 */
bool F_string_is_ascii(Stru_String_t* str);

/**
 * @brief 检查字符串是否只包含空白字符
 * 
 * 检查字符串是否只包含空白字符。
 * 
 * @param str 字符串指针
 * @return 只包含空白字符返回true，否则返回false
 */
bool F_string_is_space(Stru_String_t* str);

/**
 * @brief 检查字符串是否只包含小写字母
 * 
 * 检查字符串是否只包含小写字母。
 * 
 * @param str 字符串指针
 * @return 只包含小写字母返回true，否则返回false
 */
bool F_string_is_lower(Stru_String_t* str);

/**
 * @brief 检查字符串是否只包含大写字母
 * 
 * 检查字符串是否只包含大写字母。
 * 
 * @param str 字符串指针
 * @return 只包含大写字母返回true，否则返回false
 */
bool F_string_is_upper(Stru_String_t* str);

/* ==================== 格式化 ==================== */

/**
 * @brief 格式化字符串
 * 
 * 使用类似printf的格式创建字符串。
 * 
 * @param format 格式字符串
 * @param ... 可变参数
 * @return 新的格式化字符串指针，失败返回NULL
 */
Stru_String_t* F_string_format(const char* format, ...);

/**
 * @brief 格式化字符串到现有字符串
 * 
 * 使用类似printf的格式设置字符串内容。
 * 
 * @param str 字符串指针
 * @param format 格式字符串
 * @param ... 可变参数
 * @return 格式化成功返回true，失败返回false
 */
bool F_string_format_to(Stru_String_t* str, const char* format, ...);

/**
 * @brief 转换为整数
 * 
 * 将字符串转换为整数。
 * 
 * @param str 字符串指针
 * @param value 输出参数，转换后的整数值
 * @param base 进制（2-36，0表示自动检测）
 * @return 转换成功返回true，失败返回false
 */
bool F_string_to_int(Stru_String_t* str, int* value, int base);

/**
 * @brief 转换为浮点数
 * 
 * 将字符串转换为浮点数。
 * 
 * @param str 字符串指针
 * @param value 输出参数，转换后的浮点数值
 * @return 转换成功返回true，失败返回false
 */
bool F_string_to_double(Stru_String_t* str, double* value);

/* ==================== 其他工具 ==================== */

/**
 * @brief 分割字符串
 * 
 * 使用分隔符分割字符串。
 * 
 * @param str 字符串指针
 * @param delimiter 分隔符
 * @param count 输出参数，分割后的子字符串数量
 * @return 子字符串数组指针，失败返回NULL
 * 
 * @note 调用者负责释放返回的数组和其中的字符串
 */
Stru_String_t** F_string_split(Stru_String_t* str, char delimiter, size_t* count);

/**
 * @brief 分割C字符串
 * 
 * 使用分隔符分割C风格字符串。
 * 
 * @param cstr C风格字符串
 * @param delimiter 分隔符
 * @param count 输出参数，分割后的子字符串数量
 * @return 子字符串数组指针，失败返回NULL
 */
Stru_String_t** F_cstr_split(const char* cstr, char delimiter, size_t* count);

/**
 * @brief 连接字符串数组
 * 
 * 使用分隔符连接字符串数组。
 * 
 * @param strings 字符串数组
 * @param count 字符串数量
 * @param delimiter 分隔符
 * @return 新的连接后的字符串指针，失败返回NULL
 */
Stru_String_t* F_string_join(Stru_String_t** strings, size_t count, const char* delimiter);

#ifdef __cplusplus
}
#endif

#endif // CN_STRING_UTILS_H
