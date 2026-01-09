/**
 * @file CN_parser_exception_statements.c
 * @brief CN_Language 异常处理语句解析模块实现
 * 
 * 异常处理语句解析功能模块的实现，负责解析try-catch和throw语句。
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
// 异常处理语句解析函数实现
// ============================================

/**
 * @brief 解析throw语句
 */
Stru_AstNode_t* F_parse_throw_statement(Stru_ParserInterface_t* interface)
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
    
    // 检查抛出关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_THROW)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "抛出");
        return NULL;
    }
    
    // 保存抛出关键字令牌
    Stru_Token_t* throw_token = state->current_token;
    
    // 消耗抛出关键字
    F_advance_token(state);
    
    // 解析异常表达式
    Stru_AstNode_t* exception_expr = F_parser_parse_expression(interface);
    if (exception_expr == NULL)
    {
        // 报告错误
        F_report_invalid_expression_error(interface,
                                         state->current_token->line,
                                         state->current_token->column,
                                         "throw语句的异常表达式无效",
                                         state->current_token);
        return NULL;
    }
    
    // 检查分号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_SEMICOLON)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    ";");
        
        // 清理异常表达式节点
        F_destroy_ast_node(exception_expr);
        return NULL;
    }
    
    // 消耗分号
    F_advance_token(state);
    
    // 创建throw语句节点
    Stru_AstNode_t* throw_node = F_parser_create_ast_node(interface, Eum_AST_THROW_STMT, throw_token);
    
    // 添加异常表达式作为子节点
    F_parser_add_child_node(interface, throw_node, exception_expr);
    
    return throw_node;
}

/**
 * @brief 解析try-catch语句
 */
