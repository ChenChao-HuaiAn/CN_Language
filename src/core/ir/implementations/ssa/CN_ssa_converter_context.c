/******************************************************************************
 * 文件名: CN_ssa_converter_context.c
 * 功能: CN_Language SSA转换器上下文管理
 * 
 * 实现SSA转换器的上下文管理功能。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_ssa_converter_internal.h"

// ============================================================================
// 静态辅助函数
// ============================================================================

/**
 * @brief 复制字符串（静态辅助函数）
 * 
 * @param str 要复制的字符串
 * @return char* 复制后的字符串，失败返回NULL
 */
static char* cn_strdup(const char* str)
{
    if (str == NULL) {
        return NULL;
    }
    
    size_t len = strlen(str) + 1;
    char* copy = (char*)cn_malloc(len);
    if (copy == NULL) {
        return NULL;
    }
    
    memcpy(copy, str, len);
    return copy;
}

// ============================================================================
// 转换器上下文函数实现
// ============================================================================

/**
 * @brief 创建转换器上下文
 */
Stru_SsaConverterContext_t* F_create_converter_context(void)
{
    Stru_SsaConverterContext_t* context = (Stru_SsaConverterContext_t*)cn_malloc(
        sizeof(Stru_SsaConverterContext_t));
    if (context == NULL) {
        return NULL;
    }
    
    context->ssa_data = NULL;
    context->current_function = NULL;
    context->current_block = NULL;
    context->var_map = F_create_variable_version_map();
    context->next_temp_id = 0;
    context->errors = NULL;
    context->error_count = 0;
    
    if (context->var_map == NULL) {
        cn_free(context);
        return NULL;
    }
    
    return context;
}

/**
 * @brief 销毁转换器上下文
 */
void F_destroy_converter_context(Stru_SsaConverterContext_t* context)
{
    if (context == NULL) {
        return;
    }
    
    if (context->var_map != NULL) {
        F_destroy_variable_version_map(context->var_map);
    }
    
    // 释放错误信息数组
    if (context->errors != NULL) {
        for (size_t i = 0; i < context->error_count; i++) {
            if (context->errors[i] != NULL) {
                cn_free(context->errors[i]);
            }
        }
        cn_free(context->errors);
    }
    
    cn_free(context);
}

/**
 * @brief 添加错误信息
 */
bool F_add_error(Stru_SsaConverterContext_t* context, const char* error)
{
    if (context == NULL || error == NULL) {
        return false;
    }
    
    // 重新分配错误信息数组
    size_t new_count = context->error_count + 1;
    char** new_errors = (char**)cn_realloc(
        context->errors, new_count * sizeof(char*));
    
    if (new_errors == NULL) {
        return false;
    }
    
    // 复制错误信息
    char* error_copy = cn_strdup(error);
    if (error_copy == NULL) {
        return false;
    }
    
    context->errors = new_errors;
    context->errors[context->error_count] = error_copy;
    context->error_count = new_count;
    
    return true;
}
