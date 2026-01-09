/**
 * @file CN_parser_type_declarations.c
 * @brief CN_Language 类型声明解析模块
 * 
 * 类型声明解析模块，负责解析类型声明。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_parser_declarations.h"
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
 * @brief 解析类型声明（内部实现）
 */
static Stru_AstNode_t* parse_type_declaration_impl(Stru_ParserInterface_t* interface);

// ============================================
// 类型声明解析函数实现
// ============================================

/**
 * @brief 解析类型声明
 * 
 * 解析类型声明，如：类型 名称 = 类型表达式;。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 类型声明AST节点
 */
Stru_AstNode_t* F_parse_type_declaration(Stru_ParserInterface_t* interface)
{
    return parse_type_declaration_impl(interface);
}

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 解析类型声明（内部实现）
 */
static Stru_AstNode_t* parse_type_declaration_impl(Stru_ParserInterface_t* interface)
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
    
    // 检查类型关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_TYPE)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "类型");
        return NULL;
    }
    
    // 保存类型关键字令牌
    Stru_Token_t* type_token = state->current_token;
    
    // 消耗类型关键字
    F_advance_token(state);
    
    // 解析类型名称（标识符）
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_IDENTIFIER)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "标识符");
        return NULL;
    }
    
    // 保存类型名称令牌
    Stru_Token_t* type_name_token = state->current_token;
    const char* type_name = type_name_token->lexeme;
    
    // 消耗类型名称
    F_advance_token(state);
    
    // 检查赋值运算符
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_OPERATOR_ASSIGN)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    "=");
        return NULL;
    }
    
    // 消耗赋值运算符
    F_advance_token(state);
    
    // 解析类型表达式
    Stru_AstNode_t* type_expression = F_parse_type_expression(interface);
    if (type_expression == NULL)
    {
        // 报告错误
        F_report_invalid_type_error(interface,
                                   state->current_token->line,
                                   state->current_token->column,
                                   "类型表达式无效",
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
        
        // 清理类型表达式节点
        F_destroy_ast_node(type_expression);
        return NULL;
    }
    
    // 消耗分号
    F_advance_token(state);
    
    // 创建类型声明节点
    Stru_AstNode_t* type_decl_node = F_parser_create_ast_node(interface, Eum_AST_TYPE_NAME, type_token);
    
    // 设置类型名称
    F_parser_set_node_attribute(interface, type_decl_node, "name", (void*)type_name);
    
    // 添加类型表达式作为子节点
    F_parser_add_child_node(interface, type_decl_node, type_expression);
    
    return type_decl_node;
}
