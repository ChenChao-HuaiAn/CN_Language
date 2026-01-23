#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"

#include <stdlib.h>
#include <string.h>

static void cn_sem_build_function_scope(CnSemScope *parent_scope,
                                         CnAstFunctionDecl *function_decl,
                                         CnDiagnostics *diagnostics);
static void cn_sem_build_block_scope(CnSemScope *parent_scope,
                                     CnAstBlockStmt *block,
                                     CnDiagnostics *diagnostics);
static void cn_sem_build_stmt(CnSemScope *scope, CnAstStmt *stmt, CnDiagnostics *diagnostics);
static void cn_sem_build_expr(CnSemScope *scope, CnAstExpr *expr, CnDiagnostics *diagnostics);
static void cn_sem_build_if_stmt(CnSemScope *scope, CnAstIfStmt *if_stmt, CnDiagnostics *diagnostics);
static void cn_sem_build_while_stmt(CnSemScope *scope, CnAstWhileStmt *while_stmt, CnDiagnostics *diagnostics);
static void cn_sem_build_for_stmt(CnSemScope *scope, CnAstForStmt *for_stmt, CnDiagnostics *diagnostics);

CnSemScope *cn_sem_build_scopes(CnAstProgram *program, CnDiagnostics *diagnostics)
{
    CnSemScope *global_scope;
    size_t i;

    if (!program) {
        return NULL;
    }

    global_scope = cn_sem_scope_new(CN_SEM_SCOPE_GLOBAL, NULL);
    if (!global_scope) {
        return NULL;
    }

    // 注册内置函数：打印 (print)
    CnSemSymbol *print_sym = cn_sem_scope_insert_symbol(global_scope, "打印", strlen("打印"), CN_SEM_SYMBOL_FUNCTION);
    if (print_sym) {
        CnType **param_types = (CnType **)malloc(sizeof(CnType *));
        param_types[0] = cn_type_new_primitive(CN_TYPE_STRING);
        print_sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_VOID), param_types, 1);
    }

    // 注册内置函数：长度 (length)
    // 注意：长度函数是特殊的，它可以接受字符串或数组参数
    // 我们在符号表中标记它，但在 semantic_passes.c 中特殊处理其类型检查
    // 为了避免类型冲突，我们将其标记为 UNKNOWN 类型，让特殊处理逻辑来判断
    CnSemSymbol *length_sym = cn_sem_scope_insert_symbol(global_scope, "长度", strlen("长度"), CN_SEM_SYMBOL_FUNCTION);
    if (length_sym) {
        // 使用 UNKNOWN 类型标记，让类型检查器特殊处理
        length_sym->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
    }

    for (i = 0; i < program->function_count; ++i) {
        CnAstFunctionDecl *function_decl = program->functions[i];

        if (!function_decl) {
            continue;
        }

        CnSemSymbol *sym = cn_sem_scope_insert_symbol(global_scope,
                                   function_decl->name,
                                   function_decl->name_length,
                                   CN_SEM_SYMBOL_FUNCTION);
        if (sym) {
            // 构建完整的函数类型
            CnType **param_types = NULL;
            if (function_decl->parameter_count > 0) {
                param_types = (CnType **)malloc(sizeof(CnType *) * function_decl->parameter_count);
                for (size_t j = 0; j < function_decl->parameter_count; j++) {
                    param_types[j] = function_decl->parameters[j].declared_type;
                }
            }
            // 目前假设返回类型为 int，后续可以从 AST 获取
            sym->type = cn_type_new_function(cn_type_new_primitive(CN_TYPE_INT),
                                            param_types,
                                            function_decl->parameter_count);
        } else {
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, function_decl->name);
        }

        cn_sem_build_function_scope(global_scope, function_decl, diagnostics);
    }

    return global_scope;
}

static void cn_sem_build_function_scope(CnSemScope *parent_scope,
                                        CnAstFunctionDecl *function_decl,
                                        CnDiagnostics *diagnostics)
{
    CnSemScope *function_scope;
    size_t i;

    if (!parent_scope || !function_decl) {
        return;
    }

    function_scope = cn_sem_scope_new(CN_SEM_SCOPE_FUNCTION, parent_scope);
    if (!function_scope) {
        return;
    }

    for (i = 0; i < function_decl->parameter_count; ++i) {
        CnAstParameter *param = &function_decl->parameters[i];

        CnSemSymbol *sym = cn_sem_scope_insert_symbol(function_scope,
                                   param->name,
                                   param->name_length,
                                   CN_SEM_SYMBOL_VARIABLE);
        if (sym) {
            sym->type = param->declared_type;
        } else {
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, param->name);
        }
    }

    cn_sem_build_block_scope(function_scope, function_decl->body, diagnostics);
}

static void cn_sem_build_block_scope(CnSemScope *parent_scope,
                                     CnAstBlockStmt *block,
                                     CnDiagnostics *diagnostics)
{
    CnSemScope *block_scope;
    size_t i;

    if (!parent_scope || !block) {
        return;
    }

    block_scope = cn_sem_scope_new(CN_SEM_SCOPE_BLOCK, parent_scope);
    if (!block_scope) {
        return;
    }

    for (i = 0; i < block->stmt_count; ++i) {
        cn_sem_build_stmt(block_scope, block->stmts[i], diagnostics);
    }
}

