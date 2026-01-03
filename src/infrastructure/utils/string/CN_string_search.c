/**
 * @file CN_string_search.c
 * @brief 字符串搜索算法实现
 * 
 * 实现优化的字符串搜索算法，包括：
 * - Boyer-Moore算法
 * - Knuth-Morris-Pratt (KMP)算法
 * - Rabin-Karp算法
 * - 多模式匹配
 * 
 * @version 1.0.0
 * @date 2026-01-03
 * @license MIT
 * @author CN_Language开发团队
 */

#include "CN_string_search.h"
#include "CN_string_internal.h"
#include "simple_memory.h"
#include <string.h>
#include <ctype.h>
#include <limits.h>

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 计算字符的小写形式（ASCII）
 */
static char to_lower_ascii(char c)
{
    return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
}

/**
 * @brief 比较两个字符（考虑大小写敏感性）
 */
static bool chars_equal(char a, char b, bool case_sensitive)
{
    if (case_sensitive)
    {
        return a == b;
    }
    else
    {
        return to_lower_ascii(a) == to_lower_ascii(b);
    }
}

/**
 * @brief 比较字符串区域（考虑大小写敏感性）
 */
static bool region_matches(
    const char* text, size_t text_start,
    const char* pattern, size_t pattern_start,
    size_t length, bool case_sensitive)
{
    for (size_t i = 0; i < length; i++)
    {
        if (!chars_equal(text[text_start + i], pattern[pattern_start + i], case_sensitive))
        {
            return false;
        }
    }
    return true;
}

/* ==================== Boyer-Moore算法实现 ==================== */

/**
 * @brief Boyer-Moore坏字符表
 */
static void build_bad_char_table(
    const char* pattern, size_t pattern_len,
    int bad_char[256], bool case_sensitive)
{
    // 初始化所有字符为模式长度
    for (int i = 0; i < 256; i++)
    {
        bad_char[i] = (int)pattern_len;
    }
    
    // 对于模式中的每个字符，计算从末尾的距离
    for (size_t i = 0; i < pattern_len - 1; i++)
    {
        unsigned char c = (unsigned char)pattern[i];
        if (!case_sensitive)
        {
            c = (unsigned char)to_lower_ascii((char)c);
        }
        bad_char[c] = (int)(pattern_len - 1 - i);
    }
}

/**
 * @brief Boyer-Moore好后缀表（简化版本）
 */
static void build_good_suffix_table(
    const char* pattern, size_t pattern_len,
    int* good_suffix)
{
    // 简化实现：使用基本的好后缀规则
    for (size_t i = 0; i <= pattern_len; i++)
    {
        good_suffix[i] = (int)pattern_len;
    }
    
    // 计算后缀匹配
    for (size_t i = 1; i < pattern_len; i++)
    {
        size_t j = pattern_len - 1;
        size_t k = pattern_len - 1 - i;
        
        while (j > 0 && pattern[j] == pattern[k])
        {
            if (k == 0)
            {
                good_suffix[i] = (int)(pattern_len - j);
                break;
            }
            j--;
            k--;
        }
    }
}

/* ==================== KMP算法实现 ==================== */

/**
 * @brief 构建KMP部分匹配表（前缀函数）
 */
static void build_kmp_table(
    const char* pattern, size_t pattern_len,
    int* lps, bool case_sensitive)
{
    int length = 0;  // 当前最长前缀后缀的长度
    lps[0] = 0;      // lps[0]总是0
    
    size_t i = 1;
    while (i < pattern_len)
    {
        if (chars_equal(pattern[i], pattern[length], case_sensitive))
        {
            length++;
            lps[i] = length;
            i++;
        }
        else
        {
            if (length != 0)
            {
                length = lps[length - 1];
            }
            else
            {
                lps[i] = 0;
                i++;
            }
        }
    }
}

/* ==================== Rabin-Karp算法实现 ==================== */

/**
 * @brief 计算字符串的哈希值
 */
static unsigned long compute_hash(
    const char* str, size_t length,
    unsigned long prime, unsigned long base)
{
    unsigned long hash = 0;
    for (size_t i = 0; i < length; i++)
    {
        hash = (hash * base + (unsigned char)str[i]) % prime;
    }
    return hash;
}

/**
 * @brief 计算滚动哈希
 */
