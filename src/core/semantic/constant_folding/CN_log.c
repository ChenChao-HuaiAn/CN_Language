/**
 * @file CN_log.c
 * @brief 简单日志实现
 * 
 * 本文件实现了简单的日志功能，用于常量折叠和表达式简化模块的调试输出。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_log.h"

// 全局日志级别定义（默认为INFO级别）
CN_LogLevel g_cn_log_level = CN_LOG_LEVEL_INFO;

/**
 * @brief 设置全局日志级别
 * 
 * @param level 要设置的日志级别
 */
void CN_set_log_level(CN_LogLevel level)
{
    g_cn_log_level = level;
}

/**
 * @brief 获取当前全局日志级别
 * 
 * @return CN_LogLevel 当前日志级别
 */
CN_LogLevel CN_get_log_level(void)
{
    return g_cn_log_level;
}
