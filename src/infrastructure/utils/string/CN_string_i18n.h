/******************************************************************************
 * 文件名: CN_string_i18n.h
 * 功能: CN_Language字符串国际化模块 - 国际化支持
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现国际化支持
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_STRING_I18N_H
#define CN_STRING_I18N_H

#include <stddef.h>
#include <stdbool.h>
#include <time.h>

// ============================================================================
// 前置声明
// ============================================================================

typedef struct Stru_CN_String_t Stru_CN_String_t;

// ============================================================================
// 区域设置定义
// ============================================================================

/**
 * @brief 区域设置结构体（不透明类型）
 */
typedef struct Stru_CN_Locale_t Stru_CN_Locale_t;

/**
 * @brief 区域设置类别
 */
typedef enum Eum_CN_LocaleCategory_t
{
    Eum_LOCALE_CATEGORY_ALL = 0,           /**< 所有类别 */
    Eum_LOCALE_CATEGORY_COLLATE = 1,       /**< 字符串排序规则 */
    Eum_LOCALE_CATEGORY_CTYPE = 2,         /**< 字符分类 */
    Eum_LOCALE_CATEGORY_MONETARY = 3,      /**< 货币格式化 */
    Eum_LOCALE_CATEGORY_NUMERIC = 4,       /**< 数字格式化 */
    Eum_LOCALE_CATEGORY_TIME = 5,          /**< 时间格式化 */
    Eum_LOCALE_CATEGORY_MESSAGES = 6       /**< 消息本地化 */
} Eum_CN_LocaleCategory_t;

/**
 * @brief 区域设置信息结构体
 */
typedef struct Stru_CN_LocaleInfo_t
{
    const char* locale_id;          /**< 区域设置ID（如"zh_CN"） */
    const char* language_code;      /**< 语言代码（如"zh"） */
    const char* country_code;       /**< 国家代码（如"CN"） */
    const char* encoding;           /**< 默认编码（如"UTF-8"） */
    const char* language_name;      /**< 语言名称（如"中文"） */
    const char* country_name;       /**< 国家名称（如"中国"） */
} Stru_CN_LocaleInfo_t;

// ============================================================================
// Unicode规范化形式
// ============================================================================

/**
 * @brief Unicode规范化形式
 */
typedef enum Eum_CN_NormalizationForm_t
{
    Eum_NORMALIZATION_FORM_NFD = 0,     /**< 规范分解 */
    Eum_NORMALIZATION_FORM_NFC = 1,     /**< 规范分解后规范组合 */
    Eum_NORMALIZATION_FORM_NFKD = 2,    /**< 兼容分解 */
    Eum_NORMALIZATION_FORM_NFKC = 3     /**< 兼容分解后规范组合 */
} Eum_CN_NormalizationForm_t;

// ============================================================================
// 格式化选项
// ============================================================================

/**
 * @brief 日期格式化选项
 */
typedef struct Stru_CN_DateFormatOptions_t
{
    const char* date_format;        /**< 日期格式字符串 */
    bool use_short_format;          /**< 是否使用短格式 */
    bool include_weekday;           /**< 是否包含星期 */
    bool include_era;               /**< 是否包含纪元 */
} Stru_CN_DateFormatOptions_t;

/**
 * @brief 时间格式化选项
 */
typedef struct Stru_CN_TimeFormatOptions_t
{
    const char* time_format;        /**< 时间格式字符串 */
    bool use_24_hour;               /**< 是否使用24小时制 */
    bool include_seconds;           /**< 是否包含秒 */
    bool include_timezone;          /**< 是否包含时区 */
} Stru_CN_TimeFormatOptions_t;

/**
 * @brief 数字格式化选项
 */
typedef struct Stru_CN_NumberFormatOptions_t
{
    int min_integer_digits;         /**< 最小整数位数 */
    int max_fraction_digits;        /**< 最大小数位数 */
    int min_fraction_digits;        /**< 最小小数位数 */
    bool use_grouping_separator;    /**< 是否使用分组分隔符 */
    const char* decimal_separator;  /**< 小数分隔符 */
    const char* grouping_separator; /**< 分组分隔符 */
    const char* currency_symbol;    /**< 货币符号 */
} Stru_CN_NumberFormatOptions_t;

// ============================================================================
// 区域设置管理
// ============================================================================

/**
 * @brief 获取系统默认区域设置
 * 
 * @param category 区域设置类别
 * @return 区域设置对象，失败返回NULL
 */
Stru_CN_Locale_t* CN_i18n_get_default_locale(Eum_CN_LocaleCategory_t category);

/**
 * @brief 创建区域设置
 * 
 * @param locale_id 区域设置ID（如"zh_CN.UTF-8"）
 * @return 区域设置对象，失败返回NULL
 */
Stru_CN_Locale_t* CN_i18n_create_locale(const char* locale_id);

/**
 * @brief 复制区域设置
 * 
 * @param src 源区域设置
 * @return 新创建的区域设置副本，失败返回NULL
 */
Stru_CN_Locale_t* CN_i18n_copy_locale(const Stru_CN_Locale_t* src);