static unsigned long update_hash(
    unsigned long old_hash,
    char old_char, char new_char,
    size_t pattern_len,
    unsigned long prime, unsigned long base,
    unsigned long power)
{
    // 移除旧字符的贡献
    old_hash = (old_hash + prime - (power * (unsigned char)old_char) % prime) % prime;
    // 添加新字符的贡献
    old_hash = (old_hash * base + (unsigned char)new_char) % prime;
    return old_hash;
}

/* ==================== 公共API实现 ==================== */

size_t CN_string_find_boyer_moore(
    const Stru_CN_String_t* haystack,
    const Stru_CN_String_t* needle,
    bool case_sensitive,
    size_t start_position)
{
    if (!haystack || !needle || 
        haystack->length == 0 || needle->length == 0 ||
        needle->length > haystack->length ||
        start_position >= haystack->length)
    {
        return SIZE_MAX;
    }
    
    const char* text = haystack->data;
    const char* pattern = needle->data;
    size_t n = haystack->length;
    size_t m = needle->length;
    
    // 构建坏字符表
    int bad_char[256];
    build_bad_char_table(pattern, m, bad_char, case_sensitive);
    
    // 构建好后缀表（简化）
    int* good_suffix = (int*)cn_malloc(sizeof(int) * (m + 1));
    if (!good_suffix)
    {
        return SIZE_MAX;
    }
    build_good_suffix_table(pattern, m, good_suffix);
    
    size_t s = start_position;
    while (s <= n - m)
    {
        int j = (int)m - 1;
        
        // 从右向左比较
        while (j >= 0 && chars_equal(pattern[j], text[s + j], case_sensitive))
        {
            j--;
        }
        
        if (j < 0)
        {
            // 找到匹配
            cn_free(good_suffix);
            return s;
        }
        else
        {
            // 使用坏字符规则和好后缀规则中的较大值
            unsigned char bad_char_c = (unsigned char)text[s + j];
            if (!case_sensitive)
            {
                bad_char_c = (unsigned char)to_lower_ascii((char)bad_char_c);
            }
            
            int bad_char_shift = bad_char[bad_char_c] - (m - 1 - j);
            int good_suffix_shift = good_suffix[j + 1];
            
            s += (size_t)(bad_char_shift > good_suffix_shift ? bad_char_shift : good_suffix_shift);
        }
    }
    
    cn_free(good_suffix);
    return SIZE_MAX;
}

size_t CN_string_find_kmp(
    const Stru_CN_String_t* haystack,
    const Stru_CN_String_t* needle,
    bool case_sensitive,
    size_t start_position)
{
    if (!haystack || !needle || 
        haystack->length == 0 || needle->length == 0 ||
        needle->length > haystack->length ||
        start_position >= haystack->length)
    {
        return SIZE_MAX;
    }
    
    const char* text = haystack->data;
    const char* pattern = needle->data;
    size_t n = haystack->length;
    size_t m = needle->length;
    
    // 构建部分匹配表
    int* lps = (int*)cn_malloc(sizeof(int) * m);
    if (!lps)
    {
        return SIZE_MAX;
    }
    build_kmp_table(pattern, m, lps, case_sensitive);
    
    size_t i = start_position;
    size_t j = 0;
    
    while (i < n)
    {
        if (chars_equal(pattern[j], text[i], case_sensitive))
        {
            i++;
            j++;
        }
        
        if (j == m)
        {
            // 找到匹配
            cn_free(lps);
            return i - j;
        }
        else if (i < n && !chars_equal(pattern[j], text[i], case_sensitive))
        {
            if (j != 0)
            {
                j = (size_t)lps[j - 1];
            }
            else
            {
                i++;
            }
        }
    }
    
    cn_free(lps);
    return SIZE_MAX;
}

