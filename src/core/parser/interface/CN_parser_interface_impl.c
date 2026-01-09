/**
 * @file CN_parser_interface_impl.c
 * @brief CN_Language 语法分析器接口实现
 * 
 * 语法分析器接口的具体实现，提供语法分析、AST构建和错误处理功能。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_parser_interface_impl.h"
#include "../expression_parsers/CN_parser_expressions.h"
#include "../statement_parsers/CN_parser_statements.h"
#include "../declaration_parsers/CN_parser_declarations.h"
#include "../error_handling/CN_parser_errors.h"
#include "../utils/CN_parser_utils.h"

// ============================================
// 内部辅助函数实现
// ============================================

/**
 * @brief 获取解析器状态
 */
Stru_ParserState_t* F_get_parser_state(Stru_ParserInterface_t* interface) {
    return (Stru_ParserState_t*)interface->internal_state;
}

/**
 * @brief 前进到下一个令牌
 */
void F_advance_token(Stru_ParserState_t* state) {
    if (state->previous_token) {
        F_destroy_token(state->previous_token);
    }
    state->previous_token = state->current_token;
    state->current_token = state->next_token;
    
    if (state->lexer && state->lexer->has_more_tokens(state->lexer)) {
        state->next_token = state->lexer->next_token(state->lexer);
    } else {
        state->next_token = NULL;
    }
}

/**
 * @brief 检查当前令牌类型
 */
bool F_check_token_type(Stru_ParserState_t* state, Eum_TokenType type) {
    return state->current_token && state->current_token->type == type;
}

/**
 * @brief 匹配令牌类型
 */
bool F_match_token_type(Stru_ParserState_t* state, Eum_TokenType type) {
    if (F_check_token_type(state, type)) {
        F_advance_token(state);
        return true;
    }
    return false;
}

/**
 * @brief 消耗令牌
 */
bool F_consume_token(Stru_ParserState_t* state, Eum_TokenType type, const char* error_message) {
    if (F_match_token_type(state, type)) {
        return true;
    }
    
    // 报告错误
    if (state->current_token) {
        state->error_handler->report_unexpected_token(
            state->error_handler,
            state->current_token,
            error_message
        );
    } else {
        state->error_handler->report_missing_token(
            state->error_handler,
            state->previous_token ? state->previous_token->line + 1 : 1,
            state->previous_token ? state->previous_token->column + 1 : 1,
            error_message
        );
    }
    
    state->has_errors = true;
    return false;
}

// ============================================
// 接口函数实现
// ============================================

/**
 * @brief 初始化语法分析器
 */
bool F_parser_initialize(Stru_ParserInterface_t* interface, Stru_LexerInterface_t* lexer_interface) {
    Stru_ParserState_t* state = F_get_parser_state(interface);
    
    if (!state) {
        return false;
    }
    
    // 设置词法分析器
    state->lexer = lexer_interface;
    
    // 初始化令牌
    if (state->lexer && state->lexer->has_more_tokens(state->lexer)) {
        state->current_token = state->lexer->next_token(state->lexer);
        if (state->lexer->has_more_tokens(state->lexer)) {
            state->next_token = state->lexer->next_token(state->lexer);
        }
    }
    
    // 创建错误处理接口
    state->error_handler = F_create_syntax_error_handler();
    if (!state->error_handler) {
        return false;
    }
    
    // 创建节点栈
    state->node_stack = F_create_dynamic_array(sizeof(Stru_AstNode_t*));
    if (!state->node_stack) {
        state->error_handler->destroy(state->error_handler);
        return false;
    }
    
    state->has_errors = false;
    state->max_errors = 100; // 默认最大错误数量
    
    return true;
}

/**
 * @brief 解析程序
 */
Stru_AstNode_t* F_parser_parse_program(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (!state || !state->current_token) {
        return NULL;
    }
    
    // 创建程序节点
    Stru_AstNode_t* program_node = F_create_ast_node(Eum_AST_PROGRAM, NULL, 1, 1);
    if (!program_node) {
        return NULL;
    }
    
    // 保存当前节点
    Stru_AstNode_t* saved_current = state->current_node;
    state->current_node = program_node;
    
    // 解析程序内容（声明和语句）
    while (state->current_token) {
        Stru_AstNode_t* node = NULL;
        
        // 根据当前令牌类型决定解析什么
        if (F_token_is_keyword(state->current_token->type)) {
            // 可能是声明
            node = F_parser_parse_declaration(interface);
        } else {
            // 可能是语句
            node = F_parser_parse_statement(interface);
        }
        
        if (node) {
            F_ast_add_child(program_node, node);
        }
        
        // 如果遇到文件结束或错误太多，停止解析
        if (state->current_token == NULL || 
            (state->has_errors && state->error_handler->get_error_count(state->error_handler) >= state->max_errors)) {
            break;
        }
    }
    
    // 恢复当前节点
    state->current_node = saved_current;
    
    return program_node;
}

/**
 * @brief 解析语句
 */
Stru_AstNode_t* F_parser_parse_statement(Stru_ParserInterface_t* interface) {
    // 委托给语句解析模块
    return F_parse_statement(interface);
}

/**
 * @brief 解析表达式
 */
