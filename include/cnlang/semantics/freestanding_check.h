#ifndef CNLANG_SEMANTICS_FREESTANDING_CHECK_H
#define CNLANG_SEMANTICS_FREESTANDING_CHECK_H

#include "cnlang/frontend/ast.h"
#include "cnlang/support/diagnostics.h"
#include <stdbool.h>

/*
 * CN Language Freestanding 模式特性检查
 * 
 * 定义 freestanding 模式下禁止使用的语言特性和标准库函数，
 * 并提供语义分析阶段的检查接口。
 */

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Freestanding 模式特性限制
// =============================================================================

/**
 * Freestanding 模式下禁止使用的内建/运行时函数
 * 
 * 这些函数依赖宿主环境（文件系统、控制台输入等），
 * 在 freestanding 模式下不可用。
 */
typedef enum {
    CN_FS_FORBIDDEN_NONE = 0,
    
    // 文件 I/O 函数
    CN_FS_FORBIDDEN_FILE_OPEN,
    CN_FS_FORBIDDEN_FILE_CLOSE,
    CN_FS_FORBIDDEN_FILE_READ,
    CN_FS_FORBIDDEN_FILE_WRITE,
    CN_FS_FORBIDDEN_FILE_EOF,
    
    // 控制台输入函数
    CN_FS_FORBIDDEN_READ_LINE,
    CN_FS_FORBIDDEN_READ_INT,
    
    // 字符串高级操作（依赖动态内存分配）
    CN_FS_FORBIDDEN_STRING_SUBSTRING,
    CN_FS_FORBIDDEN_STRING_TRIM,
    CN_FS_FORBIDDEN_STRING_FORMAT,
    
    // 进程管理（未来可能支持）
    CN_FS_FORBIDDEN_PROCESS_SPAWN,
    CN_FS_FORBIDDEN_PROCESS_WAIT,
    
    // 网络操作（未来可能支持）
    CN_FS_FORBIDDEN_SOCKET_OPEN,
    CN_FS_FORBIDDEN_SOCKET_CONNECT,
    
} CnFsForbiddenFunc;

/**
 * Freestanding 模式下允许使用的核心函数
 * 
 * 这些函数在 freestanding 模式下必须可用，
 * 或者有 freestanding 版本的实现。
 */
typedef enum {
    CN_FS_ALLOWED_NONE = 0,
    
    // 基础打印函数（需要注册内核 I/O 回调）
    CN_FS_ALLOWED_PRINT_INT,
    CN_FS_ALLOWED_PRINT_BOOL,
    CN_FS_ALLOWED_PRINT_STRING,
    CN_FS_ALLOWED_PRINT_NEWLINE,
    
    // 基础字符串操作
    CN_FS_ALLOWED_STRING_CONCAT,
    CN_FS_ALLOWED_STRING_LENGTH,
    CN_FS_ALLOWED_STRING_COMPARE,
    CN_FS_ALLOWED_STRING_INDEX_OF,
    
    // 内存管理（使用静态分配器）
    CN_FS_ALLOWED_ARRAY_ALLOC,
    CN_FS_ALLOWED_ARRAY_LENGTH,
    CN_FS_ALLOWED_ARRAY_FREE,
    
    // 数学函数（需要硬件支持或软件实现）
    CN_FS_ALLOWED_MATH_ABS,
    CN_FS_ALLOWED_MATH_SQRT,
    CN_FS_ALLOWED_MATH_POW,
    
} CnFsAllowedFunc;

// =============================================================================
// 检查接口
// =============================================================================

/**
 * 检查函数调用是否在 freestanding 模式下被禁止
 * 
 * @param func_name 函数名
 * @param func_name_len 函数名长度
 * @return 如果被禁止返回对应的枚举值，否则返回 CN_FS_FORBIDDEN_NONE
 */
CnFsForbiddenFunc cn_fs_check_forbidden_function(const char* func_name, size_t func_name_len);

/**
 * 检查函数调用是否在 freestanding 模式下被允许
 * 
 * @param func_name 函数名
 * @param func_name_len 函数名长度
 * @return 如果被允许返回对应的枚举值，否则返回 CN_FS_ALLOWED_NONE
 */
CnFsAllowedFunc cn_fs_check_allowed_function(const char* func_name, size_t func_name_len);

/**
 * 获取禁止函数的描述信息（用于诊断消息）
 * 
 * @param func 禁止函数枚举值
 * @return 描述字符串
 */
const char* cn_fs_get_forbidden_func_desc(CnFsForbiddenFunc func);

/**
 * 对整个程序进行 freestanding 模式检查
 * 
 * @param program AST 程序节点
 * @param diagnostics 诊断信息收集器
 * @param enable_check 是否启用 freestanding 检查
 * @return true 如果检查通过（或未启用），false 如果发现错误
 */
bool cn_fs_check_program(CnAstProgram* program, 
                         struct CnDiagnostics* diagnostics,
                         bool enable_check);

/**
 * 检查单个表达式（递归）
 */
bool cn_fs_check_expr(CnAstExpr* expr, struct CnDiagnostics* diagnostics);

/**
 * 检查单个语句（递归）
 */
bool cn_fs_check_stmt(CnAstStmt* stmt, struct CnDiagnostics* diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_SEMANTICS_FREESTANDING_CHECK_H */
