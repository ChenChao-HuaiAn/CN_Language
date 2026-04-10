#ifndef CN_SUPPORT_DIAGNOSTICS_H
#define CN_SUPPORT_DIAGNOSTICS_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* 跨平台 strdup 支持 */
#ifdef _MSC_VER
#define cn_strdup _strdup
#else
#define cn_strdup strdup
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 诊断严重级别 ==================== */

/**
 * @brief 诊断严重级别枚举
 *
 * 定义诊断信息的严重程度
 */
typedef enum CnDiagSeverity {
    CN_DIAG_SEVERITY_ERROR = 0,     /**< 错误：阻止编译继续 */
    CN_DIAG_SEVERITY_WARNING = 1    /**< 警告：不影响编译 */
} CnDiagSeverity;

/* ==================== 诊断错误码 ==================== */

/**
 * @brief 诊断错误码枚举
 *
 * 定义所有支持的诊断错误码，用于消息表查找
 */
typedef enum CnDiagCode {
    CN_DIAG_CODE_UNKNOWN = 0,
    CN_DIAG_CODE_LEX_UNTERMINATED_STRING,
    CN_DIAG_CODE_LEX_INVALID_CHAR,
    CN_DIAG_CODE_LEX_INVALID_HEX,          // 无效的十六进制字面量
    CN_DIAG_CODE_LEX_INVALID_BINARY,       // 无效的二进制字面量
    CN_DIAG_CODE_LEX_INVALID_OCTAL,        // 无效的八进制字面量
    CN_DIAG_CODE_LEX_LITERAL_OVERFLOW,     // 字面量溢出
    CN_DIAG_CODE_LEX_UNTERMINATED_BLOCK_COMMENT,  // 未终止的块注释
    CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
    CN_DIAG_CODE_PARSE_RESERVED_FEATURE,  // 预留特性暂不支持
    CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
    CN_DIAG_CODE_PARSE_INVALID_PARAM,
    CN_DIAG_CODE_PARSE_INVALID_VAR_DECL,
    CN_DIAG_CODE_PARSE_INVALID_COMPARISON_OP,
    CN_DIAG_CODE_PARSE_INVALID_EXPR,
    CN_DIAG_CODE_SEM_DUPLICATE_SYMBOL,
    CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
    CN_DIAG_CODE_SEM_TYPE_MISMATCH,
    CN_DIAG_CODE_SEM_ARGUMENT_COUNT_MISMATCH,
    CN_DIAG_CODE_SEM_ARGUMENT_TYPE_MISMATCH,
    CN_DIAG_CODE_SEM_RETURN_OUTSIDE_FUNCTION,
    CN_DIAG_CODE_SEM_BREAK_CONTINUE_OUTSIDE_LOOP,
    CN_DIAG_CODE_SEM_MISSING_RETURN,
    CN_DIAG_CODE_SEM_INVALID_ASSIGNMENT,
    CN_DIAG_CODE_SEM_NOT_CALLABLE,
    CN_DIAG_CODE_SEM_NOT_STRUCT,
    CN_DIAG_CODE_SEM_MEMBER_NOT_FOUND,
    CN_DIAG_CODE_SEM_ACCESS_DENIED,  // 访问权限被拒绝
    // 常量相关语义错误
    CN_DIAG_CODE_SEM_CONST_NO_INITIALIZER,    // 常量声明缺少初始化表达式
    CN_DIAG_CODE_SEM_CONST_NON_CONST_INIT,    // 常量初始化表达式不是编译时常量
    CN_DIAG_CODE_SEM_SWITCH_CASE_NON_CONST,   // switch case 值不是常量表达式
    CN_DIAG_CODE_SEM_SWITCH_CASE_DUPLICATE,   // switch case 值重复
    // 静态变量相关语义错误
    CN_DIAG_CODE_SEM_STATIC_NON_CONST_INIT,   // 静态变量初始化表达式不是编译时常量
    CN_DIAG_CODE_SEM_STATIC_VOID_TYPE,        // 静态变量为 void 类型
    // 自身指针相关语义错误
    CN_DIAG_CODE_SEM_INVALID_THIS_USAGE,      // 自身指针使用无效（在静态方法或全局函数中使用）
    // 抽象类相关语义错误
    CN_DIAG_CODE_SEM_ABSTRACT_INSTANTIATION,  // 尝试实例化抽象类
    CN_DIAG_CODE_SEM_PURE_VIRTUAL_NOT_IMPL,   // 派生类未实现所有纯虚函数
    CN_DIAG_CODE_SEM_PURE_VIRTUAL_CALL        // 调用纯虚函数
} CnDiagCode;

