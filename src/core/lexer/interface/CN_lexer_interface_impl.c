/**
 * @file CN_lexer_interface_impl.c
 * @brief CN_Language 词法分析器接口实现
 * 
 * 提供词法分析器接口的具体实现，包括工厂函数和接口包装器。
 * 遵循项目架构规范，实现模块解耦和依赖注入。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include "CN_lexer_interface_impl.h"
#include "../scanner/CN_lexer_scanner.h"
#include "../token_scanners/CN_lexer_token_scanners.h"
#include "../../../infrastructure/containers/array/CN_dynamic_array.h"
#include <stdlib.h>

// ============================================
// 内部辅助函数
// ============================================

/**
 * @brief 从接口获取扫描器状态
 */
static Stru_LexerScannerState_t* F_get_scanner_state(Stru_LexerInterface_t* interface)
{
    if (interface == NULL)
    {
        return NULL;
    }
    return (Stru_LexerScannerState_t*)interface->internal_state;
}

// ============================================
// 接口函数实现（包装器）
// ============================================

/**
 * @brief 初始化词法分析器（包装器）
 */
static bool F_lexer_initialize_wrapper(Stru_LexerInterface_t* interface, const char* source, size_t length, const char* source_name)
{
    Stru_LexerScannerState_t* state = F_get_scanner_state(interface);
    if (state == NULL)
    {
        return false;
    }
    
    return F_initialize_scanner_state(state, source, length, source_name);
}

/**
 * @brief 获取下一个令牌（包装器）
 */
static Stru_Token_t* F_lexer_next_token_wrapper(Stru_LexerInterface_t* interface)
{
    Stru_LexerScannerState_t* state = F_get_scanner_state(interface);
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
    Stru_LexerScannerState_t* state = F_get_scanner_state(interface);
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
    Stru_LexerScannerState_t* state = F_get_scanner_state(interface);
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
    F_clear_scanner_error(state);
    
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
    Stru_LexerScannerState_t* state = F_get_scanner_state(interface);
    if (state == NULL || line == NULL || column == NULL)
    {
        return;
    }
    
    F_get_scanner_position(state, line, column);
}

/**
 * @brief 获取源文件名（包装器）
 */
static const char* F_lexer_get_source_name_wrapper(Stru_LexerInterface_t* interface)
{
    Stru_LexerScannerState_t* state = F_get_scanner_state(interface);
    if (state == NULL)
    {
        return "unknown.cn";
    }
    
    return F_get_scanner_source_name(state);
}

/**
 * @brief 检查是否有错误（包装器）
 */
static bool F_lexer_has_errors_wrapper(Stru_LexerInterface_t* interface)
{
    Stru_LexerScannerState_t* state = F_get_scanner_state(interface);
    if (state == NULL)
    {
        return false;
    }
    
    return F_scanner_has_errors(state);
}

/**
 * @brief 获取最后一个错误信息（包装器）
 */
static const char* F_lexer_get_last_error_wrapper(Stru_LexerInterface_t* interface)
{
    Stru_LexerScannerState_t* state = F_get_scanner_state(interface);
    if (state == NULL)
    {
        return "";
    }
    
    return F_get_scanner_last_error(state);
}

/**
 * @brief 重置词法分析器（包装器）
 */
static void F_lexer_reset_wrapper(Stru_LexerInterface_t* interface)
{
    Stru_LexerScannerState_t* state = F_get_scanner_state(interface);
    if (state == NULL)
    {
        return;
    }
    
    F_reset_scanner_state(state);
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
        F_destroy_scanner_state((Stru_LexerScannerState_t*)interface->internal_state);
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
    
    // 创建内部状态（扫描器状态）
    Stru_LexerScannerState_t* state = F_create_scanner_state();
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
