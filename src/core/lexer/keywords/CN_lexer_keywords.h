/**
 * @file CN_lexer_keywords.h
 * @brief CN_Language 词法分析器关键字识别模块
 * 
 * 提供中文关键字的识别功能，支持70个中文关键字。
 * 负责将标识符词素映射为对应的关键字令牌类型。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#ifndef CN_LEXER_KEYWORDS_H
#define CN_LEXER_KEYWORDS_H

#include "../../token/CN_token_types.h"

/**
 * @brief 识别关键字
 * 
 * 根据标识符词素识别是否为关键字，返回对应的令牌类型。
 * 如果不是关键字，返回Eum_TOKEN_IDENTIFIER。
 * 
 * @param lexeme 标识符词素
 * @return Eum_TokenType 令牌类型
 */
Eum_TokenType F_identify_keyword(const char* lexeme);

/**
 * @brief 检查是否为变量和类型声明关键字
 * 
 * 检查词素是否为变量和类型声明相关的关键字。
 * 
 * @param lexeme 标识符词素
 * @return Eum_TokenType 令牌类型，如果不是此类关键字返回Eum_TOKEN_IDENTIFIER
 */
Eum_TokenType F_identify_var_type_keyword(const char* lexeme);

/**
 * @brief 检查是否为控制结构关键字
 * 
 * 检查词素是否为控制结构相关的关键字。
 * 
 * @param lexeme 标识符词素
 * @return Eum_TokenType 令牌类型，如果不是此类关键字返回Eum_TOKEN_IDENTIFIER
 */
Eum_TokenType F_identify_control_keyword(const char* lexeme);

/**
 * @brief 检查是否为函数相关关键字
 * 
 * 检查词素是否为函数相关的关键字。
 * 
 * @param lexeme 标识符词素
 * @return Eum_TokenType 令牌类型，如果不是此类关键字返回Eum_TOKEN_IDENTIFIER
 */
Eum_TokenType F_identify_function_keyword(const char* lexeme);

/**
 * @brief 检查是否为逻辑运算符关键字
 * 
 * 检查词素是否为逻辑运算符相关的关键字。
 * 
 * @param lexeme 标识符词素
 * @return Eum_TokenType 令牌类型，如果不是此类关键字返回Eum_TOKEN_IDENTIFIER
 */
Eum_TokenType F_identify_logic_keyword(const char* lexeme);

/**
 * @brief 检查是否为字面量关键字
 * 
 * 检查词素是否为字面量相关的关键字。
 * 
 * @param lexeme 标识符词素
 * @return Eum_TokenType 令牌类型，如果不是此类关键字返回Eum_TOKEN_IDENTIFIER
 */
Eum_TokenType F_identify_literal_keyword(const char* lexeme);

/**
 * @brief 检查是否为模块系统关键字
 * 
 * 检查词素是否为模块系统相关的关键字。
 * 
 * @param lexeme 标识符词素
 * @return Eum_TokenType 令牌类型，如果不是此类关键字返回Eum_TOKEN_IDENTIFIER
 */
Eum_TokenType F_identify_module_keyword(const char* lexeme);

/**
 * @brief 检查是否为运算符关键字
 * 
 * 检查词素是否为运算符相关的关键字。
 * 
 * @param lexeme 标识符词素
 * @return Eum_TokenType 令牌类型，如果不是此类关键字返回Eum_TOKEN_IDENTIFIER
 */
Eum_TokenType F_identify_operator_keyword(const char* lexeme);

/**
 * @brief 检查是否为类型关键字
 * 
 * 检查词素是否为类型相关的关键字。
 * 
 * @param lexeme 标识符词素
 * @return Eum_TokenType 令牌类型，如果不是此类关键字返回Eum_TOKEN_IDENTIFIER
 */
Eum_TokenType F_identify_type_keyword(const char* lexeme);

/**
 * @brief 检查是否为其他关键字
 * 
 * 检查词素是否为其他类别的关键字。
 * 
 * @param lexeme 标识符词素
 * @return Eum_TokenType 令牌类型，如果不是此类关键字返回Eum_TOKEN_IDENTIFIER
 */
Eum_TokenType F_identify_other_keyword(const char* lexeme);

#endif // CN_LEXER_KEYWORDS_H
