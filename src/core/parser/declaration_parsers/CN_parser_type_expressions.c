/**
 * @file CN_parser_type_expressions.c
 * @brief CN_Language 类型表达式解析模块
 * 
 * 类型表达式解析模块，负责解析类型表达式。
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
#include <string.h>

// ============================================
// 类型表达式解析函数实现
// ============================================

/**
 * @brief 解析类型表达式
 * 
 * 解析类型表达式，如：整数[], 字符串指针。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 类型表达式AST节点
 */
Stru_AstNode_t* F_parse_type_expression(Stru_ParserInterface_t* interface)
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
    
    // 首先解析基本类型
    Stru_AstNode_t* type_node = F_parse_basic_type(interface);
    if (type_node == NULL)
    {
        return NULL;
    }
    
    // 检查是否有数组或指针修饰符
    while (state->current_token != NULL)
    {
        if (state->current_token->type == Eum_TOKEN_DELIMITER_LBRACKET)
        {
            // 数组类型
            Stru_AstNode_t* array_type = F_parse_array_type(interface);
            if (array_type == NULL)
            {
                // 清理基本类型节点
                F_destroy_ast_node(type_node);
                return NULL;
            }
            
            // 将基本类型作为数组类型的子节点
            F_parser_add_child_node(interface, array_type, type_node);
            type_node = array_type;
        }
        else if (state->current_token->type == Eum_TOKEN_KEYWORD_POINTER)
        {
            // 指针类型
            Stru_AstNode_t* pointer_type = F_parse_pointer_type(interface);
            if (pointer_type == NULL)
            {
                // 清理类型节点
                F_destroy_ast_node(type_node);
                return NULL;
            }
            
            // 将当前类型作为指针类型的子节点
            F_parser_add_child_node(interface, pointer_type, type_node);
            type_node = pointer_type;
        }
        else
        {
            // 没有更多修饰符
            break;
        }
    }
    
    return type_node;
}

/**
 * @brief 解析基本类型
 * 
 * 解析基本类型，如：整数, 浮点数, 字符串, 布尔值。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 基本类型AST节点
 */
Stru_AstNode_t* F_parse_basic_type(Stru_ParserInterface_t* interface)
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
    
    // 检查是否为基本类型关键字
    Eum_TokenType token_type = state->current_token->type;
    const char* type_name = NULL;
    
    switch (token_type)
    {
        case Eum_TOKEN_KEYWORD_INT:
            type_name = "整数";
            break;
        case Eum_TOKEN_KEYWORD_FLOAT:
            type_name = "浮点数";
            break;
        case Eum_TOKEN_KEYWORD_STRING:
            type_name = "字符串";
            break;
        case Eum_TOKEN_KEYWORD_BOOL:
            type_name = "布尔值";
            break;
        case Eum_TOKEN_KEYWORD_VOID:
            type_name = "空";
            break;
        case Eum_TOKEN_IDENTIFIER:
            // 用户定义的类型
            type_name = state->current_token->lexeme;
            break;
        default:
            // 报告错误
            F_report_unexpected_token_error(interface, state->current_token, "类型名称");
            return NULL;
    }
    
    // 保存类型令牌
    Stru_Token_t* type_token = state->current_token;
    
    // 消耗类型令牌
    F_advance_token(state);
    
    // 创建类型节点
    Stru_AstNode_t* type_node = F_parser_create_ast_node(interface, Eum_AST_TYPE_NAME, type_token);
    
    // 设置类型名称
    F_parser_set_node_attribute(interface, type_node, "name", (void*)type_name);
    
    return type_node;
}

/**
 * @brief 解析数组类型
 * 
 * 解析数组类型，如：整数[], 字符串[10]。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 数组类型AST节点
 */
Stru_AstNode_t* F_parse_array_type(Stru_ParserInterface_t* interface)
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
    
    // 检查左方括号
    if (state->current_token->type != Eum_TOKEN_DELIMITER_LBRACKET)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "[");
        return NULL;
    }
    
    // 保存左方括号令牌
    Stru_Token_t* lbracket_token = state->current_token;
    
    // 消耗左方括号
    F_advance_token(state);
    
    // 检查数组大小（可选）
    size_t array_size = 0;
    bool has_size = false;
    
    if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_LITERAL_INTEGER)
    {
        // 解析数组大小
        char* endptr;
        array_size = strtoul(state->current_token->lexeme, &endptr, 10);
        has_size = true;
        
        // 消耗整数令牌
        F_advance_token(state);
    }
    
    // 检查右方括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_RBRACKET)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    "]");
        return NULL;
    }
    
    // 消耗右方括号
    F_advance_token(state);
    
    // 创建数组类型节点
    Stru_AstNode_t* array_node = F_parser_create_ast_node(interface, Eum_AST_ARRAY_TYPE, lbracket_token);
    
    // 设置数组大小（如果有）
    if (has_size)
    {
        F_parser_set_node_attribute(interface, array_node, "size", (void*)(uintptr_t)array_size);
    }
    
    return array_node;
}

/**
 * @brief 解析指针类型
 * 
 * 解析指针类型，如：整数指针, 字符串指针。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 指针类型AST节点
 */
Stru_AstNode_t* F_parse_pointer_type(Stru_ParserInterface_t* interface)
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
    
    // 检查指针关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_POINTER)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "指针");
        return NULL;
    }
    
    // 保存指针关键字令牌
    Stru_Token_t* pointer_token = state->current_token;
    
    // 消耗指针关键字
    F_advance_token(state);
    
    // 创建指针类型节点
    Stru_AstNode_t* pointer_node = F_parser_create_ast_node(interface, Eum_AST_POINTER_TYPE, pointer_token);
    
    return pointer_node;
}
