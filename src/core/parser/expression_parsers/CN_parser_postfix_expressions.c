/**
 * @file CN_parser_postfix_expressions.c
 * @brief CN_Language 后缀表达式解析模块
 * 
 * 后缀表达式解析模块，负责解析后缀表达式。
 * 包括成员访问表达式、数组索引表达式等。
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
 * @brief 解析成员访问表达式（内部实现）
 */
static Stru_AstNode_t* parse_member_access_expression_impl(Stru_ParserInterface_t* interface,
                                                          Stru_AstNode_t* base);

/**
 * @brief 解析数组索引表达式（内部实现）
 */
static Stru_AstNode_t* parse_array_index_expression_impl(Stru_ParserInterface_t* interface,
                                                        Stru_AstNode_t* base);

// ============================================
// 后缀表达式解析函数实现
// ============================================

/**
 * @brief 解析后缀表达式
 * 
 * 解析后缀表达式，包括成员访问和数组索引。
 * 
 * @param interface 语法分析器接口指针
 * @param base 基础表达式节点
 * @return Stru_AstNode_t* 后缀表达式AST节点
 */
Stru_AstNode_t* F_parse_postfix_expression(Stru_ParserInterface_t* interface,
                                          Stru_AstNode_t* base)
{
    if (interface == NULL || base == NULL)
    {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL)
    {
        return base;
    }
    
    Stru_AstNode_t* result = base;
    
    // 循环处理所有后缀运算符
    while (state->current_token != NULL)
    {
        // 检查是否为成员访问运算符（.）
        if (state->current_token->type == Eum_TOKEN_DELIMITER_DOT)
        {
            result = parse_member_access_expression_impl(interface, result);
            if (result == NULL)
            {
                return NULL;
            }
        }
        // 检查是否为数组索引运算符（[）
        else if (state->current_token->type == Eum_TOKEN_DELIMITER_LBRACKET)
        {
            result = parse_array_index_expression_impl(interface, result);
            if (result == NULL)
            {
                return NULL;
            }
        }
        // 检查是否为函数调用运算符（(）
        else if (state->current_token->type == Eum_TOKEN_DELIMITER_LPAREN)
        {
            // 函数调用已经在其他模块中处理
            break;
        }
        else
        {
            // 不是后缀运算符，结束循环
            break;
        }
    }
    
    return result;
}

/**
 * @brief 解析成员访问表达式（内部实现）
 */
static Stru_AstNode_t* parse_member_access_expression_impl(Stru_ParserInterface_t* interface,
                                                          Stru_AstNode_t* base)
{
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL || base == NULL)
    {
        return NULL;
    }
    
    // 检查是否为成员访问运算符（.）
    if (state->current_token->type != Eum_TOKEN_DELIMITER_DOT)
    {
        return base;
    }
    
    // 保存点运算符令牌
    Stru_Token_t* dot_token = state->current_token;
    
    // 消耗点运算符
    F_advance_token(state);
    
    // 检查是否为标识符（成员名）
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_IDENTIFIER)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "标识符（成员名）");
        return NULL;
    }
    
    // 保存成员名令牌
    Stru_Token_t* member_token = state->current_token;
    
    // 消耗成员名
    F_advance_token(state);
    
    // 创建成员访问表达式节点
    Stru_AstNode_t* member_access_node = F_create_member_access_node(interface,
                                                                    dot_token,
                                                                    base,
                                                                    member_token);
    
    return member_access_node;
}

/**
 * @brief 解析数组索引表达式（内部实现）
 */
static Stru_AstNode_t* parse_array_index_expression_impl(Stru_ParserInterface_t* interface,
                                                        Stru_AstNode_t* base)
{
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL || base == NULL)
    {
        return NULL;
    }
    
    // 检查是否为左方括号
    if (state->current_token->type != Eum_TOKEN_DELIMITER_LBRACKET)
    {
        return base;
    }
    
    // 保存左方括号令牌
    Stru_Token_t* lbracket_token = state->current_token;
    
    // 消耗左方括号
    F_advance_token(state);
    
    // 解析索引表达式
    Stru_AstNode_t* index_expression = F_parse_expression(interface);
    if (index_expression == NULL)
    {
        // 报告错误
        F_report_invalid_expression_error(interface,
                                         state->current_token->line,
                                         state->current_token->column,
                                         "数组索引表达式无效",
                                         state->current_token);
        return NULL;
    }
    
    // 检查右方括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_RBRACKET)
    {
        // 报告错误
        F_report_missing_token_error(interface,
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    "]");
        return NULL;
    }
    
    // 保存右方括号令牌
    Stru_Token_t* rbracket_token = state->current_token;
    
    // 消耗右方括号
    F_advance_token(state);
    
    // 创建数组索引表达式节点
    Stru_AstNode_t* array_index_node = F_create_array_index_node(interface,
                                                                lbracket_token,
                                                                base,
                                                                index_expression,
                                                                rbracket_token);
    
    return array_index_node;
}

/**
 * @brief 解析条件表达式（三元运算符）
 */
Stru_AstNode_t* F_parse_conditional_expression(Stru_ParserInterface_t* interface)
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
    
    // 解析条件表达式
    Stru_AstNode_t* condition = F_parse_logical_or_expression(interface);
    if (condition == NULL)
    {
        return NULL;
    }
    
    // 检查是否为问号运算符（?）
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_OPERATOR_QUESTION)
    {
        // 不是条件表达式，返回条件节点
        return condition;
    }
    
    // 保存问号运算符令牌
    Stru_Token_t* question_token = state->current_token;
    
    // 消耗问号运算符
    F_advance_token(state);
    
    // 解析真值表达式
    Stru_AstNode_t* true_expression = F_parse_expression(interface);
    if (true_expression == NULL)
    {
        // 报告错误
        F_report_invalid_expression_error(interface,
                                         state->current_token->line,
                                         state->current_token->column,
                                         "条件表达式的真值部分无效",
                                         state->current_token);
        F_destroy_ast_node(condition);
        return NULL;
    }
    
    // 检查冒号运算符（:）
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_COLON)
    {
        // 报告错误
        F_report_missing_token_error(interface,
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    ":");
        F_destroy_ast_node(condition);
        F_destroy_ast_node(true_expression);
        return NULL;
    }
    
    // 保存冒号运算符令牌
    Stru_Token_t* colon_token = state->current_token;
    
    // 消耗冒号运算符
    F_advance_token(state);
    
    // 解析假值表达式
    Stru_AstNode_t* false_expression = F_parse_expression(interface);
    if (false_expression == NULL)
    {
        // 报告错误
        F_report_invalid_expression_error(interface,
                                         state->current_token->line,
                                         state->current_token->column,
                                         "条件表达式的假值部分无效",
                                         state->current_token);
        F_destroy_ast_node(condition);
        F_destroy_ast_node(true_expression);
        return NULL;
    }
    
    // 创建条件表达式节点
    Stru_AstNode_t* conditional_node = F_create_conditional_expression_node(interface,
                                                                           question_token,
                                                                           condition,
                                                                           true_expression,
                                                                           false_expression,
                                                                           colon_token);
    
    return conditional_node;
}
