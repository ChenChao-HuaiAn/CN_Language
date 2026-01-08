/**
 * @file CN_lexer_errors.c
 * @brief CN_Language 词法分析器错误处理模块实现
 * 
 * 实现词法分析器的错误处理功能，包括错误码定义、错误信息管理和错误恢复。
 * 遵循项目架构规范，实现模块化的错误处理。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include "CN_lexer_errors.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// ============================================
// 内部常量定义
// ============================================

/**
 * @brief 错误码描述表
 */
static const char* g_error_code_descriptions[] = {
    "无错误",
    "未知字符",
    "未终止的字符串",
    "无效的数字格式",
    "无效的转义序列",
    "无效的UTF-8编码",
    "缓冲区溢出",
    "内存分配失败",
    "无效的状态",
    "无效的参数",
    "意外的文件结束",
    "内部错误"
};

/**
 * @brief 错误码数量
 */
static const size_t g_error_code_count = sizeof(g_error_code_descriptions) / sizeof(g_error_code_descriptions[0]);

// ============================================
// 内部辅助函数
// ============================================

/**
 * @brief 复制错误信息
 */
static void F_copy_error_info(Stru_LexerErrorInfo_t* dest, const Stru_LexerErrorInfo_t* src)
{
    if (dest == NULL || src == NULL)
    {
        return;
    }
    
    dest->code = src->code;
    dest->line = src->line;
    dest->column = src->column;
    dest->source_name = src->source_name;
    
    if (src->message[0] != '\0')
    {
        strncpy(dest->message, src->message, sizeof(dest->message) - 1);
        dest->message[sizeof(dest->message) - 1] = '\0';
    }
    else
    {
        dest->message[0] = '\0';
    }
}

/**
 * @brief 格式化错误信息
 */
static void F_format_error_message(Stru_LexerErrorInfo_t* error_info, const char* format, va_list args)
{
    if (error_info == NULL || format == NULL)
    {
        return;
    }
    
    vsnprintf(error_info->message, sizeof(error_info->message), format, args);
    error_info->message[sizeof(error_info->message) - 1] = '\0';
}

// ============================================
// 错误上下文管理函数实现
// ============================================

/**
 * @brief 创建错误上下文
 */
Stru_LexerErrorContext_t* F_create_lexer_error_context(void)
{
    Stru_LexerErrorContext_t* context = (Stru_LexerErrorContext_t*)malloc(sizeof(Stru_LexerErrorContext_t));
    if (context == NULL)
    {
        return NULL;
    }
    
    // 初始化上下文
    context->current_error.code = Eum_LEXER_ERROR_NONE;
    context->current_error.line = 0;
    context->current_error.column = 0;
    context->current_error.source_name = NULL;
    context->current_error.message[0] = '\0';
    
    context->previous_errors = NULL;
    context->error_count = 0;
    context->has_fatal_error = false;
    
    return context;
}

/**
 * @brief 销毁错误上下文
 */
void F_destroy_lexer_error_context(Stru_LexerErrorContext_t* context)
{
    if (context == NULL)
    {
        return;
    }
    
    // 释放之前的错误链
    if (context->previous_errors != NULL)
    {
        free(context->previous_errors);
        context->previous_errors = NULL;
    }
    
    free(context);
}

/**
 * @brief 重置错误上下文
 */
void F_reset_lexer_error_context(Stru_LexerErrorContext_t* context)
{
    if (context == NULL)
    {
        return;
    }
    
    // 重置当前错误
    context->current_error.code = Eum_LEXER_ERROR_NONE;
    context->current_error.line = 0;
    context->current_error.column = 0;
    context->current_error.source_name = NULL;
    context->current_error.message[0] = '\0';
    
    // 释放之前的错误链
    if (context->previous_errors != NULL)
    {
        free(context->previous_errors);
        context->previous_errors = NULL;
    }
    
    context->error_count = 0;
    context->has_fatal_error = false;
}

// ============================================
// 错误报告函数实现
// ============================================

/**
 * @brief 报告词法分析错误
 */
bool F_report_lexer_error(Stru_LexerErrorContext_t* context,
                         Eum_LexerErrorCode_t code,
                         size_t line, size_t column,
                         const char* source_name,
                         const char* format, ...)
{
    if (context == NULL || code == Eum_LEXER_ERROR_NONE)
    {
        return false;
    }
    
    // 如果已经有当前错误，将其移动到历史记录中
    if (context->current_error.code != Eum_LEXER_ERROR_NONE)
    {
        // 分配或重新分配历史错误数组
        Stru_LexerErrorInfo_t* new_errors = (Stru_LexerErrorInfo_t*)realloc(
            context->previous_errors,
            (context->error_count + 1) * sizeof(Stru_LexerErrorInfo_t)
        );
        
        if (new_errors == NULL)
        {
            return false;
        }
        
        context->previous_errors = new_errors;
        
        // 复制当前错误到历史记录
        F_copy_error_info(&context->previous_errors[context->error_count], &context->current_error);
        context->error_count++;
    }
    
    // 设置新的当前错误
    context->current_error.code = code;
    context->current_error.line = line;
    context->current_error.column = column;
    context->current_error.source_name = source_name;
    
    // 格式化错误信息
    if (format != NULL)
    {
        va_list args;
        va_start(args, format);
        F_format_error_message(&context->current_error, format, args);
        va_end(args);
    }
    else
    {
        // 使用默认错误描述
        const char* description = F_get_lexer_error_code_description(code);
        if (description != NULL)
        {
            strncpy(context->current_error.message, description, sizeof(context->current_error.message) - 1);
            context->current_error.message[sizeof(context->current_error.message) - 1] = '\0';
        }
        else
        {
            context->current_error.message[0] = '\0';
        }
    }
    
    return true;
}

