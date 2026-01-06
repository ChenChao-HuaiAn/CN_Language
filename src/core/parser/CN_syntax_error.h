/**
 * @file CN_syntax_error.h
 * @brief CN_Language 语法错误处理接口
 * 
 * 定义语法错误结构体、错误处理接口和错误报告机制。
 * 遵循项目架构规范，支持语法分析器的错误处理需求。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 */

#ifndef CN_SYNTAX_ERROR_H
#define CN_SYNTAX_ERROR_H

#include "../token/CN_token.h"
#include "../../infrastructure/containers/array/CN_dynamic_array.h"

/**
 * @brief 语法错误严重级别枚举
 * 
 * 定义语法错误的严重级别，用于错误分类和处理。
 */
typedef enum Eum_SyntaxErrorSeverity {
    Eum_SEVERITY_INFO,        ///< 信息级别（非错误）
    Eum_SEVERITY_WARNING,     ///< 警告级别（可能有问题）
    Eum_SEVERITY_ERROR,       ///< 错误级别（语法错误）
    Eum_SEVERITY_FATAL        ///< 致命错误级别（无法恢复）
} Eum_SyntaxErrorSeverity;

/**
 * @brief 语法错误类型枚举
 * 
 * 定义所有可能的语法错误类型。
 */
typedef enum Eum_SyntaxErrorType {
    // ============================================
    // 词法错误（由词法分析器报告）
    // ============================================
    Eum_ERROR_UNKNOWN_TOKEN,          ///< 未知令牌
    Eum_ERROR_UNTERMINATED_STRING,    ///< 未终止的字符串
    Eum_ERROR_UNTERMINATED_COMMENT,   ///< 未终止的注释
    Eum_ERROR_INVALID_NUMBER,         ///< 无效的数字字面量
    Eum_ERROR_INVALID_ESCAPE,         ///< 无效的转义序列
    
    // ============================================
    // 语法错误
    // ============================================
    Eum_ERROR_UNEXPECTED_TOKEN,       ///< 意外的令牌
    Eum_ERROR_MISSING_TOKEN,          ///< 缺失的令牌
    Eum_ERROR_MISMATCHED_TOKEN,       ///< 不匹配的令牌
    Eum_ERROR_INVALID_EXPRESSION,     ///< 无效的表达式
    Eum_ERROR_INVALID_STATEMENT,      ///< 无效的语句
    Eum_ERROR_INVALID_DECLARATION,    ///< 无效的声明
    
    // ============================================
    // 结构错误
    // ============================================
    Eum_ERROR_UNEXPECTED_EOF,         ///< 意外的文件结束
    Eum_ERROR_UNCLOSED_BLOCK,         ///< 未关闭的代码块
    Eum_ERROR_UNCLOSED_PAREN,         ///< 未关闭的括号
    Eum_ERROR_UNCLOSED_BRACKET,       ///< 未关闭的方括号
    Eum_ERROR_UNCLOSED_BRACE,         ///< 未关闭的大括号
    
    // ============================================
    // 语义错误（语法分析阶段可检测的）
    // ============================================
    Eum_ERROR_DUPLICATE_DECLARATION,  ///< 重复声明
    Eum_ERROR_UNDECLARED_IDENTIFIER,  ///< 未声明的标识符
    Eum_ERROR_TYPE_MISMATCH,          ///< 类型不匹配
    Eum_ERROR_ARGUMENT_COUNT,         ///< 参数数量错误
    
    // ============================================
    // 其他错误
    // ============================================
    Eum_ERROR_INTERNAL,               ///< 内部错误
    Eum_ERROR_MEMORY,                 ///< 内存错误
    Eum_ERROR_IO,                     ///< 输入输出错误
    
    Eum_ERROR_COUNT                   ///< 错误类型总数（用于边界检查）
} Eum_SyntaxErrorType;

/**
 * @brief 语法错误结构体
 * 
 * 表示一个语法错误，包含错误的所有相关信息。
 */
