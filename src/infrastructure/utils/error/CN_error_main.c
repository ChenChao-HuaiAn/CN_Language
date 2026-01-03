/******************************************************************************
 * 文件名: CN_error_main.c
 * 功能: CN_Language错误处理框架主实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现错误处理框架主功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ==================== 全局状态 ==================== */

/** 全局错误处理回调函数 */
static void (*g_error_callback)(const Stru_CN_ErrorContext_t* context, void* user_data) = NULL;

/** 全局错误处理回调用户数据 */
static void* g_error_callback_user_data = NULL;

/** 最后一个报告的错误 */
static Stru_CN_ErrorContext_t g_last_reported_error;

/** 错误统计 */
static struct
{
    size_t total_errors;
    size_t fatal_errors;
    size_t recoverable_errors;
    size_t unrecoverable_errors;
} g_error_stats = {0, 0, 0, 0};

/** 框架配置 */
static Stru_CN_ErrorConfig_t g_config = {
    .verbose_logging = false,
    .max_chain_length = 100,
    .auto_report_to_stderr = true,
    .enable_statistics = true,
    .report_format = "[%t] [线程: %i] [%f:%l] 错误 %c: %m"
};

/** 框架是否已初始化 */
static bool g_initialized = false;

/* ==================== 辅助函数 ==================== */

/**
 * @brief 获取错误严重性级别
 */
int CN_error_get_severity(Eum_CN_ErrorCode_t error_code)
{
    // 根据错误码范围确定严重性
    if (error_code == Eum_CN_ERROR_SUCCESS)
        return 0;
    
    // 通用错误：1-5
    if (error_code <= Eum_CN_ERROR_UNKNOWN)
    {
        switch (error_code)
        {
            case Eum_CN_ERROR_OUT_OF_MEMORY:
            case Eum_CN_ERROR_INTERNAL:
            case Eum_CN_ERROR_CORRUPTED_DATA:
                return 9;
            case Eum_CN_ERROR_NULL_POINTER:
            case Eum_CN_ERROR_OUT_OF_BOUNDS:
            case Eum_CN_ERROR_SYSTEM:
                return 8;
            case Eum_CN_ERROR_INVALID_ARGUMENT:
            case Eum_CN_ERROR_FORMAT:
            case Eum_CN_ERROR_IO:
            case Eum_CN_ERROR_NETWORK:
                return 7;
            case Eum_CN_ERROR_NOT_SUPPORTED:
            case Eum_CN_ERROR_TIMEOUT:
            case Eum_CN_ERROR_BUSY:
            case Eum_CN_ERROR_NOT_FOUND:
                return 6;
            case Eum_CN_ERROR_ALREADY_EXISTS:
            case Eum_CN_ERROR_PERMISSION_DENIED:
            case Eum_CN_ERROR_CHECKSUM:
            case Eum_CN_ERROR_VERSION_MISMATCH:
                return 5;
            case Eum_CN_ERROR_CONFIGURATION:
            case Eum_CN_ERROR_NOT_IMPLEMENTED:
            case Eum_CN_ERROR_CANCELLED:
                return 4;
            case Eum_CN_ERROR_BUFFER_TOO_SMALL:
            case Eum_CN_ERROR_INVALID_STATE:
                return 3;
            case Eum_CN_ERROR_FAILURE:
            case Eum_CN_ERROR_UNKNOWN:
                return 2;
            default:
                return 1;
        }
    }
    
    // 基础设施层错误：6-8
    if (error_code <= Eum_CN_ERROR_NETWORK_BUFFER_FULL)
    {
        // 内存错误通常更严重
        if (error_code >= Eum_CN_ERROR_MEMORY_BASE && 
            error_code <= Eum_CN_ERROR_MEMORY_LEAK)
            return 8;
        
        // 数学错误
        if (error_code >= Eum_CN_ERROR_MATH_BASE && 
            error_code <= Eum_CN_ERROR_MATH_INFINITY)
            return 7;
        
        // 文件系统错误
        if (error_code >= Eum_CN_ERROR_FILESYSTEM_BASE && 
            error_code <= Eum_CN_ERROR_FILE_LOCKED)
            return 6;
        
        return 5;
    }
    
    // 核心层错误：7-9
    if (error_code <= Eum_CN_ERROR_RUNTIME_RESOURCE_LEAK)
    {
        // 运行时错误最严重
        if (error_code >= Eum_CN_ERROR_RUNTIME_BASE && 
            error_code <= Eum_CN_ERROR_RUNTIME_RESOURCE_LEAK)
            return 9;
        
        // 词法/语法错误
        if (error_code >= Eum_CN_ERROR_LEXER_BASE && 
            error_code <= Eum_CN_ERROR_PARSER_MISSING_TOKEN)
            return 7;
        
        // 语义/代码生成错误
        if (error_code >= Eum_CN_ERROR_SEMANTIC_BASE && 
            error_code <= Eum_CN_ERROR_CODEGEN_OPTIMIZATION_FAILED)
            return 8;
        
        return 6;
    }
    
    // 应用层错误：4-6
    if (error_code <= Eum_CN_ERROR_IDE_PLUGIN_VERSION_MISMATCH)
    {
        return 5;
    }
    
    // 用户自定义错误：1-3
    if (error_code >= Eum_CN_ERROR_USER_BASE && 
        error_code <= Eum_CN_ERROR_USER_6)
    {
        return 3;
    }
    
    return 1; // 默认
}

/**
 * @brief 检查错误是否可恢复
 */
