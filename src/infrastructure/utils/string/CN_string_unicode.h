/******************************************************************************
 * 文件名: CN_string_unicode.h
 * 功能: CN_Language字符串Unicode处理模块
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现Unicode大小写转换
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_STRING_UNICODE_H
#define CN_STRING_UNICODE_H

#include "CN_string_core.h"
#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// Unicode字符处理
// ============================================================================

/**
 * @brief Unicode字符信息
 */
typedef struct Stru_CN_UnicodeCharInfo_t
{
    uint32_t code_point;                /**< Unicode码点 */
    size_t utf8_length;                 /**< UTF-8编码长度（字节） */
    char utf8_bytes[6];                 /**< UTF-8编码字节 */
    bool is_letter;                     /**< 是否是字母 */
    bool is_uppercase;                  /**< 是否是大写字母 */
    bool is_lowercase;                  /**< 是否是小写字母 */
    bool is_titlecase;                  /**< 是否是标题大小写字母 */
    uint32_t uppercase_mapping;         /**< 大写映射码点 */
    uint32_t lowercase_mapping;         /**< 小写映射码点 */
    uint32_t titlecase_mapping;         /**< 标题大小写映射码点 */
} Stru_CN_UnicodeCharInfo_t;

/**
 * @brief Unicode字符类别
 */
typedef enum Eum_CN_UnicodeCategory_t
{
    Eum_UNICODE_CATEGORY_UPPERCASE_LETTER = 0,     /**< 大写字母 */
    Eum_UNICODE_CATEGORY_LOWERCASE_LETTER = 1,     /**< 小写字母 */
    Eum_UNICODE_CATEGORY_TITLECASE_LETTER = 2,     /**< 标题大小写字母 */
    Eum_UNICODE_CATEGORY_MODIFIER_LETTER = 3,      /**< 修饰字母 */
    Eum_UNICODE_CATEGORY_OTHER_LETTER = 4,         /**< 其他字母 */
    Eum_UNICODE_CATEGORY_NON_SPACING_MARK = 5,     /**< 非空格标记 */
    Eum_UNICODE_CATEGORY_SPACING_MARK = 6,         /**< 空格标记 */
    Eum_UNICODE_CATEGORY_ENCLOSING_MARK = 7,       /**< 包围标记 */
    Eum_UNICODE_CATEGORY_DECIMAL_NUMBER = 8,       /**< 十进制数字 */
    Eum_UNICODE_CATEGORY_LETTER_NUMBER = 9,        /**< 字母数字 */
    Eum_UNICODE_CATEGORY_OTHER_NUMBER = 10,        /**< 其他数字 */
    Eum_UNICODE_CATEGORY_SPACE_SEPARATOR = 11,     /**< 空格分隔符 */
    Eum_UNICODE_CATEGORY_LINE_SEPARATOR = 12,      /**< 行分隔符 */
    Eum_UNICODE_CATEGORY_PARAGRAPH_SEPARATOR = 13, /**< 段落分隔符 */
    Eum_UNICODE_CATEGORY_CONTROL = 14,             /**< 控制字符 */
    Eum_UNICODE_CATEGORY_FORMAT = 15,              /**< 格式字符 */
    Eum_UNICODE_CATEGORY_PRIVATE_USE = 16,         /**< 私有使用字符 */
    Eum_UNICODE_CATEGORY_SURROGATE = 17,           /**< 代理字符 */
    Eum_UNICODE_CATEGORY_DASH_PUNCTUATION = 18,    /**< 破折号标点 */
    Eum_UNICODE_CATEGORY_OPEN_PUNCTUATION = 19,    /**< 开标点 */
    Eum_UNICODE_CATEGORY_CLOSE_PUNCTUATION = 20,   /**< 闭标点 */
    Eum_UNICODE_CATEGORY_CONNECTOR_PUNCTUATION = 21, /**< 连接标点 */
    Eum_UNICODE_CATEGORY_OTHER_PUNCTUATION = 22,   /**< 其他标点 */
    Eum_UNICODE_CATEGORY_MATH_SYMBOL = 23,         /**< 数学符号 */
    Eum_UNICODE_CATEGORY_CURRENCY_SYMBOL = 24,     /**< 货币符号 */
    Eum_UNICODE_CATEGORY_MODIFIER_SYMBOL = 25,     /**< 修饰符号 */
    Eum_UNICODE_CATEGORY_OTHER_SYMBOL = 26,        /**< 其他符号 */
    Eum_UNICODE_CATEGORY_INITIAL_PUNCTUATION = 27, /**< 初始标点 */
    Eum_UNICODE_CATEGORY_FINAL_PUNCTUATION = 28    /**< 最终标点 */
} Eum_CN_UnicodeCategory_t;

// ============================================================================
// Unicode字符操作
// ============================================================================

