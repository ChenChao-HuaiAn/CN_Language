/******************************************************************************
 * 文件名: CN_string_internal.h
 * 功能: CN_Language字符串模块内部定义
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，共享字符串结构体定义
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_STRING_INTERNAL_H
#define CN_STRING_INTERNAL_H

#include "CN_string_core.h"  // 包含编码枚举定义

/**
 * @brief 字符串结构体（完整定义，仅供内部使用）
 */
typedef struct Stru_CN_String_t
{
    char* data;                         /**< 字符串数据（以null结尾） */
    size_t length;                      /**< 字符串长度（字节数，不包括null终止符） */
    size_t capacity;                    /**< 缓冲区容量（字节数） */
    Eum_CN_StringEncoding_t encoding;   /**< 字符串编码 */
    int ref_count;                      /**< 引用计数 */
} Stru_CN_String_t;

#endif // CN_STRING_INTERNAL_H
