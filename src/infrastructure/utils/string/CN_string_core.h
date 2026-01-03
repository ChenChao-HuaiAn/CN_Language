/******************************************************************************
 * 文件名: CN_string_core.h
 * 功能: CN_Language字符串核心模块 - 字符串结构和管理
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现字符串核心功能
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_STRING_CORE_H
#define CN_STRING_CORE_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief 字符串结构体（不透明类型）
 */
typedef struct Stru_CN_String_t Stru_CN_String_t;

/**
 * @brief 字符串编码类型
 */
typedef enum Eum_CN_StringEncoding_t
{
    Eum_STRING_ENCODING_ASCII = 0,     /**< ASCII编码 */
    Eum_STRING_ENCODING_UTF8 = 1,      /**< UTF-8编码 */
    Eum_STRING_ENCODING_UTF16 = 2,     /**< UTF-16编码 */
    Eum_STRING_ENCODING_UTF32 = 3,     /**< UTF-32编码 */
    Eum_STRING_ENCODING_GB2312 = 4,    /**< GB2312编码（简体中文） */
    Eum_STRING_ENCODING_GBK = 5,       /**< GBK编码 */
    Eum_STRING_ENCODING_GB18030 = 6    /**< GB18030编码 */
} Eum_CN_StringEncoding_t;

// ============================================================================
// 字符串创建和销毁
// ============================================================================

/**
 * @brief 创建字符串
 * 
 * @param data 字符串数据
 * @param length 数据长度（字节），0表示自动计算
 * @param encoding 字符串编码
 * @return 新创建的字符串，失败返回NULL
 */
Stru_CN_String_t* CN_string_create(const char* data, size_t length,
                                   Eum_CN_StringEncoding_t encoding);

/**
 * @brief 创建空字符串
 * 
 * @param initial_capacity 初始容量
 * @param encoding 字符串编码
 * @return 新创建的字符串，失败返回NULL
 */
Stru_CN_String_t* CN_string_create_empty(size_t initial_capacity,
                                         Eum_CN_StringEncoding_t encoding);

/**
 * @brief 复制字符串
 * 
 * @param src 源字符串
 * @return 新创建的字符串副本，失败返回NULL
 */
Stru_CN_String_t* CN_string_copy(const Stru_CN_String_t* src);

/**
 * @brief 销毁字符串
 * 
 * @param str 要销毁的字符串
 */
void CN_string_destroy(Stru_CN_String_t* str);

// ============================================================================
// 字符串属性查询
// ============================================================================

/**
 * @brief 获取字符串长度（字节数）
 * 
 * @param str 字符串
 * @return 字符串长度（字节数）
 */
size_t CN_string_length(const Stru_CN_String_t* str);

/**
 * @brief 获取字符串字符数（考虑多字节字符）
 * 
 * @param str 字符串
 * @return 字符数
 */
size_t CN_string_char_count(const Stru_CN_String_t* str);

/**
 * @brief 获取字符串容量
 * 
 * @param str 字符串
 * @return 字符串容量
 */
size_t CN_string_capacity(const Stru_CN_String_t* str);

/**
 * @brief 获取字符串编码
 * 
 * @param str 字符串
 * @return 字符串编码
 */
Eum_CN_StringEncoding_t CN_string_encoding(const Stru_CN_String_t* str);

/**
 * @brief 检查字符串是否为空
 * 
 * @param str 字符串
 * @return 如果字符串为空返回true，否则返回false
 */
bool CN_string_is_empty(const Stru_CN_String_t* str);

/**
 * @brief 获取C风格字符串指针
 * 
 * @param str 字符串
 * @return C风格字符串指针（只读）
 */
const char* CN_string_cstr(const Stru_CN_String_t* str);

// ============================================================================
// 字符串基本操作
// ============================================================================

/**
 * @brief 清空字符串
 * 
 * @param str 字符串
 */
void CN_string_clear(Stru_CN_String_t* str);

/**
 * @brief 确保字符串有足够容量
 * 
 * @param str 字符串
 * @param min_capacity 最小容量要求
 * @return 调整成功返回true，失败返回false
 */
bool CN_string_ensure_capacity(Stru_CN_String_t* str, size_t min_capacity);

/**
 * @brief 缩小字符串容量以匹配长度
 * 
 * @param str 字符串
 * @return 调整成功返回true，失败返回false
 */
bool CN_string_shrink_to_fit(Stru_CN_String_t* str);

/**
 * @brief 追加字符串
 * 
 * @param str 目标字符串
 * @param append 要追加的字符串
 * @return 追加成功返回true，失败返回false
 */
bool CN_string_append(Stru_CN_String_t* str, const Stru_CN_String_t* append);

/**
 * @brief 追加C风格字符串
 * 
 * @param str 目标字符串
 * @param cstr 要追加的C风格字符串
 * @param length 字符串长度，0表示自动计算
 * @return 追加成功返回true，失败返回false
 */
bool CN_string_append_cstr(Stru_CN_String_t* str, const char* cstr, size_t length);

/**
 * @brief 追加字符
 * 
 * @param str 目标字符串
 * @param ch 要追加的字符
 * @return 追加成功返回true，失败返回false
 */
bool CN_string_append_char(Stru_CN_String_t* str, char ch);

/**
 * @brief 插入字符串
 * 
 * @param str 目标字符串
 * @param pos 插入位置（字节偏移）
 * @param insert 要插入的字符串
 * @return 插入成功返回true，失败返回false
 */
bool CN_string_insert(Stru_CN_String_t* str, size_t pos,
                      const Stru_CN_String_t* insert);

/**
 * @brief 删除子字符串
 * 
 * @param str 目标字符串
 * @param pos 起始位置（字节偏移）
 * @param count 要删除的字节数
 * @return 删除成功返回true，失败返回false
 */
bool CN_string_remove(Stru_CN_String_t* str, size_t pos, size_t count);

/**
 * @brief 替换子字符串
 * 
 * @param str 目标字符串
 * @param pos 起始位置
 * @param count 要替换的字节数
 * @param replacement 替换字符串
 * @return 替换成功返回true，失败返回false
 */
bool CN_string_replace(Stru_CN_String_t* str, size_t pos, size_t count,
                       const Stru_CN_String_t* replacement);

/**
 * @brief 提取子字符串
 * 
 * @param str 源字符串
 * @param pos 起始位置
 * @param count 要提取的字节数
 * @return 新创建的子字符串，失败返回NULL
 */
Stru_CN_String_t* CN_string_substr(const Stru_CN_String_t* str,
                                   size_t pos, size_t count);

// ============================================================================
// 字符串工具函数
// ============================================================================

/**
 * @brief 复制C风格字符串
 * 
 * @param cstr C风格字符串
 * @param length 字符串长度，0表示自动计算
 * @return 新分配的字符串副本，调用者负责释放
 */
char* CN_string_duplicate_cstr(const char* cstr, size_t length);

#endif // CN_STRING_CORE_H
