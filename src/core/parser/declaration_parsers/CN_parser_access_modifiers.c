/**
 * @file CN_parser_access_modifiers.c
 * @brief CN_Language 访问修饰符解析模块
 * 
 * 访问修饰符解析模块，负责解析访问修饰符（公开、私有、保护）。
 * 访问修饰符语法：公开 变量 名称; 私有 函数 名称() { ... } 保护 结构体 名称 { ... }
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
#include <stdlib.h>

// ============================================
// 内部辅助函数声明
// ============================================

/**
 * @brief 解析访问修饰符声明（内部实现）
 */
static Stru_AstNode_t* parse_access_modifier_declaration_impl(Stru_ParserInterface_t* interface);

/**
 * @brief 解析公开声明（内部实现）
 */
static Stru_AstNode_t* parse_public_declaration_impl(Stru_ParserInterface_t* interface);

/**
 * @brief 解析私有声明（内部实现）
 */
static Stru_AstNode_t* parse_private_declaration_impl(Stru_ParserInterface_t* interface);

/**
 * @brief 解析保护声明（内部实现）
 */
static Stru_AstNode_t* parse_protected_declaration_impl(Stru_ParserInterface_t* interface);

// ============================================
// 访问修饰符解析函数实现
// ============================================

/**
 * @brief 解析访问修饰符声明
 * 
 * 解析访问修饰符声明，如：公开 变量 名称; 私有 函数 名称() { ... } 保护 结构体 名称 { ... }
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 访问修饰符声明AST节点
 */
Stru_AstNode_t* F_parse_access_modifier_declaration(Stru_ParserInterface_t* interface)
{
    return parse_access_modifier_declaration_impl(interface);
}

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 解析访问修饰符声明（内部实现）
 */
static Stru_AstNode_t* parse_access_modifier_declaration_impl(Stru_ParserInterface_t* interface)
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
    
    // 根据访问修饰符类型调用相应的解析函数
    switch (state->current_token->type)
    {
        case Eum_TOKEN_KEYWORD_PUBLIC:
            return parse_public_declaration_impl(interface);
            
        case Eum_TOKEN_KEYWORD_PRIVATE:
            return parse_private_declaration_impl(interface);
            
        case Eum_TOKEN_KEYWORD_PROTECTED:
            return parse_protected_declaration_impl(interface);
            
        default:
            // 不是访问修饰符，返回NULL
            return NULL;
    }
}

/**
 * @brief 解析公开声明（内部实现）
 */
static Stru_AstNode_t* parse_public_declaration_impl(Stru_ParserInterface_t* interface)
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
    
    // 检查公开关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_PUBLIC)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "公开");
        return NULL;
    }
    
    // 保存公开关键字令牌
    Stru_Token_t* public_token = state->current_token;
    
    // 消耗公开关键字
    F_advance_token(state);
    
    // 解析实际的声明（变量、函数、结构体等）
    Stru_AstNode_t* inner_declaration = F_parse_declaration(interface);
    if (inner_declaration == NULL)
    {
        // 报告错误
        F_report_invalid_declaration_error(interface,
                                         state->current_token->line,
                                         state->current_token->column,
                                         "公开声明无效",
                                         state->current_token);
        return NULL;
    }
    
    // 创建公开声明节点
    Stru_AstNode_t* public_node = F_parser_create_ast_node(interface, Eum_AST_VARIABLE_DECL, public_token);
    
    // 设置访问修饰符属性
    F_parser_set_node_attribute(interface, public_node, "access_modifier", (void*)"public");
    
    // 添加内部声明作为子节点
    F_parser_add_child_node(interface, public_node, inner_declaration);
    
    return public_node;
}

/**
 * @brief 解析私有声明（内部实现）
 */
static Stru_AstNode_t* parse_private_declaration_impl(Stru_ParserInterface_t* interface)
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
    
    // 检查私有关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_PRIVATE)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "私有");
        return NULL;
    }
    
    // 保存私有关键字令牌
    Stru_Token_t* private_token = state->current_token;
    
    // 消耗私有关键字
    F_advance_token(state);
    
    // 解析实际的声明（变量、函数、结构体等）
    Stru_AstNode_t* inner_declaration = F_parse_declaration(interface);
    if (inner_declaration == NULL)
    {
        // 报告错误
        F_report_invalid_declaration_error(interface,
                                         state->current_token->line,
                                         state->current_token->column,
                                         "私有声明无效",
                                         state->current_token);
        return NULL;
    }
    
    // 创建私有声明节点
    Stru_AstNode_t* private_node = F_parser_create_ast_node(interface, Eum_AST_VARIABLE_DECL, private_token);
    
    // 设置访问修饰符属性
    F_parser_set_node_attribute(interface, private_node, "access_modifier", (void*)"private");
    
    // 添加内部声明作为子节点
    F_parser_add_child_node(interface, private_node, inner_declaration);
    
    return private_node;
}

/**
 * @brief 解析保护声明（内部实现）
 */
static Stru_AstNode_t* parse_protected_declaration_impl(Stru_ParserInterface_t* interface)
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
    
    // 检查保护关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_PROTECTED)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "保护");
        return NULL;
    }
    
    // 保存保护关键字令牌
    Stru_Token_t* protected_token = state->current_token;
    
    // 消耗保护关键字
    F_advance_token(state);
    
    // 解析实际的声明（变量、函数、结构体等）
    Stru_AstNode_t* inner_declaration = F_parse_declaration(interface);
    if (inner_declaration == NULL)
    {
        // 报告错误
        F_report_invalid_declaration_error(interface,
                                         state->current_token->line,
                                         state->current_token->column,
                                         "保护声明无效",
                                         state->current_token);
        return NULL;
    }
    
    // 创建保护声明节点
    Stru_AstNode_t* protected_node = F_parser_create_ast_node(interface, Eum_AST_VARIABLE_DECL, protected_token);
    
    // 设置访问修饰符属性
    F_parser_set_node_attribute(interface, protected_node, "access_modifier", (void*)"protected");
    
    // 添加内部声明作为子节点
    F_parser_add_child_node(interface, protected_node, inner_declaration);
    
    return protected_node;
}
