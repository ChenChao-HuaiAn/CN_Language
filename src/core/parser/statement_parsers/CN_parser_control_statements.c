/**
 * @file CN_parser_control_statements.c
 * @brief CN_Language 控制语句解析模块实现
 * 
 * 控制语句解析功能模块的实现，负责解析中断、继续和返回语句。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_parser_statements.h"
#include "../interface/CN_parser_interface_impl.h"
#include "../error_handling/CN_parser_errors.h"
#include "../utils/CN_parser_utils.h"
#include "../../token/CN_token_types.h"
#include "../../ast/CN_ast.h"
#include <stdlib.h>

// ============================================
// 控制语句解析函数实现
// ============================================

/**
 * @brief 解析中断语句
 */
Stru_AstNode_t* F_parse_break_statement(Stru_ParserInterface_t* interface)
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
    
    // 检查是否为中断关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_BREAK)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "中断");
        return NULL;
    }
    
    // 保存关键字令牌
    Stru_Token_t* keyword_token = state->current_token;
    
    // 消耗中断关键字
    F_advance_token(state);
    
    // 检查分号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_SEMICOLON)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    ";");
        return NULL;
    }
    
    // 消耗分号
    F_advance_token(state);
    
    // 创建中断语句节点
    Stru_AstNode_t* break_node = F_parser_create_ast_node(interface, Eum_AST_BREAK_STMT, keyword_token);
    
    return break_node;
}

/**
 * @brief 解析继续语句
 */
Stru_AstNode_t* F_parse_continue_statement(Stru_ParserInterface_t* interface)
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
    
    // 检查是否为继续关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_CONTINUE)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "继续");
        return NULL;
    }
    
    // 保存关键字令牌
    Stru_Token_t* keyword_token = state->current_token;
    
    // 消耗继续关键字
    F_advance_token(state);
    
    // 检查分号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_SEMICOLON)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    ";");
        return NULL;
    }
    
    // 消耗分号
    F_advance_token(state);
    
    // 创建继续语句节点
    Stru_AstNode_t* continue_node = F_parser_create_ast_node(interface, Eum_AST_CONTINUE_STMT, keyword_token);
    
    return continue_node;
}

/**
 * @brief 解析返回语句
 */
Stru_AstNode_t* F_parse_return_statement(Stru_ParserInterface_t* interface)
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
    
    // 检查是否为返回关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_RETURN)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "返回");
        return NULL;
    }
    
    // 保存关键字令牌
    Stru_Token_t* keyword_token = state->current_token;
    
    // 消耗返回关键字
    F_advance_token(state);
    
    // 检查是否有返回值表达式
    Stru_AstNode_t* return_value = NULL;
    if (state->current_token != NULL && state->current_token->type != Eum_TOKEN_DELIMITER_SEMICOLON)
    {
        // 解析返回值表达式
        return_value = F_parser_parse_expression(interface);
        if (return_value == NULL)
        {
            // 报告错误
            F_report_invalid_expression_error(interface,
                                             state->current_token->line,
                                             state->current_token->column,
                                             "返回值表达式无效",
                                             state->current_token);
            return NULL;
        }
    }
    
    // 检查分号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_SEMICOLON)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    ";");
        
        // 清理返回值节点
        if (return_value != NULL)
        {
            F_destroy_ast_node(return_value);
        }
        
        return NULL;
    }
    
    // 消耗分号
    F_advance_token(state);
    
    // 创建返回语句节点
    Stru_AstNode_t* return_node = F_parser_create_ast_node(interface, Eum_AST_RETURN_STMT, keyword_token);
    
    // 如果有返回值，添加到节点
    if (return_value != NULL)
    {
        F_parser_add_child_node(interface, return_node, return_value);
    }
    
    return return_node;
}

/**
 * @brief 解析空语句
 */
Stru_AstNode_t* F_parse_empty_statement(Stru_ParserInterface_t* interface)
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
    
    // 检查是否为分号
    if (state->current_token->type != Eum_TOKEN_DELIMITER_SEMICOLON)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, ";");
        return NULL;
    }
    
    // 保存分号令牌
    Stru_Token_t* semicolon_token = state->current_token;
    
    // 消耗分号
    F_advance_token(state);
    
    // 创建空语句节点 - 使用表达式语句类型，但标记为空
    Stru_AstNode_t* empty_node = F_parser_create_ast_node(interface, Eum_AST_EXPRESSION_STMT, semicolon_token);
    // 标记为空语句
    F_parser_set_node_attribute(interface, empty_node, "is_empty", (void*)true);
    
    return empty_node;
}
