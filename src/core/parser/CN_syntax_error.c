/**
 * @file CN_syntax_error.c
 * @brief CN_Language 语法错误处理实现
 * 
 * 语法错误处理接口的具体实现，提供错误报告、查询和管理功能。
 * 遵循项目架构规范，支持语法分析器的错误处理需求。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_syntax_error.h"
#include "../../infrastructure/memory/CN_memory_interface.h"
#include "../../infrastructure/utils/CN_utils_interface.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ============================================
// 内部辅助函数
// ============================================

// ============================================
// 内部数据结构
// ============================================

/**
 * @brief 错误处理接口内部状态结构体
 * 
 * 存储错误处理接口的内部状态，包括错误列表和配置信息。
 */
typedef struct Stru_SyntaxErrorHandlerState_t {
    Stru_DynamicArray_t* errors;      ///< 错误列表
    size_t max_errors;                ///< 最大错误数量
    char* source_name;                ///< 源文件名
} Stru_SyntaxErrorHandlerState_t;

// ============================================
// 内部辅助函数
// ============================================

/**
 * @brief 获取错误处理接口的内部状态
 */
static Stru_SyntaxErrorHandlerState_t* F_get_handler_state(Stru_SyntaxErrorHandler_t* handler)
{
    if (handler == NULL) {
        return NULL;
    }
    return (Stru_SyntaxErrorHandlerState_t*)handler->internal_state;
}

/**
 * @brief 创建错误处理接口的内部状态
 */
static Stru_SyntaxErrorHandlerState_t* F_create_handler_state(void)
{
    Stru_SyntaxErrorHandlerState_t* state = cn_malloc(sizeof(Stru_SyntaxErrorHandlerState_t));
    if (state == NULL) {
        return NULL;
    }
    
    state->errors = F_create_dynamic_array(sizeof(Stru_SyntaxError_t*));
    if (state->errors == NULL) {
        cn_free(state);
        return NULL;
    }
    
    state->max_errors = 100;  // 默认最大错误数量
    state->source_name = NULL;
    
    return state;
}

/**
 * @brief 销毁错误处理接口的内部状态
 */
static void F_destroy_handler_state(Stru_SyntaxErrorHandlerState_t* state)
{
    if (state == NULL) {
        return;
    }
    
    // 销毁所有错误对象
    if (state->errors != NULL) {
        size_t error_count = F_dynamic_array_length(state->errors);
        for (size_t i = 0; i < error_count; i++) {
            Stru_SyntaxError_t** error_ptr = F_dynamic_array_get(state->errors, i);
            if (error_ptr != NULL && *error_ptr != NULL) {
                F_destroy_syntax_error(*error_ptr);
            }
        }
        F_destroy_dynamic_array(state->errors);
    }
    
    // 释放源文件名
    if (state->source_name != NULL) {
        cn_free(state->source_name);
    }
    
    cn_free(state);
}

// ============================================
// 错误对象操作函数实现
// ============================================

/**
 * @brief 创建语法错误
 */
Stru_SyntaxError_t* F_create_syntax_error(Eum_SyntaxErrorType type,
                                         Eum_SyntaxErrorSeverity severity,
                                         size_t line, size_t column,
                                         const char* message,
                                         Stru_Token_t* token)
{
    Stru_SyntaxError_t* error = cn_malloc(sizeof(Stru_SyntaxError_t));
    if (error == NULL) {
        return NULL;
    }
    
    error->type = type;
    error->severity = severity;
    error->line = line;
    error->column = column;
    error->token = token;
    error->source_name = NULL;
    
    // 复制错误信息
    if (message != NULL) {
        error->message = cn_strdup(message);
        if (error->message == NULL) {
            cn_free(error);
            return NULL;
        }
    } else {
        error->message = NULL;
    }
    
    // 初始化数据联合体
    error->data.expected = NULL;
    
    return error;
}

/**
 * @brief 销毁语法错误
 */
