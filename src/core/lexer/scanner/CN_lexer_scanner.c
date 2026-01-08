/**
 * @file CN_lexer_scanner.c
 * @brief CN_Language 词法分析器字符扫描模块实现
 * 
 * 实现字符扫描、位置管理和空白字符处理功能。
 * 负责源代码的字符级操作和位置跟踪。
 * 使用新的错误处理系统。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
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
    
    // 创建错误上下文
    state->error_ctx = F_create_lexer_error_context();
    if (state->error_ctx == NULL)
    {
        free(state);
        return NULL;
    }
    
    // 初始化状态
    state->source = NULL;
    state->source_length = 0;
    state->source_name = NULL;
    state->current_pos = 0;
    state->current_line = 1;
    state->current_column = 1;
    
    return state;
}

/**
 * @brief 销毁扫描器状态
 */
void F_destroy_scanner_state(Stru_LexerScannerState_t* state)
{
    if (state == NULL)
    {
        return;
    }
    
    // 销毁错误上下文
    if (state->error_ctx != NULL)
    {
        F_destroy_lexer_error_context(state->error_ctx);
        state->error_ctx = NULL;
    }
    
    free(state);
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
        F_set_scanner_error(state, Eum_LEXER_ERROR_INVALID_PARAMETER, "源代码不能为NULL");
        return false;
    }
    
    state->source = source;
    state->source_length = length;
    state->source_name = source_name ? source_name : "unknown.cn";
    state->current_pos = 0;
    state->current_line = 1;
    state->current_column = 1;
    
    // 重置错误上下文
    if (state->error_ctx != NULL)
    {
        F_reset_lexer_error_context(state->error_ctx);
    }
    
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
    
    // 重置错误上下文
    if (state->error_ctx != NULL)
    {
        F_reset_lexer_error_context(state->error_ctx);
    }
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
 * @brief 设置扫描器错误
 */
void F_set_scanner_error(Stru_LexerScannerState_t* state, 
                        Eum_LexerErrorCode_t code,
                        const char* format, ...)
{
    if (state == NULL || state->error_ctx == NULL)
    {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    // 报告错误
    F_report_lexer_error(state->error_ctx, code, 
                        state->current_line, state->current_column,
                        state->source_name, format, args);
    
    va_end(args);
}

/**
 * @brief 设置扫描器致命错误
 */
void F_set_scanner_fatal_error(Stru_LexerScannerState_t* state,
                              Eum_LexerErrorCode_t code,
                              const char* format, ...)
{
    if (state == NULL || state->error_ctx == NULL)
    {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    // 报告致命错误
    F_report_fatal_lexer_error(state->error_ctx, code,
                              state->current_line, state->current_column,
                              state->source_name, format, args);
    
    va_end(args);
}

/**
 * @brief 清除扫描器错误
 */
void F_clear_scanner_error(Stru_LexerScannerState_t* state)
{
    if (state == NULL || state->error_ctx == NULL)
    {
        return;
    }
    
    F_reset_lexer_error_context(state->error_ctx);
}

/**
 * @brief 检查扫描器是否有错误
 */
bool F_scanner_has_errors(const Stru_LexerScannerState_t* state)
{
    if (state == NULL || state->error_ctx == NULL)
    {
        return false;
    }
    
    return F_has_lexer_errors(state->error_ctx);
}

/**
 * @brief 检查扫描器是否有致命错误
 */
bool F_scanner_has_fatal_error(const Stru_LexerScannerState_t* state)
{
    if (state == NULL || state->error_ctx == NULL)
    {
        return false;
    }
    
    return F_has_fatal_lexer_error(state->error_ctx);
}

/**
 * @brief 获取扫描器最后一个错误信息
 */
const char* F_get_scanner_last_error(const Stru_LexerScannerState_t* state)
{
    if (state == NULL || state->error_ctx == NULL)
    {
        return "";
    }
    
    const Stru_LexerErrorInfo_t* error = F_get_last_lexer_error(state->error_ctx);
    if (error == NULL)
    {
        return "";
    }
    
    return error->message;
}

/**
 * @brief 获取扫描器错误上下文
 */
Stru_LexerErrorContext_t* F_get_scanner_error_context(const Stru_LexerScannerState_t* state)
{
    if (state == NULL)
    {
        return NULL;
    }
    
    return state->error_ctx;
}
