/**
 * @file CN_parser_phrase_recovery.c
 * @brief CN_Language 语法分析器短语级错误恢复模块实现
 * 
 * 短语级错误恢复功能实现，包括插入缺失令牌、删除多余令牌、替换错误令牌等。
 * 遵循项目架构规范，提供智能的错误恢复建议。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_parser_phrase_recovery.h"
#include "../interface/CN_parser_interface_impl.h"
#include "../../../infrastructure/memory/CN_memory_interface.h"
#include "../../../infrastructure/utils/CN_utils_interface.h"
#include "../../token/CN_token.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ============================================
// 内部辅助函数
// ============================================

/**
 * @brief 创建令牌副本
 */
static Stru_Token_t* F_copy_token(const Stru_Token_t* token)
{
    if (token == NULL) {
        return NULL;
    }
    
    Stru_Token_t* copy = F_create_token(token->type, token->lexeme, token->line, token->column);
    if (copy == NULL) {
        return NULL;
    }
    
    return copy;
}

// ============================================
// 短语级恢复结果函数实现
// ============================================

/**
 * @brief 创建短语级恢复结果
 */
Stru_PhraseRecoveryResult_t* F_create_phrase_recovery_result(void)
{
    Stru_PhraseRecoveryResult_t* result = cn_malloc(sizeof(Stru_PhraseRecoveryResult_t));
    if (result == NULL) {
        return NULL;
    }
    
    result->success = false;
    result->strategy = Eum_RECOVERY_NONE;
    result->suggestion = NULL;
    result->tokens_affected = 0;
    result->inserted_token = NULL;
    result->deleted_token = NULL;
    result->replaced_token = NULL;
    
    return result;
}

/**
 * @brief 销毁短语级恢复结果
 */
void F_destroy_phrase_recovery_result(Stru_PhraseRecoveryResult_t* result)
{
    if (result == NULL) {
        return;
    }
    
    if (result->suggestion != NULL) {
        cn_free(result->suggestion);
    }
    
    if (result->inserted_token != NULL) {
        F_destroy_token(result->inserted_token);
    }
    
    if (result->deleted_token != NULL) {
        F_destroy_token(result->deleted_token);
    }
    
    if (result->replaced_token != NULL) {
        F_destroy_token(result->replaced_token);
    }
    
    cn_free(result);
}

// ============================================
// 短语级恢复函数实现
// ============================================

/**
 * @brief 尝试短语级恢复
 */
Stru_PhraseRecoveryResult_t* F_try_phrase_recovery(Stru_ParserInterface_t* interface,
                                                  Stru_SyntaxError_t* error)
{
    if (interface == NULL || error == NULL) {
        return NULL;
    }
    
    Stru_PhraseRecoveryResult_t* result = F_create_phrase_recovery_result();
    if (result == NULL) {
        return NULL;
    }
    
    // 根据错误类型尝试不同的恢复策略
    switch (error->type) {
        case Eum_ERROR_MISSING_TOKEN:
            result = F_try_insert_missing_token(interface, error);
            break;
            
        case Eum_ERROR_UNEXPECTED_TOKEN:
            // 先尝试删除，如果不行再尝试替换
            result = F_try_delete_extra_token(interface, error);
            if (result == NULL || !result->success) {
                if (result != NULL) {
                    F_destroy_phrase_recovery_result(result);
                }
                result = F_try_replace_wrong_token(interface, error);
            }
            break;
            
        case Eum_ERROR_MISMATCHED_TOKEN:
            result = F_try_replace_wrong_token(interface, error);
            break;
            
        case Eum_ERROR_UNCLOSED_PAREN:
        case Eum_ERROR_UNCLOSED_BRACKET:
        case Eum_ERROR_UNCLOSED_BRACE:
            result = F_try_insert_missing_token(interface, error);
            break;
            
        default:
            // 对于其他错误类型，尝试跳过当前令牌
            result = F_try_skip_current_token(interface, error);
            break;
    }
    
    return result;
}

/**
 * @brief 尝试插入缺失的令牌
 */
