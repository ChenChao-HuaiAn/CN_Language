/**
 * @file CN_parser_interface_declarations.c
 * @brief CN_Language 接口声明解析模块
 * 
 * 接口声明解析模块，负责解析接口声明。
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
 * @brief 解析接口声明（内部实现）
 */
static Stru_AstNode_t* parse_interface_declaration_impl(Stru_ParserInterface_t* interface);

// ============================================
// 接口声明解析函数实现
// ============================================

/**
 * @brief 解析接口声明
 * 
 * 解析接口声明，如：接口 名称 { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 接口声明AST节点
 */
Stru_AstNode_t* F_parse_interface_declaration(Stru_ParserInterface_t* interface)
{
    return parse_interface_declaration_impl(interface);
}

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 解析接口声明（内部实现）
 */
static Stru_AstNode_t* parse_interface_declaration_impl(Stru_ParserInterface_t* interface)
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
    
    // 检查接口关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_INTERFACE)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "接口");
        return NULL;
    }
    
    // 保存接口关键字令牌
    Stru_Token_t* interface_token = state->current_token;
    
    // 消耗接口关键字
    F_advance_token(state);
    
    // 解析接口名称（标识符）
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_IDENTIFIER)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "标识符");
        return NULL;
    }
    
    // 保存接口名称令牌
    Stru_Token_t* interface_name_token = state->current_token;
    const char* interface_name = interface_name_token->lexeme;
    
    // 消耗接口名称
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
    
    // 创建动态数组来存储接口成员
    Stru_DynamicArray_t* members = F_create_dynamic_array_ex(sizeof(Stru_AstNode_t*), 10, NULL);
    if (members == NULL)
    {
        // 报告内存分配错误
        F_report_invalid_declaration_error(interface,
                                             state->current_token->line,
                                             state->current_token->column,
                                             "无法分配内存存储接口成员",
                                             state->current_token);
        return NULL;
    }
    
    // 解析接口成员（方法声明）
    while (state->current_token != NULL && state->current_token->type != Eum_TOKEN_DELIMITER_RBRACE)
    {
        // 解析方法声明（简化版：只解析函数声明）
        Stru_AstNode_t* method_decl = F_parse_function_declaration(interface);
        if (method_decl == NULL)
        {
            // 尝试错误恢复
            F_try_error_recovery(interface, NULL);
            break;
        }
        
        // 添加到成员数组
        F_dynamic_array_push(members, &method_decl);
        
        // 检查分号（接口方法声明以分号结束）
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
    
    // 创建接口声明节点
    // 注意：暂时使用Eum_AST_STRUCT_DECL作为接口声明节点类型
    // 后续可以添加专门的Eum_AST_INTERFACE_DECL节点类型
    Stru_AstNode_t* interface_node = F_parser_create_ast_node(interface, Eum_AST_STRUCT_DECL, interface_token);
    
    // 设置接口名称
    F_parser_set_node_attribute(interface, interface_node, "name", (void*)interface_name);
    
    // 设置成员列表作为属性
    F_parser_set_node_attribute(interface, interface_node, "members", members);
    
    // 设置节点类型为接口
    F_parser_set_node_attribute(interface, interface_node, "declaration_type", (void*)"interface");
    
    return interface_node;
}
