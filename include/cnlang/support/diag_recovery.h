/**
 * @file diag_recovery.h
 * @brief 错误恢复策略头文件
 * 
 * 提供编译器错误恢复机制，包括：
 * - 恢复策略选择（跳过、同步、插入）
 * - 级联错误抑制
 * - 同步点检测
 * 
 * @version 1.0
 * @date 2026-03-28
 */

#ifndef CNLANG_SUPPORT_DIAG_RECOVERY_H
#define CNLANG_SUPPORT_DIAG_RECOVERY_H

#include "diagnostics.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 枚举定义 ==================== */

/**
 * @brief 恢复策略枚举
 * 
 * 定义编译器遇到错误时可采取的恢复策略
 */
typedef enum CnDiagRecoveryStrategy {
    CN_DIAG_RECOVERY_NONE,       /* 不恢复，立即停止编译 */
    CN_DIAG_RECOVERY_SKIP_TOKEN, /* 跳过当前标记，继续解析 */
    CN_DIAG_RECOVERY_SYNC,       /* 同步到下一个安全点 */
    CN_DIAG_RECOVERY_INSERT      /* 插入缺失标记（虚拟标记） */
} CnDiagRecoveryStrategy;

/**
 * @brief 同步标记类型
 * 
 * 定义可作为错误恢复同步点的标记类型
 */
typedef enum CnDiagSyncPointType {
    CN_DIAG_SYNC_SEMICOLON,      /* 分号 ; */
    CN_DIAG_SYNC_RBRACE,         /* 右花括号 } */
    CN_DIAG_SYNC_RPAREN,         /* 右圆括号 ) */
    CN_DIAG_SYNC_RBRACKET,       /* 右方括号 ] */
    CN_DIAG_SYNC_COMMA,          /* 逗号 , */
    CN_DIAG_SYNC_KEYWORD         /* 关键字（函数、如果、否则等） */
} CnDiagSyncPointType;

/* ==================== 结构体定义 ==================== */

/**
 * @brief 恢复上下文结构
 * 
 * 跟踪错误恢复过程中的状态信息
 */
typedef struct CnDiagRecoveryContext {
    int error_count;                    /* 当前错误计数 */
    int max_errors;                     /* 最大错误限制（0表示无限制） */
    int cascade_suppress;               /* 级联抑制计数器 */
    int last_error_line;                /* 上次错误行号 */
    const char *last_error_file;        /* 上次错误文件名 */
} CnDiagRecoveryContext;

/* ==================== 接口函数 ==================== */

/**
 * @brief 初始化恢复上下文
 * 
 * @param ctx 恢复上下文指针
 * @param max_errors 最大错误数（0表示无限制）
 * 
 * @example
 * CnDiagRecoveryContext ctx;
 * cn_diag_recovery_init(&ctx, 100); // 最多报告100个错误
 */
void cn_diag_recovery_init(CnDiagRecoveryContext *ctx, int max_errors);

/**
 * @brief 记录错误并决定恢复策略
 * 
 * 根据错误类型和当前状态选择最佳的恢复策略：
 * - 检查是否达到最大错误数
 * - 检测级联错误
 * - 根据错误码选择策略
 * 
 * @param ctx 恢复上下文
 * @param code 错误码
 * @param filename 文件名
 * @param line 行号
 * @return 恢复策略
 */
CnDiagRecoveryStrategy cn_diag_recovery_on_error(
    CnDiagRecoveryContext *ctx,
    CnDiagCode code,
    const char *filename,
    int line
);

/**
 * @brief 检查是否为同步标记
 * 
 * 同步标记是可以作为错误恢复安全点的Token类型，
 * 包括语句结束符和关键字等。
 * 
 * @param token_type Token类型
 * @return 1表示是同步标记，0表示不是
 */
int cn_diag_is_sync_point(int token_type);

/**
 * @brief 检查是否应抑制错误（级联错误检测）
 * 
 * 当同一位置连续产生多个错误时，可能是第一个错误
 * 导致的级联效应，此时应抑制后续错误报告。
 * 
 * @param ctx 恢复上下文
 * @param filename 文件名
 * @param line 行号
 * @return 1表示应抑制，0表示应报告
 */
int cn_diag_should_suppress(
    CnDiagRecoveryContext *ctx,
    const char *filename,
    int line
);

/**
 * @brief 重置恢复上下文
 * 
 * 清除错误计数和级联抑制状态，用于开始新的编译单元。
 * 
 * @param ctx 恢复上下文
 */
void cn_diag_recovery_reset(CnDiagRecoveryContext *ctx);

/**
 * @brief 获取当前错误计数
 * 
 * @param ctx 恢复上下文
 * @return 当前错误计数
 */
int cn_diag_recovery_get_error_count(const CnDiagRecoveryContext *ctx);

/**
 * @brief 检查是否已达到最大错误数
 * 
 * @param ctx 恢复上下文
 * @return 1表示已达到上限，0表示未达到
 */
int cn_diag_recovery_is_limit_reached(const CnDiagRecoveryContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_SUPPORT_DIAG_RECOVERY_H */
