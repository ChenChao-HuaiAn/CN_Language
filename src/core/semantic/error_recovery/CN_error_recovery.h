/**
 * @file CN_error_recovery.h
 * @brief 详细的错误报告和恢复机制模块头文件
 
 * 本文件定义了详细的错误报告和恢复机制模块的接口和数据结构，负责提供
 * 详细的错误信息、错误上下文跟踪、错误恢复策略和错误报告格式化功能。
 * 遵循SOLID设计原则和分层架构。
 * 
 * 主要功能：
 * 1. 详细的错误信息：包含错误类型、位置、上下文、建议修复等
 * 2. 错误上下文跟踪：记录错误发生时的调用栈、变量状态等
 * 3. 错误恢复策略：提供多种错误恢复策略（跳过、修复、回滚等）
 * 4. 错误报告格式化：支持多种输出格式（文本、JSON、HTML等）
 * 5. 错误统计和分析：统计错误频率、分布、趋势等
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_ERROR_RECOVERY_H
#define CN_ERROR_RECOVERY_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// 前向声明
typedef struct Stru_DetailedErrorReporterInterface_t Stru_DetailedErrorReporterInterface_t;
typedef struct Stru_ErrorRecoveryInterface_t Stru_ErrorRecoveryInterface_t;
typedef struct Stru_ErrorContextInterface_t Stru_ErrorContextInterface_t;
typedef struct Stru_ErrorStatisticsInterface_t Stru_ErrorStatisticsInterface_t;
typedef struct Stru_DetailedError_t Stru_DetailedError_t;
typedef struct Stru_ErrorContext_t Stru_ErrorContext_t;
typedef struct Stru_ErrorRecoveryStrategy_t Stru_ErrorRecoveryStrategy_t;

// ============================================================================
// 错误严重性级别枚举
// ============================================================================

/**
 * @brief 错误严重性级别枚举
 */
typedef enum Eum_ErrorSeverityLevel
{
    Eum_ERROR_SEVERITY_INFO = 0,        ///< 信息级别（非错误）
    Eum_ERROR_SEVERITY_WARNING = 1,     ///< 警告级别
    Eum_ERROR_SEVERITY_ERROR = 2,       ///< 错误级别
    Eum_ERROR_SEVERITY_CRITICAL = 3,    ///< 严重错误级别
    Eum_ERROR_SEVERITY_FATAL = 4        ///< 致命错误级别
} Eum_ErrorSeverityLevel;

// ============================================================================
// 错误恢复策略枚举
// ============================================================================

/**
 * @brief 错误恢复策略枚举
 */
typedef enum Eum_ErrorRecoveryStrategy
{
    Eum_RECOVERY_STRATEGY_SKIP = 0,     ///< 跳过错误继续处理
    Eum_RECOVERY_STRATEGY_REPAIR = 1,   ///< 尝试修复错误
    Eum_RECOVERY_STRATEGY_ROLLBACK = 2, ///< 回滚到安全状态
    Eum_RECOVERY_STRATEGY_ABORT = 3,    ///< 中止处理
    Eum_RECOVERY_STRATEGY_RETRY = 4     ///< 重试操作
} Eum_ErrorRecoveryStrategy;

// ============================================================================
// 错误输出格式枚举
// ============================================================================

/**
 * @brief 错误输出格式枚举
 */
typedef enum Eum_ErrorOutputFormat
{
    Eum_ERROR_FORMAT_TEXT = 0,          ///< 纯文本格式
    Eum_ERROR_FORMAT_JSON = 1,          ///< JSON格式
    Eum_ERROR_FORMAT_XML = 2,           ///< XML格式
    Eum_ERROR_FORMAT_HTML = 3,          ///< HTML格式
    Eum_ERROR_FORMAT_MARKDOWN = 4       ///< Markdown格式
} Eum_ErrorOutputFormat;

// ============================================================================
// 详细的错误信息结构体
// ============================================================================

/**
 * @brief 详细的错误信息结构体
 */
struct Stru_DetailedError_t
{
    // 基本错误信息
    int error_code;                     ///< 错误代码
    const char* error_type;             ///< 错误类型名称
    const char* error_message;          ///< 错误消息
    Eum_ErrorSeverityLevel severity;    ///< 错误严重性级别
    
