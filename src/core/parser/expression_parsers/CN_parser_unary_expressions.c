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
#include "../declaration_parsers/CN_parser_declarations.h"
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

/**
 * @brief 解析类型转换表达式（内部实现）
 */
static Stru_AstNode_t* parse_cast_expression_impl(Stru_ParserInterface_t* interface);

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
    
    // 首先尝试解析类型转换表达式
    Stru_AstNode_t* cast_expr = parse_cast_expression_impl(interface);
    if (cast_expr != NULL)
    {
        return cast_expr;
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
    
    Stru_AstNode_t* primary_expr = NULL;
    
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
            primary_expr = F_parse_literal_expression(interface);
            break;
            
        case Eum_TOKEN_IDENTIFIER:
            // 可能是标识符表达式或函数调用
            primary_expr = F_parse_function_call_expression(interface);
            break;
            
        case Eum_TOKEN_DELIMITER_LPAREN:
            // 括号表达式
            primary_expr = F_parse_parenthesized_expression(interface);
            break;
            
        default:
            // 报告错误
            F_report_unexpected_token_error(interface, state->current_token, "表达式");
            return NULL;
    }
    
    // 如果成功解析了基本表达式，尝试解析后缀表达式
    if (primary_expr != NULL)
    {
        primary_expr = F_parse_postfix_expression(interface, primary_expr);
    }
    
    return primary_expr;
}

/**
 * @brief 解析类型转换表达式（内部实现）
 */
static Stru_AstNode_t* parse_cast_expression_impl(Stru_ParserInterface_t* interface)
{
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL)
    {
        return NULL;
    }
    
    // 检查是否为左括号
    if (state->current_token->type != Eum_TOKEN_DELIMITER_LPAREN)
    {
        return NULL;
    }
    
    // 保存当前位置，以便在不是类型转换时回退
    Stru_Token_t* saved_token = state->current_token;
    size_t saved_position = 0; // 注意：这里需要保存词法分析器的位置，但当前实现没有提供这个功能
    // 在实际实现中，需要保存词法分析器的状态以便回退
    
    // 消耗左括号
    F_advance_token(state);
    
    // 检查下一个令牌是否为类型关键字
    if (state->current_token == NULL || !F_is_type_keyword(state->current_token->type))
    {
        // 不是类型关键字，回退到左括号位置
        // 注意：由于没有回退机制，我们只能返回NULL，让调用者尝试其他解析
        // 在实际实现中，应该回退到保存的位置
        return NULL;
    }
    
    // 尝试解析类型表达式
    Stru_AstNode_t* type_node = F_parse_type_expression(interface);
    if (type_node == NULL)
    {
        return NULL;
    }
    
    // 检查是否有右括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_RPAREN)
    {
        // 报告错误：期望右括号
        F_report_unexpected_token_error(interface, state->current_token, "右括号");
        F_destroy_ast_node(type_node);
        return NULL;
    }
    
    // 消耗右括号
    F_advance_token(state);
    
    // 解析要转换的表达式
    Stru_AstNode_t* operand = parse_unary_expression_impl(interface);
    if (operand == NULL)
    {
        F_destroy_ast_node(type_node);
        return NULL;
    }
    
    // 创建类型转换表达式节点
    Stru_AstNode_t* cast_node = F_create_cast_expression_node(interface, saved_token, type_node, operand);
    if (cast_node == NULL)
    {
        F_destroy_ast_node(type_node);
        F_destroy_ast_node(operand);
        return NULL;
    }
    
    return cast_node;
}