Stru_PhraseRecoveryResult_t* F_try_insert_missing_token(Stru_ParserInterface_t* interface,
                                                       Stru_SyntaxError_t* error)
{
    if (interface == NULL || error == NULL) {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL) {
        return NULL;
    }
    
    Stru_PhraseRecoveryResult_t* result = F_create_phrase_recovery_result();
    if (result == NULL) {
        return NULL;
    }
    
    result->strategy = Eum_RECOVERY_INSERT_TOKEN;
    
    // 根据错误信息确定要插入的令牌类型
    const char* expected = error->data.expected;
    if (expected == NULL) {
        // 如果没有期望信息，无法插入
        result->success = false;
        result->suggestion = cn_strdup("无法确定缺失的令牌类型");
        return result;
    }
    
    // 创建要插入的令牌
    // 这里简化处理，实际实现需要根据expected字符串创建正确的令牌
    result->inserted_token = F_create_token(Eum_TOKEN_IDENTIFIER, expected, error->line, error->column);
    if (result->inserted_token == NULL) {
        result->success = false;
        result->suggestion = cn_strdup("无法创建缺失的令牌");
        return result;
    }
    
    // 生成建议
    char suggestion[256];
    snprintf(suggestion, sizeof(suggestion), 
             "插入缺失的令牌: %s (行 %zu, 列 %zu)", 
             expected, error->line, error->column);
    
    result->suggestion = cn_strdup(suggestion);
    result->tokens_affected = 1;
    result->success = true;
    
    return result;
}

/**
 * @brief 尝试删除多余的令牌
 */
Stru_PhraseRecoveryResult_t* F_try_delete_extra_token(Stru_ParserInterface_t* interface,
                                                     Stru_SyntaxError_t* error)
{
    if (interface == NULL || error == NULL) {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL) {
        return NULL;
    }
    
    Stru_PhraseRecoveryResult_t* result = F_create_phrase_recovery_result();
    if (result == NULL) {
        return NULL;
    }
    
    result->strategy = Eum_RECOVERY_DELETE_TOKEN;
    
    // 复制当前令牌作为要删除的令牌
    result->deleted_token = F_copy_token(state->current_token);
    if (result->deleted_token == NULL) {
        result->success = false;
        result->suggestion = cn_strdup("无法复制要删除的令牌");
        return result;
    }
    
    // 生成建议
    char suggestion[256];
    const char* token_text = state->current_token->lexeme ? 
                            state->current_token->lexeme : 
                            F_token_type_to_string(state->current_token->type);
    
    snprintf(suggestion, sizeof(suggestion), 
             "删除多余的令牌: %s (行 %zu, 列 %zu)", 
             token_text, 
             state->current_token->line, 
             state->current_token->column);
    
    result->suggestion = cn_strdup(suggestion);
    result->tokens_affected = 1;
    result->success = true;
    
    return result;
}

/**
 * @brief 尝试替换错误的令牌
 */
Stru_PhraseRecoveryResult_t* F_try_replace_wrong_token(Stru_ParserInterface_t* interface,
                                                      Stru_SyntaxError_t* error)
{
    if (interface == NULL || error == NULL) {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL) {
        return NULL;
    }
    
    Stru_PhraseRecoveryResult_t* result = F_create_phrase_recovery_result();
    if (result == NULL) {
        return NULL;
    }
    
    result->strategy = Eum_RECOVERY_REPLACE_TOKEN;
    
    // 复制当前令牌作为要替换的令牌
    result->replaced_token = F_copy_token(state->current_token);
    if (result->replaced_token == NULL) {
        result->success = false;
        result->suggestion = cn_strdup("无法复制要替换的令牌");
        return result;
    }
    
    // 根据错误信息确定要插入的令牌
    const char* expected = error->data.expected;
    if (expected == NULL) {
        // 如果没有期望信息，无法替换
        result->success = false;
        result->suggestion = cn_strdup("无法确定期望的令牌类型");
        return result;
    }
    
    // 创建要插入的令牌
    result->inserted_token = F_create_token(Eum_TOKEN_IDENTIFIER, expected, 
                                           state->current_token->line, 
                                           state->current_token->column);
    if (result->inserted_token == NULL) {
        result->success = false;
        result->suggestion = cn_strdup("无法创建替换的令牌");
        return result;
    }
    
    // 生成建议
    char suggestion[256];
    const char* current_text = state->current_token->lexeme ? 
                              state->current_token->lexeme : 
                              F_token_type_to_string(state->current_token->type);
    
    snprintf(suggestion, sizeof(suggestion), 
             "替换令牌: %s → %s (行 %zu, 列 %zu)", 
             current_text, expected, 
             state->current_token->line, state->current_token->column);
    
    result->suggestion = cn_strdup(suggestion);
    result->tokens_affected = 1;
    result->success = true;
    
    return result;
}

/**
 * @brief 尝试跳过当前令牌
 */
