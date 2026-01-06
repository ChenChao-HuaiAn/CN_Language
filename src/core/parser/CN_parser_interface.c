/**
 * @file CN_parser_interface.c
 * @brief CN_Language 语法分析器接口实现
 * 
 * 实现语法分析器抽象接口，提供语法分析、AST构建和错误处理功能。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 */

#include "CN_parser_interface.h"
#include "CN_syntax_error.h"
#include "../lexer/CN_lexer_interface.h"
#include "../token/CN_token_types.h"
#include <stdlib.h>
#include <string.h>

// ============================================
// 内部状态结构体
// ============================================

/**
 * @brief 语法分析器内部状态结构体
 * 
 * 存储语法分析器的内部状态，包括词法分析器、当前令牌、错误处理等。
 */
typedef struct Stru_ParserState_t {
    Stru_LexerInterface_t* lexer;          ///< 词法分析器接口
    Stru_Token_t* current_token;           ///< 当前令牌
    Stru_Token_t* previous_token;          ///< 前一个令牌
    Stru_Token_t* next_token;              ///< 下一个令牌（预读）
    Stru_SyntaxErrorHandler_t* error_handler; ///< 错误处理接口
    Stru_AstNode_t* current_node;          ///< 当前AST节点
    Stru_DynamicArray_t* node_stack;       ///< AST节点栈
    bool has_errors;                       ///< 是否有错误
    size_t max_errors;                     ///< 最大错误数量
} Stru_ParserState_t;

// ============================================
// 辅助函数声明
// ============================================

static bool parser_initialize(Stru_ParserInterface_t* interface, Stru_LexerInterface_t* lexer_interface);
static Stru_AstNode_t* parser_parse_program(Stru_ParserInterface_t* interface);
static Stru_AstNode_t* parser_parse_statement(Stru_ParserInterface_t* interface);
static Stru_AstNode_t* parser_parse_expression(Stru_ParserInterface_t* interface);
static Stru_AstNode_t* parser_parse_declaration(Stru_ParserInterface_t* interface);
static Stru_AstNode_t* parser_create_ast_node(Stru_ParserInterface_t* interface, Eum_AstNodeType type, Stru_Token_t* token);
static bool parser_add_child_node(Stru_ParserInterface_t* interface, Stru_AstNode_t* parent, Stru_AstNode_t* child);
static bool parser_set_node_attribute(Stru_ParserInterface_t* interface, Stru_AstNode_t* node, const char* key, void* value);
static void* parser_get_node_attribute(Stru_ParserInterface_t* interface, Stru_AstNode_t* node, const char* key);
static void parser_report_error(Stru_ParserInterface_t* interface, size_t line, size_t column, const char* message, Stru_Token_t* token);
static bool parser_has_errors(Stru_ParserInterface_t* interface);
static Stru_DynamicArray_t* parser_get_errors(Stru_ParserInterface_t* interface);
static void parser_clear_errors(Stru_ParserInterface_t* interface);
static Stru_Token_t* parser_get_current_token(Stru_ParserInterface_t* interface);
static Stru_Token_t* parser_get_previous_token(Stru_ParserInterface_t* interface);
static Stru_Token_t* parser_get_next_token(Stru_ParserInterface_t* interface);
static void parser_reset(Stru_ParserInterface_t* interface);
static void parser_destroy(Stru_ParserInterface_t* interface);

// ============================================
// 内部辅助函数
// ============================================

/**
 * @brief 获取解析器状态
 * 
 * 从接口中获取解析器内部状态。
 * 
 * @param interface 解析器接口
 * @return Stru_ParserState_t* 解析器状态
 */
static Stru_ParserState_t* get_parser_state(Stru_ParserInterface_t* interface) {
    return (Stru_ParserState_t*)interface->internal_state;
}

/**
 * @brief 前进到下一个令牌
 * 
 * 更新当前令牌，从词法分析器获取下一个令牌。
 * 
 * @param state 解析器状态
 */
