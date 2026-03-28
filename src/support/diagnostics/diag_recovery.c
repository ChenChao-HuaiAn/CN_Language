/**
 * @file diag_recovery.c
 * @brief 错误恢复策略实现
 * 
 * 实现编译器错误恢复机制，包括：
 * - 恢复策略选择算法
 * - 级联错误检测与抑制
 * - 同步点识别
 * 
 * @version 1.0
 * @date 2026-03-28
 */

#include "cnlang/support/diag_recovery.h"
#include <string.h>

/* ==================== 常量定义 ==================== */

/**
 * @brief 级联错误阈值
 * 
 * 同一行上允许的最大连续错误数，超过此阈值将触发强制同步
 */
#define CASCADE_ERROR_THRESHOLD 3

/**
 * @brief 同步标记表
 * 
 * 这些Token类型可以作为错误恢复的同步点。
 * 当发生错误时，解析器可以跳过Token直到遇到这些同步点。
 */
static const int g_sync_points[] = {
    /* 语句结束符 */
    ';',        /* 分号 - 语句结束 */
    '}',        /* 右花括号 - 块结束 */
    ')',        /* 右圆括号 - 表达式结束 */
    ']',        /* 右方括号 - 数组下标结束 */
    ',',        /* 逗号 - 参数分隔符 */
    
    /* 注意：关键字同步点需要根据实际的Token类型定义来扩展
     * 例如: TOKEN_函数, TOKEN_如果, TOKEN_否则, TOKEN_当 等
     * 这些通常开始新的语法结构，是良好的同步点
     */
};

/** @brief 同步标记数量 */
#define SYNC_POINT_COUNT (sizeof(g_sync_points) / sizeof(g_sync_points[0]))

/* ==================== 接口实现 ==================== */

/**
 * @brief 初始化恢复上下文
 */
void cn_diag_recovery_init(CnDiagRecoveryContext *ctx, int max_errors) {
    if (ctx) {
        /* 清零所有字段 */
        memset(ctx, 0, sizeof(CnDiagRecoveryContext));
        ctx->max_errors = max_errors;
        /* error_count, cascade_suppress, last_error_line 初始化为0 */
        /* last_error_file 初始化为NULL */
    }
}

/**
 * @brief 记录错误并决定恢复策略
 * 
 * 算法流程：
 * 1. 增加错误计数
 * 2. 检查是否达到最大错误限制
 * 3. 检测级联错误（同一位置多次错误）
 * 4. 根据错误类型选择恢复策略
 */