    // 位置信息
    size_t line;                        ///< 错误行号
    size_t column;                      ///< 错误列号
    const char* file_name;              ///< 文件名
    const char* function_name;          ///< 函数名
    const char* module_name;            ///< 模块名
    
    // 上下文信息
    const char* source_snippet;         ///< 源代码片段
    const char* surrounding_context;    ///< 周围上下文
    void* ast_node;                     ///< 相关AST节点
    void* symbol_info;                  ///< 相关符号信息
    
    // 时间信息
    time_t timestamp;                   ///< 错误发生时间戳
    uint64_t process_id;                ///< 进程ID
    uint64_t thread_id;                 ///< 线程ID
    
    // 修复建议
    const char* suggested_fix;          ///< 建议的修复方法
    const char* documentation_link;     ///< 相关文档链接
    
    // 调用栈信息
    void* call_stack;                   ///< 调用栈信息（实现特定）
    size_t call_stack_depth;            ///< 调用栈深度
    
    // 额外数据
    void* extra_data;                   ///< 额外数据（实现特定）
    size_t extra_data_size;             ///< 额外数据大小
};

// ============================================================================
// 错误上下文结构体
// ============================================================================

/**
 * @brief 错误上下文结构体
 */
struct Stru_ErrorContext_t
{
    // 编译状态
    const char* compilation_phase;      ///< 编译阶段（词法分析、语法分析、语义分析等）
    const char* current_pass;           ///< 当前处理过程
    
    // 符号表状态
    void* symbol_table_state;           ///< 符号表状态快照
    void* scope_stack_state;            ///< 作用域栈状态快照
    
    // 类型系统状态
    void* type_system_state;            ///< 类型系统状态快照
    
    // 内存状态
    size_t memory_usage;                ///< 内存使用量
    size_t heap_allocations;            ///< 堆分配数量
    
    // 性能指标
    uint64_t processing_time_ms;        ///< 处理时间（毫秒）
    size_t processed_nodes;             ///< 已处理的AST节点数量
    
    // 环境信息
    const char* platform;               ///< 平台信息
    const char* compiler_version;       ///< 编译器版本
    const char* language_version;       ///< 语言版本
};

// ============================================================================
// 错误恢复策略结构体
// ============================================================================

/**
 * @brief 错误恢复策略结构体
 */
struct Stru_ErrorRecoveryStrategy_t
{
    Eum_ErrorRecoveryStrategy strategy; ///< 恢复策略类型
    const char* strategy_name;          ///< 策略名称
    const char* description;            ///< 策略描述
    
    // 策略参数
    int max_retries;                    ///< 最大重试次数（仅对重试策略有效）
    int retry_delay_ms;                 ///< 重试延迟（毫秒）
    bool allow_partial_recovery;        ///< 是否允许部分恢复
    
    // 恢复动作
    void (*recovery_action)(void* context, const Stru_DetailedError_t* error); ///< 恢复动作函数
    void* recovery_context;             ///< 恢复动作上下文
};

// ============================================================================
// 详细的错误报告器接口
// ============================================================================

/**
 * @brief 详细的错误报告器接口
 
 * 负责收集、管理和报告详细的错误信息，包括错误上下文、调用栈等。
 */
struct Stru_DetailedErrorReporterInterface_t
{
    /**
     * @brief 初始化详细的错误报告器
     * 
     * @param reporter 错误报告器实例
     * @param max_errors 最大错误数量限制（0表示无限制）
     * @param enable_call_stack 是否启用调用栈跟踪
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_DetailedErrorReporterInterface_t* reporter,
                      size_t max_errors,
                      bool enable_call_stack);
    
    /**
     * @brief 报告详细的错误
     * 
     * @param reporter 错误报告器实例
     * @param error 详细的错误信息
     * @param context 错误上下文（可选）
     * @return bool 报告成功返回true，否则返回false
     */
    bool (*report_detailed_error)(Stru_DetailedErrorReporterInterface_t* reporter,
                                 const Stru_DetailedError_t* error,
                                 const Stru_ErrorContext_t* context);
    
