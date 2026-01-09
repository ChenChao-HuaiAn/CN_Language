/**
 * @file CN_parser_member_lists.c
 * @brief CN_Language 成员列表解析模块
 * 
 * 成员列表解析模块，负责解析结构体和枚举的成员列表。
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
// 成员列表解析函数实现
// ============================================

/**
 * @brief 解析结构体成员列表
 * 
 * 解析结构体成员列表。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_DynamicArray_t* 结构体成员数组
 */
Stru_DynamicArray_t* F_parse_struct_member_list(Stru_ParserInterface_t* interface)
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
    
    // 检查左花括号
    if (state->current_token->type != Eum_TOKEN_DELIMITER_LBRACE)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->current_token->line,
                                    state->current_token->column,
                                    "{");
        return NULL;
    }
    
    // 消耗左花括号
    F_advance_token(state);
    
    // 创建成员数组
    Stru_DynamicArray_t* members = F_create_dynamic_array(sizeof(Stru_AstNode_t*));
    if (members == NULL)
    {
        return NULL;
    }
    
    // 解析成员直到遇到右花括号
    while (state->current_token != NULL && 
           state->current_token->type != Eum_TOKEN_DELIMITER_RBRACE)
    {
        // 解析成员声明（变量声明）
        Stru_AstNode_t* member = F_parse_variable_declaration(interface);
        if (member == NULL)
        {
            // 报告错误但继续尝试恢复
            F_report_invalid_declaration_error(interface,
                                              state->current_token->line,
                                              state->current_token->column,
                                              "结构体成员无效",
                                              state->current_token);
            
            // 尝试同步到安全点
            Eum_TokenType sync_tokens[] = {Eum_TOKEN_DELIMITER_SEMICOLON, Eum_TOKEN_DELIMITER_RBRACE};
            F_synchronize_to_safe_point(interface, sync_tokens, 2);
            
            // 如果当前令牌是右花括号，跳出循环
            if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_DELIMITER_RBRACE)
            {
                break;
            }
            
            continue;
        }
        
        F_dynamic_array_push(members, &member);
        
        // 检查分号
        if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_SEMICOLON)
        {
            // 报告错误
            F_report_missing_token_error(interface, 
                                        state->previous_token->line,
                                        state->previous_token->column,
                                        ";");
            
            // 清理成员数组
            F_destroy_dynamic_array(members);
            return NULL;
        }
        
        // 消耗分号
        F_advance_token(state);
    }
    
    // 检查右花括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_RBRACE)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    "}");
        
        // 清理成员数组
        F_destroy_dynamic_array(members);
        return NULL;
    }
    
    // 消耗右花括号
    F_advance_token(state);
    
    return members;
}

/**
 * @brief 解析枚举成员列表
 * 
 * 解析枚举成员列表。
 * 
 * @param interface 语法分析器接口指针
 * @return Stru_DynamicArray_t* 枚举成员数组
 */
Stru_DynamicArray_t* F_parse_enum_member_list(Stru_ParserInterface_t* interface)
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
    
    // 检查左花括号
    if (state->current_token->type != Eum_TOKEN_DELIMITER_LBRACE)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->current_token->line,
                                    state->current_token->column,
                                    "{");
        return NULL;
    }
    
    // 消耗左花括号
    F_advance_token(state);
    
    // 创建成员数组
    Stru_DynamicArray_t* members = F_create_dynamic_array(sizeof(Stru_AstNode_t*));
    if (members == NULL)
    {
        return NULL;
    }
    
    // 解析成员直到遇到右花括号
    while (state->current_token != NULL && 
           state->current_token->type != Eum_TOKEN_DELIMITER_RBRACE)
    {
        // 解析枚举成员名称（标识符）
        if (state->current_token->type != Eum_TOKEN_IDENTIFIER)
        {
            // 报告错误
            F_report_unexpected_token_error(interface, state->current_token, "标识符");
            
            // 清理成员数组
            F_destroy_dynamic_array(members);
            return NULL;
        }
        
        // 保存成员名称令牌
        Stru_Token_t* member_token = state->current_token;
        const char* member_name = member_token->lexeme;
        
        // 消耗成员名称
        F_advance_token(state);
        
        // 创建枚举成员节点
        Stru_AstNode_t* member_node = F_parser_create_ast_node(interface, Eum_AST_CONSTANT_DECL, member_token);
        
        // 设置成员名称
        F_parser_set_node_attribute(interface, member_node, "name", (void*)member_name);
        
        // 检查是否有赋值
        if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_OPERATOR_ASSIGN)
        {
            // 消耗赋值运算符
            F_advance_token(state);
            
            // 解析值表达式
            Stru_AstNode_t* value = F_parser_parse_expression(interface);
            if (value == NULL)
            {
                // 报告错误
                F_report_invalid_expression_error(interface,
                                                 state->current_token->line,
                                                 state->current_token->column,
                                                 "枚举成员值无效",
                                                 state->current_token);
                
                // 清理成员节点和数组
                F_destroy_ast_node(member_node);
                F_destroy_dynamic_array(members);
                return NULL;
            }
            
            // 添加值作为子节点
            F_parser_add_child_node(interface, member_node, value);
        }
        
        F_dynamic_array_push(members, &member_node);
        
        // 检查是否有逗号（更多成员）
        if (state->current_token != NULL && state->current_token->type == Eum_TOKEN_DELIMITER_COMMA)
        {
            // 消耗逗号
            F_advance_token(state);
        }
        else if (state->current_token != NULL && state->current_token->type != Eum_TOKEN_DELIMITER_RBRACE)
        {
            // 报告错误
            F_report_missing_token_error(interface, 
                                        state->previous_token->line,
                                        state->previous_token->column,
                                        ",");
            
            // 清理成员数组
            F_destroy_dynamic_array(members);
            return NULL;
        }
    }
    
    // 检查右花括号
    if (state->current_token == NULL || state->current_token->type != Eum_TOKEN_DELIMITER_RBRACE)
    {
        // 报告错误
        F_report_missing_token_error(interface, 
                                    state->previous_token->line,
                                    state->previous_token->column,
                                    "}");
        
        // 清理成员数组
        F_destroy_dynamic_array(members);
        return NULL;
    }
    
    // 消耗右花括号
    F_advance_token(state);
    
    return members;
}
