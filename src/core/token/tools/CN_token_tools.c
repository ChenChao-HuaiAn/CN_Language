/**
 * @file CN_token_tools.c
 * @brief CN_Language 令牌工具模块实现
 * 
 * 实现令牌类型转换和字符串格式化等工具功能。
 * 遵循单一职责原则，专注于令牌工具功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include "CN_token_tools.h"
#include "../keywords/CN_token_keywords.h"
#include <string.h>

// 令牌类型转字符串
const char* F_token_tools_type_to_string(Eum_TokenType type)
{
    const Stru_KeywordInfo_t* info = F_token_keywords_find_info(type);
    if (info != NULL)
    {
        return info->chinese;
    }
    
    // 非关键字令牌类型
    switch (type)
    {
        case Eum_TOKEN_IDENTIFIER:
            return "标识符";
        case Eum_TOKEN_LITERAL_INTEGER:
            return "整数字面量";
        case Eum_TOKEN_LITERAL_FLOAT:
            return "浮点数字面量";
        case Eum_TOKEN_LITERAL_STRING:
            return "字符串字面量";
        case Eum_TOKEN_LITERAL_BOOLEAN:
            return "布尔字面量";
        case Eum_TOKEN_OPERATOR_PLUS:
            return "+";
        case Eum_TOKEN_OPERATOR_MINUS:
            return "-";
        case Eum_TOKEN_OPERATOR_MULTIPLY:
            return "*";
        case Eum_TOKEN_OPERATOR_DIVIDE:
            return "/";
        case Eum_TOKEN_OPERATOR_MODULO:
            return "%";
        case Eum_TOKEN_OPERATOR_ASSIGN:
            return "=";
        case Eum_TOKEN_OPERATOR_EQUAL:
            return "==";
        case Eum_TOKEN_OPERATOR_NOT_EQUAL:
            return "!=";
        case Eum_TOKEN_OPERATOR_LESS:
            return "<";
        case Eum_TOKEN_OPERATOR_GREATER:
            return ">";
        case Eum_TOKEN_OPERATOR_LESS_EQUAL:
            return "<=";
        case Eum_TOKEN_OPERATOR_GREATER_EQUAL:
            return ">=";
        case Eum_TOKEN_DELIMITER_COMMA:
            return ",";
        case Eum_TOKEN_DELIMITER_SEMICOLON:
            return ";";
        case Eum_TOKEN_DELIMITER_LPAREN:
            return "(";
        case Eum_TOKEN_DELIMITER_RPAREN:
            return ")";
        case Eum_TOKEN_DELIMITER_LBRACE:
            return "{";
        case Eum_TOKEN_DELIMITER_RBRACE:
            return "}";
        case Eum_TOKEN_DELIMITER_LBRACKET:
            return "[";
        case Eum_TOKEN_DELIMITER_RBRACKET:
            return "]";
        case Eum_TOKEN_EOF:
            return "文件结束";
        case Eum_TOKEN_ERROR:
            return "错误";
        default:
            return "未知令牌类型";
    }
}

// 打印令牌信息
int F_token_tools_to_string(const Stru_Token_t* token, char* buffer, size_t buffer_size)
{
    if (token == NULL || buffer == NULL || buffer_size == 0)
    {
        return 0;
    }
    
    const char* type_str = F_token_tools_type_to_string(token->type);
    
    int written = snprintf(buffer, buffer_size, 
                          "令牌[类型=%s, 词素='%s', 位置=%zu:%zu]",
                          type_str, token->lexeme, token->line, token->column);
    
    // 添加字面量值信息
    if (token->type == Eum_TOKEN_LITERAL_INTEGER)
    {
        written += snprintf(buffer + written, buffer_size - written, 
                           ", 值=%ld", token->literal_value.int_value);
    }
    else if (token->type == Eum_TOKEN_LITERAL_FLOAT)
    {
        written += snprintf(buffer + written, buffer_size - written, 
                           ", 值=%f", token->literal_value.float_value);
    }
    else if (token->type == Eum_TOKEN_LITERAL_BOOLEAN || 
             token->type == Eum_TOKEN_KEYWORD_TRUE || 
             token->type == Eum_TOKEN_KEYWORD_FALSE)
    {
        written += snprintf(buffer + written, buffer_size - written, 
                           ", 值=%s", token->literal_value.bool_value ? "真" : "假");
    }
    
    return written;
}

// 打印令牌信息到文件
int F_token_tools_print(const Stru_Token_t* token, FILE* stream)
{
    if (token == NULL || stream == NULL)
    {
        return 0;
    }
    
    char buffer[256];
    int length = F_token_tools_to_string(token, buffer, sizeof(buffer));
    if (length > 0)
    {
        return fprintf(stream, "%s\n", buffer);
    }
    
    return 0;
}

// 格式化令牌位置信息
int F_token_tools_format_position(size_t line, size_t column, char* buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0)
    {
        return 0;
    }
    
    return snprintf(buffer, buffer_size, "%zu:%zu", line, column);
}
