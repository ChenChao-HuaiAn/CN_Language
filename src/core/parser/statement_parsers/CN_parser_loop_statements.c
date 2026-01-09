/**
 * @file CN_parser_loop_statements.c
 * @brief CN_Language 循环语句解析模块实现
 * 
 * 循环语句解析功能模块的实现，负责解析while和for循环语句。
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
// 循环语句解析函数实现
// ============================================

/**
 * @brief 解析循环语句
 */
Stru_AstNode_t* F_parse_while_statement(Stru_ParserInterface_t* interface)
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
    
    // 检查while关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_WHILE)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "当");
        return NULL;
    }
    
    // 保存while关键字令牌
    Stru_Token_t* while_token = state->current_token;
    
    // 消耗while关键字
    F_advance_token(state);
    
    // 检查左括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_LPAREN)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    "(");
        return NULL;
    }
    
    // 消耗左括号
    F_advance_token(state);
    
    // 解析条件表达式
    Stru_AstNode_t* condition = F_parser_parse_expression(interface);
    if (condition == NULL)
    {
        // 报告错误
        F_report_invalid_expression_error(interface,
                                         state->current_token->line,
                                         state->current_token->column,
                                         "循环条件表达式无效",
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
        
        // 清理条件节点
        F_destroy_ast_node(condition);
        return NULL;
    }
    
    // 消耗右括号
    F_advance_token(state);
    
    // 解析循环体
    Stru_AstNode_t* body = F_parse_statement(interface);
    if (body == NULL)
    {
        // 报告错误
        F_report_invalid_statement_error(interface,
                                        state->current_token->line,
                                        state->current_token->column,
                                        "循环体语句无效",
                                        state->current_token);
        
        // 清理条件节点
        F_destroy_ast_node(condition);
        return NULL;
    }
    
    // 创建while语句节点
    Stru_AstNode_t* while_node = F_parser_create_ast_node(interface, Eum_AST_WHILE_STMT, while_token);
    
    // 添加条件和循环体作为子节点
    F_parser_add_child_node(interface, while_node, condition);
    F_parser_add_child_node(interface, while_node, body);
    
    return while_node;
}

/**
 * @brief 解析for循环语句
 */
