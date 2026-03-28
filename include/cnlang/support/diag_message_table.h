/**
 * @file diag_message_table.h
 * @brief 错误诊断消息表头文件
 * 
 * 提供中英文双语错误消息支持，包含参数化消息模板和格式化功能。
 * 与 diagnostics.h 配合使用，增强错误诊断的用户体验。
 * 
 * @version 1.0
 * @date 2026-03-28
 */

#ifndef CNLANG_SUPPORT_DIAG_MESSAGE_TABLE_H
#define CNLANG_SUPPORT_DIAG_MESSAGE_TABLE_H

#include "diagnostics.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 语言类型枚举 ==================== */

/**
 * @brief 诊断消息语言类型枚举
 * 
 * 用于选择错误消息的显示语言
 */
typedef enum CnDiagLanguage {
    CN_DIAG_LANG_ZH = 0,  /**< 中文（默认） */
    CN_DIAG_LANG_EN = 1   /**< 英文 */
} CnDiagLanguage;

/* ==================== 参数类型定义 ==================== */

/**
 * @brief 参数类型枚举
 * 
 * 定义消息模板中可使用的参数类型
 */
typedef enum CnDiagArgType {
    CN_DIAG_ARG_TYPE_STRING,   /**< 字符串参数（如标识符名称） */
    CN_DIAG_ARG_TYPE_INT,      /**< 整数参数（如行号、数量） */
    CN_DIAG_ARG_TYPE_TOKEN     /**< Token类型参数（如关键字） */
} CnDiagArgType;

/**
 * @brief 参数值联合体
 * 
 * 存储不同类型参数的实际值
 */
typedef union CnDiagArgValue {
    const char *str_val;    /**< 字符串值 */
    int int_val;            /**< 整数值 */
    int token_val;          /**< Token类型值 */
} CnDiagArgValue;

/**
 * @brief 单个参数结构
 * 
 * 包含参数类型和参数值的完整参数描述
 */
typedef struct CnDiagArg {
    CnDiagArgType type;     /**< 参数类型 */
    CnDiagArgValue value;   /**< 参数值 */
} CnDiagArg;

/**
 * @brief 参数数组结构
 * 
 * 用于传递多个参数给消息格式化函数
 */
typedef struct CnDiagArgs {
    const CnDiagArg *args;  /**< 参数数组指针 */
    int count;              /**< 参数数量 */
} CnDiagArgs;

/* ==================== 消息模板结构 ==================== */

/**
 * @brief 消息模板结构
 * 
 * 存储单个错误码对应的中英文消息模板和提示信息。
 * 消息模板使用 {0}, {1}, {2} 等占位符表示参数位置。
 * 
 * @example
 * // 模板示例
 * message_zh = "未定义的标识符 '{0}'"
 * message_en = "Undefined identifier '{0}'"
 * hint_zh = "请检查标识符是否正确拼写，或是否需要导入模块"
 * hint_en = "Check if the identifier is spelled correctly, or if a module import is needed"
 */
typedef struct CnDiagMessageTemplate {
    CnDiagCode code;        /**< 错误码，与 diagnostics.h 中的 CnDiagCode 对应 */
    const char *message_zh; /**< 中文消息模板，使用 {0}, {1} 等占位符 */
    const char *message_en; /**< 英文消息模板 */
    const char *hint_zh;    /**< 中文提示（可选，可为NULL） */
    const char *hint_en;    /**< 英文提示（可选，可为NULL） */
} CnDiagMessageTemplate;

/* ==================== 接口函数声明 ==================== */

/**
 * @brief 设置诊断消息语言
 * 
 * 设置全局诊断消息的显示语言，影响后续所有错误消息的输出。
 * 
 * @param lang 语言类型（CN_DIAG_LANG_ZH 或 CN_DIAG_LANG_EN）
 */
void cn_diag_set_language(CnDiagLanguage lang);

/**
 * @brief 获取当前诊断消息语言
 * 
 * @return 当前语言类型
 */
CnDiagLanguage cn_diag_get_language(void);

/**
 * @brief 根据错误码获取消息模板
 * 
 * 在消息表中查找指定错误码对应的消息模板。
 * 
 * @param code 错误码
 * @return 消息模板指针，未找到返回NULL
 */
const CnDiagMessageTemplate* cn_diag_get_template(CnDiagCode code);

/**
 * @brief 格式化消息
 * 
 * 根据消息模板和参数生成完整的错误消息字符串。
 * 模板中的 {0}, {1}, {2} 等占位符会被对应参数替换。
 * 
 * @param template 消息模板指针
 * @param args 参数数组（可为NULL，表示无参数消息）
 * @param lang 语言类型
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小（字节）
 * @return 格式化后的字符串长度（不含终止符），失败返回负值
 * 
 * @example
 * // 使用示例
 * const CnDiagMessageTemplate *tmpl = cn_diag_get_template(CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER);
 * CnDiagArg args[1] = { cn_diag_arg_string("变量名") };
 * CnDiagArgs arg_list = { args, 1 };
 * char buffer[256];
 * cn_diag_format_message(tmpl, &arg_list, CN_DIAG_LANG_ZH, buffer, sizeof(buffer));
 * // buffer 内容: "未定义的标识符 '变量名'"
 */
int cn_diag_format_message(const CnDiagMessageTemplate *template,
                           const CnDiagArgs *args,
                           CnDiagLanguage lang,
                           char *buffer,
                           int buffer_size);

/* ==================== 便利函数声明 ==================== */

/**
 * @brief 创建字符串参数
 * 
 * 便利函数，用于快速创建字符串类型的诊断参数。
 * 
 * @param value 字符串值（指针，不拥有所有权）
 * @return 初始化完成的 CnDiagArg 结构
 */
CnDiagArg cn_diag_arg_string(const char *value);

/**
 * @brief 创建整数参数
 * 
 * 便利函数，用于快速创建整数类型的诊断参数。
 * 
 * @param value 整数值
 * @return 初始化完成的 CnDiagArg 结构
 */
CnDiagArg cn_diag_arg_int(int value);

/**
 * @brief 创建Token参数
 * 
 * 便利函数，用于快速创建Token类型的诊断参数。
 * 
 * @param token_type Token类型值
 * @return 初始化完成的 CnDiagArg 结构
 */
CnDiagArg cn_diag_arg_token(int token_type);

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_SUPPORT_DIAG_MESSAGE_TABLE_H */
