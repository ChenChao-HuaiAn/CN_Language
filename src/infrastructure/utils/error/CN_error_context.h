/******************************************************************************
 * 文件名: CN_error_context.h
 * 功能: CN_Language错误上下文定义
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，定义错误上下文结构
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_ERROR_CONTEXT_H
#define CN_ERROR_CONTEXT_H

#include "CN_error_codes.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief 错误上下文结构体
 * 
 * 包含错误的详细上下文信息，如文件名、行号、列号等。
 */
typedef struct Stru_CN_ErrorContext_t
{
    /** 错误码 */
    Eum_CN_ErrorCode_t error_code;
    
    /** 错误消息（可选的详细描述） */
    const char* message;
    
    /** 文件名（发生错误的源文件） */
    const char* filename;
    
    /** 行号（从1开始） */
    size_t line;
    
    /** 列号（从1开始） */
    size_t column;
    
    /** 函数名 */
    const char* function;
    
    /** 模块名 */
    const char* module;
    
    /** 时间戳（毫秒） */
    uint64_t timestamp;
    
    /** 线程ID */
    uint64_t thread_id;
    
    /** 是否可恢复 */
    bool recoverable;
    
    /** 用户自定义数据指针 */
    void* user_data;
    
    /** 用户自定义数据大小 */
    size_t user_data_size;
    
} Stru_CN_ErrorContext_t;

/**
 * @brief 创建错误上下文
 * 
 * @param error_code 错误码
 * @param message 错误消息（可以为NULL）
 * @param filename 文件名（可以为NULL）
 * @param line 行号
 * @param column 列号
 * @param function 函数名（可以为NULL）
 * @param module 模块名（可以为NULL）
 * @return 错误上下文结构体
 */
Stru_CN_ErrorContext_t CN_error_create_context(
    Eum_CN_ErrorCode_t error_code,
    const char* message,
    const char* filename,
    size_t line,
    size_t column,
    const char* function,
    const char* module);

/**
 * @brief 创建简单的错误上下文（只有错误码和消息）
 * 
 * @param error_code 错误码
 * @param message 错误消息
 * @return 错误上下文结构体
 */
Stru_CN_ErrorContext_t CN_error_create_simple_context(
    Eum_CN_ErrorCode_t error_code,
    const char* message);

/**
 * @brief 创建带位置信息的错误上下文
 * 
 * @param error_code 错误码
 * @param message 错误消息
 * @param filename 文件名
 * @param line 行号
 * @param column 列号
 * @return 错误上下文结构体
 */
Stru_CN_ErrorContext_t CN_error_create_position_context(
    Eum_CN_ErrorCode_t error_code,
    const char* message,
    const char* filename,
    size_t line,
    size_t column);

/**
 * @brief 格式化错误上下文为字符串
 * 
 * @param context 错误上下文
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 实际写入的字符数（不包括终止符）
 */
size_t CN_error_format_context(
    const Stru_CN_ErrorContext_t* context,
    char* buffer,
    size_t buffer_size);

/**
 * @brief 获取错误上下文的默认消息
 * 
 * @param context 错误上下文
 * @return 默认消息字符串
 */
const char* CN_error_get_default_message(
    const Stru_CN_ErrorContext_t* context);

/**
 * @brief 检查错误上下文是否有效
 * 
 * @param context 错误上下文
 * @return true 如果有效，false 如果无效
 */
bool CN_error_is_context_valid(const Stru_CN_ErrorContext_t* context);

/**
 * @brief 清空错误上下文
 * 
 * @param context 错误上下文
 */
void CN_error_clear_context(Stru_CN_ErrorContext_t* context);

/**
 * @brief 复制错误上下文
 * 
 * @param dest 目标上下文
 * @param src 源上下文
 * @return true 如果成功，false 如果失败
 */
bool CN_error_copy_context(
    Stru_CN_ErrorContext_t* dest,
    const Stru_CN_ErrorContext_t* src);

#endif // CN_ERROR_CONTEXT_H
