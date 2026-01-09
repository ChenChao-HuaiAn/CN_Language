/**
 * @file CN_lexer_token_scanners.c
 * @brief CN_Language 词法分析器令牌扫描模块实现
 * 
 * 实现各种令牌类型的扫描功能，包括标识符、数字、字符串、运算符和分隔符。
 * 负责将字符序列转换为具体的令牌。
 * 支持大文件处理和UTF-8编码。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 2.0.0
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
 * @brief 从缓冲区读取词素
 */
static char* F_read_lexeme_from_buffer(Stru_LexerBuffer_t* buffer, size_t start_pos, size_t length)
{
    if (buffer == NULL || length == 0)
    {
        return NULL;
    }
    
    char* lexeme = (char*)malloc(length + 1);
    if (lexeme == NULL)
    {
        return NULL;
    }
    
    // 读取词素数据
    for (size_t i = 0; i < length; i++)
    {
        lexeme[i] = F_buffer_read_char(buffer, start_pos + i);
    }
    lexeme[length] = '\0';
    
    return lexeme;
}

/**
 * @brief 扫描标识符或关键字
 */
Stru_Token_t* F_scan_identifier(Stru_LexerScannerState_t* state)
{
    if (state == NULL || state->buffer == NULL)
    {
        return NULL;
    }
    
    size_t start_line = state->current_line;
    size_t start_column = state->current_column;
    
    // 获取缓冲区当前位置
    size_t start_pos = F_buffer_get_position(state->buffer);
    
    // 收集标识符字符
    while (F_buffer_has_more_chars(state->buffer))
    {
        // 查看当前字符
        char current_char = F_buffer_peek_char(state->buffer);
        if (current_char == '\0')
        {
            break;
        }
        
        // 获取当前位置的缓冲区数据
        const char* buffer_data = F_buffer_get_data(state->buffer);
        size_t buffer_pos = F_buffer_get_position(state->buffer);
        size_t buffer_size = F_buffer_get_size(state->buffer);
        
        if (buffer_pos >= buffer_size)
        {
            break;
        }
        
        // 计算当前字符在缓冲区中的位置
        const char* current_char_ptr = buffer_data + buffer_pos;
        size_t remaining = buffer_size - buffer_pos;
        
        // 使用UTF-8支持检查字符
        if (!F_is_identifier_continue_utf8(current_char_ptr, remaining))
        {
            break;
        }
        
        // 获取UTF-8字符信息以确定字符长度
        Stru_UTF8CharInfo_t char_info = F_get_utf8_char_info(current_char_ptr, remaining);
        if (!char_info.is_valid)
        {
            // 无效的UTF-8编码，报告错误并跳过
            F_set_scanner_error(state, Eum_LEXER_ERROR_INVALID_UTF8,
                               "无效的UTF-8编码在位置 %zu", buffer_pos);
            F_buffer_next_char(state->buffer);
            state->current_column++;
            continue;
        }
        
        // 移动位置
        for (size_t i = 0; i < char_info.byte_length; i++)
        {
            F_buffer_next_char(state->buffer);
        }
        state->current_column++;  // UTF-8字符在列计数中算作一个字符
    }
    
    // 获取结束位置
    size_t end_pos = F_buffer_get_position(state->buffer);
    size_t length = end_pos - start_pos;
    
    if (length == 0)
    {
        return NULL;
    }
    
    // 提取词素
    char* lexeme = F_read_lexeme_from_buffer(state->buffer, start_pos, length);
    if (lexeme == NULL)
    {
        return NULL;
    }
    
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
    if (state == NULL || state->buffer == NULL)
    {
        return NULL;
    }
    
    size_t start_line = state->current_line;
    size_t start_column = state->current_column;
    
    // 获取缓冲区当前位置
    size_t start_pos = F_buffer_get_position(state->buffer);
    
    bool has_dot = false;
    bool is_float = false;
    
    // 收集数字字符
    while (F_buffer_has_more_chars(state->buffer))
    {
        char c = F_buffer_peek_char(state->buffer);
        
        if (F_is_digit(c))
        {
            F_buffer_next_char(state->buffer);
            state->current_column++;
        }
        else if (c == '.' && !has_dot)
        {
            has_dot = true;
            is_float = true;
            F_buffer_next_char(state->buffer);
            state->current_column++;
        }
        else
        {
            break;
        }
    }
    
    // 获取结束位置
    size_t end_pos = F_buffer_get_position(state->buffer);
    size_t length = end_pos - start_pos;
    
    if (length == 0)
    {
        return NULL;
    }
    
    // 提取词素
    char* lexeme = F_read_lexeme_from_buffer(state->buffer, start_pos, length);
    if (lexeme == NULL)
    {
        return NULL;
    }
    
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
    if (state == NULL || state->buffer == NULL)
    {
        return NULL;
    }
    
    size_t start_line = state->current_line;
    size_t start_column = state->current_column;
    
    // 跳过开头的引号
    char quote_char = F_buffer_peek_char(state->buffer);
    if (quote_char != '"')
    {
        return NULL;
    }
    
    F_buffer_next_char(state->buffer);
    state->current_column++;
    
    // 获取字符串开始位置
    size_t start_pos = F_buffer_get_position(state->buffer);
    
    // 收集字符串内容
    while (F_buffer_has_more_chars(state->buffer))
    {
        char c = F_buffer_peek_char(state->buffer);
        
        if (c == '"')
        {
            break;
        }
        else if (c == '\\')
        {
            // 处理转义字符
            F_buffer_next_char(state->buffer);
            state->current_column++;
            if (F_buffer_has_more_chars(state->buffer))
            {
                F_buffer_next_char(state->buffer);
                state->current_column++;
            }
        }
        else
        {
            F_buffer_next_char(state->buffer);
            state->current_column++;
        }
    }
    
    // 获取结束位置
    size_t end_pos = F_buffer_get_position(state->buffer);
    size_t length = end_pos - start_pos;
    
    // 提取词素（不包括引号）
    char* lexeme = F_read_lexeme_from_buffer(state->buffer, start_pos, length);
    if (lexeme == NULL)
    {
        return NULL;
    }
    
    // 跳过结尾的引号
    if (F_buffer_has_more_chars(state->buffer) && F_buffer_peek_char(state->buffer) == '"')
    {
        F_buffer_next_char(state->buffer);
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
    if (state == NULL || state->buffer == NULL)
    {
        return NULL;
    }
    
    size_t start_line = state->current_line;
    size_t start_column = state->current_column;
    
    // 获取第一个字符
    char first_char = F_buffer_peek_char(state->buffer);
    if (first_char == '\0')
    {
        return NULL;
    }
    
    F_buffer_next_char(state->buffer);
    state->current_column++;
    
    // 检查是否为双字符运算符
    char second_char = '\0';
    if (F_buffer_has_more_chars(state->buffer))
    {
        second_char = F_buffer_peek_char(state->buffer);
        
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
            F_buffer_next_char(state->buffer);
            state->current_column++;
        }
        else
        {
            second_char = '\0';
        }
    }
    
    // 创建词素
    size_t length = second_char ? 2 : 1;
    char* lexeme = (char*)malloc(length + 1);
    if (lexeme == NULL)
    {
        return NULL;
    }
    
    // 读取词素数据
    size_t start_pos = F_buffer_get_position(state->buffer) - length;
    for (size_t i = 0; i < length; i++)
    {
        lexeme[i] = F_buffer_read_char(state->buffer, start_pos + i);
    }
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
    if (state == NULL || state->buffer == NULL)
    {
        return NULL;
    }
    
    size_t start_line = state->current_line;
    size_t start_column = state->current_column;
    
    char c = F_buffer_peek_char(state->buffer);
    if (c == '\0')
    {
        return NULL;
    }
    
    F_buffer_next_char(state->buffer);
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
 * @brief 检查扫描器当前位置的字符是否为字母（UTF-8支持）
 */
bool F_is_alpha_at_scanner_position(Stru_LexerScannerState_t* state)
{
    if (state == NULL || state->buffer == NULL)
    {
        return false;
    }
    
    // 查看当前字符
    char current_char = F_buffer_peek_char(state->buffer);
    if (current_char == '\0')
    {
        return false;
    }
    
    // 获取当前位置的缓冲区数据
    const char* buffer_data = F_buffer_get_data(state->buffer);
    size_t buffer_pos = F_buffer_get_position(state->buffer);
    size_t buffer_size = F_buffer_get_size(state->buffer);
    
    if (buffer_pos >= buffer_size)
    {
        return false;
    }
    
    // 计算当前字符在缓冲区中的位置
    const char* current_char_ptr = buffer_data + buffer_pos;
    size_t remaining = buffer_size - buffer_pos;
    
    // 使用UTF-8支持检查字符
    return F_is_identifier_start_utf8(current_char_ptr, remaining);
}

/**
 * @brief 扫描下一个令牌
 */
Stru_Token_t* F_scan_next_token(Stru_LexerScannerState_t* state)
{
    if (state == NULL || state->buffer == NULL)
    {
        return NULL;
    }
    
    // 跳过空白字符和注释
    F_skip_whitespace(state);
    
    // 检查是否到达文件末尾
    if (!F_buffer_has_more_chars(state->buffer))
    {
        return F_create_token(Eum_TOKEN_EOF, "", state->current_line, state->current_column);
    }
    
    // 查看下一个字符
    char c = F_buffer_peek_char(state->buffer);
    
    // 根据字符类型选择扫描函数
    if (F_is_alpha_at_scanner_position(state))
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
        F_buffer_next_char(state->buffer);
        state->current_column++;
        return token;
    }
}
