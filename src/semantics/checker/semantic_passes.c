#include "cnlang/frontend/semantics.h"
#include "cnlang/semantics/freestanding_check.h"
#include "cnlang/support/diagnostics.h"
#include <stdlib.h>

static CnType *infer_expr_type(CnSemScope *scope, CnAstExpr *expr, CnDiagnostics *diagnostics);
static void check_stmt_types(CnSemScope *scope, CnAstStmt *stmt, CnDiagnostics *diagnostics, bool in_loop);
static void check_block_types(CnSemScope *scope, CnAstBlockStmt *block, CnDiagnostics *diagnostics, bool in_loop);

static void resolve_stmt_names(CnSemScope *scope, CnAstStmt *stmt, CnDiagnostics *diagnostics);
static void resolve_expr_names(CnSemScope *scope, CnAstExpr *expr, CnDiagnostics *diagnostics);

static void resolve_block_names(CnSemScope *scope, CnAstBlockStmt *block, CnDiagnostics *diagnostics);

// 名称解析语义 pass
bool cn_sem_resolve_names(CnSemScope *global_scope,
                          CnAstProgram *program,
                          struct CnDiagnostics *diagnostics)
{
    if (!program || !global_scope) return true;

    for (size_t i = 0; i < program->function_count; ++i) {
        CnAstFunctionDecl *fn = program->functions[i];
        
        // 进入函数作用域
        CnSemScope *fn_scope = cn_sem_scope_new(CN_SEM_SCOPE_FUNCTION, global_scope);
        
        // 插入参数以构建正确的函数内部作用域环境
        for (size_t j = 0; j < fn->parameter_count; j++) {
            cn_sem_scope_insert_symbol(fn_scope, fn->parameters[j].name, fn->parameters[j].name_length, CN_SEM_SYMBOL_VARIABLE);
        }

        resolve_block_names(fn_scope, fn->body, diagnostics);
        
        cn_sem_scope_free(fn_scope);
    }

    return cn_support_diagnostics_error_count(diagnostics) == 0;
}

static void resolve_block_names(CnSemScope *scope, CnAstBlockStmt *block, CnDiagnostics *diagnostics) {
    if (!block) return;
    CnSemScope *block_scope = cn_sem_scope_new(CN_SEM_SCOPE_BLOCK, scope);
    for (size_t i = 0; i < block->stmt_count; i++) {
        resolve_stmt_names(block_scope, block->stmts[i], diagnostics);
    }
    cn_sem_scope_free(block_scope);
}

static void resolve_stmt_names(CnSemScope *scope, CnAstStmt *stmt, CnDiagnostics *diagnostics) {
    if (!stmt) return;

    switch (stmt->kind) {
        case CN_AST_STMT_BLOCK:
            resolve_block_names(scope, stmt->as.block, diagnostics);
            break;
        case CN_AST_STMT_VAR_DECL: {
            CnAstVarDecl *decl = &stmt->as.var_decl;
            resolve_expr_names(scope, decl->initializer, diagnostics);
            // 插入变量名以便后续语句引用
            cn_sem_scope_insert_symbol(scope, decl->name, decl->name_length, CN_SEM_SYMBOL_VARIABLE);
            break;
        }
        case CN_AST_STMT_EXPR:
            resolve_expr_names(scope, stmt->as.expr.expr, diagnostics);
            break;
        case CN_AST_STMT_RETURN:
            resolve_expr_names(scope, stmt->as.return_stmt.expr, diagnostics);
            break;
        case CN_AST_STMT_IF:
            resolve_expr_names(scope, stmt->as.if_stmt.condition, diagnostics);
            resolve_block_names(scope, stmt->as.if_stmt.then_block, diagnostics);
            resolve_block_names(scope, stmt->as.if_stmt.else_block, diagnostics);
            break;
        case CN_AST_STMT_WHILE:
            resolve_expr_names(scope, stmt->as.while_stmt.condition, diagnostics);
            resolve_block_names(scope, stmt->as.while_stmt.body, diagnostics);
            break;
        case CN_AST_STMT_FOR: {
            CnSemScope *for_scope = cn_sem_scope_new(CN_SEM_SCOPE_BLOCK, scope);
            resolve_stmt_names(for_scope, stmt->as.for_stmt.init, diagnostics);
            resolve_expr_names(for_scope, stmt->as.for_stmt.condition, diagnostics);
            resolve_expr_names(for_scope, stmt->as.for_stmt.update, diagnostics);
            resolve_block_names(for_scope, stmt->as.for_stmt.body, diagnostics);
            cn_sem_scope_free(for_scope);
            break;
        }
        case CN_AST_STMT_BREAK:
        case CN_AST_STMT_CONTINUE:
            break;
        default:
            break;
    }
}

