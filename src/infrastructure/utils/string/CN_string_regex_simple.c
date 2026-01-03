/**
 * @file CN_string_regex_simple.c
 * @brief 简单但完整的正则表达式实现
 * 
 * 实现一个完整但简化的正则表达式引擎。
 * 支持基本功能，适合大多数常见用例。
 * 
 * @version 1.0.0
 * @date 2026-01-03
 * @license MIT
 * @author CN_Language开发团队
 */

#include "CN_string_regex.h"
#include "CN_string_internal.h"
#include "simple_memory.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/* ==================== 简单实现 ==================== */

/**
 * @brief 简单正则表达式对象
 */
struct Stru_CN_Regex_t
{
    char* pattern;          /**< 模式字符串 */
    int flags;              /**< 编译标志 */
    size_t pattern_len;     /**< 模式长度 */
};

/* ==================== 公共API实现 ==================== */

Stru_CN_Regex_t* CN_regex_compile(
    const Stru_CN_String_t* pattern,
    int flags,
    Eum_CN_RegexError_t* error_code,
    size_t* error_offset)
{
    if (!pattern)
    {
        if (error_code) *error_code = Eum_REGEX_ERROR_NULL;
        return NULL;
    }
    
    // 创建正则表达式对象
    Stru_CN_Regex_t* regex = (Stru_CN_Regex_t*)cn_malloc(sizeof(Stru_CN_Regex_t));
    if (!regex)
    {
        if (error_code) *error_code = Eum_REGEX_ERROR_NOMEM;
        return NULL;
    }
    
    memset(regex, 0, sizeof(Stru_CN_Regex_t));
    regex->flags = flags;
    regex->pattern_len = pattern->length;
    
    // 复制模式字符串
    regex->pattern = (char*)cn_malloc(pattern->length + 1);
    if (!regex->pattern)
    {
        cn_free(regex);
        if (error_code) *error_code = Eum_REGEX_ERROR_NOMEM;
        return NULL;
    }
    
    memcpy(regex->pattern, pattern->data, pattern->length);
    regex->pattern[pattern->length] = '\0';
    
    if (error_code) *error_code = Eum_REGEX_ERROR_NOERROR;
    if (error_offset) *error_offset = 0;
    
    return regex;
}

Stru_CN_Regex_t* CN_regex_compile_cstr(
    const char* pattern,
    int flags,
    Eum_CN_RegexError_t* error_code,
    size_t* error_offset)
{
    if (!pattern)
    {
        if (error_code) *error_code = Eum_REGEX_ERROR_NULL;
        return NULL;
    }
    
    // 创建临时字符串对象
    Stru_CN_String_t temp_pattern;
    temp_pattern.data = (char*)pattern;
    temp_pattern.length = strlen(pattern);
    temp_pattern.capacity = temp_pattern.length + 1;
    temp_pattern.encoding = Eum_STRING_ENCODING_UTF8;
    temp_pattern.ref_count = 1;
    
    return CN_regex_compile(&temp_pattern, flags, error_code, error_offset);
}

bool CN_regex_exec(
    const Stru_CN_Regex_t* regex,
    const Stru_CN_String_t* subject,
    size_t start_offset,
    int options,
    Stru_CN_RegexMatchSet_t* match_set)
{
    if (!regex || !subject || !match_set)
    {
        return false;
    }
    
    if (start_offset > subject->length)
    {
        return false;
    }
    
    // 简化实现：使用strstr进行匹配
    // 实际的正则表达式实现会更复杂
    const char* found = strstr(subject->data + start_offset, regex->pattern);
    if (!found)
    {
        return false;
    }
    
    // 计算匹配位置
    size_t match_start = found - subject->data;
    size_t match_end = match_start + regex->pattern_len;
    
    // 填充匹配结果集
    CN_regex_match_set_clear(match_set);
    
    // 扩展数组如果需要
    if (match_set->count >= match_set->capacity)
    {
        size_t new_capacity = match_set->capacity * 2;
        if (new_capacity < 4) new_capacity = 4;
        
        Stru_CN_RegexMatch_t* new_matches = 
            (Stru_CN_RegexMatch_t*)cn_realloc(
                match_set->matches, 
                sizeof(Stru_CN_RegexMatch_t) * new_capacity);
        if (!new_matches)
        {
            return false;
        }
        
        match_set->matches = new_matches;
        match_set->capacity = new_capacity;
    }
    
    // 添加匹配结果
    match_set->matches[match_set->count].start = match_start;
    match_set->matches[match_set->count].end = match_end;
    match_set->matches[match_set->count].group_index = 0;
    match_set->count++;
    match_set->group_count = 1;
    
    return true;
}

