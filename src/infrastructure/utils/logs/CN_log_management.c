/******************************************************************************
 * 文件名: CN_log_management.c
 * 功能: CN_Language日志系统管理函数
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，包含输出管理和格式化器管理函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_log_internal.h"
#include <string.h>

// ============================================================================
// 输出管理函数实现
// ============================================================================

/**
 * @brief 添加日志输出处理器
 */
bool CN_log_add_output(Eum_CN_LogOutputType_t type, const void* config)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (!state->initialized || state->output_count >= CN_LOG_MAX_OUTPUTS)
        return false;
    
    CN_log_mutex_lock();
    
    // 查找是否已存在该类型的输出
    for (size_t i = 0; i < state->output_count; i++)
    {
        if (state->outputs[i].type == type)
        {
            CN_log_mutex_unlock();
            return false; // 已存在
        }
    }
    
    // 添加新输出
    Stru_CN_RegisteredOutput_t* output = &state->outputs[state->output_count];
    output->type = type;
    output->config = config;
    output->handler = NULL;
    output->initialized = false;
    
    // 查找对应的处理器
    switch (type)
    {
        case Eum_LOG_OUTPUT_CONSOLE:
            output->handler = (Stru_CN_LogOutputHandlerInterface_t*)&CN_log_console_output_handler;
            break;
        case Eum_LOG_OUTPUT_FILE:
            output->handler = (Stru_CN_LogOutputHandlerInterface_t*)&CN_log_file_output_handler;
            break;
        default:
            // 其他类型需要用户通过CN_log_set_output_handler设置
            break;
    }
    
    if (output->handler)
    {
        output->initialized = output->handler->initialize(config);
    }
    
    if (output->initialized)
        state->output_count++;
    
    CN_log_mutex_unlock();
    
    return output->initialized;
}

/**
 * @brief 移除日志输出处理器
 */
bool CN_log_remove_output(Eum_CN_LogOutputType_t type)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (!state->initialized)
        return false;
    
    CN_log_mutex_lock();
    
    bool found = false;
    for (size_t i = 0; i < state->output_count; i++)
    {
        if (state->outputs[i].type == type)
        {
            // 关闭处理器
            if (state->outputs[i].handler && state->outputs[i].initialized)
                state->outputs[i].handler->shutdown();
            
            // 移动后面的元素向前
            for (size_t j = i; j < state->output_count - 1; j++)
                state->outputs[j] = state->outputs[j + 1];
            
            state->output_count--;
            found = true;
            break;
        }
    }
    
    CN_log_mutex_unlock();
    return found;
}

/**
 * @brief 移除所有输出处理器
 */
void CN_log_remove_all_outputs(void)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (!state->initialized)
        return;
    
    CN_log_mutex_lock();
    
    for (size_t i = 0; i < state->output_count; i++)
    {
        if (state->outputs[i].handler && state->outputs[i].initialized)
            state->outputs[i].handler->shutdown();
    }
    
    state->output_count = 0;
    
    CN_log_mutex_unlock();
}

/**
 * @brief 设置输出处理器
 */
bool CN_log_set_output_handler(Eum_CN_LogOutputType_t type,
                               const Stru_CN_LogOutputHandlerInterface_t* handler)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (!state->initialized || !handler)
        return false;
    
    CN_log_mutex_lock();
    
    bool found = false;
    for (size_t i = 0; i < state->output_count; i++)
    {
        if (state->outputs[i].type == type)
        {
            // 关闭旧的处理器
            if (state->outputs[i].handler && state->outputs[i].initialized)
                state->outputs[i].handler->shutdown();
            
            // 设置新的处理器
            state->outputs[i].handler = (Stru_CN_LogOutputHandlerInterface_t*)handler;
            state->outputs[i].initialized = handler->initialize(state->outputs[i].config);
            found = true;
            break;
        }
    }
    
    CN_log_mutex_unlock();
    return found;
}

// ============================================================================
// 格式化器管理函数实现
// ============================================================================

/**
 * @brief 注册日志格式化器
 */
bool CN_log_register_formatter(const char* name,
                               const Stru_CN_LogFormatterInterface_t* formatter)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (!state->initialized || !name || !formatter || 
        state->formatter_count >= CN_LOG_MAX_FORMATTERS)
        return false;
    
    CN_log_mutex_lock();
    
    // 检查是否已存在
    for (size_t i = 0; i < state->formatter_count; i++)
    {
        if (strcmp(state->formatters[i].name, name) == 0)
        {
            CN_log_mutex_unlock();
            return false;
        }
    }
    
    // 注册新格式化器
    state->formatters[state->formatter_count].name = name;
    state->formatters[state->formatter_count].impl = formatter;
    state->formatter_count++;
    
    CN_log_mutex_unlock();
    return true;
}

/**
 * @brief 设置当前格式化器
 */
bool CN_log_set_formatter(const char* name)
{
    Stru_CN_LogSystemState_t* state = CN_log_get_state();
    
    if (!state->initialized || !name)
        return false;
    
    CN_log_mutex_lock();
    
    for (size_t i = 0; i < state->formatter_count; i++)
    {
        if (strcmp(state->formatters[i].name, name) == 0)
        {
            state->current_formatter = state->formatters[i].impl;
            CN_log_mutex_unlock();
            return true;
        }
    }
    
    CN_log_mutex_unlock();
    return false;
}
