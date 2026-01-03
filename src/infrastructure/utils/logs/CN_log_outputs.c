/******************************************************************************
 * 文件名: CN_log_outputs.c
 * 功能: CN_Language日志系统输出处理器实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现日志输出处理器
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_log_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 控制台输出配置
// ============================================================================

typedef struct Stru_CN_ConsoleConfig_t
{
    bool use_stderr_for_errors;  /**< 错误级别日志是否输出到stderr */
    bool enable_colors;          /**< 是否启用颜色输出 */
} Stru_CN_ConsoleConfig_t;

// ============================================================================
// 文件输出配置
// ============================================================================

typedef struct Stru_CN_FileConfig_t
{
    const char* filename;        /**< 文件名 */
    const char* mode;            /**< 打开模式 ("a"=追加, "w"=覆盖) */
    size_t max_size;             /**< 最大文件大小（字节，0=无限制） */
    size_t max_files;            /**< 最大文件数（日志轮转） */
} Stru_CN_FileConfig_t;

// ============================================================================
// 控制台输出处理器实现
// ============================================================================

static bool console_output_initialize(const void* config)
{
    // 控制台输出不需要特殊初始化
    return true;
}

static bool console_output_write(const char* message, size_t length, Eum_CN_LogLevel_t level)
{
    if (!message || length == 0)
        return false;
    
    // 默认配置
    static Stru_CN_ConsoleConfig_t default_config = {
        .use_stderr_for_errors = true,
        .enable_colors = true
    };
    
    // 根据日志级别选择输出流
    FILE* stream = stdout;
    if (default_config.use_stderr_for_errors && 
        (level >= Eum_LOG_LEVEL_ERROR || level == Eum_LOG_LEVEL_FATAL))
    {
        stream = stderr;
    }
    
    // 写入消息
    size_t written = fwrite(message, 1, length, stream);
    
    // 立即刷新，确保日志及时显示
    fflush(stream);
    
    return written == length;
}

static bool console_output_flush(void)
{
    fflush(stdout);
    fflush(stderr);
    return true;
}

static void console_output_shutdown(void)
{
    // 控制台输出不需要特殊清理
}

static Eum_CN_LogOutputType_t console_output_get_type(void)
{
    return Eum_LOG_OUTPUT_CONSOLE;
}

const Stru_CN_LogOutputHandlerInterface_t CN_log_console_output_handler = {
    .initialize = console_output_initialize,
    .write = console_output_write,
    .flush = console_output_flush,
    .shutdown = console_output_shutdown,
    .get_type = console_output_get_type
};

// ============================================================================
// 文件输出处理器实现
// ============================================================================

typedef struct Stru_CN_FileOutputState_t
{
    FILE* file;                  /**< 文件指针 */
    Stru_CN_FileConfig_t config; /**< 配置 */
    size_t current_size;         /**< 当前文件大小 */
    char current_filename[256];  /**< 当前文件名 */
} Stru_CN_FileOutputState_t;

static Stru_CN_FileOutputState_t g_file_state = {0};

static bool file_output_initialize(const void* config)
{
    if (!config)
        return false;
    
    const Stru_CN_FileConfig_t* file_config = (const Stru_CN_FileConfig_t*)config;
    if (!file_config->filename || file_config->filename[0] == '\0')
        return false;
    
    // 复制配置
    g_file_state.config = *file_config;
    strncpy(g_file_state.current_filename, file_config->filename, 
            sizeof(g_file_state.current_filename) - 1);
    g_file_state.current_filename[sizeof(g_file_state.current_filename) - 1] = '\0';
    
    // 打开文件
    const char* mode = file_config->mode ? file_config->mode : "a";
    g_file_state.file = fopen(g_file_state.current_filename, mode);
    if (!g_file_state.file)
        return false;
    
    // 获取当前文件大小
    fseek(g_file_state.file, 0, SEEK_END);
    g_file_state.current_size = ftell(g_file_state.file);
    fseek(g_file_state.file, 0, SEEK_SET);
    
    return true;
}

/**
 * @brief 执行日志轮转
 * 
 * @return 轮转成功返回true，失败返回false
 */
