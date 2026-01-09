/**
 * @file CN_parser_function_calls.c
 * @brief CN_Language 函数调用解析模块
 * 
 * 函数调用解析模块，负责解析函数调用表达式和参数列表。
 * 包括函数调用表达式、参数列表解析等。
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
// 函数调用解析函数实现
// ============================================

/**
 * @brief 解析函数调用表达式
 */
Stru_AstNode_t* F_parse_function_call_expression(Stru_ParserInterface_t* interface)
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
    
    // 解析函数名（标识符）
    Stru_AstNode_t* function_name_node = F_parse_identifier_expression(interface);
    if (function_name_node == NULL)
    {
        return NULL;
    }
    
    // 检查左括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_LPAREN)
    {
        // 不是函数调用，返回标识符表达式
        return function_name_node;
    }
    
    // 消耗左括号
    F_advance_token(state);
    
    // 解析参数列表
    Stru_DynamicArray_t* arguments = F_parse_argument_list(interface);
    
    // 检查右括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_RPAREN)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    ")");
        
        // 清理参数列表
        if (arguments != NULL)
        {
            // TODO: 清理参数数组中的节点
            F_destroy_dynamic_array(arguments);
        }
        
        return NULL;
    }
    
    // 消耗右括号
    F_advance_token(state);
    
    // 创建函数调用节点
    Stru_Token_t* function_name_token = state->current_token;
    Stru_AstNode_t* function_call_node = F_create_function_call_node(interface, 
                                                                    function_name_token, 
                                                                    arguments);
    
    // 清理函数名节点（函数调用节点会复制需要的信息）
    F_destroy_ast_node(function_name_node);
    
    return function_call_node;
}

/**
 * @brief 解析参数列表
 */
Stru_DynamicArray_t* F_parse_argument_list(Stru_ParserInterface_t* interface)
{
    if (interface == NULL)
    {
        return NULL;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL)
    {
        return NULL;
    }
    
    // 创建参数数组
    Stru_DynamicArray_t* arguments = F_create_dynamic_array(sizeof(Stru_AstNode_t*));
    if (arguments == NULL)
    {
        return NULL;
    }
    
    // 检查是否为空参数列表
    if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_DELIMITER_RPAREN)
    {
        return arguments; // 空参数列表
    }
    
    // 解析第一个参数
    Stru_AstNode_t* argument = F_parse_expression(interface);
    if (argument == NULL)
    {
        F_destroy_dynamic_array(arguments);
        return NULL;
    }
    
    F_dynamic_array_push(arguments, &argument);
    
    // 解析更多参数（用逗号分隔）
    while (state->current_token != NULL && state->current_token->type == Eum_TOKEN_DELIMITER_COMMA)
    {
        // 消耗逗号
        F_advance_token(state);
        
        // 解析下一个参数
        argument = F_parse_expression(interface);
        if (argument == NULL)
        {
            // 报告错误但继续尝试恢复
            F_report_invalid_expression_error(interface,
                                             state->current_token->line,
                                             state->current_token->column,
                                             "参数表达式无效",
                                             state->current_token);
            
            // 尝试同步到安全点
            Eum_TokenType sync_tokens[] = {Eum_TOKEN_DELIMITER_COMMA, Eum_TOKEN_DELIMITER_RPAREN, Eum_TOKEN_DELIMITER_SEMICOLON};
            F_synchronize_to_safe_point(interface, sync_tokens, 3);
            
            // 如果当前令牌是右括号，跳出循环
            if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_DELIMITER_RPAREN)
            {
                break;
            }
            
            continue;
        }
        
        F_dynamic_array_push(arguments, &argument);
    }
    
    return arguments;
}