static void cn_sem_build_stmt(CnSemScope *scope, CnAstStmt *stmt, CnDiagnostics *diagnostics)
{
    if (!scope || !stmt) {
        return;
    }

    switch (stmt->kind) {
    case CN_AST_STMT_BLOCK:
        cn_sem_build_block_scope(scope, stmt->as.block, diagnostics);
        break;
    case CN_AST_STMT_VAR_DECL: {
        CnAstVarDecl *var_decl = &stmt->as.var_decl;

        CnSemSymbol *sym = cn_sem_scope_insert_symbol(scope,
                                   var_decl->name,
                                   var_decl->name_length,
                                   CN_SEM_SYMBOL_VARIABLE);
        if (sym) {
            sym->type = var_decl->declared_type;
        } else {
            // 报告重复定义
            cn_support_diag_semantic_error_duplicate_symbol(
                diagnostics, NULL, 0, 0, var_decl->name);
        }

        cn_sem_build_expr(scope, var_decl->initializer, diagnostics);
        break;
    }
    case CN_AST_STMT_EXPR:
        cn_sem_build_expr(scope, stmt->as.expr.expr, diagnostics);
        break;
    case CN_AST_STMT_RETURN:
        cn_sem_build_expr(scope, stmt->as.return_stmt.expr, diagnostics);
        break;
    case CN_AST_STMT_IF:
        cn_sem_build_if_stmt(scope, &stmt->as.if_stmt, diagnostics);
        break;
    case CN_AST_STMT_WHILE:
        cn_sem_build_while_stmt(scope, &stmt->as.while_stmt, diagnostics);
        break;
    case CN_AST_STMT_FOR:
        cn_sem_build_for_stmt(scope, &stmt->as.for_stmt, diagnostics);
        break;
    case CN_AST_STMT_BREAK:
    case CN_AST_STMT_CONTINUE:
        break;
    }
}

static void cn_sem_build_if_stmt(CnSemScope *scope, CnAstIfStmt *if_stmt, CnDiagnostics *diagnostics)
{
    if (!scope || !if_stmt) {
        return;
    }

    cn_sem_build_expr(scope, if_stmt->condition, diagnostics);
    cn_sem_build_block_scope(scope, if_stmt->then_block, diagnostics);
    cn_sem_build_block_scope(scope, if_stmt->else_block, diagnostics);
}

static void cn_sem_build_while_stmt(CnSemScope *scope, CnAstWhileStmt *while_stmt, CnDiagnostics *diagnostics)
{
    if (!scope || !while_stmt) {
        return;
    }

    cn_sem_build_expr(scope, while_stmt->condition, diagnostics);
    cn_sem_build_block_scope(scope, while_stmt->body, diagnostics);
}

static void cn_sem_build_for_stmt(CnSemScope *scope, CnAstForStmt *for_stmt, CnDiagnostics *diagnostics)
{
    CnSemScope *for_scope;

    if (!scope || !for_stmt) {
        return;
    }

    for_scope = cn_sem_scope_new(CN_SEM_SCOPE_BLOCK, scope);
    if (!for_scope) {
        return;
    }

    cn_sem_build_stmt(for_scope, for_stmt->init, diagnostics);
    cn_sem_build_expr(for_scope, for_stmt->condition, diagnostics);
    cn_sem_build_expr(for_scope, for_stmt->update, diagnostics);
    cn_sem_build_block_scope(for_scope, for_stmt->body, diagnostics);
}

static void cn_sem_build_expr(CnSemScope *scope, CnAstExpr *expr, CnDiagnostics *diagnostics)
{
    size_t i;

    if (!scope || !expr) {
        return;
    }

    switch (expr->kind) {
    case CN_AST_EXPR_BINARY:
        cn_sem_build_expr(scope, expr->as.binary.left, diagnostics);
        cn_sem_build_expr(scope, expr->as.binary.right, diagnostics);
        break;
    case CN_AST_EXPR_CALL:
        cn_sem_build_expr(scope, expr->as.call.callee, diagnostics);
        for (i = 0; i < expr->as.call.argument_count; ++i) {
            cn_sem_build_expr(scope, expr->as.call.arguments[i], diagnostics);
        }
        break;
    case CN_AST_EXPR_IDENTIFIER:
    case CN_AST_EXPR_INTEGER_LITERAL:
    case CN_AST_EXPR_STRING_LITERAL:
        break;
    case CN_AST_EXPR_ASSIGN:
        cn_sem_build_expr(scope, expr->as.assign.target, diagnostics);
        cn_sem_build_expr(scope, expr->as.assign.value, diagnostics);
        break;
    case CN_AST_EXPR_LOGICAL:
        cn_sem_build_expr(scope, expr->as.logical.left, diagnostics);
        cn_sem_build_expr(scope, expr->as.logical.right, diagnostics);
        break;
    case CN_AST_EXPR_UNARY:
        cn_sem_build_expr(scope, expr->as.unary.operand, diagnostics);
        break;
    }
}
