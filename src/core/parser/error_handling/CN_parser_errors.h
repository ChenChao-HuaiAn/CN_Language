/**
 * @file CN_parser_errors.h
 * @brief CN_Language 语法分析器错误处理模块
 * 
 * 语法分析器错误处理功能模块，负责处理语法分析过程中的错误。
 * 包括错误报告、错误恢复、错误信息格式化等。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_PARSER_ERRORS_H
#define CN_PARSER_ERRORS_H

#include "../CN_parser_interface.h"
#include "../CN_syntax_error.h"
#include "CN_parser_phrase_recovery.h"

// ============================================
// 错误处理函数声明
// ============================================

/**
 * @brief 报告语法分析错误
 * 
 * 报告语法分析过程中发现的错误。
 * 
 * @param interface 语法分析器接口指针
 * @param error_type 错误类型
 * @param severity 错误严重级别
 * @param line 错误行号
 * @param column 错误列号
 * @param message 错误信息
 * @param token 出错的令牌（可选）
 * @return Stru_SyntaxError_t* 创建的错误对象
 */
Stru_SyntaxError_t* F_report_parser_error(Stru_ParserInterface_t* interface,
                                         Eum_SyntaxErrorType error_type,
                                         Eum_SyntaxErrorSeverity severity,
                                         size_t line, size_t column,
                                         const char* message,
                                         Stru_Token_t* token);

/**
 * @brief 报告意外令牌错误
 * 
 * 报告意外令牌错误（常用错误类型）。
 * 
 * @param interface 语法分析器接口指针
 * @param token 意外的令牌
 * @param expected 期望的令牌类型或内容
 * @return Stru_SyntaxError_t* 创建的错误对象
 */
Stru_SyntaxError_t* F_report_unexpected_token_error(Stru_ParserInterface_t* interface,
                                                   Stru_Token_t* token,
                                                   const char* expected);

/**
 * @brief 报告缺失令牌错误
 * 
 * 报告缺失令牌错误。
 * 
 * @param interface 语法分析器接口指针
 * @param line 错误行号
 * @param column 错误列号
 * @param expected 期望的令牌类型或内容
 * @return Stru_SyntaxError_t* 创建的错误对象
 */
Stru_SyntaxError_t* F_report_missing_token_error(Stru_ParserInterface_t* interface,
                                                size_t line, size_t column,
                                                const char* expected);

/**
 * @brief 报告类型不匹配错误
 * 
 * 报告类型不匹配错误。
 * 
 * @param interface 语法分析器接口指针
 * @param line 错误行号
 * @param column 错误列号
 * @param expected_type 期望的类型
 * @param actual_type 实际的类型
 * @param token 相关的令牌（可选）
 * @return Stru_SyntaxError_t* 创建的错误对象
 */
Stru_SyntaxError_t* F_report_type_mismatch_error(Stru_ParserInterface_t* interface,
                                                size_t line, size_t column,
                                                const char* expected_type,
                                                const char* actual_type,
                                                Stru_Token_t* token);

/**
 * @brief 报告重复声明错误
 * 
 * 报告重复声明错误。
 * 
 * @param interface 语法分析器接口指针
 * @param line 错误行号
 * @param column 错误列号
 * @param identifier 重复的标识符
 * @param token 相关的令牌（可选）
 * @return Stru_SyntaxError_t* 创建的错误对象
 */
Stru_SyntaxError_t* F_report_duplicate_declaration_error(Stru_ParserInterface_t* interface,
                                                        size_t line, size_t column,
                                                        const char* identifier,
                                                        Stru_Token_t* token);

/**
 * @brief 报告未声明标识符错误
 * 
 * 报告未声明标识符错误。
 * 
 * @param interface 语法分析器接口指针
 * @param line 错误行号
 * @param column 错误列号
 * @param identifier 未声明的标识符
 * @param token 相关的令牌（可选）
 * @return Stru_SyntaxError_t* 创建的错误对象
 */
Stru_SyntaxError_t* F_report_undeclared_identifier_error(Stru_ParserInterface_t* interface,
                                                        size_t line, size_t column,
                                                        const char* identifier,
                                                        Stru_Token_t* token);

/**
 * @brief 报告参数数量错误
 * 
 * 报告函数调用参数数量错误。
 * 
 * @param interface 语法分析器接口指针
 * @param line 错误行号
 * @param column 错误列号
 * @param function_name 函数名
 * @param expected_count 期望的参数数量
 * @param actual_count 实际的参数数量
 * @param token 相关的令牌（可选）
 * @return Stru_SyntaxError_t* 创建的错误对象
 */
Stru_SyntaxError_t* F_report_argument_count_error(Stru_ParserInterface_t* interface,
                                                 size_t line, size_t column,
                                                 const char* function_name,
                                                 size_t expected_count,
                                                 size_t actual_count,
                                                 Stru_Token_t* token);

