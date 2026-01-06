/**
 * @file CN_lexer_impl.c
 * @brief CN_Language 词法分析器核心实现
 * 
 * 实现词法分析器的核心功能，包括中文关键字识别、运算符识别、字面量识别等。
 * 遵循单一职责原则：每个函数不超过50行，每个文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 */

#include "CN_lexer_interface.h"
#include "CN_token.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

// ============================================
// 内部状态结构体
// ============================================

/**
 * @brief 词法分析器内部状态
 * 
 * 存储词法分析器的内部状态，包括源代码、位置信息和错误状态。
 */
typedef struct {
    const char* source;          ///< 源代码
    size_t source_length;        ///< 源代码长度
    const char* source_name;     ///< 源文件名
    size_t current_pos;          ///< 当前位置（字节偏移）
    size_t current_line;         ///< 当前行号
    size_t current_column;       ///< 当前列号
    bool has_error;              ///< 是否有错误
    char error_message[256];     ///< 错误信息
} Stru_LexerState_t;

// ============================================
// 内部辅助函数声明
// ============================================

static Stru_LexerState_t* F_create_lexer_state(void);
static void F_destroy_lexer_state(Stru_LexerState_t* state);
static void F_set_error(Stru_LexerState_t* state, const char* format, ...);
static void F_clear_error(Stru_LexerState_t* state);

static bool F_is_whitespace(char c);
static bool F_is_digit(char c);
static bool F_is_alpha(char c);
static bool F_is_alpha_numeric(char c);
static bool F_is_operator_char(char c);
static bool F_is_delimiter_char(char c);

static char F_peek_char(const Stru_LexerState_t* state);
static char F_next_char(Stru_LexerState_t* state);
static void F_skip_whitespace(Stru_LexerState_t* state);
static void F_skip_comment(Stru_LexerState_t* state);

static Stru_Token_t* F_scan_identifier(Stru_LexerState_t* state);
static Stru_Token_t* F_scan_number(Stru_LexerState_t* state);
static Stru_Token_t* F_scan_string(Stru_LexerState_t* state);
static Stru_Token_t* F_scan_operator(Stru_LexerState_t* state);
static Stru_Token_t* F_scan_delimiter(Stru_LexerState_t* state);

static Eum_TokenType F_identify_keyword(const char* lexeme);
static Eum_TokenType F_identify_operator(const char* lexeme);

// ============================================
// 状态管理函数
// ============================================

/**
 * @brief 创建内部状态
 */
static Stru_LexerState_t* F_create_lexer_state(void)
{
    Stru_LexerState_t* state = (Stru_LexerState_t*)malloc(sizeof(Stru_LexerState_t));
    if (state == NULL)
    {
        return NULL;
    }
    
    // 初始化状态
    state->source = NULL;
    state->source_length = 0;
    state->source_name = NULL;
    state->current_pos = 0;
    state->current_line = 1;
    state->current_column = 1;
    state->has_error = false;
    state->error_message[0] = '\0';
    
    return state;
}

/**
 * @brief 销毁内部状态
 */
static void F_destroy_lexer_state(Stru_LexerState_t* state)
{
    if (state != NULL)
    {
        free(state);
    }
}

/**
 * @brief 设置错误信息
 */
static void F_set_error(Stru_LexerState_t* state, const char* format, ...)
{
    if (state == NULL)
    {
        return;
    }
    
    state->has_error = true;
    
    // 简化错误设置，实际实现应使用va_list
    if (format != NULL)
    {
        snprintf(state->error_message, sizeof(state->error_message), "%s", format);
    }
    else
    {
        state->error_message[0] = '\0';
    }
}

/**
 * @brief 清除错误信息
 */
static void F_clear_error(Stru_LexerState_t* state)
{
    if (state != NULL)
    {
        state->has_error = false;
        state->error_message[0] = '\0';
    }
}

// ============================================
// 字符分类函数
// ============================================

/**
 * @brief 检查字符是否为空白字符
 */
static bool F_is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/**
 * @brief 检查字符是否为数字
 */
static bool F_is_digit(char c)
{
    return c >= '0' && c <= '9';
}

