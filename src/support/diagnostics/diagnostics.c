#include "cnlang/support/diagnostics.h"
#include <stdio.h>

// 便利函数：报告错误
void cn_support_diagnostics_report_error(
    CnDiagnostics *diagnostics,
    CnDiagCode code,
    const char *filename,
    int line,
    int column,
    const char *message)
{
    cn_support_diagnostics_report(
        diagnostics,
        CN_DIAG_SEVERITY_ERROR,
        code,
        filename,
        line,
        column,
        message);
}

// 便利函数：报告警告
void cn_support_diagnostics_report_warning(
    CnDiagnostics *diagnostics,
    CnDiagCode code,
    const char *filename,
    int line,
    int column,
    const char *message)
{
    cn_support_diagnostics_report(
        diagnostics,
        CN_DIAG_SEVERITY_WARNING,
        code,
        filename,
        line,
        column,
        message);
}

// 便利函数：获取错误数量
size_t cn_support_diagnostics_error_count(const CnDiagnostics *diagnostics)
{
    size_t error_count = 0;
    size_t i;

    if (!diagnostics) {
        return 0;
    }

    for (i = 0; i < diagnostics->count; i++) {
        if (diagnostics->items[i].severity == CN_DIAG_SEVERITY_ERROR) {
            error_count++;
        }
    }

    return error_count;
}

// 语义分析专用诊断接口实现

void cn_support_diag_semantic_error_duplicate_symbol(
    CnDiagnostics *diagnostics,
    const char *filename,
    int line,
    int column,
    const char *symbol_name)
{
    (void)symbol_name; // 目前尚不支持动态消息，忽略符号名
    cn_support_diagnostics_report_error(
        diagnostics,
        CN_DIAG_CODE_SEM_DUPLICATE_SYMBOL,
        filename,
        line,
        column,
        "语义错误：符号重复定义");
}

void cn_support_diag_semantic_error_undefined_identifier(
    CnDiagnostics *diagnostics,
    const char *filename,
    int line,
    int column,
    const char *identifier)
{
    (void)identifier; // 目前尚不支持动态消息，忽略标识符名
    cn_support_diagnostics_report_error(
        diagnostics,
        CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
        filename,
        line,
        column,
        "语义错误：未定义的标识符");
}

void cn_support_diag_semantic_error_type_mismatch(
    CnDiagnostics *diagnostics,
    const char *filename,
    int line,
    int column,
    const char *expected_type,
    const char *actual_type)
{
    (void)expected_type;
    (void)actual_type;
    cn_support_diagnostics_report_error(
        diagnostics,
        CN_DIAG_CODE_SEM_TYPE_MISMATCH,
        filename,
        line,
        column,
        "语义错误：类型不匹配");
}

void cn_support_diag_semantic_error_missing_return(
    CnDiagnostics *diagnostics,
    const char *filename,
    int line,
    int column,
    const char *function_name)
{
    (void)function_name;
    cn_support_diagnostics_report_error(
        diagnostics,
        CN_DIAG_CODE_SEM_MISSING_RETURN,
        filename,
        line,
        column,
        "语义错误：函数缺少返回语句");
}

void cn_support_diag_semantic_error_generic(
    CnDiagnostics *diagnostics,
    CnDiagCode code,
    const char *filename,
    int line,
    int column,
    const char *message)
{
    cn_support_diagnostics_report_error(
        diagnostics,
        code,
        filename,
        line,
        column,
        message);
}

// 便利函数：打印所有诊断信息到标准错误输出
void cn_support_diagnostics_print(const CnDiagnostics *diagnostics)
{
    size_t i;
    const CnDiagnostic *diag;
    const char *severity_str;

    if (!diagnostics) {
        return;
    }

    for (i = 0; i < diagnostics->count; i++) {
        diag = &diagnostics->items[i];

        // 确定严重级别字符串
        severity_str = (diag->severity == CN_DIAG_SEVERITY_ERROR) ? "错误" : "警告";

        // 打印诊断信息（中文格式）
        if (diag->filename && diag->line > 0 && diag->column > 0) {
            fprintf(stderr, "%s:%d:%d: %s: %s (代码: %d)\n",
                    diag->filename,
                    diag->line,
                    diag->column,
                    severity_str,
                    diag->message ? diag->message : "未知错误",
                    diag->code);
        } else if (diag->filename && diag->line > 0) {
            fprintf(stderr, "%s:%d: %s: %s (代码: %d)\n",
                    diag->filename,
                    diag->line,
                    severity_str,
                    diag->message ? diag->message : "未知错误",
                    diag->code);
        } else if (diag->filename) {
            fprintf(stderr, "%s: %s: %s (代码: %d)\n",
                    diag->filename,
                    severity_str,
                    diag->message ? diag->message : "未知错误",
                    diag->code);
        } else {
            fprintf(stderr, "%s: %s (代码: %d)\n",
                    severity_str,
                    diag->message ? diag->message : "未知错误",
                    diag->code);
        }
    }
}
