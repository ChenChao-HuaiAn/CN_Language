/**
 * @file CN_log.h
 * @brief 简单日志宏定义
 * 
 * 本文件定义了简单的日志宏，用于常量折叠和表达式简化模块的调试输出。
 * 遵循项目规范，避免直接使用printf。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_LOG_H
#define CN_LOG_H

#include <stdio.h>

// 日志级别定义
typedef enum
{
    CN_LOG_LEVEL_DEBUG = 0,
    CN_LOG_LEVEL_INFO = 1,
    CN_LOG_LEVEL_WARNING = 2,
    CN_LOG_LEVEL_ERROR = 3,
    CN_LOG_LEVEL_NONE = 4
} CN_LogLevel;

// 全局日志级别（默认为INFO）
extern CN_LogLevel g_cn_log_level;

// 函数声明
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 设置全局日志级别
 * 
 * @param level 要设置的日志级别
 */
void CN_set_log_level(CN_LogLevel level);

/**
 * @brief 获取当前全局日志级别
 * 
 * @return CN_LogLevel 当前日志级别
 */
CN_LogLevel CN_get_log_level(void);

#ifdef __cplusplus
}
#endif

// 日志宏定义
#define CN_LOG(level, format, ...) \
    do { \
        if (level >= g_cn_log_level) { \
            const char* level_str = "UNKNOWN"; \
            switch (level) { \
                case CN_LOG_LEVEL_DEBUG: level_str = "DEBUG"; break; \
                case CN_LOG_LEVEL_INFO: level_str = "INFO"; break; \
                case CN_LOG_LEVEL_WARNING: level_str = "WARNING"; break; \
                case CN_LOG_LEVEL_ERROR: level_str = "ERROR"; break; \
                default: break; \
            } \
            fprintf(stderr, "[%s] %s:%d: " format "\n", \
                    level_str, __FILE__, __LINE__, ##__VA_ARGS__); \
        } \
    } while (0)

// 便捷日志宏
#define CN_LOG_DEBUG(format, ...) CN_LOG(CN_LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)
#define CN_LOG_INFO(format, ...) CN_LOG(CN_LOG_LEVEL_INFO, format, ##__VA_ARGS__)
#define CN_LOG_WARNING(format, ...) CN_LOG(CN_LOG_LEVEL_WARNING, format, ##__VA_ARGS__)
#define CN_LOG_ERROR(format, ...) CN_LOG(CN_LOG_LEVEL_ERROR, format, ##__VA_ARGS__)

#endif /* CN_LOG_H */
