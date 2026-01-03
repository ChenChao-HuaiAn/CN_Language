/******************************************************************************
 * 文件名: CN_string_core.c
 * 功能: CN_Language字符串核心模块实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现字符串核心功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_string_core.h"
#include "CN_string_internal.h"
#include "simple_memory.h"  // 使用简单的内存包装器进行测试
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>
#include <locale.h>
#include <limits.h>
#include <stdint.h>

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 计算UTF-8字符串的字符数
 * 
 * @param str UTF-8字符串
 * @param len 字符串长度（字节）
 * @return 字符数
 */
static size_t utf8_char_count(const char* str, size_t len)
{
    size_t count = 0;
    const char* end = str + len;
    
    while (str < end)
    {
        // UTF-8字符的第一个字节决定了字符长度
        unsigned char c = (unsigned char)*str;
        if (c < 0x80)
        {
            str += 1;  // ASCII字符
        }
        else if ((c & 0xE0) == 0xC0)
        {
            str += 2;  // 2字节UTF-8
        }
        else if ((c & 0xF0) == 0xE0)
        {
            str += 3;  // 3字节UTF-8
        }
        else if ((c & 0xF8) == 0xF0)
        {
            str += 4;  // 4字节UTF-8
        }
        else
        {
            // 无效的UTF-8序列，跳过1字节
            str += 1;
        }
        count++;
    }
    
    return count;
}

/**
 * @brief 计算字符串长度（如果length为0）
 * 
 * @param data 字符串数据
 * @param length 指定长度，0表示自动计算
 * @return 字符串长度
 */
static size_t calculate_length(const char* data, size_t length)
{
    if (length == 0 && data != NULL)
    {
        return strlen(data);
    }
    return length;
}

/**
 * @brief 调整字符串容量
 * 
 * @param str 字符串
 * @param new_capacity 新容量
 * @return 调整成功返回true，失败返回false
 */
static bool resize_string(Stru_CN_String_t* str, size_t new_capacity)
{
    if (new_capacity <= str->capacity)
    {
        return true;
    }
    
    // 确保至少有一个字节用于null终止符
    if (new_capacity == 0)
    {
        new_capacity = 1;
    }
    
    char* new_data = (char*)cn_realloc(str->data, new_capacity);
    if (new_data == NULL)
    {
        return false;
    }
    
    str->data = new_data;
    str->capacity = new_capacity;
    return true;
}

// ============================================================================
// 字符串创建和销毁
// ============================================================================

Stru_CN_String_t* CN_string_create(const char* data, size_t length,
                                   Eum_CN_StringEncoding_t encoding)
{
    // 计算实际长度
    size_t actual_length = calculate_length(data, length);
    
    // 分配字符串结构体
    Stru_CN_String_t* str = (Stru_CN_String_t*)cn_malloc(sizeof(Stru_CN_String_t));
    if (str == NULL)
    {
        return NULL;
    }
    
    // 初始化结构体
    str->length = actual_length;
    str->capacity = actual_length + 1;  // 包括null终止符
    str->encoding = encoding;
    str->ref_count = 1;
    
    // 分配数据缓冲区
    str->data = (char*)cn_malloc(str->capacity);
    if (str->data == NULL)
    {
        cn_free(str);
        return NULL;
    }
    
    // 复制数据
    if (data != NULL && actual_length > 0)
    {
        memcpy(str->data, data, actual_length);
    }
    str->data[actual_length] = '\0';
    
    return str;
}

Stru_CN_String_t* CN_string_create_empty(size_t initial_capacity,
                                         Eum_CN_StringEncoding_t encoding)
{
    // 确保最小容量
    if (initial_capacity == 0)
    {
        initial_capacity = 16;  // 默认初始容量
    }
    
    // 分配字符串结构体
    Stru_CN_String_t* str = (Stru_CN_String_t*)cn_malloc(sizeof(Stru_CN_String_t));
    if (str == NULL)
    {
        return NULL;
    }
    
    // 初始化结构体
    str->length = 0;
    str->capacity = initial_capacity;
    str->encoding = encoding;
    str->ref_count = 1;
    
    // 分配数据缓冲区
    str->data = (char*)cn_malloc(str->capacity);
    if (str->data == NULL)
    {
        cn_free(str);
        return NULL;
    }
    
    str->data[0] = '\0';
    
    return str;
}