bool CN_error_is_recoverable(Eum_CN_ErrorCode_t error_code)
{
    int severity = CN_error_get_severity(error_code);
    
    // 严重性低于5的错误通常可恢复
    if (severity < 5)
        return true;
    
    // 特定不可恢复的错误
    switch (error_code)
    {
        case Eum_CN_ERROR_OUT_OF_MEMORY:
        case Eum_CN_ERROR_INTERNAL:
        case Eum_CN_ERROR_CORRUPTED_DATA:
        case Eum_CN_ERROR_MEMORY_DOUBLE_FREE:
        case Eum_CN_ERROR_MEMORY_LEAK:
        case Eum_CN_ERROR_RUNTIME_STACK_OVERFLOW:
        case Eum_CN_ERROR_RUNTIME_HEAP_OVERFLOW:
        case Eum_CN_ERROR_RUNTIME_DEADLOCK:
            return false;
        default:
            return severity < 8; // 严重性8及以上不可恢复
    }
}

/**
 * @brief 报告错误到标准错误输出
 */
static void report_to_stderr(const Stru_CN_ErrorContext_t* context)
{
    if (!context || !g_config.auto_report_to_stderr)
        return;
    
    char buffer[1024];
    size_t written = CN_error_format_context(context, buffer, sizeof(buffer));
    
    if (written > 0)
    {
        fprintf(stderr, "%s\n", buffer);
    }
}

/**
 * @brief 更新错误统计
 */
static void update_error_stats(const Stru_CN_ErrorContext_t* context)
{
    if (!g_config.enable_statistics)
        return;
    
    g_error_stats.total_errors++;
    
    if (context->recoverable)
    {
        g_error_stats.recoverable_errors++;
    }
    else
    {
        g_error_stats.unrecoverable_errors++;
    }
    
    int severity = CN_error_get_severity(context->error_code);
    if (severity >= 8)
    {
        g_error_stats.fatal_errors++;
    }
}

/* ==================== 公共API实现 ==================== */

/**
 * @brief 错误处理框架初始化
 */
bool CN_error_framework_init(void)
{
    if (g_initialized)
        return true;
    
    // 初始化最后一个报告的错误
    CN_error_clear_context(&g_last_reported_error);
    
    // 重置统计
    memset(&g_error_stats, 0, sizeof(g_error_stats));
    
    g_initialized = true;
    return true;
}

/**
 * @brief 错误处理框架清理
 */
void CN_error_framework_cleanup(void)
{
    if (!g_initialized)
        return;
    
    // 清空最后一个报告的错误
    CN_error_clear_context(&g_last_reported_error);
    
    // 重置回调
    g_error_callback = NULL;
    g_error_callback_user_data = NULL;
    
    g_initialized = false;
}

/**
 * @brief 设置全局错误处理回调
 */
void CN_error_set_global_handler(
    void (*callback)(const Stru_CN_ErrorContext_t* context, void* user_data),
    void* user_data)
{
    g_error_callback = callback;
    g_error_callback_user_data = user_data;
}

/**
 * @brief 报告错误到全局处理器
 */
void CN_error_report(const Stru_CN_ErrorContext_t* context)
{
    if (!context || !CN_error_is_context_valid(context))
        return;
    
    // 确保框架已初始化
    if (!g_initialized)
    {
        CN_error_framework_init();
    }
    
    // 更新最后一个报告的错误
    CN_error_copy_context(&g_last_reported_error, context);
    
    // 更新统计
    update_error_stats(context);
    
    // 报告到标准错误输出
    report_to_stderr(context);
    
    // 调用全局回调
    if (g_error_callback)
    {
        g_error_callback(context, g_error_callback_user_data);
    }
}

/**
 * @brief 快速报告错误（简化接口）
 */
void CN_error_report_quick(
    Eum_CN_ErrorCode_t error_code,
    const char* message,
    const char* filename,
    size_t line)
{
    Stru_CN_ErrorContext_t context = CN_error_create_position_context(
        error_code, message, filename, line, 0);
    
    CN_error_report(&context);
}

/**
 * @brief 获取最后一个报告的错误
 */
const Stru_CN_ErrorContext_t* CN_error_get_last_reported(void)
{
    if (g_last_reported_error.error_code == Eum_CN_ERROR_SUCCESS)
        return NULL;
    
    return &g_last_reported_error;
}

/**
 * @brief 清除所有报告的错误
 */
void CN_error_clear_all_reported(void)
{
    CN_error_clear_context(&g_last_reported_error);
    memset(&g_error_stats, 0, sizeof(g_error_stats));
}

/**
 * @brief 检查是否有错误发生
 */
bool CN_error_has_occurred(void)
{
    return g_last_reported_error.error_code != Eum_CN_ERROR_SUCCESS;
}

/**
 * @brief 错误处理框架版本信息
 */
const char* CN_error_framework_version(void)
{
    return "CN_Language错误处理框架 v1.0.0";
}

/**
 * @brief 获取当前配置
 */
Stru_CN_ErrorConfig_t CN_error_get_config(void)
{
    return g_config;
}

/**
 * @brief 设置配置
 */
void CN_error_set_config(const Stru_CN_ErrorConfig_t* config)
{
    if (!config)
        return;
    
    g_config = *config;
}

/**
 * @brief 获取错误统计信息
 */
void CN_error_get_statistics(
    size_t* total_errors,
    size_t* fatal_errors,
    size_t* recoverable_errors,
    size_t* unrecoverable_errors)
{
    if (total_errors)
        *total_errors = g_error_stats.total_errors;
    
    if (fatal_errors)
        *fatal_errors = g_error_stats.fatal_errors;
    
    if (recoverable_errors)
        *recoverable_errors = g_error_stats.recoverable_errors;
    
    if (unrecoverable_errors)
        *unrecoverable_errors = g_error_stats.unrecoverable_errors;
}
