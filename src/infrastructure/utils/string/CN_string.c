/******************************************************************************
 * 文件名: CN_string.c
 * 功能: CN_Language字符串处理工具实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现字符串处理功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_string.h"
#include "../../memory/CN_memory.h"
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>
#include <locale.h>
#include <limits.h>
#include <stdint.h>

// ============================================================================
// 字符串结构体定义（私有）
// ============================================================================

/**
 * @brief 字符串结构体（完整定义）
 */
struct Stru_CN_String_t
{
    char* data;                         /**< 字符串数据（以null结尾） */
    size_t length;                      /**< 字符串长度（字节数，不包括null终止符） */
    size_t capacity;                    /**< 缓冲区容量（字节数） */
    Eum_CN_StringEncoding_t encoding;   /**< 字符串编码 */
    int ref_count;                      /**< 引用计数 */
};

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
// 字符串操作
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
// 字符串比较（基础实现）
// ============================================================================

int CN_string_compare(const Stru_CN_String_t* str1,
                      const Stru_CN_String_t* str2,
                      Eum_CN_StringCompareOption_t options)
{
    if (str1 == NULL && str2 == NULL)
    {
        return 0;
    }
    if (str1 == NULL)
    {
        return -1;
    }
    if (str2 == NULL)
    {
        return 1;
    }
    
    // 基础实现：区分大小写的比较
    if (options == Eum_STRING_COMPARE_CASE_INSENSITIVE)
    {
        // 简单的不区分大小写比较（仅支持ASCII）
        const char* s1 = str1->data;
        const char* s2 = str2->data;
        size_t len1 = str1->length;
        size_t len2 = str2->length;
        size_t min_len = len1 < len2 ? len1 : len2;
        
        for (size_t i = 0; i < min_len; i++)
        {
            char c1 = toupper((unsigned char)s1[i]);
            char c2 = toupper((unsigned char)s2[i]);
            if (c1 != c2)
            {
                return c1 - c2;
            }
        }
        
        if (len1 != len2)
        {
            return len1 - len2;
        }
        return 0;
    }
    else
    {
        // 区分大小写的比较
        int result = memcmp(str1->data, str2->data, 
                           str1->length < str2->length ? str1->length : str2->length);
        if (result != 0)
        {
            return result;
        }
        
        // 如果公共部分相等，比较长度
        if (str1->length != str2->length)
        {
            return str1->length - str2->length;
        }
        return 0;
    }
}

bool CN_string_equal(const Stru_CN_String_t* str1,
                     const Stru_CN_String_t* str2,
                     bool case_sensitive)
{
    if (str1 == NULL && str2 == NULL)
    {
        return true;
    }
    if (str1 == NULL || str2 == NULL)
    {
        return false;
    }
    
    if (str1->length != str2->length)
    {
        return false;
    }
    
    Eum_CN_StringCompareOption_t options = case_sensitive ? 
        Eum_STRING_COMPARE_DEFAULT : Eum_STRING_COMPARE_CASE_INSENSITIVE;
    
    return CN_string_compare(str1, str2, options) == 0;
}

bool CN_string_starts_with(const Stru_CN_String_t* str,
                           const Stru_CN_String_t* prefix,
                           bool case_sensitive)
{
    if (str == NULL || prefix == NULL)
    {
        return false;
    }
    
    if (prefix->length > str->length)
    {
        return false;
    }
    
    // 提取前缀部分进行比较
    Stru_CN_String_t* substr = CN_string_substr(str, 0, prefix->length);
    if (substr == NULL)
    {
        return false;
    }
    
    bool result = CN_string_equal(substr, prefix, case_sensitive);
    CN_string_destroy(substr);
    
    return result;
}

