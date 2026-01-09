/**
 * @file CN_parser_switch_statements.c
 * @brief CN_Language switch-case语句解析模块实现
 * 
 * switch-case语句解析功能模块的实现，负责解析switch-case多分支选择语句。
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
// switch-case语句解析函数实现
// ============================================

/**
 * @brief 解析case语句
 */
Stru_AstNode_t* F_parse_case_statement(Stru_ParserInterface_t* interface)
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
    
    // 检查情况关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_CASE)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "情况");
        return NULL;
    }
    
    // 保存情况关键字令牌
    Stru_Token_t* case_token = state->current_token;
    
    // 消耗情况关键字
    F_advance_token(state);
    
    // 解析case表达式
    Stru_AstNode_t* case_expression = F_parser_parse_expression(interface);
    if (case_expression == NULL)
    {
        // 报告错误
        F_report_invalid_expression_error(interface,
                                         state->current_token->line,
                                         state->current_token->column,
                                         "case表达式无效",
                                         state->current_token);
        return NULL;
    }
    
    // 检查冒号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_COLON)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    ":");
        
        // 清理case表达式节点
        F_destroy_ast_node(case_expression);
        return NULL;
    }
    
    // 消耗冒号
    F_advance_token(state);
    
    // 解析case语句体（可以是单个语句或代码块）
    Stru_AstNode_t* case_body = NULL;
    
    // 检查是否是代码块
    if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_DELIMITER_LBRACE)
    {
        // 解析代码块
        case_body = F_parse_block_statement(interface);
    }
    else
    {
        // 解析单个语句
        case_body = F_parse_statement(interface);
    }
    
    if (case_body == NULL)
    {
        // 报告错误
        F_report_invalid_statement_error(interface,
                                        state->current_token->line,
                                        state->current_token->column,
                                        "case语句体无效",
                                        state->current_token);
        
        // 清理case表达式节点
        F_destroy_ast_node(case_expression);
        return NULL;
    }
    
    // 创建case语句节点
    Stru_AstNode_t* case_node = F_parser_create_ast_node(interface, Eum_AST_CASE_STMT, case_token);
    
    // 添加case表达式和语句体作为子节点
    F_parser_add_child_node(interface, case_node, case_expression);
    F_parser_add_child_node(interface, case_node, case_body);
    
    return case_node;
}

/**
 * @brief 解析default语句
 */
Stru_AstNode_t* F_parse_default_statement(Stru_ParserInterface_t* interface)
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
    
    // 检查默认关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_DEFAULT)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "默认");
        return NULL;
    }
    
    // 保存默认关键字令牌
    Stru_Token_t* default_token = state->current_token;
    
    // 消耗默认关键字
    F_advance_token(state);
    
    // 检查冒号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_COLON)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    ":");
        return NULL;
    }
    
    // 消耗冒号
    F_advance_token(state);
    
    // 解析default语句体（可以是单个语句或代码块）
    Stru_AstNode_t* default_body = NULL;
    
    // 检查是否是代码块
    if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_DELIMITER_LBRACE)
    {
        // 解析代码块
        default_body = F_parse_block_statement(interface);
    }
    else
    {
        // 解析单个语句
        default_body = F_parse_statement(interface);
    }
    
    if (default_body == NULL)
    {
        // 报告错误
        F_report_invalid_statement_error(interface,
                                        state->current_token->line,
                                        state->current_token->column,
                                        "default语句体无效",
                                        state->current_token);
        return NULL;
    }
    
    // 创建default语句节点
    Stru_AstNode_t* default_node = F_parser_create_ast_node(interface, Eum_AST_DEFAULT_STMT, default_token);
    
    // 添加default语句体作为子节点
    F_parser_add_child_node(interface, default_node, default_body);
    
    return default_node;
}

/**
 * @brief 解析switch语句
 */
