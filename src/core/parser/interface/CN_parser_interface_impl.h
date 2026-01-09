/**
 * @file CN_parser_interface_impl.h
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

#ifndef CN_PARSER_INTERFACE_IMPL_H
#define CN_PARSER_INTERFACE_IMPL_H

#include "../CN_parser_interface.h"
#include "../CN_syntax_error.h"
#include "../../lexer/CN_lexer_interface.h"
#include "../../token/CN_token_types.h"
#include "../../ast/CN_ast.h"
#include "../../../infrastructure/containers/array/CN_dynamic_array.h"
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
// 内部辅助函数声明
// ============================================

/**
 * @brief 获取解析器状态
 * 
 * 从接口中获取解析器内部状态。
 * 
 * @param interface 解析器接口
 * @return Stru_ParserState_t* 解析器状态
 */
Stru_ParserState_t* F_get_parser_state(Stru_ParserInterface_t* interface);

/**
 * @brief 前进到下一个令牌
 * 
 * 更新当前令牌，从词法分析器获取下一个令牌。
 * 
 * @param state 解析器状态
 */
void F_advance_token(Stru_ParserState_t* state);

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
bool F_check_token_type(Stru_ParserState_t* state, Eum_TokenType type);

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
bool F_match_token_type(Stru_ParserState_t* state, Eum_TokenType type);

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
bool F_consume_token(Stru_ParserState_t* state, Eum_TokenType type, const char* error_message);

// ============================================
// 接口函数声明
// ============================================

/**
 * @brief 初始化语法分析器
 */
bool F_parser_initialize(Stru_ParserInterface_t* interface, Stru_LexerInterface_t* lexer_interface);

/**
 * @brief 解析程序
 */
Stru_AstNode_t* F_parser_parse_program(Stru_ParserInterface_t* interface);

/**
 * @brief 解析语句
 */
Stru_AstNode_t* F_parser_parse_statement(Stru_ParserInterface_t* interface);

/**
 * @brief 解析表达式
 */
Stru_AstNode_t* F_parser_parse_expression(Stru_ParserInterface_t* interface);

/**
 * @brief 解析声明
 */
Stru_AstNode_t* F_parser_parse_declaration(Stru_ParserInterface_t* interface);

/**
 * @brief 创建AST节点
 */
Stru_AstNode_t* F_parser_create_ast_node(Stru_ParserInterface_t* interface, Eum_AstNodeType type, Stru_Token_t* token);

/**
 * @brief 添加子节点
 */
bool F_parser_add_child_node(Stru_ParserInterface_t* interface, Stru_AstNode_t* parent, Stru_AstNode_t* child);

/**
 * @brief 设置节点属性
 */
bool F_parser_set_node_attribute(Stru_ParserInterface_t* interface, Stru_AstNode_t* node, const char* key, void* value);

/**
 * @brief 获取节点属性
 */
void* F_parser_get_node_attribute(Stru_ParserInterface_t* interface, Stru_AstNode_t* node, const char* key);

/**
 * @brief 报告语法错误
 */
void F_parser_report_error(Stru_ParserInterface_t* interface, size_t line, size_t column, const char* message, Stru_Token_t* token);

/**
 * @brief 检查是否有错误
 */
bool F_parser_has_errors(Stru_ParserInterface_t* interface);

/**
 * @brief 获取错误列表
 */
Stru_DynamicArray_t* F_parser_get_errors(Stru_ParserInterface_t* interface);

/**
 * @brief 清除错误
 */
void F_parser_clear_errors(Stru_ParserInterface_t* interface);

/**
 * @brief 获取当前令牌
 */
Stru_Token_t* F_parser_get_current_token(Stru_ParserInterface_t* interface);

/**
 * @brief 获取前一个令牌
 */
Stru_Token_t* F_parser_get_previous_token(Stru_ParserInterface_t* interface);

/**
 * @brief 获取下一个令牌
 */
Stru_Token_t* F_parser_get_next_token(Stru_ParserInterface_t* interface);

/**
 * @brief 重置语法分析器
 */
void F_parser_reset(Stru_ParserInterface_t* interface);

/**
 * @brief 销毁语法分析器
 */
void F_parser_destroy(Stru_ParserInterface_t* interface);

#endif // CN_PARSER_INTERFACE_IMPL_H
