/**
 * @file CN_lexer_errors.h
 * @brief CN_Language 词法分析器错误处理模块
 * 
 * 提供词法分析器的错误处理功能，包括错误码定义、错误信息管理和错误恢复。
 * 遵循项目架构规范，实现模块化的错误处理。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#ifndef CN_LEXER_ERRORS_H
#define CN_LEXER_ERRORS_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief 词法分析器错误码枚举
 * 
 * 定义词法分析过程中可能遇到的各种错误类型。
 */
typedef enum Eum_LexerErrorCode_t {
    Eum_LEXER_ERROR_NONE = 0,               ///< 无错误
    Eum_LEXER_ERROR_UNKNOWN_CHAR,           ///< 未知字符
    Eum_LEXER_ERROR_UNTERMINATED_STRING,    ///< 未终止的字符串
    Eum_LEXER_ERROR_INVALID_NUMBER,         ///< 无效的数字格式
    Eum_LEXER_ERROR_INVALID_ESCAPE,         ///< 无效的转义序列
    Eum_LEXER_ERROR_INVALID_UTF8,           ///< 无效的UTF-8编码
    Eum_LEXER_ERROR_BUFFER_OVERFLOW,        ///< 缓冲区溢出
    Eum_LEXER_ERROR_MEMORY_ALLOCATION,      ///< 内存分配失败
    Eum_LEXER_ERROR_INVALID_STATE,          ///< 无效的状态
    Eum_LEXER_ERROR_INVALID_PARAMETER,      ///< 无效的参数
    Eum_LEXER_ERROR_EOF_UNEXPECTED,         ///< 意外的文件结束
    Eum_LEXER_ERROR_INTERNAL                ///< 内部错误
} Eum_LexerErrorCode_t;

/**
 * @brief 词法分析器错误信息结构体
 * 
 * 存储错误的详细信息，包括错误码、位置和描述。
 */
typedef struct Stru_LexerErrorInfo_t {
    Eum_LexerErrorCode_t code;      ///< 错误码
    size_t line;                    ///< 错误发生的行号（从1开始）
    size_t column;                  ///< 错误发生的列号（从1开始）
    const char* source_name;        ///< 源文件名
    char message[256];              ///< 错误描述信息
} Stru_LexerErrorInfo_t;

/**
 * @brief 错误上下文结构体
 * 
 * 存储错误上下文信息，支持错误链。
 */
typedef struct Stru_LexerErrorContext_t {
    Stru_LexerErrorInfo_t current_error;    ///< 当前错误
    Stru_LexerErrorInfo_t* previous_errors; ///< 之前的错误链（可选）
    size_t error_count;                     ///< 错误数量
    bool has_fatal_error;                   ///< 是否有致命错误
} Stru_LexerErrorContext_t;

// ============================================
// 错误上下文管理函数
// ============================================

/**
 * @brief 创建错误上下文
 * 
 * 创建并初始化一个新的错误上下文。
 * 
 * @return Stru_LexerErrorContext_t* 新创建的错误上下文
 * @retval NULL 创建失败
 */
Stru_LexerErrorContext_t* F_create_lexer_error_context(void);

/**
 * @brief 销毁错误上下文
 * 
 * 释放错误上下文占用的所有资源。
 * 
 * @param context 要销毁的错误上下文
 */
void F_destroy_lexer_error_context(Stru_LexerErrorContext_t* context);

/**
 * @brief 重置错误上下文
 * 
 * 清除错误上下文中的所有错误信息。
 * 
 * @param context 要重置的错误上下文
 */
void F_reset_lexer_error_context(Stru_LexerErrorContext_t* context);

// ============================================
// 错误报告函数
// ============================================

/**
 * @brief 报告词法分析错误
 * 
 * 向错误上下文报告一个新的错误。
 * 
 * @param context 错误上下文
 * @param code 错误码
 * @param line 错误行号
 * @param column 错误列号
 * @param source_name 源文件名
 * @param format 错误信息格式字符串
 * @param ... 格式参数
 * @return true 错误报告成功
 * @return false 错误报告失败
 */
bool F_report_lexer_error(Stru_LexerErrorContext_t* context,
                         Eum_LexerErrorCode_t code,
                         size_t line, size_t column,
                         const char* source_name,
                         const char* format, ...);

/**
 * @brief 报告致命错误
 * 
 * 报告一个致命错误，表示词法分析无法继续。
 * 
 * @param context 错误上下文
 * @param code 错误码
 * @param line 错误行号
 * @param column 错误列号
 * @param source_name 源文件名
 * @param format 错误信息格式字符串
 * @param ... 格式参数
 * @return true 错误报告成功
 * @return false 错误报告失败
 */
bool F_report_fatal_lexer_error(Stru_LexerErrorContext_t* context,
                               Eum_LexerErrorCode_t code,
                               size_t line, size_t column,
                               const char* source_name,
                               const char* format, ...);

// ============================================
// 错误查询函数
// ============================================

/**
 * @brief 检查是否有错误
 * 
 * 检查错误上下文中是否有任何错误。
 * 
 * @param context 错误上下文
 * @return true 有错误
 * @return false 没有错误
 */
bool F_has_lexer_errors(const Stru_LexerErrorContext_t* context);

/**
 * @brief 检查是否有致命错误
 * 
 * 检查错误上下文中是否有致命错误。
 * 
 * @param context 错误上下文
 * @return true 有致命错误
 * @return false 没有致命错误
 */
bool F_has_fatal_lexer_error(const Stru_LexerErrorContext_t* context);

/**
 * @brief 获取错误数量
 * 
 * 获取错误上下文中的错误总数。
 * 
 * @param context 错误上下文
 * @return size_t 错误数量
 */
size_t F_get_lexer_error_count(const Stru_LexerErrorContext_t* context);

/**
 * @brief 获取最后一个错误信息
 * 
 * 获取最后一个错误的详细信息。
 * 
 * @param context 错误上下文
 * @return const Stru_LexerErrorInfo_t* 最后一个错误信息，NULL表示没有错误
 */
const Stru_LexerErrorInfo_t* F_get_last_lexer_error(const Stru_LexerErrorContext_t* context);

/**
 * @brief 获取错误信息字符串
 * 
 * 获取指定错误的描述字符串。
 * 
 * @param context 错误上下文
 * @param index 错误索引（0表示第一个错误）
 * @return const char* 错误描述字符串，NULL表示索引无效
 */
const char* F_get_lexer_error_message(const Stru_LexerErrorContext_t* context, size_t index);

/**
 * @brief 获取错误码描述
 * 
 * 获取错误码的文本描述。
 * 
 * @param code 错误码
 * @return const char* 错误码描述
 */
const char* F_get_lexer_error_code_description(Eum_LexerErrorCode_t code);

// ============================================
// 错误恢复函数
// ============================================

/**
 * @brief 尝试从错误中恢复
 * 
 * 尝试从当前错误中恢复，继续词法分析。
 * 
 * @param context 错误上下文
 * @param current_char 当前字符
 * @param next_char 下一个字符
 * @return true 成功恢复，可以继续分析
 * @return false 无法恢复，需要停止分析
 */
bool F_try_recover_from_lexer_error(Stru_LexerErrorContext_t* context,
                                   char current_char, char next_char);

/**
 * @brief 跳过错误字符
 * 
 * 跳过导致错误的字符，尝试继续分析。
 * 
 * @param context 错误上下文
 * @param state 扫描器状态（用于获取当前位置）
 * @return size_t 跳过的字符数
 */
size_t F_skip_error_chars(Stru_LexerErrorContext_t* context, void* scanner_state);

#endif // CN_LEXER_ERRORS_H
