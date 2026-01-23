#include "cnlang/semantics/freestanding_check.h"
#include "cnlang/support/diagnostics.h"
#include <string.h>

/*
 * CN Language Freestanding 模式特性检查实现
 */

// =============================================================================
// 函数名匹配辅助
// =============================================================================

static bool name_matches(const char* name, size_t name_len, const char* target) {
    size_t target_len = strlen(target);
    return name_len == target_len && memcmp(name, target, name_len) == 0;
}

// =============================================================================
// 禁止函数检查
// =============================================================================

CnFsForbiddenFunc cn_fs_check_forbidden_function(const char* func_name, size_t func_name_len) {
    if (!func_name) {
        return CN_FS_FORBIDDEN_NONE;
    }
    
    // 文件 I/O 函数
    if (name_matches(func_name, func_name_len, "文件打开")) {
        return CN_FS_FORBIDDEN_FILE_OPEN;
    }
    if (name_matches(func_name, func_name_len, "文件关闭")) {
        return CN_FS_FORBIDDEN_FILE_CLOSE;
    }
    if (name_matches(func_name, func_name_len, "文件读取")) {
        return CN_FS_FORBIDDEN_FILE_READ;
    }
    if (name_matches(func_name, func_name_len, "文件写入")) {
        return CN_FS_FORBIDDEN_FILE_WRITE;
    }
    if (name_matches(func_name, func_name_len, "文件结束")) {
        return CN_FS_FORBIDDEN_FILE_EOF;
    }
    
    // 控制台输入函数
    if (name_matches(func_name, func_name_len, "读取行")) {
        return CN_FS_FORBIDDEN_READ_LINE;
    }
    if (name_matches(func_name, func_name_len, "读取整数")) {
        return CN_FS_FORBIDDEN_READ_INT;
    }
    
    // 字符串高级操作
    if (name_matches(func_name, func_name_len, "字符串截取")) {
        return CN_FS_FORBIDDEN_STRING_SUBSTRING;
    }
    if (name_matches(func_name, func_name_len, "字符串去空格")) {
        return CN_FS_FORBIDDEN_STRING_TRIM;
    }
    if (name_matches(func_name, func_name_len, "字符串格式化")) {
        return CN_FS_FORBIDDEN_STRING_FORMAT;
    }
    
    // 进程管理
    if (name_matches(func_name, func_name_len, "进程创建")) {
        return CN_FS_FORBIDDEN_PROCESS_SPAWN;
    }
    if (name_matches(func_name, func_name_len, "进程等待")) {
        return CN_FS_FORBIDDEN_PROCESS_WAIT;
    }
    
    // 网络操作
    if (name_matches(func_name, func_name_len, "套接字打开")) {
        return CN_FS_FORBIDDEN_SOCKET_OPEN;
    }
    if (name_matches(func_name, func_name_len, "套接字连接")) {
        return CN_FS_FORBIDDEN_SOCKET_CONNECT;
    }
    
    return CN_FS_FORBIDDEN_NONE;
}

// =============================================================================
// 允许函数检查（用于白名单验证）
// =============================================================================

CnFsAllowedFunc cn_fs_check_allowed_function(const char* func_name, size_t func_name_len) {
    if (!func_name) {
        return CN_FS_ALLOWED_NONE;
    }
    
    // 基础打印函数
    if (name_matches(func_name, func_name_len, "打印") ||
        name_matches(func_name, func_name_len, "打印整数")) {
        return CN_FS_ALLOWED_PRINT_INT;
    }
    if (name_matches(func_name, func_name_len, "打印布尔")) {
        return CN_FS_ALLOWED_PRINT_BOOL;
    }
    if (name_matches(func_name, func_name_len, "打印字符串")) {
        return CN_FS_ALLOWED_PRINT_STRING;
    }
    if (name_matches(func_name, func_name_len, "打印行")) {
        return CN_FS_ALLOWED_PRINT_NEWLINE;
    }
    
    // 基础字符串操作
    if (name_matches(func_name, func_name_len, "字符串拼接")) {
        return CN_FS_ALLOWED_STRING_CONCAT;
    }
    if (name_matches(func_name, func_name_len, "字符串长度")) {
        return CN_FS_ALLOWED_STRING_LENGTH;
    }
    if (name_matches(func_name, func_name_len, "字符串比较")) {
        return CN_FS_ALLOWED_STRING_COMPARE;
    }
    if (name_matches(func_name, func_name_len, "字符串查找")) {
        return CN_FS_ALLOWED_STRING_INDEX_OF;
    }
    
    // 内存管理
    if (name_matches(func_name, func_name_len, "数组分配")) {
        return CN_FS_ALLOWED_ARRAY_ALLOC;
    }
    if (name_matches(func_name, func_name_len, "数组长度")) {
        return CN_FS_ALLOWED_ARRAY_LENGTH;
    }
    if (name_matches(func_name, func_name_len, "数组释放")) {
        return CN_FS_ALLOWED_ARRAY_FREE;
    }
    
    // 数学函数
    if (name_matches(func_name, func_name_len, "绝对值")) {
        return CN_FS_ALLOWED_MATH_ABS;
    }
    if (name_matches(func_name, func_name_len, "平方根")) {
        return CN_FS_ALLOWED_MATH_SQRT;
    }
    if (name_matches(func_name, func_name_len, "幂")) {
        return CN_FS_ALLOWED_MATH_POW;
    }
    
    return CN_FS_ALLOWED_NONE;
}

