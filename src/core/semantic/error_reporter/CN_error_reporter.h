/**
 * @file CN_error_reporter.h
 * @brief 语义错误报告器模块头文件
 
 * 本文件定义了语义错误报告器模块的接口和数据结构，负责收集、管理和报告
 * 语义分析过程中发现的错误。遵循SOLID设计原则和分层架构。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_ERROR_REPORTER_H
#define CN_ERROR_REPORTER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct Stru_SemanticErrorReporterInterface_t Stru_SemanticErrorReporterInterface_t;
typedef struct Stru_SemanticError_t Stru_SemanticError_t;

// ============================================================================
// 语义错误类型枚举
// ============================================================================

/**
 * @brief 语义错误类型枚举
 */
typedef enum Eum_SemanticErrorType
{
    Eum_SEMANTIC_ERROR_UNDEFINED_SYMBOL,      ///< 未定义符号
    Eum_SEMANTIC_ERROR_REDEFINED_SYMBOL,      ///< 重复定义符号
    Eum_SEMANTIC_ERROR_TYPE_MISMATCH,         ///< 类型不匹配
    Eum_SEMANTIC_ERROR_INVALID_OPERATION,     ///< 无效操作
    Eum_SEMANTIC_ERROR_INVALID_ARGUMENTS,     ///< 无效参数
    Eum_SEMANTIC_ERROR_UNREACHABLE_CODE,      ///< 不可达代码
    Eum_SEMANTIC_ERROR_MISSING_RETURN,        ///< 缺少返回语句
    Eum_SEMANTIC_ERROR_INVALID_RETURN_TYPE,   ///< 无效返回类型
    Eum_SEMANTIC_ERROR_UNINITIALIZED_VARIABLE,///< 未初始化变量
    Eum_SEMANTIC_ERROR_CONST_ASSIGNMENT,      ///< 常量赋值
    Eum_SEMANTIC_ERROR_SCOPE_VIOLATION,       ///< 作用域违规
    Eum_SEMANTIC_ERROR_CIRCULAR_DEPENDENCY,   ///< 循环依赖
    Eum_SEMANTIC_ERROR_INVALID_IMPORT,        ///< 无效导入
    Eum_SEMANTIC_ERROR_INVALID_EXPORT         ///< 无效导出
} Eum_SemanticErrorType;

// ============================================================================
// 语义错误信息结构体
// ============================================================================

/**
 * @brief 语义错误信息结构体
 */
struct Stru_SemanticError_t
{
    Eum_SemanticErrorType type;      ///< 错误类型
    const char* message;             ///< 错误消息
    size_t line;                     ///< 错误行号
    size_t column;                   ///< 错误列号
    const char* file_name;           ///< 文件名
    void* related_node;              ///< 相关AST节点（可选）
    void* extra_data;                ///< 额外数据（实现特定）
};

// ============================================================================
// 语义错误报告接口
// ============================================================================

/**
 * @brief 语义错误报告接口
 
 * 负责收集、管理和报告语义分析过程中发现的错误。
 */
struct Stru_SemanticErrorReporterInterface_t
{
    /**
     * @brief 初始化错误报告器
     * 
     * @param reporter 错误报告器实例
     * @param max_errors 最大错误数量限制（0表示无限制）
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_SemanticErrorReporterInterface_t* reporter,
                      size_t max_errors);
    
    /**
     * @brief 报告语义错误
     * 
     * @param reporter 错误报告器实例
     * @param error 错误信息
     * @return bool 报告成功返回true，否则返回false（如达到错误上限）
     */
    bool (*report_error)(Stru_SemanticErrorReporterInterface_t* reporter,
                        const Stru_SemanticError_t* error);
    
    /**
     * @brief 报告语义警告
     * 
     * @param reporter 错误报告器实例
     * @param error 警告信息（使用错误结构体）
     * @return bool 报告成功返回true，否则返回false
     */
    bool (*report_warning)(Stru_SemanticErrorReporterInterface_t* reporter,
                          const Stru_SemanticError_t* error);
    
    /**
     * @brief 获取错误数量
     * 
     * @param reporter 错误报告器实例
     * @return size_t 错误数量
     */
    size_t (*get_error_count)(Stru_SemanticErrorReporterInterface_t* reporter);
    
    /**
     * @brief 获取警告数量
     * 
     * @param reporter 错误报告器实例
     * @return size_t 警告数量
     */
    size_t (*get_warning_count)(Stru_SemanticErrorReporterInterface_t* reporter);
    
    /**
     * @brief 获取所有错误
     * 
     * @param reporter 错误报告器实例
     * @param errors 输出参数：错误数组
     * @param max_errors 最大错误数量
     * @return size_t 实际获取的错误数量
     */
    size_t (*get_all_errors)(Stru_SemanticErrorReporterInterface_t* reporter,
                            Stru_SemanticError_t* errors, size_t max_errors);
    
