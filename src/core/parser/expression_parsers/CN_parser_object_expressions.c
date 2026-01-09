/**
 * @file CN_parser_object_expressions.c
 * @brief CN_Language 对象表达式解析模块
 * 
 * 对象表达式解析模块，负责解析对象创建和销毁表达式。
 * 包括新对象创建表达式（新 类型()）和对象销毁表达式（删除 对象）。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_parser_expressions.h"
#include "../interface/CN_parser_interface_impl.h"
#include "../error_handling/CN_parser_errors.h"
#include "../utils/CN_parser_utils.h"
#include "../../../infrastructure/containers/array/CN_dynamic_array.h"
#include "../../token/CN_token_types.h"
#include "../../ast/CN_ast.h"
#include <stdlib.h>
#include <string.h>

// ============================================
// 内部辅助函数声明
// ============================================

/**
 * @brief 解析对象创建表达式（内部实现）
 */
static Stru_AstNode_t* parse_new_expression_impl(Stru_ParserInterface_t* interface);

/**
 * @brief 解析对象销毁表达式（内部实现）
 */
static Stru_AstNode_t* parse_delete_expression_impl(Stru_ParserInterface_t* interface);

// ============================================
// 对象表达式解析函数实现
// ============================================

/**
 * @brief 解析对象创建表达式
 * 
 * 解析对象创建表达式，如：新 类型()。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 对象创建表达式AST节点
 */
Stru_AstNode_t* F_parse_new_expression(Stru_ParserInterface_t* interface)
{
    return parse_new_expression_impl(interface);
}

/**
 * @brief 解析对象销毁表达式
 * 
 * 解析对象销毁表达式，如：删除 对象。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 对象销毁表达式AST节点
 */
Stru_AstNode_t* F_parse_delete_expression(Stru_ParserInterface_t* interface)
{
    return parse_delete_expression_impl(interface);
}

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 解析对象创建表达式（内部实现）
 */
static Stru_AstNode_t* parse_new_expression_impl(Stru_ParserInterface_t* interface)
{
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL)
    {
        return NULL;
    }
    
    // 检查是否为"新"关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_NEW)
    {
        return NULL; // 不是对象创建表达式
    }
    
    // 保存"新"关键字令牌
    Stru_Token_t* new_token = state->current_token;
    
    // 消耗"新"关键字
    F_advance_token(state);
    
    // 解析类型名称
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_IDENTIFIER)
    {
        // 报告错误：期望类型名称
        F_report_unexpected_token_error(interface, state->current_token, "类型名称");
        return NULL;
    }
    
    // 保存类型名称令牌
    Stru_Token_t* type_token = state->current_token;
    
    // 消耗类型名称
    F_advance_token(state);
    
    // 检查是否有参数列表（可选）
    Stru_DynamicArray_t* arguments = NULL;
    
    if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_DELIMITER_LPAREN)
    {
        // 解析参数列表
        arguments = F_parse_argument_list(interface);
        if (arguments == NULL)
        {
            // 参数列表解析失败，但可能没有参数
            // 继续处理，arguments保持为NULL
        }
    }
    
    // 创建对象创建表达式节点
    Stru_AstNode_t* new_node = F_create_new_expression_node(interface, new_token, type_token, arguments);
    
    // 清理参数列表（如果创建节点函数没有接管所有权）
    if (arguments != NULL)
    {
        // 注意：如果F_create_new_expression_node接管了arguments的所有权，则不应在这里释放
        // 这取决于具体实现，这里假设函数会复制或接管参数
        // 在实际实现中，需要根据F_create_new_expression_node的文档决定
    }
    
    return new_node;
}

/**
 * @brief 解析对象销毁表达式（内部实现）
 */
static Stru_AstNode_t* parse_delete_expression_impl(Stru_ParserInterface_t* interface)
{
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (state == NULL || state->current_token == NULL)
    {
        return NULL;
    }
    
    // 检查是否为"删除"关键字
    if (state->current_token->type != Eum_TOKEN_KEYWORD_DELETE)
    {
        return NULL; // 不是对象销毁表达式
    }
    
    // 保存"删除"关键字令牌
    Stru_Token_t* delete_token = state->current_token;
    
    // 消耗"删除"关键字
    F_advance_token(state);
    
    // 解析要删除的对象表达式
    Stru_AstNode_t* object_expression = F_parse_unary_expression(interface);
    if (object_expression == NULL)
    {
        // 报告错误：期望对象表达式
        F_report_invalid_expression_error(interface,
                                         state->current_token ? state->current_token->line : 1,
                                         state->current_token ? state->current_token->column : 1,
                                         "对象表达式无效",
                                         state->current_token);
        return NULL;
    }
    
    // 创建对象销毁表达式节点
    Stru_AstNode_t* delete_node = F_create_delete_expression_node(interface, delete_token, object_expression);
    
    return delete_node;
}