Stru_AstNode_t* F_parse_switch_statement(Stru_ParserInterface_t* interface)
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
    
    // 检查选择关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_SWITCH)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "选择");
        return NULL;
    }
    
    // 保存选择关键字令牌
    Stru_Token_t* switch_token = state->current_token;
    
    // 消耗选择关键字
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
    
    // 解析switch表达式
    Stru_AstNode_t* switch_expression = F_parser_parse_expression(interface);
    if (switch_expression == NULL)
    {
        // 报告错误
        F_report_invalid_expression_error(interface,
                                         state->current_token->line,
                                         state->current_token->column,
                                         "switch表达式无效",
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
        
        // 清理switch表达式节点
        F_destroy_ast_node(switch_expression);
        return NULL;
    }
    
    // 消耗右括号
    F_advance_token(state);
    
    // 检查左花括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_LBRACE)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    "{");
        
        // 清理switch表达式节点
        F_destroy_ast_node(switch_expression);
        return NULL;
    }
    
    // 消耗左花括号
    F_advance_token(state);
    
    // 解析case和default语句
    Stru_DynamicArray_t* case_nodes = F_create_dynamic_array(sizeof(Stru_AstNode_t*));
    Stru_AstNode_t* default_node = NULL;
    
    while (state->current_token != NULL && state->current_token->type != Eum_TOKEN_DELIMITER_RBRACE)
    {
        // 检查是否是case语句
        if (state->current_token->type == Eum_TOKEN_KEYWORD_CASE)
        {
            Stru_AstNode_t* case_node = F_parse_case_statement(interface);
            if (case_node == NULL)
            {
                // 报告错误
                F_report_invalid_statement_error(interface,
                                                state->current_token->line,
                                                state->current_token->column,
                                                "case语句无效",
                                                state->current_token);
                
                // 清理资源
                F_destroy_dynamic_array(case_nodes);
                F_destroy_ast_node(switch_expression);
                if (default_node != NULL)
                {
                    F_destroy_ast_node(default_node);
                }
                return NULL;
            }
            
            // 添加到case节点数组
            F_dynamic_array_push(case_nodes, &case_node);
        }
        // 检查是否是default语句
        else if (state->current_token->type == Eum_TOKEN_KEYWORD_DEFAULT)
        {
            if (default_node != NULL)
            {
                // 报告错误：多个default语句
                // 使用重复声明错误类型
                F_report_parser_error(interface,
                                     Eum_ERROR_DUPLICATE_DECLARATION,  // 错误类型：重复声明
                                     Eum_SEVERITY_ERROR,               // 错误严重级别
                                     state->current_token->line,
                                     state->current_token->column,
                                     "switch语句中只能有一个default分支",
                                     state->current_token);
                
                // 清理资源
                F_destroy_dynamic_array(case_nodes);
                F_destroy_ast_node(switch_expression);
                return NULL;
            }
            
            default_node = F_parse_default_statement(interface);
            if (default_node == NULL)
            {
                // 报告错误
                F_report_invalid_statement_error(interface,
                                                state->current_token->line,
                                                state->current_token->column,
                                                "default语句无效",
                                                state->current_token);
                
                // 清理资源
                F_destroy_dynamic_array(case_nodes);
                F_destroy_ast_node(switch_expression);
                return NULL;
            }
        }
        else
        {
            // 报告错误：期望case或default
            F_report_unexpected_token_error(interface, state->current_token, "情况或默认");
            
            // 清理资源
            F_destroy_dynamic_array(case_nodes);
            F_destroy_ast_node(switch_expression);
            if (default_node != NULL)
            {
                F_destroy_ast_node(default_node);
            }
            return NULL;
        }
    }
    
    // 检查右花括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_RBRACE)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    "}");
        
        // 清理资源
        F_destroy_dynamic_array(case_nodes);
        F_destroy_ast_node(switch_expression);
        if (default_node != NULL)
        {
            F_destroy_ast_node(default_node);
        }
        return NULL;
    }
    
    // 消耗右花括号
    F_advance_token(state);
    
    // 创建switch语句节点
    Stru_AstNode_t* switch_node = F_parser_create_ast_node(interface, Eum_AST_SWITCH_STMT, switch_token);
    
    // 添加switch表达式作为子节点
    F_parser_add_child_node(interface, switch_node, switch_expression);
    
    // 添加case节点作为子节点
    size_t case_count = F_dynamic_array_length(case_nodes);
    for (size_t i = 0; i < case_count; i++)
    {
        Stru_AstNode_t** case_node_ptr = (Stru_AstNode_t**)F_dynamic_array_get(case_nodes, i);
        if (case_node_ptr != NULL && *case_node_ptr != NULL)
        {
            F_parser_add_child_node(interface, switch_node, *case_node_ptr);
        }
    }
    
    // 添加default节点作为子节点（如果有）
    if (default_node != NULL)
    {
        F_parser_add_child_node(interface, switch_node, default_node);
    }
    
    // 清理动态数组
    F_destroy_dynamic_array(case_nodes);
    
    return switch_node;
}
