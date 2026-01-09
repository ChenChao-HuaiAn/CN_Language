/**
 * @file CN_parser_conditional_statements.c
 * @brief CN_Language 条件语句解析模块实现
 * 
 * 条件语句解析功能模块的实现，负责解析if条件语句。
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
// 条件语句解析函数实现
// ============================================

/**
 * @brief 解析条件语句
 */
Stru_AstNode_t* F_parse_if_statement(Stru_ParserInterface_t* interface)
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
    
    // 检查如果关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_IF)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "如果");
        return NULL;
    }
    
    // 保存如果关键字令牌
    Stru_Token_t* if_token = state->current_token;
    
    // 消耗如果关键字
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
    Stru_AstNode_t* condition = F_parse_expression(interface);
    if (condition == NULL)
    {
        // 报告错误
        F_report_invalid_expression_error(interface,
                                         state->current_token->line,
                                         state->current_token->column,
                                         "条件表达式无效",
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
    
    // 解析then分支
    Stru_AstNode_t* then_branch = F_parse_statement(interface);
    if (then_branch == NULL)
    {
        // 报告错误
        F_report_invalid_statement_error(interface,
                                        state->current_token->line,
                                        state->current_token->column,
                                        "then分支语句无效",
                                        state->current_token);
        
        // 清理条件节点
        F_destroy_ast_node(condition);
        return NULL;
    }
    
    // 检查是否有else分支
    Stru_AstNode_t* else_branch = NULL;
    if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_KEYWORD_ELSE)
    {
        // 消耗else关键字
        F_advance_token(state);
        
        // 解析else分支
        else_branch = F_parse_statement(interface);
        if (else_branch == NULL)
        {
            // 报告错误
            F_report_invalid_statement_error(interface,
                                            state->current_token->line,
                                            state->current_token->column,
                                            "else分支语句无效",
                                            state->current_token);
            
            // 清理条件节点和then分支
            F_destroy_ast_node(condition);
            F_destroy_ast_node(then_branch);
            return NULL;
        }
    }
    
    // 创建if语句节点
    Stru_AstNode_t* if_node = F_parser_create_ast_node(interface, Eum_AST_IF_STMT, if_token);
    
    // 添加条件、then分支和else分支作为子节点
    F_parser_add_child_node(interface, if_node, condition);
    F_parser_add_child_node(interface, if_node, then_branch);
    if (else_branch != NULL)
    {
        F_parser_add_child_node(interface, if_node, else_branch);
    }
    
    return if_node;
}