/**
 * @brief 销毁区域设置
 * 
 * @param locale 要销毁的区域设置
 */
void CN_i18n_destroy_locale(Stru_CN_Locale_t* locale);

/**
 * @brief 获取区域设置信息
 * 
 * @param locale 区域设置
 * @param info 输出参数，接收区域设置信息
 * @return 获取成功返回true，失败返回false
 */
bool CN_i18n_get_locale_info(const Stru_CN_Locale_t* locale,
                             Stru_CN_LocaleInfo_t* info);

/**
 * @brief 设置当前线程的区域设置
 * 
 * @param locale 区域设置
 * @param category 区域设置类别
 * @return 设置成功返回true，失败返回false
 */
bool CN_i18n_set_thread_locale(Stru_CN_Locale_t* locale,
                               Eum_CN_LocaleCategory_t category);

/**
 * @brief 获取可用区域设置列表
 * 
 * @param count 输出参数，接收区域设置数量
 * @return 区域设置信息数组，调用者负责释放，失败返回NULL
 */
Stru_CN_LocaleInfo_t* CN_i18n_get_available_locales(size_t* count);

// ============================================================================
// Unicode规范化
// ============================================================================

/**
 * @brief 对字符串进行Unicode规范化
 * 
 * @param str 字符串
 * @param form 规范化形式
 * @return 规范化成功返回true，失败返回false
 */
bool CN_i18n_normalize_string(Stru_CN_String_t* str,
                              Eum_CN_NormalizationForm_t form);

/**
 * @brief 创建规范化后的字符串副本
 * 
 * @param src 源字符串
 * @param form 规范化形式
 * @return 新创建的规范化字符串，失败返回NULL
 */
Stru_CN_String_t* CN_i18n_create_normalized_string(
    const Stru_CN_String_t* src,
    Eum_CN_NormalizationForm_t form);

/**
 * @brief 检查字符串是否已规范化
 * 
 * @param str 字符串
 * @param form 规范化形式
 * @return 如果已规范化返回true，否则返回false
 */
bool CN_i18n_is_normalized(const Stru_CN_String_t* str,
                           Eum_CN_NormalizationForm_t form);

// ============================================================================
// 字符串比较和排序
// ============================================================================

/**
 * @brief 基于区域设置的字符串比较
 * 
 * @param str1 第一个字符串
 * @param str2 第二个字符串
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @return 比较结果：<0表示str1<str2，0表示相等，>0表示str1>str2
 */
int CN_i18n_compare_strings(const Stru_CN_String_t* str1,
                            const Stru_CN_String_t* str2,
                            const Stru_CN_Locale_t* locale);

/**
 * @brief 基于区域设置的不区分大小写字符串比较
 * 
 * @param str1 第一个字符串
 * @param str2 第二个字符串
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @return 比较结果：<0表示str1<str2，0表示相等，>0表示str1>str2
 */
int CN_i18n_compare_strings_case_insensitive(const Stru_CN_String_t* str1,
                                             const Stru_CN_String_t* str2,
                                             const Stru_CN_Locale_t* locale);

/**
 * @brief 对字符串数组进行基于区域设置的排序
 * 
 * @param strings 字符串数组
 * @param count 字符串数量
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @param case_sensitive 是否区分大小写
 * @return 排序成功返回true，失败返回false
 */
bool CN_i18n_sort_strings(Stru_CN_String_t** strings, size_t count,
                          const Stru_CN_Locale_t* locale,
                          bool case_sensitive);

// ============================================================================
// 字符分类
// ============================================================================

/**
 * @brief 检查字符是否为字母（基于区域设置）
 * 
 * @param ch 字符（UTF-8编码）
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @return 如果是字母返回true，否则返回false
 */
bool CN_i18n_is_alpha(const char* ch, const Stru_CN_Locale_t* locale);

/**
 * @brief 检查字符是否为数字（基于区域设置）
 * 
 * @param ch 字符（UTF-8编码）
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @return 如果是数字返回true，否则返回false
 */
bool CN_i18n_is_digit(const char* ch, const Stru_CN_Locale_t* locale);

/**
 * @brief 检查字符是否为字母或数字（基于区域设置）
 * 
 * @param ch 字符（UTF-8编码）
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @return 如果是字母或数字返回true，否则返回false
 */
bool CN_i18n_is_alnum(const char* ch, const Stru_CN_Locale_t* locale);

/**
 * @brief 检查字符是否为空白字符（基于区域设置）
 * 
 * @param ch 字符（UTF-8编码）
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @return 如果是空白字符返回true，否则返回false
 */
bool CN_i18n_is_space(const char* ch, const Stru_CN_Locale_t* locale);

/**
 * @brief 检查字符是否为标点符号（基于区域设置）
 * 
 * @param ch 字符（UTF-8编码）
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @return 如果是标点符号返回true，否则返回false
 */
bool CN_i18n_is_punct(const char* ch, const Stru_CN_Locale_t* locale);

// ============================================================================
// 大小写转换（基于区域设置）
// ============================================================================

