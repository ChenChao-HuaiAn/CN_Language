/**
 * @file CN_parser_virtual_declarations.c
 * @brief CN_Language 虚拟函数声明解析模块
 * 
 * 虚拟函数声明解析模块，负责解析虚拟函数声明。
 * 虚拟函数声明语法：虚拟 函数 名称(参数) { ... }
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
 * @brief 解析虚拟函数声明（内部实现）
 */
static Stru_AstNode_t* parse_virtual_declaration_impl(Stru_ParserInterface_t* interface);

// ============================================
// 虚拟函数声明解析函数实现
// ============================================

/**
 * @brief 解析虚拟函数声明
 * 
 * 解析虚拟函数声明，如：虚拟 函数 名称(参数) { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 虚拟函数声明AST节点
 */
Stru_AstNode_t* F_parse_virtual_declaration(Stru_ParserInterface_t* interface)
{
    return parse_virtual_declaration_impl(interface);
}

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 解析虚拟函数声明（内部实现）
 */
static Stru_AstNode_t* parse_virtual_declaration_impl(Stru_ParserInterface_t* interface)
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
    
    // 检查虚拟关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_VIRTUAL)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "虚拟");
        return NULL;
    }
    
    // 保存虚拟关键字令牌
    Stru_Token_t* virtual_token = state->current_token;
    
    // 消耗虚拟关键字
    F_advance_token(state);
    
    // 检查下一个令牌是否为函数关键字
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_KEYWORD_FUNCTION)
    {
        // 报告错误：虚拟只能修饰函数
        F_report_unexpected_token_error(interface, state->current_token, "函数");
        return NULL;
    }
    
    // 解析函数声明
    Stru_AstNode_t* function_declaration = F_parse_function_declaration(interface);
    if (function_declaration == NULL)
    {
        // 报告错误
        F_report_invalid_declaration_error(interface,
                                         state->current_token->line,
                                         state->current_token->column,
                                         "虚拟函数声明无效",
                                         state->current_token);
        return NULL;
    }
    
    // 创建虚拟函数声明节点
    Stru_AstNode_t* virtual_node = F_parser_create_ast_node(interface, Eum_AST_FUNCTION_DECL, virtual_token);
    
    // 设置虚拟属性
    F_parser_set_node_attribute(interface, virtual_node, "is_virtual", (void*)1);
    
    // 添加函数声明作为子节点
    F_parser_add_child_node(interface, virtual_node, function_declaration);
    
    return virtual_node;
}
