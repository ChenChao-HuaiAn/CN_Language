/**
 * @file CN_lexer_impl.c
 * @brief CN_Language 词法分析器核心实现（适配器版本）
 * 
 * 使用新的模块化实现作为词法分析器的核心实现。
 * 此文件作为适配器，保持与原始接口的兼容性。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 */

#include "CN_lexer_interface.h"
#include "interface/CN_lexer_interface_impl.h"
#include <stdlib.h>

// ============================================
// 内部状态结构体（简化版本）
// ============================================

/**
 * @brief 词法分析器适配器状态
 * 
 * 存储词法分析器的内部状态，包装新的模块化实现。
 */
typedef struct {
    Stru_LexerInterface_t* lexer_interface;  ///< 新的词法分析器接口
} Stru_LexerAdapterState_t;

// ============================================
// 内部辅助函数
// ============================================

/**
 * @brief 创建适配器状态
 */
static Stru_LexerAdapterState_t* F_create_adapter_state(void)
{
    Stru_LexerAdapterState_t* state = (Stru_LexerAdapterState_t*)malloc(sizeof(Stru_LexerAdapterState_t));
    if (state == NULL)
    {
        return NULL;
    }
    
    state->lexer_interface = F_create_lexer_interface_impl();
    if (state->lexer_interface == NULL)
    {
        free(state);
        return NULL;
    }
    
    return state;
}

/**
 * @brief 销毁适配器状态
 */
static void F_destroy_adapter_state(Stru_LexerAdapterState_t* state)
{
    if (state != NULL)
    {
        if (state->lexer_interface != NULL)
        {
            state->lexer_interface->destroy(state->lexer_interface);
        }
        free(state);
    }
}

/**
 * @brief 从接口获取适配器状态
 */
static Stru_LexerAdapterState_t* F_get_adapter_state(Stru_LexerInterface_t* interface)
{
    if (interface == NULL)
    {
        return NULL;
    }
    return (Stru_LexerAdapterState_t*)interface->internal_state;
}

// ============================================
// 接口函数实现（适配器）
// ============================================

/**
 * @brief 初始化词法分析器（适配器）
 */
static bool F_lexer_initialize_adapter(Stru_LexerInterface_t* interface, const char* source, size_t length, const char* source_name)
{
    Stru_LexerAdapterState_t* state = F_get_adapter_state(interface);
    if (state == NULL || state->lexer_interface == NULL)
    {
        return false;
    }
    
    return state->lexer_interface->initialize(state->lexer_interface, source, length, source_name);
}

/**
 * @brief 获取下一个令牌（适配器）
 */
static Stru_Token_t* F_lexer_next_token_adapter(Stru_LexerInterface_t* interface)
{
    Stru_LexerAdapterState_t* state = F_get_adapter_state(interface);
    if (state == NULL || state->lexer_interface == NULL)
    {
        return NULL;
    }
    
    return state->lexer_interface->next_token(state->lexer_interface);
}

/**
 * @brief 检查是否还有更多令牌（适配器）
 */
static bool F_lexer_has_more_tokens_adapter(Stru_LexerInterface_t* interface)
{
    Stru_LexerAdapterState_t* state = F_get_adapter_state(interface);
    if (state == NULL || state->lexer_interface == NULL)
    {
        return false;
    }
    
    return state->lexer_interface->has_more_tokens(state->lexer_interface);
}

/**
 * @brief 批量令牌化（适配器）
 */
static Stru_DynamicArray_t* F_lexer_tokenize_all_adapter(Stru_LexerInterface_t* interface)
{
    Stru_LexerAdapterState_t* state = F_get_adapter_state(interface);
    if (state == NULL || state->lexer_interface == NULL)
    {
        return NULL;
    }
    
    return state->lexer_interface->tokenize_all(state->lexer_interface);
}

/**
 * @brief 获取当前位置（适配器）
 */
static void F_lexer_get_position_adapter(Stru_LexerInterface_t* interface, size_t* line, size_t* column)
{
    Stru_LexerAdapterState_t* state = F_get_adapter_state(interface);
    if (state == NULL || state->lexer_interface == NULL || line == NULL || column == NULL)
    {
        return;
    }
    
    state->lexer_interface->get_position(state->lexer_interface, line, column);
}