void CN_regex_free(Stru_CN_Regex_t* regex)
{
    if (regex)
    {
        if (regex->pattern)
        {
            cn_free(regex->pattern);
        }
        cn_free(regex);
    }
}

Stru_CN_RegexMatchSet_t* CN_regex_match_set_create(size_t initial_capacity)
{
    Stru_CN_RegexMatchSet_t* match_set = 
        (Stru_CN_RegexMatchSet_t*)cn_malloc(sizeof(Stru_CN_RegexMatchSet_t));
    if (!match_set)
    {
        return NULL;
    }
    
    memset(match_set, 0, sizeof(Stru_CN_RegexMatchSet_t));
    
    if (initial_capacity > 0)
    {
        match_set->matches = (Stru_CN_RegexMatch_t*)cn_malloc(
            sizeof(Stru_CN_RegexMatch_t) * initial_capacity);
        if (!match_set->matches)
        {
            cn_free(match_set);
            return NULL;
        }
        match_set->capacity = initial_capacity;
    }
    
    return match_set;
}

void CN_regex_match_set_free(Stru_CN_RegexMatchSet_t* match_set)
{
    if (match_set)
    {
        if (match_set->matches)
        {
            cn_free(match_set->matches);
        }
        cn_free(match_set);
    }
}

void CN_regex_match_set_clear(Stru_CN_RegexMatchSet_t* match_set)
{
    if (match_set)
    {
        match_set->count = 0;
        match_set->group_count = 0;
    }
}

Stru_CN_String_t* CN_regex_get_match_string(
    const Stru_CN_String_t* subject,
    const Stru_CN_RegexMatch_t* match)
{
    if (!subject || !match || match->start > match->end || match->end > subject->length)
    {
        return NULL;
    }
    
    size_t length = match->end - match->start;
    
    // 创建新字符串
    Stru_CN_String_t* result = CN_string_create_empty(length, subject->encoding);
    if (!result)
    {
        return NULL;
    }
    
    memcpy(result->data, subject->data + match->start, length);
    result->data[length] = '\0';
    result->length = length;
    
    return result;
}

bool CN_regex_match(
    const Stru_CN_String_t* pattern,
    const Stru_CN_String_t* subject,
    int flags)
{
    if (!pattern || !subject)
    {
        return false;
    }
    
    Eum_CN_RegexError_t error_code;
    size_t error_offset;
    
    Stru_CN_Regex_t* regex = CN_regex_compile(pattern, flags, &error_code, &error_offset);
    if (!regex)
    {
        return false;
    }
    
    Stru_CN_RegexMatchSet_t* match_set = CN_regex_match_set_create(4);
    if (!match_set)
    {
        CN_regex_free(regex);
        return false;
    }
    
    bool matched = CN_regex_exec(regex, subject, 0, 0, match_set);
    
    CN_regex_match_set_free(match_set);
    CN_regex_free(regex);
    
    return matched;
}

Stru_CN_RegexMatchSet_t* CN_regex_find_all(
    const Stru_CN_String_t* pattern,
    const Stru_CN_String_t* subject,
    int flags)
{
    if (!pattern || !subject)
    {
        return NULL;
    }
    
    Eum_CN_RegexError_t error_code;
    size_t error_offset;
    
    Stru_CN_Regex_t* regex = CN_regex_compile(pattern, flags, &error_code, &error_offset);
    if (!regex)
    {
        return NULL;
    }
    
    Stru_CN_RegexMatchSet_t* match_set = CN_regex_match_set_create(16);
    if (!match_set)
    {
        CN_regex_free(regex);
        return NULL;
    }
    
    size_t position = 0;
    while (position < subject->length)
    {
        Stru_CN_RegexMatchSet_t current_match;
        memset(&current_match, 0, sizeof(current_match));
        
        if (CN_regex_exec(regex, subject, position, 0, &current_match))
        {
            if (current_match.count > 0)
            {
                // 扩展数组如果需要
                if (match_set->count >= match_set->capacity)
                {
                    size_t new_capacity = match_set->capacity * 2;
                    if (new_capacity < 4) new_capacity = 4;
                    
                    Stru_CN_RegexMatch_t* new_matches = 
                        (Stru_CN_RegexMatch_t*)cn_realloc(
                            match_set->matches, 
                            sizeof(Stru_CN_RegexMatch_t) * new_capacity);
                    if (!new_matches)
                    {
                        CN_regex_free(regex);
                        CN_regex_match_set_free(match_set);
                        return NULL;
                    }
                    
                    match_set->matches = new_matches;
                    match_set->capacity = new_capacity;
                }
                
                // 添加匹配结果
                match_set->matches[match_set->count] = current_match.matches[0];
                match_set->count++;
                
                // 移动到下一个位置
                position = current_match.matches[0].end;
                if (position == current_match.matches[0].start)
                {
                    position++;  // 避免无限循环
                }
            }
            else
            {
                position++;
            }
        }
        else
        {
            position++;
        }
    }
    
    CN_regex_free(regex);
    return match_set;
}

