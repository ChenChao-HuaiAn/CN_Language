/******************************************************************************
 * 文件名: CN_string.h
 * 功能: CN_Language字符串处理工具 - 主头文件
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，定义字符串处理接口
 *  2026-01-03: 重构为模块化设计，包含所有子模块
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_STRING_H
#define CN_STRING_H

// ============================================================================
// 包含所有字符串模块
// ============================================================================

#include "CN_string_core.h"      // 核心字符串功能
#include "CN_string_unicode.h"   // Unicode支持
#include "CN_string_encoding.h"  // 编码转换
#include "CN_string_search.h"    // 搜索算法
#include "CN_string_regex.h"     // 正则表达式
#include "CN_string_i18n.h"      // 国际化

// ============================================================================
// 兼容性定义（保持向后兼容）
// ============================================================================

/**
 * @brief 字符串比较选项（兼容旧版本）
 */
typedef enum Eum_CN_StringCompareOption_t
{
    Eum_STRING_COMPARE_DEFAULT = 0,    /**< 默认比较（区分大小写） */
    Eum_STRING_COMPARE_CASE_INSENSITIVE = 1, /**< 不区分大小写 */
    Eum_STRING_COMPARE_NATURAL = 2,    /**< 自然排序（如"file2"在"file10"前） */
    Eum_STRING_COMPARE_LOCALE = 3      /**< 基于区域设置比较 */
} Eum_CN_StringCompareOption_t;

// ============================================================================
// 字符串比较函数（兼容旧版本）
// ============================================================================

/**
 * @brief 比较两个字符串（兼容旧版本）
 * 
 * @param str1 第一个字符串
 * @param str2 第二个字符串
 * @param options 比较选项
 * @return 比较结果：<0表示str1<str2，0表示相等，>0表示str1>str2
 */
int CN_string_compare(const Stru_CN_String_t* str1,
                      const Stru_CN_String_t* str2,
                      Eum_CN_StringCompareOption_t options);

/**
 * @brief 检查字符串是否相等（兼容旧版本）
 * 
 * @param str1 第一个字符串
 * @param str2 第二个字符串
 * @param case_sensitive 是否区分大小写
 * @return 如果字符串相等返回true，否则返回false
 */
bool CN_string_equal(const Stru_CN_String_t* str1,
                     const Stru_CN_String_t* str2,
                     bool case_sensitive);

/**
 * @brief 检查字符串是否以指定前缀开头（兼容旧版本）
 * 
 * @param str 字符串
 * @param prefix 前缀
 * @param case_sensitive 是否区分大小写
 * @return 如果以指定前缀开头返回true，否则返回false
 */
bool CN_string_starts_with(const Stru_CN_String_t* str,
                           const Stru_CN_String_t* prefix,
                           bool case_sensitive);

/**
 * @brief 检查字符串是否以指定后缀结尾（兼容旧版本）
 * 
 * @param str 字符串
 * @param suffix 后缀
 * @param case_sensitive 是否区分大小写
 * @return 如果以指定后缀结尾返回true，否则返回false
 */
bool CN_string_ends_with(const Stru_CN_String_t* str,
                         const Stru_CN_String_t* suffix,
                         bool case_sensitive);

/**
 * @brief 查找子字符串（兼容旧版本）
 * 
 * @param str 字符串
 * @param substr 要查找的子字符串
 * @param start_pos 开始查找的位置
 * @param case_sensitive 是否区分大小写
 * @return 子字符串位置（字节偏移），未找到返回SIZE_MAX
 */
size_t CN_string_find(const Stru_CN_String_t* str,
                      const Stru_CN_String_t* substr,
                      size_t start_pos, bool case_sensitive);

// ============================================================================
// 字符串转换函数（兼容旧版本）
// ============================================================================

/**
 * @brief 转换为大写（兼容旧版本，使用Unicode版本）
 * 
 * @param str 字符串
 * @return 转换成功返回true，失败返回false
 */
bool CN_string_to_upper(Stru_CN_String_t* str);

/**
 * @brief 转换为小写（兼容旧版本，使用Unicode版本）
 * 
 * @param str 字符串
 * @return 转换成功返回true，失败返回false
 */
bool CN_string_to_lower(Stru_CN_String_t* str);

/**
 * @brief 去除首尾空白字符（兼容旧版本）
 * 
 * @param str 字符串
 */
void CN_string_trim(Stru_CN_String_t* str);

/**
 * @brief 去除开头空白字符（兼容旧版本）
 * 
 * @param str 字符串
 */
void CN_string_trim_start(Stru_CN_String_t* str);

/**
 * @brief 去除结尾空白字符（兼容旧版本）
 * 
 * @param str 字符串
 */
void CN_string_trim_end(Stru_CN_String_t* str);

/**
 * @brief 转换字符串编码（兼容旧版本）
 * 
 * @param str 字符串
 * @param new_encoding 新编码
 * @return 转换成功返回true，失败返回false
 */
bool CN_string_convert_encoding(Stru_CN_String_t* str,
                                Eum_CN_StringEncoding_t new_encoding);

// ============================================================================
// 格式化字符串函数（兼容旧版本）
// ============================================================================

/**
 * @brief 创建格式化字符串（兼容旧版本）
 * 
 * @param format 格式字符串
 * @param ... 格式参数
 * @return 新创建的格式化字符串，失败返回NULL
 */
Stru_CN_String_t* CN_string_format(const char* format, ...);

/**
 * @brief 追加格式化字符串（兼容旧版本）
 * 
 * @param str 目标字符串
 * @param format 格式字符串
 * @param ... 格式参数
 * @return 追加成功返回true，失败返回false
 */
bool CN_string_append_format(Stru_CN_String_t* str, const char* format, ...);

// ============================================================================
// 工具函数（兼容旧版本）
// ============================================================================

/**
 * @brief 分割字符串（兼容旧版本）
 * 
 * @param str 源字符串
 * @param delimiter 分隔符
 * @param max_parts 最大分割部分数（0表示无限制）
 * @param part_count 输出参数，接收实际分割部分数
 * @return 分割后的字符串数组，调用者负责释放，失败返回NULL
 */
Stru_CN_String_t** CN_string_split(const Stru_CN_String_t* str,
                                   const Stru_CN_String_t* delimiter,
                                   size_t max_parts, size_t* part_count);

/**
 * @brief 连接字符串数组（兼容旧版本）
 * 
 * @param strings 字符串数组
 * @param count 字符串数量
 * @param separator 分隔符（可为NULL）
 * @return 新创建的连接后的字符串，失败返回NULL
 */
Stru_CN_String_t* CN_string_join(Stru_CN_String_t* const* strings,
                                 size_t count,
                                 const Stru_CN_String_t* separator);

/**
 * @brief 转义字符串（兼容旧版本）
 * 
 * @param str 源字符串
 * @return 新创建的转义后的字符串，失败返回NULL
 */
Stru_CN_String_t* CN_string_escape(const Stru_CN_String_t* str);

/**
 * @brief 反转义字符串（兼容旧版本）
 * 
 * @param str 源字符串
 * @return 新创建的反转义后的字符串，失败返回NULL
 */
Stru_CN_String_t* CN_string_unescape(const Stru_CN_String_t* str);

#endif // CN_STRING_H