static void resolve_expr_names(CnSemScope *scope, CnAstExpr *expr, CnDiagnostics *diagnostics) {
    if (!expr) return;
    switch (expr->kind) {
        case CN_AST_EXPR_IDENTIFIER: {
            CnSemSymbol *sym = cn_sem_scope_lookup(scope, expr->as.identifier.name, expr->as.identifier.name_length);
            if (!sym) {
                cn_support_diag_semantic_error_undefined_identifier(
                    diagnostics, NULL, 0, 0, expr->as.identifier.name);
            }
            break;
        }
        case CN_AST_EXPR_BINARY:
            resolve_expr_names(scope, expr->as.binary.left, diagnostics);
            resolve_expr_names(scope, expr->as.binary.right, diagnostics);
            break;
        case CN_AST_EXPR_CALL:
            resolve_expr_names(scope, expr->as.call.callee, diagnostics);
            for (size_t i = 0; i < expr->as.call.argument_count; i++) {
                resolve_expr_names(scope, expr->as.call.arguments[i], diagnostics);
            }
            break;
        case CN_AST_EXPR_ASSIGN:
            resolve_expr_names(scope, expr->as.assign.target, diagnostics);
            resolve_expr_names(scope, expr->as.assign.value, diagnostics);
            break;
        case CN_AST_EXPR_LOGICAL:
            resolve_expr_names(scope, expr->as.logical.left, diagnostics);
            resolve_expr_names(scope, expr->as.logical.right, diagnostics);
            break;
        case CN_AST_EXPR_UNARY:
            resolve_expr_names(scope, expr->as.unary.operand, diagnostics);
            break;
        case CN_AST_EXPR_ARRAY_LITERAL:
            // 解析数组字面量中每个元素的名称
            for (size_t i = 0; i < expr->as.array_literal.element_count; i++) {
                resolve_expr_names(scope, expr->as.array_literal.elements[i], diagnostics);
            }
            break;
        case CN_AST_EXPR_INDEX:
            // 解析索引访问表达式
            resolve_expr_names(scope, expr->as.index.array, diagnostics);
            resolve_expr_names(scope, expr->as.index.index, diagnostics);
            break;
        default: break;
    }
}

// 类型检查语义 pass
bool cn_sem_check_types(CnSemScope *global_scope,
                        CnAstProgram *program,
                        struct CnDiagnostics *diagnostics)
{
    if (!program || !global_scope) return true;

    for (size_t i = 0; i < program->function_count; ++i) {
        CnAstFunctionDecl *fn = program->functions[i];
        
        CnSemScope *fn_scope = cn_sem_scope_new(CN_SEM_SCOPE_FUNCTION, global_scope);
        for (size_t j = 0; j < fn->parameter_count; j++) {
            CnSemSymbol *sym = cn_sem_scope_insert_symbol(fn_scope, fn->parameters[j].name, fn->parameters[j].name_length, CN_SEM_SYMBOL_VARIABLE);
            if (sym) sym->type = fn->parameters[j].declared_type;
        }

        check_block_types(fn_scope, fn->body, diagnostics, false);
        
        cn_sem_scope_free(fn_scope);
    }

