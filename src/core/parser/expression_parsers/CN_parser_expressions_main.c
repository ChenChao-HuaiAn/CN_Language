/**
 * @file CN_parser_expressions_main.c
 * @brief CN_Language 表达式解析主模块
 * 
 * 表达式解析主模块，负责协调各种表达式类型的解析。
 * 包括表达式解析入口函数和主要表达式类型分发。
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
 * @brief 解析赋值表达式（内部实现）
 */
static Stru_AstNode_t* parse_assignment_expression_impl(Stru_ParserInterface_t* interface);

/**
 * @brief 解析逻辑或表达式（内部实现）
 */
static Stru_AstNode_t* parse_logical_or_expression_impl(Stru_ParserInterface_t* interface);

/**
 * @brief 解析逻辑与表达式（内部实现）
 */
static Stru_AstNode_t* parse_logical_and_expression_impl(Stru_ParserInterface_t* interface);

/**
 * @brief 解析相等性表达式（内部实现）
 */
static Stru_AstNode_t* parse_equality_expression_impl(Stru_ParserInterface_t* interface);

/**
 * @brief 解析关系表达式（内部实现）
 */
static Stru_AstNode_t* parse_relational_expression_impl(Stru_ParserInterface_t* interface);

/**
 * @brief 解析加法表达式（内部实现）
 */
static Stru_AstNode_t* parse_additive_expression_impl(Stru_ParserInterface_t* interface);

/**
 * @brief 解析乘法表达式（内部实现）
 */
static Stru_AstNode_t* parse_multiplicative_expression_impl(Stru_ParserInterface_t* interface);

/**
 * @brief 解析一元表达式（内部实现）
 */
static Stru_AstNode_t* parse_unary_expression_impl(Stru_ParserInterface_t* interface);

/**
 * @brief 解析基本表达式（内部实现）
 */
static Stru_AstNode_t* parse_primary_expression_impl(Stru_ParserInterface_t* interface);

// ============================================
// 表达式解析函数实现
// ============================================

/**
 * @brief 解析表达式
 */
Stru_AstNode_t* F_parse_expression(Stru_ParserInterface_t* interface)
{
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 从赋值表达式开始解析（最低优先级）
    return parse_assignment_expression_impl(interface);
}

/**
 * @brief 解析赋值表达式
 */
Stru_AstNode_t* F_parse_assignment_expression(Stru_ParserInterface_t* interface)
{
    return parse_assignment_expression_impl(interface);
}

/**
 * @brief 解析逻辑或表达式
 */
Stru_AstNode_t* F_parse_logical_or_expression(Stru_ParserInterface_t* interface)
{
    return parse_logical_or_expression_impl(interface);
}

/**
 * @brief 解析逻辑与表达式
 */
Stru_AstNode_t* F_parse_logical_and_expression(Stru_ParserInterface_t* interface)
{
    return parse_logical_and_expression_impl(interface);
}

/**
 * @brief 解析相等性表达式
 */
Stru_AstNode_t* F_parse_equality_expression(Stru_ParserInterface_t* interface)
{
    return parse_equality_expression_impl(interface);
}

/**
 * @brief 解析关系表达式
 */
Stru_AstNode_t* F_parse_relational_expression(Stru_ParserInterface_t* interface)
{
    return parse_relational_expression_impl(interface);
}

/**
 * @brief 解析加法表达式
 */
Stru_AstNode_t* F_parse_additive_expression(Stru_ParserInterface_t* interface)
{
    return parse_additive_expression_impl(interface);
}

/**
 * @brief 解析乘法表达式
 */
Stru_AstNode_t* F_parse_multiplicative_expression(Stru_ParserInterface_t* interface)
{
    return parse_multiplicative_expression_impl(interface);
}

/**
 * @brief 解析一元表达式
 */
Stru_AstNode_t* F_parse_unary_expression(Stru_ParserInterface_t* interface)
{
    return parse_unary_expression_impl(interface);
}

/**
 * @brief 解析基本表达式
 */
Stru_AstNode_t* F_parse_primary_expression(Stru_ParserInterface_t* interface)
{
    return parse_primary_expression_impl(interface);
}

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 解析赋值表达式（内部实现）
 */
