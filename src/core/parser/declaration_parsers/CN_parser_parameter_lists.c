/**
 * @file CN_parser_parameter_lists.c
 * @brief CN_Language 参数列表解析模块
 * 
 * 参数列表解析模块，负责解析函数声明的参数列表。
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
// 参数列表解析函数实现
// ============================================

/**
 * @brief 解析参数列表
 * 
 * 解析函数声明的参数列表。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_DynamicArray_t* 参数声明数组
 */
Stru_DynamicArray_t* F_parse_parameter_list(Stru_ParserInterface_t* interface)
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
    
    // 检查左括号
    if (state->current_token->type != Eum_TOKEN_DELIMITER_LPAREN)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->current_token->line,
                                    state->current_token->column,
                                    "(");
        return NULL;
    }
    
    // 消耗左括号
    F_advance_token(state);
    
    // 创建参数数组
    Stru_DynamicArray_t* parameters = F_create_dynamic_array(sizeof(Stru_AstNode_t*));
    if (parameters == NULL)
    {
        return NULL;
    }
    
    // 解析参数直到遇到右括号
    while (state->current_token != NULL && 
           state->current_token->type != Eum_TOKEN_DELIMITER_RPAREN)
    {
        // 解析参数
        Stru_AstNode_t* parameter = F_parse_parameter(interface);
        if (parameter == NULL)
        {
            // 报告错误但继续尝试恢复
            F_report_invalid_parameter_error(interface,
                                            state->current_token->line,
                                            state->current_token->column,
                                            "参数无效",
                                            state->current_token);
            
            // 尝试同步到安全点
            Eum_TokenType sync_tokens[] = {Eum_TOKEN_DELIMITER_COMMA, Eum_TOKEN_DELIMITER_RPAREN};
            F_synchronize_to_safe_point(interface, sync_tokens, 2);
            
            // 如果当前令牌是右括号，跳出循环
            if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_DELIMITER_RPAREN)
            {
                break;
            }
            
            continue;
        }
        
        F_dynamic_array_push(parameters, &parameter);
        
        // 检查是否有逗号（更多参数）
        if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_DELIMITER_COMMA)
        {
            // 消耗逗号
            F_advance_token(state);
        }
        else if (state->current_token != NULL && state->current_token->type != Eum_TOKEN_DELIMITER_RPAREN)
        {
            // 报告错误
            F_report_missing_token_error(interface, 
                                        state->previous_token->line,
                                        state->previous_token->column,
                                        ",");
            
            // 清理参数数组
            F_destroy_dynamic_array(parameters);
            return NULL;
        }
    }
    
    // 检查右括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_RPAREN)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    ")");
        
        // 清理参数数组
        F_destroy_dynamic_array(parameters);
        return NULL;
    }
    
    // 消耗右括号
    F_advance_token(state);
    
    return parameters;
}

/**
 * @brief 解析单个参数
 * 
 * 解析函数声明的单个参数。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_AstNode_t* 参数声明AST节点
 */
Stru_AstNode_t* F_parse_parameter(Stru_ParserInterface_t* interface)
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
    
    // 解析参数名称（标识符）
    if (state->current_token->type != Eum_TOKEN_IDENTIFIER)
    {
        // 报告错误
        F_report_unexpected_token_error(interface, state->current_token, "标识符");
        return NULL;
    }
    
    // 保存参数名称令牌
    Stru_Token_t* param_name_token = state->current_token;
    const char* param_name = param_name_token->lexeme;
    
    // 消耗参数名称
    F_advance_token(state);
    
    // 检查冒号（类型注解）
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_COLON)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    ":");
        return NULL;
    }
    
    // 消耗冒号
    F_advance_token(state);
    
    // 解析参数类型
    Stru_AstNode_t* param_type = F_parse_type_expression(interface);
    if (param_type == NULL)
    {
        // 报告错误
        F_report_invalid_type_error(interface,
                                   state->current_token->line,
                                   state->current_token->column,
                                   "参数类型无效",
                                   state->current_token);
        return NULL;
    }
    
    // 创建参数声明节点
    Stru_AstNode_t* param_node = F_parser_create_ast_node(interface, Eum_AST_PARAMETER_DECL, param_name_token);
    
    // 设置参数名称
    F_parser_set_node_attribute(interface, param_node, "name", (void*)param_name);
    
    // 添加类型作为子节点
    F_parser_add_child_node(interface, param_node, param_type);
    
    return param_node;
}
