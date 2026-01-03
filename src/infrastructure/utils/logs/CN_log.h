/******************************************************************************
 * 文件名: CN_log.h
 * 功能: CN_Language统一日志系统接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，定义统一日志接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_LOG_H
#define CN_LOG_H

#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file CN_log.h
 * @brief CN_Language统一日志系统接口
 * 
 * 此文件提供了CN_Language项目的统一日志接口。
 * 支持多输出目标、日志级别过滤、格式化输出和线程安全。
 * 
 * 使用示例：
 * @code
 * #include "CN_log.h"
 * 
 * // 初始化日志系统
 * CN_log_init(Eum_LOG_LEVEL_INFO, true);
 * 
 * // 添加控制台输出
 * CN_log_add_output(Eum_LOG_OUTPUT_CONSOLE, NULL);
 * 
 * // 添加文件输出
 * CN_log_add_output(Eum_LOG_OUTPUT_FILE, "logs/app.log");
 * 
 * // 记录日志
 * CN_log(Eum_LOG_LEVEL_INFO, "应用程序启动");
 * CN_log(Eum_LOG_LEVEL_DEBUG, "用户 %s 登录，ID: %d", username, user_id);
 * 
 * // 关闭日志系统
 * CN_log_shutdown();
 * @endcode
 */

// ============================================================================
// 日志级别定义
// ============================================================================

/**
 * @brief 日志级别枚举
 * 
 * 定义不同严重程度的日志级别，从最详细到最严重。
 */
typedef enum Eum_CN_LogLevel_t
{
    Eum_LOG_LEVEL_TRACE = 0,    /**< 跟踪信息：最详细的调试信息 */
    Eum_LOG_LEVEL_DEBUG = 1,    /**< 调试信息：开发调试用 */
    Eum_LOG_LEVEL_INFO  = 2,    /**< 一般信息：正常操作信息 */
    Eum_LOG_LEVEL_WARN  = 3,    /**< 警告信息：可能有问题但程序可继续 */
    Eum_LOG_LEVEL_ERROR = 4,    /**< 错误信息：功能受影响但程序可继续 */
    Eum_LOG_LEVEL_FATAL = 5     /**< 致命错误：程序无法继续运行 */
} Eum_CN_LogLevel_t;

// ============================================================================
// 日志输出目标定义
// ============================================================================

/**
 * @brief 日志输出目标类型枚举
 */
typedef enum Eum_CN_LogOutputType_t
{
    Eum_LOG_OUTPUT_CONSOLE = 0,  /**< 控制台输出（stdout/stderr） */
    Eum_LOG_OUTPUT_FILE    = 1,  /**< 文件输出 */
    Eum_LOG_OUTPUT_NETWORK = 2,  /**< 网络输出（TCP/UDP） */
    Eum_LOG_OUTPUT_SYSLOG  = 3,  /**< 系统日志（syslog） */
    Eum_LOG_OUTPUT_CUSTOM  = 4   /**< 自定义输出处理器 */
} Eum_CN_LogOutputType_t;

// ============================================================================
// 日志格式化器接口
// ============================================================================

/**
 * @brief 日志格式化器接口
 * 
 * 定义日志消息格式化接口，支持自定义格式化策略。
 */
typedef struct Stru_CN_LogFormatterInterface_t
{
    /**
     * @brief 格式化日志消息
     * 
     * @param buffer 输出缓冲区
     * @param buffer_size 缓冲区大小
     * @param level 日志级别
     * @param timestamp 时间戳（毫秒）
     * @param file 源文件名
     * @param line 源文件行号
     * @param function 函数名
     * @param format 格式字符串
     * @param args 可变参数列表
     * @return 格式化后的字符串长度（不包括终止符）
     */
    size_t (*format)(char* buffer, size_t buffer_size,
                     Eum_CN_LogLevel_t level,
                     long long timestamp,
                     const char* file,
                     int line,
                     const char* function,
                     const char* format,
                     va_list args);
    
    /**
     * @brief 获取格式化器名称
     * 
     * @return 格式化器名称
     */
    const char* (*get_name)(void);
    
} Stru_CN_LogFormatterInterface_t;