size_t CN_string_find_rabin_karp(
    const Stru_CN_String_t* haystack,
    const Stru_CN_String_t* needle,
    bool case_sensitive,
    size_t start_position)
{
    if (!haystack || !needle || 
        haystack->length == 0 || needle->length == 0 ||
        needle->length > haystack->length ||
        start_position >= haystack->length)
    {
        return SIZE_MAX;
    }
    
    // 对于区分大小写的搜索，我们需要处理大小写转换
    // 简化实现：只处理区分大小写的情况
    if (!case_sensitive)
    {
        // 回退到KMP算法
        return CN_string_find_kmp(haystack, needle, false, start_position);
    }
    
    const char* text = haystack->data;
    const char* pattern = needle->data;
    size_t n = haystack->length;
    size_t m = needle->length;
    
    // 使用大素数和基数
    unsigned long prime = 1000000007;
    unsigned long base = 256;
    
    // 计算base^(m-1) % prime
    unsigned long power = 1;
    for (size_t i = 0; i < m - 1; i++)
    {
        power = (power * base) % prime;
    }
    
    // 计算模式和文本第一个窗口的哈希值
    unsigned long pattern_hash = compute_hash(pattern, m, prime, base);
    unsigned long text_hash = compute_hash(text + start_position, m, prime, base);
    
    // 滑动窗口
    for (size_t i = start_position; i <= n - m; i++)
    {
        // 检查哈希值是否匹配
        if (pattern_hash == text_hash)
        {
            // 验证实际字符串是否匹配（避免哈希冲突）
            if (region_matches(text, i, pattern, 0, m, true))
            {
                return i;
            }
        }
        
        // 计算下一个窗口的哈希值
        if (i < n - m)
        {
            text_hash = update_hash(
                text_hash, text[i], text[i + m],
                m, prime, base, power);
            
            // 确保哈希值为正
            if (text_hash < 0)
            {
                text_hash += prime;
            }
        }
    }
    
    return SIZE_MAX;
}

Stru_CN_SearchOptions_t CN_string_get_default_search_options(void)
{
    Stru_CN_SearchOptions_t options;
    options.algorithm = Eum_SEARCH_ALGORITHM_AUTO;
    options.case_sensitive = true;
    options.backward = false;
    options.start_position = 0;
    options.max_matches = 0;
    return options;
}

size_t CN_string_find_with_algorithm(
    const Stru_CN_String_t* haystack,
    const Stru_CN_String_t* needle,
    const Stru_CN_SearchOptions_t* options)
{
    if (!haystack || !needle || !options)
    {
        return SIZE_MAX;
    }
    
    // 获取实际算法（如果是AUTO，则选择推荐算法）
    Eum_CN_SearchAlgorithm_t algorithm = options->algorithm;
    if (algorithm == Eum_SEARCH_ALGORITHM_AUTO)
    {
        algorithm = CN_string_get_recommended_algorithm(
            needle->length, haystack->length);
    }
    
    // 根据算法调用相应的函数
    switch (algorithm)
    {
        case Eum_SEARCH_ALGORITHM_BOYER_MOORE:
            return CN_string_find_boyer_moore(
                haystack, needle, options->case_sensitive, options->start_position);
            
        case Eum_SEARCH_ALGORITHM_KMP:
            return CN_string_find_kmp(
                haystack, needle, options->case_sensitive, options->start_position);
            
        case Eum_SEARCH_ALGORITHM_RABIN_KARP:
            return CN_string_find_rabin_karp(
                haystack, needle, options->case_sensitive, options->start_position);
            
        case Eum_SEARCH_ALGORITHM_SIMPLE:
        default:
            // 简单线性搜索（向后兼容）
            {
                const char* text = haystack->data;
                const char* pattern = needle->data;
                size_t n = haystack->length;
                size_t m = needle->length;
                
                if (m == 0 || n < m || options->start_position >= n)
                {
                    return SIZE_MAX;
                }
                
                size_t end = n - m;
                for (size_t i = options->start_position; i <= end; i++)
                {
                    if (region_matches(text, i, pattern, 0, m, options->case_sensitive))
                    {
                        return i;
                    }
                }
                return SIZE_MAX;
            }
    }
}

size_t CN_string_find_cstr_with_algorithm(
    const Stru_CN_String_t* haystack,
    const char* needle,
    const Stru_CN_SearchOptions_t* options)
{
    if (!haystack || !needle || !options)
    {
        return SIZE_MAX;
    }
    
    // 创建临时字符串对象
    Stru_CN_String_t temp_needle;
    temp_needle.data = (char*)needle;
    temp_needle.length = strlen(needle);
    temp_needle.capacity = temp_needle.length + 1;
    temp_needle.encoding = Eum_STRING_ENCODING_UTF8;  // 假设UTF-8
    temp_needle.ref_count = 1;  // 临时对象，不会被释放
    
    return CN_string_find_with_algorithm(haystack, &temp_needle, options);
}

