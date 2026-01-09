/**
 * @file CN_parser_variable_declarations.c
 * @brief CN_Language 变量声明解析模块
 * 
 * 变量声明解析模块，负责解析变量声明。
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
 * @brief 解析变量声明（内部实现）
 */
static Stru_AstNode_t* parse_variable_declaration_impl(Stru_ParserInterface_t* interface);

// ============================================
// 变量声明解析函数实现
// ============================================

/**
 * @brief 解析变量声明
 * 
 * 解析变量声明，如：变量 名称 = 值;。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 变量声明AST节点
 */
Stru_AstNode_t* F_parse_variable_declaration(Stru_ParserInterface_t* interface)
{
    return parse_variable_declaration_impl(interface);
}

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 解析变量声明（内部实现）
 */
static Stru_AstNode_t* parse_variable_declaration_impl(Stru_ParserInterface_t* interface)
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
    
    // 检查变量关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_VAR)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "变量");
        return NULL;
    }
    
    // 保存变量关键字令牌
    Stru_Token_t* var_token = state->current_token;
    
    // 消耗变量关键字
    F_advance_token(state);
    
    // 解析变量名称（标识符）
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_IDENTIFIER)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "标识符");
        return NULL;
    }
    
    // 保存变量名称令牌
    Stru_Token_t* var_name_token = state->current_token;
    const char* var_name = var_name_token->lexeme;
    
    // 消耗变量名称
    F_advance_token(state);
    
    // 检查类型注解（可选）
    Stru_AstNode_t* type_node = NULL;
    if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_DELIMITER_COLON)
    {
        // 消耗冒号
        F_advance_token(state);
        
        // 解析类型表达式
        type_node = F_parse_type_expression(interface);
        if (type_node == NULL)
        {
            // 报告错误
            F_report_invalid_declaration_error(interface,
                                       state->current_token->line,
                                       state->current_token->column,
                                       "变量类型无效",
                                       state->current_token);
            return NULL;
        }
    }
    
    // 检查初始化（可选）
    Stru_AstNode_t* initializer = NULL;
    if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_OPERATOR_ASSIGN)
    {
        // 消耗赋值运算符
        F_advance_token(state);
        
        // 解析初始化表达式
        initializer = F_parser_parse_expression(interface);
        if (initializer == NULL)
        {
            // 报告错误
            F_report_invalid_declaration_error(interface,
                                             state->current_token->line,
                                             state->current_token->column,
                                             "变量初始化表达式无效",
                                             state->current_token);
            
            // 清理类型节点
            if (type_node != NULL)
            {
                F_destroy_ast_node(type_node);
            }
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
        
        // 清理类型节点和初始化节点
        if (type_node != NULL)
        {
            F_destroy_ast_node(type_node);
        }
        if (initializer != NULL)
        {
            F_destroy_ast_node(initializer);
        }
        return NULL;
    }
    
    // 消耗分号
    F_advance_token(state);
    
    // 创建变量声明节点
    Stru_AstNode_t* var_node = F_parser_create_ast_node(interface, Eum_AST_VARIABLE_DECL, var_token);
    
    // 设置变量名称
    F_parser_set_node_attribute(interface, var_node, "name", (void*)var_name);
    
    // 添加类型和初始化器作为子节点
    if (type_node != NULL)
    {
        F_parser_add_child_node(interface, var_node, type_node);
    }
    
    if (initializer != NULL)
    {
        F_parser_add_child_node(interface, var_node, initializer);
    }
    
    return var_node;
}