    return cn_support_diagnostics_error_count(diagnostics) == 0;
}

static void check_block_types(CnSemScope *scope, CnAstBlockStmt *block, CnDiagnostics *diagnostics, bool in_loop) {
    if (!block) return;
    CnSemScope *block_scope = cn_sem_scope_new(CN_SEM_SCOPE_BLOCK, scope);
    for (size_t i = 0; i < block->stmt_count; i++) {
        check_stmt_types(block_scope, block->stmts[i], diagnostics, in_loop);
    }
    cn_sem_scope_free(block_scope);
}

static void check_stmt_types(CnSemScope *scope, CnAstStmt *stmt, CnDiagnostics *diagnostics, bool in_loop) {
    if (!stmt) return;

    switch (stmt->kind) {
        case CN_AST_STMT_BLOCK:
            check_block_types(scope, stmt->as.block, diagnostics, in_loop);
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
                        cn_support_diag_semantic_error_type_mismatch(
                            diagnostics, NULL, 0, 0, "变量声明类型", "初始值类型");
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
            check_block_types(scope, stmt->as.if_stmt.then_block, diagnostics, in_loop);
            check_block_types(scope, stmt->as.if_stmt.else_block, diagnostics, in_loop);
            break;
        case CN_AST_STMT_WHILE:
            infer_expr_type(scope, stmt->as.while_stmt.condition, diagnostics);
            check_block_types(scope, stmt->as.while_stmt.body, diagnostics, true);
            break;
        case CN_AST_STMT_FOR: {
            CnSemScope *for_scope = cn_sem_scope_new(CN_SEM_SCOPE_BLOCK, scope);
            check_stmt_types(for_scope, stmt->as.for_stmt.init, diagnostics, in_loop);
            infer_expr_type(for_scope, stmt->as.for_stmt.condition, diagnostics);
            infer_expr_type(for_scope, stmt->as.for_stmt.update, diagnostics);
            check_block_types(for_scope, stmt->as.for_stmt.body, diagnostics, true);
            cn_sem_scope_free(for_scope);
            break;
        }
        case CN_AST_STMT_BREAK:
        case CN_AST_STMT_CONTINUE:
            if (!in_loop) {
                cn_support_diag_semantic_error_generic(
                    diagnostics,
                    CN_DIAG_CODE_SEM_BREAK_CONTINUE_OUTSIDE_LOOP,
                    NULL, 0, 0,
                    "语义错误：break 或 continue 语句不在循环内");
            }
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
                cn_support_diag_semantic_error_undefined_identifier(
                    diagnostics, NULL, 0, 0, expr->as.identifier.name);
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
        case CN_AST_EXPR_CALL: {
            CnType *callee_type = infer_expr_type(scope, expr->as.call.callee, diagnostics);
            
            // 特殊处理：内置函数 "长度" 可以接受字符串或数组
            if (expr->as.call.callee->kind == CN_AST_EXPR_IDENTIFIER &&
                strcmp(expr->as.call.callee->as.identifier.name, "长度") == 0) {
                
                if (expr->as.call.argument_count != 1) {
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_ARGUMENT_COUNT_MISMATCH,
                        NULL, 0, 0,
                        "语义错误：长度函数需要一个参数");
                } else {
                    CnType *arg_type = infer_expr_type(scope, expr->as.call.arguments[0], diagnostics);
                    if (arg_type && arg_type->kind != CN_TYPE_STRING && arg_type->kind != CN_TYPE_ARRAY) {
                        cn_support_diag_semantic_error_generic(
                            diagnostics,
                            CN_DIAG_CODE_SEM_ARGUMENT_TYPE_MISMATCH,
                            NULL, 0, 0,
                            "语义错误：长度函数参数必须是字符串或数组类型");
                    }
                }
                expr->type = cn_type_new_primitive(CN_TYPE_INT);
            }
            else if (callee_type && callee_type->kind == CN_TYPE_FUNCTION) {
                // 检查参数个数
                if (expr->as.call.argument_count != callee_type->as.function.param_count) {
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_ARGUMENT_COUNT_MISMATCH,
                        NULL, 0, 0,
                        "语义错误：函数调用参数个数不匹配");
                } else {
                    // 逐个检查参数类型
                    for (size_t i = 0; i < expr->as.call.argument_count; i++) {
                        CnType *arg_type = infer_expr_type(scope, expr->as.call.arguments[i], diagnostics);
                        if (arg_type && !cn_type_compatible(arg_type, callee_type->as.function.param_types[i])) {
                            cn_support_diag_semantic_error_generic(
                                diagnostics,
                                CN_DIAG_CODE_SEM_ARGUMENT_TYPE_MISMATCH,
                                NULL, 0, 0,
                                "语义错误：函数调用参数类型不匹配");
                        }
                    }
                }
                expr->type = callee_type->as.function.return_type;
            } else {
                if (callee_type && callee_type->kind != CN_TYPE_UNKNOWN) {
                    cn_support_diag_semantic_error_type_mismatch(
                        diagnostics, NULL, 0, 0, "函数类型", "非函数类型");
                }
                expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
            }
            break;
        }
        case CN_AST_EXPR_ARRAY_LITERAL: {
            // 数组字面量类型推导
            CnType *element_type = NULL;
            
            // 空数组 [] 默认为 int 数组
            if (expr->as.array_literal.element_count == 0) {
                element_type = cn_type_new_primitive(CN_TYPE_INT);
            } else {
                // 从第一个元素推导类型
                element_type = infer_expr_type(scope, expr->as.array_literal.elements[0], diagnostics);
                
                // 检查所有元素类型一致
                for (size_t i = 1; i < expr->as.array_literal.element_count; i++) {
                    CnType *curr_type = infer_expr_type(scope, expr->as.array_literal.elements[i], diagnostics);
                    if (curr_type && element_type && !cn_type_compatible(curr_type, element_type)) {
                        cn_support_diag_semantic_error_type_mismatch(
                            diagnostics, NULL, 0, 0, 
                            "数组元素类型不一致", "数组元素类型");
                        element_type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                        break;
                    }
                }
            }
            
            // 创建数组类型
            expr->type = cn_type_new_array(element_type, expr->as.array_literal.element_count);
            break;
        }
        case CN_AST_EXPR_INDEX: {
            // 数组索引访问类型推导 arr[index]
            CnType *array_type = infer_expr_type(scope, expr->as.index.array, diagnostics);
            CnType *index_type = infer_expr_type(scope, expr->as.index.index, diagnostics);
            
            // 检查数组类型
            if (!array_type || array_type->kind != CN_TYPE_ARRAY) {
                cn_support_diag_semantic_error_generic(
                    diagnostics,
                    CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                    NULL, 0, 0,
                    "语义错误：索引操作的对象必须是数组类型");
                expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
            } else if (!index_type || index_type->kind != CN_TYPE_INT) {
                // 检查索引类型，必须是整数
                cn_support_diag_semantic_error_generic(
                    diagnostics,
                    CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                    NULL, 0, 0,
                    "语义错误：数组索引必须是整数类型");
                expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
            } else {
                // 索引表达式的类型是数组元素的类型
                expr->type = array_type->as.array.element_type;
            }
            break;
        }
        default:
            expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
            break;
    }

    return expr->type;
}

// Freestanding 模式检查实现
bool cn_sem_check_freestanding(CnAstProgram *program,
                               struct CnDiagnostics *diagnostics,
                               bool enable_check) {
    return cn_fs_check_program(program, diagnostics, enable_check);
}
