/**
 * @file CN_lexer_operators.c
 * @brief CN_Language 词法分析器运算符识别模块实现
 * 
 * 实现运算符的识别功能，支持算术、比较、赋值等运算符。
 * 负责将运算符词素映射为对应的运算符令牌类型。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include "CN_lexer_operators.h"
#include <string.h>

/**
 * @brief 检查是否为算术运算符
 */
Eum_TokenType F_identify_arithmetic_operator(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_ERROR;
    }
    
    // 算术运算符
    if (strcmp(lexeme, "+") == 0) return Eum_TOKEN_OPERATOR_PLUS;
    if (strcmp(lexeme, "-") == 0) return Eum_TOKEN_OPERATOR_MINUS;
    if (strcmp(lexeme, "*") == 0) return Eum_TOKEN_OPERATOR_MULTIPLY;
    if (strcmp(lexeme, "/") == 0) return Eum_TOKEN_OPERATOR_DIVIDE;
    if (strcmp(lexeme, "%") == 0) return Eum_TOKEN_OPERATOR_MODULO;
    
    return Eum_TOKEN_ERROR;
}

/**
 * @brief 检查是否为比较运算符
 */
Eum_TokenType F_identify_comparison_operator(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_ERROR;
    }
    
    // 比较运算符
    if (strcmp(lexeme, "==") == 0) return Eum_TOKEN_OPERATOR_EQUAL;
    if (strcmp(lexeme, "!=") == 0) return Eum_TOKEN_OPERATOR_NOT_EQUAL;
    if (strcmp(lexeme, "<") == 0) return Eum_TOKEN_OPERATOR_LESS;
    if (strcmp(lexeme, ">") == 0) return Eum_TOKEN_OPERATOR_GREATER;
    if (strcmp(lexeme, "<=") == 0) return Eum_TOKEN_OPERATOR_LESS_EQUAL;
    if (strcmp(lexeme, ">=") == 0) return Eum_TOKEN_OPERATOR_GREATER_EQUAL;
    
    return Eum_TOKEN_ERROR;
}

/**
 * @brief 检查是否为赋值运算符
 */
Eum_TokenType F_identify_assignment_operator(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_ERROR;
    }
    
    // 赋值运算符
    if (strcmp(lexeme, "=") == 0) return Eum_TOKEN_OPERATOR_ASSIGN;
    if (strcmp(lexeme, "+=") == 0) return Eum_TOKEN_OPERATOR_PLUS_ASSIGN;
    if (strcmp(lexeme, "-=") == 0) return Eum_TOKEN_OPERATOR_MINUS_ASSIGN;
    if (strcmp(lexeme, "*=") == 0) return Eum_TOKEN_OPERATOR_MULTIPLY_ASSIGN;
    if (strcmp(lexeme, "/=") == 0) return Eum_TOKEN_OPERATOR_DIVIDE_ASSIGN;
    if (strcmp(lexeme, "%=") == 0) return Eum_TOKEN_OPERATOR_MODULO_ASSIGN;
    
    return Eum_TOKEN_ERROR;
}

/**
 * @brief 检查是否为逻辑运算符
 */
Eum_TokenType F_identify_logical_operator(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_ERROR;
    }
    
    // 逻辑运算符（符号形式）
    if (strcmp(lexeme, "&&") == 0) return Eum_TOKEN_KEYWORD_AND;
    if (strcmp(lexeme, "||") == 0) return Eum_TOKEN_KEYWORD_OR;
    if (strcmp(lexeme, "!") == 0) return Eum_TOKEN_KEYWORD_NOT;
    
    return Eum_TOKEN_ERROR;
}

/**
 * @brief 检查是否为位运算符
 */
Eum_TokenType F_identify_bitwise_operator(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_ERROR;
    }
    
    // 位运算符
    if (strcmp(lexeme, "&") == 0) return Eum_TOKEN_ERROR; // 位与
    if (strcmp(lexeme, "|") == 0) return Eum_TOKEN_ERROR; // 位或
    if (strcmp(lexeme, "^") == 0) return Eum_TOKEN_ERROR; // 位异或
    if (strcmp(lexeme, "~") == 0) return Eum_TOKEN_ERROR; // 位非
    if (strcmp(lexeme, "<<") == 0) return Eum_TOKEN_ERROR; // 左移
    if (strcmp(lexeme, ">>") == 0) return Eum_TOKEN_ERROR; // 右移
    
    return Eum_TOKEN_ERROR;
}

/**
 * @brief 检查是否为其他运算符
 */
Eum_TokenType F_identify_other_operator(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_ERROR;
    }
    
    // 其他运算符
    if (strcmp(lexeme, ".") == 0) return Eum_TOKEN_ERROR; // 成员访问
    if (strcmp(lexeme, "->") == 0) return Eum_TOKEN_ERROR; // 指针成员访问
    if (strcmp(lexeme, "++") == 0) return Eum_TOKEN_ERROR; // 自增
    if (strcmp(lexeme, "--") == 0) return Eum_TOKEN_ERROR; // 自减
    if (strcmp(lexeme, "?") == 0) return Eum_TOKEN_ERROR; // 三元运算符
    if (strcmp(lexeme, ":") == 0) return Eum_TOKEN_ERROR; // 三元运算符
    
    return Eum_TOKEN_ERROR;
}

/**
 * @brief 识别运算符
 */
Eum_TokenType F_identify_operator(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_ERROR;
    }
    
    // 按类别检查运算符，提高效率
    Eum_TokenType type;
    
    // 1. 检查算术运算符
    type = F_identify_arithmetic_operator(lexeme);
    if (type != Eum_TOKEN_ERROR) return type;
    
    // 2. 检查比较运算符
    type = F_identify_comparison_operator(lexeme);
    if (type != Eum_TOKEN_ERROR) return type;
    
    // 3. 检查赋值运算符
    type = F_identify_assignment_operator(lexeme);
    if (type != Eum_TOKEN_ERROR) return type;
    
    // 4. 检查逻辑运算符
    type = F_identify_logical_operator(lexeme);
    if (type != Eum_TOKEN_ERROR) return type;
    
    // 5. 检查位运算符
    type = F_identify_bitwise_operator(lexeme);
    if (type != Eum_TOKEN_ERROR) return type;
    
    // 6. 检查其他运算符
    type = F_identify_other_operator(lexeme);
    if (type != Eum_TOKEN_ERROR) return type;
    
    // 未知运算符
    return Eum_TOKEN_ERROR;
}