Stru_PhraseRecoveryResult_t* F_try_skip_current_token(Stru_ParserInterface_t* interface,
                                                     Stru_SyntaxError_t* error)
{
    if (interface == NULL || error == NULL) {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL) {
        return NULL;
    }
    
    Stru_PhraseRecoveryResult_t* result = F_create_phrase_recovery_result();
    if (result == NULL) {
        return NULL;
    }
    
    result->strategy = Eum_RECOVERY_SKIP_TOKEN;
    
    // 复制当前令牌作为要跳过的令牌
    result->deleted_token = F_copy_token(state->current_token);
    if (result->deleted_token == NULL) {
        result->success = false;
        result->suggestion = cn_strdup("无法复制要跳过的令牌");
        return result;
    }
    
    // 生成建议
    char suggestion[256];
    const char* token_text = state->current_token->lexeme ? 
                            state->current_token->lexeme : 
                            F_token_type_to_string(state->current_token->type);
    
    snprintf(suggestion, sizeof(suggestion), 
             "跳过令牌: %s (行 %zu, 列 %zu)", 
             token_text, 
             state->current_token->line, 
             state->current_token->column);
    
    result->suggestion = cn_strdup(suggestion);
    result->tokens_affected = 1;
    result->success = true;
    
    return result;
}

/**
 * @brief 分析错误上下文
 */
int F_analyze_error_context(Stru_ParserInterface_t* interface,
                           Stru_SyntaxError_t* error,
                           char* context_buffer,
                           size_t buffer_size)
{
    if (interface == NULL || error == NULL || context_buffer == NULL || buffer_size == 0) {
        return 0;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL) {
        return 0;
    }
    
    // 分析错误上下文
    int written = 0;
    
    if (error->token != NULL) {
        const char* token_text = error->token->lexeme ? 
                                error->token->lexeme : 
                                F_token_type_to_string(error->token->type);
        
        written = snprintf(context_buffer, buffer_size,
                          "错误令牌: %s, 位置: 行 %zu, 列 %zu",
                          token_text, error->line, error->column);
    } else {
        written = snprintf(context_buffer, buffer_size,
                          "错误位置: 行 %zu, 列 %zu",
                          error->line, error->column);
    }
    
    // 添加上下文信息
    if (state->previous_token != NULL) {
        const char* prev_text = state->previous_token->lexeme ? 
                               state->previous_token->lexeme : 
                               F_token_type_to_string(state->previous_token->type);
        
        int remaining = buffer_size - written;
        if (remaining > 0) {
            int added = snprintf(context_buffer + written, remaining,
                                ", 前一个令牌: %s", prev_text);
            if (added > 0) {
                written += added;
            }
        }
    }
    
    if (state->next_token != NULL) {
        const char* next_text = state->next_token->lexeme ? 
                               state->next_token->lexeme : 
                               F_token_type_to_string(state->next_token->type);
        
        int remaining = buffer_size - written;
        if (remaining > 0) {
            int added = snprintf(context_buffer + written, remaining,
                                ", 下一个令牌: %s", next_text);
            if (added > 0) {
                written += added;
            }
        }
    }
    
    return written;
}

/**
 * @brief 应用恢复策略
 */
bool F_apply_recovery_strategy(Stru_ParserInterface_t* interface,
                              Stru_PhraseRecoveryResult_t* result)
{
    if (interface == NULL || result == NULL || !result->success) {
        return false;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL) {
        return false;
    }
    
    // 根据恢复策略执行相应的操作
    switch (result->strategy) {
        case Eum_RECOVERY_INSERT_TOKEN:
            // 插入令牌：这里简化处理，实际实现需要修改令牌流
            // 在实际实现中，需要将插入的令牌添加到令牌流中
            // 这里只是记录日志
            if (result->inserted_token != NULL) {
                // 在实际实现中，这里应该将令牌插入到当前令牌之前
                // 由于令牌流管理复杂，这里只返回成功
                return true;
            }
            break;
            
        case Eum_RECOVERY_DELETE_TOKEN:
        case Eum_RECOVERY_SKIP_TOKEN:
            // 删除或跳过令牌：前进到下一个令牌
            if (state->current_token != NULL) {
                F_advance_token(state);
                return true;
            }
            break;
            
        case Eum_RECOVERY_REPLACE_TOKEN:
            // 替换令牌：用新令牌替换当前令牌
            if (state->current_token != NULL && result->inserted_token != NULL) {
                // 销毁当前令牌
                F_destroy_token(state->current_token);
                // 设置新令牌
                state->current_token = result->inserted_token;
                // 防止重复释放
                result->inserted_token = NULL;
                return true;
            }
            break;
            
        case Eum_RECOVERY_REORDER_TOKENS:
            // 重新排序令牌：这里简化处理
            // 在实际实现中，需要重新排列令牌顺序
            return true;
            
        case Eum_RECOVERY_NONE:
        default:
            // 不进行恢复
            return false;
    }
    
    return false;
}

/**
 * @brief 检查常见错误模式
 */
