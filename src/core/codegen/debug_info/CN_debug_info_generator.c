/******************************************************************************
 * 文件名: CN_debug_info_generator.c
 * 功能: CN_Language 调试信息生成器实现
 * 
 * 实现调试信息生成器的具体功能，支持生成源代码映射、符号表、行号表等调试信息。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_debug_info_interface.h"
#include "../../../infrastructure/memory/CN_memory_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// ============================================================================
// 内部类型定义
// ============================================================================

/**
 * @brief 调试信息生成器内部状态结构体
 */
typedef struct Stru_DebugInfoGeneratorState_t {
    Stru_DebugInfoConfig_t config;          ///< 当前配置
    Stru_DynamicArray_t* errors;            ///< 错误信息数组
    Stru_DynamicArray_t* warnings;          ///< 警告信息数组
    bool initialized;                       ///< 是否已初始化
} Stru_DebugInfoGeneratorState_t;

// ============================================================================
// 内部函数声明
// ============================================================================

static Stru_DebugInfoGeneratorState_t* create_state(void);
static void destroy_state(Stru_DebugInfoGeneratorState_t* state);
static void add_error(Stru_DebugInfoGeneratorState_t* state, const char* format, ...);
static void add_warning(Stru_DebugInfoGeneratorState_t* state, const char* format, ...);

// 工厂函数实现
static Stru_DebugInfoGeneratorInterface_t* create_interface_impl(Eum_DebugInfoFormat format);

// 接口函数实现
static bool initialize_impl(Stru_DebugInfoGeneratorInterface_t* interface, 
                           const Stru_DebugInfoConfig_t* config);
static Stru_DebugInfoResult_t* generate_from_ast_impl(Stru_DebugInfoGeneratorInterface_t* interface,
                                                     Stru_AstNode_t* ast, uint64_t code_address);
static Stru_DebugInfoResult_t* generate_from_ir_impl(Stru_DebugInfoGeneratorInterface_t* interface,
                                                    void* ir, uint64_t code_address);
static Stru_DebugInfoResult_t* generate_source_map_impl(Stru_DebugInfoGeneratorInterface_t* interface,
                                                       const char* generated_code,
                                                       const char* source_code,
                                                       Stru_SourceMapEntry_t* mappings,
                                                       size_t mapping_count);
static Stru_DebugInfoResult_t* convert_format_impl(Stru_DebugInfoGeneratorInterface_t* interface,
                                                  const Stru_DebugInfoResult_t* source_result,
                                                  Eum_DebugInfoFormat target_format);
static bool serialize_impl(Stru_DebugInfoGeneratorInterface_t* interface,
                          const Stru_DebugInfoResult_t* result,
                          uint8_t** data, size_t* size);
static Stru_DebugInfoResult_t* deserialize_impl(Stru_DebugInfoGeneratorInterface_t* interface,
                                               const uint8_t* data, size_t size,
                                               Eum_DebugInfoFormat format);
static Stru_DynamicArray_t* get_supported_formats_impl(Stru_DebugInfoGeneratorInterface_t* interface);
static bool is_format_supported_impl(Stru_DebugInfoGeneratorInterface_t* interface,
                                    Eum_DebugInfoFormat format);
static Stru_DebugInfoConfig_t get_current_config_impl(Stru_DebugInfoGeneratorInterface_t* interface);
static bool has_errors_impl(Stru_DebugInfoGeneratorInterface_t* interface);
static Stru_DynamicArray_t* get_errors_impl(Stru_DebugInfoGeneratorInterface_t* interface);
static void clear_errors_impl(Stru_DebugInfoGeneratorInterface_t* interface);
static void reset_impl(Stru_DebugInfoGeneratorInterface_t* interface);
static void destroy_impl(Stru_DebugInfoGeneratorInterface_t* interface);

// ============================================================================
// 内部函数实现
// ============================================================================

/**
 * @brief 创建调试信息生成器状态
 */
static Stru_DebugInfoGeneratorState_t* create_state(void)
{
    Stru_DebugInfoGeneratorState_t* state = (Stru_DebugInfoGeneratorState_t*)cn_malloc(
        sizeof(Stru_DebugInfoGeneratorState_t));
    if (!state) {
        return NULL;
    }
    
    // 初始化配置为默认值
    state->config = F_create_default_debug_info_config();
    
    // 创建错误和警告数组
    state->errors = F_create_dynamic_array(sizeof(char*));
    state->warnings = F_create_dynamic_array(sizeof(char*));
    
    if (!state->errors || !state->warnings) {
        if (state->errors) F_destroy_dynamic_array(state->errors);
        if (state->warnings) F_destroy_dynamic_array(state->warnings);
        cn_free(state);
        return NULL;
    }
    
    state->initialized = false;
    return state;
}

