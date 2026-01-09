/**
 * @file CN_parser_unary_expressions.c
 * @brief CN_Language 一元表达式解析模块
 * 
 * 一元表达式解析模块，负责解析一元表达式。
 * 包括负号表达式、逻辑非表达式等。
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
// 内部辅助函数声明
// ============================================

/**
 * @brief 解析一元表达式（内部实现）
 */
static Stru_AstNode_t* parse_unary_expression_impl(Stru_ParserInterface_t* interface);

/**
 * @brief 解析基本表达式（内部实现）
 */
static Stru_AstNode_t* parse_primary_expression_impl(Stru_ParserInterface_t* interface);

// ============================================
// 一元表达式解析函数实现
// ============================================

/**
 * @brief 解析一元表达式
 */
Stru_AstNode_t* F_parse_unary_expression(Stru_ParserInterface_t* interface)
{
    return parse_unary_expression_impl(interface);
}

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 解析一元表达式（内部实现）
 */
static Stru_AstNode_t* parse_unary_expression_impl(Stru_ParserInterface_t* interface)
{
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL)
    {
        return NULL;
    }
    
    // 检查是否为一元运算符
    if (state->current_token->type == Eum_TOKEN_KEYWORD_NOT ||
        state->current_token->type == Eum_TOKEN_OPERATOR_MINUS ||
        state->current_token->type == Eum_TOKEN_OPERATOR_PLUS)
    {
        // 保存运算符令牌
        Stru_Token_t* operator_token = state->current_token;
        
        // 消耗运算符
        F_advance_token(state);
        
        // 解析操作数
        Stru_AstNode_t* operand = parse_unary_expression_impl(interface);
        if (operand == NULL)
        {
            return NULL;
        }
        
        // 创建一元表达式节点
        Stru_AstNode_t* unary_node = F_create_unary_expression_node(interface,
                                                                   operator_token,
                                                                   operand);
        return unary_node;
    }
    
    // 不是一元运算符，解析基本表达式
    return parse_primary_expression_impl(interface);
}

/**
 * @brief 解析基本表达式（内部实现）
 */
static Stru_AstNode_t* parse_primary_expression_impl(Stru_ParserInterface_t* interface)
{
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
