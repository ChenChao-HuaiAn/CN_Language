/**
 * @file diagnostics.c
 * @brief 诊断系统实现
 *
 * 实现诊断信息的收集、管理和输出功能。
 * 支持中英文双语消息、参数化消息和修复建议。
 */

#include "cnlang/support/diagnostics.h"
#include "cnlang/support/diag_message_table.h"
#include <stdio.h>
#include <string.h>

/* ==================== 全局诊断配置 ==================== */

/**
 * @brief 全局诊断配置
 *
 * 存储诊断系统的全局设置，包括语言、错误限制等
 */
static CnDiagConfig g_diag_config = {
    CN_DIAG_LANG_ZH,  /* 默认中文 */
    0,                /* 无错误限制 */
    1,                /* 启用建议 */
    1                 /* 启用恢复 */
};

/**
 * @brief 错误计数器
 *
 * 记录当前编译过程中遇到的错误数量
 */
static int g_error_count = 0;

/* ==================== 配置函数实现 ==================== */

/**
 * @brief 获取诊断配置
 */
CnDiagConfig* cn_diag_get_config(void) {
    return &g_diag_config;
}

/**
 * @brief 设置诊断配置
 */
void cn_diag_set_config(const CnDiagConfig *config) {
    if (config) {
        g_diag_config = *config;
        /* 同步语言设置 */
        cn_diag_set_language(config->language);
    }
}

/* ==================== 增强诊断函数实现 ==================== */

/**
 * @brief 报告增强诊断信息
 *
 * 支持参数化消息和修复建议的诊断报告接口
 */
void cn_diag_report_ex(void *ctx,
                       CnDiagSeverity severity,
                       CnDiagCode code,
                       const char *filename,
                       int line,
                       int column,
                       const CnDiagArgs *args,
                       const char *suggestion) {
    const CnDiagMessageTemplate *tmpl;
    char message[512];
    CnDiagnosticEx diag;
    CnDiagArgs empty_args = {NULL, 0};
    
    /* 1. 检查错误计数 */
    if (severity == CN_DIAG_SEVERITY_ERROR) {
        g_error_count++;
        if (g_diag_config.max_errors > 0 &&
            g_error_count > g_diag_config.max_errors) {
            return; /* 达到最大错误数，停止报告 */
        }
    }
    
    /* 2. 获取消息模板 */
    tmpl = cn_diag_get_template(code);
    
    /* 3. 格式化消息 */
    if (tmpl) {
        cn_diag_format_message(tmpl, args, g_diag_config.language,
                               message, sizeof(message));
    } else {
        snprintf(message, sizeof(message), "未知错误 (代码: %d)", code);
    }
    
    /* 4. 构建诊断结构并输出 */
    diag.severity = severity;
    diag.code = code;
    diag.filename = filename;
    diag.line = line;
    diag.column = column;
    diag.end_line = line;
    diag.end_column = column;
    diag.args = args ? (CnDiagArgs*)args : &empty_args;
    diag.suggestion = suggestion;
    
    /* 5. 如果有上下文，也添加到诊断集合 */
    if (ctx) {
        CnDiagnostics *diagnostics = (CnDiagnostics *)ctx;
        cn_support_diagnostics_report(
            diagnostics,
            severity,
            code,
            filename,
            line,
            column,
            message);
    }
    
    /* 6. 打印诊断信息 */
    cn_diag_print_ex(&diag);
}

/**
 * @brief 打印增强诊断信息
 *
 * 格式化输出增强诊断信息，支持双语消息
 */
void cn_diag_print_ex(const CnDiagnosticEx *diag) {
    const char *severity_str;
    const CnDiagMessageTemplate *tmpl;
    char message[512];
    const char *hint;
    
    if (!diag) return;
    
    /* 确定严重级别字符串 */
    severity_str = (diag->severity == CN_DIAG_SEVERITY_ERROR) ? "错误" : "警告";
    
    /* 输出位置信息 */
    if (diag->filename && diag->line > 0 && diag->column > 0) {
        fprintf(stderr, "%s:%d:%d: %s: ",
                diag->filename,
                diag->line,
                diag->column,
                severity_str);
    } else if (diag->filename && diag->line > 0) {
        fprintf(stderr, "%s:%d: %s: ",
                diag->filename,
                diag->line,
                severity_str);
    } else if (diag->filename) {
        fprintf(stderr, "%s: %s: ",
                diag->filename,
                severity_str);
    } else {
        fprintf(stderr, "%s: ", severity_str);
    }
    
    /* 获取并格式化消息 */
    tmpl = cn_diag_get_template(diag->code);
    if (tmpl) {
        cn_diag_format_message(tmpl, diag->args, g_diag_config.language,
                               message, sizeof(message));
        fprintf(stderr, "%s\n", message);
        
        /* 输出提示信息 */
        hint = (g_diag_config.language == CN_DIAG_LANG_ZH)
               ? tmpl->hint_zh : tmpl->hint_en;
        if (hint && g_diag_config.enable_suggestions) {
            fprintf(stderr, "  提示: %s\n", hint);
        }
    } else {
        fprintf(stderr, "未知错误 (代码: %d)\n", diag->code);
    }
    
    /* 输出修复建议 */
    if (diag->suggestion && g_diag_config.enable_suggestions) {
        fprintf(stderr, "  建议: %s\n", diag->suggestion);
    }
}

