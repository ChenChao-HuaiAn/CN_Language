/******************************************************************************
 * 文件名: CN_error.h
 * 功能: CN_Language错误处理框架主头文件
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，统一错误处理接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_ERROR_H
#define CN_ERROR_H

/**
 * @file CN_error.h
 * @brief CN_Language错误处理框架主头文件
 * 
 * 此文件提供了CN_Language项目的统一错误处理接口。
 * 包含错误码定义、错误上下文传递和错误链支持。
 * 
 * 使用示例：
 * @code
 * #include "CN_error.h"
 * 
 * // 创建错误上下文
 * Stru_CN_ErrorContext_t ctx = CN_error_create_simple_context(
 *     Eum_CN_ERROR_INVALID_ARGUMENT,
 *     "参数无效");
 * 
 * // 创建错误链
 * Stru_CN_ErrorChain_t chain = CN_error_chain_create(10, true);
 * 
 * // 添加错误到链
 * CN_error_chain_add(&chain, &ctx);
 * 
 * // 处理错误链
 * // ...
 * 
 * // 销毁错误链
 * CN_error_chain_destroy(&chain);
 * @endcode
 */

#include "CN_error_codes.h"
#include "CN_error_context.h"
#include "CN_error_chain.h"

/**
 * @brief 错误处理框架初始化
 * 
 * 初始化错误处理框架，设置默认配置。
 * 此函数应在程序开始时调用。
 * 
 * @return true 如果初始化成功，false 如果失败
 */
bool CN_error_framework_init(void);

/**
 * @brief 错误处理框架清理
 * 
 * 清理错误处理框架，释放所有资源。
 * 此函数应在程序结束时调用。
 */
void CN_error_framework_cleanup(void);

/**
 * @brief 设置全局错误处理回调
 * 
 * @param callback 错误处理回调函数
 * @param user_data 用户数据
 */
void CN_error_set_global_handler(
    void (*callback)(const Stru_CN_ErrorContext_t* context, void* user_data),
    void* user_data);

/**
 * @brief 报告错误到全局处理器
 * 
 * @param context 错误上下文
 */
void CN_error_report(const Stru_CN_ErrorContext_t* context);

/**
 * @brief 快速报告错误（简化接口）
 * 
 * @param error_code 错误码
 * @param message 错误消息
 * @param filename 文件名
 * @param line 行号
 */
void CN_error_report_quick(
    Eum_CN_ErrorCode_t error_code,
    const char* message,
    const char* filename,
    size_t line);

/**
 * @brief 获取最后一个报告的错误
 * 
 * @return 最后一个错误的上下文，如果没有错误则返回NULL
 */
const Stru_CN_ErrorContext_t* CN_error_get_last_reported(void);

/**
 * @brief 清除所有报告的错误
 */
void CN_error_clear_all_reported(void);

/**
 * @brief 检查是否有错误发生
 * 
 * @return true 如果有错误发生，false 如果没有错误
 */
bool CN_error_has_occurred(void);

/**
 * @brief 获取错误严重性级别
 * 
 * @param error_code 错误码
 * @return 错误严重性级别（0-10，0表示无错误，10表示最严重）
 */
int CN_error_get_severity(Eum_CN_ErrorCode_t error_code);

/**
 * @brief 检查错误是否可恢复
 * 
 * @param error_code 错误码
 * @return true 如果错误可恢复，false 如果不可恢复
 */
bool CN_error_is_recoverable(Eum_CN_ErrorCode_t error_code);

/**
 * @brief 错误处理框架版本信息
 * 
 * @return 版本字符串
 */
const char* CN_error_framework_version(void);

/**
 * @brief 错误处理框架配置
 * 
 * 设置错误处理框架的配置选项。
 */
typedef struct Stru_CN_ErrorConfig_t
{
    /** 是否启用详细日志 */
    bool verbose_logging;
    
    /** 最大错误链长度（0表示无限制） */
    size_t max_chain_length;
    
    /** 是否自动报告到标准错误输出 */
    bool auto_report_to_stderr;
    
    /** 是否启用错误统计 */
    bool enable_statistics;
    
    /** 错误报告格式 */
    const char* report_format;
    
} Stru_CN_ErrorConfig_t;

/**
 * @brief 获取当前配置
 * 
 * @return 当前配置
 */
Stru_CN_ErrorConfig_t CN_error_get_config(void);

/**
 * @brief 设置配置
 * 
 * @param config 新配置
 */
void CN_error_set_config(const Stru_CN_ErrorConfig_t* config);

#endif // CN_ERROR_H