bool CN_string_ends_with(const Stru_CN_String_t* str,
                         const Stru_CN_String_t* suffix,
                         bool case_sensitive)
{
    if (str == NULL || suffix == NULL)
    {
        return false;
    }
    
    if (suffix->length > str->length)
    {
        return false;
    }
    
    // 计算后缀开始位置
    size_t start_pos = str->length - suffix->length;
    
    // 提取后缀部分进行比较
    Stru_CN_String_t* substr = CN_string_substr(str, start_pos, suffix->length);
    if (substr == NULL)
    {
        return false;
    }
    
    bool result = CN_string_equal(substr, suffix, case_sensitive);
    CN_string_destroy(substr);
    
    return result;
}

size_t CN_string_find(const Stru_CN_String_t* str,
                      const Stru_CN_String_t* substr,
                      size_t start_pos, bool case_sensitive)
{
    if (str == NULL || substr == NULL || substr->length == 0)
    {
        return SIZE_MAX;
    }
    
    if (start_pos >= str->length)
    {
        return SIZE_MAX;
    }
    
    if (substr->length > str->length - start_pos)
    {
        return SIZE_MAX;
    }
    
    // 基础实现：简单搜索
    const char* haystack = str->data + start_pos;
    const char* needle = substr->data;
    size_t haystack_len = str->length - start_pos;
    size_t needle_len = substr->length;
    
    if (case_sensitive)
    {
        // 区分大小写的搜索
        for (size_t i = 0; i <= haystack_len - needle_len; i++)
        {
            if (memcmp(haystack + i, needle, needle_len) == 0)
            {
                return start_pos + i;
            }
        }
    }
    else
    {
        // 不区分大小写的搜索（仅支持ASCII）
        for (size_t i = 0; i <= haystack_len - needle_len; i++)
        {
            bool match = true;
            for (size_t j = 0; j < needle_len; j++)
            {
                char c1 = toupper((unsigned char)haystack[i + j]);
                char c2 = toupper((unsigned char)needle[j]);
                if (c1 != c2)
                {
                    match = false;
                    break;
                }
            }
            if (match)
            {
                return start_pos + i;
            }
        }
    }
    
    return SIZE_MAX;
}

// ============================================================================
// 字符串转换
// ============================================================================

bool CN_string_to_upper(Stru_CN_String_t* str)
{
    if (str == NULL || str->data == NULL)
    {
        return false;
    }
    
    // 简单实现：仅支持ASCII字符
    for (size_t i = 0; i < str->length; i++)
    {
        if (str->data[i] >= 'a' && str->data[i] <= 'z')
        {
            str->data[i] = str->data[i] - 'a' + 'A';
        }
    }
    
    return true;
}

bool CN_string_to_lower(Stru_CN_String_t* str)
{
    if (str == NULL || str->data == NULL)
    {
        return false;
    }
    
    // 简单实现：仅支持ASCII字符
    for (size_t i = 0; i < str->length; i++)
    {
        if (str->data[i] >= 'A' && str->data[i] <= 'Z')
        {
            str->data[i] = str->data[i] - 'A' + 'a';
        }
    }
    
    return true;
}

void CN_string_trim(Stru_CN_String_t* str)
{
    if (str == NULL || str->data == NULL)
    {
        return;
    }
    
    CN_string_trim_start(str);
    CN_string_trim_end(str);
}

void CN_string_trim_start(Stru_CN_String_t* str)
{
    if (str == NULL || str->data == NULL)
    {
        return;
    }
    
    size_t start = 0;
    while (start < str->length && isspace((unsigned char)str->data[start]))
    {
        start++;
    }
    
    if (start > 0)
    {
        CN_string_remove(str, 0, start);
    }
}

void CN_string_trim_end(Stru_CN_String_t* str)
{
    if (str == NULL || str->data == NULL)
    {
        return;
    }
    
    size_t end = str->length;
    while (end > 0 && isspace((unsigned char)str->data[end - 1]))
    {
        end--;
    }
    
    if (end < str->length)
    {
        str->length = end;
        str->data[str->length] = '\0';
    }
}

bool CN_string_convert_encoding(Stru_CN_String_t* str,
                                Eum_CN_StringEncoding_t new_encoding)
{
    if (str == NULL)
    {
        return false;
    }
    
    // 基础实现：目前只支持UTF-8到UTF-8的"转换"
    if (str->encoding == new_encoding)
    {
        return true;
    }
    
    // TODO: 实现真正的编码转换
    // 目前只记录新编码
    str->encoding = new_encoding;
    return true;
}

