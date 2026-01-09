/**
 * @file CN_lexer_token_scanners.c
 * @brief CN_Language 词法分析器令牌扫描模块实现
 * 
 * 实现各种令牌类型的扫描功能，包括标识符、数字、字符串、运算符和分隔符。
 * 负责将字符序列转换为具体的令牌。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include "CN_lexer_token_scanners.h"
#include "../keywords/CN_lexer_keywords.h"
#include "../operators/CN_lexer_operators.h"
#include "../utils/CN_lexer_utils.h"
#include "../errors/CN_lexer_errors.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief 检查字符是否为字母（包括中文字符）
 */
bool F_is_alpha(char c)
{
    return F_is_alpha_utf8(c);
}

/**
 * @brief 检查字符是否为数字
 */
bool F_is_digit(char c)
{
    return F_is_digit_simple(c);
}

/**
 * @brief 检查字符是否为字母或数字
 */
bool F_is_alpha_numeric(char c)
{
    return F_is_alnum(c);
}

/**
 * @brief 检查字符是否为运算符字符
 */
bool F_is_operator_char(char c)
{
    return F_is_operator_char_simple(c);
}

/**
 * @brief 检查字符是否为分隔符字符
 */
bool F_is_delimiter_char(char c)
{
    return F_is_delimiter_char_simple(c);
}

/**
 * @brief 扫描标识符或关键字
 */
Stru_Token_t* F_scan_identifier(Stru_LexerScannerState_t* state)
{
    if (state == NULL || state->source == NULL)
    {
        return NULL;
    }
    
    size_t start_pos = state->current_pos;
    size_t start_line = state->current_line;
    size_t start_column = state->current_column;
    
    // 收集标识符字符
    while (state->current_pos < state->source_length)
    {
        char c = state->source[state->current_pos];
        
        // 检查是否为字母或数字
        if (!F_is_alpha_numeric(c))
        {
            break;
        }
        
        // 如果是中文字符的第一个字节（UTF-8编码），跳过整个字符
        if ((unsigned char)c >= 0xE0 && (unsigned char)c <= 0xEF)
        {
            // UTF-8中文字符通常是3个字节
            state->current_pos += 3;
            state->current_column++;  // 中文字符在列计数中算作一个字符
        }
        else if ((unsigned char)c >= 0xC0 && (unsigned char)c <= 0xDF)
        {
            // 2字节UTF-8字符
            state->current_pos += 2;
            state->current_column++;
        }
        else
        {
            // ASCII字符
            state->current_pos++;
            state->current_column++;
        }
    }
    
    // 提取词素
    size_t length = state->current_pos - start_pos;
    char* lexeme = (char*)malloc(length + 1);
    if (lexeme == NULL)
    {
        return NULL;
    }
    
    strncpy(lexeme, state->source + start_pos, length);
    lexeme[length] = '\0';
    
    // 识别关键字
    Eum_TokenType type = F_identify_keyword(lexeme);
    
    // 创建令牌
    Stru_Token_t* token = F_create_token(type, lexeme, start_line, start_column);
    free(lexeme);
    
    return token;
}

/**
 * @brief 扫描数字字面量
 */
Stru_Token_t* F_scan_number(Stru_LexerScannerState_t* state)
{
    if (state == NULL || state->source == NULL)
    {
        return NULL;
    }
    
    size_t start_pos = state->current_pos;
    size_t start_line = state->current_line;
    size_t start_column = state->current_column;
    
    bool has_dot = false;
    bool is_float = false;
    
    // 收集数字字符
    while (state->current_pos < state->source_length)
    {
        char c = state->source[state->current_pos];
        
        if (F_is_digit(c))
        {
            state->current_pos++;
            state->current_column++;
        }
        else if (c == '.' && !has_dot)
        {
            has_dot = true;
            is_float = true;
            state->current_pos++;
            state->current_column++;
        }
        else
        {
            break;
        }
    }
    
    // 提取词素
    size_t length = state->current_pos - start_pos;
    char* lexeme = (char*)malloc(length + 1);
    if (lexeme == NULL)
    {
        return NULL;
    }
    
    strncpy(lexeme, state->source + start_pos, length);
    lexeme[length] = '\0';
    
    // 创建令牌
    Eum_TokenType type = is_float ? Eum_TOKEN_LITERAL_FLOAT : Eum_TOKEN_LITERAL_INTEGER;
    Stru_Token_t* token = F_create_token(type, lexeme, start_line, start_column);
    
    // 设置字面量值
    if (is_float)
    {
        double value = atof(lexeme);
        F_token_set_float_value(token, value);
    }
    else
    {
        long value = atol(lexeme);
        F_token_set_int_value(token, value);
    }
    
    free(lexeme);
    return token;
}

/**
 * @brief 扫描字符串字面量
 */
Stru_Token_t* F_scan_string(Stru_LexerScannerState_t* state)
{
    if (state == NULL || state->source == NULL)
    {
        return NULL;
    }
    
    size_t start_line = state->current_line;
    size_t start_column = state->current_column;
    
    // 跳过开头的引号
    state->current_pos++;
    state->current_column++;
    
    size_t start_pos = state->current_pos;
    
    // 收集字符串内容
    while (state->current_pos < state->source_length)
    {
        char c = state->source[state->current_pos];
        
        if (c == '"')
        {
            break;
        }
        else if (c == '\\')
        {
            // 处理转义字符
            state->current_pos++;
            state->current_column++;
            if (state->current_pos < state->source_length)
            {
                state->current_pos++;
                state->current_column++;
            }
        }
        else
        {
            state->current_pos++;
            state->current_column++;
        }
    }
    
    // 提取词素（不包括引号）
    size_t length = state->current_pos - start_pos;
    char* lexeme = (char*)malloc(length + 1);
    if (lexeme == NULL)
    {
        return NULL;
    }
    
    strncpy(lexeme, state->source + start_pos, length);
    lexeme[length] = '\0';
    
    // 跳过结尾的引号
    if (state->current_pos < state->source_length && state->source[state->current_pos] == '"')
    {
        state->current_pos++;
        state->current_column++;
    }
    
    // 创建令牌
    Stru_Token_t* token = F_create_token(Eum_TOKEN_LITERAL_STRING, lexeme, start_line, start_column);
    free(lexeme);
    
    return token;
}

