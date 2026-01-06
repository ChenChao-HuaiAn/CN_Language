/**
 * @file CN_utils_interface.h
 * @brief 工具函数库接口定义
 * 
 * 本文件定义了CN_Language项目的工具函数库接口，提供字符串处理、
 * 数学函数、错误处理等通用工具函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_UTILS_INTERFACE_H
#define CN_UTILS_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// 字符串处理函数
// ============================================================================

/**
 * @brief 复制字符串
 * 
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @param max_len 最大复制长度（包括终止符）
 * @return char* 目标字符串指针
 */
char* cn_strcpy(char* dest, const char* src, size_t max_len);

/**
 * @brief 连接字符串
 * 
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @param max_len 最大长度（包括终止符）
 * @return char* 目标字符串指针
 */
char* cn_strcat(char* dest, const char* src, size_t max_len);

/**
 * @brief 比较字符串
 * 
 * @param str1 第一个字符串
 * @param str2 第二个字符串
 * @return int 比较结果（0表示相等）
 */
int cn_strcmp(const char* str1, const char* str2);

/**
 * @brief 比较字符串（指定长度）
 * 
 * @param str1 第一个字符串
 * @param str2 第二个字符串
 * @param n 比较的最大字符数
 * @return int 比较结果（0表示相等）
 */
int cn_strncmp(const char* str1, const char* str2, size_t n);

/**
 * @brief 获取字符串长度
 * 
 * @param str 字符串
 * @return size_t 字符串长度（不包括终止符）
 */
size_t cn_strlen(const char* str);

/**
 * @brief 查找字符
 * 
 * @param str 字符串
 * @param ch 要查找的字符
 * @return char* 字符位置指针，未找到返回NULL
 */
char* cn_strchr(const char* str, int ch);

/**
 * @brief 查找子字符串
 * 
 * @param haystack 主字符串
 * @param needle 子字符串
 * @return char* 子字符串位置指针，未找到返回NULL
 */
char* cn_strstr(const char* haystack, const char* needle);

/**
 * @brief 复制内存块到字符串
 * 
 * @param dest 目标缓冲区
 * @param src 源内存
 * @param n 要复制的字节数
 * @return char* 目标字符串指针
 */
char* cn_strncpy(char* dest, const char* src, size_t n);

// ============================================================================
// 内存操作函数（与内存管理模块互补）
// ============================================================================

/**
 * @brief 移动内存块
 * 
 * @param dest 目标内存
 * @param src 源内存
 * @param size 要移动的字节数
 * @return void* 目标内存指针
 */
void* cn_memmove(void* dest, const void* src, size_t size);

/**
 * @brief 查找内存中的字节
 * 
 * @param ptr 内存指针
 * @param value 要查找的值
 * @param size 要搜索的字节数
 * @return void* 找到的位置指针，未找到返回NULL
 */
void* cn_memchr(const void* ptr, int value, size_t size);

// ============================================================================
// 数学函数
// ============================================================================

/**
 * @brief 绝对值（整数）
 * 
 * @param x 整数值
 * @return int 绝对值
 */
int cn_abs(int x);

/**
 * @brief 绝对值（长整数）
 * 
 * @param x 长整数值
 * @return long 绝对值
 */
long cn_labs(long x);

/**
 * @brief 绝对值（长长整数）
 * 
 * @param x 长长整数值
 * @return long long 绝对值
 */
long long cn_llabs(long long x);

/**
 * @brief 除法运算
 * 
 * @param numer 被除数
 * @param denom 除数
 * @param rem 余数输出参数
 * @return int 商
 */
int cn_div(int numer, int denom, int* rem);

/**
 * @brief 最小值
 * 
 * @param a 第一个值
 * @param b 第二个值
 * @return int 最小值
 */
int cn_min(int a, int b);

/**
 * @brief 最大值
 * 
 * @param a 第一个值
 * @param b 第二个值
 * @return int 最大值
 */
int cn_max(int a, int b);

/**
 * @brief 限制值在范围内
 * 
 * @param value 要限制的值
 * @param min 最小值
 * @param max 最大值
 * @return int 限制后的值
 */
int cn_clamp(int value, int min, int max);

// ============================================================================
// 错误处理函数
// ============================================================================

/**
 * @brief 错误代码枚举
 */
typedef enum Eum_ErrorCode_t
{
    Eum_ERROR_NONE = 0,              ///< 无错误
    Eum_ERROR_INVALID_ARGUMENT,      ///< 无效参数
    Eum_ERROR_OUT_OF_MEMORY,         ///< 内存不足
    Eum_ERROR_BUFFER_OVERFLOW,       ///< 缓冲区溢出
    Eum_ERROR_FILE_NOT_FOUND,        ///< 文件未找到
    Eum_ERROR_PERMISSION_DENIED,     ///< 权限被拒绝
    Eum_ERROR_IO_ERROR,              ///< IO错误
    Eum_ERROR_FORMAT_ERROR,          ///< 格式错误
    Eum_ERROR_SYNTAX_ERROR,          ///< 语法错误
    Eum_ERROR_RUNTIME_ERROR,         ///< 运行时错误
    Eum_ERROR_NOT_IMPLEMENTED,       ///< 未实现
    Eum_ERROR_INTERNAL_ERROR         ///< 内部错误
} Eum_ErrorCode_t;