// =============================================================================
// 诊断信息描述
// =============================================================================

const char* cn_fs_get_forbidden_func_desc(CnFsForbiddenFunc func) {
    switch (func) {
        case CN_FS_FORBIDDEN_FILE_OPEN:
            return "文件打开操作在 freestanding 模式下不可用（无文件系统）";
        case CN_FS_FORBIDDEN_FILE_CLOSE:
            return "文件关闭操作在 freestanding 模式下不可用（无文件系统）";
        case CN_FS_FORBIDDEN_FILE_READ:
            return "文件读取操作在 freestanding 模式下不可用（无文件系统）";
        case CN_FS_FORBIDDEN_FILE_WRITE:
            return "文件写入操作在 freestanding 模式下不可用（无文件系统）";
        case CN_FS_FORBIDDEN_FILE_EOF:
            return "文件结束检查在 freestanding 模式下不可用（无文件系统）";
            
        case CN_FS_FORBIDDEN_READ_LINE:
            return "控制台输入在 freestanding 模式下不可用（使用内核提供的输入机制）";
        case CN_FS_FORBIDDEN_READ_INT:
            return "控制台输入在 freestanding 模式下不可用（使用内核提供的输入机制）";
            
        case CN_FS_FORBIDDEN_STRING_SUBSTRING:
            return "字符串截取在 freestanding 模式下可能不可用（依赖动态内存）";
        case CN_FS_FORBIDDEN_STRING_TRIM:
            return "字符串去空格在 freestanding 模式下可能不可用（依赖动态内存）";
        case CN_FS_FORBIDDEN_STRING_FORMAT:
            return "字符串格式化在 freestanding 模式下不可用（依赖复杂运行时）";
            
        case CN_FS_FORBIDDEN_PROCESS_SPAWN:
            return "进程创建在 freestanding 模式下不可用（无进程管理）";
        case CN_FS_FORBIDDEN_PROCESS_WAIT:
            return "进程等待在 freestanding 模式下不可用（无进程管理）";
            
        case CN_FS_FORBIDDEN_SOCKET_OPEN:
            return "网络套接字操作在 freestanding 模式下不可用（无网络栈）";
        case CN_FS_FORBIDDEN_SOCKET_CONNECT:
            return "网络连接操作在 freestanding 模式下不可用（无网络栈）";
            
        default:
            return "此函数在 freestanding 模式下不可用";
    }
}

// =============================================================================
// AST 遍历检查
// =============================================================================

static bool check_call_expr(CnAstExpr* expr, struct CnDiagnostics* diagnostics) {
    if (expr->kind != CN_AST_EXPR_CALL) {
        return true;
    }
    
    // 获取被调用函数名
    if (expr->as.call.callee->kind != CN_AST_EXPR_IDENTIFIER) {
        return true;  // 不是直接函数调用（可能是函数指针），暂不检查
    }
    
    const char* func_name = expr->as.call.callee->as.identifier.name;
    size_t func_name_len = expr->as.call.callee->as.identifier.name_length;
    
    // 检查是否在禁止列表中
    CnFsForbiddenFunc forbidden = cn_fs_check_forbidden_function(func_name, func_name_len);
    if (forbidden != CN_FS_FORBIDDEN_NONE) {
        // 报告错误
        const char* desc = cn_fs_get_forbidden_func_desc(forbidden);
        cn_support_diag_semantic_error_generic(diagnostics, 0, NULL, 0, 0, desc);
        return false;
    }
    
    // 递归检查参数
    for (size_t i = 0; i < expr->as.call.argument_count; i++) {
        if (!cn_fs_check_expr(expr->as.call.arguments[i], diagnostics)) {
            return false;
        }
    }
    
    return true;
}

