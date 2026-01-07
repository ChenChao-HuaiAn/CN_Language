/**
 * @file CN_string_core.h
 * @brief 字符串核心模块头文件
 * 
 * 提供字符串数据结构的核心接口声明。
 * 包括字符串的创建、销毁、基本属性和容量管理功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 2.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_STRING_CORE_H
#define CN_STRING_CORE_H

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
 * 字符串结构体包含数据缓冲区、字符长度、字节长度和容量信息。
 */
typedef struct Stru_String_t
{
    char* data;                 /**< 字符串数据（UTF-8编码） */
    size_t length;              /**< 字符串长度（字符数） */
    size_t capacity;            /**< 当前分配的容量（字节数） */
    size_t byte_length;         /**< 字符串字节长度 */
} Stru_String_t;

/* ==================== 创建和销毁 ==================== */

/**
 * @brief 创建空字符串
 * 
 * 创建并初始化一个空字符串。
 * 
 * @return 指向新创建的字符串的指针，失败返回NULL
 */
Stru_String_t* F_string_create_empty(void);

/**
 * @brief 创建字符串
 * 
 * 分配并初始化一个新的字符串。
 * 
 * @param initial_capacity 初始容量（字节数）
 * @return 指向新创建的字符串的指针，失败返回NULL
 * 
 * @note 如果initial_capacity小于默认初始容量(32)，将使用默认值
 */
Stru_String_t* F_string_create_with_capacity(size_t initial_capacity);

/**
 * @brief 从C字符串创建字符串
 * 
 * 从C风格字符串创建新的字符串。
 * 
 * @param cstr C风格字符串（UTF-8编码）
 * @return 指向新创建的字符串的指针，失败返回NULL
 * 
 * @note 如果cstr为NULL，函数返回NULL
 */
Stru_String_t* F_string_create_from_cstr(const char* cstr);

/**
 * @brief 创建字符串副本
 * 
 * 创建字符串的副本。
 * 
 * @param str 要复制的字符串指针
 * @return 指向新创建的字符串副本的指针，失败返回NULL
 * 
 * @note 如果str为NULL，函数返回NULL
 */
Stru_String_t* F_string_create_copy(Stru_String_t* str);

/**
 * @brief 销毁字符串
 * 
 * 释放字符串占用的所有内存。
 * 
 * @param str 要销毁的字符串指针
 * 
 * @note 如果str为NULL，函数不执行任何操作
 */
void F_string_destroy(Stru_String_t* str);

/* ==================== 基本属性 ==================== */

/**
 * @brief 检查字符串是否为空
 * 
 * 检查字符串是否为空（长度为0）。
 * 
 * @param str 字符串指针
 * @return 如果字符串为空返回true，否则返回false
 * 
 * @note 如果str为NULL，函数返回true
 */
bool F_string_is_empty(Stru_String_t* str);

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
 * 
 * @note 返回的指针指向字符串内部缓冲区，不要修改或释放
 */
const char* F_string_cstr(Stru_String_t* str);

/**
 * @brief 获取字符串容量
 * 
 * 返回字符串当前分配的容量（字节数）。
 * 
 * @param str 字符串指针
 * @return 字符串容量，如果str为NULL返回0
 */
size_t F_string_capacity(Stru_String_t* str);

/* ==================== 容量管理 ==================== */

/**
 * @brief 确保字符串容量
 * 
 * 确保字符串有足够容量容纳指定字节数。
 * 如果当前容量不足，会自动扩容。
 * 
 * @param str 字符串指针
 * @param required_bytes 需要的字节数（不包括null终止符）
 * @return 容量足够或扩容成功返回true，失败返回false
 */
bool F_string_ensure_capacity(Stru_String_t* str, size_t required_bytes);

/**
 * @brief 调整字符串容量
 * 
 * 调整字符串的容量到指定值。
 * 如果new_capacity小于当前字节长度，调整失败。
 * 
 * @param str 字符串指针
 * @param new_capacity 新的容量（字节数）
 * @return 调整成功返回true，失败返回false
 * 
 * @note 如果new_capacity小于当前字节长度，函数返回false
 */
bool F_string_reserve(Stru_String_t* str, size_t new_capacity);

/**
 * @brief 收缩字符串容量
 * 
 * 将字符串容量收缩到刚好容纳当前内容。
 * 可以减少内存使用。
 * 
 * @param str 字符串指针
 * @return 收缩成功返回true，失败返回false
 */
bool F_string_shrink_to_fit(Stru_String_t* str);

/* ==================== 内部函数（供其他模块使用） ==================== */

/**
 * @brief 确保字符串有足够容量
 * 
 * 检查字符串是否有足够容量容纳新内容，如果没有则扩容。
 * 
 * @param str 字符串指针
 * @param additional_bytes 需要额外增加的字节数
 * @return 容量足够或扩容成功返回true，失败返回false
 */
bool ensure_capacity(Stru_String_t* str, size_t additional_bytes);

/**
 * @brief 计算UTF-8字符串长度
 * 
 * 计算UTF-8编码字符串的字符数（不是字节数）。
 * 
 * @param utf8_str UTF-8字符串
 * @return 字符数
 */
size_t utf8_strlen(const char* utf8_str);

/**
 * @brief 获取UTF-8字符的字节长度
 * 
 * 获取UTF-8字符的字节长度。
 * 
 * @param first_byte UTF-8字符的第一个字节
 * @return 字符的字节长度（1-4）
 */
size_t utf8_char_len(unsigned char first_byte);

#ifdef __cplusplus
}
#endif

#endif // CN_STRING_CORE_H