static bool perform_log_rotation(void)
{
    if (g_file_state.config.max_size == 0 || g_file_state.config.max_files == 0)
        return true; // 不需要轮转
    
    if (g_file_state.current_size < g_file_state.config.max_size)
        return true; // 文件大小未超过限制
    
    // 关闭当前文件
    if (g_file_state.file)
    {
        fclose(g_file_state.file);
        g_file_state.file = NULL;
    }
    
    // 重命名旧文件
    for (int i = g_file_state.config.max_files - 1; i > 0; i--)
    {
        char old_name[256];
        char new_name[256];
        
        if (i == 1)
        {
            snprintf(old_name, sizeof(old_name), "%s", g_file_state.current_filename);
        }
        else
        {
            snprintf(old_name, sizeof(old_name), "%s.%d", g_file_state.current_filename, i - 1);
        }
        
        snprintf(new_name, sizeof(new_name), "%s.%d", g_file_state.current_filename, i);
        
        // 重命名文件
        rename(old_name, new_name);
    }
    
    // 重新打开文件
    const char* mode = g_file_state.config.mode ? g_file_state.config.mode : "a";
    g_file_state.file = fopen(g_file_state.current_filename, mode);
    if (!g_file_state.file)
        return false;
    
    g_file_state.current_size = 0;
    
    return true;
}

static bool file_output_write(const char* message, size_t length, Eum_CN_LogLevel_t level)
{
    if (!g_file_state.file || !message || length == 0)
        return false;
    
    // 检查是否需要日志轮转
    if (!perform_log_rotation())
        return false;
    
    // 写入文件
    size_t written = fwrite(message, 1, length, g_file_state.file);
    if (written == length)
    {
        g_file_state.current_size += written;
        return true;
    }
    
    return false;
}

static bool file_output_flush(void)
{
    if (!g_file_state.file)
        return false;
    
    return fflush(g_file_state.file) == 0;
}

static void file_output_shutdown(void)
{
    if (g_file_state.file)
    {
        fclose(g_file_state.file);
        g_file_state.file = NULL;
    }
    
    memset(&g_file_state, 0, sizeof(g_file_state));
}

static Eum_CN_LogOutputType_t file_output_get_type(void)
{
    return Eum_LOG_OUTPUT_FILE;
}

const Stru_CN_LogOutputHandlerInterface_t CN_log_file_output_handler = {
    .initialize = file_output_initialize,
    .write = file_output_write,
    .flush = file_output_flush,
    .shutdown = file_output_shutdown,
    .get_type = file_output_get_type
};

// ============================================================================
// 网络输出处理器实现（简化版）
// ============================================================================

typedef struct Stru_CN_NetworkConfig_t
{
    const char* host;            /**< 主机名或IP地址 */
    int port;                    /**< 端口号 */
    int protocol;                /**< 协议类型（0=TCP, 1=UDP） */
} Stru_CN_NetworkConfig_t;

static bool network_output_initialize(const void* config)
{
    // 网络输出需要平台特定的网络代码
    // 这里返回false表示不支持
    return false;
}

static bool network_output_write(const char* message, size_t length, Eum_CN_LogLevel_t level)
{
    // 网络输出需要平台特定的网络代码
    return false;
}

static bool network_output_flush(void)
{
    // 网络输出不需要刷新
    return true;
}

static void network_output_shutdown(void)
{
    // 网络输出不需要特殊清理
}

static Eum_CN_LogOutputType_t network_output_get_type(void)
{
    return Eum_LOG_OUTPUT_NETWORK;
}

// ============================================================================
// 系统日志输出处理器实现（简化版）
// ============================================================================

static bool syslog_output_initialize(const void* config)
{
    // 系统日志需要平台特定的syslog代码
    // 这里返回false表示不支持
    return false;
}

static bool syslog_output_write(const char* message, size_t length, Eum_CN_LogLevel_t level)
{
    // 系统日志需要平台特定的syslog代码
    return false;
}

static bool syslog_output_flush(void)
{
    // 系统日志不需要刷新
    return true;
}

static void syslog_output_shutdown(void)
{
    // 系统日志不需要特殊清理
}

static Eum_CN_LogOutputType_t syslog_output_get_type(void)
{
    return Eum_LOG_OUTPUT_SYSLOG;
}