bool cn_fs_check_expr(CnAstExpr* expr, struct CnDiagnostics* diagnostics) {
    if (!expr) {
        return true;
    }
    
    bool result = true;
    
    switch (expr->kind) {
        case CN_AST_EXPR_CALL:
            result = check_call_expr(expr, diagnostics);
            break;
            
        case CN_AST_EXPR_BINARY:
            result = cn_fs_check_expr(expr->as.binary.left, diagnostics) &&
                    cn_fs_check_expr(expr->as.binary.right, diagnostics);
            break;
            
        case CN_AST_EXPR_UNARY:
            result = cn_fs_check_expr(expr->as.unary.operand, diagnostics);
            break;
            
        case CN_AST_EXPR_LOGICAL:
            result = cn_fs_check_expr(expr->as.logical.left, diagnostics) &&
                    cn_fs_check_expr(expr->as.logical.right, diagnostics);
            break;
            
        case CN_AST_EXPR_ASSIGN:
            result = cn_fs_check_expr(expr->as.assign.target, diagnostics) &&
                    cn_fs_check_expr(expr->as.assign.value, diagnostics);
            break;
            
        default:
            // 字面量、标识符等不需要检查
            break;
    }
    
    return result;
}

bool cn_fs_check_stmt(CnAstStmt* stmt, struct CnDiagnostics* diagnostics) {
    if (!stmt) {
        return true;
    }
    
    bool result = true;
    
    switch (stmt->kind) {
        case CN_AST_STMT_EXPR:
            result = cn_fs_check_expr(stmt->as.expr.expr, diagnostics);
            break;
            
        case CN_AST_STMT_VAR_DECL:
            result = cn_fs_check_expr(stmt->as.var_decl.initializer, diagnostics);
            break;
            
        case CN_AST_STMT_RETURN:
            result = cn_fs_check_expr(stmt->as.return_stmt.expr, diagnostics);
            break;
            
        case CN_AST_STMT_IF:
            result = cn_fs_check_expr(stmt->as.if_stmt.condition, diagnostics);
            if (result && stmt->as.if_stmt.then_block) {
                for (size_t i = 0; i < stmt->as.if_stmt.then_block->stmt_count; i++) {
                    if (!cn_fs_check_stmt(stmt->as.if_stmt.then_block->stmts[i], diagnostics)) {
                        result = false;
                    }
                }
            }
            if (result && stmt->as.if_stmt.else_block) {
                for (size_t i = 0; i < stmt->as.if_stmt.else_block->stmt_count; i++) {
                    if (!cn_fs_check_stmt(stmt->as.if_stmt.else_block->stmts[i], diagnostics)) {
                        result = false;
                    }
                }
            }
            break;
            
        case CN_AST_STMT_WHILE:
            result = cn_fs_check_expr(stmt->as.while_stmt.condition, diagnostics);
            if (result && stmt->as.while_stmt.body) {
                for (size_t i = 0; i < stmt->as.while_stmt.body->stmt_count; i++) {
                    if (!cn_fs_check_stmt(stmt->as.while_stmt.body->stmts[i], diagnostics)) {
                        result = false;
                    }
                }
            }
            break;
            
        case CN_AST_STMT_FOR:
            result = cn_fs_check_stmt(stmt->as.for_stmt.init, diagnostics) &&
                    cn_fs_check_expr(stmt->as.for_stmt.condition, diagnostics) &&
                    cn_fs_check_expr(stmt->as.for_stmt.update, diagnostics);
            if (result && stmt->as.for_stmt.body) {
                for (size_t i = 0; i < stmt->as.for_stmt.body->stmt_count; i++) {
                    if (!cn_fs_check_stmt(stmt->as.for_stmt.body->stmts[i], diagnostics)) {
                        result = false;
                    }
                }
            }
            break;
            
        case CN_AST_STMT_BLOCK:
            if (stmt->as.block) {
                for (size_t i = 0; i < stmt->as.block->stmt_count; i++) {
                    if (!cn_fs_check_stmt(stmt->as.block->stmts[i], diagnostics)) {
                        result = false;
                    }
                }
            }
            break;
            
        case CN_AST_STMT_BREAK:
        case CN_AST_STMT_CONTINUE:
            // 控制流语句本身不需要检查
            break;
            
        default:
            break;
    }
    
    return result;
}

bool cn_fs_check_program(CnAstProgram* program, 
                         struct CnDiagnostics* diagnostics,
                         bool enable_check) {
    if (!program || !enable_check) {
        return true;
    }
    
    bool result = true;
    
    // 检查所有函数
    for (size_t i = 0; i < program->function_count; i++) {
        CnAstFunctionDecl* func = program->functions[i];
        if (func->body) {
            for (size_t j = 0; j < func->body->stmt_count; j++) {
                if (!cn_fs_check_stmt(func->body->stmts[j], diagnostics)) {
                    result = false;
                }
            }
        }
    }
    
    return result;
}