void F_destroy_syntax_error(Stru_SyntaxError_t* error)
{
    if (error == NULL) {
        return;
    }
    
    if (error->message != NULL) {
        cn_free(error->message);
    }
    
    if (error->source_name != NULL) {
        cn_free(error->source_name);
    }
    
    if (error->data.expected != NULL) {
        cn_free(error->data.expected);
    }
    
    cn_free(error);
}

/**
 * @brief 复制语法错误
 */
Stru_SyntaxError_t* F_copy_syntax_error(const Stru_SyntaxError_t* error)
{
    if (error == NULL) {
        return NULL;
    }
    
    Stru_SyntaxError_t* copy = F_create_syntax_error(
        error->type,
        error->severity,
        error->line,
        error->column,
        error->message,
        error->token
    );
    
    if (copy == NULL) {
        return NULL;
    }
    
    // 复制源文件名
    if (error->source_name != NULL) {
        copy->source_name = cn_strdup(error->source_name);
        if (copy->source_name == NULL) {
            F_destroy_syntax_error(copy);
            return NULL;
        }
    }
    
    // 复制数据
    if (error->data.expected != NULL) {
        copy->data.expected = cn_strdup(error->data.expected);
        if (copy->data.expected == NULL) {
            F_destroy_syntax_error(copy);
            return NULL;
        }
    }
    
    return copy;
}

/**
 * @brief 设置错误期望内容
 */
void F_error_set_expected(Stru_SyntaxError_t* error, const char* expected)
{
    if (error == NULL) {
        return;
    }
    
    // 释放旧的期望内容
    if (error->data.expected != NULL) {
        cn_free(error->data.expected);
    }
    
    // 设置新的期望内容
    if (expected != NULL) {
        error->data.expected = cn_strdup(expected);
    } else {
        error->data.expected = NULL;
    }
}

/**
 * @brief 设置错误实际找到的内容
 */
void F_error_set_found(Stru_SyntaxError_t* error, const char* found)
{
    if (error == NULL) {
        return;
    }
    
    // 释放旧的找到内容
    if (error->data.found != NULL) {
        cn_free(error->data.found);
    }
    
    // 设置新的找到内容
    if (found != NULL) {
        error->data.found = cn_strdup(found);
    } else {
        error->data.found = NULL;
    }
}

/**
 * @brief 设置错误数量信息
 */
void F_error_set_count(Stru_SyntaxError_t* error, size_t count)
{
    if (error == NULL) {
        return;
    }
    
    error->data.count = count;
}

/**
 * @brief 设置错误源文件名
 */
void F_error_set_source_name(Stru_SyntaxError_t* error, const char* source_name)
{
    if (error == NULL) {
        return;
    }
    
    // 释放旧的源文件名
    if (error->source_name != NULL) {
        cn_free(error->source_name);
    }
    
    // 设置新的源文件名
    if (source_name != NULL) {
        error->source_name = cn_strdup(source_name);
    } else {
        error->source_name = NULL;
    }
}

// ============================================
// 错误类型和严重级别转换函数实现
// ============================================

/**
 * @brief 错误类型转字符串
 */
