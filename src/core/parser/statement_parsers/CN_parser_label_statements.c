/**
 * @file CN_parser_label_statements.c
 * @brief CN_Language 标签语句解析模块实现
 * 
 * 标签语句解析功能模块的实现，负责解析标签语句和goto语句。
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
#include "../../token/CN_token_types.h"
#include "../../ast/CN_ast.h"
#include <stdlib.h>
#include <string.h>

// ============================================
// 标签语句解析函数实现
// ============================================

/**
 * @brief 解析标签语句
 * 
 * 解析标签语句，如：标签: 语句。
 * 标签语句由标签名称、冒号和语句组成。
 */
Stru_AstNode_t* F_parse_label_statement(Stru_ParserInterface_t* interface)
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
    
    // 检查是否为标签关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_LABEL)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "标签");
        return NULL;
    }
    
    // 保存标签关键字令牌
    Stru_Token_t* label_keyword_token = state->current_token;
    
    // 消耗标签关键字
    F_advance_token(state);
    
    // 检查下一个令牌是否为标识符（标签名称）
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_IDENTIFIER)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "标识符（标签名称）");
        return NULL;
    }
    
    // 保存标签名称令牌
    Stru_Token_t* label_name_token = state->current_token;
    
    // 消耗标签名称
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
    
    // 解析标签后的语句
    Stru_AstNode_t* labeled_statement = F_parse_statement(interface);
    if (labeled_statement == NULL)
    {
        // 报告错误
        F_report_invalid_statement_error(interface,
                                        state->current_token->line,
                                        state->current_token->column,
                                        "标签后的语句无效",
                                        state->current_token);
        return NULL;
    }
    
    // 创建标签语句节点
    // 注意：AST节点类型中可能没有专门的标签语句类型，我们可以使用表达式语句类型
    // 但标记为标签语句
    Stru_AstNode_t* label_node = F_parser_create_ast_node(interface, Eum_AST_EXPRESSION_STMT, label_keyword_token);
    
    // 标记为标签语句
    F_parser_set_node_attribute(interface, label_node, "is_label", (void*)true);
    
    // 设置标签名称
    F_parser_set_node_attribute(interface, label_node, "label_name", (void*)label_name_token->lexeme);
    
    // 添加标签后的语句作为子节点
    F_parser_add_child_node(interface, label_node, labeled_statement);
    
    return label_node;
}

/**
 * @brief 解析goto语句
 * 
 * 解析goto语句，如：转到 标签;。
 * goto语句由goto关键字、标签名称和分号组成。
 */
Stru_AstNode_t* F_parse_goto_statement(Stru_ParserInterface_t* interface)
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
    
    // 检查是否为goto关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_GOTO)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "转到");
        return NULL;
    }
    
    // 保存goto关键字令牌
    Stru_Token_t* goto_token = state->current_token;
    
    // 消耗goto关键字
    F_advance_token(state);
    
    // 检查下一个令牌是否为标识符（标签名称）
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_IDENTIFIER)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "标识符（标签名称）");
        return NULL;
    }
    
    // 保存标签名称令牌
    Stru_Token_t* label_name_token = state->current_token;
    
    // 消耗标签名称
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
    
    // 创建goto语句节点
    // 注意：AST节点类型中可能没有专门的goto语句类型，我们可以使用表达式语句类型
    // 但标记为goto语句
    Stru_AstNode_t* goto_node = F_parser_create_ast_node(interface, Eum_AST_EXPRESSION_STMT, goto_token);
    
    // 标记为goto语句
    F_parser_set_node_attribute(interface, goto_node, "is_goto", (void*)true);
    
    // 设置目标标签名称
    F_parser_set_node_attribute(interface, goto_node, "target_label", (void*)label_name_token->lexeme);
    
    return goto_node;
}
