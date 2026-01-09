/**
 * @file CN_parser_token_utils.c
 * @brief CN_Language 语法分析器令牌工具模块实现
 * 
 * 语法分析器令牌工具函数模块实现，提供令牌处理相关辅助函数。
 * 包括令牌类型检查、运算符优先级计算等函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_parser_utils.h"
#include "../../token/CN_token_types.h"
#include <string.h>

// ============================================
// 令牌工具函数实现
// ============================================

/**
 * @brief 检查令牌是否为关键字
 */
bool F_is_keyword_token(Eum_TokenType token_type) {
    // 检查是否在关键字范围内
    return (token_type >= Eum_TOKEN_KEYWORD_VAR && 
            token_type <= Eum_TOKEN_KEYWORD_FINALLY);
}

/**
 * @brief 检查令牌是否为运算符
 */
bool F_is_operator_token(Eum_TokenType token_type) {
    // 检查是否为运算符关键字
    if (token_type >= Eum_TOKEN_KEYWORD_ADD && 
        token_type <= Eum_TOKEN_KEYWORD_GREATER_EQUAL) {
        return true;
    }
    
    // 检查是否为符号运算符
    if (token_type >= Eum_TOKEN_OPERATOR_PLUS && 
        token_type <= Eum_TOKEN_OPERATOR_GREATER_EQUAL) {
        return true;
    }
    
    return false;
}

/**
 * @brief 检查令牌是否为分隔符
 */
bool F_is_delimiter_token(Eum_TokenType token_type) {
    return (token_type >= Eum_TOKEN_DELIMITER_COMMA && 
            token_type <= Eum_TOKEN_DELIMITER_RBRACKET);
}

/**
 * @brief 检查令牌是否为字面量
 */
bool F_is_literal_token(Eum_TokenType token_type) {
    return (token_type >= Eum_TOKEN_LITERAL_INTEGER && 
            token_type <= Eum_TOKEN_LITERAL_BOOLEAN) ||
           (token_type == Eum_TOKEN_KEYWORD_TRUE) ||
           (token_type == Eum_TOKEN_KEYWORD_FALSE) ||
           (token_type == Eum_TOKEN_KEYWORD_NULL);
}

/**
 * @brief 检查令牌是否为类型关键字
 */
bool F_is_type_keyword(Eum_TokenType token_type) {
    // 检查是否为类型声明关键字
    if (token_type >= Eum_TOKEN_KEYWORD_INT && 
        token_type <= Eum_TOKEN_KEYWORD_REFERENCE) {
        return true;
    }
    
    // 检查是否为类型关键字
    if (token_type >= Eum_TOKEN_KEYWORD_TYPE && 
        token_type <= Eum_TOKEN_KEYWORD_TEMPLATE) {
        return true;
    }
    
    return false;
}

/**
 * @brief 获取运算符优先级
 */
int F_get_operator_precedence(Eum_TokenType token_type) {
    switch (token_type) {
        // 赋值运算符（优先级1）
        case Eum_TOKEN_OPERATOR_ASSIGN:
            return 1;
            
        // 逻辑或运算符（优先级2）
        case Eum_TOKEN_KEYWORD_OR:
            return 2;
            
        // 逻辑与运算符（优先级3）
        case Eum_TOKEN_KEYWORD_AND:
        case Eum_TOKEN_KEYWORD_AND2:
            return 3;
            
        // 比较运算符（优先级4）
        case Eum_TOKEN_OPERATOR_EQUAL:
        case Eum_TOKEN_OPERATOR_NOT_EQUAL:
        case Eum_TOKEN_KEYWORD_EQUAL:
        case Eum_TOKEN_KEYWORD_NOT_EQUAL:
            return 4;
            
        // 关系运算符（优先级5）
        case Eum_TOKEN_OPERATOR_LESS:
        case Eum_TOKEN_OPERATOR_GREATER:
        case Eum_TOKEN_OPERATOR_LESS_EQUAL:
        case Eum_TOKEN_OPERATOR_GREATER_EQUAL:
        case Eum_TOKEN_KEYWORD_LESS:
        case Eum_TOKEN_KEYWORD_GREATER:
        case Eum_TOKEN_KEYWORD_LESS_EQUAL:
        case Eum_TOKEN_KEYWORD_GREATER_EQUAL:
            return 5;
            
        // 加减运算符（优先级6）
        case Eum_TOKEN_OPERATOR_PLUS:
        case Eum_TOKEN_OPERATOR_MINUS:
        case Eum_TOKEN_KEYWORD_ADD:
        case Eum_TOKEN_KEYWORD_SUBTRACT:
            return 6;
            
        // 乘除模运算符（优先级7）
        case Eum_TOKEN_OPERATOR_MULTIPLY:
        case Eum_TOKEN_OPERATOR_DIVIDE:
        case Eum_TOKEN_OPERATOR_MODULO:
        case Eum_TOKEN_KEYWORD_MULTIPLY:
        case Eum_TOKEN_KEYWORD_DIVIDE:
        case Eum_TOKEN_KEYWORD_MODULO:
            return 7;
            
        // 一元运算符（优先级8）
        case Eum_TOKEN_KEYWORD_NOT:
            return 8;
            
        // 括号、函数调用（优先级9）
        case Eum_TOKEN_DELIMITER_LPAREN:
        case Eum_TOKEN_DELIMITER_LBRACKET:
            return 9;
            
        default:
            return 0; // 不是运算符
    }
}

/**
 * @brief 检查运算符是否为右结合
 */
bool F_is_right_associative(Eum_TokenType token_type) {
    // 赋值运算符是右结合的
    return (token_type == Eum_TOKEN_OPERATOR_ASSIGN);
}
