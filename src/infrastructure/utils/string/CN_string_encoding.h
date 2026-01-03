/******************************************************************************
 * 文件名: CN_string_encoding.h
 * 功能: CN_Language字符串编码转换模块
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，定义编码转换接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_STRING_ENCODING_H
#define CN_STRING_ENCODING_H

#include "CN_string_core.h"
#include <stdint.h>

// ============================================================================
// 编码转换函数声明
// ============================================================================

/**
 * @brief 将字符串转换为指定编码
 * 
 * @param str 源字符串
 * @param target_encoding 目标编码
 * @return 新创建的转换后的字符串，失败返回NULL
 */
Stru_CN_String_t* CN_string_convert_to_encoding(const Stru_CN_String_t* str,
                                                Eum_CN_StringEncoding_t target_encoding);

/**
 * @brief 检查是否支持指定的编码转换
 * 
 * @param source_encoding 源编码
 * @param target_encoding 目标编码
 * @return 如果支持转换返回true，否则返回false
 */
bool CN_string_encoding_conversion_supported(Eum_CN_StringEncoding_t source_encoding,
                                             Eum_CN_StringEncoding_t target_encoding);

/**
 * @brief 获取编码名称
 * 
 * @param encoding 编码类型
 * @return 编码名称字符串
 */
const char* CN_string_encoding_name(Eum_CN_StringEncoding_t encoding);

/**
 * @brief 检测字符串编码
 * 
 * @param data 字符串数据
 * @param length 数据长度
 * @return 检测到的编码类型
 */
Eum_CN_StringEncoding_t CN_string_detect_encoding(const char* data, size_t length);

/**
 * @brief UTF-8到UTF-16转换
 * 
 * @param utf8_str UTF-8字符串
 * @param utf8_len UTF-8字符串长度
 * @param utf16_str 输出参数：UTF-16字符串缓冲区
 * @param utf16_buf_size 缓冲区大小（以16位字符计）
 * @return 实际写入的UTF-16字符数（不包括null终止符）
 */
size_t CN_string_utf8_to_utf16(const char* utf8_str, size_t utf8_len,
                               uint16_t* utf16_str, size_t utf16_buf_size);

/**
 * @brief UTF-16到UTF-8转换
 * 
 * @param utf16_str UTF-16字符串
 * @param utf16_len UTF-16字符串长度（以16位字符计）
 * @param utf8_str 输出参数：UTF-8字符串缓冲区
 * @param utf8_buf_size 缓冲区大小（以字节计）
 * @return 实际写入的UTF-8字节数（不包括null终止符）
 */
size_t CN_string_utf16_to_utf8(const uint16_t* utf16_str, size_t utf16_len,
                               char* utf8_str, size_t utf8_buf_size);

/**
 * @brief UTF-8到UTF-32转换
 * 
 * @param utf8_str UTF-8字符串
 * @param utf8_len UTF-8字符串长度
 * @param utf32_str 输出参数：UTF-32字符串缓冲区
 * @param utf32_buf_size 缓冲区大小（以32位字符计）
 * @return 实际写入的UTF-32字符数（不包括null终止符）
 */
size_t CN_string_utf8_to_utf32(const char* utf8_str, size_t utf8_len,
                               uint32_t* utf32_str, size_t utf32_buf_size);

/**
 * @brief UTF-32到UTF-8转换
 * 
 * @param utf32_str UTF-32字符串
 * @param utf32_len UTF-32字符串长度（以32位字符计）
 * @param utf8_str 输出参数：UTF-8字符串缓冲区
 * @param utf8_buf_size 缓冲区大小（以字节计）
 * @return 实际写入的UTF-8字节数（不包括null终止符）
 */
size_t CN_string_utf32_to_utf8(const uint32_t* utf32_str, size_t utf32_len,
                               char* utf8_str, size_t utf8_buf_size);

/**
 * @brief GB2312到UTF-8转换（简体中文）
 * 
 * @param gb2312_str GB2312字符串
 * @param gb2312_len GB2312字符串长度
 * @param utf8_str 输出参数：UTF-8字符串缓冲区
 * @param utf8_buf_size 缓冲区大小（以字节计）
 * @return 实际写入的UTF-8字节数（不包括null终止符）
 */
size_t CN_string_gb2312_to_utf8(const char* gb2312_str, size_t gb2312_len,
                                char* utf8_str, size_t utf8_buf_size);

/**
 * @brief UTF-8到GB2312转换（简体中文）
 * 
 * @param utf8_str UTF-8字符串
 * @param utf8_len UTF-8字符串长度
 * @param gb2312_str 输出参数：GB2312字符串缓冲区
 * @param gb2312_buf_size 缓冲区大小（以字节计）
 * @return 实际写入的GB2312字节数（不包括null终止符）
 */
size_t CN_string_utf8_to_gb2312(const char* utf8_str, size_t utf8_len,
                                char* gb2312_str, size_t gb2312_buf_size);

#endif // CN_STRING_ENCODING_H
