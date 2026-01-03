/******************************************************************************
 * 文件名: CN_log_internal.h
 * 功能: CN_Language日志系统内部头文件
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，定义日志系统内部结构
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_LOG_INTERNAL_H
#define CN_LOG_INTERNAL_H

#include "CN_log.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 内部常量定义
// ============================================================================

/** 默认缓冲区大小 */
#define CN_LOG_DEFAULT_BUFFER_SIZE 1024

/** 最大格式化器数量 */
#define CN_LOG_MAX_FORMATTERS 10

/** 最大输出处理器数量 */
#define CN_LOG_MAX_OUTPUTS 10

/** 异步队列默认大小 */
#define CN_LOG_DEFAULT_ASYNC_QUEUE_SIZE 1000

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief 日志消息结构
 */
typedef struct Stru_CN_LogMessage_t
{
    Eum_CN_LogLevel_t level;      /**< 日志级别 */
    long long timestamp;          /**< 时间戳（毫秒） */
    const char* file;             /**< 源文件名 */
    int line;                     /**< 源文件行号 */
    const char* function;         /**< 函数名 */
    char message[1];              /**< 可变长度消息（柔性数组） */
} Stru_CN_LogMessage_t;

/**
 * @brief 异步日志队列项
 */
typedef struct Stru_CN_AsyncLogItem_t
{
    Stru_CN_LogMessage_t* message;  /**< 日志消息 */
    size_t message_size;            /**< 消息大小 */
} Stru_CN_AsyncLogItem_t;

/**
 * @brief 异步日志队列
 */
typedef struct Stru_CN_AsyncLogQueue_t
{
    Stru_CN_AsyncLogItem_t* items;  /**< 队列项数组 */
    size_t capacity;                /**< 队列容量 */
    size_t head;                    /**< 队列头 */
    size_t tail;                    /**< 队列尾 */
    size_t count;                   /**< 当前项数 */
} Stru_CN_AsyncLogQueue_t;

/**
 * @brief 注册的格式化器
 */
typedef struct Stru_CN_RegisteredFormatter_t
{
    const char* name;                              /**< 格式化器名称 */
    const Stru_CN_LogFormatterInterface_t* impl;   /**< 格式化器实现 */
} Stru_CN_RegisteredFormatter_t;

/**
 * @brief 注册的输出处理器
 */
typedef struct Stru_CN_RegisteredOutput_t
{
    Eum_CN_LogOutputType_t type;                   /**< 输出类型 */
    const void* config;                            /**< 配置参数 */
    Stru_CN_LogOutputHandlerInterface_t* handler;  /**< 处理器实现 */
    bool initialized;                              /**< 是否已初始化 */
} Stru_CN_RegisteredOutput_t;

/**
 * @brief 日志系统内部状态
 */
typedef struct Stru_CN_LogSystemState_t
{
    // 配置
    Stru_CN_LogConfig_t config;                    /**< 当前配置 */
    
    // 格式化器
    Stru_CN_RegisteredFormatter_t formatters[CN_LOG_MAX_FORMATTERS]; /**< 注册的格式化器 */
    size_t formatter_count;                         /**< 格式化器数量 */
    const Stru_CN_LogFormatterInterface_t* current_formatter; /**< 当前格式化器 */
    
    // 输出处理器
    Stru_CN_RegisteredOutput_t outputs[CN_LOG_MAX_OUTPUTS]; /**< 注册的输出处理器 */
    size_t output_count;                          /**< 输出处理器数量 */
    
    // 异步日志
    Stru_CN_AsyncLogQueue_t async_queue;          /**< 异步队列 */
    bool async_thread_running;                    /**< 异步线程运行状态 */
    
    // 线程安全
    void* mutex;                                  /**< 互斥锁（平台相关） */
    
    // 统计信息
    size_t total_logs;                            /**< 总日志数 */
    size_t filtered_logs;                         /**< 被过滤的日志数 */
    size_t failed_writes;                         /**< 写入失败的日志数 */
    
    // 状态标志
    bool initialized;                             /**< 系统是否已初始化 */
    bool enabled;                                 /**< 日志是否启用 */
    
} Stru_CN_LogSystemState_t;

// ============================================================================
// 内部函数声明
// ============================================================================

/**
 * @brief 获取日志系统状态（单例）
 * 
 * @return 日志系统状态指针
 */
