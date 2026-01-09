/**
 * @file CN_parser_async_statements.c
 * @brief CN_Language 异步语句解析模块实现
 * 
 * 异步语句解析功能模块的实现，负责解析异步函数声明和等待语句。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_parser_statements.h"
#include "../interface/CN_parser_interface_impl.h"
#include "../error_handling/CN_parser_errors.h"
#include "../utils/CN_parser_utils.h"
#include "../declaration_parsers/CN_parser_declarations.h"
#include "../../token/CN_token_types.h"
#include "../../ast/CN_ast.h"
#include <stdlib.h>

// ============================================
// 异步语句解析函数实现
// ============================================

/**
 * @brief 解析异步语句
 * 
 * 解析异步语句，如：异步 函数 名称() { ... }。
 * 异步语句实际上是异步函数声明，它是一个声明而不是语句。
 * 但为了语法分析器的完整性，我们将其作为语句处理。
 */
Stru_AstNode_t* F_parse_async_statement(Stru_ParserInterface_t* interface)
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
    
    // 检查是否为异步关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_ASYNC)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "异步");
        return NULL;
    }
    
    // 保存异步关键字令牌
    Stru_Token_t* async_token = state->current_token;
    
    // 消耗异步关键字
    F_advance_token(state);
    
    // 检查下一个令牌是否为函数关键字
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_KEYWORD_FUNCTION)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "函数");
        return NULL;
    }
    
    // 异步语句实际上是异步函数声明，委托给声明解析模块
    // 但我们需要标记这是一个异步函数
    Stru_AstNode_t* function_decl = F_parse_declaration(interface);
    if (function_decl == NULL)
    {
        // 报告错误
        F_report_invalid_declaration_error(interface,
                                          state->current_token->line,
                                          state->current_token->column,
                                          "异步函数声明无效",
                                          state->current_token);
        return NULL;
    }
    
    // 标记函数为异步
    F_parser_set_node_attribute(interface, function_decl, "is_async", (void*)true);
    
    // 设置异步关键字令牌属性
    F_parser_set_node_attribute(interface, function_decl, "async_token", async_token);
    
    return function_decl;
}

/**
 * @brief 解析等待语句
 * 
 * 解析等待语句，如：等待 表达式;。
 */
Stru_AstNode_t* F_parse_await_statement(Stru_ParserInterface_t* interface)
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
    
    // 检查是否为等待关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_AWAIT)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "等待");
        return NULL;
    }
    
    // 保存等待关键字令牌
    Stru_Token_t* await_token = state->current_token;
    
    // 消耗等待关键字
    F_advance_token(state);
    
    // 解析等待的表达式
    Stru_AstNode_t* expression = F_parser_parse_expression(interface);
    if (expression == NULL)
    {
        // 报告错误
        F_report_invalid_expression_error(interface,
                                         state->current_token->line,
                                         state->current_token->column,
                                         "等待表达式无效",
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
        
        // 清理表达式节点
        F_destroy_ast_node(expression);
        
        return NULL;
    }
    
    // 消耗分号
    F_advance_token(state);
    
    // 创建等待语句节点
    // 注意：AST节点类型中可能没有专门的等待语句类型，我们可以使用表达式语句类型
    // 但标记为等待语句
    Stru_AstNode_t* await_node = F_parser_create_ast_node(interface, Eum_AST_EXPRESSION_STMT, await_token);
    
    // 标记为等待语句
    F_parser_set_node_attribute(interface, await_node, "is_await", (void*)true);
    
    // 添加表达式作为子节点
    F_parser_add_child_node(interface, await_node, expression);
    
    return await_node;
}