Stru_AstNode_t* F_parse_try_catch_statement(Stru_ParserInterface_t* interface)
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
    
    // 检查尝试关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_TRY)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "尝试");
        return NULL;
    }
    
    // 保存尝试关键字令牌
    Stru_Token_t* try_token = state->current_token;
    
    // 消耗尝试关键字
    F_advance_token(state);
    
    // 解析try代码块
    Stru_AstNode_t* try_block = F_parse_block_statement(interface);
    if (try_block == NULL)
    {
        // 报告错误
        F_report_invalid_statement_error(interface,
                                        state->current_token->line,
                                        state->current_token->column,
                                        "try代码块无效",
                                        state->current_token);
        return NULL;
    }
    
    // 解析catch子句（可以有0个或多个）
    Stru_DynamicArray_t* catch_clauses = F_create_dynamic_array(sizeof(Stru_AstNode_t*));
    Stru_AstNode_t* finally_block = NULL;
    
    // 解析catch和finally子句
    while (state->current_token != NULL)
    {
        // 检查是否是catch子句
        if (state->current_token->type == Eum_TOKEN_KEYWORD_CATCH)
        {
            // 保存catch关键字令牌
            Stru_Token_t* catch_token = state->current_token;
            
            // 消耗catch关键字
            F_advance_token(state);
            
            // 检查左括号
            if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_LPAREN)
            {
                // 报告错误
                F_report_missing_token_error(interface, 
                                            state->previous_token->line,
                                            state->previous_token->column,
                                            "(");
                
                // 清理资源
                F_destroy_dynamic_array(catch_clauses);
                F_destroy_ast_node(try_block);
                return NULL;
            }
            
            // 消耗左括号
            F_advance_token(state);
            
            // 解析异常类型（可选）
            Stru_AstNode_t* exception_type = NULL;
            if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_IDENTIFIER)
            {
                // 解析异常类型标识符
                exception_type = F_parser_parse_expression(interface);
                if (exception_type == NULL)
                {
                    // 报告错误
                    F_report_invalid_expression_error(interface,
                                                     state->current_token->line,
                                                     state->current_token->column,
                                                     "catch子句的异常类型无效",
                                                     state->current_token);
                    
                    // 清理资源
                    F_destroy_dynamic_array(catch_clauses);
                    F_destroy_ast_node(try_block);
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
                
                // 清理资源
                if (exception_type != NULL)
                {
                    F_destroy_ast_node(exception_type);
                }
                F_destroy_dynamic_array(catch_clauses);
                F_destroy_ast_node(try_block);
                return NULL;
            }
            
            // 消耗右括号
            F_advance_token(state);
            
            // 解析catch代码块
            Stru_AstNode_t* catch_block = F_parse_block_statement(interface);
            if (catch_block == NULL)
            {
                // 报告错误
                F_report_invalid_statement_error(interface,
                                                state->current_token->line,
                                                state->current_token->column,
                                                "catch代码块无效",
                                                state->current_token);
                
                // 清理资源
                if (exception_type != NULL)
                {
                    F_destroy_ast_node(exception_type);
                }
                F_destroy_dynamic_array(catch_clauses);
                F_destroy_ast_node(try_block);
                return NULL;
            }
            
            // 创建catch语句节点
            Stru_AstNode_t* catch_node = F_parser_create_ast_node(interface, Eum_AST_CATCH_STMT, catch_token);
            
            // 添加异常类型和catch代码块作为子节点
            if (exception_type != NULL)
            {
                F_parser_add_child_node(interface, catch_node, exception_type);
            }
            F_parser_add_child_node(interface, catch_node, catch_block);
            
            // 添加到catch子句数组
            F_dynamic_array_push(catch_clauses, &catch_node);
        }
        // 检查是否是finally子句
        else if (state->current_token->type == Eum_TOKEN_KEYWORD_FINALLY)
        {
            if (finally_block != NULL)
            {
                // 报告错误：多个finally子句
                F_report_parser_error(interface,
                                     Eum_ERROR_DUPLICATE_DECLARATION,  // 错误类型：重复声明
                                     Eum_SEVERITY_ERROR,               // 错误严重级别
                                     state->current_token->line,
                                     state->current_token->column,
                                     "try-catch语句中只能有一个finally子句",
                                     state->current_token);
                
                // 清理资源
                F_destroy_dynamic_array(catch_clauses);
                F_destroy_ast_node(try_block);
                return NULL;
            }
            
            // 保存finally关键字令牌
            Stru_Token_t* finally_token = state->current_token;
            
            // 消耗finally关键字
            F_advance_token(state);
            
            // 解析finally代码块
            finally_block = F_parse_block_statement(interface);
            if (finally_block == NULL)
            {
                // 报告错误
                F_report_invalid_statement_error(interface,
                                                state->current_token->line,
                                                state->current_token->column,
                                                "finally代码块无效",
                                                state->current_token);
                
                // 清理资源
                F_destroy_dynamic_array(catch_clauses);
                F_destroy_ast_node(try_block);
                return NULL;
            }
            
            // 创建finally语句节点
            Stru_AstNode_t* finally_node = F_parser_create_ast_node(interface, Eum_AST_FINALLY_STMT, finally_token);
            
            // 添加finally代码块作为子节点
            F_parser_add_child_node(interface, finally_node, finally_block);
            
            // 设置finally块
            finally_block = finally_node;
        }
        else
        {
            // 不是catch或finally，结束解析
            break;
        }
    }
    
    // 检查是否有至少一个catch子句或finally子句
    size_t catch_count = F_dynamic_array_length(catch_clauses);
    if (catch_count == 0 && finally_block == NULL)
    {
        // 报告错误：try语句必须至少有一个catch或finally子句
        F_report_parser_error(interface,
                             Eum_ERROR_INVALID_STATEMENT,  // 错误类型：无效的语句
                             Eum_SEVERITY_ERROR,           // 错误严重级别
                             state->current_token->line,
                             state->current_token->column,
                             "try语句必须至少有一个catch子句或finally子句",
                             state->current_token);
        
        // 清理资源
        F_destroy_dynamic_array(catch_clauses);
        F_destroy_ast_node(try_block);
        return NULL;
    }
    
    // 创建try语句节点
    Stru_AstNode_t* try_catch_node = F_parser_create_ast_node(interface, Eum_AST_TRY_STMT, try_token);
    
    // 添加try代码块作为子节点
    F_parser_add_child_node(interface, try_catch_node, try_block);
    
    // 添加catch子句作为子节点
    for (size_t i = 0; i < catch_count; i++)
    {
        Stru_AstNode_t** catch_node_ptr = (Stru_AstNode_t**)F_dynamic_array_get(catch_clauses, i);
        if (catch_node_ptr != NULL && *catch_node_ptr != NULL)
        {
            F_parser_add_child_node(interface, try_catch_node, *catch_node_ptr);
        }
    }
    
    // 添加finally子句作为子节点（如果有）
    if (finally_block != NULL)
    {
        F_parser_add_child_node(interface, try_catch_node, finally_block);
    }
    
    // 清理动态数组
    F_destroy_dynamic_array(catch_clauses);
    
    return try_catch_node;
}
