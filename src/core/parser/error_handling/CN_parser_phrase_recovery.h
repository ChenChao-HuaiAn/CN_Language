/**
 * @file CN_parser_phrase_recovery.h
 * @brief CN_Language 语法分析器短语级错误恢复模块
 * 
 * 短语级错误恢复功能，包括插入缺失令牌、删除多余令牌、替换错误令牌等。
 * 遵循项目架构规范，提供智能的错误恢复建议。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#ifndef CN_PARSER_PHRASE_RECOVERY_H
#define CN_PARSER_PHRASE_RECOVERY_H

#include "../CN_parser_interface.h"
#include "../CN_syntax_error.h"

// ============================================
// 短语级恢复策略枚举
// ============================================

/**
 * @brief 短语级恢复策略枚举
 * 
 * 定义不同的短语级恢复策略。
 */
typedef enum Eum_PhraseRecoveryStrategy {
    Eum_RECOVERY_INSERT_TOKEN,      ///< 插入缺失的令牌
    Eum_RECOVERY_DELETE_TOKEN,      ///< 删除多余的令牌
    Eum_RECOVERY_REPLACE_TOKEN,     ///< 替换错误的令牌
    Eum_RECOVERY_SKIP_TOKEN,        ///< 跳过当前令牌
    Eum_RECOVERY_REORDER_TOKENS,    ///< 重新排序令牌
    Eum_RECOVERY_NONE               ///< 不进行短语级恢复
} Eum_PhraseRecoveryStrategy;

// ============================================
// 短语级恢复结果结构体
// ============================================

/**
 * @brief 短语级恢复结果结构体
 * 
 * 存储短语级恢复的结果信息。
 */
typedef struct Stru_PhraseRecoveryResult_t {
    bool success;                           ///< 恢复是否成功
    Eum_PhraseRecoveryStrategy strategy;    ///< 使用的恢复策略
    char* suggestion;                       ///< 恢复建议
    size_t tokens_affected;                 ///< 受影响的令牌数量
    Stru_Token_t* inserted_token;           ///< 插入的令牌（如果有）
    Stru_Token_t* deleted_token;            ///< 删除的令牌（如果有）
    Stru_Token_t* replaced_token;           ///< 替换的令牌（如果有）
} Stru_PhraseRecoveryResult_t;

// ============================================
// 短语级恢复函数声明
// ============================================

/**
 * @brief 创建短语级恢复结果
 * 
 * 创建并初始化一个新的短语级恢复结果对象。
 * 
 * @return Stru_PhraseRecoveryResult_t* 新创建的恢复结果对象
 */
Stru_PhraseRecoveryResult_t* F_create_phrase_recovery_result(void);

/**
 * @brief 销毁短语级恢复结果
 * 
 * 释放短语级恢复结果对象占用的所有内存。
 * 
 * @param result 要销毁的恢复结果对象
 */
void F_destroy_phrase_recovery_result(Stru_PhraseRecoveryResult_t* result);

/**
 * @brief 尝试短语级恢复
 * 
 * 尝试使用短语级恢复策略修复语法错误。
 * 
 * @param interface 语法分析器接口指针
 * @param error 发生的语法错误
 * @return Stru_PhraseRecoveryResult_t* 恢复结果，NULL表示无法恢复
 */
Stru_PhraseRecoveryResult_t* F_try_phrase_recovery(Stru_ParserInterface_t* interface,
                                                  Stru_SyntaxError_t* error);

/**
 * @brief 尝试插入缺失的令牌
 * 
 * 尝试插入缺失的令牌来修复语法错误。
 * 
 * @param interface 语法分析器接口指针
 * @param error 发生的语法错误
 * @return Stru_PhraseRecoveryResult_t* 恢复结果，NULL表示无法恢复
 */
Stru_PhraseRecoveryResult_t* F_try_insert_missing_token(Stru_ParserInterface_t* interface,
                                                       Stru_SyntaxError_t* error);

/**
 * @brief 尝试删除多余的令牌
 * 
 * 尝试删除多余的令牌来修复语法错误。
 * 
 * @param interface 语法分析器接口指针
 * @param error 发生的语法错误
 * @return Stru_PhraseRecoveryResult_t* 恢复结果，NULL表示无法恢复
 */
Stru_PhraseRecoveryResult_t* F_try_delete_extra_token(Stru_ParserInterface_t* interface,
                                                     Stru_SyntaxError_t* error);

/**
 * @brief 尝试替换错误的令牌
 * 
 * 尝试替换错误的令牌来修复语法错误。
 * 
 * @param interface 语法分析器接口指针
 * @param error 发生的语法错误
 * @return Stru_PhraseRecoveryResult_t* 恢复结果，NULL表示无法恢复
 */
Stru_PhraseRecoveryResult_t* F_try_replace_wrong_token(Stru_ParserInterface_t* interface,
                                                      Stru_SyntaxError_t* error);

/**
 * @brief 尝试跳过当前令牌
 * 
 * 尝试跳过当前令牌来修复语法错误。
 * 
 * @param interface 语法分析器接口指针
 * @param error 发生的语法错误
 * @return Stru_PhraseRecoveryResult_t* 恢复结果，NULL表示无法恢复
 */
Stru_PhraseRecoveryResult_t* F_try_skip_current_token(Stru_ParserInterface_t* interface,
                                                     Stru_SyntaxError_t* error);

/**
 * @brief 分析错误上下文
 * 
 * 分析错误发生的上下文，确定可能的恢复策略。
 * 
 * @param interface 语法分析器接口指针
 * @param error 发生的语法错误
 * @param context_buffer 上下文缓冲区
 * @param buffer_size 缓冲区大小
 * @return int 写入的字符数
 */
int F_analyze_error_context(Stru_ParserInterface_t* interface,
                           Stru_SyntaxError_t* error,
                           char* context_buffer,
                           size_t buffer_size);

/**
 * @brief 生成修复建议
 * 
 * 根据错误类型和上下文生成修复建议。
 * 
 * @param interface 语法分析器接口指针
 * @param error 发生的语法错误
 * @param suggestion_buffer 建议缓冲区
 * @param buffer_size 缓冲区大小
 * @return int 写入的字符数
 */
int F_generate_fix_suggestion(Stru_ParserInterface_t* interface,
                             Stru_SyntaxError_t* error,
                             char* suggestion_buffer,
                             size_t buffer_size);

/**
 * @brief 应用恢复策略
 * 
 * 应用短语级恢复策略到语法分析器。
 * 
 * @param interface 语法分析器接口指针
 * @param result 恢复结果
 * @return bool 应用成功返回true，否则返回false
 */
bool F_apply_recovery_strategy(Stru_ParserInterface_t* interface,
                              Stru_PhraseRecoveryResult_t* result);

/**
 * @brief 检查常见错误模式
 * 
 * 检查错误是否匹配常见的错误模式。
 * 
 * @param interface 语法分析器接口指针
 * @param error 发生的语法错误
 * @param pattern_buffer 模式缓冲区
 * @param buffer_size 缓冲区大小
 * @return bool 匹配常见模式返回true，否则返回false
 */
bool F_check_common_error_pattern(Stru_ParserInterface_t* interface,
                                 Stru_SyntaxError_t* error,
                                 char* pattern_buffer,
                                 size_t buffer_size);

#endif // CN_PARSER_PHRASE_RECOVERY_H
