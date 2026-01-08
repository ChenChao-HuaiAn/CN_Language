/**
 * @file CN_token_lifecycle.c
 * @brief CN_Language 令牌生命周期管理模块实现
 * 
 * 实现令牌的创建、销毁、复制等生命周期管理功能。
 * 遵循单一职责原则，专注于令牌生命周期管理。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include "CN_token_lifecycle.h"
#include <stdlib.h>
#include <string.h>

// 创建令牌
Stru_Token_t* F_token_lifecycle_create(Eum_TokenType type, const char* lexeme, size_t line, size_t column)
{
    if (lexeme == NULL)
    {
        return NULL;
    }
    
    Stru_Token_t* token = (Stru_Token_t*)malloc(sizeof(Stru_Token_t));
    if (token == NULL)
    {
        return NULL;
    }
    
    // 初始化基本字段
    token->type = type;
    token->line = line;
    token->column = column;
    token->length = strlen(lexeme);
    
    // 复制词素
    token->lexeme = (char*)malloc(token->length + 1);
    if (token->lexeme == NULL)
    {
        free(token);
        return NULL;
    }
    strcpy(token->lexeme, lexeme);
    
    // 初始化字面量值
    token->literal_value.int_value = 0;
    
    return token;
}

// 销毁令牌
void F_token_lifecycle_destroy(Stru_Token_t* token)
{
    if (token == NULL)
    {
        return;
    }
    
    if (token->lexeme != NULL)
    {
        free(token->lexeme);
    }
    
    free(token);
}

// 复制令牌
Stru_Token_t* F_token_lifecycle_copy(const Stru_Token_t* token)
{
    if (token == NULL)
    {
        return NULL;
    }
    
    Stru_Token_t* new_token = F_token_lifecycle_create(token->type, token->lexeme, token->line, token->column);
    if (new_token == NULL)
    {
        return NULL;
    }
    
    // 复制字面量值
    switch (token->type)
    {
        case Eum_TOKEN_LITERAL_INTEGER:
            new_token->literal_value.int_value = token->literal_value.int_value;
            break;
        case Eum_TOKEN_LITERAL_FLOAT:
            new_token->literal_value.float_value = token->literal_value.float_value;
            break;
        case Eum_TOKEN_LITERAL_BOOLEAN:
        case Eum_TOKEN_KEYWORD_TRUE:
        case Eum_TOKEN_KEYWORD_FALSE:
            new_token->literal_value.bool_value = token->literal_value.bool_value;
            break;
        default:
            break;
    }
    
    return new_token;
}

// 比较两个令牌
bool F_token_lifecycle_equals(const Stru_Token_t* token1, const Stru_Token_t* token2)
{
    if (token1 == NULL || token2 == NULL)
    {
        return token1 == token2;
    }
    
    if (token1->type != token2->type)
    {
        return false;
    }
    
    if (strcmp(token1->lexeme, token2->lexeme) != 0)
    {
        return false;
    }
    
    if (token1->line != token2->line || token1->column != token2->column)
    {
        return false;
    }
    
    return true;
}

// 获取令牌类型
Eum_TokenType F_token_lifecycle_get_type(const Stru_Token_t* token)
{
    return token != NULL ? token->type : Eum_TOKEN_ERROR;
}

// 获取令牌词素
const char* F_token_lifecycle_get_lexeme(const Stru_Token_t* token)
{
    return token != NULL ? token->lexeme : "";
}

// 获取令牌位置
void F_token_lifecycle_get_position(const Stru_Token_t* token, size_t* line, size_t* column)
{
    if (token != NULL && line != NULL && column != NULL)
    {
        *line = token->line;
        *column = token->column;
    }
}
