/**
 * @file CN_parser_module_declarations.c
 * @brief CN_Language 模块声明解析模块
 * 
 * 模块声明解析模块，负责解析模块声明。
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
 * @brief 解析模块声明（内部实现）
 */
static Stru_AstNode_t* parse_module_declaration_impl(Stru_ParserInterface_t* interface);

// ============================================
// 模块声明解析函数实现
// ============================================

/**
 * @brief 解析模块声明
 * 
 * 解析模块声明，如：模块 名称 { ... }。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 模块声明AST节点
 */
Stru_AstNode_t* F_parse_module_declaration(Stru_ParserInterface_t* interface)
{
    return parse_module_declaration_impl(interface);
}

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 解析模块声明（内部实现）
 */
static Stru_AstNode_t* parse_module_declaration_impl(Stru_ParserInterface_t* interface)
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
    
    // 检查模块关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_MODULE)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "模块");
        return NULL;
    }
    
    // 保存模块关键字令牌
    Stru_Token_t* module_token = state->current_token;
    
    // 消耗模块关键字
    F_advance_token(state);
    
    // 解析模块名称（标识符）
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_IDENTIFIER)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "标识符");
        return NULL;
    }
    
    // 保存模块名称令牌
    Stru_Token_t* module_name_token = state->current_token;
    const char* module_name = module_name_token->lexeme;
    
    // 消耗模块名称
    F_advance_token(state);
    
    // 解析模块体（代码块）
    Stru_AstNode_t* body = F_parse_block_statement(interface);
    if (body == NULL)
    {
        // 报告错误
        F_report_invalid_module_body_error(interface,
                                          state->current_token->line,
                                          state->current_token->column,
                                          "模块体无效",
                                          state->current_token);
        return NULL;
    }
    
    // 创建模块声明节点
    Stru_AstNode_t* module_node = F_parser_create_ast_node(interface, Eum_AST_MODULE, module_token);
    
    // 设置模块名称
    F_parser_set_node_attribute(interface, module_node, "name", (void*)module_name);
    
    // 添加模块体作为子节点
    F_parser_add_child_node(interface, module_node, body);
    
    return module_node;
}
