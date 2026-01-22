#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include <stdlib.h>

static CnType *infer_expr_type(CnSemScope *scope, CnAstExpr *expr, CnDiagnostics *diagnostics);
static void check_stmt_types(CnSemScope *scope, CnAstStmt *stmt, CnDiagnostics *diagnostics);
static void check_block_types(CnSemScope *scope, CnAstBlockStmt *block, CnDiagnostics *diagnostics);

// 名称解析语义 pass
bool cn_sem_resolve_names(CnSemScope *global_scope,
                          CnAstProgram *program,
                          struct CnDiagnostics *diagnostics)
{
    (void)global_scope;
    (void)program;
    (void)diagnostics;

    // 目前 scope_builder 已经完成了符号插入，简单的名称解析可以在这里进一步实现（如绑定标识符到符号）
    return true;
}

// 类型检查语义 pass
bool cn_sem_check_types(CnSemScope *global_scope,
                        CnAstProgram *program,
                        struct CnDiagnostics *diagnostics)
{
    if (!program || !global_scope) return true;

    for (size_t i = 0; i < program->function_count; ++i) {
        CnAstFunctionDecl *fn = program->functions[i];
        // 查找函数作用域（在 scope_builder 中已经创建，这里简单起见重新进入）
        // 实际上应该在 AST 中记录 scope，或者在这里按同样逻辑推导
        // 为了演示，我们假设我们能找到函数对应的作用域
        // 注意：目前 scope_builder.c 中 cn_sem_build_function_scope 创建了新作用域但没存回 AST
        // 这里我们简化处理，假设我们重新构建或由 caller 保证
        
        // 暂时假设我们遍历时同步维护作用域
        CnSemScope *fn_scope = cn_sem_scope_new(CN_SEM_SCOPE_FUNCTION, global_scope);
        // 重新插入参数到临时作用域（正式实现应当复用之前的 scope）
        for (size_t j = 0; j < fn->parameter_count; j++) {
            CnSemSymbol *sym = cn_sem_scope_insert_symbol(fn_scope, fn->parameters[j].name, fn->parameters[j].name_length, CN_SEM_SYMBOL_VARIABLE);
            if (sym) sym->type = fn->parameters[j].declared_type;
        }

        check_block_types(fn_scope, fn->body, diagnostics);
        
        cn_sem_scope_free(fn_scope);
    }

    return true;
}

static void check_block_types(CnSemScope *scope, CnAstBlockStmt *block, CnDiagnostics *diagnostics) {
    if (!block) return;
    CnSemScope *block_scope = cn_sem_scope_new(CN_SEM_SCOPE_BLOCK, scope);
    for (size_t i = 0; i < block->stmt_count; i++) {
        check_stmt_types(block_scope, block->stmts[i], diagnostics);
    }
    cn_sem_scope_free(block_scope);
}

static void check_stmt_types(CnSemScope *scope, CnAstStmt *stmt, CnDiagnostics *diagnostics) {
    if (!stmt) return;

    switch (stmt->kind) {
        case CN_AST_STMT_BLOCK:
            check_block_types(scope, stmt->as.block, diagnostics);
            break;
        case CN_AST_STMT_VAR_DECL: {
            CnAstVarDecl *decl = &stmt->as.var_decl;
            CnType *init_type = infer_expr_type(scope, decl->initializer, diagnostics);
            
            // 插入符号到当前作用域
            CnSemSymbol *sym = cn_sem_scope_insert_symbol(scope, decl->name, decl->name_length, CN_SEM_SYMBOL_VARIABLE);
            if (sym) {
                if (decl->declared_type) {
                    sym->type = decl->declared_type;
                    if (init_type && !cn_type_compatible(init_type, sym->type)) {
                        // 报错：类型不匹配
                        cn_support_diagnostics_report(diagnostics, CN_DIAG_SEVERITY_ERROR, 0, NULL, 0, 0, "变量初始化类型不匹配");
                    }
                } else {
                    // 类型推断：var a = 1;
                    sym->type = init_type;
                }
            }
            break;
        }
        case CN_AST_STMT_EXPR:
            infer_expr_type(scope, stmt->as.expr.expr, diagnostics);
            break;
        case CN_AST_STMT_RETURN:
            infer_expr_type(scope, stmt->as.return_stmt.expr, diagnostics);
            break;
        case CN_AST_STMT_IF:
            infer_expr_type(scope, stmt->as.if_stmt.condition, diagnostics);
            check_block_types(scope, stmt->as.if_stmt.then_block, diagnostics);
            check_block_types(scope, stmt->as.if_stmt.else_block, diagnostics);
            break;
        case CN_AST_STMT_WHILE:
            infer_expr_type(scope, stmt->as.while_stmt.condition, diagnostics);
            check_block_types(scope, stmt->as.while_stmt.body, diagnostics);
            break;
        default:
            break;
    }
}

static CnType *infer_expr_type(CnSemScope *scope, CnAstExpr *expr, CnDiagnostics *diagnostics) {
    if (!expr) return NULL;

    switch (expr->kind) {
        case CN_AST_EXPR_INTEGER_LITERAL:
            expr->type = cn_type_new_primitive(CN_TYPE_INT);
            break;
        case CN_AST_EXPR_STRING_LITERAL:
            expr->type = cn_type_new_primitive(CN_TYPE_STRING);
            break;
        case CN_AST_EXPR_IDENTIFIER: {
            CnSemSymbol *sym = cn_sem_scope_lookup(scope, expr->as.identifier.name, expr->as.identifier.name_length);
            if (sym) {
                expr->type = sym->type;
            } else {
                // 报错：未定义标识符
                cn_support_diagnostics_report(diagnostics, CN_DIAG_SEVERITY_ERROR, 0, NULL, 0, 0, "未定义的标识符");
                expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
            }
            break;
        }
        case CN_AST_EXPR_BINARY: {
            CnType *left = infer_expr_type(scope, expr->as.binary.left, diagnostics);
            CnType *right = infer_expr_type(scope, expr->as.binary.right, diagnostics);
            
            if (left && right && cn_type_compatible(left, right)) {
                // 简单的算术运算结果类型
                if (expr->as.binary.op >= CN_AST_BINARY_OP_EQ && expr->as.binary.op <= CN_AST_BINARY_OP_GE) {
                    expr->type = cn_type_new_primitive(CN_TYPE_BOOL);
                } else {
                    expr->type = left; // 假设 int + int -> int
                }
            } else {
                expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
            }
            break;
        }
        case CN_AST_EXPR_ASSIGN: {
            CnType *target = infer_expr_type(scope, expr->as.assign.target, diagnostics);
            CnType *val = infer_expr_type(scope, expr->as.assign.value, diagnostics);
            if (target && val && cn_type_compatible(val, target)) {
                expr->type = target;
            } else {
                expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
            }
            break;
        }
        case CN_AST_EXPR_LOGICAL: {
            infer_expr_type(scope, expr->as.logical.left, diagnostics);
            infer_expr_type(scope, expr->as.logical.right, diagnostics);
            expr->type = cn_type_new_primitive(CN_TYPE_BOOL);
            break;
        }
        case CN_AST_EXPR_UNARY: {
            CnType *inner = infer_expr_type(scope, expr->as.unary.operand, diagnostics);
            if (expr->as.unary.op == CN_AST_UNARY_OP_NOT) {
                expr->type = cn_type_new_primitive(CN_TYPE_BOOL);
            } else {
                expr->type = inner;
            }
            break;
        }
        default:
            expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
            break;
    }

    return expr->type;
}