/**
 * @brief 销毁调试信息生成器状态
 */
static void destroy_state(Stru_DebugInfoGeneratorState_t* state)
{
    if (!state) {
        return;
    }
    
    // 释放错误信息字符串
    if (state->errors) {
        for (size_t i = 0; i < F_dynamic_array_length(state->errors); i++) {
            char** error_ptr = (char**)F_dynamic_array_get(state->errors, i);
            if (error_ptr && *error_ptr) {
                cn_free(*error_ptr);
            }
        }
        F_destroy_dynamic_array(state->errors);
    }
    
    // 释放警告信息字符串
    if (state->warnings) {
        for (size_t i = 0; i < F_dynamic_array_length(state->warnings); i++) {
            char** warning_ptr = (char**)F_dynamic_array_get(state->warnings, i);
            if (warning_ptr && *warning_ptr) {
                cn_free(*warning_ptr);
            }
        }
        F_destroy_dynamic_array(state->warnings);
    }
    
    cn_free(state);
}

/**
 * @brief 添加错误信息
 */
static void add_error(Stru_DebugInfoGeneratorState_t* state, const char* format, ...)
{
    if (!state || !state->errors || !format) {
        return;
    }
    
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    char* error_msg = strdup(buffer);
    if (error_msg) {
        F_dynamic_array_push(state->errors, &error_msg);
    }
}

/**
 * @brief 添加警告信息
 */
static void add_warning(Stru_DebugInfoGeneratorState_t* state, const char* format, ...)
{
    if (!state || !state->warnings || !format) {
        return;
    }
    
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    char* warning_msg = strdup(buffer);
    if (warning_msg) {
        F_dynamic_array_push(state->warnings, &warning_msg);
    }
}

// ============================================================================
// 工厂函数实现
// ============================================================================

/**
 * @brief 创建调试信息生成器接口实例（内部实现）
 */
static Stru_DebugInfoGeneratorInterface_t* create_interface_impl(Eum_DebugInfoFormat format)
{
    // 分配接口内存
    Stru_DebugInfoGeneratorInterface_t* interface = (Stru_DebugInfoGeneratorInterface_t*)cn_malloc(
        sizeof(Stru_DebugInfoGeneratorInterface_t));
    if (!interface) {
        return NULL;
    }
    
    // 创建内部状态
    Stru_DebugInfoGeneratorState_t* state = create_state();
    if (!state) {
        cn_free(interface);
        return NULL;
    }
    
    // 设置默认配置
    state->config.format = format;
    
    // 初始化所有函数指针
    interface->initialize = initialize_impl;
    interface->generate_from_ast = generate_from_ast_impl;
    interface->generate_from_ir = generate_from_ir_impl;
    interface->generate_source_map = generate_source_map_impl;
    interface->convert_format = convert_format_impl;
    interface->serialize = serialize_impl;
    interface->deserialize = deserialize_impl;
    interface->get_supported_formats = get_supported_formats_impl;
    interface->is_format_supported = is_format_supported_impl;
    interface->get_current_config = get_current_config_impl;
    interface->has_errors = has_errors_impl;
    interface->get_errors = get_errors_impl;
    interface->clear_errors = clear_errors_impl;
    interface->reset = reset_impl;
    interface->destroy = destroy_impl;
    
    // 设置内部状态
    interface->internal_state = state;
    
    return interface;
}

// ============================================================================
// 接口函数实现
// ============================================================================

/**
 * @brief 初始化调试信息生成器
 */
static bool initialize_impl(Stru_DebugInfoGeneratorInterface_t* interface, 
                           const Stru_DebugInfoConfig_t* config)
{
    if (!interface || !interface->internal_state || !config) {
        return false;
    }
    
    Stru_DebugInfoGeneratorState_t* state = (Stru_DebugInfoGeneratorState_t*)interface->internal_state;
    
    // 验证配置
    if (config->level < Eum_DEBUG_LEVEL_NONE || config->level > Eum_DEBUG_LEVEL_EXTENDED) {
        add_error(state, "无效的调试信息级别: %d", config->level);
        return false;
    }
    
    // 保存配置
    state->config = *config;
    state->initialized = true;
    
    return true;
}