const char* F_syntax_error_type_to_string(Eum_SyntaxErrorType type)
{
    switch (type) {
        case Eum_ERROR_UNKNOWN_TOKEN:
            return "未知令牌";
        case Eum_ERROR_UNTERMINATED_STRING:
            return "未终止的字符串";
        case Eum_ERROR_UNTERMINATED_COMMENT:
            return "未终止的注释";
        case Eum_ERROR_INVALID_NUMBER:
            return "无效的数字字面量";
        case Eum_ERROR_INVALID_ESCAPE:
            return "无效的转义序列";
        case Eum_ERROR_UNEXPECTED_TOKEN:
            return "意外的令牌";
        case Eum_ERROR_MISSING_TOKEN:
            return "缺失的令牌";
        case Eum_ERROR_MISMATCHED_TOKEN:
            return "不匹配的令牌";
        case Eum_ERROR_INVALID_EXPRESSION:
            return "无效的表达式";
        case Eum_ERROR_INVALID_STATEMENT:
            return "无效的语句";
        case Eum_ERROR_INVALID_DECLARATION:
            return "无效的声明";
        case Eum_ERROR_UNEXPECTED_EOF:
            return "意外的文件结束";
        case Eum_ERROR_UNCLOSED_BLOCK:
            return "未关闭的代码块";
        case Eum_ERROR_UNCLOSED_PAREN:
            return "未关闭的括号";
        case Eum_ERROR_UNCLOSED_BRACKET:
            return "未关闭的方括号";
        case Eum_ERROR_UNCLOSED_BRACE:
            return "未关闭的大括号";
        case Eum_ERROR_DUPLICATE_DECLARATION:
            return "重复声明";
        case Eum_ERROR_UNDECLARED_IDENTIFIER:
            return "未声明的标识符";
        case Eum_ERROR_TYPE_MISMATCH:
            return "类型不匹配";
        case Eum_ERROR_ARGUMENT_COUNT:
            return "参数数量错误";
        case Eum_ERROR_INTERNAL:
            return "内部错误";
        case Eum_ERROR_MEMORY:
            return "内存错误";
        case Eum_ERROR_IO:
            return "输入输出错误";
        default:
            return "未知错误类型";
    }
}

/**
 * @brief 错误严重级别转字符串
 */
const char* F_syntax_error_severity_to_string(Eum_SyntaxErrorSeverity severity)
{
    switch (severity) {
        case Eum_SEVERITY_INFO:
            return "信息";
        case Eum_SEVERITY_WARNING:
            return "警告";
        case Eum_SEVERITY_ERROR:
            return "错误";
        case Eum_SEVERITY_FATAL:
            return "致命错误";
        default:
            return "未知严重级别";
    }
}

// ============================================
// 错误处理接口函数实现
// ============================================

/**
 * @brief 报告语法错误（接口函数）
 */
static Stru_SyntaxError_t* F_handler_report_error(Stru_SyntaxErrorHandler_t* handler,
                                                 Eum_SyntaxErrorType type,
                                                 Eum_SyntaxErrorSeverity severity,
                                                 size_t line, size_t column,
                                                 const char* message,
                                                 Stru_Token_t* token)
{
    Stru_SyntaxErrorHandlerState_t* state = F_get_handler_state(handler);
    if (state == NULL) {
        return NULL;
    }
    
    // 检查是否已达到最大错误数量
    if (F_dynamic_array_length(state->errors) >= state->max_errors) {
        return NULL;
    }
    
    // 创建错误对象
    Stru_SyntaxError_t* error = F_create_syntax_error(type, severity, line, column, message, token);
    if (error == NULL) {
        return NULL;
    }
    
    // 设置源文件名
    if (state->source_name != NULL) {
        error->source_name = cn_strdup(state->source_name);
        if (error->source_name == NULL) {
            F_destroy_syntax_error(error);
            return NULL;
        }
    }
    
    // 添加到错误列表
    if (!F_dynamic_array_push(state->errors, &error)) {
        F_destroy_syntax_error(error);
        return NULL;
    }
    
    return error;
}

/**
 * @brief 报告意外令牌错误（接口函数）
 */
static Stru_SyntaxError_t* F_handler_report_unexpected_token(Stru_SyntaxErrorHandler_t* handler,
                                                            Stru_Token_t* token,
                                                            const char* expected)
{
    if (token == NULL) {
        return NULL;
    }
    
    char message[256];
    if (expected != NULL) {
        snprintf(message, sizeof(message), "意外的令牌 '%s'，期望: %s", 
                 token->lexeme ? token->lexeme : "(null)", expected);
    } else {
        snprintf(message, sizeof(message), "意外的令牌 '%s'", 
                 token->lexeme ? token->lexeme : "(null)");
    }
    
    Stru_SyntaxError_t* error = F_handler_report_error(handler,
                                                      Eum_ERROR_UNEXPECTED_TOKEN,
                                                      Eum_SEVERITY_ERROR,
                                                      token->line,
                                                      token->column,
                                                      message,
                                                      token);
    
    if (error != NULL && expected != NULL) {
        F_error_set_expected(error, expected);
    }
    
    return error;
}

