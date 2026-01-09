/**
 * @file CN_parser_block_statements.c
 * @brief CN_Language 代码块和表达式语句解析模块实现
 * 
 * 代码块和表达式语句解析功能模块的实现，负责解析代码块语句和表达式语句。
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
#include "../../../infrastructure/containers/array/CN_dynamic_array.h"
#include "../../token/CN_token_types.h"
#include "../../ast/CN_ast.h"
#include <stdlib.h>

// ============================================
// 内部辅助函数声明
// ============================================

/**
 * @brief 解析表达式语句（内部实现）
 */
static Stru_AstNode_t* parse_expression_statement_impl(Stru_ParserInterface_t* interface);

/**
 * @brief 解析代码块语句（内部实现）
 */
static Stru_AstNode_t* parse_block_statement_impl(Stru_ParserInterface_t* interface);

// ============================================
// 代码块和表达式语句解析函数实现
// ============================================

/**
 * @brief 解析表达式语句
 */
Stru_AstNode_t* F_parse_expression_statement(Stru_ParserInterface_t* interface)
{
    return parse_expression_statement_impl(interface);
}

/**
 * @brief 解析代码块语句
 */
Stru_AstNode_t* F_parse_block_statement(Stru_ParserInterface_t* interface)
{
    return parse_block_statement_impl(interface);
}

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 解析表达式语句（内部实现）
 */
static Stru_AstNode_t* parse_expression_statement_impl(Stru_ParserInterface_t* interface)
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
    
    // 解析表达式
    Stru_AstNode_t* expression = F_parse_expression(interface);
    if (expression == NULL)
    {
        // 报告错误
        F_report_invalid_expression_error(interface,
                                         state->current_token->line,
                                         state->current_token->column,
                                         "表达式无效",
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
    
    // 创建表达式语句节点
    Stru_AstNode_t* expression_statement = F_parser_create_ast_node(interface, Eum_AST_EXPRESSION_STMT, NULL);
    
    // 添加表达式作为子节点
    F_parser_add_child_node(interface, expression_statement, expression);
    
    return expression_statement;
}

/**
 * @brief 解析代码块语句（内部实现）
 */
static Stru_AstNode_t* parse_block_statement_impl(Stru_ParserInterface_t* interface)
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
    
    // 检查左花括号
    if (state->current_token->type != Eum_TOKEN_DELIMITER_LBRACE)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "{");
        return NULL;
    }
    
    // 保存左花括号令牌
    Stru_Token_t* lbrace_token = state->current_token;
    
    // 消耗左花括号
    F_advance_token(state);
    
    // 解析语句列表
    Stru_DynamicArray_t* statements = F_parse_statement_list(interface);
    if (statements == NULL)
    {
        // 可能没有语句，这是允许的
        statements = F_create_dynamic_array(sizeof(Stru_AstNode_t*));
        if (statements == NULL)
        {
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
        
        // 清理语句数组
        F_destroy_dynamic_array(statements);
        return NULL;
    }
    
    // 消耗右花括号
    F_advance_token(state);
    
    // 创建代码块语句节点
    Stru_AstNode_t* block_node = F_parser_create_ast_node(interface, Eum_AST_BLOCK_STMT, lbrace_token);
    
    // 添加语句列表作为属性
    F_parser_set_node_attribute(interface, block_node, "statements", statements);
    
    return block_node;
}
