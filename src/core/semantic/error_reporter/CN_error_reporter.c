/**
 * @file CN_error_reporter.c
 * @brief 语义错误报告器模块实现
 
 * 本文件实现了语义错误报告器模块的功能，包括错误收集、管理和报告等操作。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_error_reporter.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 语义错误报告接口私有数据结构
// ============================================================================

/**
 * @brief 语义错误报告接口私有数据
 */
typedef struct Stru_SemanticErrorReporterData_t
{
    size_t max_errors;                         ///< 最大错误数量限制
    size_t error_count;                        ///< 错误数量
    size_t warning_count;                      ///< 警告数量
    size_t error_capacity;                     ///< 错误数组容量
    Stru_SemanticError_t* errors;              ///< 错误数组
    void (*error_callback)(const Stru_SemanticError_t* error, void* user_data);  ///< 错误回调
    void* callback_user_data;                  ///< 回调用户数据
} Stru_SemanticErrorReporterData_t;

// ============================================================================
// 语义错误报告接口实现函数
// ============================================================================

/**
 * @brief 语义错误报告器初始化函数
 */
static bool semantic_error_reporter_initialize(
    Stru_SemanticErrorReporterInterface_t* reporter, size_t max_errors)
{
    if (reporter == NULL || reporter->private_data != NULL)
    {
        return false;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)malloc(sizeof(Stru_SemanticErrorReporterData_t));
    if (data == NULL)
    {
        return false;
    }
    
    data->max_errors = max_errors;
    data->error_count = 0;
    data->warning_count = 0;
    data->error_capacity = (max_errors > 0 && max_errors < 64) ? max_errors : 64;
    data->errors = (Stru_SemanticError_t*)malloc(data->error_capacity * sizeof(Stru_SemanticError_t));
    data->error_callback = NULL;
    data->callback_user_data = NULL;
    
    if (data->errors == NULL)
    {
        free(data);
        return false;
    }
    
    reporter->private_data = data;
    return true;
}

/**
 * @brief 报告语义错误
 */