/**
 * @brief 扫描运算符
 */
Stru_Token_t* F_scan_operator(Stru_LexerScannerState_t* state)
{
    if (state == NULL || state->source == NULL)
    {
        return NULL;
    }
    
    size_t start_pos = state->current_pos;
    size_t start_line = state->current_line;
    size_t start_column = state->current_column;
    
    // 获取第一个字符
    char first_char = state->source[state->current_pos];
    state->current_pos++;
    state->current_column++;
    
    // 检查是否为双字符运算符
    char second_char = '\0';
    if (state->current_pos < state->source_length)
    {
        second_char = state->source[state->current_pos];
        
        // 检查可能的双字符运算符
        if ((first_char == '=' && second_char == '=') ||  // ==
            (first_char == '!' && second_char == '=') ||  // !=
            (first_char == '<' && second_char == '=') ||  // <=
            (first_char == '>' && second_char == '=') ||  // >=
            (first_char == '+' && second_char == '=') ||  // +=
            (first_char == '-' && second_char == '=') ||  // -=
            (first_char == '*' && second_char == '=') ||  // *=
            (first_char == '/' && second_char == '='))    // /=
        {
            state->current_pos++;
            state->current_column++;
        }
        else
        {
            second_char = '\0';
        }
    }
    
    // 提取词素
    size_t length = second_char ? 2 : 1;
    char* lexeme = (char*)malloc(length + 1);
    if (lexeme == NULL)
    {
        return NULL;
    }
    
    strncpy(lexeme, state->source + start_pos, length);
    lexeme[length] = '\0';
    
    // 识别运算符类型
    Eum_TokenType type = F_identify_operator(lexeme);
    Stru_Token_t* token = F_create_token(type, lexeme, start_line, start_column);
    free(lexeme);
    
    return token;
}

/**
 * @brief 扫描分隔符
 */
Stru_Token_t* F_scan_delimiter(Stru_LexerScannerState_t* state)
{
    if (state == NULL || state->source == NULL)
    {
        return NULL;
    }
    
    size_t start_line = state->current_line;
    size_t start_column = state->current_column;
    
    char c = state->source[state->current_pos];
    state->current_pos++;
    state->current_column++;
    
    // 创建词素
    char lexeme[2] = {c, '\0'};
    
    // 识别分隔符类型
    Eum_TokenType type = Eum_TOKEN_ERROR;
    switch (c)
    {
        case ',':
            type = Eum_TOKEN_DELIMITER_COMMA;
            break;
        case ';':
            type = Eum_TOKEN_DELIMITER_SEMICOLON;
            break;
        case ':':
            type = Eum_TOKEN_DELIMITER_COLON;
            break;
        case '(':
            type = Eum_TOKEN_DELIMITER_LPAREN;
            break;
        case ')':
            type = Eum_TOKEN_DELIMITER_RPAREN;
            break;
        case '{':
            type = Eum_TOKEN_DELIMITER_LBRACE;
            break;
        case '}':
            type = Eum_TOKEN_DELIMITER_RBRACE;
            break;
        case '[':
            type = Eum_TOKEN_DELIMITER_LBRACKET;
            break;
        case ']':
            type = Eum_TOKEN_DELIMITER_RBRACKET;
            break;
        default:
            type = Eum_TOKEN_ERROR;
            break;
    }
    
    // 创建令牌
    Stru_Token_t* token = F_create_token(type, lexeme, start_line, start_column);
    return token;
}

/**
 * @brief 扫描下一个令牌
 */
Stru_Token_t* F_scan_next_token(Stru_LexerScannerState_t* state)
{
    if (state == NULL || state->source == NULL)
    {
        return NULL;
    }
    
    // 跳过空白字符和注释
    F_skip_whitespace(state);
    
    // 检查是否到达文件末尾
    if (state->current_pos >= state->source_length)
    {
        return F_create_token(Eum_TOKEN_EOF, "", state->current_line, state->current_column);
    }
    
    // 查看下一个字符
    char c = F_peek_char(state);
    
    // 根据字符类型选择扫描函数
    if (F_is_alpha(c))
    {
        return F_scan_identifier(state);
    }
    else if (F_is_digit(c))
    {
        return F_scan_number(state);
    }
    else if (c == '"')
    {
        return F_scan_string(state);
    }
    else if (F_is_operator_char(c))
    {
        return F_scan_operator(state);
    }
    else if (F_is_delimiter_char(c))
    {
        return F_scan_delimiter(state);
    }
    else
    {
        // 未知字符，创建错误令牌
        char lexeme[2] = {c, '\0'};
        Stru_Token_t* token = F_create_token(Eum_TOKEN_ERROR, lexeme, 
                                            state->current_line, state->current_column);
        F_set_scanner_error(state, Eum_LEXER_ERROR_UNKNOWN_CHAR, 
                           "未知字符: %c (0x%02x)", c, (unsigned char)c);
        state->current_pos++;
        state->current_column++;
        return token;
    }
}