/**
 * @brief 报告缺失令牌错误（接口函数）
 */
static Stru_SyntaxError_t* F_handler_report_missing_token(Stru_SyntaxErrorHandler_t* handler,
                                                         size_t line, size_t column,
                                                         const char* expected)
{
    char message[256];
    if (expected != NULL) {
        snprintf(message, sizeof(message), "缺失的令牌，期望: %s", expected);
    } else {
        snprintf(message, sizeof(message), "缺失的令牌");
    }
    
    Stru_SyntaxError_t* error = F_handler_report_error(handler,
                                                      Eum_ERROR_MISSING_TOKEN,
                                                      Eum_SEVERITY_ERROR,
                                                      line,
                                                      column,
                                                      message,
                                                      NULL);
    
    if (error != NULL && expected != NULL) {
        F_error_set_expected(error, expected);
    }
    
    return error;
}

/**
 * @brief 检查是否有错误（接口函数）
 */
static bool F_handler_has_errors(Stru_SyntaxErrorHandler_t* handler)
{
    Stru_SyntaxErrorHandlerState_t* state = F_get_handler_state(handler);
    if (state == NULL || state->errors == NULL) {
        return false;
    }
    
    return F_dynamic_array_length(state->errors) > 0;
}

/**
 * @brief 检查是否有特定严重级别的错误（接口函数）
 */