/**
 * @brief 将字符串转换为大写（基于区域设置）
 * 
 * @param str 字符串
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @return 转换成功返回true，失败返回false
 */
bool CN_i18n_to_upper(Stru_CN_String_t* str, const Stru_CN_Locale_t* locale);

/**
 * @brief 将字符串转换为小写（基于区域设置）
 * 
 * @param str 字符串
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @return 转换成功返回true，失败返回false
 */
bool CN_i18n_to_lower(Stru_CN_String_t* str, const Stru_CN_Locale_t* locale);

/**
 * @brief 将字符串转换为标题大小写（基于区域设置）
 * 
 * @param str 字符串
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @return 转换成功返回true，失败返回false
 */
bool CN_i18n_to_title(Stru_CN_String_t* str, const Stru_CN_Locale_t* locale);

// ============================================================================
// 日期和时间格式化
// ============================================================================

/**
 * @brief 格式化日期
 * 
 * @param time 时间值
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @param options 格式化选项（NULL表示使用默认选项）
 * @return 格式化后的字符串，失败返回NULL
 */
Stru_CN_String_t* CN_i18n_format_date(time_t time,
                                      const Stru_CN_Locale_t* locale,
                                      const Stru_CN_DateFormatOptions_t* options);

/**
 * @brief 格式化时间
 * 
 * @param time 时间值
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @param options 格式化选项（NULL表示使用默认选项）
 * @return 格式化后的字符串，失败返回NULL
 */
Stru_CN_String_t* CN_i18n_format_time(time_t time,
                                      const Stru_CN_Locale_t* locale,
                                      const Stru_CN_TimeFormatOptions_t* options);

/**
 * @brief 格式化日期和时间
 * 
 * @param time 时间值
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @param date_options 日期格式化选项
 * @param time_options 时间格式化选项
 * @return 格式化后的字符串，失败返回NULL
 */
Stru_CN_String_t* CN_i18n_format_datetime(
    time_t time,
    const Stru_CN_Locale_t* locale,
    const Stru_CN_DateFormatOptions_t* date_options,
    const Stru_CN_TimeFormatOptions_t* time_options);

// ============================================================================
// 数字格式化
// ============================================================================

/**
 * @brief 格式化整数
 * 
 * @param value 整数值
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @param options 格式化选项（NULL表示使用默认选项）
 * @return 格式化后的字符串，失败返回NULL
 */
Stru_CN_String_t* CN_i18n_format_integer(long long value,
                                         const Stru_CN_Locale_t* locale,
                                         const Stru_CN_NumberFormatOptions_t* options);

/**
 * @brief 格式化浮点数
 * 
 * @param value 浮点数值
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @param options 格式化选项（NULL表示使用默认选项）
 * @return 格式化后的字符串，失败返回NULL
 */
Stru_CN_String_t* CN_i18n_format_float(double value,
                                       const Stru_CN_Locale_t* locale,
                                       const Stru_CN_NumberFormatOptions_t* options);

/**
 * @brief 格式化货币
 * 
 * @param value 货币值
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @param options 格式化选项（NULL表示使用默认选项）
 * @return 格式化后的字符串，失败返回NULL
 */
Stru_CN_String_t* CN_i18n_format_currency(double value,
                                          const Stru_CN_Locale_t* locale,
                                          const Stru_CN_NumberFormatOptions_t* options);

// ============================================================================
// 消息本地化
// ============================================================================

/**
 * @brief 获取本地化消息
 * 
 * @param message_id 消息ID
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @param default_message 默认消息（当找不到本地化消息时使用）
 * @return 本地化消息字符串，失败返回NULL
 */
Stru_CN_String_t* CN_i18n_get_message(const char* message_id,
                                      const Stru_CN_Locale_t* locale,
                                      const char* default_message);

/**
 * @brief 格式化本地化消息
 * 
 * @param message_id 消息ID
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @param default_format 默认格式字符串
 * @param ... 格式参数
 * @return 格式化后的本地化消息，失败返回NULL
 */
Stru_CN_String_t* CN_i18n_format_message(const char* message_id,
                                         const Stru_CN_Locale_t* locale,
                                         const char* default_format, ...);

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 检测字符串的语言
 * 
 * @param str 字符串
 * @param confidence 输出参数，接收检测置信度（0.0-1.0）
 * @return 检测到的语言代码（如"zh"），失败返回NULL
 */
const char* CN_i18n_detect_language(const Stru_CN_String_t* str,
                                    float* confidence);

/**
 * @brief 转换字符串编码（考虑区域设置）
 * 
 * @param str 字符串
 * @param target_encoding 目标编码
 * @param locale 区域设置（NULL表示使用默认区域设置）
 * @return 转换成功返回true，失败返回false
 */
bool CN_i18n_convert_encoding(Stru_CN_String_t* str,
                              int target_encoding,
                              const Stru_CN_Locale_t* locale);

/**
 * @brief 获取区域设置的默认编码
 * 
 * @param locale 区域设置
 * @return 默认编码类型
 */
int CN_i18n_get_locale_default_encoding(const Stru_CN_Locale_t* locale);

#endif // CN_STRING_I18N_H
