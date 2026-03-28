#ifndef CN_SUPPORT_DIAGNOSTICS_H
#define CN_SUPPORT_DIAGNOSTICS_H

#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// 诊断严重级别
typedef enum CnDiagSeverity {
    CN_DIAG_SEVERITY_ERROR = 0,
    CN_DIAG_SEVERITY_WARNING = 1
} CnDiagSeverity;

// 诊断错误码（雏形）
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
    CN_DIAG_CODE_SEM_STATIC_VOID_TYPE         // 静态变量为 void 类型
} CnDiagCode;

// 单条诊断信息
typedef struct CnDiagnostic {
    CnDiagSeverity severity;
    CnDiagCode code;
    const char *filename; // 可为 NULL，表示未知文件
    int line;             // 行号，从 1 开始；0 表示未知
    int column;           // 列号，从 1 开始；0 表示未知
    const char *message;  // 不拥有所有权，一般为静态字符串
} CnDiagnostic;

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
    if (!diagnostics) {
        return;
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
    diagnostics->items[diagnostics->count].filename = filename;
    diagnostics->items[diagnostics->count].line = line;
    diagnostics->items[diagnostics->count].column = column;
    diagnostics->items[diagnostics->count].message = message;
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
void cn_support_diag_semantic_error_duplicate_symbol(
    CnDiagnostics *diagnostics,
    const char *filename,
    int line,
    int column,
    const char *symbol_name);

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

// 便利函数：打印所有诊断信息
void cn_support_diagnostics_print(const CnDiagnostics *diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* CN_SUPPORT_DIAGNOSTICS_H */