static bool F_handler_has_errors_of_severity(Stru_SyntaxErrorHandler_t* handler,
                                            Eum_SyntaxErrorSeverity severity)
{
    Stru_SyntaxErrorHandlerState_t* state = F_get_handler_state(handler);
    if (state == NULL || state->errors == NULL) {
        return false;
    }
    
    size_t error_count = F_dynamic_array_length(state->errors);
    for (size_t i = 0; i < error_count; i++) {
        Stru_SyntaxError_t** error_ptr = F_dynamic_array_get(state->errors, i);
        if (error_ptr != NULL && *error_ptr != NULL && (*error_ptr)->severity == severity) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 获取错误数量（接口函数）
 */
static size_t F_handler_get_error_count(Stru_SyntaxErrorHandler_t* handler)
{
    Stru_SyntaxErrorHandlerState_t* state = F_get_handler_state(handler);
    if (state == NULL || state->errors == NULL) {
        return 0;
    }
    
    return F_dynamic_array_length(state->errors);
}

/**
 * @brief 获取特定严重级别的错误数量（接口函数）
 */
static size_t F_handler_get_error_count_by_severity(Stru_SyntaxErrorHandler_t* handler,
                                                   Eum_SyntaxErrorSeverity severity)
{
    Stru_SyntaxErrorHandlerState_t* state = F_get_handler_state(handler);
    if (state == NULL || state->errors == NULL) {
        return 0;
    }
    
    size_t count = 0;
    size_t error_count = F_dynamic_array_length(state->errors);
    for (size_t i = 0; i < error_count; i++) {
        Stru_SyntaxError_t** error_ptr = F_dynamic_array_get(state->errors, i);
        if (error_ptr != NULL && *error_ptr != NULL && (*error_ptr)->severity == severity) {
            count++;
        }
    }
    
    return count;
}

/**
 * @brief 获取错误列表（接口函数）
 */
static Stru_DynamicArray_t* F_handler_get_errors(Stru_SyntaxErrorHandler_t* handler)
{
    Stru_SyntaxErrorHandlerState_t* state = F_get_handler_state(handler);
    if (state == NULL) {
        return NULL;
    }
    
    return state->errors;
}

/**
 * @brief 获取特定严重级别的错误列表（接口函数）
 */
static Stru_DynamicArray_t* F_handler_get_errors_by_severity(Stru_SyntaxErrorHandler_t* handler,
                                                            Eum_SyntaxErrorSeverity severity)
{
    Stru_SyntaxErrorHandlerState_t* state = F_get_handler_state(handler);
    if (state == NULL || state->errors == NULL) {
        return NULL;
    }
    
    // 创建新的动态数组来存储过滤后的错误
    Stru_DynamicArray_t* filtered_errors = F_create_dynamic_array(sizeof(Stru_SyntaxError_t*));
    if (filtered_errors == NULL) {
        return NULL;
    }
    
    size_t error_count = F_dynamic_array_length(state->errors);
    for (size_t i = 0; i < error_count; i++) {
        Stru_SyntaxError_t** error_ptr = F_dynamic_array_get(state->errors, i);
        if (error_ptr != NULL && *error_ptr != NULL && (*error_ptr)->severity == severity) {
            if (!F_dynamic_array_push(filtered_errors, error_ptr)) {
                F_destroy_dynamic_array(filtered_errors);
                return NULL;
            }
        }
    }
    
    return filtered_errors;
}

/**
 * @brief 获取最后一个错误（接口函数）
 */
static Stru_SyntaxError_t* F_handler_get_last_error(Stru_SyntaxErrorHandler_t* handler)
{
    Stru_SyntaxErrorHandlerState_t* state = F_get_handler_state(handler);
    if (state == NULL || state->errors == NULL) {
        return NULL;
    }
    
    size_t error_count = F_dynamic_array_length(state->errors);
    if (error_count == 0) {
        return NULL;
    }
    
    Stru_SyntaxError_t** error_ptr = F_dynamic_array_get(state->errors, error_count - 1);
    if (error_ptr == NULL) {
        return NULL;
    }
    
    return *error_ptr;
}

/**
 * @brief 清除所有错误（接口函数）
 */
static void F_handler_clear_errors(Stru_SyntaxErrorHandler_t* handler)
{
    Stru_SyntaxErrorHandlerState_t* state = F_get_handler_state(handler);
    if (state == NULL || state->errors == NULL) {
        return;
    }
    
    // 销毁所有错误对象
    size_t error_count = F_dynamic_array_length(state->errors);
    for (size_t i = 0; i < error_count; i++) {
        Stru_SyntaxError_t** error_ptr = F_dynamic_array_get(state->errors, i);
        if (error_ptr != NULL && *error_ptr != NULL) {
            F_destroy_syntax_error(*error_ptr);
        }
    }
    
    // 清空数组
    F_dynamic_array_clear(state->errors);
}

/**
 * @brief 清除特定严重级别的错误（接口函数）
 */
static void F_handler_clear_errors_by_severity(Stru_SyntaxErrorHandler_t* handler,
                                              Eum_SyntaxErrorSeverity severity)
{
    Stru_SyntaxErrorHandlerState_t* state = F_get_handler_state(handler);
    if (state == NULL || state->errors == NULL) {
        return;
    }
    
    // 从后向前遍历，避免索引问题
    size_t i = F_dynamic_array_length(state->errors);
    while (i > 0) {
        i--;
        Stru_SyntaxError_t** error_ptr = F_dynamic_array_get(state->errors, i);
        if (error_ptr != NULL && *error_ptr != NULL && (*error_ptr)->severity == severity) {
            F_destroy_syntax_error(*error_ptr);
            F_dynamic_array_remove(state->errors, i);
        }
    }
}

/**
 * @brief 设置最大错误数量（接口函数）
 */
static void F_handler_set_max_errors(Stru_SyntaxErrorHandler_t* handler, size_t max_errors)
{
    Stru_SyntaxErrorHandlerState_t* state = F_get_handler_state(handler);
    if (state == NULL) {
        return;
    }
    
    state->max_errors = max_errors;
}

/**
 * @brief 获取最大错误数量（接口函数）
 */
static size_t F_handler_get_max_errors(Stru_SyntaxErrorHandler_t* handler)
{
    Stru_SyntaxErrorHandlerState_t* state = F_get_handler_state(handler);
    if (state == NULL) {
        return 0;
    }
    
    return state->max_errors;
}

/**
 * @brief 格式化错误信息（接口函数）
 */
static int F_handler_format_error(Stru_SyntaxErrorHandler_t* handler,
                                 const Stru_SyntaxError_t* error,
                                 char* buffer, size_t buffer_size)
{
    if (error == NULL || buffer == NULL || buffer_size == 0) {
        return 0;
    }
    
    const char* severity_str = F_syntax_error_severity_to_string(error->severity);
    const char* type_str = F_syntax_error_type_to_string(error->type);
    
    int written = 0;
    
    if (error->source_name != NULL) {
        written = snprintf(buffer, buffer_size,
                          "%s:%zu:%zu: %s: %s: %s",
                          error->source_name,
                          error->line,
                          error->column,
                          severity_str,
                          type_str,
                          error->message ? error->message : "");
    } else {
        written = snprintf(buffer, buffer_size,
                          "行 %zu, 列 %zu: %s: %s: %s",
                          error->line,
                          error->column,
                          severity_str,
                          type_str,
                          error->message ? error->message : "");
    }
    
    if (written < 0 || (size_t)written >= buffer_size) {
        return buffer_size - 1;
    }
    
    return written;
}

/**
 * @brief 打印所有错误（接口函数）
 */
static void F_handler_print_all_errors(Stru_SyntaxErrorHandler_t* handler)
{
    Stru_SyntaxErrorHandlerState_t* state = F_get_handler_state(handler);
    if (state == NULL || state->errors == NULL) {
        return;
    }
    
    size_t error_count = F_dynamic_array_length(state->errors);
    for (size_t i = 0; i < error_count; i++) {
        Stru_SyntaxError_t** error_ptr = F_dynamic_array_get(state->errors, i);
        if (error_ptr != NULL && *error_ptr != NULL) {
            char buffer[512];
            int written = F_handler_format_error(handler, *error_ptr, buffer, sizeof(buffer));
            if (written > 0) {
                printf("%s\n", buffer);
            }
        }
    }
}

/**
 * @brief 销毁错误处理接口（接口函数）
 */
static void F_handler_destroy(Stru_SyntaxErrorHandler_t* handler)
{
    if (handler == NULL) {
        return;
    }
    
    Stru_SyntaxErrorHandlerState_t* state = F_get_handler_state(handler);
    if (state != NULL) {
        F_destroy_handler_state(state);
    }
    
    cn_free(handler);
}

// ============================================
// 工厂函数实现
// ============================================

/**
 * @brief 创建错误处理接口实例
 */
Stru_SyntaxErrorHandler_t* F_create_syntax_error_handler(void)
{
    Stru_SyntaxErrorHandler_t* handler = cn_malloc(sizeof(Stru_SyntaxErrorHandler_t));
    if (handler == NULL) {
        return NULL;
    }
    
    // 创建内部状态
    Stru_SyntaxErrorHandlerState_t* state = F_create_handler_state();
    if (state == NULL) {
        cn_free(handler);
        return NULL;
    }
    
    handler->internal_state = state;
    
    // 设置接口函数指针
    handler->report_error = F_handler_report_error;
    handler->report_unexpected_token = F_handler_report_unexpected_token;
    handler->report_missing_token = F_handler_report_missing_token;
    handler->has_errors = F_handler_has_errors;
    handler->has_errors_of_severity = F_handler_has_errors_of_severity;
    handler->get_error_count = F_handler_get_error_count;
    handler->get_error_count_by_severity = F_handler_get_error_count_by_severity;
    handler->get_errors = F_handler_get_errors;
    handler->get_errors_by_severity = F_handler_get_errors_by_severity;
    handler->get_last_error = F_handler_get_last_error;
    handler->clear_errors = F_handler_clear_errors;
    handler->clear_errors_by_severity = F_handler_clear_errors_by_severity;
    handler->set_max_errors = F_handler_set_max_errors;
    handler->get_max_errors = F_handler_get_max_errors;
    handler->format_error = F_handler_format_error;
    handler->print_all_errors = F_handler_print_all_errors;
    handler->destroy = F_handler_destroy;
    
    return handler;
}