    /**
     * @brief 获取所有警告
     * 
     * @param reporter 错误报告器实例
     * @param warnings 输出参数：警告数组
     * @param max_warnings 最大警告数量
     * @return size_t 实际获取的警告数量
     */
    size_t (*get_all_warnings)(Stru_SemanticErrorReporterInterface_t* reporter,
                              Stru_SemanticError_t* warnings, size_t max_warnings);
    
    /**
     * @brief 清空所有错误和警告
     * 
     * @param reporter 错误报告器实例
     */
    void (*clear_all)(Stru_SemanticErrorReporterInterface_t* reporter);
    
    /**
     * @brief 检查是否有错误
     * 
     * @param reporter 错误报告器实例
     * @return bool 有错误返回true，否则返回false
     */
    bool (*has_errors)(Stru_SemanticErrorReporterInterface_t* reporter);
    
    /**
     * @brief 检查是否有警告
     * 
     * @param reporter 错误报告器实例
     * @return bool 有警告返回true，否则返回false
     */
    bool (*has_warnings)(Stru_SemanticErrorReporterInterface_t* reporter);
    
    /**
     * @brief 设置错误处理回调
     * 
     * @param reporter 错误报告器实例
     * @param callback 回调函数
     * @param user_data 用户数据
     */
    void (*set_error_callback)(Stru_SemanticErrorReporterInterface_t* reporter,
                              void (*callback)(const Stru_SemanticError_t* error, void* user_data),
                              void* user_data);
    
    /**
     * @brief 过滤错误
     * 
     * 根据错误类型、位置等条件过滤错误。
     * 
     * @param reporter 错误报告器实例
     * @param filter_type 要过滤的错误类型（如果为0，不过滤类型）
     * @param min_line 最小行号（如果为0，不过滤行号）
     * @param max_line 最大行号（如果为0，不过滤行号）
     * @param file_name 文件名过滤（如果为NULL，不过滤文件名）
     * @return size_t 过滤后的错误数量
     */
    size_t (*filter_errors)(Stru_SemanticErrorReporterInterface_t* reporter,
                           Eum_SemanticErrorType filter_type,
                           size_t min_line, size_t max_line,
                           const char* file_name);
    
    /**
     * @brief 分类错误
     * 
     * 将错误按类型分类，返回各类错误的数量。
     * 
     * @param reporter 错误报告器实例
     * @param error_counts 输出参数：各类错误数量数组（大小为错误类型数量）
     * @param max_types 错误类型最大数量
     * @return size_t 实际处理的错误类型数量
     */
    size_t (*categorize_errors)(Stru_SemanticErrorReporterInterface_t* reporter,
                               size_t* error_counts, size_t max_types);
    
    /**
     * @brief 跟踪错误位置
     * 
     * 获取错误在源代码中的位置信息，包括行号、列号、文件名等。
     * 
     * @param reporter 错误报告器实例
     * @param error_index 错误索引
     * @param line 输出参数：行号
     * @param column 输出参数：列号
     * @param file_name 输出参数：文件名（缓冲区）
     * @param file_name_size 文件名缓冲区大小
     * @return bool 获取成功返回true，否则返回false
     */
    bool (*track_error_location)(Stru_SemanticErrorReporterInterface_t* reporter,
                                size_t error_index,
                                size_t* line, size_t* column,
                                char* file_name, size_t file_name_size);
    
    /**
     * @brief 获取错误严重性
     * 
     * 根据错误类型和上下文判断错误的严重性。
     * 
     * @param reporter 错误报告器实例
     * @param error_index 错误索引
     * @return int 错误严重性（0-10，0表示信息，10表示致命错误）
     */
    int (*get_error_severity)(Stru_SemanticErrorReporterInterface_t* reporter,
                             size_t error_index);
    
    /**
     * @brief 销毁错误报告器
     * 
     * @param reporter 错误报告器实例
     */
    void (*destroy)(Stru_SemanticErrorReporterInterface_t* reporter);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 工厂函数声明
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建语义错误报告器接口实例
 * 
 * @return Stru_SemanticErrorReporterInterface_t* 错误报告器接口实例，失败返回NULL
 */
Stru_SemanticErrorReporterInterface_t* F_create_semantic_error_reporter_interface(void);

/**
 * @brief 销毁语义错误报告器接口实例
 * 
 * @param interface 要销毁的接口实例
 */
void F_destroy_semantic_error_reporter_interface(Stru_SemanticErrorReporterInterface_t* interface);

#ifdef __cplusplus
}
#endif

#endif /* CN_ERROR_REPORTER_H */
