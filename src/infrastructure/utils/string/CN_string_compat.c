/******************************************************************************
 * 文件名: CN_string_compat.c
 * 功能: CN_Language字符串兼容性模块实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现向后兼容的函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_string.h"
#include "CN_string_internal.h"  // 使用内部头文件获取结构体定义
#include "CN_string_unicode.h"
#include "simple_memory.h"       // 使用简单的内存包装器
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>
#include <locale.h>
#include <limits.h>
#include <stdint.h>

// ============================================================================
// 字符串比较函数实现（兼容旧版本）
// ============================================================================

/**
 * @brief 比较两个字符串（兼容旧版本）
 */
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
        // 使用Unicode不区分大小写比较
        return CN_string_compare_case_insensitive_unicode(str1, str2);
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

/**
 * @brief 检查字符串是否相等（兼容旧版本）
 */
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

/**
 * @brief 检查字符串是否以指定前缀开头（兼容旧版本）
 */
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

/**
 * @brief 检查字符串是否以指定后缀结尾（兼容旧版本）
 */
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

/**
 * @brief 查找子字符串（兼容旧版本）
 */
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
        // 不区分大小写的搜索（使用Unicode版本）
        // 简单实现：转换为小写后搜索
        Stru_CN_String_t* lower_haystack = CN_string_to_lower_copy(str);
        Stru_CN_String_t* lower_needle = CN_string_to_lower_copy(substr);
        
        if (lower_haystack == NULL || lower_needle == NULL)
        {
            if (lower_haystack != NULL) CN_string_destroy(lower_haystack);
            if (lower_needle != NULL) CN_string_destroy(lower_needle);
            
            // 回退到简单ASCII搜索
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
        else
        {
            // 使用小写版本搜索
            size_t result = CN_string_find(lower_haystack, lower_needle, start_pos, true);
            CN_string_destroy(lower_haystack);
            CN_string_destroy(lower_needle);
            
            if (result != SIZE_MAX)
            {
                return result;
            }
        }
    }
    
    return SIZE_MAX;
}

// ============================================================================
// 字符串转换函数实现（兼容旧版本）
// ============================================================================

/**
 * @brief 转换为大写（兼容旧版本，使用Unicode版本）
 */
bool CN_string_to_upper(Stru_CN_String_t* str)
{
    // 使用Unicode版本
    return CN_string_to_upper_unicode(str);
}

/**
 * @brief 转换为小写（兼容旧版本，使用Unicode版本）
 */
bool CN_string_to_lower(Stru_CN_String_t* str)
{
    // 使用Unicode版本
    return CN_string_to_lower_unicode(str);
}

/**
 * @brief 去除首尾空白字符（兼容旧版本）
 */
void CN_string_trim(Stru_CN_String_t* str)
{
    if (str == NULL || str->data == NULL)
    {
        return;
    }
    
    CN_string_trim_start(str);
    CN_string_trim_end(str);
}

/**
 * @brief 去除开头空白字符（兼容旧版本）
 */
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

/**
 * @brief 去除结尾空白字符（兼容旧版本）
 */
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

/**
 * @brief 转换字符串编码（兼容旧版本）
 */
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
// 格式化字符串函数实现（兼容旧版本）
// ============================================================================

/**
 * @brief 创建格式化字符串（兼容旧版本）
 */
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

/**
 * @brief 追加格式化字符串（兼容旧版本）
 */
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
// 工具函数实现（兼容旧版本）
// ============================================================================

/**
 * @brief 分割字符串（兼容旧版本）
 */
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

/**
 * @brief 连接字符串数组（兼容旧版本）
 */
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

/**
 * @brief 转义字符串（兼容旧版本）
 */
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
            default: CN_string_append_char(escaped, c); break;
        }
    }
    
    return escaped;
}

/**
 * @brief 反转义字符串（兼容旧版本）
 */
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
                case 'n': CN_string_append_char(unescaped, '\n'); i++; break;
                case 't': CN_string_append_char(unescaped, '\t'); i++; break;
                case 'r': CN_string_append_char(unescaped, '\r'); i++; break;
                case '\"': CN_string_append_char(unescaped, '\"'); i++; break;
                case '\\': CN_string_append_char(unescaped, '\\'); i++; break;
                default: CN_string_append_char(unescaped, c); break;
            }
        }
        else
        {
            CN_string_append_char(unescaped, c);
        }
    }
    
    return unescaped;
}
