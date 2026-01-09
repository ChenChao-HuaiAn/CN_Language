/**
 * @file CN_parser_primary_expressions.c
 * @brief CN_Language 基本表达式解析模块
 * 
 * 基本表达式解析模块，负责解析基本表达式。
 * 包括字面量表达式、标识符表达式、括号表达式等。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_parser_expressions.h"
#include "../interface/CN_parser_interface_impl.h"
#include "../error_handling/CN_parser_errors.h"
#include "../utils/CN_parser_utils.h"
#include "../../../infrastructure/containers/array/CN_dynamic_array.h"
#include "../../token/CN_token_types.h"
#include "../../ast/CN_ast.h"
#include <stdlib.h>
#include <string.h>

// ============================================
// 基本表达式解析函数实现
// ============================================

/**
 * @brief 解析基本表达式
 */
Stru_AstNode_t* F_parse_primary_expression(Stru_ParserInterface_t* interface)
{
    if (interface == NULL)
    {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL)
    {
        return NULL;
    }
    
    // 根据当前令牌类型决定解析什么
    switch (state->current_token->type)
    {
        case Eum_TOKEN_LITERAL_INTEGER:
        case Eum_TOKEN_LITERAL_FLOAT:
        case Eum_TOKEN_LITERAL_STRING:
        case Eum_TOKEN_LITERAL_BOOLEAN:
        case Eum_TOKEN_KEYWORD_TRUE:
        case Eum_TOKEN_KEYWORD_FALSE:
            // 字面量表达式
            return F_parse_literal_expression(interface);
            
        case Eum_TOKEN_IDENTIFIER:
            // 可能是标识符表达式或函数调用
            return F_parse_function_call_expression(interface);
            
        case Eum_TOKEN_DELIMITER_LPAREN:
            // 括号表达式
            return F_parse_parenthesized_expression(interface);
            
        default:
            // 报告错误
            F_report_unexpected_token_error(interface, state->current_token, "表达式");
            return NULL;
    }
}

/**
 * @brief 解析字面量表达式
 */
Stru_AstNode_t* F_parse_literal_expression(Stru_ParserInterface_t* interface)
{
    if (interface == NULL)
    {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL)
    {
        return NULL;
    }
    
    // 检查当前令牌是否为字面量
    if (!F_is_literal_token(state->current_token->type))
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "字面量");
        return NULL;
    }
    
    // 创建字面量节点
    Stru_AstNode_t* node = F_create_literal_node(interface, state->current_token);
    
    // 前进到下一个令牌
    F_advance_token(state);
    
    return node;
}

/**
 * @brief 解析标识符表达式
 */
Stru_AstNode_t* F_parse_identifier_expression(Stru_ParserInterface_t* interface)
{
    if (interface == NULL)
    {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL)
    {
        return NULL;
    }
    
    // 检查当前令牌是否为标识符
    if (state->current_token->type != Eum_TOKEN_IDENTIFIER)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "标识符");
        return NULL;
    }
    
    // 创建标识符节点
    Stru_AstNode_t* node = F_create_identifier_node(interface, state->current_token);
    
    // 前进到下一个令牌
    F_advance_token(state);
    
    return node;
}

/**
 * @brief 解析括号表达式
 */
Stru_AstNode_t* F_parse_parenthesized_expression(Stru_ParserInterface_t* interface)
{
    if (interface == NULL)
    {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL)
    {
        return NULL;
    }
    
    // 检查是否为左括号
    if (state->current_token->type != Eum_TOKEN_DELIMITER_LPAREN)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "(");
        return NULL;
    }
    
    // 消耗左括号
    F_advance_token(state);
    
    // 解析括号内的表达式
    Stru_AstNode_t* expression = F_parse_expression(interface);
    if (expression == NULL)
    {
        // 报告错误
        F_report_invalid_expression_error(interface, 
                                         state->current_token->line,
                                         state->current_token->column,
                                         "括号内表达式无效",
                                         state->current_token);
        return NULL;
    }
    
    // 检查右括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_RPAREN)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    ")");
        return NULL;
    }
    
    // 消耗右括号
    F_advance_token(state);
    
    return expression;
}
