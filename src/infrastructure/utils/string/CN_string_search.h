/**
 * @file CN_string_search.h
 * @brief 字符串搜索算法模块
 * 
 * 提供优化的字符串搜索算法，包括：
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

#ifndef CN_STRING_SEARCH_H
#define CN_STRING_SEARCH_H

#include <stdbool.h>
#include <stddef.h>
#include "CN_string_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 搜索算法类型枚举
 */
enum Eum_CN_SearchAlgorithm_t
{
    Eum_SEARCH_ALGORITHM_BOYER_MOORE = 0,   /**< Boyer-Moore算法（默认） */
    Eum_SEARCH_ALGORITHM_KMP,               /**< Knuth-Morris-Pratt算法 */
    Eum_SEARCH_ALGORITHM_RABIN_KARP,        /**< Rabin-Karp算法 */
    Eum_SEARCH_ALGORITHM_SIMPLE,            /**< 简单线性搜索（向后兼容） */
    Eum_SEARCH_ALGORITHM_AUTO               /**< 自动选择最佳算法 */
};

typedef enum Eum_CN_SearchAlgorithm_t Eum_CN_SearchAlgorithm_t;

/**
 * @brief 搜索选项结构体
 */
struct Stru_CN_SearchOptions_t
{
    Eum_CN_SearchAlgorithm_t algorithm;     /**< 搜索算法 */
    bool case_sensitive;                    /**< 是否区分大小写 */
    bool backward;                          /**< 是否向后搜索 */
    size_t start_position;                  /**< 起始位置 */
    size_t max_matches;                     /**< 最大匹配数（0表示无限制） */
};

typedef struct Stru_CN_SearchOptions_t Stru_CN_SearchOptions_t;

/**
 * @brief 匹配结果结构体
 */
struct Stru_CN_MatchResult_t
{
    size_t position;                        /**< 匹配位置 */
    size_t length;                          /**< 匹配长度 */
    size_t match_index;                     /**< 匹配索引（从0开始） */
};

typedef struct Stru_CN_MatchResult_t Stru_CN_MatchResult_t;

/**
 * @brief 匹配结果列表结构体
 */
struct Stru_CN_MatchList_t
{
    Stru_CN_MatchResult_t* matches;         /**< 匹配结果数组 */
    size_t count;                           /**< 匹配数量 */
    size_t capacity;                        /**< 数组容量 */
};

typedef struct Stru_CN_MatchList_t Stru_CN_MatchList_t;

/* ==================== 基础搜索函数 ==================== */

/**
 * @brief 查找子字符串（使用指定算法）
 * 
 * @param haystack 要搜索的字符串
 * @param needle 要查找的子字符串
 * @param options 搜索选项
 * @return 匹配位置，如果未找到返回SIZE_MAX
 */
size_t CN_string_find_with_algorithm(
    const Stru_CN_String_t* haystack,
    const Stru_CN_String_t* needle,
    const Stru_CN_SearchOptions_t* options);

/**
 * @brief 查找子字符串（C字符串版本）
 * 
 * @param haystack 要搜索的字符串
 * @param needle 要查找的子字符串（C字符串）
 * @param options 搜索选项
 * @return 匹配位置，如果未找到返回SIZE_MAX
 */
size_t CN_string_find_cstr_with_algorithm(
    const Stru_CN_String_t* haystack,
    const char* needle,
    const Stru_CN_SearchOptions_t* options);

/**
 * @brief 查找所有匹配
 * 
 * @param haystack 要搜索的字符串
 * @param needle 要查找的子字符串
 * @param options 搜索选项
 * @return 匹配结果列表，使用后需要调用CN_string_free_match_list释放
 */
Stru_CN_MatchList_t* CN_string_find_all(
    const Stru_CN_String_t* haystack,
    const Stru_CN_String_t* needle,
    const Stru_CN_SearchOptions_t* options);

/**
 * @brief 释放匹配结果列表
 * 
 * @param match_list 要释放的匹配结果列表
 */
void CN_string_free_match_list(Stru_CN_MatchList_t* match_list);

/* ==================== 特定算法函数 ==================== */

