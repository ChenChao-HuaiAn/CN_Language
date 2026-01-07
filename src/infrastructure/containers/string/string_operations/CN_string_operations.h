/**
 * @file CN_string_operations.h
 * @brief 字符串操作模块头文件
 * 
 * 提供字符串的基本操作接口声明。
 * 包括字符串的追加、比较、复制、清空等操作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_STRING_OPERATIONS_H
#define CN_STRING_OPERATIONS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "../string_core/CN_string_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 字符串操作 ==================== */

/**
 * @brief 追加字符串
 * 
 * 在字符串末尾追加另一个字符串。
 * 
 * @param dest 目标字符串指针
 * @param src 源字符串指针
 * @return 追加成功返回true，失败返回false
 * 
 * @note 如果dest或src为NULL，函数返回false
 */
bool F_string_append(Stru_String_t* dest, Stru_String_t* src);

/**
 * @brief 追加C字符串
 * 
 * 在字符串末尾追加C风格字符串。
 * 
 * @param dest 目标字符串指针
 * @param cstr C风格字符串
 * @return 追加成功返回true，失败返回false
 * 
 * @note 如果dest或cstr为NULL，函数返回false
 */
bool F_string_append_cstr(Stru_String_t* dest, const char* cstr);

/**
 * @brief 追加字符
 * 
 * 在字符串末尾追加单个字符。
 * 
 * @param str 字符串指针
 * @param ch 要追加的字符
 * @return 追加成功返回true，失败返回false
 * 
 * @note 支持UTF-8多字节字符
 */
bool F_string_append_char(Stru_String_t* str, char ch);

/**
 * @brief 清空字符串
 * 
 * 清空字符串内容，但不释放字符串本身。
 * 
 * @param str 字符串指针
 * 
 * @note 清空后字符串长度和字节长度都变为0
 */
void F_string_clear(Stru_String_t* str);

/**
 * @brief 比较字符串
 * 
 * 比较两个字符串是否相等。
 * 
 * @param str1 第一个字符串指针
 * @param str2 第二个字符串指针
 * @return 相等返回0，str1小于str2返回负数，否则返回正数
 * 
 * @note 使用标准C库的strcmp进行比较
 */
int F_string_compare(Stru_String_t* str1, Stru_String_t* str2);

/**
 * @brief 比较字符串（不区分大小写）
 * 
 * 比较两个字符串是否相等，不区分大小写。
 * 
 * @param str1 第一个字符串指针
 * @param str2 第二个字符串指针
 * @return 相等返回0，str1小于str2返回负数，否则返回正数
 */
int F_string_compare_case_insensitive(Stru_String_t* str1, Stru_String_t* str2);

/**
 * @brief 复制字符串
 * 
 * 复制源字符串到目标字符串。
 * 
 * @param dest 目标字符串指针
 * @param src 源字符串指针
 * @return 复制成功返回true，失败返回false
 * 
 * @note 目标字符串的原有内容会被覆盖
 */
bool F_string_copy(Stru_String_t* dest, Stru_String_t* src);

/**
 * @brief 复制C字符串
 * 
 * 复制C风格字符串到目标字符串。
 * 
 * @param dest 目标字符串指针
 * @param cstr C风格字符串
 * @return 复制成功返回true，失败返回false
 */
bool F_string_copy_cstr(Stru_String_t* dest, const char* cstr);

/**
 * @brief 连接字符串
 * 
 * 连接两个字符串，返回新的字符串。
 * 
 * @param str1 第一个字符串指针
 * @param str2 第二个字符串指针
 * @return 新的连接后的字符串指针，失败返回NULL
 */
Stru_String_t* F_string_concat(Stru_String_t* str1, Stru_String_t* str2);

/**
 * @brief 连接C字符串
 * 
 * 连接字符串和C风格字符串，返回新的字符串。
 * 
 * @param str 字符串指针
 * @param cstr C风格字符串
 * @return 新的连接后的字符串指针，失败返回NULL
 */
Stru_String_t* F_string_concat_cstr(Stru_String_t* str, const char* cstr);

#ifdef __cplusplus
}
#endif

#endif // CN_STRING_OPERATIONS_H