typedef struct Stru_SyntaxError_t {
    Eum_SyntaxErrorType type;         ///< 错误类型
    Eum_SyntaxErrorSeverity severity; ///< 错误严重级别
    size_t line;                      ///< 错误行号（从1开始）
    size_t column;                    ///< 错误列号（从1开始）
    char* message;                    ///< 错误信息
    char* source_name;                ///< 源文件名
    Stru_Token_t* token;              ///< 相关的令牌（可选）
    
    /**
     * @brief 错误数据联合体
     * 
     * 根据错误类型存储相应的数据。
     */
    union {
        char* expected;               ///< 期望的内容
        char* found;                  ///< 实际找到的内容
        size_t count;                 ///< 数量信息
        void* custom_data;            ///< 自定义数据
    } data;
    
} Stru_SyntaxError_t;

/**
 * @brief 语法错误处理接口结构体
 * 
 * 定义语法错误处理的完整接口，包含错误报告、查询和管理功能。
 */
typedef struct Stru_SyntaxErrorHandler_t {
    // ============================================
    // 错误报告
    // ============================================
    
    /**
     * @brief 报告语法错误
     * 
     * 报告一个语法错误。
     * 
     * @param handler 错误处理接口指针
     * @param type 错误类型
     * @param severity 错误严重级别
     * @param line 错误行号
     * @param column 错误列号
     * @param message 错误信息
     * @param token 相关的令牌（可选）
     * @return Stru_SyntaxError_t* 创建的错误对象，NULL表示失败
     */
    Stru_SyntaxError_t* (*report_error)(struct Stru_SyntaxErrorHandler_t* handler,
                                       Eum_SyntaxErrorType type,
                                       Eum_SyntaxErrorSeverity severity,
                                       size_t line, size_t column,
                                       const char* message,
                                       Stru_Token_t* token);
    
    /**
     * @brief 报告意外令牌错误
     * 
     * 报告意外令牌错误（常用错误类型）。
     * 
     * @param handler 错误处理接口指针
     * @param token 意外的令牌
     * @param expected 期望的令牌类型或内容
     * @return Stru_SyntaxError_t* 创建的错误对象，NULL表示失败
     */
    Stru_SyntaxError_t* (*report_unexpected_token)(struct Stru_SyntaxErrorHandler_t* handler,
                                                  Stru_Token_t* token,
                                                  const char* expected);
    
    /**
     * @brief 报告缺失令牌错误
     * 
     * 报告缺失令牌错误。
     * 
     * @param handler 错误处理接口指针
     * @param line 错误行号
     * @param column 错误列号
     * @param expected 期望的令牌类型或内容
     * @return Stru_SyntaxError_t* 创建的错误对象，NULL表示失败
     */
    Stru_SyntaxError_t* (*report_missing_token)(struct Stru_SyntaxErrorHandler_t* handler,
                                               size_t line, size_t column,
                                               const char* expected);
    
    // ============================================
    // 错误查询
    // ============================================
    
    /**
     * @brief 检查是否有错误
     * 
     * 检查是否记录了任何错误。
     * 
     * @param handler 错误处理接口指针
     * @return true 有错误
     * @return false 没有错误
     */
    bool (*has_errors)(struct Stru_SyntaxErrorHandler_t* handler);
    
    /**
     * @brief 检查是否有特定严重级别的错误
     * 
     * 检查是否记录了特定严重级别的错误。
     * 
     * @param handler 错误处理接口指针
     * @param severity 错误严重级别
     * @return true 有该级别的错误
     * @return false 没有该级别的错误
     */
    bool (*has_errors_of_severity)(struct Stru_SyntaxErrorHandler_t* handler,
                                  Eum_SyntaxErrorSeverity severity);
    
    /**
     * @brief 获取错误数量
     * 
     * 获取记录的错误总数。
     * 
     * @param handler 错误处理接口指针
     * @return size_t 错误数量
     */
    size_t (*get_error_count)(struct Stru_SyntaxErrorHandler_t* handler);
    
    /**
     * @brief 获取特定严重级别的错误数量
     * 
     * 获取特定严重级别的错误数量。
     * 
     * @param handler 错误处理接口指针
     * @param severity 错误严重级别
     * @return size_t 错误数量
     */
    size_t (*get_error_count_by_severity)(struct Stru_SyntaxErrorHandler_t* handler,
                                         Eum_SyntaxErrorSeverity severity);
    
    /**
     * @brief 获取错误列表
     * 
     * 获取所有错误的列表。
     * 
     * @param handler 错误处理接口指针
     * @return Stru_DynamicArray_t* 错误数组，NULL表示无错误
     */
    Stru_DynamicArray_t* (*get_errors)(struct Stru_SyntaxErrorHandler_t* handler);
    
    /**
     * @brief 获取特定严重级别的错误列表
     * 
     * 获取特定严重级别的错误列表。
     * 
     * @param handler 错误处理接口指针
     * @param severity 错误严重级别
     * @return Stru_DynamicArray_t* 错误数组，NULL表示无错误
     */
    Stru_DynamicArray_t* (*get_errors_by_severity)(struct Stru_SyntaxErrorHandler_t* handler,
                                                  Eum_SyntaxErrorSeverity severity);
    
    /**
     * @brief 获取最后一个错误
     * 
     * 获取最后一个报告的错误。
     * 
     * @param handler 错误处理接口指针
     * @return Stru_SyntaxError_t* 最后一个错误，NULL表示无错误
     */
    Stru_SyntaxError_t* (*get_last_error)(struct Stru_SyntaxErrorHandler_t* handler);
    
    // ============================================
    // 错误管理
    // ============================================
    
    /**
     * @brief 清除所有错误
     * 
     * 清除所有已记录的错误。
     * 
     * @param handler 错误处理接口指针
     */
    void (*clear_errors)(struct Stru_SyntaxErrorHandler_t* handler);
    
    /**
     * @brief 清除特定严重级别的错误
     * 
     * 清除特定严重级别的错误。
     * 
     * @param handler 错误处理接口指针
     * @param severity 错误严重级别
     */
    void (*clear_errors_by_severity)(struct Stru_SyntaxErrorHandler_t* handler,
                                    Eum_SyntaxErrorSeverity severity);
    
    /**
     * @brief 设置最大错误数量
     * 
     * 设置允许记录的最大错误数量。
     * 
     * @param handler 错误处理接口指针
     * @param max_errors 最大错误数量
     */
    void (*set_max_errors)(struct Stru_SyntaxErrorHandler_t* handler, size_t max_errors);
    
    /**
     * @brief 获取最大错误数量
     * 
     * 获取允许记录的最大错误数量。
     * 
     * @param handler 错误处理接口指针
     * @return size_t 最大错误数量
     */
    size_t (*get_max_errors)(struct Stru_SyntaxErrorHandler_t* handler);
    
    // ============================================
    // 错误格式化
    // ============================================
    
    /**
     * @brief 格式化错误信息
     * 
     * 将错误格式化为可读的字符串。
     * 
     * @param handler 错误处理接口指针
     * @param error 错误对象
     * @param buffer 输出缓冲区
     * @param buffer_size 缓冲区大小
     * @return int 写入的字符数（不包括终止空字符）
     */
    int (*format_error)(struct Stru_SyntaxErrorHandler_t* handler,
                       const Stru_SyntaxError_t* error,
                       char* buffer, size_t buffer_size);
    
    /**
     * @brief 打印所有错误
     * 
     * 打印所有记录的错误。
     * 
     * @param handler 错误处理接口指针
     */
    void (*print_all_errors)(struct Stru_SyntaxErrorHandler_t* handler);
    
    // ============================================
    // 资源管理
    // ============================================
    
    /**
     * @brief 销毁错误处理接口
     * 
     * 释放错误处理接口占用的所有资源。
     * 
     * @param handler 错误处理接口指针
     */
    void (*destroy)(struct Stru_SyntaxErrorHandler_t* handler);
    
    // ============================================
    // 内部状态（不直接暴露）
    // ============================================
    
    /**
     * @brief 内部状态指针
     * 
     * 具体实现可以使用此指针存储内部状态。
     * 接口使用者不应直接访问此字段。
     */
    void* internal_state;
    
} Stru_SyntaxErrorHandler_t;

