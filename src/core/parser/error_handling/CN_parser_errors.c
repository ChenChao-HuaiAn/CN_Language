/**
 * @file CN_parser_errors.c
 * @brief CN_Language 语法分析器错误处理模块实现
 * 
 * 语法分析器错误处理功能模块实现，负责处理语法分析过程中的错误。
 * 包括错误报告、错误恢复、错误信息格式化等。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_parser_errors.h"
#include "../interface/CN_parser_interface_impl.h"
#include "../../../infrastructure/memory/CN_memory_interface.h"
#include "../../../infrastructure/utils/CN_utils_interface.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// ============================================
// 错误处理函数实现
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
                                         Stru_Token_t* token)
{
    if (interface == NULL) {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->error_handler == NULL) {
        return NULL;
    }
    
    // 创建错误对象
    Stru_SyntaxError_t* error = state->error_handler->report_error(
        state->error_handler, error_type, severity, line, column, message, token);
    
    if (error != NULL) {
        state->has_errors = true;
    }
    
    return error;
}

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
                                                   const char* expected)
{
    if (interface == NULL || token == NULL) {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->error_handler == NULL) {
        return NULL;
    }
    
    // 使用错误处理接口的报告函数
    Stru_SyntaxError_t* error = state->error_handler->report_unexpected_token(
        state->error_handler, token, expected);
    
    if (error != NULL) {
        state->has_errors = true;
    }
    
    return error;
}

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
                                                const char* expected)
{
    if (interface == NULL) {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->error_handler == NULL) {
        return NULL;
    }
    
    // 使用错误处理接口的报告函数
    Stru_SyntaxError_t* error = state->error_handler->report_missing_token(
        state->error_handler, line, column, expected);
    
    if (error != NULL) {
        state->has_errors = true;
    }
    
    return error;
}

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
                                                Stru_Token_t* token)
{
    if (interface == NULL) {
        return NULL;
    }
    
    char message[256];
    if (expected_type != NULL && actual_type != NULL) {
        snprintf(message, sizeof(message), 
                 "类型不匹配，期望: %s，实际: %s", 
                 expected_type, actual_type);
    } else if (expected_type != NULL) {
        snprintf(message, sizeof(message), 
                 "类型不匹配，期望: %s", 
                 expected_type);
    } else {
        snprintf(message, sizeof(message), 
                 "类型不匹配");
    }
    
    return F_report_parser_error(interface,
                                Eum_ERROR_TYPE_MISMATCH,
                                Eum_SEVERITY_ERROR,
                                line, column,
                                message, token);
}

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
                                                        Stru_Token_t* token)
{
    if (interface == NULL || identifier == NULL) {
        return NULL;
    }
    
    char message[256];
    snprintf(message, sizeof(message), 
             "重复声明标识符: %s", 
             identifier);
    
    return F_report_parser_error(interface,
                                Eum_ERROR_DUPLICATE_DECLARATION,
                                Eum_SEVERITY_ERROR,
                                line, column,
                                message, token);
}

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
                                                        Stru_Token_t* token)
{
    if (interface == NULL || identifier == NULL) {
        return NULL;
    }
    
    char message[256];
    snprintf(message, sizeof(message), 
             "未声明的标识符: %s", 
             identifier);
    
    return F_report_parser_error(interface,
                                Eum_ERROR_UNDECLARED_IDENTIFIER,
                                Eum_SEVERITY_ERROR,
                                line, column,
                                message, token);
}

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
                                                 Stru_Token_t* token)
{
    if (interface == NULL || function_name == NULL) {
        return NULL;
    }
    
    char message[256];
    snprintf(message, sizeof(message), 
             "函数 '%s' 参数数量错误，期望: %zu，实际: %zu", 
             function_name, expected_count, actual_count);
    
    return F_report_parser_error(interface,
                                Eum_ERROR_ARGUMENT_COUNT,
                                Eum_SEVERITY_ERROR,
                                line, column,
                                message, token);
}

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
                                                     Stru_Token_t* token)
{
    if (interface == NULL) {
        return NULL;
    }
    
    char full_message[256];
    if (message != NULL) {
        snprintf(full_message, sizeof(full_message), 
                 "无效表达式: %s", 
                 message);
    } else {
        snprintf(full_message, sizeof(full_message), 
                 "无效表达式");
    }
    
    return F_report_parser_error(interface,
                                Eum_ERROR_INVALID_EXPRESSION,
                                Eum_SEVERITY_ERROR,
                                line, column,
                                full_message, token);
}

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
                                                    Stru_Token_t* token)
{
    if (interface == NULL) {
        return NULL;
    }
    
    char full_message[256];
    if (message != NULL) {
        snprintf(full_message, sizeof(full_message), 
                 "无效语句: %s", 
                 message);
    } else {
        snprintf(full_message, sizeof(full_message), 
                 "无效语句");
    }
    
    return F_report_parser_error(interface,
                                Eum_ERROR_INVALID_STATEMENT,
                                Eum_SEVERITY_ERROR,
                                line, column,
                                full_message, token);
}

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
                                                      Stru_Token_t* token)
{
    if (interface == NULL) {
        return NULL;
    }
    
    char full_message[256];
    if (message != NULL) {
        snprintf(full_message, sizeof(full_message), 
                 "无效声明: %s", 
                 message);
    } else {
        snprintf(full_message, sizeof(full_message), 
                 "无效声明");
    }
    
    return F_report_parser_error(interface,
                                Eum_ERROR_INVALID_DECLARATION,
                                Eum_SEVERITY_ERROR,
                                line, column,
                                full_message, token);
}

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
                                                     Stru_Token_t* token)
{
    if (interface == NULL) {
        return NULL;
    }
    
    char message[256];
    if (structure_type != NULL) {
        snprintf(message, sizeof(message), 
                 "未关闭的%s", 
                 structure_type);
    } else {
        snprintf(message, sizeof(message), 
                 "未关闭的结构");
    }
    
    // 根据结构类型选择具体的错误类型
    Eum_SyntaxErrorType error_type = Eum_ERROR_UNCLOSED_BLOCK;
    if (structure_type != NULL) {
        if (strstr(structure_type, "括号") != NULL) {
            error_type = Eum_ERROR_UNCLOSED_PAREN;
        } else if (strstr(structure_type, "方括号") != NULL) {
            error_type = Eum_ERROR_UNCLOSED_BRACKET;
        } else if (strstr(structure_type, "大括号") != NULL) {
            error_type = Eum_ERROR_UNCLOSED_BRACE;
        }
    }
    
    return F_report_parser_error(interface,
                                error_type,
                                Eum_SEVERITY_ERROR,
                                line, column,
                                message, token);
}

/**
 * @brief 尝试错误恢复
 * 
 * 在发生错误后尝试恢复语法分析。
 * 
 * @param interface 语法分析器接口指针
 * @param error 发生的错误
 * @return bool 恢复成功返回true，否则返回false
 */