// ============================================================================
// 格式化字符串（基础实现）
// ============================================================================

Stru_CN_String_t* CN_string_format(const char* format, ...)
{
    if (format == NULL)
    {
        return NULL;
    }
    
    va_list args;
    va_start(args, format);
    
    // 第一次调用确定需要的缓冲区大小
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    
    if (needed < 0)
    {
        va_end(args);
        return NULL;
    }
    
    // 分配缓冲区
    size_t buffer_size = (size_t)needed + 1;
    char* buffer = (char*)cn_malloc(buffer_size);
    if (buffer == NULL)
    {
        va_end(args);
        return NULL;
    }
    
    // 实际格式化
    int written = vsnprintf(buffer, buffer_size, format, args);
    va_end(args);
    
    if (written < 0)
    {
        cn_free(buffer);
        return NULL;
    }
    
    // 创建字符串
    Stru_CN_String_t* str = CN_string_create(buffer, (size_t)written, Eum_STRING_ENCODING_UTF8);
    cn_free(buffer);
    
    return str;
}

bool CN_string_append_format(Stru_CN_String_t* str, const char* format, ...)
{
    if (str == NULL || format == NULL)
    {
        return false;
    }
    
    va_list args;
    va_start(args, format);
    
    // 第一次调用确定需要的缓冲区大小
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    
    if (needed < 0)
    {
        va_end(args);
        return false;
    }
    
    // 分配缓冲区
    size_t buffer_size = (size_t)needed + 1;
    char* buffer = (char*)cn_malloc(buffer_size);
    if (buffer == NULL)
    {
        va_end(args);
        return false;
    }
    
    // 实际格式化
    int written = vsnprintf(buffer, buffer_size, format, args);
    va_end(args);
    
    if (written < 0)
    {
        cn_free(buffer);
        return false;
    }
    
    // 追加到字符串
    bool result = CN_string_append_cstr(str, buffer, (size_t)written);
    cn_free(buffer);
    
    return result;
}

// ============================================================================
// 工具函数
// ============================================================================

Stru_CN_String_t** CN_string_split(const Stru_CN_String_t* str,
                                   const Stru_CN_String_t* delimiter,
                                   size_t max_parts, size_t* part_count)
{
    if (str == NULL || delimiter == NULL || delimiter->length == 0 || part_count == NULL)
    {
        if (part_count != NULL)
        {
            *part_count = 0;
        }
        return NULL;
    }
    
    // 计算最大可能的部分数
    size_t max_possible_parts = str->length / delimiter->length + 2;
    if (max_parts == 0 || max_parts > max_possible_parts)
    {
        max_parts = max_possible_parts;
    }
    
    // 分配结果数组
    Stru_CN_String_t** parts = (Stru_CN_String_t**)cn_calloc(max_parts, sizeof(Stru_CN_String_t*));
    if (parts == NULL)
    {
        *part_count = 0;
        return NULL;
    }
    
    size_t count = 0;
    size_t start = 0;
    size_t pos = 0;
    
    while (pos <= str->length && count < max_parts - 1)
    {
        // 查找分隔符
        size_t found = CN_string_find(str, delimiter, pos, true);
        if (found == SIZE_MAX)
        {
            break;
        }
        
        // 提取子字符串
        parts[count] = CN_string_substr(str, start, found - start);
        if (parts[count] == NULL)
        {
            // 清理并返回
            for (size_t i = 0; i < count; i++)
            {
                CN_string_destroy(parts[i]);
            }
            cn_free(parts);
            *part_count = 0;
            return NULL;
        }
        
        count++;
        start = found + delimiter->length;
        pos = start;
    }
    
    // 添加最后一部分
    if (count < max_parts)
    {
        parts[count] = CN_string_substr(str, start, str->length - start);
        if (parts[count] != NULL)
        {
            count++;
        }
    }
    
    *part_count = count;
    return parts;
}