Stru_CN_String_t* CN_string_copy(const Stru_CN_String_t* src)
{
    if (src == NULL)
    {
        return NULL;
    }
    
    // 创建新字符串
    Stru_CN_String_t* copy = CN_string_create(src->data, src->length, src->encoding);
    if (copy == NULL)
    {
        return NULL;
    }
    
    return copy;
}

void CN_string_destroy(Stru_CN_String_t* str)
{
    if (str == NULL)
    {
        return;
    }
    
    // 减少引用计数
    str->ref_count--;
    
    // 如果引用计数为0，释放内存
    if (str->ref_count <= 0)
    {
        if (str->data != NULL)
        {
            cn_free(str->data);
        }
        cn_free(str);
    }
}

// ============================================================================
// 字符串属性查询
// ============================================================================

size_t CN_string_length(const Stru_CN_String_t* str)
{
    if (str == NULL)
    {
        return 0;
    }
    return str->length;
}

size_t CN_string_char_count(const Stru_CN_String_t* str)
{
    if (str == NULL || str->data == NULL)
    {
        return 0;
    }
    
    // 目前只支持UTF-8编码的字符计数
    if (str->encoding == Eum_STRING_ENCODING_UTF8)
    {
        return utf8_char_count(str->data, str->length);
    }
    
    // 对于其他编码，暂时返回字节数
    return str->length;
}

size_t CN_string_capacity(const Stru_CN_String_t* str)
{
    if (str == NULL)
    {
        return 0;
    }
    return str->capacity;
}

Eum_CN_StringEncoding_t CN_string_encoding(const Stru_CN_String_t* str)
{
    if (str == NULL)
    {
        return Eum_STRING_ENCODING_ASCII;
    }
    return str->encoding;
}

bool CN_string_is_empty(const Stru_CN_String_t* str)
{
    if (str == NULL || str->data == NULL)
    {
        return true;
    }
    return str->length == 0;
}

const char* CN_string_cstr(const Stru_CN_String_t* str)
{
    if (str == NULL)
    {
        return NULL;
    }
    return str->data;
}

// ============================================================================
// 字符串基本操作
// ============================================================================

void CN_string_clear(Stru_CN_String_t* str)
{
    if (str == NULL || str->data == NULL)
    {
        return;
    }
    
    str->length = 0;
    str->data[0] = '\0';
}

bool CN_string_ensure_capacity(Stru_CN_String_t* str, size_t min_capacity)
{
    if (str == NULL)
    {
        return false;
    }
    
    if (min_capacity <= str->capacity)
    {
        return true;
    }
    
    return resize_string(str, min_capacity);
}

bool CN_string_shrink_to_fit(Stru_CN_String_t* str)
{
    if (str == NULL)
    {
        return false;
    }
    
    size_t new_capacity = str->length + 1;  // 包括null终止符
    if (new_capacity >= str->capacity)
    {
        return true;  // 不需要调整
    }
    
    return resize_string(str, new_capacity);
}

bool CN_string_append(Stru_CN_String_t* str, const Stru_CN_String_t* append)
{
    if (str == NULL || append == NULL)
    {
        return false;
    }
    
    return CN_string_append_cstr(str, append->data, append->length);
}

