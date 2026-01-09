/**
 * @file CN_parser_statements_main.c
 * @brief CN_Language 语句解析主模块实现
 * 
 * 语句解析功能模块的主实现，负责语句类型分发和主解析函数。
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
#include "../declaration_parsers/CN_parser_declarations.h"
#include "../../../infrastructure/containers/array/CN_dynamic_array.h"
#include "../../token/CN_token_types.h"
#include "../../ast/CN_ast.h"
#include <stdlib.h>
#include <string.h>

// ============================================
// 语句解析函数实现
// ============================================

/**
 * @brief 解析语句
 */
Stru_AstNode_t* F_parse_statement(Stru_ParserInterface_t* interface)
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
    
    // 根据当前令牌类型决定解析什么类型的语句
    switch (state->current_token->type)
    {
        case Eum_TOKEN_DELIMITER_SEMICOLON:
            // 空语句
            return F_parse_empty_statement(interface);
            
        case Eum_TOKEN_DELIMITER_LBRACE:
            // 代码块语句
            return F_parse_block_statement(interface);
            
        case Eum_TOKEN_KEYWORD_IF:
            // 条件语句
            return F_parse_if_statement(interface);
            
        case Eum_TOKEN_KEYWORD_WHILE:
            // while循环语句
            return F_parse_while_statement(interface);
            
        case Eum_TOKEN_KEYWORD_FOR:
            // for循环语句
            return F_parse_for_statement(interface);
            
        case Eum_TOKEN_KEYWORD_BREAK:
            // 中断语句
            return F_parse_break_statement(interface);
            
        case Eum_TOKEN_KEYWORD_CONTINUE:
            // 继续语句
            return F_parse_continue_statement(interface);
            
        case Eum_TOKEN_KEYWORD_RETURN:
            // 返回语句
            return F_parse_return_statement(interface);
            
        case Eum_TOKEN_KEYWORD_VAR:
            // 变量声明语句
            return F_parse_variable_declaration_statement(interface);
            
        default:
            // 表达式语句
            return F_parse_expression_statement(interface);
    }
}

/**
 * @brief 解析变量声明语句
 */
Stru_AstNode_t* F_parse_variable_declaration_statement(Stru_ParserInterface_t* interface)
{
    // 变量声明语句实际上是一个声明，委托给声明解析模块
    return F_parse_declaration(interface);
}

/**
 * @brief 解析语句列表
 */
Stru_DynamicArray_t* F_parse_statement_list(Stru_ParserInterface_t* interface)
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
    
    // 创建语句数组
    Stru_DynamicArray_t* statements = F_create_dynamic_array(sizeof(Stru_AstNode_t*));
    if (statements == NULL)
    {
        return NULL;
    }
    
    // 解析语句直到遇到右花括号或文件结束
    while (state->current_token != NULL && 
           state->current_token->type != Eum_TOKEN_DELIMITER_RBRACE)
    {
        // 解析语句
        Stru_AstNode_t* statement = F_parse_statement(interface);
        if (statement == NULL)
        {
            // 报告错误但继续尝试恢复
            F_report_invalid_statement_error(interface,
                                            state->current_token->line,
                                            state->current_token->column,
                                            "语句无效",
                                            state->current_token);
            
            // 尝试同步到安全点
            Eum_TokenType sync_tokens[] = {Eum_TOKEN_DELIMITER_SEMICOLON, Eum_TOKEN_DELIMITER_RBRACE};
            F_synchronize_to_safe_point(interface, sync_tokens, 2);
            
            // 如果当前令牌是右花括号，跳出循环
            if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_DELIMITER_RBRACE)
            {
                break;
            }
            
            continue;
        }
        
        F_dynamic_array_push(statements, &statement);
    }
    
    return statements;
}
