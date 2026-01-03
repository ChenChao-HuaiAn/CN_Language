/**
 * @file CN_string_regex.h
 * @brief 正则表达式模块
 * 
 * 提供正则表达式匹配功能，支持：
 * - 基本正则表达式语法
 * - Unicode字符类
 * - 分组和捕获
 * - 量词：*, +, ?, {n,m}
 * - 锚点：^, $
 * - 字符类：[...], [^...]
 * 
 * @version 1.0.0
 * @date 2026-01-03
 * @license MIT
 * @author CN_Language开发团队
 */

#ifndef CN_STRING_REGEX_H
#define CN_STRING_REGEX_H

#include <stdbool.h>
#include <stddef.h>
#include "CN_string_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 正则表达式编译选项
 */
enum Eum_CN_RegexCompileFlag_t
{
    Eum_REGEX_CASELESS      = 1 << 0,   /**< 不区分大小写 */
    Eum_REGEX_MULTILINE     = 1 << 1,   /**< 多行模式 */
    Eum_REGEX_DOTALL        = 1 << 2,   /**< .匹配所有字符包括换行符 */
    Eum_REGEX_EXTENDED      = 1 << 3,   /**< 扩展语法 */
    Eum_REGEX_ANCHORED      = 1 << 4,   /**< 锚定到字符串开始 */
    Eum_REGEX_DOLLAR_ENDONLY = 1 << 5,  /**< $只匹配字符串结尾 */
    Eum_REGEX_UNICODE       = 1 << 6,   /**< Unicode模式 */
    Eum_REGEX_NO_AUTO_CAPTURE = 1 << 7  /**< 禁用自动捕获 */
};

typedef enum Eum_CN_RegexCompileFlag_t Eum_CN_RegexCompileFlag_t;

/**
 * @brief 正则表达式执行选项
 */
enum Eum_CN_RegexExecFlag_t
{
    Eum_REGEX_NOTBOL        = 1 << 0,   /**< 字符串开始不是行开始 */
    Eum_REGEX_NOTEOL        = 1 << 1,   /**< 字符串结尾不是行结尾 */
    Eum_REGEX_NOTEMPTY      = 1 << 2,   /**< 空字符串不是有效匹配 */
    Eum_REGEX_PARTIAL       = 1 << 3    /**< 部分匹配 */
};

typedef enum Eum_CN_RegexExecFlag_t Eum_CN_RegexExecFlag_t;

/**
 * @brief 正则表达式错误代码
 */
enum Eum_CN_RegexError_t
{
    Eum_REGEX_ERROR_NOERROR = 0,        /**< 无错误 */
    Eum_REGEX_ERROR_NOMEM,              /**< 内存不足 */
    Eum_REGEX_ERROR_BADPAT,             /**< 模式语法错误 */
    Eum_REGEX_ERROR_BADUTF8,            /**< 无效的UTF-8序列 */
    Eum_REGEX_ERROR_BADOPTION,          /**< 无效选项 */
    Eum_REGEX_ERROR_BADMAGIC,           /**< 魔数错误 */
    Eum_REGEX_ERROR_UNKNOWN_NODE,       /**< 未知节点类型 */
    Eum_REGEX_ERROR_NOMATCH,            /**< 无匹配 */
    Eum_REGEX_ERROR_PARTIAL,            /**< 部分匹配 */
    Eum_REGEX_ERROR_BADPARTIAL,         /**< 无效的部分匹配 */
    Eum_REGEX_ERROR_INTERNAL,           /**< 内部错误 */
    Eum_REGEX_ERROR_BADCOUNT,           /**< 量词计数错误 */
    Eum_REGEX_ERROR_DFA_UITEM,          /**< DFA项错误 */
    Eum_REGEX_ERROR_DFA_UCOND,          /**< DFA条件错误 */
    Eum_REGEX_ERROR_DFA_UMLIMIT,        /**< DFA匹配限制 */
    Eum_REGEX_ERROR_DFA_WSSIZE,         /**< DFA工作空间大小 */
    Eum_REGEX_ERROR_DFA_RECURSE,        /**< DFA递归 */
    Eum_REGEX_ERROR_RECURSIONLIMIT,     /**< 递归限制 */
    Eum_REGEX_ERROR_NULL,               /**< 空参数 */
    Eum_REGEX_ERROR_BADOFFSET,          /**< 偏移量错误 */
    Eum_REGEX_ERROR_BADREPLACEMENT,     /**< 替换字符串错误 */
    Eum_REGEX_ERROR_BADUTFOFFSET,       /**< UTF-8偏移量错误 */
    Eum_REGEX_ERROR_CALLOUT,            /**< 调用错误 */
    Eum_REGEX_ERROR_BADREFERENCE        /**< 引用错误 */
};

typedef enum Eum_CN_RegexError_t Eum_CN_RegexError_t;

/**
 * @brief 正则表达式匹配结果
 */
struct Stru_CN_RegexMatch_t
{
    size_t start;                       /**< 匹配开始位置 */
    size_t end;                         /**< 匹配结束位置（下一个字符位置） */
    size_t group_index;                 /**< 分组索引（0表示整个匹配） */
};

typedef struct Stru_CN_RegexMatch_t Stru_CN_RegexMatch_t;

/**
 * @brief 正则表达式匹配结果集
 */
struct Stru_CN_RegexMatchSet_t
{
    Stru_CN_RegexMatch_t* matches;      /**< 匹配结果数组 */
    size_t count;                       /**< 匹配数量 */
    size_t capacity;                    /**< 数组容量 */
    size_t group_count;                 /**< 分组数量 */
};

