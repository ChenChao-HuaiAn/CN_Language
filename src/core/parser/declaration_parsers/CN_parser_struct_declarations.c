/**
 * @file CN_parser_struct_declarations.c
 * @brief CN_Language 结构体声明解析模块
 * 
 * 结构体声明解析模块，负责解析结构体声明。
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
 * @brief 解析结构体声明（内部实现）
 */
static Stru_AstNode_t* parse_struct_declaration_impl(Stru_ParserInterface_t* interface);

// ============================================
// 结构体声明解析函数实现
// ============================================

/**
 * @brief 解析结构体声明
 * 
 * 解析结构体声明，如：结构体 名称 { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 结构体声明AST节点
 */
Stru_AstNode_t* F_parse_struct_declaration(Stru_ParserInterface_t* interface)
{
    return parse_struct_declaration_impl(interface);
}

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 解析结构体声明（内部实现）
 */
static Stru_AstNode_t* parse_struct_declaration_impl(Stru_ParserInterface_t* interface)
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
    
    // 检查结构体关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_STRUCT)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "结构体");
        return NULL;
    }
    
    // 保存结构体关键字令牌
    Stru_Token_t* struct_token = state->current_token;
    
    // 消耗结构体关键字
    F_advance_token(state);
    
    // 解析结构体名称（标识符）
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_IDENTIFIER)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "标识符");
        return NULL;
    }
    
    // 保存结构体名称令牌
    Stru_Token_t* struct_name_token = state->current_token;
    const char* struct_name = struct_name_token->lexeme;
    
    // 消耗结构体名称
    F_advance_token(state);
    
    // 解析结构体成员列表
    Stru_DynamicArray_t* members = F_parse_struct_member_list(interface);
    if (members == NULL)
    {
        // 报告错误
        F_report_invalid_struct_members_error(interface,
                                             state->current_token->line,
                                             state->current_token->column,
                                             "结构体成员列表无效",
                                             state->current_token);
        return NULL;
    }
    
    // 创建结构体声明节点
    Stru_AstNode_t* struct_node = F_parser_create_ast_node(interface, Eum_AST_STRUCT_DECL, struct_token);
    
    // 设置结构体名称
    F_parser_set_node_attribute(interface, struct_node, "name", (void*)struct_name);
    
    // 设置成员列表作为属性
    F_parser_set_node_attribute(interface, struct_node, "members", members);
    
    return struct_node;
}