/**
 * @brief 从AST生成调试信息
 */
static Stru_DebugInfoResult_t* generate_from_ast_impl(Stru_DebugInfoGeneratorInterface_t* interface,
                                                     Stru_AstNode_t* ast, uint64_t code_address)
{
    if (!interface || !interface->internal_state || !ast) {
        return NULL;
    }
    
    Stru_DebugInfoGeneratorState_t* state = (Stru_DebugInfoGeneratorState_t*)interface->internal_state;
    
    // 检查是否已初始化
    if (!state->initialized) {
        add_error(state, "调试信息生成器未初始化");
        return NULL;
    }
    
    // 创建结果结构体
    Stru_DebugInfoResult_t* result = (Stru_DebugInfoResult_t*)cn_malloc(sizeof(Stru_DebugInfoResult_t));
    if (!result) {
        add_error(state, "内存分配失败");
        return NULL;
    }
    
    // 初始化结果
    memset(result, 0, sizeof(Stru_DebugInfoResult_t));
    result->format = state->config.format;
    result->success = false;
    
    // 创建错误和警告数组
    result->errors = F_create_dynamic_array(sizeof(char*));
    result->warnings = F_create_dynamic_array(sizeof(char*));
    
    if (!result->errors || !result->warnings) {
        if (result->errors) F_destroy_dynamic_array(result->errors);
        if (result->warnings) F_destroy_dynamic_array(result->warnings);
        cn_free(result);
        return NULL;
    }
    
    // 根据配置级别生成调试信息
    switch (state->config.level) {
        case Eum_DEBUG_LEVEL_NONE:
            // 无调试信息
            result->success = true;
            break;
            
        case Eum_DEBUG_LEVEL_MINIMAL:
            // 最小调试信息：仅生成行号表
            result->line_table = F_create_dynamic_array(sizeof(Stru_LineTableEntry_t));
            if (result->line_table) {
                // 这里应该遍历AST生成行号表
                // 目前是占位符实现
                result->line_entry_count = 0;
                result->success = true;
            }
            break;
            
        case Eum_DEBUG_LEVEL_BASIC:
            // 基本调试信息：行号表+符号表
            result->line_table = F_create_dynamic_array(sizeof(Stru_LineTableEntry_t));
            result->symbols = F_create_dynamic_array(sizeof(Stru_DebugSymbol_t));
            if (result->line_table && result->symbols) {
                // 这里应该遍历AST生成行号表和符号表
                // 目前是占位符实现
                result->line_entry_count = 0;
                result->symbol_count = 0;
                result->success = true;
            }
            break;
            
        case Eum_DEBUG_LEVEL_FULL:
        case Eum_DEBUG_LEVEL_EXTENDED:
            // 完整/扩展调试信息：所有信息
            result->line_table = F_create_dynamic_array(sizeof(Stru_LineTableEntry_t));
            result->symbols = F_create_dynamic_array(sizeof(Stru_DebugSymbol_t));
            result->source_map = F_create_dynamic_array(sizeof(Stru_SourceMapEntry_t));
            if (result->line_table && result->symbols && result->source_map) {
                // 这里应该遍历AST生成所有调试信息
                // 目前是占位符实现
                result->line_entry_count = 0;
                result->symbol_count = 0;
                result->source_map_entry_count = 0;
                result->success = true;
            }
            break;
            
        default:
            add_error(state, "不支持的调试信息级别: %d", state->config.level);
            break;
    }
    
    // 复制错误和警告信息
    if (state->errors) {
        for (size_t i = 0; i < F_dynamic_array_length(state->errors); i++) {
            char** error_ptr = (char**)F_dynamic_array_get(state->errors, i);
            if (error_ptr && *error_ptr) {
                char* error_copy = strdup(*error_ptr);
                if (error_copy) {
                    F_dynamic_array_push(result->errors, &error_copy);
                }
            }
        }
    }
    
    if (state->warnings) {
        for (size_t i = 0; i < F_dynamic_array_length(state->warnings); i++) {
            char** warning_ptr = (char**)F_dynamic_array_get(state->warnings, i);
            if (warning_ptr && *warning_ptr) {
                char* warning_copy = strdup(*warning_ptr);
                if (warning_copy) {
                    F_dynamic_array_push(result->warnings, &warning_copy);
                }
            }
        }
    }
    
    return result;
}

