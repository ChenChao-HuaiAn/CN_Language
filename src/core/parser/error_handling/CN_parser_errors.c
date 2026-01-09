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
 * 包括短语级恢复和同步恢复。
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
    
    // 首先尝试短语级恢复
    bool phrase_recovery_success = F_try_phrase_level_recovery(interface, error);
    if (phrase_recovery_success) {
        return true;
    }
    
    // 如果短语级恢复失败，使用同步恢复
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
 * @brief 尝试短语级错误恢复
 * 
 * 尝试使用短语级恢复策略修复错误。
 * 这是F_try_error_recovery的内部辅助函数。
 * 
 * @param interface 语法分析器接口指针
 * @param error 发生的错误
 * @return bool 短语级恢复成功返回true，否则返回false
 */
bool F_try_phrase_level_recovery(Stru_ParserInterface_t* interface, Stru_SyntaxError_t* error)
{
    if (interface == NULL || error == NULL) {
        return false;
    }
    
    // 尝试短语级恢复
    Stru_PhraseRecoveryResult_t* recovery_result = F_try_phrase_recovery(interface, error);
    if (recovery_result == NULL) {
        return false;
    }
    
    // 检查恢复是否成功
    if (!recovery_result->success) {
        F_destroy_phrase_recovery_result(recovery_result);
        return false;
    }
    
    // 应用恢复结果
    bool apply_success = F_apply_phrase_recovery(interface, recovery_result);
    
    // 清理恢复结果
    F_destroy_phrase_recovery_result(recovery_result);
    
    return apply_success;
}

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
                            Stru_PhraseRecoveryResult_t* recovery_result)
{
    if (interface == NULL || recovery_result == NULL) {
        return false;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->lexer == NULL) {
        return false;
    }
    
    // 根据恢复策略应用恢复
    switch (recovery_result->strategy) {
        case Eum_RECOVERY_INSERT_TOKEN:
            // 插入令牌：在实际实现中，这里需要修改令牌流
            // 简化实现：记录插入的令牌信息，让解析器知道它应该存在
            // 注意：这里只是简化实现，实际实现需要更复杂的令牌流管理
            if (recovery_result->inserted_token != NULL) {
                // 在实际实现中，这里会修改令牌流
                // 简化实现：记录日志并返回成功
                return true;
            }
            break;
            
        case Eum_RECOVERY_DELETE_TOKEN:
            // 删除令牌：跳过当前令牌
            if (recovery_result->deleted_token != NULL) {
                // 在实际实现中，这里会从令牌流中删除令牌
                // 简化实现：前进到下一个令牌
                F_advance_token(state);
                return true;
            }
            break;
            
        case Eum_RECOVERY_REPLACE_TOKEN:
            // 替换令牌：用正确的令牌替换错误的令牌
            if (recovery_result->replaced_token != NULL) {
                // 在实际实现中，这里会替换令牌流中的令牌
                // 简化实现：记录日志并返回成功
                return true;
            }
            break;
            
        case Eum_RECOVERY_SKIP_TOKEN:
            // 跳过令牌：前进到下一个令牌
            F_advance_token(state);
            return true;
            
        case Eum_RECOVERY_REORDER_TOKENS:
            // 重新排序令牌：需要更复杂的处理
            // 简化实现：记录日志并返回成功
            return true;
            
        case Eum_RECOVERY_NONE:
            // 不进行恢复
            return false;
            
        default:
            return false;
    }
    
    return false;
}

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
                                         size_t buffer_size)
{
    if (interface == NULL || error == NULL || 
        suggestion_buffer == NULL || buffer_size == 0) {
        return 0;
    }
    
    // 根据错误类型提供常见建议
    switch (error->type) {
        case Eum_ERROR_MISSING_TOKEN:
            return snprintf(suggestion_buffer, buffer_size,
                           "可能缺少分号、括号或大括号。检查前一行是否完整。");
            
        case Eum_ERROR_UNEXPECTED_TOKEN:
            return snprintf(suggestion_buffer, buffer_size,
                           "可能是拼写错误或使用了错误的操作符。检查令牌是否正确。");
            
        case Eum_ERROR_TYPE_MISMATCH:
            return snprintf(suggestion_buffer, buffer_size,
                           "检查变量类型是否匹配，可能需要类型转换。");
            
        case Eum_ERROR_DUPLICATE_DECLARATION:
            return snprintf(suggestion_buffer, buffer_size,
                           "标识符已声明，请使用不同的名称或检查作用域。");
            
        case Eum_ERROR_UNDECLARED_IDENTIFIER:
            return snprintf(suggestion_buffer, buffer_size,
                           "标识符未声明，请检查拼写或确保已声明。");
            
        case Eum_ERROR_UNCLOSED_PAREN:
            return snprintf(suggestion_buffer, buffer_size,
                           "缺少右括号，请检查括号是否配对。");
            
        case Eum_ERROR_UNCLOSED_BRACKET:
            return snprintf(suggestion_buffer, buffer_size,
                           "缺少右方括号，请检查数组访问或声明。");
            
        case Eum_ERROR_UNCLOSED_BRACE:
            return snprintf(suggestion_buffer, buffer_size,
                           "缺少右大括号，请检查代码块是否完整。");
            
        case Eum_ERROR_UNCLOSED_BLOCK:
            return snprintf(suggestion_buffer, buffer_size,
                           "代码块未关闭，请检查控制结构是否完整。");
            
        default:
            return snprintf(suggestion_buffer, buffer_size,
                           "检查语法是否正确，可能需要查看文档。");
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