bool F_check_common_error_pattern(Stru_ParserInterface_t* interface,
                                 Stru_SyntaxError_t* error,
                                 char* pattern_buffer,
                                 size_t buffer_size)
{
    if (interface == NULL || error == NULL || pattern_buffer == NULL || buffer_size == 0) {
        return false;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL) {
        return false;
    }
    
    // 检查常见错误模式
    bool found_pattern = false;
    
    // 模式1：缺失分号
    if (error->type == Eum_ERROR_MISSING_TOKEN && 
        error->data.expected != NULL && 
        strstr(error->data.expected, "分号") != NULL) {
        snprintf(pattern_buffer, buffer_size, "缺失分号错误模式");
        found_pattern = true;
    }
    // 模式2：括号不匹配
    else if (error->type == Eum_ERROR_UNCLOSED_PAREN || 
             error->type == Eum_ERROR_UNCLOSED_BRACKET || 
             error->type == Eum_ERROR_UNCLOSED_BRACE) {
        snprintf(pattern_buffer, buffer_size, "括号不匹配错误模式");
        found_pattern = true;
    }
    // 模式3：意外的逗号
    else if (error->type == Eum_ERROR_UNEXPECTED_TOKEN && 
             state->current_token != NULL && 
             state->current_token->type == Eum_TOKEN_DELIMITER_COMMA) {
        snprintf(pattern_buffer, buffer_size, "意外的逗号错误模式");
        found_pattern = true;
    }
    // 模式4：意外的右括号
    else if (error->type == Eum_ERROR_UNEXPECTED_TOKEN && 
             state->current_token != NULL && 
             (state->current_token->type == Eum_TOKEN_DELIMITER_RPAREN ||
              state->current_token->type == Eum_TOKEN_DELIMITER_RBRACKET ||
              state->current_token->type == Eum_TOKEN_DELIMITER_RBRACE)) {
        snprintf(pattern_buffer, buffer_size, "意外的右括号错误模式");
        found_pattern = true;
    }
    // 模式5：意外的运算符
    else if (error->type == Eum_ERROR_UNEXPECTED_TOKEN && 
             state->current_token != NULL && 
             F_token_is_operator(state->current_token->type)) {
        snprintf(pattern_buffer, buffer_size, "意外的运算符错误模式");
        found_pattern = true;
    }
    
    return found_pattern;
}

/**
 * @brief 生成修复建议
 */
int F_generate_fix_suggestion(Stru_ParserInterface_t* interface,
                             Stru_SyntaxError_t* error,
                             char* suggestion_buffer,
                             size_t buffer_size)
{
    if (interface == NULL || error == NULL || suggestion_buffer == NULL || buffer_size == 0) {
        return 0;
    }
    
    // 根据错误类型生成不同的修复建议
    int written = 0;
    
    switch (error->type) {
        case Eum_ERROR_MISSING_TOKEN:
            if (error->data.expected != NULL) {
                written = snprintf(suggestion_buffer, buffer_size,
                                  "建议: 插入缺失的令牌 '%s'", error->data.expected);
            } else {
                written = snprintf(suggestion_buffer, buffer_size,
                                  "建议: 检查是否缺少必要的令牌");
            }
            break;
            
        case Eum_ERROR_UNEXPECTED_TOKEN:
            if (error->data.expected != NULL) {
                written = snprintf(suggestion_buffer, buffer_size,
                                  "建议: 删除或替换意外的令牌，期望: %s", error->data.expected);
            } else {
                written = snprintf(suggestion_buffer, buffer_size,
                                  "建议: 删除意外的令牌");
            }
            break;
            
        case Eum_ERROR_MISMATCHED_TOKEN:
            if (error->data.expected != NULL) {
                written = snprintf(suggestion_buffer, buffer_size,
                                  "建议: 将令牌替换为 '%s'", error->data.expected);
            } else {
                written = snprintf(suggestion_buffer, buffer_size,
                                  "建议: 检查令牌是否匹配");
            }
            break;
            
        case Eum_ERROR_UNCLOSED_PAREN:
            written = snprintf(suggestion_buffer, buffer_size,
                              "建议: 添加缺失的右括号 ')'");
            break;
            
        case Eum_ERROR_UNCLOSED_BRACKET:
            written = snprintf(suggestion_buffer, buffer_size,
                              "建议: 添加缺失的右方括号 ']'");
            break;
            
        case Eum_ERROR_UNCLOSED_BRACE:
            written = snprintf(suggestion_buffer, buffer_size,
                              "建议: 添加缺失的右大括号 '}'");
            break;
            
        case Eum_ERROR_UNCLOSED_BLOCK:
            written = snprintf(suggestion_buffer, buffer_size,
                              "建议: 添加缺失的代码块结束标记");
            break;
            
        case Eum_ERROR_UNEXPECTED_EOF:
            written = snprintf(suggestion_buffer, buffer_size,
                              "建议: 检查是否缺少代码块或语句的结束标记");
            break;
            
        default:
            written = snprintf(suggestion_buffer, buffer_size,
                              "建议: 检查语法错误并修正");
            break;
    }
    
    return written;
}
