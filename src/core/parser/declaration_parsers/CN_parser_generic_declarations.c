/**
 * @file CN_parser_generic_declarations.c
 * @brief CN_Language 泛型声明解析模块
 * 
 * 泛型声明解析模块，负责解析泛型声明。
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
 * @brief 解析泛型声明（内部实现）
 */
static Stru_AstNode_t* parse_generic_declaration_impl(Stru_ParserInterface_t* interface);

/**
 * @brief 解析泛型参数列表
 */
static Stru_DynamicArray_t* parse_generic_parameter_list(Stru_ParserInterface_t* interface);

// ============================================
// 泛型声明解析函数实现
// ============================================

/**
 * @brief 解析泛型声明
 * 
 * 解析泛型声明，如：泛型 名称<T> { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 泛型声明AST节点
 */
Stru_AstNode_t* F_parse_generic_declaration(Stru_ParserInterface_t* interface)
{
    return parse_generic_declaration_impl(interface);
}

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 解析泛型声明（内部实现）
 */
static Stru_AstNode_t* parse_generic_declaration_impl(Stru_ParserInterface_t* interface)
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
    
    // 检查泛型关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_GENERIC)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "泛型");
        return NULL;
    }
    
    // 保存泛型关键字令牌
    Stru_Token_t* generic_token = state->current_token;
    
    // 消耗泛型关键字
    F_advance_token(state);
    
    // 解析泛型名称（标识符）
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_IDENTIFIER)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "标识符");
        return NULL;
    }
    
    // 保存泛型名称令牌
    Stru_Token_t* generic_name_token = state->current_token;
    const char* generic_name = generic_name_token->lexeme;
    
    // 消耗泛型名称
    F_advance_token(state);
    
    // 解析泛型参数列表（可选）
    Stru_DynamicArray_t* generic_params = NULL;
    
    if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_OPERATOR_LESS)
    {
        // 消耗小于号
        F_advance_token(state);
        
        // 解析泛型参数列表
        generic_params = parse_generic_parameter_list(interface);
        
        if (generic_params == NULL)
        {
            // 报告错误
            F_report_invalid_declaration_error(interface,
                                                 state->current_token->line,
                                                 state->current_token->column,
                                                 "泛型参数列表无效",
                                                 state->current_token);
            return NULL;
        }
        
        // 检查大于号
        if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_OPERATOR_GREATER)
        {
            // 报告错误
            F_report_unexpected_token_error(interface, state->current_token, ">");
            
            // 清理内存
            if (generic_params != NULL)
            {
                F_destroy_dynamic_array(generic_params);
            }
            return NULL;
        }
        
        // 消耗大于号
        F_advance_token(state);
    }
    
    // 检查左大括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_LBRACE)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "{");
        
        // 清理内存
        if (generic_params != NULL)
        {
            F_destroy_dynamic_array(generic_params);
        }
        return NULL;
    }
    
    // 消耗左大括号
    F_advance_token(state);
    
    // 创建动态数组来存储泛型成员
    Stru_DynamicArray_t* members = F_create_dynamic_array_ex(sizeof(Stru_AstNode_t*), 10, NULL);
    if (members == NULL)
    {
        // 报告内存分配错误
        F_report_invalid_declaration_error(interface,
                                             state->current_token->line,
                                             state->current_token->column,
                                             "无法分配内存存储泛型成员",
                                             state->current_token);
        
        // 清理内存
        if (generic_params != NULL)
        {
            F_destroy_dynamic_array(generic_params);
        }
        return NULL;
    }
    
    // 解析泛型成员（可以是类、结构体、函数等）
    while (state->current_token != NULL && state->current_token->type != Eum_TOKEN_DELIMITER_RBRACE)
    {
        // 根据下一个令牌决定解析哪种声明
        Stru_AstNode_t* member_decl = NULL;
        
        if (state->current_token != NULL)
        {
            switch (state->current_token->type)
            {
                case Eum_TOKEN_KEYWORD_CLASS:
                    // 解析类声明
                    member_decl = F_parse_class_declaration(interface);
                    break;
                    
                case Eum_TOKEN_KEYWORD_STRUCT:
                    // 解析结构体声明
                    member_decl = F_parse_struct_declaration(interface);
                    break;
                    
                case Eum_TOKEN_KEYWORD_INTERFACE:
                    // 解析接口声明
                    member_decl = F_parse_interface_declaration(interface);
                    break;
                    
                case Eum_TOKEN_KEYWORD_FUNCTION:
                    // 解析函数声明
                    member_decl = F_parse_function_declaration(interface);
                    break;
                    
                case Eum_TOKEN_KEYWORD_VAR:
                case Eum_TOKEN_KEYWORD_INT:
                case Eum_TOKEN_KEYWORD_FLOAT:
                case Eum_TOKEN_KEYWORD_STRING:
                case Eum_TOKEN_KEYWORD_BOOL:
                    // 解析变量声明
                    member_decl = F_parse_variable_declaration(interface);
                    break;
                    
                default:
                    // 无法识别的成员类型，尝试错误恢复
                    F_try_error_recovery(interface, NULL);
                    break;
            }
        }
        
        if (member_decl == NULL)
        {
            // 尝试错误恢复
            F_try_error_recovery(interface, NULL);
            break;
        }
        
        // 添加到成员数组
        F_dynamic_array_push(members, &member_decl);
        
        // 检查分号（泛型成员声明以分号结束）
        if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_DELIMITER_SEMICOLON)
        {
            F_advance_token(state);
        }
    }
    
    // 检查右大括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_RBRACE)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "}");
        
        // 清理内存
        F_destroy_dynamic_array(members);
        if (generic_params != NULL)
        {
            F_destroy_dynamic_array(generic_params);
        }
        return NULL;
    }
    
    // 消耗右大括号
    F_advance_token(state);
    
    // 创建泛型声明节点
    // 注意：暂时使用Eum_AST_STRUCT_DECL作为泛型声明节点类型
    // 后续可以添加专门的Eum_AST_GENERIC_DECL节点类型
    Stru_AstNode_t* generic_node = F_parser_create_ast_node(interface, Eum_AST_STRUCT_DECL, generic_token);
    
    // 设置泛型名称
    F_parser_set_node_attribute(interface, generic_node, "name", (void*)generic_name);
    
    // 设置泛型参数列表作为属性
    if (generic_params != NULL)
    {
        F_parser_set_node_attribute(interface, generic_node, "generic_params", generic_params);
    }
    
    // 设置成员列表作为属性
    F_parser_set_node_attribute(interface, generic_node, "members", members);
    
    // 设置节点类型为泛型
    F_parser_set_node_attribute(interface, generic_node, "declaration_type", (void*)"generic");
    
    return generic_node;
}

