/**
 * @file CN_lexer_scanner.c
 * @brief CN_Language 词法分析器字符扫描模块实现
 * 
 * 实现字符扫描、位置管理和空白字符处理功能。
 * 负责源代码的字符级操作和位置跟踪。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include "CN_lexer_scanner.h"
#include "../utils/CN_lexer_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/**
 * @brief 创建扫描器状态
 */
Stru_LexerScannerState_t* F_create_scanner_state(void)
{
    Stru_LexerScannerState_t* state = (Stru_LexerScannerState_t*)malloc(sizeof(Stru_LexerScannerState_t));
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
 * @brief 销毁扫描器状态
 */
void F_destroy_scanner_state(Stru_LexerScannerState_t* state)
{
    if (state != NULL)
    {
        free(state);
    }
}

/**
 * @brief 初始化扫描器状态
 */
bool F_initialize_scanner_state(Stru_LexerScannerState_t* state, 
                               const char* source, size_t length, 
                               const char* source_name)
{
    if (state == NULL)
    {
        return false;
    }
    
    if (source == NULL)
    {
        F_set_scanner_error(state, "源代码不能为NULL");
        return false;
    }
    
    state->source = source;
    state->source_length = length;
    state->source_name = source_name ? source_name : "unknown.cn";
    state->current_pos = 0;
    state->current_line = 1;
    state->current_column = 1;
    state->has_error = false;
    state->error_message[0] = '\0';
    
    return true;
}

/**
 * @brief 重置扫描器状态
 */
void F_reset_scanner_state(Stru_LexerScannerState_t* state)
{
    if (state == NULL)
    {
        return;
    }
    
    state->current_pos = 0;
    state->current_line = 1;
    state->current_column = 1;
    state->has_error = false;
    state->error_message[0] = '\0';
}

/**
 * @brief 查看下一个字符（不移动位置）
 */
char F_peek_char(const Stru_LexerScannerState_t* state)
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
char F_next_char(Stru_LexerScannerState_t* state)
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
void F_skip_whitespace(Stru_LexerScannerState_t* state)
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
void F_skip_comment(Stru_LexerScannerState_t* state)
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

/**
 * @brief 检查是否还有更多字符
 */
bool F_has_more_chars(const Stru_LexerScannerState_t* state)
{
    if (state == NULL || state->source == NULL)
    {
        return false;
    }
    
    return state->current_pos < state->source_length;
}

/**
 * @brief 获取当前位置
 */
void F_get_scanner_position(const Stru_LexerScannerState_t* state, 
                           size_t* line, size_t* column)
{
    if (state == NULL || line == NULL || column == NULL)
    {
        return;
    }
    
    *line = state->current_line;
    *column = state->current_column;
}

/**
 * @brief 获取源文件名
 */
const char* F_get_scanner_source_name(const Stru_LexerScannerState_t* state)
{
    if (state == NULL)
    {
        return "unknown.cn";
    }
    
    return state->source_name ? state->source_name : "unknown.cn";
}

/**
 * @brief 设置错误信息
 */
void F_set_scanner_error(Stru_LexerScannerState_t* state, const char* format, ...)
{
    if (state == NULL)
    {
        return;
    }
    
    state->has_error = true;
    
    if (format != NULL)
    {
        va_list args;
        va_start(args, format);
        vsnprintf(state->error_message, sizeof(state->error_message), format, args);
        va_end(args);
    }
    else
    {
        state->error_message[0] = '\0';
    }
}

/**
 * @brief 清除错误信息
 */
void F_clear_scanner_error(Stru_LexerScannerState_t* state)
{
    if (state != NULL)
    {
        state->has_error = false;
        state->error_message[0] = '\0';
    }
}

/**
 * @brief 检查是否有错误
 */
bool F_scanner_has_errors(const Stru_LexerScannerState_t* state)
{
    if (state == NULL)
    {
        return false;
    }
    
    return state->has_error;
}

/**
 * @brief 获取最后一个错误信息
 */
const char* F_get_scanner_last_error(const Stru_LexerScannerState_t* state)
{
    if (state == NULL)
    {
        return "";
    }
    
    return state->error_message;
}