    /**
     * @brief 获取错误详细信息
     * 
     * @param reporter 错误报告器实例
     * @param error_index 错误索引
     * @param error 输出参数：错误详细信息
     * @param context 输出参数：错误上下文（可选）
     * @return bool 获取成功返回true，否则返回false
     */
    bool (*get_error_details)(Stru_DetailedErrorReporterInterface_t* reporter,
                             size_t error_index,
                             Stru_DetailedError_t* error,
                             Stru_ErrorContext_t* context);
    
    /**
     * @brief 格式化错误报告
     * 
     * @param reporter 错误报告器实例
     * @param error_index 错误索引
     * @param format 输出格式
     * @param buffer 输出缓冲区
     * @param buffer_size 缓冲区大小
     * @return size_t 格式化后的字符串长度（不包括终止符）
     */
    size_t (*format_error_report)(Stru_DetailedErrorReporterInterface_t* reporter,
                                 size_t error_index,
                                 Eum_ErrorOutputFormat format,
                                 char* buffer,
                                 size_t buffer_size);
    
    /**
     * @brief 导出所有错误报告
     * 
     * @param reporter 错误报告器实例
     * @param format 输出格式
     * @param file_path 输出文件路径
     * @return bool 导出成功返回true，否则返回false
     */
    bool (*export_all_errors)(Stru_DetailedErrorReporterInterface_t* reporter,
                             Eum_ErrorOutputFormat format,
                             const char* file_path);
    
    /**
     * @brief 销毁详细的错误报告器
     * 
     * @param reporter 错误报告器实例
     */
    void (*destroy)(Stru_DetailedErrorReporterInterface_t* reporter);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 错误恢复接口
// ============================================================================

/**
 * @brief 错误恢复接口
 
 * 负责实现错误恢复策略，帮助从错误中恢复并继续处理。
 */
struct Stru_ErrorRecoveryInterface_t
{
    /**
     * @brief 初始化错误恢复器
     * 
     * @param recovery 错误恢复器实例
     * @param default_strategy 默认恢复策略
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_ErrorRecoveryInterface_t* recovery,
                      Eum_ErrorRecoveryStrategy default_strategy);
    
    /**
     * @brief 注册恢复策略
     * 
     * @param recovery 错误恢复器实例
     * @param error_type 错误类型
     * @param strategy 恢复策略
     * @return bool 注册成功返回true，否则返回false
     */
    bool (*register_recovery_strategy)(Stru_ErrorRecoveryInterface_t* recovery,
                                      const char* error_type,
                                      const Stru_ErrorRecoveryStrategy_t* strategy);
    
    /**
     * @brief 执行错误恢复
     * 
     * @param recovery 错误恢复器实例
     * @param error 错误信息
     * @param context 错误上下文
     * @param recovery_result 输出参数：恢复结果
     * @return bool 恢复成功返回true，否则返回false
     */
    bool (*perform_recovery)(Stru_ErrorRecoveryInterface_t* recovery,
                            const Stru_DetailedError_t* error,
                            const Stru_ErrorContext_t* context,
                            void** recovery_result);
    
    /**
     * @brief 销毁错误恢复器
     * 
     * @param recovery 错误恢复器实例
     */
    void (*destroy)(Stru_ErrorRecoveryInterface_t* recovery);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 错误上下文接口
// ============================================================================

/**
 * @brief 错误上下文接口
 
 * 负责捕获和管理错误发生时的上下文信息。
 */
struct Stru_ErrorContextInterface_t
{
    /**
     * @brief 初始化错误上下文管理器
     * 
     * @param context 错误上下文管理器实例
     * @param enable_snapshots 是否启用状态快照
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_ErrorContextInterface_t* context,
                      bool enable_snapshots);
    
    /**
     * @brief 捕获当前上下文
     * 
     * @param context 错误上下文管理器实例
     * @param phase 当前编译阶段
     * @param pass 当前处理过程
     * @param captured_context 输出参数：捕获的上下文
     * @return bool 捕获成功返回true，否则返回false
     */
    bool (*capture_context)(Stru_ErrorContextInterface_t* context,
                           const char* phase,
                           const char* pass,
                           Stru_ErrorContext_t* captured_context);
    