Stru_CN_MatchList_t* CN_string_find_all(
    const Stru_CN_String_t* haystack,
    const Stru_CN_String_t* needle,
    const Stru_CN_SearchOptions_t* options)
{
    if (!haystack || !needle || !options || needle->length == 0)
    {
        return NULL;
    }
    
    // 创建匹配列表
    Stru_CN_MatchList_t* match_list = (Stru_CN_MatchList_t*)cn_malloc(sizeof(Stru_CN_MatchList_t));
    if (!match_list)
    {
        return NULL;
    }
    
    match_list->matches = NULL;
    match_list->count = 0;
    match_list->capacity = 0;
    
    // 初始容量
    size_t initial_capacity = 16;
    match_list->matches = (Stru_CN_MatchResult_t*)cn_malloc(sizeof(Stru_CN_MatchResult_t) * initial_capacity);
    if (!match_list->matches)
    {
        cn_free(match_list);
        return NULL;
    }
    match_list->capacity = initial_capacity;
    
    // 搜索所有匹配
    size_t position = options->start_position;
    size_t match_index = 0;
    
    while (position < haystack->length)
    {
        Stru_CN_SearchOptions_t current_options = *options;
        current_options.start_position = position;
        
        size_t found_pos = CN_string_find_with_algorithm(haystack, needle, &current_options);
        if (found_pos == SIZE_MAX)
        {
            break;
        }
        
        // 检查是否达到最大匹配数
        if (options->max_matches > 0 && match_index >= options->max_matches)
        {
            break;
        }
        
        // 扩展数组如果需要
        if (match_list->count >= match_list->capacity)
        {
            size_t new_capacity = match_list->capacity * 2;
            Stru_CN_MatchResult_t* new_matches = (Stru_CN_MatchResult_t*)cn_realloc(
                match_list->matches, sizeof(Stru_CN_MatchResult_t) * new_capacity);
            if (!new_matches)
            {
                CN_string_free_match_list(match_list);
                return NULL;
            }
            match_list->matches = new_matches;
            match_list->capacity = new_capacity;
        }
        
        // 添加匹配结果
        match_list->matches[match_list->count].position = found_pos;
        match_list->matches[match_list->count].length = needle->length;
        match_list->matches[match_list->count].match_index = match_index;
        match_list->count++;
        
        // 移动到下一个位置
        position = found_pos + 1;
        match_index++;
    }
    
    return match_list;
}

void CN_string_free_match_list(Stru_CN_MatchList_t* match_list)
{
    if (match_list)
    {
        if (match_list->matches)
        {
            cn_free(match_list->matches);
        }
        cn_free(match_list);
    }
}

const char* CN_string_get_algorithm_name(Eum_CN_SearchAlgorithm_t algorithm)
{
    switch (algorithm)
    {
        case Eum_SEARCH_ALGORITHM_BOYER_MOORE:
            return "Boyer-Moore";
        case Eum_SEARCH_ALGORITHM_KMP:
            return "Knuth-Morris-Pratt (KMP)";
        case Eum_SEARCH_ALGORITHM_RABIN_KARP:
            return "Rabin-Karp";
        case Eum_SEARCH_ALGORITHM_SIMPLE:
            return "Simple Linear Search";
        case Eum_SEARCH_ALGORITHM_AUTO:
            return "Auto (Recommended)";
        default:
            return "Unknown";
    }
}

bool CN_string_algorithm_supports_case_sensitive(Eum_CN_SearchAlgorithm_t algorithm)
{
    // 所有算法都支持区分大小写
    // 但Rabin-Karp算法在不区分大小写时需要特殊处理
    return true;
}

Eum_CN_SearchAlgorithm_t CN_string_get_recommended_algorithm(
    size_t pattern_length,
    size_t text_length)
{
    // 简单的推荐策略：
    // - 短模式：使用Boyer-Moore
    // - 长模式：使用KMP
    // - 非常长的文本和模式：使用Rabin-Karp
    
    if (pattern_length == 0 || text_length == 0)
    {
        return Eum_SEARCH_ALGORITHM_SIMPLE;
    }
    
    if (pattern_length <= 4)
    {
        // 非常短的模式，简单搜索可能更快
        return Eum_SEARCH_ALGORITHM_SIMPLE;
    }
    else if (pattern_length <= 256)
    {
        // 中等长度模式，Boyer-Moore通常最快
        return Eum_SEARCH_ALGORITHM_BOYER_MOORE;
    }
    else if (text_length > 1000000 && pattern_length > 100)
    {
        // 非常大的文本和模式，Rabin-Karp可能更好
        return Eum_SEARCH_ALGORITHM_RABIN_KARP;
    }
    else
    {
        // 其他情况使用KMP
        return Eum_SEARCH_ALGORITHM_KMP;
    }
}

