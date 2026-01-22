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
