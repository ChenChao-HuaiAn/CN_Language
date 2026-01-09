/**
 * @file CN_parser_abstract_declarations.c
 * @brief CN_Language 抽象函数声明解析模块
 * 
 * 抽象函数声明解析模块，负责解析抽象函数声明。
 * 抽象函数声明语法：抽象 函数 名称(参数);
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_parser_declarations.h"
#include "../interface/CN_parser_interface_impl.h"
#include "../error_handling/CN_parser_errors.h"
#include "../utils/CN_parser_utils.h"
#include "../../../infrastructure/containers/array/CN_dynamic_array.h"
#include "../../token/CN_token_types.h"
#include "../../ast/CN_ast.h"
#include <stdlib.h>

// ============================================
// 内部辅助函数声明
// ============================================

/**
 * @brief 解析抽象函数声明（内部实现）
 */
static Stru_AstNode_t* parse_abstract_declaration_impl(Stru_ParserInterface_t* interface);

// ============================================
// 抽象函数声明解析函数实现
// ============================================

/**
 * @brief 解析抽象函数声明
 * 
 * 解析抽象函数声明，如：抽象 函数 名称(参数);。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 抽象函数声明AST节点
 */
Stru_AstNode_t* F_parse_abstract_declaration(Stru_ParserInterface_t* interface)
{
    return parse_abstract_declaration_impl(interface);
}

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 解析抽象函数声明（内部实现）
 */
static Stru_AstNode_t* parse_abstract_declaration_impl(Stru_ParserInterface_t* interface)
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
    
    // 检查抽象关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_ABSTRACT)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "抽象");
        return NULL;
    }
    
    // 保存抽象关键字令牌
    Stru_Token_t* abstract_token = state->current_token;
    
    // 消耗抽象关键字
    F_advance_token(state);
    
    // 检查下一个令牌是否为函数关键字
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_KEYWORD_FUNCTION)
    {
        // 报告错误：抽象只能修饰函数
        F_report_unexpected_token_error(interface, state->current_token, "函数");
        return NULL;
    }
    
    // 保存函数关键字令牌
    Stru_Token_t* function_token = state->current_token;
    
    // 消耗函数关键字
    F_advance_token(state);
    
    // 检查函数名称
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_IDENTIFIER)
    {
        // 报告错误：缺少函数名称
        F_report_missing_token_error(interface, 
                                    state->previous_token->line, 
                                    state->previous_token->column, 
                                    "函数名称");
        return NULL;
    }
    
    // 保存函数名称令牌
    Stru_Token_t* name_token = state->current_token;
    
    // 消耗函数名称
    F_advance_token(state);
    
    // 检查左括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_LPAREN)
    {
        // 报告错误：缺少左括号
        F_report_missing_token_error(interface, 
                                    state->previous_token->line, 
                                    state->previous_token->column, 
                                    "(");
        return NULL;
    }
    
    // 消耗左括号
    F_advance_token(state);
    
    // 解析参数列表
    Stru_DynamicArray_t* parameters = NULL;
    
    // 检查是否有参数（如果不是右括号）
    if (state->current_token != NULL && state->current_token->type != Eum_TOKEN_DELIMITER_RPAREN)
    {
        parameters = F_parse_parameter_list(interface);
        if (parameters == NULL)
        {
            // 报告错误：参数列表解析失败
            F_report_invalid_declaration_error(interface,
                                             state->current_token->line,
                                             state->current_token->column,
                                             "抽象函数参数列表无效",
                                             state->current_token);
            return NULL;
        }
    }
    
    // 检查右括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_RPAREN)
    {
        // 报告错误：缺少右括号
        F_report_missing_token_error(interface, 
                                    state->previous_token->line, 
                                    state->previous_token->column, 
                                    ")");
        if (parameters != NULL)
        {
            F_destroy_dynamic_array(parameters);
        }
        return NULL;
    }
    
    // 消耗右括号
    F_advance_token(state);
    
    // 检查分号（抽象函数以分号结束，没有函数体）
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_SEMICOLON)
    {
        // 报告错误：缺少分号
        F_report_missing_token_error(interface, 
                                    state->previous_token->line, 
                                    state->previous_token->column, 
                                    ";");
        if (parameters != NULL)
        {
            F_destroy_dynamic_array(parameters);
        }
        return NULL;
    }
    
    // 消耗分号
    F_advance_token(state);
    
    // 创建抽象函数声明节点
    Stru_AstNode_t* abstract_node = F_parser_create_ast_node(interface, Eum_AST_FUNCTION_DECL, abstract_token);
    
    // 设置抽象属性
    F_parser_set_node_attribute(interface, abstract_node, "is_abstract", (void*)1);
    
    // 设置函数名称
    F_parser_set_node_attribute(interface, abstract_node, "name", (void*)name_token->lexeme);
    
    // 设置参数列表
    if (parameters != NULL)
    {
        F_parser_set_node_attribute(interface, abstract_node, "parameters", (void*)parameters);
    }
    else
    {
        // 创建空参数数组
        Stru_DynamicArray_t* empty_params = F_create_dynamic_array_ex(sizeof(Stru_AstNode_t*), 0, NULL);
        if (empty_params != NULL)
        {
            F_parser_set_node_attribute(interface, abstract_node, "parameters", (void*)empty_params);
        }
    }
    
    // 设置返回类型（默认为空）
    F_parser_set_node_attribute(interface, abstract_node, "return_type", NULL);
    
    // 设置函数体为空（抽象函数没有函数体）
    F_parser_set_node_attribute(interface, abstract_node, "body", NULL);
    
    return abstract_node;
}