typedef struct Stru_CN_RegexMatchSet_t Stru_CN_RegexMatchSet_t;

/**
 * @brief 正则表达式对象（不透明类型）
 */
typedef struct Stru_CN_Regex_t Stru_CN_Regex_t;

/* ==================== 正则表达式编译和执行 ==================== */

/**
 * @brief 编译正则表达式
 * 
 * @param pattern 正则表达式模式
 * @param flags 编译标志
 * @param error_code 错误代码输出参数
 * @param error_offset 错误位置输出参数
 * @return 编译后的正则表达式对象，失败返回NULL
 */
Stru_CN_Regex_t* CN_regex_compile(
    const Stru_CN_String_t* pattern,
    int flags,
    Eum_CN_RegexError_t* error_code,
    size_t* error_offset);

/**
 * @brief 编译正则表达式（C字符串版本）
 * 
 * @param pattern 正则表达式模式（C字符串）
 * @param flags 编译标志
 * @param error_code 错误代码输出参数
 * @param error_offset 错误位置输出参数
 * @return 编译后的正则表达式对象，失败返回NULL
 */
Stru_CN_Regex_t* CN_regex_compile_cstr(
    const char* pattern,
    int flags,
    Eum_CN_RegexError_t* error_code,
    size_t* error_offset);

/**
 * @brief 执行正则表达式匹配
 * 
 * @param regex 编译后的正则表达式
 * @param subject 要匹配的字符串
 * @param start_offset 开始偏移量
 * @param options 执行选项
 * @param match_set 匹配结果集输出参数
 * @return 匹配成功返回true，否则返回false
 */
bool CN_regex_exec(
    const Stru_CN_Regex_t* regex,
    const Stru_CN_String_t* subject,
    size_t start_offset,
    int options,
    Stru_CN_RegexMatchSet_t* match_set);

/**
 * @brief 释放正则表达式对象
 * 
 * @param regex 要释放的正则表达式对象
 */
void CN_regex_free(Stru_CN_Regex_t* regex);

/* ==================== 匹配结果管理 ==================== */

/**
 * @brief 创建匹配结果集
 * 
 * @param initial_capacity 初始容量
 * @return 匹配结果集，失败返回NULL
 */
Stru_CN_RegexMatchSet_t* CN_regex_match_set_create(size_t initial_capacity);

/**
 * @brief 释放匹配结果集
 * 
 * @param match_set 要释放的匹配结果集
 */
void CN_regex_match_set_free(Stru_CN_RegexMatchSet_t* match_set);

/**
 * @brief 清空匹配结果集
 * 
 * @param match_set 要清空的匹配结果集
 */
void CN_regex_match_set_clear(Stru_CN_RegexMatchSet_t* match_set);

/**
 * @brief 获取匹配的字符串
 * 
 * @param subject 原始字符串
 * @param match 匹配结果
 * @return 匹配的字符串，需要调用CN_string_destroy释放
 */
Stru_CN_String_t* CN_regex_get_match_string(
    const Stru_CN_String_t* subject,
    const Stru_CN_RegexMatch_t* match);

/* ==================== 工具函数 ==================== */

/**
 * @brief 检查字符串是否匹配正则表达式
 * 
 * @param pattern 正则表达式模式
 * @param subject 要检查的字符串
 * @param flags 编译标志
 * @return 匹配返回true，否则返回false
 */
bool CN_regex_match(
    const Stru_CN_String_t* pattern,
    const Stru_CN_String_t* subject,
    int flags);

/**
 * @brief 查找所有匹配
 * 
 * @param pattern 正则表达式模式
 * @param subject 要搜索的字符串
 * @param flags 编译标志
 * @return 匹配结果集，需要调用CN_regex_match_set_free释放
 */
Stru_CN_RegexMatchSet_t* CN_regex_find_all(
    const Stru_CN_String_t* pattern,
    const Stru_CN_String_t* subject,
    int flags);

/**
 * @brief 替换匹配的字符串
 * 
 * @param pattern 正则表达式模式
 * @param subject 原始字符串
 * @param replacement 替换字符串
 * @param flags 编译标志
 * @return 替换后的字符串，需要调用CN_string_destroy释放
 */
Stru_CN_String_t* CN_regex_replace(
    const Stru_CN_String_t* pattern,
    const Stru_CN_String_t* subject,
    const Stru_CN_String_t* replacement,
    int flags);

/**
 * @brief 分割字符串
 * 
 * @param pattern 正则表达式模式
 * @param subject 要分割的字符串
 * @param flags 编译标志
 * @param part_count 部分数量输出参数
 * @return 分割后的字符串数组，需要调用CN_regex_free_split_result释放
 */
Stru_CN_String_t** CN_regex_split(
    const Stru_CN_String_t* pattern,
    const Stru_CN_String_t* subject,
    int flags,
    size_t* part_count);

/**
 * @brief 释放分割结果
 * 
 * @param parts 分割结果数组
 * @param part_count 部分数量
 */
void CN_regex_free_split_result(
    Stru_CN_String_t** parts,
    size_t part_count);

/**
 * @brief 获取错误消息
 * 
 * @param error_code 错误代码
 * @return 错误消息字符串
 */
const char* CN_regex_get_error_message(Eum_CN_RegexError_t error_code);

/**
 * @brief 转义正则表达式特殊字符
 * 
 * @param str 要转义的字符串
 * @return 转义后的字符串，需要调用CN_string_destroy释放
 */
Stru_CN_String_t* CN_regex_escape(const Stru_CN_String_t* str);

#ifdef __cplusplus
}
#endif

#endif /* CN_STRING_REGEX_H */