/**
 * @brief 检查字符是否为字母（包括中文字符）
 */
static bool F_is_alpha(char c)
{
    // 简单实现：检查是否为ASCII字母或中文字符的第一个字节
    // 实际实现应处理UTF-8编码
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c & 0x80);
}

/**
 * @brief 检查字符是否为字母或数字
 */
static bool F_is_alpha_numeric(char c)
{
    return F_is_alpha(c) || F_is_digit(c);
}

/**
 * @brief 检查字符是否为运算符字符
 */
static bool F_is_operator_char(char c)
{
    return strchr("+-*/%=!<>", c) != NULL;
}

/**
 * @brief 检查字符是否为分隔符字符
 */
static bool F_is_delimiter_char(char c)
{
    return strchr(",;(){}[]", c) != NULL;
}

// ============================================
// 字符操作函数
// ============================================

/**
 * @brief 查看下一个字符（不移动位置）
 */
static char F_peek_char(const Stru_LexerState_t* state)
{
    if (state == NULL || state->source == NULL || state->current_pos >= state->source_length)
    {
        return '\0';
    }
    
    return state->source[state->current_pos];
}

/**
 * @brief 获取下一个字符并移动位置
 */
static char F_next_char(Stru_LexerState_t* state)
{
    if (state == NULL || state->source == NULL || state->current_pos >= state->source_length)
    {
        return '\0';
    }
    
    char c = state->source[state->current_pos];
    state->current_pos++;
    
    if (c == '\n')
    {
        state->current_line++;
        state->current_column = 1;
    }
    else
    {
        state->current_column++;
    }
    
    return c;
}

/**
 * @brief 跳过空白字符
 */
static void F_skip_whitespace(Stru_LexerState_t* state)
{
    if (state == NULL || state->source == NULL)
    {
        return;
    }
    
    while (state->current_pos < state->source_length)
    {
        char c = state->source[state->current_pos];
        
        if (c == ' ' || c == '\t')
        {
            state->current_pos++;
            state->current_column++;
        }
        else if (c == '\n')
        {
            state->current_pos++;
            state->current_line++;
            state->current_column = 1;
        }
        else if (c == '\r')
        {
            state->current_pos++;
            // 处理Windows换行符\r\n
            if (state->current_pos < state->source_length && 
                state->source[state->current_pos] == '\n')
            {
                state->current_pos++;
            }
            state->current_line++;
            state->current_column = 1;
        }
        else if (c == '#')
        {
            // 跳过注释
            F_skip_comment(state);
        }
        else
        {
            break;
        }
    }
}

/**
 * @brief 跳过注释
 */
static void F_skip_comment(Stru_LexerState_t* state)
{
    if (state == NULL || state->source == NULL)
    {
        return;
    }
    
    // 跳过注释字符
    state->current_pos++;
    state->current_column++;
    
    // 跳过直到行尾
    while (state->current_pos < state->source_length)
    {
        char c = state->source[state->current_pos];
        if (c == '\n' || c == '\r')
        {
            break;
        }
        state->current_pos++;
        state->current_column++;
    }
}

// ============================================
// 令牌扫描函数
// ============================================

/**
 * @brief 扫描标识符或关键字
 */
static Stru_Token_t* F_scan_identifier(Stru_LexerState_t* state)
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
        if (!F_is_alpha_numeric(c))
        {
            break;
        }
        state->current_pos++;
        state->current_column++;
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
    if (type == Eum_TOKEN_IDENTIFIER)
    {
        // 不是关键字，是普通标识符
        type = Eum_TOKEN_IDENTIFIER;
    }
    
    // 创建令牌
    Stru_Token_t* token = F_create_token(type, lexeme, start_line, start_column);
    free(lexeme);
    
    return token;
}

/**
 * @brief 扫描数字字面量
 */
static Stru_Token_t* F_scan_number(Stru_LexerState_t* state)
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
static Stru_Token_t* F_scan_string(Stru_LexerState_t* state)
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
static Stru_Token_t* F_scan_operator(Stru_LexerState_t* state)
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
static Stru_Token_t* F_scan_delimiter(Stru_LexerState_t* state)
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