Stru_CN_LogSystemState_t* CN_log_get_state(void);

/**
 * @brief 初始化互斥锁
 * 
 * @return 初始化成功返回true，失败返回false
 */
bool CN_log_mutex_init(void);

/**
 * @brief 销毁互斥锁
 */
void CN_log_mutex_destroy(void);

/**
 * @brief 锁定互斥锁
 */
void CN_log_mutex_lock(void);

/**
 * @brief 解锁互斥锁
 */
void CN_log_mutex_unlock(void);

/**
 * @brief 获取当前时间戳（毫秒）
 * 
 * @return 当前时间戳
 */
long long CN_log_get_timestamp(void);

/**
 * @brief 格式化日志级别为字符串
 * 
 * @param level 日志级别
 * @return 级别字符串
 */
const char* CN_log_level_to_string(Eum_CN_LogLevel_t level);

/**
 * @brief 获取日志级别颜色代码（控制台）
 * 
 * @param level 日志级别
 * @return 颜色代码字符串
 */
const char* CN_log_get_level_color(Eum_CN_LogLevel_t level);

/**
 * @brief 创建日志消息
 * 
 * @param level 日志级别
 * @param file 源文件名
 * @param line 源文件行号
 * @param function 函数名
 * @param format 格式字符串
 * @param args 可变参数
 * @return 日志消息指针，失败返回NULL
 */
Stru_CN_LogMessage_t* CN_log_create_message(
    Eum_CN_LogLevel_t level,
    const char* file,
    int line,
    const char* function,
    const char* format,
    va_list args);

/**
 * @brief 释放日志消息
 * 
 * @param message 日志消息
 */
void CN_log_free_message(Stru_CN_LogMessage_t* message);

/**
 * @brief 处理日志消息（同步模式）
 * 
 * @param message 日志消息
 */
void CN_log_process_message_sync(Stru_CN_LogMessage_t* message);

/**
 * @brief 处理日志消息（异步模式）
 * 
 * @param message 日志消息
 * @return 处理成功返回true，失败返回false
 */
bool CN_log_process_message_async(Stru_CN_LogMessage_t* message);

/**
 * @brief 异步日志线程函数
 * 
 * @param arg 线程参数
 * @return 线程返回值
 */
void* CN_log_async_thread(void* arg);

/**
 * @brief 初始化异步队列
 * 
 * @param queue 队列指针
 * @param capacity 队列容量
 * @return 初始化成功返回true，失败返回false
 */
bool CN_log_async_queue_init(Stru_CN_AsyncLogQueue_t* queue, size_t capacity);

/**
 * @brief 销毁异步队列
 * 
 * @param queue 队列指针
 */
void CN_log_async_queue_destroy(Stru_CN_AsyncLogQueue_t* queue);

/**
 * @brief 向异步队列添加消息
 * 
 * @param queue 队列指针
 * @param message 日志消息
 * @param message_size 消息大小
 * @return 添加成功返回true，失败返回false
 */
bool CN_log_async_queue_push(Stru_CN_AsyncLogQueue_t* queue,
                            Stru_CN_LogMessage_t* message,
                            size_t message_size);

/**
 * @brief 从异步队列获取消息
 * 
 * @param queue 队列指针
 * @param item 输出参数：队列项
 * @return 获取成功返回true，队列为空返回false
 */
bool CN_log_async_queue_pop(Stru_CN_AsyncLogQueue_t* queue,
                           Stru_CN_AsyncLogItem_t* item);

/**
 * @brief 默认格式化器实现
 */
extern const Stru_CN_LogFormatterInterface_t CN_log_default_formatter;

/**
 * @brief 简单格式化器实现
 */
extern const Stru_CN_LogFormatterInterface_t CN_log_simple_formatter;

/**
 * @brief JSON格式化器实现
 */
extern const Stru_CN_LogFormatterInterface_t CN_log_json_formatter;

/**
 * @brief 控制台输出处理器实现
 */
extern const Stru_CN_LogOutputHandlerInterface_t CN_log_console_output_handler;

/**
 * @brief 文件输出处理器实现
 */
extern const Stru_CN_LogOutputHandlerInterface_t CN_log_file_output_handler;

#ifdef __cplusplus
}
#endif

#endif // CN_LOG_INTERNAL_H