/**
 * @brief 报告致命错误
 */
bool F_report_fatal_lexer_error(Stru_LexerErrorContext_t* context,
                               Eum_LexerErrorCode_t code,
                               size_t line, size_t column,
                               const char* source_name,
                               const char* format, ...)
{
    if (context == NULL || code == Eum_LEXER_ERROR_NONE)
    {
        return false;
    }
    
    // 报告错误
    bool success = F_report_lexer_error(context, code, line, column, source_name, format);
    
    // 标记为致命错误
    if (success)
    {
        context->has_fatal_error = true;
    }
    
    return success;
}

// ============================================
// 错误查询函数实现
// ============================================

/**
 * @brief 检查是否有错误
 */
bool F_has_lexer_errors(const Stru_LexerErrorContext_t* context)
{
    if (context == NULL)
    {
        return false;
    }
    
    return context->current_error.code != Eum_LEXER_ERROR_NONE || context->error_count > 0;
}

/**
 * @brief 检查是否有致命错误
 */
bool F_has_fatal_lexer_error(const Stru_LexerErrorContext_t* context)
{
    if (context == NULL)
    {
        return false;
    }
    
    return context->has_fatal_error;
}

/**
 * @brief 获取错误数量
 */
size_t F_get_lexer_error_count(const Stru_LexerErrorContext_t* context)
{
    if (context == NULL)
    {
        return 0;
    }
    
    size_t count = context->error_count;
    if (context->current_error.code != Eum_LEXER_ERROR_NONE)
    {
        count++;
    }
    
    return count;
}

/**
 * @brief 获取最后一个错误信息
 */
const Stru_LexerErrorInfo_t* F_get_last_lexer_error(const Stru_LexerErrorContext_t* context)
{
    if (context == NULL)
    {
        return NULL;
    }
    
    if (context->current_error.code != Eum_LEXER_ERROR_NONE)
    {
        return &context->current_error;
    }
    else if (context->error_count > 0)
    {
        return &context->previous_errors[context->error_count - 1];
    }
    
    return NULL;
}

/**
 * @brief 获取错误信息字符串
 */
const char* F_get_lexer_error_message(const Stru_LexerErrorContext_t* context, size_t index)
{
    if (context == NULL)
    {
        return NULL;
    }
    
    size_t total_errors = F_get_lexer_error_count(context);
    if (index >= total_errors)
    {
        return NULL;
    }
    
    // 计算实际索引
    if (context->current_error.code != Eum_LEXER_ERROR_NONE)
    {
        if (index == 0)
        {
            return context->current_error.message;
        }
        else
        {
            index--;
        }
    }
    
    if (index < context->error_count)
    {
        return context->previous_errors[index].message;
    }
    
    return NULL;
}

/**
 * @brief 获取错误码描述
 */
const char* F_get_lexer_error_code_description(Eum_LexerErrorCode_t code)
{
    if (code >= 0 && code < g_error_code_count)
    {
        return g_error_code_descriptions[code];
    }
    
    return "未知错误码";
}

// ============================================
// 错误恢复函数实现
// ============================================

/**
 * @brief 尝试从错误中恢复
 */
bool F_try_recover_from_lexer_error(Stru_LexerErrorContext_t* context,
                                   char current_char, char next_char)
{
    if (context == NULL)
    {
        return false;
    }
    
    // 如果有致命错误，无法恢复
    if (context->has_fatal_error)
    {
        return false;
    }
    
    // 根据错误类型尝试恢复
    switch (context->current_error.code)
    {
        case Eum_LEXER_ERROR_UNKNOWN_CHAR:
            // 对于未知字符，如果下一个字符是有效的开始字符，可以恢复
            if (next_char == '\0')
            {
                return false;  // 文件结束，无法恢复
            }
            
            // 检查下一个字符是否可能是有效令牌的开始
            if ((next_char >= 'a' && next_char <= 'z') ||
                (next_char >= 'A' && next_char <= 'Z') ||
                (next_char >= '0' && next_char <= '9') ||
                next_char == '"' ||
                strchr("+-*/%=!<>", next_char) != NULL ||
                strchr(",;(){}[]", next_char) != NULL)
            {
                return true;  // 可以恢复
            }
            break;
            
        case Eum_LEXER_ERROR_UNTERMINATED_STRING:
            // 对于未终止的字符串，通常无法恢复，除非我们到达文件末尾
            return false;
            
        case Eum_LEXER_ERROR_INVALID_NUMBER:
            // 对于无效数字，跳过当前字符并检查下一个
            if (next_char == '\0')
            {
                return false;
            }
            
            // 如果下一个字符不是数字或点，可以恢复
            if (!(next_char >= '0' && next_char <= '9') && next_char != '.')
            {
                return true;
            }
            break;
            
        case Eum_LEXER_ERROR_INVALID_ESCAPE:
            // 对于无效转义，跳过转义序列并继续
            return true;
            
        case Eum_LEXER_ERROR_INVALID_UTF8:
            // 对于无效UTF-8，跳过整个字符并继续
            return true;
            
        default:
            // 对于其他错误，默认无法恢复
            break;
    }
    
    return false;
}

/**
 * @brief 跳过错误字符
 */
size_t F_skip_error_chars(Stru_LexerErrorContext_t* context, void* scanner_state)
{
    // 这是一个简化实现，实际实现应该根据扫描器状态跳过字符
    (void)context;
    (void)scanner_state;
    
    // 默认跳过1个字符
    return 1;
}