// ============================================
// 关键字和运算符识别函数
// ============================================

/**
 * @brief 识别关键字
 */
static Eum_TokenType F_identify_keyword(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_IDENTIFIER;
    }
    
    // 这里应该使用CN_token.c中的关键字表
    // 简化实现：只识别一些常见关键字
    if (strcmp(lexeme, "变量") == 0) return Eum_TOKEN_KEYWORD_VAR;
    if (strcmp(lexeme, "如果") == 0) return Eum_TOKEN_KEYWORD_IF;
    if (strcmp(lexeme, "否则") == 0) return Eum_TOKEN_KEYWORD_ELSE;
    if (strcmp(lexeme, "当") == 0) return Eum_TOKEN_KEYWORD_WHILE;
    if (strcmp(lexeme, "循环") == 0) return Eum_TOKEN_KEYWORD_FOR;
    if (strcmp(lexeme, "返回") == 0) return Eum_TOKEN_KEYWORD_RETURN;
    if (strcmp(lexeme, "函数") == 0) return Eum_TOKEN_KEYWORD_FUNCTION;
    if (strcmp(lexeme, "主程序") == 0) return Eum_TOKEN_KEYWORD_MAIN;
    if (strcmp(lexeme, "常量") == 0) return Eum_TOKEN_KEYWORD_CONST;
    if (strcmp(lexeme, "真") == 0) return Eum_TOKEN_KEYWORD_TRUE;
    if (strcmp(lexeme, "假") == 0) return Eum_TOKEN_KEYWORD_FALSE;
    if (strcmp(lexeme, "空") == 0) return Eum_TOKEN_KEYWORD_NULL;
    if (strcmp(lexeme, "无") == 0) return Eum_TOKEN_KEYWORD_VOID;
    if (strcmp(lexeme, "模块") == 0) return Eum_TOKEN_KEYWORD_MODULE;
    if (strcmp(lexeme, "导入") == 0) return Eum_TOKEN_KEYWORD_IMPORT;
    if (strcmp(lexeme, "导出") == 0) return Eum_TOKEN_KEYWORD_EXPORT;
    if (strcmp(lexeme, "类型") == 0) return Eum_TOKEN_KEYWORD_TYPE;
    if (strcmp(lexeme, "类") == 0) return Eum_TOKEN_KEYWORD_CLASS;
    if (strcmp(lexeme, "对象") == 0) return Eum_TOKEN_KEYWORD_OBJECT;
    
    return Eum_TOKEN_IDENTIFIER;
}

/**
 * @brief 识别运算符
 */
static Eum_TokenType F_identify_operator(const char* lexeme)
{
    if (lexeme == NULL)
    {
        return Eum_TOKEN_ERROR;
    }
    
    if (strcmp(lexeme, "+") == 0) return Eum_TOKEN_OPERATOR_PLUS;
    if (strcmp(lexeme, "-") == 0) return Eum_TOKEN_OPERATOR_MINUS;
    if (strcmp(lexeme, "*") == 0) return Eum_TOKEN_OPERATOR_MULTIPLY;
    if (strcmp(lexeme, "/") == 0) return Eum_TOKEN_OPERATOR_DIVIDE;
    if (strcmp(lexeme, "%") == 0) return Eum_TOKEN_OPERATOR_MODULO;
    if (strcmp(lexeme, "=") == 0) return Eum_TOKEN_OPERATOR_ASSIGN;
    if (strcmp(lexeme, "==") == 0) return Eum_TOKEN_OPERATOR_EQUAL;
    if (strcmp(lexeme, "!=") == 0) return Eum_TOKEN_OPERATOR_NOT_EQUAL;
    if (strcmp(lexeme, "<") == 0) return Eum_TOKEN_OPERATOR_LESS;
    if (strcmp(lexeme, ">") == 0) return Eum_TOKEN_OPERATOR_GREATER;
    if (strcmp(lexeme, "<=") == 0) return Eum_TOKEN_OPERATOR_LESS_EQUAL;
    if (strcmp(lexeme, ">=") == 0) return Eum_TOKEN_OPERATOR_GREATER_EQUAL;
    if (strcmp(lexeme, "+=") == 0) return Eum_TOKEN_OPERATOR_PLUS;
    if (strcmp(lexeme, "-=") == 0) return Eum_TOKEN_OPERATOR_MINUS;
    if (strcmp(lexeme, "*=") == 0) return Eum_TOKEN_OPERATOR_MULTIPLY;
    if (strcmp(lexeme, "/=") == 0) return Eum_TOKEN_OPERATOR_DIVIDE;
    
    return Eum_TOKEN_ERROR;
}