/**
 * @brief 报告无效表达式错误
 * 
 * 报告无效表达式错误。
 * 
 * @param interface 语法分析器接口指针
 * @param line 错误行号
 * @param column 错误列号
 * @param message 错误信息
 * @param token 相关的令牌（可选）
 * @return Stru_SyntaxError_t* 创建的错误对象
 */
Stru_SyntaxError_t* F_report_invalid_expression_error(Stru_ParserInterface_t* interface,
                                                     size_t line, size_t column,
                                                     const char* message,
                                                     Stru_Token_t* token);

/**
 * @brief 报告无效语句错误
 * 
 * 报告无效语句错误。
 * 
 * @param interface 语法分析器接口指针
 * @param line 错误行号
 * @param column 错误列号
 * @param message 错误信息
 * @param token 相关的令牌（可选）
 * @return Stru_SyntaxError_t* 创建的错误对象
 */
Stru_SyntaxError_t* F_report_invalid_statement_error(Stru_ParserInterface_t* interface,
                                                    size_t line, size_t column,
                                                    const char* message,
                                                    Stru_Token_t* token);

/**
 * @brief 报告无效声明错误
 * 
 * 报告无效声明错误。
 * 
 * @param interface 语法分析器接口指针
 * @param line 错误行号
 * @param column 错误列号
 * @param message 错误信息
 * @param token 相关的令牌（可选）
 * @return Stru_SyntaxError_t* 创建的错误对象
 */
Stru_SyntaxError_t* F_report_invalid_declaration_error(Stru_ParserInterface_t* interface,
                                                      size_t line, size_t column,
                                                      const char* message,
                                                      Stru_Token_t* token);

/**
 * @brief 报告未关闭结构错误
 * 
 * 报告未关闭的代码块、括号、方括号或大括号错误。
 * 
 * @param interface 语法分析器接口指针
 * @param line 错误行号
 * @param column 错误列号
 * @param structure_type 结构类型（如"代码块"、"括号"等）
 * @param token 相关的令牌（可选）
 * @return Stru_SyntaxError_t* 创建的错误对象
 */
Stru_SyntaxError_t* F_report_unclosed_structure_error(Stru_ParserInterface_t* interface,
                                                     size_t line, size_t column,
                                                     const char* structure_type,
                                                     Stru_Token_t* token);

/**
 * @brief 尝试错误恢复
 * 
 * 在发生错误后尝试恢复语法分析。
 * 包括短语级恢复和同步恢复。
 * 
 * @param interface 语法分析器接口指针
 * @param error 发生的错误
 * @return bool 恢复成功返回true，否则返回false
 */
bool F_try_error_recovery(Stru_ParserInterface_t* interface, Stru_SyntaxError_t* error);

/**
 * @brief 尝试短语级错误恢复
 * 
 * 尝试使用短语级恢复策略修复错误。
 * 这是F_try_error_recovery的内部辅助函数。
 * 
 * @param interface 语法分析器接口指针
 * @param error 发生的错误
 * @return bool 短语级恢复成功返回true，否则返回false
 */
bool F_try_phrase_level_recovery(Stru_ParserInterface_t* interface, Stru_SyntaxError_t* error);

/**
 * @brief 应用短语级恢复结果
 * 
 * 应用短语级恢复的结果到语法分析器状态。
 * 
 * @param interface 语法分析器接口指针
 * @param recovery_result 恢复结果
 * @return bool 应用成功返回true，否则返回false
 */
bool F_apply_phrase_recovery(Stru_ParserInterface_t* interface, 
                            Stru_PhraseRecoveryResult_t* recovery_result);

/**
 * @brief 获取常见错误模式建议
 * 
 * 根据错误类型和上下文获取常见的错误模式和建议。
 * 
 * @param interface 语法分析器接口指针
 * @param error 发生的错误
 * @param suggestion_buffer 建议缓冲区
 * @param buffer_size 缓冲区大小
 * @return int 写入的字符数
 */
int F_get_common_error_pattern_suggestion(Stru_ParserInterface_t* interface,
                                         Stru_SyntaxError_t* error,
                                         char* suggestion_buffer,
                                         size_t buffer_size);

/**
 * @brief 同步到安全点
 * 
 * 在发生错误后同步到安全点继续分析。
 * 
 * @param interface 语法分析器接口指针
 * @param sync_tokens 同步令牌类型数组
 * @param sync_count 同步令牌数量
 */
void F_synchronize_to_safe_point(Stru_ParserInterface_t* interface,
                                Eum_TokenType* sync_tokens,
                                size_t sync_count);

/**
 * @brief 获取错误上下文信息
 * 
 * 获取错误发生的上下文信息。
 * 
 * @param interface 语法分析器接口指针
 * @param line 行号
 * @param column 列号
 * @param context_buffer 上下文缓冲区
 * @param buffer_size 缓冲区大小
 * @return int 写入的字符数
 */
int F_get_error_context(Stru_ParserInterface_t* interface,
                       size_t line, size_t column,
                       char* context_buffer, size_t buffer_size);

#endif // CN_PARSER_ERRORS_H