/**
 * @brief 获取源文件名（适配器）
 */
static const char* F_lexer_get_source_name_adapter(Stru_LexerInterface_t* interface)
{
    Stru_LexerAdapterState_t* state = F_get_adapter_state(interface);
    if (state == NULL || state->lexer_interface == NULL)
    {
        return "unknown.cn";
    }
    
    return state->lexer_interface->get_source_name(state->lexer_interface);
}

/**
 * @brief 检查是否有错误（适配器）
 */
static bool F_lexer_has_errors_adapter(Stru_LexerInterface_t* interface)
{
    Stru_LexerAdapterState_t* state = F_get_adapter_state(interface);
    if (state == NULL || state->lexer_interface == NULL)
    {
        return false;
    }
    
    return state->lexer_interface->has_errors(state->lexer_interface);
}

/**
 * @brief 获取最后一个错误信息（适配器）
 */
static const char* F_lexer_get_last_error_adapter(Stru_LexerInterface_t* interface)
{
    Stru_LexerAdapterState_t* state = F_get_adapter_state(interface);
    if (state == NULL || state->lexer_interface == NULL)
    {
        return "";
    }
    
    return state->lexer_interface->get_last_error(state->lexer_interface);
}

/**
 * @brief 重置词法分析器（适配器）
 */
static void F_lexer_reset_adapter(Stru_LexerInterface_t* interface)
{
    Stru_LexerAdapterState_t* state = F_get_adapter_state(interface);
    if (state == NULL || state->lexer_interface == NULL)
    {
        return;
    }
    
    state->lexer_interface->reset(state->lexer_interface);
}

/**
 * @brief 销毁词法分析器（适配器）
 */
static void F_lexer_destroy_adapter(Stru_LexerInterface_t* interface)
{
    if (interface == NULL)
    {
        return;
    }
    
    if (interface->internal_state != NULL)
    {
        F_destroy_adapter_state((Stru_LexerAdapterState_t*)interface->internal_state);
        interface->internal_state = NULL;
    }
    
    free(interface);
}

// ============================================
// 工厂函数实现（适配器版本）
// ============================================

/**
 * @brief 创建词法分析器接口实例（适配器版本）
 * 
 * 注意：此函数与interface/CN_lexer_interface_impl.c中的函数重复。
 * 为了保持向后兼容性，我们提供此实现，但将其设为静态以避免链接冲突。
 */
static Stru_LexerInterface_t* F_create_lexer_interface_impl_adapter(void)
{
    // 分配接口内存
    Stru_LexerInterface_t* interface = (Stru_LexerInterface_t*)malloc(sizeof(Stru_LexerInterface_t));
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 创建适配器状态
    Stru_LexerAdapterState_t* state = F_create_adapter_state();
    if (state == NULL)
    {
        free(interface);
        return NULL;
    }
    
    // 设置接口函数指针
    interface->initialize = F_lexer_initialize_adapter;
    interface->next_token = F_lexer_next_token_adapter;
    interface->has_more_tokens = F_lexer_has_more_tokens_adapter;
    interface->tokenize_all = F_lexer_tokenize_all_adapter;
    interface->get_position = F_lexer_get_position_adapter;
    interface->get_source_name = F_lexer_get_source_name_adapter;
    interface->has_errors = F_lexer_has_errors_adapter;
    interface->get_last_error = F_lexer_get_last_error_adapter;
    interface->reset = F_lexer_reset_adapter;
    interface->destroy = F_lexer_destroy_adapter;
    interface->internal_state = state;
    
    return interface;
}

/**
 * @brief 创建词法分析器接口实例（公共接口 - 适配器版本）
 * 
 * 注意：此函数与CN_lexer_interface.c中的函数重复。
 * 为了保持向后兼容性，我们提供此实现，但只在定义了USE_ADAPTER_VERSION宏时才启用。
 */
#ifdef USE_ADAPTER_VERSION
Stru_LexerInterface_t* F_create_lexer_interface(void)
{
    return F_create_lexer_interface_impl_adapter();
}
#endif
