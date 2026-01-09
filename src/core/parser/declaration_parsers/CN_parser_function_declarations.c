/**
 * @file CN_parser_function_declarations.c
 * @brief CN_Language 函数声明解析模块
 * 
 * 函数声明解析模块，负责解析函数声明。
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
 * @brief 解析函数声明（内部实现）
 */
static Stru_AstNode_t* parse_function_declaration_impl(Stru_ParserInterface_t* interface);

// ============================================
// 函数声明解析函数实现
// ============================================

/**
 * @brief 解析函数声明
 * 
 * 解析函数声明，如：函数 名称(参数) { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 函数声明AST节点
 */
Stru_AstNode_t* F_parse_function_declaration(Stru_ParserInterface_t* interface)
{
    return parse_function_declaration_impl(interface);
}

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 解析函数声明（内部实现）
 */
static Stru_AstNode_t* parse_function_declaration_impl(Stru_ParserInterface_t* interface)
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
    
    // 检查函数关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_FUNCTION)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "函数");
        return NULL;
    }
    
    // 保存函数关键字令牌
    Stru_Token_t* function_token = state->current_token;
    
    // 消耗函数关键字
    F_advance_token(state);
    
    // 解析函数名称（标识符）
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_IDENTIFIER)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "标识符");
        return NULL;
    }
    
    // 保存函数名称令牌
    Stru_Token_t* func_name_token = state->current_token;
    const char* func_name = func_name_token->lexeme;
    
    // 消耗函数名称
    F_advance_token(state);
    
    // 解析参数列表
    Stru_DynamicArray_t* parameters = F_parse_parameter_list(interface);
    if (parameters == NULL)
    {
        // 报告错误
        F_report_invalid_declaration_error(interface,
                                          state->current_token->line,
                                          state->current_token->column,
                                          "函数参数列表无效",
                                          state->current_token);
        return NULL;
    }
    
    // 检查返回类型注解（可选）
    Stru_AstNode_t* return_type = NULL;
    if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_DELIMITER_COLON)
    {
        // 消耗冒号
        F_advance_token(state);
        
        // 解析返回类型
        return_type = F_parse_type_expression(interface);
        if (return_type == NULL)
        {
            // 报告错误
            F_report_invalid_declaration_error(interface,
                                              state->current_token->line,
                                              state->current_token->column,
                                              "函数返回类型无效",
                                              state->current_token);
            
            // 清理参数数组
            F_destroy_dynamic_array(parameters);
            return NULL;
        }
    }
    
    // 解析函数体（代码块）
    Stru_AstNode_t* body = F_parser_parse_statement(interface);
    if (body == NULL)
    {
        // 报告错误
        F_report_invalid_declaration_error(interface,
                                          state->current_token->line,
                                          state->current_token->column,
                                          "函数体无效",
                                          state->current_token);
        
        // 清理参数数组和返回类型
        F_destroy_dynamic_array(parameters);
        if (return_type != NULL)
        {
            F_destroy_ast_node(return_type);
        }
        return NULL;
    }
    
    // 创建函数声明节点
    Stru_AstNode_t* func_node = F_parser_create_ast_node(interface, Eum_AST_FUNCTION_DECL, function_token);
    
    // 设置函数名称
    F_parser_set_node_attribute(interface, func_node, "name", (void*)func_name);
    
    // 设置参数列表作为属性
    F_parser_set_node_attribute(interface, func_node, "parameters", parameters);
    
    // 添加返回类型和函数体作为子节点
    if (return_type != NULL)
    {
        F_parser_add_child_node(interface, func_node, return_type);
    }
    
    F_parser_add_child_node(interface, func_node, body);
    
    return func_node;
}