Stru_AstNode_t* F_parser_parse_expression(Stru_ParserInterface_t* interface) {
    // 委托给表达式解析模块
    return F_parse_expression(interface);
}

/**
 * @brief 解析声明
 */
Stru_AstNode_t* F_parser_parse_declaration(Stru_ParserInterface_t* interface) {
    // 委托给声明解析模块
    return F_parse_declaration(interface);
}

/**
 * @brief 创建AST节点
 */
Stru_AstNode_t* F_parser_create_ast_node(Stru_ParserInterface_t* interface, Eum_AstNodeType type, Stru_Token_t* token) {
    size_t line = 1;
    size_t column = 1;
    
    if (token) {
        line = token->line;
        column = token->column;
    } else {
        Stru_ParserState_t* state = F_get_parser_state(interface);
        if (state && state->current_token) {
            line = state->current_token->line;
            column = state->current_token->column;
        }
    }
    
    return F_create_ast_node(type, token, line, column);
}

/**
 * @brief 添加子节点
 */
bool F_parser_add_child_node(Stru_ParserInterface_t* interface, Stru_AstNode_t* parent, Stru_AstNode_t* child) {
    if (!parent || !child) {
        return false;
    }
    
    return F_ast_add_child(parent, child);
}

/**
 * @brief 设置节点属性
 */
bool F_parser_set_node_attribute(Stru_ParserInterface_t* interface, Stru_AstNode_t* node, const char* key, void* value) {
    if (!node || !key) {
        return false;
    }
    
    return F_ast_set_attribute(node, key, value);
}

/**
 * @brief 获取节点属性
 */
void* F_parser_get_node_attribute(Stru_ParserInterface_t* interface, Stru_AstNode_t* node, const char* key) {
    if (!node || !key) {
        return NULL;
    }
    
    return F_ast_get_attribute(node, key);
}

/**
 * @brief 报告语法错误
 */
void F_parser_report_error(Stru_ParserInterface_t* interface, size_t line, size_t column, const char* message, Stru_Token_t* token) {
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (!state || !state->error_handler) {
        return;
    }
    
    state->error_handler->report_error(
        state->error_handler,
        Eum_ERROR_INVALID_EXPRESSION,  // 使用已定义的错误类型
        Eum_SEVERITY_ERROR,
        line, column,
        message,
        token
    );
    
    state->has_errors = true;
}

/**
 * @brief 检查是否有错误
 */
bool F_parser_has_errors(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (!state) {
        return false;
    }
    
    return state->has_errors;
}

/**
 * @brief 获取错误列表
 */
Stru_DynamicArray_t* F_parser_get_errors(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (!state || !state->error_handler) {
        return NULL;
    }
    
    return state->error_handler->get_errors(state->error_handler);
}

/**
 * @brief 清除错误
 */
void F_parser_clear_errors(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (!state || !state->error_handler) {
        return;
    }
    
    state->error_handler->clear_errors(state->error_handler);
    state->has_errors = false;
}

/**
 * @brief 获取当前令牌
 */
Stru_Token_t* F_parser_get_current_token(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (!state) {
        return NULL;
    }
    
    return state->current_token;
}

/**
 * @brief 获取前一个令牌
 */
Stru_Token_t* F_parser_get_previous_token(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (!state) {
        return NULL;
    }
    
    return state->previous_token;
}

/**
 * @brief 获取下一个令牌
 */
Stru_Token_t* F_parser_get_next_token(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (!state) {
        return NULL;
    }
    
    return state->next_token;
}

/**
 * @brief 重置语法分析器
 */
void F_parser_reset(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (!state) {
        return;
    }
    
    // 释放令牌
    if (state->current_token) {
        F_destroy_token(state->current_token);
        state->current_token = NULL;
    }
    if (state->previous_token) {
        F_destroy_token(state->previous_token);
        state->previous_token = NULL;
    }
    if (state->next_token) {
        F_destroy_token(state->next_token);
        state->next_token = NULL;
    }
    
    // 重置词法分析器
    if (state->lexer) {
        state->lexer->reset(state->lexer);
    }
    
    // 清除错误
    F_parser_clear_errors(interface);
    
    // 清空节点栈
    if (state->node_stack) {
        F_destroy_dynamic_array(state->node_stack);
        state->node_stack = F_create_dynamic_array(sizeof(Stru_AstNode_t*));
    }
    
    state->current_node = NULL;
    state->has_errors = false;
}

/**
 * @brief 销毁语法分析器
 */
void F_parser_destroy(Stru_ParserInterface_t* interface) {
    if (!interface) {
        return;
    }
    
    Stru_ParserState_t* state = F_get_parser_state(interface);
    if (!state) {
        free(interface);
        return;
    }
    
    // 释放令牌
    if (state->current_token) {
        F_destroy_token(state->current_token);
    }
    if (state->previous_token) {
        F_destroy_token(state->previous_token);
    }
    if (state->next_token) {
        F_destroy_token(state->next_token);
    }
    
    // 销毁错误处理接口
    if (state->error_handler) {
        state->error_handler->destroy(state->error_handler);
    }
    
    // 销毁节点栈
    if (state->node_stack) {
        F_destroy_dynamic_array(state->node_stack);
    }
    
    // 释放状态
    free(state);
    
    // 释放接口
    free(interface);
}