bool F_try_error_recovery(Stru_ParserInterface_t* interface, Stru_SyntaxError_t* error)
{
    if (interface == NULL || error == NULL) {
        return false;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL) {
        return false;
    }
    
    // 默认同步令牌：分号、右大括号、文件结尾
    Eum_TokenType sync_tokens[] = {
        Eum_TOKEN_DELIMITER_SEMICOLON,
        Eum_TOKEN_DELIMITER_RBRACE,
        Eum_TOKEN_EOF
    };
    
    F_synchronize_to_safe_point(interface, sync_tokens, 
                               sizeof(sync_tokens) / sizeof(sync_tokens[0]));
    
    // 检查是否成功同步
    if (state->current_token != NULL) {
        for (size_t i = 0; i < sizeof(sync_tokens) / sizeof(sync_tokens[0]); i++) {
            if (state->current_token->type == sync_tokens[i]) {
                return true;
            }
        }
    }
    
    return false;
}

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
                                size_t sync_count)
{
    if (interface == NULL || sync_tokens == NULL || sync_count == 0) {
        return;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->lexer == NULL) {
        return;
    }
    
    // 跳过令牌直到找到同步令牌
    while (state->current_token != NULL && state->current_token->type != Eum_TOKEN_EOF) {
        // 检查是否为同步令牌
        bool found_sync = false;
        for (size_t i = 0; i < sync_count; i++) {
            if (state->current_token->type == sync_tokens[i]) {
                found_sync = true;
                break;
            }
        }
        
        if (found_sync) {
            break;
        }
        
        // 前进到下一个令牌
        F_advance_token(state);
    }
}

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
                       char* context_buffer, size_t buffer_size)
{
    if (interface == NULL || context_buffer == NULL || buffer_size == 0) {
        return 0;
    }
    
    // 简单实现：返回位置信息
    int written = snprintf(context_buffer, buffer_size,
                          "位置: 行 %zu, 列 %zu", line, column);
    
    return written;
}
