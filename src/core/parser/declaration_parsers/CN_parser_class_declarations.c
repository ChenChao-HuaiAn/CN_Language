/**
 * @file CN_parser_class_declarations.c
 * @brief CN_Language 类声明解析模块
 * 
 * 类声明解析模块，负责解析类声明。
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
 * @brief 解析类声明（内部实现）
 */
static Stru_AstNode_t* parse_class_declaration_impl(Stru_ParserInterface_t* interface);

// ============================================
// 类声明解析函数实现
// ============================================

/**
 * @brief 解析类声明
 * 
 * 解析类声明，如：类 名称 { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 类声明AST节点
 */
Stru_AstNode_t* F_parse_class_declaration(Stru_ParserInterface_t* interface)
{
    return parse_class_declaration_impl(interface);
}

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 解析类声明（内部实现）
 */
static Stru_AstNode_t* parse_class_declaration_impl(Stru_ParserInterface_t* interface)
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
    
    // 检查类关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_CLASS)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "类");
        return NULL;
    }
    
    // 保存类关键字令牌
    Stru_Token_t* class_token = state->current_token;
    
    // 消耗类关键字
    F_advance_token(state);
    
    // 解析类名称（标识符）
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_IDENTIFIER)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "标识符");
        return NULL;
    }
    
    // 保存类名称令牌
    Stru_Token_t* class_name_token = state->current_token;
    const char* class_name = class_name_token->lexeme;
    
    // 消耗类名称
    F_advance_token(state);
    
    // 检查左大括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_LBRACE)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "{");
        return NULL;
    }
    
    // 消耗左大括号
    F_advance_token(state);
    
    // 创建动态数组来存储类成员
    Stru_DynamicArray_t* members = F_create_dynamic_array_ex(sizeof(Stru_AstNode_t*), 10, NULL);
    if (members == NULL)
    {
        // 报告内存分配错误
        F_report_invalid_declaration_error(interface,
                                             state->current_token->line,
                                             state->current_token->column,
                                             "无法分配内存存储类成员",
                                             state->current_token);
        return NULL;
    }
    
    // 解析类成员（字段和方法）
    while (state->current_token != NULL && state->current_token->type != Eum_TOKEN_DELIMITER_RBRACE)
    {
        // 检查访问修饰符（可选）
        bool has_access_modifier = false;
        const char* access_modifier = NULL;
        
        if (state->current_token != NULL && 
            (state->current_token->type == Eum_TOKEN_KEYWORD_PUBLIC ||
             state->current_token->type == Eum_TOKEN_KEYWORD_PRIVATE ||
             state->current_token->type == Eum_TOKEN_KEYWORD_PROTECTED))
        {
            has_access_modifier = true;
            
            if (state->current_token->type == Eum_TOKEN_KEYWORD_PUBLIC)
                access_modifier = "public";
            else if (state->current_token->type == Eum_TOKEN_KEYWORD_PRIVATE)
                access_modifier = "private";
            else if (state->current_token->type == Eum_TOKEN_KEYWORD_PROTECTED)
                access_modifier = "protected";
            
            // 消耗访问修饰符
            F_advance_token(state);
        }
        
        // 检查其他修饰符（可选）
        bool has_static = false;
        bool has_virtual = false;
        bool has_abstract = false;
        bool has_final = false;
        
        while (state->current_token != NULL && 
               (state->current_token->type == Eum_TOKEN_KEYWORD_STATIC ||
                state->current_token->type == Eum_TOKEN_KEYWORD_VIRTUAL ||
                state->current_token->type == Eum_TOKEN_KEYWORD_ABSTRACT ||
                state->current_token->type == Eum_TOKEN_KEYWORD_FINAL))
        {
            if (state->current_token->type == Eum_TOKEN_KEYWORD_STATIC)
                has_static = true;
            else if (state->current_token->type == Eum_TOKEN_KEYWORD_VIRTUAL)
                has_virtual = true;
            else if (state->current_token->type == Eum_TOKEN_KEYWORD_ABSTRACT)
                has_abstract = true;
            else if (state->current_token->type == Eum_TOKEN_KEYWORD_FINAL)
                has_final = true;
            
            // 消耗修饰符
            F_advance_token(state);
        }
        
        // 解析成员声明（变量声明或函数声明）
        Stru_AstNode_t* member_decl = NULL;
        
        // 检查是否是变量声明
        if (state->current_token != NULL && 
            (state->current_token->type == Eum_TOKEN_KEYWORD_VAR ||
             state->current_token->type == Eum_TOKEN_KEYWORD_INT ||
             state->current_token->type == Eum_TOKEN_KEYWORD_FLOAT ||
             state->current_token->type == Eum_TOKEN_KEYWORD_STRING ||
             state->current_token->type == Eum_TOKEN_KEYWORD_BOOL))
        {
            // 解析变量声明
            member_decl = F_parse_variable_declaration(interface);
        }
        else if (state->current_token != NULL && 
                 state->current_token->type == Eum_TOKEN_KEYWORD_FUNCTION)
        {
            // 解析函数声明
            member_decl = F_parse_function_declaration(interface);
        }
        else
        {
            // 无法识别的成员类型，尝试错误恢复
            F_try_error_recovery(interface, NULL);
            break;
        }
        
        if (member_decl == NULL)
        {
            // 尝试错误恢复
            F_try_error_recovery(interface, NULL);
            break;
        }
        
        // 设置成员属性
        if (has_access_modifier)
        {
            F_parser_set_node_attribute(interface, member_decl, "access", (void*)access_modifier);
        }
        
        if (has_static)
        {
            F_parser_set_node_attribute(interface, member_decl, "static", (void*)"true");
        }
        
        if (has_virtual)
        {
            F_parser_set_node_attribute(interface, member_decl, "virtual", (void*)"true");
        }
        
        if (has_abstract)
        {
            F_parser_set_node_attribute(interface, member_decl, "abstract", (void*)"true");
        }
        
        if (has_final)
        {
            F_parser_set_node_attribute(interface, member_decl, "final", (void*)"true");
        }
        
        // 添加到成员数组
        F_dynamic_array_push(members, &member_decl);
        
        // 检查分号（类成员声明以分号结束）
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
        return NULL;
    }
    
    // 消耗右大括号
    F_advance_token(state);
    
    // 创建类声明节点
    // 注意：暂时使用Eum_AST_STRUCT_DECL作为类声明节点类型
    // 后续可以添加专门的Eum_AST_CLASS_DECL节点类型
    Stru_AstNode_t* class_node = F_parser_create_ast_node(interface, Eum_AST_STRUCT_DECL, class_token);
    
    // 设置类名称
    F_parser_set_node_attribute(interface, class_node, "name", (void*)class_name);
    
    // 设置成员列表作为属性
    F_parser_set_node_attribute(interface, class_node, "members", members);
    
    // 设置节点类型为类
    F_parser_set_node_attribute(interface, class_node, "declaration_type", (void*)"class");
    
    return class_node;
}