static bool semantic_error_reporter_report_error(
    Stru_SemanticErrorReporterInterface_t* reporter, const Stru_SemanticError_t* error)
{
    if (reporter == NULL || reporter->private_data == NULL || error == NULL)
    {
        return false;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    
    // 检查错误数量限制
    if (data->max_errors > 0 && data->error_count >= data->max_errors)
    {
        return false;
    }
    
    // 检查是否需要扩容
    if (data->error_count >= data->error_capacity)
    {
        size_t new_capacity = data->error_capacity * 2;
        Stru_SemanticError_t* new_errors = 
            (Stru_SemanticError_t*)realloc(data->errors, new_capacity * sizeof(Stru_SemanticError_t));
        if (new_errors == NULL)
        {
            return false;
        }
        data->errors = new_errors;
        data->error_capacity = new_capacity;
    }
    
    // 添加错误
    data->errors[data->error_count] = *error;
    data->error_count++;
    
    // 调用回调函数
    if (data->error_callback != NULL)
    {
        data->error_callback(error, data->callback_user_data);
    }
    
    return true;
}

/**
 * @brief 报告语义警告
 */
static bool semantic_error_reporter_report_warning(
    Stru_SemanticErrorReporterInterface_t* reporter, const Stru_SemanticError_t* error)
{
    if (reporter == NULL || reporter->private_data == NULL || error == NULL)
    {
        return false;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    
    data->warning_count++;
    
    // 调用回调函数
    if (data->error_callback != NULL)
    {
        data->error_callback(error, data->callback_user_data);
    }
    
    return true;
}

/**
 * @brief 获取错误数量
 */
static size_t semantic_error_reporter_get_error_count(
    Stru_SemanticErrorReporterInterface_t* reporter)
{
    if (reporter == NULL || reporter->private_data == NULL)
    {
        return 0;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    return data->error_count;
}

/**
 * @brief 获取警告数量
 */
static size_t semantic_error_reporter_get_warning_count(
    Stru_SemanticErrorReporterInterface_t* reporter)
{
    if (reporter == NULL || reporter->private_data == NULL)
    {
        return 0;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    return data->warning_count;
}

/**
 * @brief 获取所有错误
 */
static size_t semantic_error_reporter_get_all_errors(
    Stru_SemanticErrorReporterInterface_t* reporter, Stru_SemanticError_t* errors,
    size_t max_errors)
{
    if (reporter == NULL || reporter->private_data == NULL || errors == NULL)
    {
        return 0;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    size_t count = (data->error_count < max_errors) ? data->error_count : max_errors;
    
    for (size_t i = 0; i < count; i++)
    {
        errors[i] = data->errors[i];
    }
    
    return count;
}

/**
 * @brief 获取所有警告
 */
static size_t semantic_error_reporter_get_all_warnings(
    Stru_SemanticErrorReporterInterface_t* reporter, Stru_SemanticError_t* warnings,
    size_t max_warnings)
{
    (void)reporter;
    (void)warnings;
    (void)max_warnings;
    
    // 桩实现：警告不单独存储
    return 0;
}

/**
 * @brief 清空所有错误和警告
 */
static void semantic_error_reporter_clear_all(
    Stru_SemanticErrorReporterInterface_t* reporter)
{
    if (reporter == NULL || reporter->private_data == NULL)
    {
        return;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    data->error_count = 0;
    data->warning_count = 0;
}

/**
 * @brief 检查是否有错误
 */
static bool semantic_error_reporter_has_errors(
    Stru_SemanticErrorReporterInterface_t* reporter)
{
    if (reporter == NULL || reporter->private_data == NULL)
    {
        return false;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    return data->error_count > 0;
}

/**
 * @brief 检查是否有警告
 */
static bool semantic_error_reporter_has_warnings(
    Stru_SemanticErrorReporterInterface_t* reporter)
{
    if (reporter == NULL || reporter->private_data == NULL)
    {
        return false;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    return data->warning_count > 0;
}

/**
 * @brief 设置错误处理回调
 */
static void semantic_error_reporter_set_error_callback(
    Stru_SemanticErrorReporterInterface_t* reporter,
    void (*callback)(const Stru_SemanticError_t* error, void* user_data),
    void* user_data)
{
    if (reporter == NULL || reporter->private_data == NULL)
    {
        return;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    data->error_callback = callback;
    data->callback_user_data = user_data;
}

/**
 * @brief 过滤错误
 */
static size_t semantic_error_reporter_filter_errors(
    Stru_SemanticErrorReporterInterface_t* reporter,
    Eum_SemanticErrorType filter_type,
    size_t min_line, size_t max_line,
    const char* file_name)
{
    if (reporter == NULL || reporter->private_data == NULL)
    {
        return 0;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    
    size_t filtered_count = 0;
    
    for (size_t i = 0; i < data->error_count; i++)
    {
        Stru_SemanticError_t* error = &data->errors[i];
        bool matches = true;
        
        // 过滤错误类型
        if (filter_type != 0 && error->type != filter_type)
        {
            matches = false;
        }
        
        // 过滤行号范围
        if (min_line > 0 && error->line < min_line)
        {
            matches = false;
        }
        if (max_line > 0 && error->line > max_line)
        {
            matches = false;
        }
        
        // 过滤文件名
        if (file_name != NULL && error->file_name != NULL && 
            strcmp(error->file_name, file_name) != 0)
        {
            matches = false;
        }
        
        if (matches)
        {
            filtered_count++;
        }
    }
    
    return filtered_count;
}

/**
 * @brief 分类错误
 */
static size_t semantic_error_reporter_categorize_errors(
    Stru_SemanticErrorReporterInterface_t* reporter,
    size_t* error_counts, size_t max_types)
{
    if (reporter == NULL || reporter->private_data == NULL || error_counts == NULL)
    {
        return 0;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    
    // 初始化错误计数数组
    for (size_t i = 0; i < max_types; i++)
    {
        error_counts[i] = 0;
    }
    
    // 统计各类错误数量
    size_t type_count = 0;
    
    for (size_t i = 0; i < data->error_count; i++)
    {
        Stru_SemanticError_t* error = &data->errors[i];
        size_t type_index = (size_t)error->type;
        
        if (type_index < max_types)
        {
            error_counts[type_index]++;
            if (error_counts[type_index] == 1)
            {
                type_count++;
            }
        }
    }
    
    return type_count;
}

/**
 * @brief 跟踪错误位置
 */
static bool semantic_error_reporter_track_error_location(
    Stru_SemanticErrorReporterInterface_t* reporter,
    size_t error_index,
    size_t* line, size_t* column,
    char* file_name, size_t file_name_size)
{
    if (reporter == NULL || reporter->private_data == NULL || 
        line == NULL || column == NULL || file_name == NULL)
    {
        return false;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    
    if (error_index >= data->error_count)
    {
        return false;
    }
    
    Stru_SemanticError_t* error = &data->errors[error_index];
    
    *line = error->line;
    *column = error->column;
    
    if (error->file_name != NULL)
    {
        // 复制文件名，确保不超过缓冲区大小
        size_t copy_size = file_name_size - 1;
        if (strlen(error->file_name) < copy_size)
        {
            copy_size = strlen(error->file_name);
        }
        strncpy(file_name, error->file_name, copy_size);
        file_name[copy_size] = '\0';
    }
    else
    {
        file_name[0] = '\0';
    }
    
    return true;
}

/**
 * @brief 获取错误严重性
 */
static int semantic_error_reporter_get_error_severity(
    Stru_SemanticErrorReporterInterface_t* reporter,
    size_t error_index)
{
    if (reporter == NULL || reporter->private_data == NULL)
    {
        return 0;
    }
    
    Stru_SemanticErrorReporterData_t* data = 
        (Stru_SemanticErrorReporterData_t*)reporter->private_data;
    
    if (error_index >= data->error_count)
    {
        return 0;
    }
    
    Stru_SemanticError_t* error = &data->errors[error_index];
    
    // 根据错误类型判断严重性
    switch (error->type)
    {
        case Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL:
        case Eum_SEMANTIC_ERROR_TYPE_MISMATCH:
        case Eum_SEMANTIC_ERROR_INVALID_OPERATION:
        case Eum_SEMANTIC_ERROR_INVALID_ARGUMENTS:
        case Eum_SEMANTIC_ERROR_MISSING_RETURN:
        case Eum_SEMANTIC_ERROR_INVALID_RETURN_TYPE:
        case Eum_SEMANTIC_ERROR_UNINITIALIZED_VARIABLE:
        case Eum_SEMANTIC_ERROR_CONST_ASSIGNMENT:
        case Eum_SEMANTIC_ERROR_SCOPE_VIOLATION:
        case Eum_SEMANTIC_ERROR_CIRCULAR_DEPENDENCY:
            return 8;  // 严重错误
            
        case Eum_SEMANTIC_ERROR_REDEFINED_SYMBOL:
        case Eum_SEMANTIC_ERROR_INVALID_IMPORT:
        case Eum_SEMANTIC_ERROR_INVALID_EXPORT:
            return 6;  // 中等错误
            
        case Eum_SEMANTIC_ERROR_UNREACHABLE_CODE:
            return 4;  // 警告级别
            
        default:
            return 5;  // 默认中等错误
    }
}

/**
 * @brief 销毁错误报告器
 */
static void semantic_error_reporter_destroy(
    Stru_SemanticErrorReporterInterface_t* reporter)
{
    if (reporter == NULL)
    {
        return;
    }
    
    if (reporter->private_data != NULL)
    {
        Stru_SemanticErrorReporterData_t* data = 
            (Stru_SemanticErrorReporterData_t*)reporter->private_data;
        
        if (data->errors != NULL)
        {
            free(data->errors);
        }
        
        free(data);
        reporter->private_data = NULL;
    }
}

// ============================================================================
// 工厂函数实现
// ============================================================================

/**
 * @brief 创建语义错误报告器接口实例
 */
Stru_SemanticErrorReporterInterface_t* F_create_semantic_error_reporter_interface(void)
{
    Stru_SemanticErrorReporterInterface_t* interface = 
        (Stru_SemanticErrorReporterInterface_t*)malloc(
            sizeof(Stru_SemanticErrorReporterInterface_t));
    
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 设置接口函数
    interface->initialize = semantic_error_reporter_initialize;
    interface->report_error = semantic_error_reporter_report_error;
    interface->report_warning = semantic_error_reporter_report_warning;
    interface->get_error_count = semantic_error_reporter_get_error_count;
    interface->get_warning_count = semantic_error_reporter_get_warning_count;
    interface->get_all_errors = semantic_error_reporter_get_all_errors;
    interface->get_all_warnings = semantic_error_reporter_get_all_warnings;
    interface->clear_all = semantic_error_reporter_clear_all;
    interface->has_errors = semantic_error_reporter_has_errors;
    interface->has_warnings = semantic_error_reporter_has_warnings;
    interface->set_error_callback = semantic_error_reporter_set_error_callback;
    interface->filter_errors = semantic_error_reporter_filter_errors;
    interface->categorize_errors = semantic_error_reporter_categorize_errors;
    interface->track_error_location = semantic_error_reporter_track_error_location;
    interface->get_error_severity = semantic_error_reporter_get_error_severity;
    interface->destroy = semantic_error_reporter_destroy;
    interface->private_data = NULL;
    
    return interface;
}

/**
 * @brief 销毁语义错误报告器接口实例
 */
void F_destroy_semantic_error_reporter_interface(
    Stru_SemanticErrorReporterInterface_t* interface)
{
    if (interface == NULL)
    {
        return;
    }
    
    if (interface->destroy != NULL)
    {
        interface->destroy(interface);
    }
    
    free(interface);
}