// ============================================================================
// 日志输出处理器接口
// ============================================================================

/**
 * @brief 日志输出处理器接口
 * 
 * 定义日志输出处理器接口，支持多种输出目标。
 */
typedef struct Stru_CN_LogOutputHandlerInterface_t
{
    /**
     * @brief 初始化输出处理器
     * 
     * @param config 配置参数（类型特定）
     * @return 初始化成功返回true，失败返回false
     */
    bool (*initialize)(const void* config);
    
    /**
     * @brief 输出日志消息
     * 
     * @param message 格式化后的日志消息
     * @param length 消息长度
     * @param level 日志级别
     * @return 输出成功返回true，失败返回false
     */
    bool (*write)(const char* message, size_t length, Eum_CN_LogLevel_t level);
    
    /**
     * @brief 刷新输出缓冲区
     * 
     * @return 刷新成功返回true，失败返回false
     */
    bool (*flush)(void);
    
    /**
     * @brief 关闭输出处理器
     */
    void (*shutdown)(void);
    
    /**
     * @brief 获取处理器类型
     * 
     * @return 处理器类型
     */
    Eum_CN_LogOutputType_t (*get_type)(void);
    
} Stru_CN_LogOutputHandlerInterface_t;

// ============================================================================
// 日志系统配置
// ============================================================================

/**
 * @brief 日志系统配置结构
 */
typedef struct Stru_CN_LogConfig_t
{
    /** 默认日志级别（低于此级别的日志将被过滤） */
    Eum_CN_LogLevel_t default_level;
    
    /** 是否启用线程安全 */
    bool thread_safe;
    
    /** 是否启用异步日志（提高性能） */
    bool async_mode;
    
    /** 异步模式下的队列大小（0表示同步模式） */
    size_t async_queue_size;
    
    /** 是否启用颜色输出（控制台） */
    bool enable_colors;
    
    /** 是否显示时间戳 */
    bool show_timestamp;
    
    /** 是否显示源文件位置 */
    bool show_source_location;
    
    /** 是否显示线程ID */
    bool show_thread_id;
    
    /** 默认格式化器名称 */
    const char* default_formatter;
    
} Stru_CN_LogConfig_t;

// ============================================================================
// 日志系统核心函数
// ============================================================================

/**
 * @brief 初始化日志系统
 * 
 * @param config 日志系统配置（可为NULL，使用默认配置）
 * @return 初始化成功返回true，失败返回false
 */
bool CN_log_init(const Stru_CN_LogConfig_t* config);

/**
 * @brief 关闭日志系统
 * 
 * 释放所有资源，刷新所有输出。
 */
void CN_log_shutdown(void);

/**
 * @brief 记录日志（核心函数）
 * 
 * @param level 日志级别
 * @param file 源文件名（使用__FILE__）
 * @param line 源文件行号（使用__LINE__）
 * @param function 函数名（使用__func__）
 * @param format 格式字符串
 * @param ... 可变参数
 */
void CN_log_core(Eum_CN_LogLevel_t level,
                 const char* file,
                 int line,
                 const char* function,
                 const char* format,
                 ...);

/**
 * @brief 记录日志（简化接口）
 * 
 * @param level 日志级别
 * @param format 格式字符串
 * @param ... 可变参数
 */
void CN_log(Eum_CN_LogLevel_t level, const char* format, ...);

// ============================================================================
// 输出管理函数
// ============================================================================

/**
 * @brief 添加日志输出处理器
 * 
 * @param type 输出类型
 * @param config 配置参数（类型特定）
 * @return 添加成功返回true，失败返回false
 */
bool CN_log_add_output(Eum_CN_LogOutputType_t type, const void* config);

/**
 * @brief 移除日志输出处理器
 * 
 * @param type 输出类型
 * @return 移除成功返回true，失败返回false
 */