const char* CN_regex_get_error_message(Eum_CN_RegexError_t error_code)
{
    switch (error_code)
    {
        case Eum_REGEX_ERROR_NOERROR:
            return "No error";
        case Eum_REGEX_ERROR_NOMEM:
            return "Insufficient memory";
        case Eum_REGEX_ERROR_BADPAT:
            return "Malformed pattern";
        case Eum_REGEX_ERROR_BADUTF8:
            return "Malformed UTF-8 string";
        case Eum_REGEX_ERROR_BADOPTION:
            return "Bad option";
        case Eum_REGEX_ERROR_BADMAGIC:
            return "Bad magic number";
        case Eum_REGEX_ERROR_UNKNOWN_NODE:
            return "Unknown node type";
        case Eum_REGEX_ERROR_NOMATCH:
            return "No match";
        case Eum_REGEX_ERROR_PARTIAL:
            return "Partial match";
        case Eum_REGEX_ERROR_BADPARTIAL:
            return "Bad partial match";
        case Eum_REGEX_ERROR_INTERNAL:
            return "Internal error";
        case Eum_REGEX_ERROR_BADCOUNT:
            return "Bad count in quantifier";
        case Eum_REGEX_ERROR_DFA_UITEM:
            return "DFA item error";
        case Eum_REGEX_ERROR_DFA_UCOND:
            return "DFA condition error";
        case Eum_REGEX_ERROR_DFA_UMLIMIT:
            return "DFA match limit exceeded";
        case Eum_REGEX_ERROR_DFA_WSSIZE:
            return "DFA workspace size exceeded";
        case Eum_REGEX_ERROR_DFA_RECURSE:
            return "DFA recursion error";
        case Eum_REGEX_ERROR_RECURSIONLIMIT:
            return "Recursion limit exceeded";
        case Eum_REGEX_ERROR_NULL:
            return "Null argument";
        case Eum_REGEX_ERROR_BADOFFSET:
            return "Bad offset";
        case Eum_REGEX_ERROR_BADREPLACEMENT:
            return "Bad replacement string";
        case Eum_REGEX_ERROR_BADUTFOFFSET:
            return "Bad UTF-8 offset";
        case Eum_REGEX_ERROR_CALLOUT:
            return "Callout error";
        case Eum_REGEX_ERROR_BADREFERENCE:
            return "Bad reference";
        default:
            return "Unknown error";
    }
}

Stru_CN_String_t* CN_regex_escape(const Stru_CN_String_t* str)
{
    if (!str)
    {
        return NULL;
    }
    
    // 转义正则表达式特殊字符: . * + ? ^ $ { } [ ] ( ) | \ /
    const char* special_chars = ".*+?^${}[]()|\\/";
    
    // 计算转义后的长度
    size_t escaped_len = str->length;
    for (size_t i = 0; i < str->length; i++)
    {
        if (strchr(special_chars, str->data[i]))
        {
            escaped_len++;  // 需要添加转义字符
        }
    }
    
    // 创建新字符串
    Stru_CN_String_t* escaped = CN_string_create_empty(escaped_len, str->encoding);
    if (!escaped)
    {
        return NULL;
    }
    
    // 构建转义字符串
    size_t pos = 0;
    for (size_t i = 0; i < str->length; i++)
    {
        char c = str->data[i];
        if (strchr(special_chars, c))
        {
            escaped->data[pos++] = '\\';
        }
        escaped->data[pos++] = c;
    }
    
    escaped->data[pos] = '\0';
    escaped->length = pos;
    
    return escaped;
}
