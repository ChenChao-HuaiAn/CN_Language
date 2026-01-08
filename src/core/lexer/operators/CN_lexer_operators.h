/**
 * @file CN_lexer_operators.h
 * @brief CN_Language 词法分析器运算符识别模块
 * 
 * 提供运算符的识别功能，支持算术、比较、赋值等运算符。
 * 负责将运算符词素映射为对应的运算符令牌类型。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#ifndef CN_LEXER_OPERATORS_H
#define CN_LEXER_OPERATORS_H

#include "../../token/CN_token_types.h"

/**
 * @brief 识别运算符
 * 
 * 根据运算符词素识别运算符类型，返回对应的令牌类型。
 * 
 * @param lexeme 运算符词素
 * @return Eum_TokenType 运算符令牌类型
 * @retval Eum_TOKEN_ERROR 未知运算符
 */
Eum_TokenType F_identify_operator(const char* lexeme);

/**
 * @brief 检查是否为算术运算符
 * 
 * 检查词素是否为算术运算符。
 * 
 * @param lexeme 运算符词素
 * @return Eum_TokenType 运算符令牌类型，如果不是算术运算符返回Eum_TOKEN_ERROR
 */
Eum_TokenType F_identify_arithmetic_operator(const char* lexeme);

/**
 * @brief 检查是否为比较运算符
 * 
 * 检查词素是否为比较运算符。
 * 
 * @param lexeme 运算符词素
 * @return Eum_TokenType 运算符令牌类型，如果不是比较运算符返回Eum_TOKEN_ERROR
 */
Eum_TokenType F_identify_comparison_operator(const char* lexeme);

/**
 * @brief 检查是否为赋值运算符
 * 
 * 检查词素是否为赋值运算符。
 * 
 * @param lexeme 运算符词素
 * @return Eum_TokenType 运算符令牌类型，如果不是赋值运算符返回Eum_TOKEN_ERROR
 */
Eum_TokenType F_identify_assignment_operator(const char* lexeme);

/**
 * @brief 检查是否为逻辑运算符
 * 
 * 检查词素是否为逻辑运算符。
 * 
 * @param lexeme 运算符词素
 * @return Eum_TokenType 运算符令牌类型，如果不是逻辑运算符返回Eum_TOKEN_ERROR
 */
Eum_TokenType F_identify_logical_operator(const char* lexeme);

/**
 * @brief 检查是否为位运算符
 * 
 * 检查词素是否为位运算符。
 * 
 * @param lexeme 运算符词素
 * @return Eum_TokenType 运算符令牌类型，如果不是位运算符返回Eum_TOKEN_ERROR
 */
Eum_TokenType F_identify_bitwise_operator(const char* lexeme);

/**
 * @brief 检查是否为其他运算符
 * 
 * 检查词素是否为其他类型的运算符。
 * 
 * @param lexeme 运算符词素
 * @return Eum_TokenType 运算符令牌类型，如果不是此类运算符返回Eum_TOKEN_ERROR
 */
Eum_TokenType F_identify_other_operator(const char* lexeme);

#endif // CN_LEXER_OPERATORS_H