/**
 * @brief 从IR生成调试信息
 */
static Stru_DebugInfoResult_t* generate_from_ir_impl(Stru_DebugInfoGeneratorInterface_t* interface,
                                                    void* ir, uint64_t code_address)
{
    // 目前与AST版本相同，实际实现中应该处理IR
    return generate_from_ast_impl(interface, NULL, code_address);
}

/**
 * @brief 生成源代码映射
 */
static Stru_DebugInfoResult_t* generate_source_map_impl(Stru_DebugInfoGeneratorInterface_t* interface,
                                                       const char* generated_code,
                                                       const char* source_code,
                                                       Stru_SourceMapEntry_t* mappings,
                                                       size_t mapping_count)
{
    if (!interface || !interface->internal_state || !generated_code || !source_code) {
        return NULL;
    }
    
    Stru_DebugInfoGeneratorState_t* state = (Stru_DebugInfoGeneratorState_t*)interface->internal_state;
    
    // 创建结果结构体
    Stru_DebugInfoResult_t* result = (Stru_DebugInfoResult_t*)cn_malloc(sizeof(Stru_DebugInfoResult_t));
    if (!result) {
        return NULL;
    }
    
    // 初始化结果
    memset(result, 0, sizeof(Stru_DebugInfoResult_t));
    result->format = Eum_DEBUG_INFO_SOURCE_MAP;
    result->success = false;
    
    // 创建错误和警告数组
    result->errors = F_create_dynamic_array(sizeof(char*));
    result->warnings = F_create_dynamic_array(sizeof(char*));
    result->source_map = F_create_dynamic_array(sizeof(Stru_SourceMapEntry_t));
    
    if (!result->errors || !result->warnings || !result->source_map) {
        if (result->errors) F_destroy_dynamic_array(result->errors);
        if (result->warnings) F_destroy_dynamic_array(result->warnings);
        if (result->source_map) F_destroy_dynamic_array(result->source_map);
        cn_free(result);
        return NULL;
    }
    
    // 复制映射条目
    if (mappings && mapping_count > 0) {
        for (size_t i = 0; i < mapping_count; i++) {
            F_dynamic_array_push(result->source_map, &mappings[i]);
        }
        result->source_map_entry_count = mapping_count;
    }
    
    // 生成源代码映射JSON
    // 这里简化处理，实际应该生成完整的Source Map v3格式
    size_t json_size = 1024 + mapping_count * 128;
    char* json = (char*)cn_malloc(json_size);
    if (json) {
        // 生成基本的Source Map结构
        snprintf(json, json_size,
            "{\n"
            "  \"version\": 3,\n"
            "  \"file\": \"generated.cn\",\n"
            "  \"sourceRoot\": \"%s\",\n"
            "  \"sources\": [\"source.cn\"],\n"
            "  \"names\": [],\n"
            "  \"mappings\": \"\"\n"
            "}",
            state->config.source_root ? state->config.source_root : "");
        
        result->debug_data = json;
        result->debug_data_size = strlen(json);
        result->success = true;
    }
    
    return result;
}

/**
 * @brief 转换调试信息格式
 */