bool CN_string_append_cstr(Stru_CN_String_t* str, const char* cstr, size_t length)
{
    if (str == NULL || cstr == NULL)
    {
        return false;
    }
    
    size_t append_length = calculate_length(cstr, length);
    if (append_length == 0)
    {
        return true;  // 没有数据要追加
    }
    
    // 确保有足够容量
    size_t new_length = str->length + append_length;
    if (!CN_string_ensure_capacity(str, new_length + 1))  // +1 for null terminator
    {
        return false;
    }
    
    // 追加数据
    memcpy(str->data + str->length, cstr, append_length);
    str->length = new_length;
    str->data[str->length] = '\0';
    
    return true;
}

bool CN_string_append_char(Stru_CN_String_t* str, char ch)
{
    if (str == NULL)
    {
        return false;
    }
    
    // 确保有足够容量
    if (!CN_string_ensure_capacity(str, str->length + 2))  // +1 for char, +1 for null terminator
    {
        return false;
    }
    
    // 追加字符
    str->data[str->length] = ch;
    str->length++;
    str->data[str->length] = '\0';
    
    return true;
}

bool CN_string_insert(Stru_CN_String_t* str, size_t pos,
                      const Stru_CN_String_t* insert)
{
    if (str == NULL || insert == NULL)
    {
        return false;
    }
    
    // 检查位置是否有效
    if (pos > str->length)
    {
        pos = str->length;  // 插入到末尾
    }
    
    // 确保有足够容量
    size_t new_length = str->length + insert->length;
    if (!CN_string_ensure_capacity(str, new_length + 1))  // +1 for null terminator
    {
        return false;
    }
    
    // 移动现有数据为插入腾出空间
    if (pos < str->length)
    {
        memmove(str->data + pos + insert->length, 
                str->data + pos, 
                str->length - pos);
    }
    
    // 插入新数据
    memcpy(str->data + pos, insert->data, insert->length);
    str->length = new_length;
    str->data[str->length] = '\0';
    
    return true;
}

bool CN_string_remove(Stru_CN_String_t* str, size_t pos, size_t count)
{
    if (str == NULL)
    {
        return false;
    }
    
    // 检查位置是否有效
    if (pos >= str->length)
    {
        return false;
    }
    
    // 调整删除数量
    if (count > str->length - pos)
    {
        count = str->length - pos;
    }
    
    if (count == 0)
    {
        return true;  // 没有数据要删除
    }
    
    // 移动数据覆盖要删除的部分
    size_t remaining = str->length - pos - count;
    if (remaining > 0)
    {
        memmove(str->data + pos, 
                str->data + pos + count, 
                remaining);
    }
    
    str->length -= count;
    str->data[str->length] = '\0';
    
    return true;
}

bool CN_string_replace(Stru_CN_String_t* str, size_t pos, size_t count,
                       const Stru_CN_String_t* replacement)
{
    if (str == NULL)
    {
        return false;
    }
    
    // 先删除旧内容
    if (!CN_string_remove(str, pos, count))
    {
        return false;
    }
    
    // 再插入新内容
    if (replacement != NULL)
    {
        return CN_string_insert(str, pos, replacement);
    }
    
    return true;
}

Stru_CN_String_t* CN_string_substr(const Stru_CN_String_t* str,
                                   size_t pos, size_t count)
{
    if (str == NULL)
    {
        return NULL;
    }
    
    // 检查位置是否有效
    if (pos >= str->length)
    {
        return CN_string_create_empty(16, str->encoding);
    }
    
    // 调整提取数量
    if (count > str->length - pos || count == 0)
    {
        count = str->length - pos;
    }
    
    // 创建子字符串
    return CN_string_create(str->data + pos, count, str->encoding);
}

// ============================================================================
// 字符串工具函数
// ============================================================================

char* CN_string_duplicate_cstr(const char* cstr, size_t length)
{
    size_t actual_length = calculate_length(cstr, length);
    
    char* duplicate = (char*)cn_malloc(actual_length + 1);
    if (duplicate == NULL)
    {
        return NULL;
    }
    
    if (actual_length > 0 && cstr != NULL)
    {
        memcpy(duplicate, cstr, actual_length);
    }
    duplicate[actual_length] = '\0';
    
    return duplicate;
}