/* ==================== 多模式匹配实现（简化） ==================== */

Stru_CN_MultiPatternMatcher_t* CN_string_create_multi_pattern_matcher(
    const Stru_CN_String_t** patterns,
    size_t pattern_count,
    bool case_sensitive)
{
    // 简化实现：只存储模式指针
    // 实际实现应该使用Aho-Corasick算法
    
    if (!patterns || pattern_count == 0)
    {
        return NULL;
    }
    
    Stru_CN_MultiPatternMatcher_t* matcher = 
        (Stru_CN_MultiPatternMatcher_t*)cn_malloc(sizeof(Stru_CN_MultiPatternMatcher_t));
    if (!matcher)
    {
        return NULL;
    }
    
    // 存储模式指针（简化实现）
    matcher->internal_data = (void*)patterns;
    matcher->pattern_count = pattern_count;
    
    return matcher;
}

Stru_CN_MatchList_t* CN_string_search_multi_pattern(
    const Stru_CN_MultiPatternMatcher_t* matcher,
    const Stru_CN_String_t* text,
    size_t start_position)
{
    if (!matcher || !text)
    {
        return NULL;
    }
    
    // 简化实现：依次搜索每个模式
    const Stru_CN_String_t** patterns = (const Stru_CN_String_t**)matcher->internal_data;
    
    // 创建匹配列表
    Stru_CN_MatchList_t* match_list = (Stru_CN_MatchList_t*)cn_malloc(sizeof(Stru_CN_MatchList_t));
    if (!match_list)
    {
        return NULL;
    }
    
    match_list->matches = NULL;
    match_list->count = 0;
    match_list->capacity = 0;
    
    // 初始容量
    size_t initial_capacity = 16;
    match_list->matches = (Stru_CN_MatchResult_t*)cn_malloc(sizeof(Stru_CN_MatchResult_t) * initial_capacity);
    if (!match_list->matches)
    {
        cn_free(match_list);
        return NULL;
    }
    match_list->capacity = initial_capacity;
    
    // 搜索每个模式
    for (size_t pattern_idx = 0; pattern_idx < matcher->pattern_count; pattern_idx++)
    {
        const Stru_CN_String_t* pattern = patterns[pattern_idx];
        if (!pattern || pattern->length == 0)
        {
            continue;
        }
        
        Stru_CN_SearchOptions_t options = CN_string_get_default_search_options();
        options.start_position = start_position;
        options.case_sensitive = true;  // 简化：假设区分大小写
        
        size_t position = start_position;
        size_t match_index = 0;
        
        while (position < text->length)
        {
            options.start_position = position;
            size_t found_pos = CN_string_find_with_algorithm(text, pattern, &options);
            if (found_pos == SIZE_MAX)
            {
                break;
            }
            
            // 扩展数组如果需要
            if (match_list->count >= match_list->capacity)
            {
                size_t new_capacity = match_list->capacity * 2;
                Stru_CN_MatchResult_t* new_matches = (Stru_CN_MatchResult_t*)cn_realloc(
                    match_list->matches, sizeof(Stru_CN_MatchResult_t) * new_capacity);
                if (!new_matches)
                {
                    CN_string_free_match_list(match_list);
                    return NULL;
                }
                match_list->matches = new_matches;
                match_list->capacity = new_capacity;
            }
            
            // 添加匹配结果
            match_list->matches[match_list->count].position = found_pos;
            match_list->matches[match_list->count].length = pattern->length;
            match_list->matches[match_list->count].match_index = match_index;
            match_list->count++;
            
            // 移动到下一个位置
            position = found_pos + 1;
            match_index++;
        }
    }
    
    return match_list;
}

void CN_string_free_multi_pattern_matcher(Stru_CN_MultiPatternMatcher_t* matcher)
{
    if (matcher)
    {
        // 注意：我们只释放匹配器结构体，不释放模式数组
        // 因为模式数组由调用者管理
        cn_free(matcher);
    }
}