static Stru_DebugInfoResult_t* convert_format_impl(Stru_DebugInfoGeneratorInterface_t* interface,
                                                  const Stru_DebugInfoResult_t* source_result,
                                                  Eum_DebugInfoFormat target_format)
{
    if (!interface || !interface->internal_state || !source_result) {
        return NULL;
    }
    
    Stru_DebugInfoGeneratorState_t* state = (Stru_DebugInfoGeneratorState_t*)interface->internal_state;
    
    // 检查是否支持目标格式
    if (!is_format_supported_impl(interface, target_format)) {
        add_error(state, "不支持的调试信息格式: %d", target_format);
        return NULL;
    }
    
    // 创建结果结构体
    Stru_DebugInfoResult_t* result = (Stru_DebugInfoResult_t*)cn_malloc(sizeof(Stru_DebugInfoResult_t));
    if (!result) {
        add_error(state, "内存分配失败");
        return NULL;
    }
    
    // 初始化结果
    memset(result, 0, sizeof(Stru_DebugInfoResult_t));
    result->format = target_format;
    result->success = false;
    
    // 创建错误和警告数组
    result->errors = F_create_dynamic_array(sizeof(char*));
    result->warnings = F_create_dynamic_array(sizeof(char*));
    
    if (!result->errors || !result->warnings) {
        if (result->errors) F_destroy_dynamic_array(result->errors);
        if (result->warnings) F_destroy_dynamic_array(result->warnings);
        cn_free(result);
        return NULL;
    }
    
    // 复制源结果的数据
    // 这里简化处理，实际应该进行格式转换
    result->success = source_result->success;
    
    // 复制错误和警告信息
    if (source_result->errors) {
        for (size_t i = 0; i < F_dynamic_array_length(source_result->errors); i++) {
            char** error_ptr = (char**)F_dynamic_array_get(source_result->errors, i);
            if (error_ptr && *error_ptr) {
                char* error_copy = strdup(*error_ptr);
                if (error_copy) {
                    F_dynamic_array_push(result->errors, &error_copy);
                }
            }
        }
    }
    
    if (source_result->warnings) {
        for (size_t i = 0; i < F_dynamic_array_length(source_result->warnings); i++) {
            char** warning_ptr = (char**)F_dynamic_array_get(source_result->warnings, i);
            if (warning_ptr && *warning_ptr) {
                char* warning_copy = strdup(*warning_ptr);
                if (warning_copy) {
                    F_dynamic_array_push(result->warnings, &warning_copy);
                }
            }
        }
    }
    
    return result;
}

/**
 * @brief 序列化调试信息
 */
static bool serialize_impl(Stru_DebugInfoGeneratorInterface_t* interface,
                          const Stru_DebugInfoResult_t* result,
                          uint8_t** data, size_t* size)
{
    if (!interface || !interface->internal_state || !result || !data || !size) {
        return false;
    }
    
    Stru_DebugInfoGeneratorState_t* state = (Stru_DebugInfoGeneratorState_t*)interface->internal_state;
    
    // 检查是否有调试数据
    if (!result->debug_data || result->debug_data_size == 0) {
        add_error(state, "没有调试数据可序列化");
        return false;
    }
    
    // 分配内存
    *data = (uint8_t*)cn_malloc(result->debug_data_size);
    if (!*data) {
        add_error(state, "内存分配失败");
        return false;
    }
    
    // 复制数据
    memcpy(*data, result->debug_data, result->debug_data_size);
    *size = result->debug_data_size;
    
    return true;
}

/**
 * @brief 反序列化调试信息
 */
static Stru_DebugInfoResult_t* deserialize_impl(Stru_DebugInfoGeneratorInterface_t* interface,
                                               const uint8_t* data, size_t size,
                                               Eum_DebugInfoFormat format)
{
    if (!interface || !interface->internal_state || !data || size == 0) {
        return NULL;
    }
    
    Stru_DebugInfoGeneratorState_t* state = (Stru_DebugInfoGeneratorState_t*)interface->internal_state;
    
    // 创建结果结构体
    Stru_DebugInfoResult_t* result = (Stru_DebugInfoResult_t*)cn_malloc(sizeof(Stru_DebugInfoResult_t));
    if (!result) {
        add_error(state, "内存分配失败");
        return NULL;
    }
    
    // 初始化结果
    memset(result, 0, sizeof(Stru_DebugInfoResult_t));
    result->format = format;
    result->success = false;
    
    // 创建错误和警告数组
    result->errors = F_create_dynamic_array(sizeof(char*));
    result->warnings = F_create_dynamic_array(sizeof(char*));
    
    if (!result->errors || !result->warnings) {
        if (result->errors) F_destroy_dynamic_array(result->errors);
        if (result->warnings) F_destroy_dynamic_array(result->warnings);
        cn_free(result);
        return NULL;
    }
    
    // 复制数据
    result->debug_data = (const char*)cn_malloc(size);
    if (!result->debug_data) {
        F_destroy_dynamic_array(result->errors);
        F_destroy_dynamic_array(result->warnings);
        cn_free(result);
        return NULL;
    }
    
    memcpy((void*)result->debug_data, data, size);
    result->debug_data_size = size;
    result->success = true;
    
    return result;
}

/**
 * @brief 获取支持的调试信息格式
 */