// ============================================
// 主扫描函数
// ============================================

/**
 * @brief 扫描下一个令牌
 */
static Stru_Token_t* F_scan_next_token(Stru_LexerState_t* state)
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
        F_set_error(state, "未知字符: %c (0x%02x)", c, (unsigned char)c);
        state->current_pos++;
        state->current_column++;
        return token;
    }
}

// ============================================
// 接口函数实现（包装器）
// ============================================

// 辅助函数：从接口获取内部状态
static Stru_LexerState_t* F_get_lexer_state(Stru_LexerInterface_t* interface)
{
    if (interface == NULL)
    {
        return NULL;
    }
    return (Stru_LexerState_t*)interface->internal_state;
}

/**
 * @brief 初始化词法分析器（包装器）
 */
static bool F_lexer_initialize_wrapper(Stru_LexerInterface_t* interface, const char* source, size_t length, const char* source_name)
{
    Stru_LexerState_t* state = F_get_lexer_state(interface);
    if (state == NULL)
    {
        return false;
    }
    
    if (source == NULL)
    {
        F_set_error(state, "无效的初始化参数：源代码为空");
        return false;
    }
    
    // 重置状态
    state->source = source;
    state->source_length = length;
    state->source_name = source_name ? source_name : "unknown.cn";
    state->current_pos = 0;
    state->current_line = 1;
    state->current_column = 1;
    F_clear_error(state);
    
    return true;
}

/**
 * @brief 获取下一个令牌（包装器）
 */
static Stru_Token_t* F_lexer_next_token_wrapper(Stru_LexerInterface_t* interface)
{
    Stru_LexerState_t* state = F_get_lexer_state(interface);
    if (state == NULL || state->source == NULL)
    {
        return NULL;
    }
    
    return F_scan_next_token(state);
}

/**
 * @brief 检查是否还有更多令牌（包装器）
 */
static bool F_lexer_has_more_tokens_wrapper(Stru_LexerInterface_t* interface)
{
    Stru_LexerState_t* state = F_get_lexer_state(interface);
    if (state == NULL || state->source == NULL)
    {
        return false;
    }
    
    // 保存当前位置
    size_t saved_pos = state->current_pos;
    size_t saved_line = state->current_line;
    size_t saved_column = state->current_column;
    
    // 跳过空白字符
    F_skip_whitespace(state);
    
    // 检查是否还有字符
    bool has_more = state->current_pos < state->source_length;
    
    // 恢复位置
    state->current_pos = saved_pos;
    state->current_line = saved_line;
    state->current_column = saved_column;
    
    return has_more;
}

/**
 * @brief 批量令牌化（包装器）
 */
static Stru_DynamicArray_t* F_lexer_tokenize_all_wrapper(Stru_LexerInterface_t* interface)
{
    Stru_LexerState_t* state = F_get_lexer_state(interface);
    if (state == NULL || state->source == NULL)
    {
        return NULL;
    }
    
    // 创建动态数组
    Stru_DynamicArray_t* tokens = F_create_dynamic_array(sizeof(Stru_Token_t*));
    if (tokens == NULL)
    {
        return NULL;
    }
    
    // 保存当前位置
    size_t saved_pos = state->current_pos;
    size_t saved_line = state->current_line;
    size_t saved_column = state->current_column;
    
    // 重置到开始位置
    state->current_pos = 0;
    state->current_line = 1;
    state->current_column = 1;
    F_clear_error(state);
    
    // 收集所有令牌
    while (true)
    {
        Stru_Token_t* token = F_scan_next_token(state);
        if (token == NULL)
        {
            break;
        }
        
        if (token->type == Eum_TOKEN_EOF)
        {
            F_dynamic_array_push(tokens, &token);
            break;
        }
        
        F_dynamic_array_push(tokens, &token);
        
        if (token->type == Eum_TOKEN_ERROR)
        {
            break;
        }
    }
    
    // 恢复位置
    state->current_pos = saved_pos;
    state->current_line = saved_line;
    state->current_column = saved_column;
    
    return tokens;
}