/**
 * @brief 错误信息结构
 */
typedef struct Stru_ErrorInfo_t
{
    Eum_ErrorCode_t code;            ///< 错误代码
    const char* message;             ///< 错误消息
    const char* file;                ///< 发生错误的文件
    int line;                        ///< 发生错误的行号
    struct Stru_ErrorInfo_t* cause;  ///< 原因错误（可为NULL）
} Stru_ErrorInfo_t;

/**
 * @brief 设置当前线程的错误信息
 * 
 * @param code 错误代码
 * @param message 错误消息
 * @param file 文件名
 * @param line 行号
 * @param cause 原因错误（可为NULL）
 */
void cn_set_error(Eum_ErrorCode_t code, const char* message,
                  const char* file, int line, Stru_ErrorInfo_t* cause);

/**
 * @brief 获取当前线程的错误信息
 * 
 * @return Stru_ErrorInfo_t* 错误信息指针，无错误返回NULL
 */
Stru_ErrorInfo_t* cn_get_error(void);

/**
 * @brief 清除当前线程的错误信息
 */
void cn_clear_error(void);

/**
 * @brief 创建错误信息
 * 
 * @param code 错误代码
 * @param message 错误消息
 * @param file 文件名
 * @param line 行号
 * @param cause 原因错误（可为NULL）
 * @return Stru_ErrorInfo_t* 错误信息指针
 */
Stru_ErrorInfo_t* cn_create_error(Eum_ErrorCode_t code, const char* message,
                                  const char* file, int line, Stru_ErrorInfo_t* cause);

/**
 * @brief 销毁错误信息
 * 
 * @param error 要销毁的错误信息
 */
void cn_destroy_error(Stru_ErrorInfo_t* error);

// ============================================================================
// 日志函数
// ============================================================================

/**
 * @brief 日志级别枚举
 */
typedef enum Eum_LogLevel_t
{
    Eum_LOG_LEVEL_TRACE = 0,         ///< 跟踪级别（最详细）
    Eum_LOG_LEVEL_DEBUG,             ///< 调试级别
    Eum_LOG_LEVEL_INFO,              ///< 信息级别
    Eum_LOG_LEVEL_WARNING,           ///< 警告级别
    Eum_LOG_LEVEL_ERROR,             ///< 错误级别
    Eum_LOG_LEVEL_FATAL              ///< 致命级别（最严重）
} Eum_LogLevel_t;

/**
 * @brief 日志记录函数
 * 
 * @param level 日志级别
 * @param file 文件名
 * @param line 行号
 * @param format 格式字符串
 * @param ... 可变参数
 */
void cn_log(Eum_LogLevel_t level, const char* file, int line,
            const char* format, ...);

/**
 * @brief 设置日志级别
 * 
 * @param level 最小日志级别（低于此级别的日志将被忽略）
 */
void cn_set_log_level(Eum_LogLevel_t level);

/**
 * @brief 设置日志输出回调
 * 
 * @param callback 日志输出回调函数
 */
void cn_set_log_callback(void (*callback)(Eum_LogLevel_t level,
                                          const char* file, int line,
                                          const char* message));

// ============================================================================
// 断言函数
// ============================================================================

/**
 * @brief 断言宏（在调试模式下有效）
 */
#ifdef CN_DEBUG
#define CN_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            cn_log(Eum_LOG_LEVEL_FATAL, __FILE__, __LINE__, \
                   "Assertion failed: %s", #condition); \
            cn_set_error(Eum_ERROR_INTERNAL_ERROR, \
                        "Assertion failed: " #condition, \
                        __FILE__, __LINE__, NULL); \
        } \
    } while (0)
#else
#define CN_ASSERT(condition) ((void)0)
#endif

/**
 * @brief 检查条件并设置错误
 * 
 * @param condition 条件
 * @param error_code 错误代码（如果条件为假）
 * @param error_message 错误消息（如果条件为假）
 */
#define CN_CHECK(condition, error_code, error_message) \
    do { \
        if (!(condition)) { \
            cn_set_error((error_code), (error_message), \
                        __FILE__, __LINE__, NULL); \
            return false; \
        } \
    } while (0)

/**
 * @brief 检查条件并设置错误（返回指定值）
 * 
 * @param condition 条件
 * @param error_code 错误代码（如果条件为假）
 * @param error_message 错误消息（如果条件为假）
 * @param return_value 返回值（如果条件为假）
 */
#define CN_CHECK_RETURN(condition, error_code, error_message, return_value) \
    do { \
        if (!(condition)) { \
            cn_set_error((error_code), (error_message), \
                        __FILE__, __LINE__, NULL); \
            return (return_value); \
        } \
    } while (0)

#endif // CN_UTILS_INTERFACE_H