bool CN_log_remove_output(Eum_CN_LogOutputType_t type);

/**
 * @brief 移除所有输出处理器
 */
void CN_log_remove_all_outputs(void);

/**
 * @brief 设置输出处理器
 * 
 * @param type 输出类型
 * @param handler 处理器接口
 * @return 设置成功返回true，失败返回false
 */
bool CN_log_set_output_handler(Eum_CN_LogOutputType_t type,
                               const Stru_CN_LogOutputHandlerInterface_t* handler);

// ============================================================================
// 格式化器管理函数
// ============================================================================

/**
 * @brief 注册日志格式化器
 * 
 * @param name 格式化器名称
 * @param formatter 格式化器接口
 * @return 注册成功返回true，失败返回false
 */
bool CN_log_register_formatter(const char* name,
                               const Stru_CN_LogFormatterInterface_t* formatter);

/**
 * @brief 设置当前格式化器
 * 
 * @param name 格式化器名称
 * @return 设置成功返回true，失败返回false
 */
bool CN_log_set_formatter(const char* name);

// ============================================================================
// 配置管理函数
// ============================================================================

/**
 * @brief 获取当前配置
 * 
 * @return 当前配置
 */
Stru_CN_LogConfig_t CN_log_get_config(void);

/**
 * @brief 更新配置
 * 
 * @param config 新配置
 * @return 更新成功返回true，失败返回false
 */
bool CN_log_set_config(const Stru_CN_LogConfig_t* config);

/**
 * @brief 设置日志级别
 * 
 * @param level 新的日志级别
 */
void CN_log_set_level(Eum_CN_LogLevel_t level);

/**
 * @brief 获取当前日志级别
 * 
 * @return 当前日志级别
 */
Eum_CN_LogLevel_t CN_log_get_level(void);

// ============================================================================
// 便捷日志宏
// ============================================================================

/**
 * @def CN_LOG_TRACE(format, ...)
 * @brief 记录跟踪级别日志
 */
#define CN_LOG_TRACE(format, ...) \
    CN_log_core(Eum_LOG_LEVEL_TRACE, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/**
 * @def CN_LOG_DEBUG(format, ...)
 * @brief 记录调试级别日志
 */
#define CN_LOG_DEBUG(format, ...) \
    CN_log_core(Eum_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/**
 * @def CN_LOG_INFO(format, ...)
 * @brief 记录信息级别日志
 */
#define CN_LOG_INFO(format, ...) \
    CN_log_core(Eum_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/**
 * @def CN_LOG_WARN(format, ...)
 * @brief 记录警告级别日志
 */
#define CN_LOG_WARN(format, ...) \
    CN_log_core(Eum_LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/**
 * @def CN_LOG_ERROR(format, ...)
 * @brief 记录错误级别日志
 */
#define CN_LOG_ERROR(format, ...) \
    CN_log_core(Eum_LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/**
 * @def CN_LOG_FATAL(format, ...)
 * @brief 记录致命级别日志
 */
#define CN_LOG_FATAL(format, ...) \
    CN_log_core(Eum_LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

// ============================================================================
// 性能优化函数
// ============================================================================

/**
 * @brief 启用/禁用日志
 * 
 * @param enabled true启用日志，false禁用日志
 */
void CN_log_set_enabled(bool enabled);

/**
 * @brief 检查日志是否启用
 * 
 * @return 日志启用状态
 */
bool CN_log_is_enabled(void);

/**
 * @brief 刷新所有输出
 * 
 * 强制刷新所有输出处理器的缓冲区。
 */
void CN_log_flush_all(void);

/**
 * @brief 获取日志系统统计信息
 * 
 * @param total_logs 总日志数（输出参数）
 * @param filtered_logs 被过滤的日志数（输出参数）
 * @param failed_writes 写入失败的日志数（输出参数）
 */
void CN_log_get_stats(size_t* total_logs, size_t* filtered_logs, size_t* failed_writes);

#ifdef __cplusplus
}
#endif

#endif // CN_LOG_H