static Stru_DynamicArray_t* get_supported_formats_impl(Stru_DebugInfoGeneratorInterface_t* interface)
{
    if (!interface || !interface->internal_state) {
        return NULL;
    }
    
    // 创建格式数组
    Stru_DynamicArray_t* formats = F_create_dynamic_array(sizeof(Eum_DebugInfoFormat));
    if (!formats) {
        return NULL;
    }
    
    // 添加支持的格式
    Eum_DebugInfoFormat format;
    
    format = Eum_DEBUG_INFO_SOURCE_MAP;
    F_dynamic_array_push(formats, &format);
    
    // 可以添加更多支持的格式
    
    return formats;
}

/**
 * @brief 检查格式支持
 */
static bool is_format_supported_impl(Stru_DebugInfoGeneratorInterface_t* interface,
                                    Eum_DebugInfoFormat format)
{
    // 目前只支持源代码映射格式
    return format == Eum_DEBUG_INFO_SOURCE_MAP;
}

/**
 * @brief 获取当前配置
 */
static Stru_DebugInfoConfig_t get_current_config_impl(Stru_DebugInfoGeneratorInterface_t* interface)
{
    Stru_DebugInfoConfig_t config;
    memset(&config, 0, sizeof(config));
    
    if (!interface || !interface->internal_state) {
        return config;
    }
    
    Stru_DebugInfoGeneratorState_t* state = (Stru_DebugInfoGeneratorState_t*)interface->internal_state;
    return state->config;
}

/**
 * @brief 检查是否有错误
 */
static bool has_errors_impl(Stru_DebugInfoGeneratorInterface_t* interface)
{
    if (!interface || !interface->internal_state) {
        return false;
    }
    
    Stru_DebugInfoGeneratorState_t* state = (Stru_DebugInfoGeneratorState_t*)interface->internal_state;
    return state->errors && F_dynamic_array_length(state->errors) > 0;
}

/**
 * @brief 获取错误信息
 */
static Stru_DynamicArray_t* get_errors_impl(Stru_DebugInfoGeneratorInterface_t* interface)
{
    if (!interface || !interface->internal_state) {
        return NULL;
    }
    
    Stru_DebugInfoGeneratorState_t* state = (Stru_DebugInfoGeneratorState_t*)interface->internal_state;
    return state->errors;
}

/**
 * @brief 清除错误
 */
static void clear_errors_impl(Stru_DebugInfoGeneratorInterface_t* interface)
{
    if (!interface || !interface->internal_state) {
        return;
    }
    
    Stru_DebugInfoGeneratorState_t* state = (Stru_DebugInfoGeneratorState_t*)interface->internal_state;
    
    if (state->errors) {
        for (size_t i = 0; i < F_dynamic_array_length(state->errors); i++) {
            char** error_ptr = (char**)F_dynamic_array_get(state->errors, i);
            if (error_ptr && *error_ptr) {
                cn_free(*error_ptr);
            }
        }
        F_dynamic_array_clear(state->errors);
    }
    
    if (state->warnings) {
        for (size_t i = 0; i < F_dynamic_array_length(state->warnings); i++) {
            char** warning_ptr = (char**)F_dynamic_array_get(state->warnings, i);
            if (warning_ptr && *warning_ptr) {
                cn_free(*warning_ptr);
            }
        }
        F_dynamic_array_clear(state->warnings);
    }
}

/**
 * @brief 重置调试信息生成器
 */
static void reset_impl(Stru_DebugInfoGeneratorInterface_t* interface)
{
    if (!interface || !interface->internal_state) {
        return;
    }
    
    Stru_DebugInfoGeneratorState_t* state = (Stru_DebugInfoGeneratorState_t*)interface->internal_state;
    
    // 清除错误和警告
    clear_errors_impl(interface);
    
    // 重置配置为默认值
    state->config = F_create_default_debug_info_config();
    state->initialized = false;
}

/**
 * @brief 销毁调试信息生成器
 */
static void destroy_impl(Stru_DebugInfoGeneratorInterface_t* interface)
{
    if (!interface) {
        return;
    }
    
    // 销毁内部状态
    if (interface->internal_state) {
        destroy_state((Stru_DebugInfoGeneratorState_t*)interface->internal_state);
        interface->internal_state = NULL;
    }
    
    // 销毁接口
    cn_free(interface);
}

// ============================================================================
// 工厂函数（公共）
// ============================================================================

/**
 * @brief 创建调试信息生成器接口实例
 */
Stru_DebugInfoGeneratorInterface_t* F_create_debug_info_generator_interface(Eum_DebugInfoFormat format)
{
    return create_interface_impl(format);
}

/**
 * @brief 创建默认调试信息配置
 */