// ============================================
// 错误对象操作函数
// ============================================

/**
 * @brief 创建语法错误
 * 
 * 创建并初始化一个新的语法错误对象。
 * 
 * @param type 错误类型
 * @param severity 错误严重级别
 * @param line 错误行号
 * @param column 错误列号
 * @param message 错误信息
 * @param token 相关的令牌（可选）
 * @return Stru_SyntaxError_t* 新创建的语法错误对象，失败返回NULL
 */
Stru_SyntaxError_t* F_create_syntax_error(Eum_SyntaxErrorType type,
                                         Eum_SyntaxErrorSeverity severity,
                                         size_t line, size_t column,
                                         const char* message,
                                         Stru_Token_t* token);

/**
 * @brief 销毁语法错误
 * 
 * 释放语法错误对象占用的所有内存。
 * 
 * @param error 要销毁的语法错误对象
 */
void F_destroy_syntax_error(Stru_SyntaxError_t* error);

/**
 * @brief 复制语法错误
 * 
 * 创建语法错误对象的深拷贝。
 * 
 * @param error 要复制的语法错误对象
 * @return Stru_SyntaxError_t* 新复制的语法错误对象
 */
Stru_SyntaxError_t* F_copy_syntax_error(const Stru_SyntaxError_t* error);

/**
 * @brief 设置错误期望内容
 * 
 * 为错误对象设置期望的内容。
 * 
 * @param error 错误对象
 * @param expected 期望的内容
 */