Stru_CN_String_t* CN_string_join(Stru_CN_String_t* const* strings,
                                 size_t count,
                                 const Stru_CN_String_t* separator)
{
    if (strings == NULL || count == 0)
    {
        return CN_string_create_empty(16, Eum_STRING_ENCODING_UTF8);
    }
    
    // 计算总长度
    size_t total_length = 0;
    for (size_t i = 0; i < count; i++)
    {
        if (strings[i] != NULL)
        {
            total_length += strings[i]->length;
        }
    }
    
    // 添加分隔符长度
    if (separator != NULL && count > 1)
    {
        total_length += separator->length * (count - 1);
    }
    
    // 创建结果字符串
    Stru_CN_String_t* result = CN_string_create_empty(total_length + 1, Eum_STRING_ENCODING_UTF8);
    if (result == NULL)
    {
        return NULL;
    }
    
    // 连接字符串
    for (size_t i = 0; i < count; i++)
    {
        if (strings[i] != NULL)
        {
            CN_string_append(result, strings[i]);
        }
        
        // 添加分隔符（除了最后一个）
        if (separator != NULL && i < count - 1)
        {
            CN_string_append(result, separator);
        }
    }
    
    return result;
}

Stru_CN_String_t* CN_string_escape(const Stru_CN_String_t* str)
{
    if (str == NULL)
    {
        return NULL;
    }
    
    // 计算转义后的长度
    size_t escaped_length = 0;
    for (size_t i = 0; i < str->length; i++)
    {
        char c = str->data[i];
        switch (c)
        {
            case '\n': escaped_length += 2; break;  // \n
            case '\t': escaped_length += 2; break;  // \t
            case '\r': escaped_length += 2; break;  // \r
            case '\"': escaped_length += 2; break;  // \"
            case '\\': escaped_length += 2; break;  // 反斜杠
            default:   escaped_length += 1; break;
        }
    }
    
    // 创建转义字符串
    Stru_CN_String_t* escaped = CN_string_create_empty(escaped_length + 1, str->encoding);
    if (escaped == NULL)
    {
        return NULL;
    }
    
    // 执行转义
    for (size_t i = 0; i < str->length; i++)
    {
        char c = str->data[i];
        switch (c)
        {
            case '\n': CN_string_append_cstr(escaped, "\\n", 2); break;
            case '\t': CN_string_append_cstr(escaped, "\\t", 2); break;
            case '\r': CN_string_append_cstr(escaped, "\\r", 2); break;
            case '\"': CN_string_append_cstr(escaped, "\\\"", 2); break;
            case '\\': CN_string_append_cstr(escaped, "\\\\", 2); break;
            default:   CN_string_append_char(escaped, c); break;
        }
    }
    
    return escaped;
}

Stru_CN_String_t* CN_string_unescape(const Stru_CN_String_t* str)
{
    if (str == NULL)
    {
        return NULL;
    }
    
    // 创建反转义字符串
    Stru_CN_String_t* unescaped = CN_string_create_empty(str->length + 1, str->encoding);
    if (unescaped == NULL)
    {
        return NULL;
    }
    
    // 执行反转义
    for (size_t i = 0; i < str->length; i++)
    {
        char c = str->data[i];
        if (c == '\\' && i + 1 < str->length)
        {
            char next = str->data[i + 1];
            switch (next)
            {
                case 'n':  CN_string_append_char(unescaped, '\n'); i++; break;
                case 't':  CN_string_append_char(unescaped, '\t'); i++; break;
                case 'r':  CN_string_append_char(unescaped, '\r'); i++; break;
                case '\"': CN_string_append_char(unescaped, '\"'); i++; break;
                case '\\': CN_string_append_char(unescaped, '\\'); i++; break;
                default:   CN_string_append_char(unescaped, c); break;
            }
        }
        else
        {
            CN_string_append_char(unescaped, c);
        }
    }
    
    return unescaped;
}

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
