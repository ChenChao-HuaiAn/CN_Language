/**
 * @file CN_token_values.c
 * @brief CN_Language 令牌字面量值操作模块实现
 * 
 * 实现令牌字面量值的设置和获取操作。
 * 遵循单一职责原则，专注于令牌值操作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include "CN_token_values.h"

// 设置整数字面量值
void F_token_values_set_int(Stru_Token_t* token, long value)
{
    if (token != NULL && token->type == Eum_TOKEN_LITERAL_INTEGER)
    {
        token->literal_value.int_value = value;
    }
}

// 设置浮点数字面量值
void F_token_values_set_float(Stru_Token_t* token, double value)
{
    if (token != NULL && token->type == Eum_TOKEN_LITERAL_FLOAT)
    {
        token->literal_value.float_value = value;
    }
}

// 设置布尔字面量值
void F_token_values_set_bool(Stru_Token_t* token, bool value)
{
    if (token != NULL && 
        (token->type == Eum_TOKEN_LITERAL_BOOLEAN || 
         token->type == Eum_TOKEN_KEYWORD_TRUE || 
         token->type == Eum_TOKEN_KEYWORD_FALSE))
    {
        token->literal_value.bool_value = value;
    }
}

// 获取整数字面量值
long F_token_values_get_int(const Stru_Token_t* token)
{
    if (token != NULL && token->type == Eum_TOKEN_LITERAL_INTEGER)
    {
        return token->literal_value.int_value;
    }
    return 0;
}

// 获取浮点数字面量值
double F_token_values_get_float(const Stru_Token_t* token)
{
    if (token != NULL && token->type == Eum_TOKEN_LITERAL_FLOAT)
    {
        return token->literal_value.float_value;
    }
    return 0.0;
}

// 获取布尔字面量值
bool F_token_values_get_bool(const Stru_Token_t* token)
{
    if (token != NULL && 
        (token->type == Eum_TOKEN_LITERAL_BOOLEAN || 
         token->type == Eum_TOKEN_KEYWORD_TRUE || 
         token->type == Eum_TOKEN_KEYWORD_FALSE))
    {
        return token->literal_value.bool_value;
    }
    return false;
}