Stru_DebugInfoConfig_t F_create_default_debug_info_config(void)
{
    Stru_DebugInfoConfig_t config;
    memset(&config, 0, sizeof(config));
    
    config.format = Eum_DEBUG_INFO_SOURCE_MAP;
    config.level = Eum_DEBUG_LEVEL_BASIC;
    config.include_variable_info = true;
    config.include_type_info = false;
    config.include_source_code = false;
    config.compress_debug_info = false;
    config.source_root = NULL;
    config.output_path = NULL;
    
    return config;
}

/**
 * @brief 销毁调试信息结果
 */
void F_destroy_debug_info_result(Stru_DebugInfoResult_t* result)
{
    if (!result) {
        return;
    }
    
    // 释放调试数据
    if (result->debug_data) {
        cn_free((void*)result->debug_data);
    }
    
    // 释放符号表
    if (result->symbols) {
        for (size_t i = 0; i < F_dynamic_array_length(result->symbols); i++) {
            Stru_DebugSymbol_t* symbol = (Stru_DebugSymbol_t*)F_dynamic_array_get(result->symbols, i);
            if (symbol) {
                if (symbol->name) cn_free((void*)symbol->name);
                if (symbol->type_name) cn_free((void*)symbol->type_name);
                if (symbol->type_info) cn_free(symbol->type_info);
            }
        }
        F_destroy_dynamic_array(result->symbols);
    }
    
    // 释放行号表
    if (result->line_table) {
        for (size_t i = 0; i < F_dynamic_array_length(result->line_table); i++) {
            Stru_LineTableEntry_t* entry = (Stru_LineTableEntry_t*)F_dynamic_array_get(result->line_table, i);
            if (entry && entry->file_path) {
                cn_free((void*)entry->file_path);
            }
        }
        F_destroy_dynamic_array(result->line_table);
    }
    
    // 释放源代码映射
    if (result->source_map) {
        for (size_t i = 0; i < F_dynamic_array_length(result->source_map); i++) {
            Stru_SourceMapEntry_t* entry = (Stru_SourceMapEntry_t*)F_dynamic_array_get(result->source_map, i);
            if (entry) {
                if (entry->source_file) cn_free((void*)entry->source_file);
                if (entry->symbol_name) cn_free((void*)entry->symbol_name);
            }
        }
        F_destroy_dynamic_array(result->source_map);
    }
    
    // 释放错误信息
    if (result->errors) {
        for (size_t i = 0; i < F_dynamic_array_length(result->errors); i++) {
            char** error_ptr = (char**)F_dynamic_array_get(result->errors, i);
            if (error_ptr && *error_ptr) {
                cn_free(*error_ptr);
            }
        }
        F_destroy_dynamic_array(result->errors);
    }
    
    // 释放警告信息
    if (result->warnings) {
        for (size_t i = 0; i < F_dynamic_array_length(result->warnings); i++) {
            char** warning_ptr = (char**)F_dynamic_array_get(result->warnings, i);
            if (warning_ptr && *warning_ptr) {
                cn_free(*warning_ptr);
            }
        }
        F_destroy_dynamic_array(result->warnings);
    }
    
    cn_free(result);
}

/**
 * @brief 调试信息格式转字符串
 */
const char* F_debug_info_format_to_string(Eum_DebugInfoFormat format)
{
    switch (format) {
        case Eum_DEBUG_INFO_DWARF:      return "DWARF";
        case Eum_DEBUG_INFO_PDB:        return "PDB";
        case Eum_DEBUG_INFO_STABS:      return "STABS";
        case Eum_DEBUG_INFO_SOURCE_MAP: return "SOURCE_MAP";
        case Eum_DEBUG_INFO_CUSTOM:     return "CUSTOM";
        default:                        return "UNKNOWN";
    }
}

/**
 * @brief 调试信息级别转字符串
 */
const char* F_debug_info_level_to_string(Eum_DebugInfoLevel level)
{
    switch (level) {
        case Eum_DEBUG_LEVEL_NONE:      return "NONE";
        case Eum_DEBUG_LEVEL_MINIMAL:   return "MINIMAL";
        case Eum_DEBUG_LEVEL_BASIC:     return "BASIC";
        case Eum_DEBUG_LEVEL_FULL:      return "FULL";
        case Eum_DEBUG_LEVEL_EXTENDED:  return "EXTENDED";
        default:                        return "UNKNOWN";
    }
}