Stru_AstNode_t* F_parse_for_statement(Stru_ParserInterface_t* interface)
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
    
    // 检查for关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_FOR)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "循环");
        return NULL;
    }
    
    // 保存for关键字令牌
    Stru_Token_t* for_token = state->current_token;
    
    // 消耗for关键字
    F_advance_token(state);
    
    // 检查左括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_LPAREN)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    "(");
        return NULL;
    }
    
    // 消耗左括号
    F_advance_token(state);
    
    // 解析初始化部分（可选）
    Stru_AstNode_t* init = NULL;
    if (state->current_token != NULL && state->current_token->type != Eum_TOKEN_DELIMITER_SEMICOLON)
    {
        // 可能是变量声明或表达式
        if (state->current_token->type == Eum_TOKEN_KEYWORD_VAR)
        {
            init = F_parser_parse_declaration(interface);
        }
        else
        {
            init = F_parser_parse_expression(interface);
        }
        
        if (init == NULL)
        {
            // 报告错误
            F_report_invalid_expression_error(interface,
                                             state->current_token->line,
                                             state->current_token->column,
                                             "for循环初始化部分无效",
                                             state->current_token);
            return NULL;
        }
    }
    
    // 检查第一个分号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_SEMICOLON)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    ";");
        
        // 清理初始化节点
        if (init != NULL)
        {
            F_destroy_ast_node(init);
        }
        return NULL;
    }
    
    // 消耗第一个分号
    F_advance_token(state);
    
    // 解析条件部分（可选）
    Stru_AstNode_t* condition = NULL;
    if (state->current_token != NULL && state->current_token->type != Eum_TOKEN_DELIMITER_SEMICOLON)
    {
        condition = F_parser_parse_expression(interface);
        if (condition == NULL)
        {
            // 报告错误
            F_report_invalid_expression_error(interface,
                                             state->current_token->line,
                                             state->current_token->column,
                                             "for循环条件部分无效",
                                             state->current_token);
            
            // 清理初始化节点
            if (init != NULL)
            {
                F_destroy_ast_node(init);
            }
            return NULL;
        }
    }
    
    // 检查第二个分号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_SEMICOLON)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    ";");
        
        // 清理初始化节点和条件节点
        if (init != NULL)
        {
            F_destroy_ast_node(init);
        }
        if (condition != NULL)
        {
            F_destroy_ast_node(condition);
        }
        return NULL;
    }
    
    // 消耗第二个分号
    F_advance_token(state);
    
    // 解析更新部分（可选）
    Stru_AstNode_t* update = NULL;
    if (state->current_token != NULL && state->current_token->type != Eum_TOKEN_DELIMITER_RPAREN)
    {
        update = F_parser_parse_expression(interface);
        if (update == NULL)
        {
            // 报告错误
            F_report_invalid_expression_error(interface,
                                             state->current_token->line,
                                             state->current_token->column,
                                             "for循环更新部分无效",
                                             state->current_token);
            
            // 清理初始化节点和条件节点
            if (init != NULL)
            {
                F_destroy_ast_node(init);
            }
            if (condition != NULL)
            {
                F_destroy_ast_node(condition);
            }
            return NULL;
        }
    }
    
    // 检查右括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_RPAREN)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    ")");
        
        // 清理所有节点
        if (init != NULL)
        {
            F_destroy_ast_node(init);
        }
        if (condition != NULL)
        {
            F_destroy_ast_node(condition);
        }
        if (update != NULL)
        {
            F_destroy_ast_node(update);
        }
        return NULL;
    }
    
    // 消耗右括号
    F_advance_token(state);
    
    // 解析循环体
    Stru_AstNode_t* body = F_parse_statement(interface);
    if (body == NULL)
    {
        // 报告错误
        F_report_invalid_statement_error(interface,
                                        state->current_token->line,
                                        state->current_token->column,
                                        "for循环体语句无效",
                                        state->current_token);
        
        // 清理所有节点
        if (init != NULL)
        {
            F_destroy_ast_node(init);
        }
        if (condition != NULL)
        {
            F_destroy_ast_node(condition);
        }
        if (update != NULL)
        {
            F_destroy_ast_node(update);
        }
        return NULL;
    }
    
    // 创建for语句节点
    Stru_AstNode_t* for_node = F_parser_create_ast_node(interface, Eum_AST_FOR_STMT, for_token);
    
    // 添加初始化、条件、更新和循环体作为子节点
    if (init != NULL)
    {
        F_parser_add_child_node(interface, for_node, init);
    }
    else
    {
        // 添加空节点作为占位符
        Stru_AstNode_t* empty_init = F_parser_create_ast_node(interface, Eum_AST_EXPRESSION_STMT, NULL);
        F_parser_set_node_attribute(interface, empty_init, "is_empty", (void*)true);
        F_parser_add_child_node(interface, for_node, empty_init);
    }
    
    if (condition != NULL)
    {
        F_parser_add_child_node(interface, for_node, condition);
    }
    else
    {
        // 添加空节点作为占位符
        Stru_AstNode_t* empty_condition = F_parser_create_ast_node(interface, Eum_AST_EXPRESSION_STMT, NULL);
        F_parser_set_node_attribute(interface, empty_condition, "is_empty", (void*)true);
        F_parser_add_child_node(interface, for_node, empty_condition);
    }
    
    if (update != NULL)
    {
        F_parser_add_child_node(interface, for_node, update);
    }
    else
    {
        // 添加空节点作为占位符
        Stru_AstNode_t* empty_update = F_parser_create_ast_node(interface, Eum_AST_EXPRESSION_STMT, NULL);
        F_parser_set_node_attribute(interface, empty_update, "is_empty", (void*)true);
        F_parser_add_child_node(interface, for_node, empty_update);
    }
    
    F_parser_add_child_node(interface, for_node, body);
    
    return for_node;
}