    /**
     * @brief 销毁错误上下文管理器
     * 
     * @param context 错误上下文管理器实例
     */
    void (*destroy)(Stru_ErrorContextInterface_t* context);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
};

// ============================================================================
// 错误统计接口
// ============================================================================

/**
 * @brief 错误统计接口
 
 * 负责统计和分析错误信息，提供错误频率、分布、趋势等统计信息。
 */
struct Stru_ErrorStatisticsInterface_t
{
    /**
     * @brief 初始化错误统计器
     * 
     * @param statistics 错误统计器实例
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_ErrorStatisticsInterface_t* statistics);
    
    /**
     * @brief 添加错误到统计
     * 
     * @param statistics 错误统计器实例
     * @param error 错误信息
     * @param context 错误上下文
     * @return bool 添加成功返回true，否则返回false
     */
    bool (*add_error)(Stru_ErrorStatisticsInterface_t* statistics,
                     const Stru_DetailedError_t* error,
                     const Stru_ErrorContext_t* context);
    
    /**
     * @brief 获取错误频率统计
     * 
     * @param statistics 错误统计器实例
     * @param error_type_counts 输出参数：错误类型计数
     * @param max_types 最大错误类型数量
     * @return size_t 实际错误类型数量
     */
    size_t (*get_error_frequency)(Stru_ErrorStatisticsInterface_t* statistics,
                                 void** error_type_counts,
                                 size_t max_types);
    
    /**
     * @brief 获取错误分布统计
     * 
     * @param statistics 错误统计器实例
     * @param file_distribution 输出参数：文件分布
     * @param module_distribution 输出参数：模块分布
     * @param severity_distribution 输出参数：严重性分布
     * @return bool 获取成功返回true，否则返回false
     */
    bool (*get_error_distribution)(Stru_ErrorStatisticsInterface_t* statistics,
                                  void** file_distribution,
                                  void** module_distribution,
                                  void** severity_distribution);
    
    /**
     * @brief 获取错误趋势分析
     * 
     * @param statistics 错误统计器实例
     * @param time_period 时间周期（秒）
     * @param trend_data 输出参数：趋势数据
     * @param max_data_points 最大数据点数量
     * @return size_t 实际数据点数量
     */
    size_t (*get_error_trend)(Stru_ErrorStatisticsInterface_t* statistics,
                             time_t time_period,
                             void** trend_data,
                             size_t max_data_points);
    
    /**
     * @brief 销毁错误统计器
     * 
     * @param statistics 错误统计器实例
     */
    void (*destroy)(Stru_ErrorStatisticsInterface_t* statistics);
    
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
 * @brief 创建详细的错误报告器接口实例
 * 
 * @return Stru_DetailedErrorReporterInterface_t* 错误报告器接口实例，失败返回NULL
 */
Stru_DetailedErrorReporterInterface_t* F_create_detailed_error_reporter_interface(void);

/**
 * @brief 创建错误恢复接口实例
 * 
 * @return Stru_ErrorRecoveryInterface_t* 错误恢复接口实例，失败返回NULL
 */
Stru_ErrorRecoveryInterface_t* F_create_error_recovery_interface(void);

/**
 * @brief 创建错误上下文接口实例
 * 
 * @return Stru_ErrorContextInterface_t* 错误上下文接口实例，失败返回NULL
 */
Stru_ErrorContextInterface_t* F_create_error_context_interface(void);

/**
 * @brief 创建错误统计接口实例
 * 
 * @return Stru_ErrorStatisticsInterface_t* 错误统计接口实例，失败返回NULL
 */
Stru_ErrorStatisticsInterface_t* F_create_error_statistics_interface(void);

/**
 * @brief 销毁详细的错误报告器接口实例
 * 
 * @param interface 错误报告器接口实例
 */
void F_destroy_detailed_error_reporter_interface(Stru_DetailedErrorReporterInterface_t* interface);

/**
 * @brief 销毁错误恢复接口实例
 * 
 * @param interface 错误恢复接口实例
 */
void F_destroy_error_recovery_interface(Stru_ErrorRecoveryInterface_t* interface);

/**
 * @brief 销毁错误上下文接口实例
 * 
 * @param interface 错误上下文接口实例
 */
void F_destroy_error_context_interface(Stru_ErrorContextInterface_t* interface);

/**
 * @brief 销毁错误统计接口实例
 * 
 * @param interface 错误统计接口实例
 */
void F_destroy_error_statistics_interface(Stru_ErrorStatisticsInterface_t* interface);

#ifdef __cplusplus
}
#endif

#endif /* CN_ERROR_RECOVERY_H */