static Stru_AstNode_t* parse_assignment_expression_impl(Stru_ParserInterface_t* interface)
{
    // 解析逻辑或表达式（更高优先级）
    Stru_AstNode_t* left = parse_logical_or_expression_impl(interface);
    if (left == NULL)
    {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL)
    {
        return left;
    }
    
    // 检查是否为赋值运算符
    if (state->current_token->type == Eum_TOKEN_OPERATOR_ASSIGN)
    {
        // 保存运算符令牌
        Stru_Token_t* operator_token = state->current_token;
        
        // 消耗赋值运算符
        F_advance_token(state);
        
        // 解析右侧表达式
        Stru_AstNode_t* right = parse_assignment_expression_impl(interface);
        if (right == NULL)
        {
            // 清理左侧节点
            F_destroy_ast_node(left);
            return NULL;
        }
        
        // 创建赋值表达式节点
        Stru_AstNode_t* assignment_node = F_create_binary_expression_node(interface,
                                                                         operator_token,
                                                                         left,
                                                                         right);
        return assignment_node;
    }
    
    return left;
}

/**
 * @brief 解析逻辑或表达式（内部实现）
 */
static Stru_AstNode_t* parse_logical_or_expression_impl(Stru_ParserInterface_t* interface)
{
    // 解析逻辑与表达式（更高优先级）
    Stru_AstNode_t* left = parse_logical_and_expression_impl(interface);
    if (left == NULL)
    {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL)
    {
        return left;
    }
    
    // 检查是否为逻辑或运算符
    while (state->current_token != NULL && state->current_token->type == Eum_TOKEN_KEYWORD_OR)
    {
        // 保存运算符令牌
        Stru_Token_t* operator_token = state->current_token;
        
        // 消耗运算符
        F_advance_token(state);
        
        // 解析右侧表达式
        Stru_AstNode_t* right = parse_logical_and_expression_impl(interface);
        if (right == NULL)
        {
            // 清理左侧节点
            F_destroy_ast_node(left);
            return NULL;
        }
        
        // 创建逻辑或表达式节点
        Stru_AstNode_t* logical_or_node = F_create_binary_expression_node(interface,
                                                                         operator_token,
                                                                         left,
                                                                         right);
        left = logical_or_node;
    }
    
    return left;
}

/**
 * @brief 解析逻辑与表达式（内部实现）
 */
static Stru_AstNode_t* parse_logical_and_expression_impl(Stru_ParserInterface_t* interface)
{
    // 解析相等性表达式（更高优先级）
    Stru_AstNode_t* left = parse_equality_expression_impl(interface);
    if (left == NULL)
    {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL)
    {
        return left;
    }
    
    // 检查是否为逻辑与运算符
    while (state->current_token != NULL && state->current_token->type == Eum_TOKEN_KEYWORD_AND)
    {
        // 保存运算符令牌
        Stru_Token_t* operator_token = state->current_token;
        
        // 消耗运算符
        F_advance_token(state);
        
        // 解析右侧表达式
        Stru_AstNode_t* right = parse_equality_expression_impl(interface);
        if (right == NULL)
        {
            // 清理左侧节点
            F_destroy_ast_node(left);
            return NULL;
        }
        
        // 创建逻辑与表达式节点
        Stru_AstNode_t* logical_and_node = F_create_binary_expression_node(interface,
                                                                          operator_token,
                                                                          left,
                                                                          right);
        left = logical_and_node;
    }
    
    return left;
}

/**
 * @brief 解析相等性表达式（内部实现）
 */
static Stru_AstNode_t* parse_equality_expression_impl(Stru_ParserInterface_t* interface)
{
    // 解析关系表达式（更高优先级）
    Stru_AstNode_t* left = parse_relational_expression_impl(interface);
    if (left == NULL)
    {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL)
    {
        return left;
    }
    
    // 检查是否为相等性运算符
    while (state->current_token != NULL && 
           (state->current_token->type == Eum_TOKEN_OPERATOR_EQUAL ||
            state->current_token->type == Eum_TOKEN_OPERATOR_NOT_EQUAL))
    {
        // 保存运算符令牌
        Stru_Token_t* operator_token = state->current_token;
        
        // 消耗运算符
        F_advance_token(state);
        
        // 解析右侧表达式
        Stru_AstNode_t* right = parse_relational_expression_impl(interface);
        if (right == NULL)
        {
            // 清理左侧节点
            F_destroy_ast_node(left);
            return NULL;
        }
        
        // 创建相等性表达式节点
        Stru_AstNode_t* equality_node = F_create_binary_expression_node(interface,
                                                                       operator_token,
                                                                       left,
                                                                       right);
        left = equality_node;
    }
    
    return left;
}