/**
 * @brief 获取当前位置（包装器）
 */
static void F_lexer_get_position_wrapper(Stru_LexerInterface_t* interface, size_t* line, size_t* column)
{
    Stru_LexerState_t* state = F_get_lexer_state(interface);
    if (state == NULL || line == NULL || column == NULL)
    {
        return;
    }
    
    *line = state->current_line;
    *column = state->current_column;
}

/**
 * @brief 获取源文件名（包装器）
 */
static const char* F_lexer_get_source_name_wrapper(Stru_LexerInterface_t* interface)
{
    Stru_LexerState_t* state = F_get_lexer_state(interface);
    if (state == NULL)
    {
        return "unknown.cn";
    }
    
    return state->source_name ? state->source_name : "unknown.cn";
}

/**
 * @brief 检查是否有错误（包装器）
 */
static bool F_lexer_has_errors_wrapper(Stru_LexerInterface_t* interface)
{
    Stru_LexerState_t* state = F_get_lexer_state(interface);
    if (state == NULL)
    {
        return false;
    }
    
    return state->has_error;
}

/**
 * @brief 获取最后一个错误信息（包装器）
 */
static const char* F_lexer_get_last_error_wrapper(Stru_LexerInterface_t* interface)
{
    Stru_LexerState_t* state = F_get_lexer_state(interface);
    if (state == NULL)
    {
        return "";
    }
    
    return state->error_message;
}

/**
 * @brief 重置词法分析器（包装器）
 */
static void F_lexer_reset_wrapper(Stru_LexerInterface_t* interface)
{
    Stru_LexerState_t* state = F_get_lexer_state(interface);
    if (state == NULL)
    {
        return;
    }
    
    state->current_pos = 0;
    state->current_line = 1;
    state->current_column = 1;
    F_clear_error(state);
}

/**
 * @brief 销毁词法分析器（包装器）
 */
static void F_lexer_destroy_wrapper(Stru_LexerInterface_t* interface)
{
    if (interface == NULL)
    {
        return;
    }
    
    if (interface->internal_state != NULL)
    {
        F_destroy_lexer_state((Stru_LexerState_t*)interface->internal_state);
        interface->internal_state = NULL;
    }
    
    free(interface);
}

// ============================================
// 工厂函数实现
// ============================================

/**
 * @brief 创建词法分析器接口实例（实现版本）
 */
Stru_LexerInterface_t* F_create_lexer_interface_impl(void)
{
    // 分配接口内存
    Stru_LexerInterface_t* interface = (Stru_LexerInterface_t*)malloc(sizeof(Stru_LexerInterface_t));
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 创建内部状态
    Stru_LexerState_t* state = F_create_lexer_state();
    if (state == NULL)
    {
        free(interface);
        return NULL;
    }
    
    // 设置接口函数指针
    interface->initialize = F_lexer_initialize_wrapper;
    interface->next_token = F_lexer_next_token_wrapper;
    interface->has_more_tokens = F_lexer_has_more_tokens_wrapper;
    interface->tokenize_all = F_lexer_tokenize_all_wrapper;
    interface->get_position = F_lexer_get_position_wrapper;
    interface->get_source_name = F_lexer_get_source_name_wrapper;
    interface->has_errors = F_lexer_has_errors_wrapper;
    interface->get_last_error = F_lexer_get_last_error_wrapper;
    interface->reset = F_lexer_reset_wrapper;
    interface->destroy = F_lexer_destroy_wrapper;
    interface->internal_state = state;
    
    return interface;
}

/**
 * @brief 创建词法分析器接口实例（公共接口）
 */
Stru_LexerInterface_t* F_create_lexer_interface(void)
{
    return F_create_lexer_interface_impl();
}
