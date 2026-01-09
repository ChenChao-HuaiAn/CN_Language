/**
 * @file CN_parser_static_declarations.c
 * @brief CN_Language 静态声明解析模块
 * 
 * 静态声明解析模块，负责解析静态变量和静态函数声明。
 * 静态变量声明语法：静态 变量 名称 = 值;
 * 静态函数声明语法：静态 函数 名称(参数) { ... }
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
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
#include "../statement_parsers/CN_parser_statements.h"
#include <stdlib.h>

// ============================================
// 内部辅助函数声明
// ============================================

/**
 * @brief 解析静态变量声明（内部实现）
 */
static Stru_AstNode_t* parse_static_variable_declaration_impl(Stru_ParserInterface_t* interface);

/**
 * @brief 解析静态函数声明（内部实现）
 */
static Stru_AstNode_t* parse_static_function_declaration_impl(Stru_ParserInterface_t* interface);

// ============================================
// 静态声明解析函数实现
// ============================================

/**
 * @brief 解析静态声明
 * 
 * 解析静态变量或静态函数声明。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 静态声明AST节点
 */
Stru_AstNode_t* F_parse_static_declaration(Stru_ParserInterface_t* interface)
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
    
    // 检查静态关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_STATIC)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "静态");
        return NULL;
    }
    
    // 消耗静态关键字
    F_advance_token(state);
    
    // 检查下一个令牌类型，确定是变量还是函数声明
    if (state->current_token == NULL)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "变量或函数");
        return NULL;
    }
    
    // 根据下一个令牌类型决定解析什么类型的声明
    switch (state->current_token->type)
    {
        case Eum_TOKEN_KEYWORD_VAR:
            // 静态变量声明
            return parse_static_variable_declaration_impl(interface);
            
        case Eum_TOKEN_KEYWORD_FUNCTION:
            // 静态函数声明
            return parse_static_function_declaration_impl(interface);
            
        default:
            // 报告错误
            F_report_unexpected_token_error(interface, state->current_token, "变量或函数");
            return NULL;
    }
}

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 解析静态变量声明（内部实现）
 */
static Stru_AstNode_t* parse_static_variable_declaration_impl(Stru_ParserInterface_t* interface)
{
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
                                       "静态变量类型无效",
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
                                             "静态变量初始化表达式无效",
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
    
    // 创建静态变量声明节点
    Stru_AstNode_t* static_var_node = F_parser_create_ast_node(interface, Eum_AST_VARIABLE_DECL, var_token);
    
    // 设置变量名称
    F_parser_set_node_attribute(interface, static_var_node, "name", (void*)var_name);
    
    // 设置静态修饰符属性
    F_parser_set_node_attribute(interface, static_var_node, "is_static", (void*)1);
    
    // 添加类型和初始化器作为子节点
    if (type_node != NULL)
    {
        F_parser_add_child_node(interface, static_var_node, type_node);
    }
    
    if (initializer != NULL)
    {
        F_parser_add_child_node(interface, static_var_node, initializer);
    }
    
    return static_var_node;
}

/**
 * @brief 解析静态函数声明（内部实现）
 */
static Stru_AstNode_t* parse_static_function_declaration_impl(Stru_ParserInterface_t* interface)
{
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
    Stru_Token_t* function_name_token = state->current_token;
    const char* function_name = function_name_token->lexeme;
    
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
    
    // 检查返回类型（可选）
    Stru_AstNode_t* return_type_node = NULL;
    if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_DELIMITER_COLON)
    {
        // 消耗冒号
        F_advance_token(state);
        
        // 解析返回类型表达式
        return_type_node = F_parse_type_expression(interface);
        if (return_type_node == NULL)
        {
            // 报告错误
            F_report_invalid_declaration_error(interface,
                                           state->current_token->line,
                                           state->current_token->column,
                                           "函数返回类型无效",
                                           state->current_token);
            
            // 清理参数列表
            F_destroy_dynamic_array(parameters);
            return NULL;
        }
    }
    
    // 检查函数体
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_LBRACE)
    {
        // 报告错误
        F_report_missing_token_error(interface,
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    "{");
        
        // 清理参数列表和返回类型节点
        F_destroy_dynamic_array(parameters);
        if (return_type_node != NULL)
        {
            F_destroy_ast_node(return_type_node);
        }
        return NULL;
    }
    
    // 消耗左花括号
    F_advance_token(state);
    
    // 解析函数体（语句块）
    Stru_AstNode_t* function_body = F_parse_block_statement(interface);
    if (function_body == NULL)
    {
        // 报告错误
        F_report_invalid_declaration_error(interface,
                                         state->current_token->line,
                                         state->current_token->column,
                                         "函数体无效",
                                         state->current_token);
        
        // 清理参数列表和返回类型节点
        F_destroy_dynamic_array(parameters);
        if (return_type_node != NULL)
        {
            F_destroy_ast_node(return_type_node);
        }
        return NULL;
    }
    
    // 检查右花括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_RBRACE)
    {
        // 报告错误
        F_report_missing_token_error(interface,
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    "}");
        
        // 清理参数列表、返回类型节点和函数体
        F_destroy_dynamic_array(parameters);
        if (return_type_node != NULL)
        {
            F_destroy_ast_node(return_type_node);
        }
        if (function_body != NULL)
        {
            F_destroy_ast_node(function_body);
        }
        return NULL;
    }
    
    // 消耗右花括号
    F_advance_token(state);
    
    // 创建静态函数声明节点
    Stru_AstNode_t* static_function_node = F_parser_create_ast_node(interface, Eum_AST_FUNCTION_DECL, function_token);
    
    // 设置函数名称
    F_parser_set_node_attribute(interface, static_function_node, "name", (void*)function_name);
    
    // 设置静态修饰符属性
    F_parser_set_node_attribute(interface, static_function_node, "is_static", (void*)1);
    
    // 添加参数、返回类型和函数体作为子节点
    // 注意：这里需要将参数列表转换为AST节点数组
    // 在实际实现中，可能需要创建专门的参数列表节点
    
    if (return_type_node != NULL)
    {
        F_parser_add_child_node(interface, static_function_node, return_type_node);
    }
    
    if (function_body != NULL)
    {
        F_parser_add_child_node(interface, static_function_node, function_body);
    }
    
    // 清理参数列表（在实际实现中，参数列表应该被转换为AST节点并添加到函数节点中）
    F_destroy_dynamic_array(parameters);
    
    return static_function_node;
}
