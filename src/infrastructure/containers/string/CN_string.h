/**
 * @file CN_string.h
 * @brief 字符串模块头文件
 * 
 * 提供字符串数据结构的定义和接口声明。
 * 字符串模块支持UTF-8编码的字符串操作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_STRING_H
#define CN_STRING_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 字符串结构体
 * 
 * 动态字符串数据结构，支持UTF-8编码。
 */
typedef struct Stru_String_t
{
    char* data;                 /**< 字符串数据（UTF-8编码） */
    size_t length;              /**< 字符串长度（字符数） */
    size_t capacity;            /**< 当前分配的容量（字节数） */
    size_t byte_length;         /**< 字符串字节长度 */
} Stru_String_t;

/**
 * @brief 创建字符串
 * 
 * 分配并初始化一个新的字符串。
 * 
 * @param initial_capacity 初始容量（字节数）
 * @return 指向新创建的字符串的指针，失败返回NULL
 */
Stru_String_t* F_create_string(size_t initial_capacity);

/**
 * @brief 从C字符串创建字符串
 * 
 * 从C风格字符串创建新的字符串。
 * 
 * @param cstr C风格字符串（UTF-8编码）
 * @return 指向新创建的字符串的指针，失败返回NULL
 */
Stru_String_t* F_create_string_from_cstr(const char* cstr);

/**
 * @brief 销毁字符串
 * 
 * 释放字符串占用的所有内存。
 * 
 * @param str 要销毁的字符串指针
 * 
 * @note 如果str为NULL，函数不执行任何操作
 */
void F_destroy_string(Stru_String_t* str);

/**
 * @brief 获取字符串长度
 * 
 * 返回字符串中的字符数（不是字节数）。
 * 
 * @param str 字符串指针
 * @return 字符串长度，如果str为NULL返回0
 */
size_t F_string_length(Stru_String_t* str);

/**
 * @brief 获取字符串字节长度
 * 
 * 返回字符串的字节长度。
 * 
 * @param str 字符串指针
 * @return 字符串字节长度，如果str为NULL返回0
 */
size_t F_string_byte_length(Stru_String_t* str);

/**
 * @brief 获取C风格字符串
 * 
 * 返回字符串的C风格表示（以null结尾）。
 * 
 * @param str 字符串指针
 * @return C风格字符串，如果str为NULL返回NULL
 */
const char* F_string_cstr(Stru_String_t* str);

/**
 * @brief 追加字符串
 * 
 * 在字符串末尾追加另一个字符串。
 * 
 * @param dest 目标字符串指针
 * @param src 源字符串指针
 * @return 追加成功返回true，失败返回false
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
 */
bool F_string_append_cstr(Stru_String_t* dest, const char* cstr);

/**
 * @brief 清空字符串
 * 
 * 清空字符串内容，但不释放字符串本身。
 * 
 * @param str 字符串指针
 * 
 * @note 清空后字符串长度变为0
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
 */
int F_string_compare(Stru_String_t* str1, Stru_String_t* str2);

/**
 * @brief 复制字符串
 * 
 * 复制源字符串到目标字符串。
 * 
 * @param dest 目标字符串指针
 * @param src 源字符串指针
 * @return 复制成功返回true，失败返回false
 */
bool F_string_copy(Stru_String_t* dest, Stru_String_t* src);

/**
 * @brief 获取子字符串
 * 
 * 获取字符串的子字符串。
 * 
 * @param str 源字符串指针
 * @param start 起始位置（字符索引）
 * @param length 子字符串长度
 * @return 新的子字符串指针，失败返回NULL
 */
Stru_String_t* F_string_substring(Stru_String_t* str, size_t start, size_t length);

/**
 * @brief 查找子字符串
 * 
 * 在字符串中查找子字符串。
 * 
 * @param str 源字符串指针
 * @param substr 要查找的子字符串
 * @param start_pos 起始查找位置
 * @return 子字符串位置（字符索引），未找到返回-1
 */
int64_t F_string_find(Stru_String_t* str, Stru_String_t* substr, size_t start_pos);

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
 * @brief 转换为大写
 * 
 * 将字符串转换为大写。
 * 
 * @param str 字符串指针
 * @return 转换成功返回true，失败返回false
 */
bool F_string_to_upper(Stru_String_t* str);

/**
 * @brief 转换为小写
 * 
 * 将字符串转换为小写。
 * 
 * @param str 字符串指针
 * @return 转换成功返回true，失败返回false
 */
bool F_string_to_lower(Stru_String_t* str);

/**
 * @brief 去除空白字符
 * 
 * 去除字符串两端的空白字符。
 * 
 * @param str 字符串指针
 * @return 去除成功返回true，失败返回false
 */
bool F_string_trim(Stru_String_t* str);

#ifdef __cplusplus
}
#endif

#endif // CN_STRING_H
