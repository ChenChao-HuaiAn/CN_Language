/**
 * @file CN_token_query.c
 * @brief CN_Language 令牌类型查询模块实现
 * 
 * 实现令牌类型的查询和分类功能。
 * 遵循单一职责原则，专注于令牌类型查询。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include "CN_token_query.h"
#include "../keywords/CN_token_keywords.h"

// 判断是否为关键字令牌
bool F_token_query_is_keyword(Eum_TokenType type)
{
    return F_token_keywords_find_info(type) != NULL;
}

// 判断是否为运算符令牌
bool F_token_query_is_operator(Eum_TokenType type)
{
    // 检查符号运算符
    if (type >= Eum_TOKEN_OPERATOR_PLUS && type <= Eum_TOKEN_OPERATOR_GREATER_EQUAL)
    {
        return true;
    }
    
    // 检查关键字运算符
    const Stru_KeywordInfo_t* info = F_token_keywords_find_info(type);
    if (info != NULL && info->precedence > 0)
    {
        return true;
    }
    
    return false;
}

// 判断是否为字面量令牌
bool F_token_query_is_literal(Eum_TokenType type)
{
    return (type >= Eum_TOKEN_LITERAL_INTEGER && type <= Eum_TOKEN_LITERAL_BOOLEAN) ||
           type == Eum_TOKEN_KEYWORD_TRUE ||
           type == Eum_TOKEN_KEYWORD_FALSE ||
           type == Eum_TOKEN_KEYWORD_NULL;
}

// 判断是否为分隔符令牌
bool F_token_query_is_delimiter(Eum_TokenType type)
{
    return type >= Eum_TOKEN_DELIMITER_COMMA && type <= Eum_TOKEN_DELIMITER_RBRACKET;
}

// 获取运算符优先级
int F_token_query_get_precedence(Eum_TokenType type)
{
    const Stru_KeywordInfo_t* info = F_token_keywords_find_info(type);
    if (info != NULL)
    {
        return info->precedence;
    }
    
    // 符号运算符的优先级
    switch (type)
    {
        case Eum_TOKEN_OPERATOR_MULTIPLY:
        case Eum_TOKEN_OPERATOR_DIVIDE:
        case Eum_TOKEN_OPERATOR_MODULO:
            return 3;
        case Eum_TOKEN_OPERATOR_PLUS:
        case Eum_TOKEN_OPERATOR_MINUS:
            return 4;
        case Eum_TOKEN_OPERATOR_LESS:
        case Eum_TOKEN_OPERATOR_GREATER:
        case Eum_TOKEN_OPERATOR_LESS_EQUAL:
        case Eum_TOKEN_OPERATOR_GREATER_EQUAL:
            return 6;
        case Eum_TOKEN_OPERATOR_EQUAL:
        case Eum_TOKEN_OPERATOR_NOT_EQUAL:
            return 7;
        default:
            return 0;
    }
}