/* ==================== 前向声明 ==================== */

/**
 * @brief 语言类型枚举（前向声明）
 *
 * 完整定义在 diag_message_table.h 中
 */
typedef enum CnDiagLanguage CnDiagLanguage;

/**
 * @brief 参数数组结构（前向声明）
 *
 * 完整定义在 diag_message_table.h 中
 */
typedef struct CnDiagArgs CnDiagArgs;

/* ==================== 基础诊断结构 ==================== */

/**
 * @brief 单条诊断信息结构
 *
 * 基础诊断信息，用于简单的错误报告
 */
typedef struct CnDiagnostic {
    CnDiagSeverity severity;    /**< 严重级别 */
    CnDiagCode code;            /**< 错误码 */
    const char *filename;       /**< 文件名（可为NULL，表示未知文件） */
    int line;                   /**< 行号，从1开始；0表示未知 */
    int column;                 /**< 列号，从1开始；0表示未知 */
    const char *message;        /**< 消息（不拥有所有权，一般为静态字符串） */
} CnDiagnostic;

/* ==================== 增强诊断结构 ==================== */

/**
 * @brief 增强的诊断信息结构
 *
 * 支持双语消息、参数化消息和修复建议
 */
typedef struct CnDiagnosticEx {
    CnDiagSeverity severity;    /**< 严重级别 */
    CnDiagCode code;            /**< 错误码 */
    const char *filename;       /**< 文件名 */
    int line;                   /**< 行号 */
    int column;                 /**< 列号 */
    int end_line;               /**< 结束行号（用于多行错误） */
    int end_column;             /**< 结束列号 */
    CnDiagArgs *args;           /**< 参数化消息参数（可为NULL） */
    const char *suggestion;     /**< 修复建议（可为NULL） */
} CnDiagnosticEx;

/* ==================== 诊断配置结构 ==================== */

/**
 * @brief 诊断配置结构
 *
 * 控制诊断系统的行为
 */
typedef struct CnDiagConfig {
    CnDiagLanguage language;    /**< 输出语言 */
    int max_errors;             /**< 最大错误数（0表示无限制） */
    int enable_suggestions;     /**< 是否启用修复建议 */
    int enable_recovery;        /**< 是否启用智能恢复 */
} CnDiagConfig;

// 诊断集合
struct CnDiagnostics {
    CnDiagnostic *items;
    size_t count;
    size_t capacity;
};

typedef struct CnDiagnostics CnDiagnostics;

// 初始化诊断集合
static inline void cn_support_diagnostics_init(CnDiagnostics *diagnostics)
{
    if (!diagnostics) {
        return;
    }
    diagnostics->items = NULL;
    diagnostics->count = 0;
    diagnostics->capacity = 0;
}

// 释放诊断集合内部资源
static inline void cn_support_diagnostics_free(CnDiagnostics *diagnostics)
{
    size_t i;
    if (!diagnostics) {
        return;
    }
    // 释放每个诊断项的文件名和消息字符串
    for (i = 0; i < diagnostics->count; i++) {
        if (diagnostics->items[i].filename) {
            free((void*)diagnostics->items[i].filename);
        }
        if (diagnostics->items[i].message) {
            free((void*)diagnostics->items[i].message);
        }
    }
    free(diagnostics->items);
    diagnostics->items = NULL;
    diagnostics->count = 0;
    diagnostics->capacity = 0;
}

// 追加一条诊断信息
static inline void cn_support_diagnostics_report(
    CnDiagnostics *diagnostics,
    CnDiagSeverity severity,
    CnDiagCode code,
    const char *filename,
    int line,
    int column,
    const char *message)
{
    CnDiagnostic *new_items;
    size_t new_capacity;

    if (!diagnostics) {
        return;
    }

    if (diagnostics->count >= diagnostics->capacity) {
        new_capacity = diagnostics->capacity == 0 ? 4 : diagnostics->capacity * 2;
        new_items = (CnDiagnostic *)realloc(diagnostics->items,
                                            new_capacity * sizeof(CnDiagnostic));
        if (!new_items) {
            return;
        }
        diagnostics->items = new_items;
        diagnostics->capacity = new_capacity;
    }

    diagnostics->items[diagnostics->count].severity = severity;
    diagnostics->items[diagnostics->count].code = code;
    // 使用 cn_strdup 复制文件名和消息字符串，避免指针悬空问题
    diagnostics->items[diagnostics->count].filename = filename ? cn_strdup(filename) : NULL;
    diagnostics->items[diagnostics->count].line = line;
    diagnostics->items[diagnostics->count].column = column;
    diagnostics->items[diagnostics->count].message = message ? cn_strdup(message) : NULL;
    diagnostics->count += 1;
}