void F_error_set_expected(Stru_SyntaxError_t* error, const char* expected);

/**
 * @brief 设置错误实际找到的内容
 * 
 * 为错误对象设置实际找到的内容。
 * 
 * @param error 错误对象
 * @param found 实际找到的内容
 */
void F_error_set_found(Stru_SyntaxError_t* error, const char* found);

/**
 * @brief 设置错误数量信息
 * 
 * 为错误对象设置数量信息。
 * 
 * @param error 错误对象
 * @param count 数量
 */
void F_error_set_count(Stru_SyntaxError_t* error, size_t count);

/**
 * @brief 设置错误源文件名
 * 
 * 为错误对象设置源文件名。
 * 
 * @param error 错误对象
 * @param source_name 源文件名
 */
void F_error_set_source_name(Stru_SyntaxError_t* error, const char* source_name);

// ============================================
// 错误类型和严重级别转换函数
// ============================================

/**
 * @brief 错误类型转字符串
 * 
 * 将错误类型转换为可读的字符串表示。
 * 
 * @param type 错误类型
 * @return const char* 类型字符串表示
 */
const char* F_syntax_error_type_to_string(Eum_SyntaxErrorType type);

/**
 * @brief 错误严重级别转字符串
 * 
 * 将错误严重级别转换为可读的字符串表示。
 * 
 * @param severity 错误严重级别
 * @return const char* 严重级别字符串表示
 */
const char* F_syntax_error_severity_to_string(Eum_SyntaxErrorSeverity severity);

// ============================================
// 工厂函数
// ============================================

/**
 * @brief 创建错误处理接口实例
 * 
 * 创建并返回一个新的错误处理接口实例。
 * 调用者负责在不再使用时调用destroy函数。
 * 
 * @return Stru_SyntaxErrorHandler_t* 新创建的错误处理接口实例
 */
Stru_SyntaxErrorHandler_t* F_create_syntax_error_handler(void);

#endif // CN_SYNTAX_ERROR_H
