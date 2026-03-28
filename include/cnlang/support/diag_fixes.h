/**
 * @file diag_fixes.h
 * @brief 错误诊断修复建议头文件
 * 
 * 提供错误修复建议功能，为常见错误提供具体的修复方案。
 * 与 diagnostics.h 和 diag_message_table.h 配合使用。
 * 
 * @version 1.0
 * @date 2026-03-28
 */

#ifndef CNLANG_SUPPORT_DIAG_FIXES_H
#define CNLANG_SUPPORT_DIAG_FIXES_H

#include "diagnostics.h"
#include "diag_message_table.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 修复建议类型枚举 ==================== */

/**
 * @brief 修复建议类型枚举
 * 
 * 定义不同类型的修复操作
 */
typedef enum CnDiagFixType {
    CN_DIAG_FIX_INSERT,      /**< 插入内容（如添加缺失的分号） */
    CN_DIAG_FIX_DELETE,      /**< 删除内容（如删除多余的符号） */
    CN_DIAG_FIX_REPLACE,     /**< 替换内容（如修正拼写错误） */
    CN_DIAG_FIX_REORDER      /**< 重排序（如调整参数顺序） */
} CnDiagFixType;

/* ==================== 修复建议结构 ==================== */

/**
 * @brief 修复建议结构
 * 
 * 包含完整的修复信息，包括修复类型、位置、文本和描述
 */
typedef struct CnDiagFix {
    CnDiagFixType type;           /**< 修复类型 */
    const char *description_zh;   /**< 中文描述 */
    const char *description_en;   /**< 英文描述 */
    const char *old_text;         /**< 原文本（替换/删除时使用） */
    const char *new_text;         /**< 新文本（插入/替换时使用） */
    int insert_line;              /**< 插入行号（从1开始） */
    int insert_column;            /**< 插入列号（从1开始） */
    int end_line;                 /**< 结束行号（用于多行修复） */
    int end_column;               /**< 结束列号 */
} CnDiagFix;

/* ==================== 核心接口函数 ==================== */

/**
 * @brief 根据错误码生成修复建议
 * 
 * 根据错误码和参数生成对应的修复建议。
 * 
 * @param code 错误码
 * @param args 错误参数（可为NULL）
 * @param lang 语言类型
 * @param fix 输出的修复建议结构
 * @return 1表示有建议，0表示无建议
 * 
 * @example
 * CnDiagFix fix;
 * if (cn_diag_generate_fix(CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER, &args, CN_DIAG_LANG_ZH, &fix)) {
 *     // 使用修复建议
 * }
 */
int cn_diag_generate_fix(
    CnDiagCode code,
    const CnDiagArgs *args,
    CnDiagLanguage lang,
    CnDiagFix *fix
);

/**
 * @brief 获取修复建议的描述文本
 * 
 * 根据语言类型返回对应的描述文本。
 * 
 * @param fix 修复建议结构指针
 * @param lang 语言类型
 * @return 描述文本（中文或英文）
 */
const char* cn_diag_get_fix_description(
    const CnDiagFix *fix,
    CnDiagLanguage lang
);

/**
 * @brief 格式化修复建议为字符串
 * 
 * 将修复建议格式化为可读的字符串，用于显示或日志。
 * 
 * @param fix 修复建议结构指针
 * @param lang 语言类型
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 实际写入的字符数（不含终止符）
 * 
 * @example
 * char buffer[256];
 * cn_diag_format_fix(&fix, CN_DIAG_LANG_ZH, buffer, sizeof(buffer));
 * // 输出类似: "建议: 在行尾添加分号 ';'"
 */
int cn_diag_format_fix(
    const CnDiagFix *fix,
    CnDiagLanguage lang,
    char *buffer,
    int buffer_size
);

/* ==================== 常见错误的建议生成函数 ==================== */

/**
 * @brief 生成"缺少分号"的建议
 * 
 * 为缺少分号的语法错误生成修复建议。
 * 
 * @param fix 输出的修复建议
 * @param lang 语言类型
 * @return 1表示成功生成建议
 */
int cn_diag_fix_missing_semicolon(CnDiagFix *fix, CnDiagLanguage lang);

/**
 * @brief 生成"未定义标识符"的建议
 * 
 * 为未定义标识符错误生成修复建议，可能包括拼写修正建议。
 * 
 * @param fix 输出的修复建议
 * @param identifier 标识符名称
 * @param lang 语言类型
 * @return 1表示成功生成建议
 */
int cn_diag_fix_undefined_identifier(
    CnDiagFix *fix,
    const char *identifier,
    CnDiagLanguage lang
);

/**
 * @brief 生成"类型不匹配"的建议
 * 
 * 为类型不匹配错误生成修复建议。
 * 
 * @param fix 输出的修复建议
 * @param expected 期望的类型名称
 * @param actual 实际的类型名称
 * @param lang 语言类型
 * @return 1表示成功生成建议
 */
int cn_diag_fix_type_mismatch(
    CnDiagFix *fix,
    const char *expected,
    const char *actual,
    CnDiagLanguage lang
);

/**
 * @brief 生成"重复符号"的建议
 * 
 * 为重复定义符号错误生成修复建议。
 * 
 * @param fix 输出的修复建议
 * @param symbol_name 符号名称
 * @param first_def_line 第一次定义的行号
 * @param lang 语言类型
 * @return 1表示成功生成建议
 */
int cn_diag_fix_duplicate_symbol(
    CnDiagFix *fix,
    const char *symbol_name,
    int first_def_line,
    CnDiagLanguage lang
);

/**
 * @brief 生成"缺少返回语句"的建议
 * 
 * 为函数缺少返回语句错误生成修复建议。
 * 
 * @param fix 输出的修复建议
 * @param return_type 返回类型名称
 * @param lang 语言类型
 * @return 1表示成功生成建议
 */
int cn_diag_fix_missing_return(
    CnDiagFix *fix,
    const char *return_type,
    CnDiagLanguage lang
);

/**
 * @brief 生成"参数数量不匹配"的建议
 * 
 * 为函数调用参数数量不匹配错误生成修复建议。
 * 
 * @param fix 输出的修复建议
 * @param expected 期望的参数数量
 * @param actual 实际的参数数量
 * @param lang 语言类型
 * @return 1表示成功生成建议
 */
int cn_diag_fix_argument_count_mismatch(
    CnDiagFix *fix,
    int expected,
    int actual,
    CnDiagLanguage lang
);

/**
 * @brief 生成"常量缺少初始化"的建议
 * 
 * 为常量声明缺少初始化表达式错误生成修复建议。
 * 
 * @param fix 输出的修复建议
 * @param const_name 常量名称
 * @param lang 语言类型
 * @return 1表示成功生成建议
 */
int cn_diag_fix_const_no_initializer(
    CnDiagFix *fix,
    const char *const_name,
    CnDiagLanguage lang
);

/**
 * @brief 生成"无效赋值"的建议
 * 
 * 为无效赋值错误（如对常量赋值）生成修复建议。
 * 
 * @param fix 输出的修复建议
 * @param var_name 变量名称
 * @param is_const 是否为常量
 * @param lang 语言类型
 * @return 1表示成功生成建议
 */
int cn_diag_fix_invalid_assignment(
    CnDiagFix *fix,
    const char *var_name,
    int is_const,
    CnDiagLanguage lang
);

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_SUPPORT_DIAG_FIXES_H */