/**
 * @brief 解析泛型参数列表
 */
static Stru_DynamicArray_t* parse_generic_parameter_list(Stru_ParserInterface_t* interface)
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
    
    // 创建动态数组来存储泛型参数
    Stru_DynamicArray_t* params = F_create_dynamic_array_ex(sizeof(char*), 5, NULL);
    if (params == NULL)
    {
        return NULL;
    }
    
    // 解析第一个参数
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_IDENTIFIER)
    {
        F_destroy_dynamic_array(params);
        return NULL;
    }
    
    // 保存第一个参数
    const char* first_param = state->current_token->lexeme;
    F_dynamic_array_push(params, &first_param);
    
    // 消耗第一个参数
    F_advance_token(state);
    
    // 解析更多参数（用逗号分隔）
    while (state->current_token != NULL && state->current_token->type == Eum_TOKEN_DELIMITER_COMMA)
    {
        // 消耗逗号
        F_advance_token(state);
        
        // 解析参数
        if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_IDENTIFIER)
        {
            // 报告错误
            F_report_unexpected_token_error(interface, state->current_token, "标识符");
            F_destroy_dynamic_array(params);
            return NULL;
        }
        
        // 保存参数
        const char* param = state->current_token->lexeme;
        F_dynamic_array_push(params, &param);
        
        // 消耗参数
        F_advance_token(state);
    }
    
    return params;
}