/**
 * @brief 使用Boyer-Moore算法查找子字符串
 * 
 * @param haystack 要搜索的字符串
 * @param needle 要查找的子字符串
 * @param case_sensitive 是否区分大小写
 * @param start_position 起始位置
 * @return 匹配位置，如果未找到返回SIZE_MAX
 */
size_t CN_string_find_boyer_moore(
    const Stru_CN_String_t* haystack,
    const Stru_CN_String_t* needle,
    bool case_sensitive,
    size_t start_position);

/**
 * @brief 使用KMP算法查找子字符串
 * 
 * @param haystack 要搜索的字符串
 * @param needle 要查找的子字符串
 * @param case_sensitive 是否区分大小写
 * @param start_position 起始位置
 * @return 匹配位置，如果未找到返回SIZE_MAX
 */
size_t CN_string_find_kmp(
    const Stru_CN_String_t* haystack,
    const Stru_CN_String_t* needle,
    bool case_sensitive,
    size_t start_position);

/**
 * @brief 使用Rabin-Karp算法查找子字符串
 * 
 * @param haystack 要搜索的字符串
 * @param needle 要查找的子字符串
 * @param case_sensitive 是否区分大小写
 * @param start_position 起始位置
 * @return 匹配位置，如果未找到返回SIZE_MAX
 */
size_t CN_string_find_rabin_karp(
    const Stru_CN_String_t* haystack,
    const Stru_CN_String_t* needle,
    bool case_sensitive,
    size_t start_position);

/* ==================== 多模式匹配 ==================== */

/**
 * @brief 多模式匹配结构体
 */
struct Stru_CN_MultiPatternMatcher_t
{
    void* internal_data;                    /**< 内部数据（不透明） */
    size_t pattern_count;                   /**< 模式数量 */
};

typedef struct Stru_CN_MultiPatternMatcher_t Stru_CN_MultiPatternMatcher_t;

/**
 * @brief 创建多模式匹配器
 * 
 * @param patterns 模式字符串数组
 * @param pattern_count 模式数量
 * @param case_sensitive 是否区分大小写
 * @return 多模式匹配器，使用后需要调用CN_string_free_multi_pattern_matcher释放
 */
Stru_CN_MultiPatternMatcher_t* CN_string_create_multi_pattern_matcher(
    const Stru_CN_String_t** patterns,
    size_t pattern_count,
    bool case_sensitive);

/**
 * @brief 使用多模式匹配器搜索
 * 
 * @param matcher 多模式匹配器
 * @param text 要搜索的文本
 * @param start_position 起始位置
 * @return 匹配结果列表，使用后需要调用CN_string_free_match_list释放
 */
Stru_CN_MatchList_t* CN_string_search_multi_pattern(
    const Stru_CN_MultiPatternMatcher_t* matcher,
    const Stru_CN_String_t* text,
    size_t start_position);

/**
 * @brief 释放多模式匹配器
 * 
 * @param matcher 要释放的多模式匹配器
 */
void CN_string_free_multi_pattern_matcher(Stru_CN_MultiPatternMatcher_t* matcher);

/* ==================== 工具函数 ==================== */

/**
 * @brief 获取默认搜索选项
 * 
 * @return 默认搜索选项
 */
Stru_CN_SearchOptions_t CN_string_get_default_search_options(void);

/**
 * @brief 获取算法名称
 * 
 * @param algorithm 算法类型
 * @return 算法名称字符串
 */
const char* CN_string_get_algorithm_name(Eum_CN_SearchAlgorithm_t algorithm);

/**
 * @brief 检查算法是否支持区分大小写
 * 
 * @param algorithm 算法类型
 * @return 是否支持区分大小写
 */
bool CN_string_algorithm_supports_case_sensitive(Eum_CN_SearchAlgorithm_t algorithm);

/**
 * @brief 获取推荐算法（基于模式长度和文本长度）
 * 
 * @param pattern_length 模式长度
 * @param text_length 文本长度
 * @return 推荐算法
 */
Eum_CN_SearchAlgorithm_t CN_string_get_recommended_algorithm(
    size_t pattern_length,
    size_t text_length);

#ifdef __cplusplus
}
#endif

#endif /* CN_STRING_SEARCH_H */