static void advance_token(Stru_ParserState_t* state) {
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
 * 
 * 检查当前令牌是否是指定类型。
 * 
 * @param state 解析器状态
 * @param type 令牌类型
 * @return true 类型匹配
 * @return false 类型不匹配
 */
static bool check_token_type(Stru_ParserState_t* state, Eum_TokenType type) {
    return state->current_token && state->current_token->type == type;
}

/**
 * @brief 匹配令牌类型
 * 
 * 如果当前令牌是指定类型，则前进到下一个令牌。
 * 
 * @param state 解析器状态
 * @param type 令牌类型
 * @return true 类型匹配并已前进
 * @return false 类型不匹配
 */
static bool match_token_type(Stru_ParserState_t* state, Eum_TokenType type) {
    if (check_token_type(state, type)) {
        advance_token(state);
        return true;
    }
    return false;
}

/**
 * @brief 消耗令牌
 * 
 * 消耗当前令牌，报告错误如果不匹配。
 * 
 * @param state 解析器状态
 * @param type 期望的令牌类型
 * @param error_message 错误信息
 * @return true 成功消耗
 * @return false 令牌不匹配
 */
static bool consume_token(Stru_ParserState_t* state, Eum_TokenType type, const char* error_message) {
    if (match_token_type(state, type)) {
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
static bool parser_initialize(Stru_ParserInterface_t* interface, Stru_LexerInterface_t* lexer_interface) {
    Stru_ParserState_t* state = get_parser_state(interface);
    
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
static Stru_AstNode_t* parser_parse_program(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = get_parser_state(interface);
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
            node = parser_parse_declaration(interface);
        } else {
            // 可能是语句
            node = parser_parse_statement(interface);
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
static Stru_AstNode_t* parser_parse_statement(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = get_parser_state(interface);
    if (!state || !state->current_token) {
        return NULL;
    }
    
    // 根据当前令牌类型解析不同的语句
    // 注意：这里简化实现，实际需要根据具体的令牌类型常量
    // 暂时返回一个简单的表达式语句节点
    Stru_AstNode_t* expr_node = parser_parse_expression(interface);
    if (!expr_node) {
        return NULL;
    }
    
    // 创建表达式语句节点
    Stru_AstNode_t* stmt_node = F_create_ast_node(Eum_AST_EXPRESSION_STMT, 
                                                 state->current_token,
                                                 state->current_token->line,
                                                 state->current_token->column);
    if (!stmt_node) {
        F_destroy_ast_node(expr_node);
        return NULL;
    }
    
    F_ast_add_child(stmt_node, expr_node);
    
    // 消耗分号（如果存在）
    if (state->current_token && state->current_token->type == Eum_TOKEN_DELIMITER_SEMICOLON) {
        advance_token(state);
    }
    
    return stmt_node;
}

/**
 * @brief 解析表达式
 */
static Stru_AstNode_t* parser_parse_expression(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = get_parser_state(interface);
    if (!state || !state->current_token) {
        return NULL;
    }
    
    // 简化实现：创建一个字面量表达式节点
    // 实际实现需要根据语法规范解析各种表达式
    Stru_AstNode_t* expr_node = F_create_ast_node(Eum_AST_LITERAL_EXPR,
                                                 state->current_token,
                                                 state->current_token->line,
                                                 state->current_token->column);
    if (!expr_node) {
        return NULL;
    }
    
    // 根据令牌类型设置值
    switch (state->current_token->type) {
        case Eum_TOKEN_LITERAL_INTEGER:
            F_ast_set_int_value(expr_node, F_token_get_int_value(state->current_token));
            break;
        case Eum_TOKEN_LITERAL_FLOAT:
            F_ast_set_float_value(expr_node, F_token_get_float_value(state->current_token));
            break;
        case Eum_TOKEN_LITERAL_STRING:
            F_ast_set_string_value(expr_node, state->current_token->lexeme);
            break;
        case Eum_TOKEN_LITERAL_BOOLEAN:
            F_ast_set_bool_value(expr_node, F_token_get_bool_value(state->current_token));
            break;
        case Eum_TOKEN_KEYWORD_TRUE:
            F_ast_set_bool_value(expr_node, true);
            break;
        case Eum_TOKEN_KEYWORD_FALSE:
            F_ast_set_bool_value(expr_node, false);
            break;
        default:
            // 对于标识符，创建标识符表达式
            F_destroy_ast_node(expr_node);
            expr_node = F_create_ast_node(Eum_AST_IDENTIFIER_EXPR,
                                         state->current_token,
                                         state->current_token->line,
                                         state->current_token->column);
            if (expr_node) {
                F_ast_set_identifier(expr_node, state->current_token->lexeme);
            }
            break;
    }
    
    if (expr_node) {
        advance_token(state);
    }
    
    return expr_node;
}

/**
 * @brief 解析声明
 */
static Stru_AstNode_t* parser_parse_declaration(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = get_parser_state(interface);
    if (!state || !state->current_token) {
        return NULL;
    }
    
    // 根据关键字类型解析不同的声明
    // 注意：这里简化实现，实际需要根据具体的令牌类型常量
    // 暂时返回一个简单的变量声明节点
    Stru_AstNode_t* decl_node = F_create_ast_node(Eum_AST_VARIABLE_DECL,
                                                 state->current_token,
                                                 state->current_token->line,
                                                 state->current_token->column);
    if (!decl_node) {
        return NULL;
    }
    
    // 消耗关键字
    advance_token(state);
    
    // 解析标识符
    if (state->current_token && state->current_token->type == Eum_TOKEN_IDENTIFIER) {
        F_ast_set_identifier(decl_node, state->current_token->lexeme);
        advance_token(state);
    }
    
    // 解析类型（如果有）
    // 注意：CN语言中类型关键字直接跟在标识符后面
    // 例如：整数 年龄 = 25;
    if (state->current_token && F_token_is_keyword(state->current_token->type)) {
        // 检查是否是类型关键字
        Eum_TokenType token_type = state->current_token->type;
        if (token_type == Eum_TOKEN_KEYWORD_INT ||
            token_type == Eum_TOKEN_KEYWORD_FLOAT ||
            token_type == Eum_TOKEN_KEYWORD_STRING ||
            token_type == Eum_TOKEN_KEYWORD_BOOL ||
            token_type == Eum_TOKEN_KEYWORD_ARRAY ||
            token_type == Eum_TOKEN_KEYWORD_STRUCT ||
            token_type == Eum_TOKEN_KEYWORD_ENUM ||
            token_type == Eum_TOKEN_KEYWORD_POINTER ||
            token_type == Eum_TOKEN_KEYWORD_REFERENCE) {
            
            F_ast_set_type_name(decl_node, state->current_token->lexeme);
            advance_token(state);
        }
    }
    
    // 解析初始值（如果有）
    if (state->current_token && state->current_token->type == Eum_TOKEN_OPERATOR_ASSIGN) {
        advance_token(state); // 消耗等号
        
        Stru_AstNode_t* init_expr = parser_parse_expression(interface);
        if (init_expr) {
            F_ast_add_child(decl_node, init_expr);
        }
    }
    
    // 消耗分号
    if (state->current_token && state->current_token->type == Eum_TOKEN_DELIMITER_SEMICOLON) {
        advance_token(state);
    }
    
    return decl_node;
}

/**
 * @brief 创建AST节点
 */
static Stru_AstNode_t* parser_create_ast_node(Stru_ParserInterface_t* interface, Eum_AstNodeType type, Stru_Token_t* token) {
    size_t line = 1;
    size_t column = 1;
    
    if (token) {
        line = token->line;
        column = token->column;
    } else {
        Stru_ParserState_t* state = get_parser_state(interface);
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
static bool parser_add_child_node(Stru_ParserInterface_t* interface, Stru_AstNode_t* parent, Stru_AstNode_t* child) {
    if (!parent || !child) {
        return false;
    }
    
    return F_ast_add_child(parent, child);
}

/**
 * @brief 设置节点属性
 */
static bool parser_set_node_attribute(Stru_ParserInterface_t* interface, Stru_AstNode_t* node, const char* key, void* value) {
    if (!node || !key) {
        return false;
    }
    
    return F_ast_set_attribute(node, key, value);
}

/**
 * @brief 获取节点属性
 */
static void* parser_get_node_attribute(Stru_ParserInterface_t* interface, Stru_AstNode_t* node, const char* key) {
    if (!node || !key) {
        return NULL;
    }
    
    return F_ast_get_attribute(node, key);
}

/**
 * @brief 报告语法错误
 */
static void parser_report_error(Stru_ParserInterface_t* interface, size_t line, size_t column, const char* message, Stru_Token_t* token) {
    Stru_ParserState_t* state = get_parser_state(interface);
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
static bool parser_has_errors(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = get_parser_state(interface);
    if (!state) {
        return false;
    }
    
    return state->has_errors;
}

/**
 * @brief 获取错误列表
 */
static Stru_DynamicArray_t* parser_get_errors(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = get_parser_state(interface);
    if (!state || !state->error_handler) {
        return NULL;
    }
    
    return state->error_handler->get_errors(state->error_handler);
}

/**
 * @brief 清除错误
 */
static void parser_clear_errors(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = get_parser_state(interface);
    if (!state || !state->error_handler) {
        return;
    }
    
    state->error_handler->clear_errors(state->error_handler);
    state->has_errors = false;
}

/**
 * @brief 获取当前令牌
 */
static Stru_Token_t* parser_get_current_token(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = get_parser_state(interface);
    if (!state) {
        return NULL;
    }
    
    return state->current_token;
}

/**
 * @brief 获取前一个令牌
 */
static Stru_Token_t* parser_get_previous_token(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = get_parser_state(interface);
    if (!state) {
        return NULL;
    }
    
    return state->previous_token;
}

/**
 * @brief 获取下一个令牌
 */
static Stru_Token_t* parser_get_next_token(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = get_parser_state(interface);
    if (!state) {
        return NULL;
    }
    
    return state->next_token;
}

/**
 * @brief 重置语法分析器
 */
static void parser_reset(Stru_ParserInterface_t* interface) {
    Stru_ParserState_t* state = get_parser_state(interface);
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
    parser_clear_errors(interface);
    
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
static void parser_destroy(Stru_ParserInterface_t* interface) {
    if (!interface) {
        return;
    }
    
    Stru_ParserState_t* state = get_parser_state(interface);
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

// ============================================
// 工厂函数实现
// ============================================

/**
 * @brief 创建语法分析器接口实例
 */
Stru_ParserInterface_t* F_create_parser_interface(void) {
    // 分配接口内存
    Stru_ParserInterface_t* interface = (Stru_ParserInterface_t*)malloc(sizeof(Stru_ParserInterface_t));
    if (!interface) {
        return NULL;
    }
    
    // 分配内部状态内存
    Stru_ParserState_t* state = (Stru_ParserState_t*)malloc(sizeof(Stru_ParserState_t));
    if (!state) {
        free(interface);
        return NULL;
    }
    
    // 初始化内部状态
    memset(state, 0, sizeof(Stru_ParserState_t));
    state->max_errors = 100;
    
    // 设置接口函数指针
    interface->initialize = parser_initialize;
    interface->parse_program = parser_parse_program;
    interface->parse_statement = parser_parse_statement;
    interface->parse_expression = parser_parse_expression;
    interface->parse_declaration = parser_parse_declaration;
    interface->create_ast_node = parser_create_ast_node;
    interface->add_child_node = parser_add_child_node;
    interface->set_node_attribute = parser_set_node_attribute;
    interface->get_node_attribute = parser_get_node_attribute;
    interface->report_error = parser_report_error;
    interface->has_errors = parser_has_errors;
    interface->get_errors = parser_get_errors;
    interface->clear_errors = parser_clear_errors;
    interface->get_current_token = parser_get_current_token;
    interface->get_previous_token = parser_get_previous_token;
    interface->get_next_token = parser_get_next_token;
    interface->reset = parser_reset;
    interface->destroy = parser_destroy;
    
    // 设置内部状态指针
    interface->internal_state = state;
    
    return interface;
}