/**
 * @brief 获取UTF-8字符的Unicode码点
 * 
 * @param utf8_str UTF-8字符串
 * @param str_len 字符串长度（字节）
 * @param pos 当前位置（字节偏移）
 * @param next_pos 输出参数，下一个字符的位置
 * @return Unicode码点，无效返回0
 */
uint32_t CN_unicode_get_code_point(const char* utf8_str, size_t str_len, 
                                   size_t pos, size_t* next_pos);

/**
 * @brief 将Unicode码点编码为UTF-8
 * 
 * @param code_point Unicode码点
 * @param buffer 输出缓冲区（至少6字节）
 * @return UTF-8编码长度（字节）
 */
size_t CN_unicode_encode_utf8(uint32_t code_point, char* buffer);

/**
 * @brief 获取Unicode字符信息
 * 
 * @param code_point Unicode码点
 * @param info 输出参数，字符信息
 * @return 获取成功返回true，失败返回false
 */
bool CN_unicode_get_char_info(uint32_t code_point, Stru_CN_UnicodeCharInfo_t* info);

/**
 * @brief 获取Unicode字符类别
 * 
 * @param code_point Unicode码点
 * @return 字符类别
 */
Eum_CN_UnicodeCategory_t CN_unicode_get_category(uint32_t code_point);

// ============================================================================
// Unicode大小写转换
// ============================================================================

/**
 * @brief 转换为大写（Unicode支持）
 * 
 * @param str 字符串
 * @return 转换成功返回true，失败返回false
 */
bool CN_string_to_upper_unicode(Stru_CN_String_t* str);

/**
 * @brief 转换为小写（Unicode支持）
 * 
 * @param str 字符串
 * @return 转换成功返回true，失败返回false
 */
bool CN_string_to_lower_unicode(Stru_CN_String_t* str);

/**
 * @brief 转换为标题大小写（Unicode支持）
 * 
 * @param str 字符串
 * @return 转换成功返回true，失败返回false
 */
bool CN_string_to_title_unicode(Stru_CN_String_t* str);

/**
 * @brief 转换为大写（Unicode支持）并返回新字符串
 * 
 * @param str 源字符串
 * @return 新创建的大写字符串，失败返回NULL
 */
Stru_CN_String_t* CN_string_to_upper_copy(const Stru_CN_String_t* str);

/**
 * @brief 转换为小写（Unicode支持）并返回新字符串
 * 
 * @param str 源字符串
 * @return 新创建的小写字符串，失败返回NULL
 */
Stru_CN_String_t* CN_string_to_lower_copy(const Stru_CN_String_t* str);

/**
 * @brief 转换为标题大小写（Unicode支持）并返回新字符串
 * 
 * @param str 源字符串
 * @return 新创建的标题大小写字符串，失败返回NULL
 */
Stru_CN_String_t* CN_string_to_title_copy(const Stru_CN_String_t* str);

// ============================================================================
// Unicode字符串比较
// ============================================================================

/**
 * @brief Unicode不区分大小写比较
 * 
 * @param str1 第一个字符串
 * @param str2 第二个字符串
 * @return 比较结果：<0表示str1<str2，0表示相等，>0表示str1>str2
 */
int CN_string_compare_case_insensitive_unicode(const Stru_CN_String_t* str1,
                                               const Stru_CN_String_t* str2);

/**
 * @brief Unicode不区分大小写检查字符串是否相等
 * 
 * @param str1 第一个字符串
 * @param str2 第二个字符串
 * @return 如果字符串相等返回true，否则返回false
 */
bool CN_string_equal_case_insensitive_unicode(const Stru_CN_String_t* str1,
                                              const Stru_CN_String_t* str2);

// ============================================================================
// Unicode规范化
// ============================================================================

/**
 * @brief Unicode规范化形式
 */
typedef enum Eum_CN_UnicodeNormalizationForm_t
{
    Eum_UNICODE_NFD = 0,   /**< 规范分解 */
    Eum_UNICODE_NFC = 1,   /**< 规范分解后规范组合 */
    Eum_UNICODE_NFKD = 2,  /**< 兼容分解 */
    Eum_UNICODE_NFKC = 3   /**< 兼容分解后规范组合 */
} Eum_CN_UnicodeNormalizationForm_t;

/**
 * @brief 对字符串进行Unicode规范化
 * 
 * @param str 字符串
 * @param form 规范化形式
 * @return 规范化成功返回true，失败返回false
 */
bool CN_string_normalize_unicode(Stru_CN_String_t* str,
                                 Eum_CN_UnicodeNormalizationForm_t form);

/**
 * @brief 对字符串进行Unicode规范化并返回新字符串
 * 
 * @param str 源字符串
 * @param form 规范化形式
 * @return 新创建的规范化字符串，失败返回NULL
 */
Stru_CN_String_t* CN_string_normalize_copy(const Stru_CN_String_t* str,
                                           Eum_CN_UnicodeNormalizationForm_t form);

#endif // CN_STRING_UNICODE_H