/**
 * @brief 检查是否应继续诊断
 *
 * 根据错误计数判断是否应继续编译
 */
int cn_diag_should_continue(void) {
    if (g_diag_config.max_errors <= 0) return 1;
    return g_error_count < g_diag_config.max_errors;
}

/**
 * @brief 重置诊断状态
 *
 * 清空错误计数器，重置诊断系统状态
 */
void cn_diag_reset(void) {
    g_error_count = 0;
}

/* ==================== 原有便利函数实现 ==================== */

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
    if (!diagnostics) return;
    
    // 使用静态缓冲区存储错误消息（避免局部变量被释放）
    static char error_msg[256];
    
    if (identifier && identifier[0] != '\0') {
        snprintf(error_msg, sizeof(error_msg), "语义错误：未定义的标识符 '%s'", identifier);
    } else {
        snprintf(error_msg, sizeof(error_msg), "语义错误：未定义的标识符");
    }
    
    cn_support_diagnostics_report_error(
        diagnostics,
        CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
        filename ? filename : "<未知文件>",
        line,
        column,
        error_msg);
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
    if (!diagnostics) return;
    cn_support_diagnostics_report_error(
        diagnostics,
        code,
        filename ? filename : "<未知文件>",
        line,
        column,
        message ? message : "语义错误");
}

// 常量相关语义错误诊断接口

void cn_support_diag_semantic_error_const_no_initializer(
    CnDiagnostics *diagnostics,
    const char *filename,
    int line,
    int column,
    const char *const_name)
{
    (void)const_name;
    cn_support_diagnostics_report_error(
        diagnostics,
        CN_DIAG_CODE_SEM_CONST_NO_INITIALIZER,
        filename,
        line,
        column,
        "语义错误：常量声明必须包含初始化表达式");
}

void cn_support_diag_semantic_error_const_non_const_init(
    CnDiagnostics *diagnostics,
    const char *filename,
    int line,
    int column,
    const char *const_name)
{
    (void)const_name;
    cn_support_diagnostics_report_error(
        diagnostics,
        CN_DIAG_CODE_SEM_CONST_NON_CONST_INIT,
        filename,
        line,
        column,
        "语义错误：常量初始化表达式必须是编译时常量");
}

void cn_support_diag_semantic_error_switch_case_non_const(
    CnDiagnostics *diagnostics,
    const char *filename,
    int line,
    int column)
{
    cn_support_diagnostics_report_error(
        diagnostics,
        CN_DIAG_CODE_SEM_SWITCH_CASE_NON_CONST,
        filename,
        line,
        column,
        "语义错误：switch case 值必须是常量表达式");
}

void cn_support_diag_semantic_error_switch_case_duplicate(
    CnDiagnostics *diagnostics,
    const char *filename,
    int line,
    int column,
    int case_value)
{
    (void)case_value;
    cn_support_diagnostics_report_error(
        diagnostics,
        CN_DIAG_CODE_SEM_SWITCH_CASE_DUPLICATE,
        filename,
        line,
        column,
        "语义错误：switch case 值重复");
}

// 便利函数：打印所有诊断信息到标准错误输出
void cn_support_diagnostics_print(const CnDiagnostics *diagnostics)
{
    size_t i;
    const CnDiagnostic *diag;
    const char *severity_str;
    const char *msg;

    if (!diagnostics) {
        return;
    }

    for (i = 0; i < diagnostics->count; i++) {
        diag = &diagnostics->items[i];

        // 确定严重级别字符串
        severity_str = (diag->severity == CN_DIAG_SEVERITY_ERROR) ? "错误" : "警告";
        
        // 确定消息：处理 NULL 和空字符串的情况
        msg = diag->message;
        if (!msg || msg[0] == '\0') {
            msg = "未知错误";
        }

        // 打印诊断信息（中文格式）
        if (diag->filename && diag->line > 0 && diag->column > 0) {
            fprintf(stderr, "%s:%d:%d: %s: %s (代码: %d)\n",
                    diag->filename,
                    diag->line,
                    diag->column,
                    severity_str,
                    msg,
                    diag->code);
        } else if (diag->filename && diag->line > 0) {
            fprintf(stderr, "%s:%d: %s: %s (代码: %d)\n",
                    diag->filename,
                    diag->line,
                    severity_str,
                    msg,
                    diag->code);
        } else if (diag->filename) {
            fprintf(stderr, "%s: %s: %s (代码: %d)\n",
                    diag->filename,
                    severity_str,
                    msg,
                    diag->code);
        } else {
            fprintf(stderr, "%s: %s (代码: %d)\n",
                    severity_str,
                    msg,
                    diag->code);
        }
    }
}
