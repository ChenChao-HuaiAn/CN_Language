/**
 * @file CN_parser_enum_declarations.c
 * @brief CN_Language 枚举声明解析模块
 * 
 * 枚举声明解析模块，负责解析枚举声明。
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
 * @brief 解析枚举声明（内部实现）
 */
static Stru_AstNode_t* parse_enum_declaration_impl(Stru_ParserInterface_t* interface);

// ============================================
// 枚举声明解析函数实现
// ============================================

/**
 * @brief 解析枚举声明
 * 
 * 解析枚举声明，如：枚举 名称 { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 枚举声明AST节点
 */
Stru_AstNode_t* F_parse_enum_declaration(Stru_ParserInterface_t* interface)
{
    return parse_enum_declaration_impl(interface);
}

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 解析枚举声明（内部实现）
 */
static Stru_AstNode_t* parse_enum_declaration_impl(Stru_ParserInterface_t* interface)
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
    
    // 检查枚举关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_ENUM)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "枚举");
        return NULL;
    }
    
    // 保存枚举关键字令牌
    Stru_Token_t* enum_token = state->current_token;
    
    // 消耗枚举关键字
    F_advance_token(state);
    
    // 解析枚举名称（标识符）
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_IDENTIFIER)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "标识符");
        return NULL;
    }
    
    // 保存枚举名称令牌
    Stru_Token_t* enum_name_token = state->current_token;
    const char* enum_name = enum_name_token->lexeme;
    
    // 消耗枚举名称
    F_advance_token(state);
    
    // 解析枚举成员列表
    Stru_DynamicArray_t* members = F_parse_enum_member_list(interface);
    if (members == NULL)
    {
        // 报告错误
        F_report_invalid_declaration_error(interface,
                                          state->current_token->line,
                                          state->current_token->column,
                                          "枚举成员列表无效",
                                          state->current_token);
        return NULL;
    }
    
    // 创建枚举声明节点
    Stru_AstNode_t* enum_node = F_parser_create_ast_node(interface, Eum_AST_ENUM_DECL, enum_token);
    
    // 设置枚举名称
    F_parser_set_node_attribute(interface, enum_node, "name", (void*)enum_name);
    
    // 设置成员列表作为属性
    F_parser_set_node_attribute(interface, enum_node, "members", members);
    
    return enum_node;
}
