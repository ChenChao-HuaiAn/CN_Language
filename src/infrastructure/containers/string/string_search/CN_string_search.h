/**
 * @file CN_string_search.h
 * @brief 字符串搜索模块头文件
 * 
 * 提供字符串的搜索和替换功能接口声明。
 * 包括子字符串提取、查找、替换等操作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_STRING_SEARCH_H
#define CN_STRING_SEARCH_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "../string_core/CN_string_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 子字符串操作 ==================== */

/**
 * @brief 获取子字符串
 * 
 * 获取字符串的子字符串。
 * 
 * @param str 源字符串指针
 * @param start 起始位置（字符索引）
 * @param length 子字符串长度
 * @return 新的子字符串指针，失败返回NULL
 * 
 * @note 如果start超出字符串范围或length为0，返回NULL
 */
Stru_String_t* F_string_substring(Stru_String_t* str, size_t start, size_t length);

/**
 * @brief 获取子字符串（字节位置）
 * 
 * 获取字符串的子字符串，使用字节位置而不是字符位置。
 * 
 * @param str 源字符串指针
 * @param byte_start 起始字节位置
 * @param byte_length 子字符串字节长度
 * @return 新的子字符串指针，失败返回NULL
 * 
 * @note 此函数不验证UTF-8边界，调用者需确保参数正确
 */
Stru_String_t* F_string_substring_bytes(Stru_String_t* str, size_t byte_start, size_t byte_length);

/**
 * @brief 获取左子字符串
 * 
 * 获取字符串从左开始的子字符串。
 * 
 * @param str 源字符串指针
 * @param length 子字符串长度
 * @return 新的子字符串指针，失败返回NULL
 */
Stru_String_t* F_string_left(Stru_String_t* str, size_t length);

/**
 * @brief 获取右子字符串
 * 
 * 获取字符串从右开始的子字符串。
 * 
 * @param str 源字符串指针
 * @param length 子字符串长度
 * @return 新的子字符串指针，失败返回NULL
 */
Stru_String_t* F_string_right(Stru_String_t* str, size_t length);

/* ==================== 查找操作 ==================== */

/**
 * @brief 查找子字符串
 * 
 * 在字符串中查找子字符串。
 * 
 * @param str 源字符串指针
 * @param substr 要查找的子字符串
 * @param start_pos 起始查找位置（字符索引）
 * @return 子字符串位置（字符索引），未找到返回-1
 */
int64_t F_string_find(Stru_String_t* str, Stru_String_t* substr, size_t start_pos);

/**
 * @brief 查找C子字符串
 * 
 * 在字符串中查找C风格子字符串。
 * 
 * @param str 源字符串指针
 * @param csubstr 要查找的C风格子字符串
 * @param start_pos 起始查找位置（字符索引）
 * @return 子字符串位置（字符索引），未找到返回-1
 */
int64_t F_string_find_cstr(Stru_String_t* str, const char* csubstr, size_t start_pos);

/**
 * @brief 从后向前查找子字符串
 * 
 * 从字符串末尾开始向前查找子字符串。
 * 
 * @param str 源字符串指针
 * @param substr 要查找的子字符串
 * @param start_pos 起始查找位置（字符索引，从后向前）
 * @return 子字符串位置（字符索引），未找到返回-1
 */
int64_t F_string_rfind(Stru_String_t* str, Stru_String_t* substr, size_t start_pos);

/**
 * @brief 查找字符
 * 
 * 在字符串中查找字符。
 * 
 * @param str 源字符串指针
 * @param ch 要查找的字符
 * @param start_pos 起始查找位置（字符索引）
 * @return 字符位置（字符索引），未找到返回-1
 */
int64_t F_string_find_char(Stru_String_t* str, char ch, size_t start_pos);

/**
 * @brief 从后向前查找字符
 * 
 * 从字符串末尾开始向前查找字符。
 * 
 * @param str 源字符串指针
 * @param ch 要查找的字符
 * @param start_pos 起始查找位置（字符索引，从后向前）
 * @return 字符位置（字符索引），未找到返回-1
 */
int64_t F_string_rfind_char(Stru_String_t* str, char ch, size_t start_pos);

/* ==================== 替换操作 ==================== */

/**
 * @brief 替换子字符串
 * 
 * 替换字符串中的子字符串。
 * 
 * @param str 字符串指针
 * @param old_substr 要替换的子字符串
 * @param new_substr 新的子字符串
 * @return 替换成功返回true，失败返回false
 */
bool F_string_replace(Stru_String_t* str, Stru_String_t* old_substr, Stru_String_t* new_substr);

/**
 * @brief 替换C子字符串
 * 
 * 替换字符串中的C风格子字符串。
 * 
 * @param str 字符串指针
 * @param old_csubstr 要替换的C风格子字符串
 * @param new_csubstr 新的C风格子字符串
 * @return 替换成功返回true，失败返回false
 */
bool F_string_replace_cstr(Stru_String_t* str, const char* old_csubstr, const char* new_csubstr);

/**
 * @brief 替换所有子字符串
 * 
 * 替换字符串中所有出现的子字符串。
 * 
 * @param str 字符串指针
 * @param old_substr 要替换的子字符串
 * @param new_substr 新的子字符串
 * @return 替换的次数，失败返回-1
 */
int64_t F_string_replace_all(Stru_String_t* str, Stru_String_t* old_substr, Stru_String_t* new_substr);

/**
 * @brief 替换所有C子字符串
 * 
 * 替换字符串中所有出现的C风格子字符串。
 * 
 * @param str 字符串指针
 * @param old_csubstr 要替换的C风格子字符串
 * @param new_csubstr 新的C风格子字符串
 * @return 替换的次数，失败返回-1
 */
int64_t F_string_replace_all_cstr(Stru_String_t* str, const char* old_csubstr, const char* new_csubstr);

#ifdef __cplusplus
}
#endif

#endif // CN_STRING_SEARCH_H