/**
 * @brief 解析关系表达式（内部实现）
 */
static Stru_AstNode_t* parse_relational_expression_impl(Stru_ParserInterface_t* interface)
{
    // 解析加法表达式（更高优先级）
    Stru_AstNode_t* left = parse_additive_expression_impl(interface);
    if (left == NULL)
    {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL)
    {
        return left;
    }
    
    // 检查是否为关系运算符
    while (state->current_token != NULL && 
           (state->current_token->type == Eum_TOKEN_OPERATOR_LESS ||
            state->current_token->type == Eum_TOKEN_OPERATOR_LESS_EQUAL ||
            state->current_token->type == Eum_TOKEN_OPERATOR_GREATER ||
            state->current_token->type == Eum_TOKEN_OPERATOR_GREATER_EQUAL))
    {
        // 保存运算符令牌
        Stru_Token_t* operator_token = state->current_token;
        
        // 消耗运算符
        F_advance_token(state);
        
        // 解析右侧表达式
        Stru_AstNode_t* right = parse_additive_expression_impl(interface);
        if (right == NULL)
        {
            // 清理左侧节点
            F_destroy_ast_node(left);
            return NULL;
        }
        
        // 创建关系表达式节点
        Stru_AstNode_t* relational_node = F_create_binary_expression_node(interface,
                                                                         operator_token,
                                                                         left,
                                                                         right);
        left = relational_node;
    }
    
    return left;
}

/**
 * @brief 解析加法表达式（内部实现）
 */
static Stru_AstNode_t* parse_additive_expression_impl(Stru_ParserInterface_t* interface)
{
    // 解析乘法表达式（更高优先级）
    Stru_AstNode_t* left = parse_multiplicative_expression_impl(interface);
    if (left == NULL)
    {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL)
    {
        return left;
    }
    
    // 检查是否为加法运算符
    while (state->current_token != NULL && 
           (state->current_token->type == Eum_TOKEN_OPERATOR_PLUS ||
            state->current_token->type == Eum_TOKEN_OPERATOR_MINUS))
    {
        // 保存运算符令牌
        Stru_Token_t* operator_token = state->current_token;
        
        // 消耗运算符
        F_advance_token(state);
        
        // 解析右侧表达式
        Stru_AstNode_t* right = parse_multiplicative_expression_impl(interface);
        if (right == NULL)
        {
            // 清理左侧节点
            F_destroy_ast_node(left);
            return NULL;
        }
        
        // 创建加法表达式节点
        Stru_AstNode_t* additive_node = F_create_binary_expression_node(interface,
                                                                       operator_token,
                                                                       left,
                                                                       right);
        left = additive_node;
    }
    
    return left;
}

/**
 * @brief 解析乘法表达式（内部实现）
 */
static Stru_AstNode_t* parse_multiplicative_expression_impl(Stru_ParserInterface_t* interface)
{
    // 解析一元表达式（更高优先级）
    Stru_AstNode_t* left = parse_unary_expression_impl(interface);
    if (left == NULL)
    {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL)
    {
        return left;
    }
    
    // 检查是否为乘法运算符
    while (state->current_token != NULL && 
           (state->current_token->type == Eum_TOKEN_OPERATOR_MULTIPLY ||
            state->current_token->type == Eum_TOKEN_OPERATOR_DIVIDE ||
            state->current_token->type == Eum_TOKEN_OPERATOR_MODULO))
    {
        // 保存运算符令牌
        Stru_Token_t* operator_token = state->current_token;
        
        // 消耗运算符
        F_advance_token(state);
        
        // 解析右侧表达式
        Stru_AstNode_t* right = parse_unary_expression_impl(interface);
        if (right == NULL)
        {
            // 清理左侧节点
            F_destroy_ast_node(left);
            return NULL;
        }
        
        // 创建乘法表达式节点
        Stru_AstNode_t* multiplicative_node = F_create_binary_expression_node(interface,
                                                                             operator_token,
                                                                             left,
                                                                             right);
        left = multiplicative_node;
    }
    
    return left;
}