CnDiagRecoveryStrategy cn_diag_recovery_on_error(
    CnDiagRecoveryContext *ctx,
    CnDiagCode code,
    const char *filename,
    int line
) {
    if (!ctx) {
        /* 无效上下文，停止编译 */
        return CN_DIAG_RECOVERY_NONE;
    }
    
    /* 增加错误计数 */
    ctx->error_count++;
    
    /* 检查是否达到最大错误数 */
    if (ctx->max_errors > 0 && ctx->error_count >= ctx->max_errors) {
        /* 达到上限，停止编译 */
        return CN_DIAG_RECOVERY_NONE;
    }
    
    /* 检测级联错误 */
    if (filename && ctx->last_error_file &&
        strcmp(filename, ctx->last_error_file) == 0 &&
        line == ctx->last_error_line) {
        /* 同一位置的连续错误，可能是级联效应 */
        ctx->cascade_suppress++;
        if (ctx->cascade_suppress >= CASCADE_ERROR_THRESHOLD) {
            /* 级联错误过多，强制同步到安全点 */
            return CN_DIAG_RECOVERY_SYNC;
        }
    } else {
        /* 不同位置的错误，重置级联计数 */
        ctx->cascade_suppress = 0;
    }
    
    /* 更新最后错误位置 */
    ctx->last_error_file = filename;
    ctx->last_error_line = line;
    
    /* 根据错误类型选择恢复策略 */
    switch (code) {
        /* 期望标记类错误 - 尝试虚拟插入 */
        case CN_DIAG_CODE_PARSE_EXPECTED_TOKEN:
            return CN_DIAG_RECOVERY_INSERT;
            
        /* 无效语法类错误 - 跳过当前标记 */
        case CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME:
        case CN_DIAG_CODE_PARSE_INVALID_PARAM:
        case CN_DIAG_CODE_PARSE_INVALID_VAR_DECL:
        case CN_DIAG_CODE_PARSE_INVALID_COMPARISON_OP:
        case CN_DIAG_CODE_PARSE_INVALID_EXPR:
        case CN_DIAG_CODE_LEX_INVALID_CHAR:
            return CN_DIAG_RECOVERY_SKIP_TOKEN;
            
        /* 词法错误 - 跳过当前标记 */
        case CN_DIAG_CODE_LEX_UNTERMINATED_STRING:
        case CN_DIAG_CODE_LEX_UNTERMINATED_BLOCK_COMMENT:
        case CN_DIAG_CODE_LEX_INVALID_HEX:
        case CN_DIAG_CODE_LEX_INVALID_BINARY:
        case CN_DIAG_CODE_LEX_INVALID_OCTAL:
        case CN_DIAG_CODE_LEX_LITERAL_OVERFLOW:
            return CN_DIAG_RECOVERY_SKIP_TOKEN;
            
        /* 语义错误 - 通常可以继续解析 */
        case CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER:
        case CN_DIAG_CODE_SEM_TYPE_MISMATCH:
        case CN_DIAG_CODE_SEM_DUPLICATE_SYMBOL:
        case CN_DIAG_CODE_SEM_ARGUMENT_COUNT_MISMATCH:
        case CN_DIAG_CODE_SEM_ARGUMENT_TYPE_MISMATCH:
            return CN_DIAG_RECOVERY_SYNC;
            
        /* 默认策略 - 同步到安全点 */
        default:
            return CN_DIAG_RECOVERY_SYNC;
    }
}

/**
 * @brief 检查是否为同步标记
 * 
 * 遍历同步标记表，检查给定的Token类型是否为同步点
 */
int cn_diag_is_sync_point(int token_type) {
    for (size_t i = 0; i < SYNC_POINT_COUNT; i++) {
        if (g_sync_points[i] == token_type) {
            return 1;  /* 是同步标记 */
        }
    }
    return 0;  /* 不是同步标记 */
}

/**
 * @brief 检查是否应抑制错误
 * 
 * 当级联抑制计数超过阈值且位置未变化时，抑制后续错误报告
 */
int cn_diag_should_suppress(
    CnDiagRecoveryContext *ctx,
    const char *filename,
    int line
) {
    if (!ctx) {
        return 0;  /* 无效上下文，不抑制 */
    }
    
    /* 检查级联抑制条件 */
    if (ctx->cascade_suppress >= CASCADE_ERROR_THRESHOLD) {
        /* 只有当位置仍然相同时才抑制 */
        if (filename && ctx->last_error_file &&
            strcmp(filename, ctx->last_error_file) == 0 &&
            line == ctx->last_error_line) {
            return 1;  /* 抑制错误 */
        }
    }
    
    return 0;  /* 不抑制 */
}

/**
 * @brief 重置恢复上下文
 * 
 * 清除所有状态，准备处理新的编译单元
 */
void cn_diag_recovery_reset(CnDiagRecoveryContext *ctx) {
    if (ctx) {
        ctx->error_count = 0;
        ctx->cascade_suppress = 0;
        ctx->last_error_file = NULL;
        ctx->last_error_line = 0;
        /* max_errors 保持不变，允许复用配置 */
    }
}

/**
 * @brief 获取当前错误计数
 */
int cn_diag_recovery_get_error_count(const CnDiagRecoveryContext *ctx) {
    return ctx ? ctx->error_count : 0;
}

/**
 * @brief 检查是否已达到最大错误数
 */
int cn_diag_recovery_is_limit_reached(const CnDiagRecoveryContext *ctx) {
    if (!ctx) {
        return 1;  /* 无效上下文视为达到上限 */
    }
    
    /* max_errors为0表示无限制 */
    if (ctx->max_errors == 0) {
        return 0;
    }
    
    return ctx->error_count >= ctx->max_errors;
}