// 便利函数：报告错误
void cn_support_diagnostics_report_error(
    CnDiagnostics *diagnostics,
    CnDiagCode code,
    const char *filename,
    int line,
    int column,
    const char *message);

// 便利函数：报告警告
void cn_support_diagnostics_report_warning(
    CnDiagnostics *diagnostics,
    CnDiagCode code,
    const char *filename,
    int line,
    int column,
    const char *message);

// 便利函数：获取错误数量
size_t cn_support_diagnostics_error_count(const CnDiagnostics *diagnostics);

// 语义分析专用诊断接口
// 注意：symbol_name 可以是非 null 结尾的字符串，需要配合 symbol_name_length 使用
void cn_support_diag_semantic_error_duplicate_symbol(
    CnDiagnostics *diagnostics,
    const char *filename,
    int line,
    int column,
    const char *symbol_name,
    size_t symbol_name_length);

void cn_support_diag_semantic_error_undefined_identifier(
    CnDiagnostics *diagnostics,
    const char *filename,
    int line,
    int column,
    const char *identifier);

void cn_support_diag_semantic_error_type_mismatch(
    CnDiagnostics *diagnostics,
    const char *filename,
    int line,
    int column,
    const char *expected_type,
    const char *actual_type);

void cn_support_diag_semantic_error_missing_return(
    CnDiagnostics *diagnostics,
    const char *filename,
    int line,
    int column,
    const char *function_name);

void cn_support_diag_semantic_error_generic(
    CnDiagnostics *diagnostics,
    CnDiagCode code,
    const char *filename,
    int line,
    int column,
    const char *message);

// 常量相关语义错误诊断接口

void cn_support_diag_semantic_error_const_no_initializer(
    CnDiagnostics *diagnostics,
    const char *filename,
    int line,
    int column,
    const char *const_name);

void cn_support_diag_semantic_error_const_non_const_init(
    CnDiagnostics *diagnostics,
    const char *filename,
    int line,
    int column,
    const char *const_name);

void cn_support_diag_semantic_error_switch_case_non_const(
    CnDiagnostics *diagnostics,
    const char *filename,
    int line,
    int column);

void cn_support_diag_semantic_error_switch_case_duplicate(
    CnDiagnostics *diagnostics,
    const char *filename,
    int line,
    int column,
    int case_value);

/**
 * @brief 便利函数：打印所有诊断信息
 * @param diagnostics 诊断集合
 */
void cn_support_diagnostics_print(const CnDiagnostics *diagnostics);

/* ==================== 增强诊断接口 ==================== */

/**
 * @brief 报告增强诊断信息
 *
 * 支持参数化消息和修复建议的诊断报告接口
 *
 * @param ctx 诊断上下文（CnDiagnostics指针）
 * @param severity 严重级别
 * @param code 错误码
 * @param filename 文件名
 * @param line 行号
 * @param column 列号
 * @param args 参数数组（可为NULL）
 * @param suggestion 修复建议（可为NULL）
 */
void cn_diag_report_ex(void *ctx,
                       CnDiagSeverity severity,
                       CnDiagCode code,
                       const char *filename,
                       int line,
                       int column,
                       const CnDiagArgs *args,
                       const char *suggestion);

/**
 * @brief 获取诊断配置
 *
 * 返回当前诊断系统的配置信息
 *
 * @return 诊断配置指针
 */
CnDiagConfig* cn_diag_get_config(void);

/**
 * @brief 设置诊断配置
 *
 * 更新诊断系统的配置信息
 *
 * @param config 新配置
 */
void cn_diag_set_config(const CnDiagConfig *config);

/**
 * @brief 打印增强诊断信息
 *
 * 格式化输出增强诊断信息，支持双语消息
 *
 * @param diag 增强诊断信息
 */
void cn_diag_print_ex(const CnDiagnosticEx *diag);

/**
 * @brief 检查是否应继续诊断
 *
 * 根据错误计数判断是否应继续编译
 *
 * @return 1表示继续，0表示应停止
 */
int cn_diag_should_continue(void);

/**
 * @brief 重置诊断状态
 *
 * 清空错误计数器，重置诊断系统状态
 */
void cn_diag_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* CN_SUPPORT_DIAGNOSTICS_H */
