/**
 * @file CN_string_transform.h
 * @brief 字符串转换模块头文件
 * 
 * 提供字符串的转换功能接口声明。
 * 包括大小写转换、空白处理、格式化等操作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_STRING_TRANSFORM_H
#define CN_STRING_TRANSFORM_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "../string_core/CN_string_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 大小写转换 ==================== */

/**
 * @brief 转换为大写
 * 
 * 将字符串转换为大写。
 * 
 * @param str 字符串指针
 * @return 转换成功返回true，失败返回false
 * 
 * @note 当前实现仅支持ASCII字符
 */
bool F_string_to_upper(Stru_String_t* str);

/**
 * @brief 转换为小写
 * 
 * 将字符串转换为小写。
 * 
 * @param str 字符串指针
 * @return 转换成功返回true，失败返回false
 * 
 * @note 当前实现仅支持ASCII字符
 */
bool F_string_to_lower(Stru_String_t* str);

/**
 * @brief 反转大小写
 * 
 * 反转字符串中字符的大小写。
 * 
 * @param str 字符串指针
 * @return 转换成功返回true，失败返回false
 */
bool F_string_swap_case(Stru_String_t* str);

/**
 * @brief 首字母大写
 * 
 * 将字符串中每个单词的首字母转换为大写。
 * 
 * @param str 字符串指针
 * @return 转换成功返回true，失败返回false
 */
bool F_string_capitalize(Stru_String_t* str);

/**
 * @brief 标题化
 * 
 * 将字符串转换为标题格式（每个单词首字母大写）。
 * 
 * @param str 字符串指针
 * @return 转换成功返回true，失败返回false
 */
bool F_string_title(Stru_String_t* str);

/* ==================== 空白处理 ==================== */

/**
 * @brief 去除空白字符
 * 
 * 去除字符串两端的空白字符。
 * 
 * @param str 字符串指针
 * @return 去除成功返回true，失败返回false
 */
bool F_string_trim(Stru_String_t* str);

/**
 * @brief 去除左侧空白字符
 * 
 * 去除字符串左侧的空白字符。
 * 
 * @param str 字符串指针
 * @return 去除成功返回true，失败返回false
 */
bool F_string_ltrim(Stru_String_t* str);

/**
 * @brief 去除右侧空白字符
 * 
 * 去除字符串右侧的空白字符。
 * 
 * @param str 字符串指针
 * @return 去除成功返回true，失败返回false
 */
bool F_string_rtrim(Stru_String_t* str);

/**
 * @brief 去除所有空白字符
 * 
 * 去除字符串中所有的空白字符。
 * 
 * @param str 字符串指针
 * @return 去除成功返回true，失败返回false
 */
bool F_string_remove_whitespace(Stru_String_t* str);

/* ==================== 格式化操作 ==================== */

/**
 * @brief 反转字符串
 * 
 * 反转字符串中的字符顺序。
 * 
 * @param str 字符串指针
 * @return 反转成功返回true，失败返回false
 */
bool F_string_reverse(Stru_String_t* str);

/**
 * @brief 填充字符串
 * 
 * 在字符串左侧填充字符，使其达到指定长度。
 * 
 * @param str 字符串指针
 * @param length 目标长度
 * @param fill_char 填充字符
 * @return 填充成功返回true，失败返回false
 */
bool F_string_pad_left(Stru_String_t* str, size_t length, char fill_char);

/**
 * @brief 填充字符串
 * 
 * 在字符串右侧填充字符，使其达到指定长度。
 * 
 * @param str 字符串指针
 * @param length 目标长度
 * @param fill_char 填充字符
 * @return 填充成功返回true，失败返回false
 */
bool F_string_pad_right(Stru_String_t* str, size_t length, char fill_char);

/**
 * @brief 填充字符串
 * 
 * 在字符串两侧填充字符，使其达到指定长度。
 * 
 * @param str 字符串指针
 * @param length 目标长度
 * @param fill_char 填充字符
 * @return 填充成功返回true，失败返回false
 */
bool F_string_pad_both(Stru_String_t* str, size_t length, char fill_char);

/* ==================== 编码转换 ==================== */

/**
 * @brief 转换为ASCII
 * 
 * 将字符串转换为ASCII编码（非ASCII字符替换为指定字符）。
 * 
 * @param str 字符串指针
 * @param replacement 替换字符
 * @return 转换成功返回true，失败返回false
 */
bool F_string_to_ascii(Stru_String_t* str, char replacement);

#ifdef __cplusplus
}
#endif

#endif // CN_STRING_TRANSFORM_H
