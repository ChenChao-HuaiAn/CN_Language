#include "cnlang/frontend/semantics.h"
#include "cnlang/semantics/freestanding_check.h"
#include "cnlang/support/diagnostics.h"
#include <stdlib.h>

static CnType *infer_expr_type(CnSemScope *scope, CnAstExpr *expr, CnDiagnostics *diagnostics);
static void check_stmt_types(CnSemScope *scope, CnAstStmt *stmt, CnDiagnostics *diagnostics, bool in_loop);
static void check_block_types(CnSemScope *scope, CnAstBlockStmt *block, CnDiagnostics *diagnostics, bool in_loop);
static CnType *infer_function_return_type(CnSemScope *scope, CnAstBlockStmt *block, CnDiagnostics *diagnostics);

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
        case CN_AST_STMT_STRUCT_DECL:
            // 结构体声明在全局作用域构建时已处理
            break;
        case CN_AST_STMT_ENUM_DECL:
            // 枚举声明在全局作用域构建时已处理
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
        case CN_AST_EXPR_MEMBER_ACCESS:
            // 解析结构体成员访问表达式
            resolve_expr_names(scope, expr->as.member.object, diagnostics);
            break;
        case CN_AST_EXPR_STRUCT_LITERAL:
            // 解析结构体字面量表达式
            for (size_t i = 0; i < expr->as.struct_lit.field_count; i++) {
                resolve_expr_names(scope, expr->as.struct_lit.fields[i].value, diagnostics);
            }
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
        
        // 推断函数返回类型：遍历函数体中的return语句
        CnType *inferred_return_type = infer_function_return_type(fn_scope, fn->body, diagnostics);
        if (inferred_return_type) {
            // 更新函数符号的返回类型
            CnSemSymbol *fn_sym = cn_sem_scope_lookup(global_scope, fn->name, fn->name_length);
            if (fn_sym && fn_sym->type && fn_sym->type->kind == CN_TYPE_FUNCTION) {
                fn_sym->type->as.function.return_type = inferred_return_type;
            }
        }
        
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
                    // 特殊处理：显式数组类型与数组字面量类型的统一
                    if (decl->declared_type->kind == CN_TYPE_ARRAY && init_type && init_type->kind == CN_TYPE_ARRAY) {
                        // 如果显式类型的元素类型与初始化器的元素类型兼容，则使用初始化器的类型（包含长度信息）
                        CnType *decl_elem = decl->declared_type->as.array.element_type;
                        CnType *init_elem = init_type->as.array.element_type;
                        
                        if (decl_elem && init_elem && cn_type_compatible(decl_elem, init_elem)) {
                            // 使用初始化器的数组类型（带有实际长度）
                            sym->type = init_type;
                        } else {
                            // 元素类型不兼容，报错
                            cn_support_diag_semantic_error_type_mismatch(
                                diagnostics, NULL, 0, 0, "数组元素类型", "初始化器元素类型");
                            sym->type = decl->declared_type;
                        }
                    }
                    // 特殊处理：允许将函数名赋值给函数指针变量
                    // 如果声明类型是函数指针，且初始化器类型是函数，则允许
                    else if (decl->declared_type->kind == CN_TYPE_POINTER &&
                        decl->declared_type->as.pointer_to && decl->declared_type->as.pointer_to->kind == CN_TYPE_FUNCTION &&
                        init_type && init_type->kind == CN_TYPE_FUNCTION) {
                        // 检查函数类型是否匹配
                        if (!cn_type_equals(decl->declared_type->as.pointer_to, init_type)) {
                            cn_support_diag_semantic_error_generic(
                                diagnostics,
                                CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                                NULL, 0, 0,
                                "语义错误：函数类型与函数指针类型不匹配");
                        }
                        sym->type = decl->declared_type;
                    } else if (init_type && !cn_type_compatible(init_type, decl->declared_type)) {
                        // 报错：类型不匹配
                        cn_support_diag_semantic_error_type_mismatch(
                            diagnostics, NULL, 0, 0, "变量声明类型", "初始值类型");
                        sym->type = decl->declared_type;
                    } else {
                        sym->type = decl->declared_type;
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
        case CN_AST_STMT_IF: {
            CnType *cond_type = infer_expr_type(scope, stmt->as.if_stmt.condition, diagnostics);
            // 检查条件表达式是否为布尔类型
            if (cond_type && cond_type->kind != CN_TYPE_BOOL) {
                cn_support_diag_semantic_error_generic(
                    diagnostics,
                    CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                    NULL, 0, 0,
                    "语义错误：if 语句条件必须为布尔类型");
            }
            check_block_types(scope, stmt->as.if_stmt.then_block, diagnostics, in_loop);
            check_block_types(scope, stmt->as.if_stmt.else_block, diagnostics, in_loop);
            break;
        }
        case CN_AST_STMT_WHILE: {
            CnType *cond_type = infer_expr_type(scope, stmt->as.while_stmt.condition, diagnostics);
            // 检查条件表达式是否为布尔类型
            if (cond_type && cond_type->kind != CN_TYPE_BOOL) {
                cn_support_diag_semantic_error_generic(
                    diagnostics,
                    CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                    NULL, 0, 0,
                    "语义错误：while 语句条件必须为布尔类型");
            }
            check_block_types(scope, stmt->as.while_stmt.body, diagnostics, true);
            break;
        }
        case CN_AST_STMT_FOR: {
            CnSemScope *for_scope = cn_sem_scope_new(CN_SEM_SCOPE_BLOCK, scope);
            check_stmt_types(for_scope, stmt->as.for_stmt.init, diagnostics, in_loop);
            CnType *cond_type = infer_expr_type(for_scope, stmt->as.for_stmt.condition, diagnostics);
            // 检查条件表达式是否为布尔类型
            if (cond_type && cond_type->kind != CN_TYPE_BOOL) {
                cn_support_diag_semantic_error_generic(
                    diagnostics,
                    CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                    NULL, 0, 0,
                    "语义错误：for 语句条件必须为布尔类型");
            }
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
        case CN_AST_EXPR_FLOAT_LITERAL:
            expr->type = cn_type_new_primitive(CN_TYPE_FLOAT);
            break;
        case CN_AST_EXPR_STRING_LITERAL:
            expr->type = cn_type_new_primitive(CN_TYPE_STRING);
            break;
        case CN_AST_EXPR_BOOL_LITERAL:
            expr->type = cn_type_new_primitive(CN_TYPE_BOOL);
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
            
            // 指针加减运算：指针 +/- 整数
            if ((expr->as.binary.op == CN_AST_BINARY_OP_ADD ||
                 expr->as.binary.op == CN_AST_BINARY_OP_SUB) &&
                left && right) {
                if (left->kind == CN_TYPE_POINTER && right->kind == CN_TYPE_INT) {
                    expr->type = left;
                    break;
                }
                if (expr->as.binary.op == CN_AST_BINARY_OP_ADD &&
                    left->kind == CN_TYPE_INT && right->kind == CN_TYPE_POINTER) {
                    expr->type = right;
                    break;
                }
            }
            
            // 整数与浮点数混合运算：int + float -> float
            if (left && right) {
                CnTypeKind result_kind = CN_TYPE_UNKNOWN;
                
                if ((left->kind == CN_TYPE_INT || left->kind == CN_TYPE_FLOAT) &&
                    (right->kind == CN_TYPE_INT || right->kind == CN_TYPE_FLOAT)) {
                    // 如果任一操作数是 float，结果为 float；否则为 int
                    if (left->kind == CN_TYPE_FLOAT || right->kind == CN_TYPE_FLOAT) {
                        result_kind = CN_TYPE_FLOAT;
                    } else {
                        result_kind = CN_TYPE_INT;
                    }
                    
                    // 比较运算符返回布尔类型
                    if (expr->as.binary.op >= CN_AST_BINARY_OP_EQ && expr->as.binary.op <= CN_AST_BINARY_OP_GE) {
                        expr->type = cn_type_new_primitive(CN_TYPE_BOOL);
                    } else {
                        expr->type = cn_type_new_primitive(result_kind);
                    }
                    break;
                }
            }
            
            if (left && right && cn_type_compatible(left, right)) {
                // 简单的算术运算结果类型
                if (expr->as.binary.op >= CN_AST_BINARY_OP_EQ && expr->as.binary.op <= CN_AST_BINARY_OP_GE) {
                    expr->type = cn_type_new_primitive(CN_TYPE_BOOL);
                } else {
                    expr->type = left;
                }
            } else {
                expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
            }
            break;
        }
        case CN_AST_EXPR_ASSIGN: {
            // 赋值：先生成右值，再 STORE 到左值地址
            CnType *target = infer_expr_type(scope, expr->as.assign.target, diagnostics);
            CnType *val = infer_expr_type(scope, expr->as.assign.value, diagnostics);
            
            // 检查左值是否合法：只能是标识符或索引访问
            CnAstExpr *target_expr = expr->as.assign.target;
            if (target_expr->kind != CN_AST_EXPR_IDENTIFIER && 
                target_expr->kind != CN_AST_EXPR_INDEX) {
                cn_support_diag_semantic_error_generic(
                    diagnostics,
                    CN_DIAG_CODE_SEM_INVALID_ASSIGNMENT,
                    NULL, 0, 0,
                    "语义错误：赋值目标必须是变量或数组索引访问");
                expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                break;
            }
            
            // 特殊处理：允许将函数名赋值给函数指针
            // 如果 target 是函数指针类型，且 val 是函数类型，则允许赋值
            if (target && target->kind == CN_TYPE_POINTER &&
                target->as.pointer_to && target->as.pointer_to->kind == CN_TYPE_FUNCTION) {
                // target是函数指针类型
                if (val && val->kind == CN_TYPE_FUNCTION) {
                    // 检查函数类型是否匹配
                    if (cn_type_equals(target->as.pointer_to, val)) {
                        expr->type = target;
                        break;
                    } else {
                        cn_support_diag_semantic_error_generic(
                            diagnostics,
                            CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                            NULL, 0, 0,
                            "语义错误：函数类型与函数指针类型不匹配");
                        expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                        break;
                    }
                } else {
                    // 赋值的不是函数类型,报错
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                        NULL, 0, 0,
                        "语义错误：不能将非函数类型赋值给函数指针");
                    expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                    break;
                }
            }
            
            if (target && val && cn_type_compatible(val, target)) {
                expr->type = target;
            } else {
                // 类型不匹配,报错
                cn_support_diag_semantic_error_generic(
                    diagnostics,
                    CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                    NULL, 0, 0,
                    "语义错误：赋值类型不匹配");
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
            switch (expr->as.unary.op) {
                case CN_AST_UNARY_OP_NOT:
                    expr->type = cn_type_new_primitive(CN_TYPE_BOOL);
                    break;
                case CN_AST_UNARY_OP_MINUS:
                    expr->type = inner;
                    break;
                case CN_AST_UNARY_OP_ADDRESS_OF:
                    if (inner) {
                        expr->type = cn_type_new_pointer(inner);
                    } else {
                        expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                    }
                    break;
                case CN_AST_UNARY_OP_DEREFERENCE:
                    if (inner && inner->kind == CN_TYPE_POINTER && inner->as.pointer_to) {
                        expr->type = inner->as.pointer_to;
                    } else {
                        cn_support_diag_semantic_error_generic(
                            diagnostics,
                            CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                            NULL, 0, 0,
                            "语义错误：解引用操作的对象必须是指针类型");
                        expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                    }
                    break;
                default:
                    expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                    break;
            }
            break;
        }
        case CN_AST_EXPR_CALL: {
            CnType *callee_type = infer_expr_type(scope, expr->as.call.callee, diagnostics);
            
            // 特殊处理：内置函数 "长度" 可以接受字符串或数组
            if (expr->as.call.callee->kind == CN_AST_EXPR_IDENTIFIER &&
                expr->as.call.callee->as.identifier.name_length == strlen("长度") &&
                strncmp(expr->as.call.callee->as.identifier.name, "长度", 
                        expr->as.call.callee->as.identifier.name_length) == 0) {
                
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
            // 处理函数指针调用：函数指针是指向函数类型的指针
            else if (callee_type && callee_type->kind == CN_TYPE_POINTER && 
                     callee_type->as.pointer_to && 
                     callee_type->as.pointer_to->kind == CN_TYPE_FUNCTION) {
                CnType *func_type = callee_type->as.pointer_to;
                
                // 检查参数个数
                if (expr->as.call.argument_count != func_type->as.function.param_count) {
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_ARGUMENT_COUNT_MISMATCH,
                        NULL, 0, 0,
                        "语义错误：函数指针调用参数个数不匹配");
                } else {
                    // 逐个检查参数类型
                    for (size_t i = 0; i < expr->as.call.argument_count; i++) {
                        CnType *arg_type = infer_expr_type(scope, expr->as.call.arguments[i], diagnostics);
                        if (arg_type && !cn_type_compatible(arg_type, func_type->as.function.param_types[i])) {
                            cn_support_diag_semantic_error_generic(
                                diagnostics,
                                CN_DIAG_CODE_SEM_ARGUMENT_TYPE_MISMATCH,
                                NULL, 0, 0,
                                "语义错误：函数指针调用参数类型不匹配");
                        }
                    }
                }
                expr->type = func_type->as.function.return_type;
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
                        diagnostics, NULL, 0, 0, "函数或函数指针类型", "非函数类型");
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
        case CN_AST_EXPR_MEMBER_ACCESS: {
            // 成员访问类型推导：支持结构体 obj.member 和模块 module.member
            // 首先检查左操作数是否为模块
            if (expr->as.member.object->kind == CN_AST_EXPR_IDENTIFIER) {
                CnSemSymbol *sym = cn_sem_scope_lookup(
                    scope,
                    expr->as.member.object->as.identifier.name,
                    expr->as.member.object->as.identifier.name_length);
                
                // 如果是模块符号，在模块作用域中查找成员
                if (sym && sym->kind == CN_SEM_SYMBOL_MODULE && sym->as.module_scope) {
                    CnSemSymbol *member_sym = cn_sem_scope_lookup_shallow(
                        sym->as.module_scope,
                        expr->as.member.member_name,
                        expr->as.member.member_name_length);
                    
                    if (!member_sym) {
                        cn_support_diag_semantic_error_generic(
                            diagnostics,
                            CN_DIAG_CODE_SEM_MEMBER_NOT_FOUND,
                            NULL, 0, 0,
                            "语义错误：模块中不存在该成员");
                        expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                    } else {
                        // 模块成员访问的类型是成员的类型
                        expr->type = member_sym->type;
                        // 设置对象表达式的类型（标记为 VOID，表示模块）
                        expr->as.member.object->type = cn_type_new_primitive(CN_TYPE_VOID);
                    }
                    break;
                }
            }
            
            // 否则按照结构体成员访问处理
            CnType *object_type = infer_expr_type(scope, expr->as.member.object, diagnostics);
            
            // 如果是箭头访问，对象必须是指针类型
            if (expr->as.member.is_arrow) {
                if (!object_type || object_type->kind != CN_TYPE_POINTER) {
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                        NULL, 0, 0,
                        "语义错误：箭头操作符->的左操作数必须是指针类型");
                    expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                    break;
                }
                // 获取指针指向的类型
                object_type = object_type->as.pointer_to;
            }
            
            // 检查对象是否为结构体类型
            if (!object_type || object_type->kind != CN_TYPE_STRUCT) {
                cn_support_diag_semantic_error_generic(
                    diagnostics,
                    CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                    NULL, 0, 0,
                    "语义错误：成员访问操作的对象必须是结构体类型");
                expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                break;
            }
            
            // 在结构体类型中查找成员
            CnStructField *field = cn_type_struct_find_field(
                object_type,
                expr->as.member.member_name,
                expr->as.member.member_name_length);
            
            if (!field) {
                cn_support_diag_semantic_error_generic(
                    diagnostics,
                    CN_DIAG_CODE_SEM_MEMBER_NOT_FOUND,
                    NULL, 0, 0,
                    "语义错误：结构体中不存在该成员");
                expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
            } else {
                // 成员访问表达式的类型是成员的类型
                expr->type = field->field_type;
            }
            break;
        }
        case CN_AST_EXPR_STRUCT_LITERAL: {
            // 结构体字面量类型推导
            // 查找结构体类型定义
            CnSemSymbol *struct_sym = cn_sem_scope_lookup(
                scope,
                expr->as.struct_lit.struct_name,
                expr->as.struct_lit.struct_name_length);
            
            if (!struct_sym || struct_sym->kind != CN_SEM_SYMBOL_STRUCT) {
                cn_support_diag_semantic_error_generic(
                    diagnostics,
                    CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
                    NULL, 0, 0,
                    "语义错误：未定义的结构体类型");
                expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                break;
            }
            
            // 检查字段初始化的类型
            for (size_t i = 0; i < expr->as.struct_lit.field_count; i++) {
                CnStructField *field = cn_type_struct_find_field(
                    struct_sym->type,
                    expr->as.struct_lit.fields[i].field_name,
                    expr->as.struct_lit.fields[i].field_name_length);
                
                if (!field) {
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_MEMBER_NOT_FOUND,
                        NULL, 0, 0,
                        "语义错误：结构体中不存在该成员");
                } else {
                    CnType *init_type = infer_expr_type(
                        scope,
                        expr->as.struct_lit.fields[i].value,
                        diagnostics);
                    
                    if (init_type && !cn_type_compatible(init_type, field->field_type)) {
                        cn_support_diag_semantic_error_type_mismatch(
                            diagnostics, NULL, 0, 0,
                            "结构体字段类型", "初始化值类型");
                    }
                }
            }
            
            expr->type = struct_sym->type;
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

// 推断函数返回类型：遍历函数体中的return语句
static CnType *infer_return_from_stmt(CnAstStmt *stmt) {
    if (!stmt) return NULL;
    
    switch (stmt->kind) {
        case CN_AST_STMT_RETURN:
            // 找到return语句,返回其表达式的类型
            if (stmt->as.return_stmt.expr && stmt->as.return_stmt.expr->type) {
                return stmt->as.return_stmt.expr->type;
            }
            return cn_type_new_primitive(CN_TYPE_VOID);
        
        case CN_AST_STMT_BLOCK:
            // 遍历块中的所有语句
            if (stmt->as.block) {
                for (size_t i = 0; i < stmt->as.block->stmt_count; i++) {
                    CnType *ret = infer_return_from_stmt(stmt->as.block->stmts[i]);
                    if (ret) return ret;
                }
            }
            break;
        
        case CN_AST_STMT_IF:
            // 检查then分支
            if (stmt->as.if_stmt.then_block) {
                for (size_t i = 0; i < stmt->as.if_stmt.then_block->stmt_count; i++) {
                    CnType *ret = infer_return_from_stmt(stmt->as.if_stmt.then_block->stmts[i]);
                    if (ret) return ret;
                }
            }
            // 检查else分支
            if (stmt->as.if_stmt.else_block) {
                for (size_t i = 0; i < stmt->as.if_stmt.else_block->stmt_count; i++) {
                    CnType *ret = infer_return_from_stmt(stmt->as.if_stmt.else_block->stmts[i]);
                    if (ret) return ret;
                }
            }
            break;
        
        case CN_AST_STMT_WHILE:
            if (stmt->as.while_stmt.body) {
                for (size_t i = 0; i < stmt->as.while_stmt.body->stmt_count; i++) {
                    CnType *ret = infer_return_from_stmt(stmt->as.while_stmt.body->stmts[i]);
                    if (ret) return ret;
                }
            }
            break;
        
        case CN_AST_STMT_FOR:
            if (stmt->as.for_stmt.body) {
                for (size_t i = 0; i < stmt->as.for_stmt.body->stmt_count; i++) {
                    CnType *ret = infer_return_from_stmt(stmt->as.for_stmt.body->stmts[i]);
                    if (ret) return ret;
                }
            }
            break;
        
        default:
            break;
    }
    
    return NULL;
}

static CnType *infer_function_return_type(CnSemScope *scope, CnAstBlockStmt *block, CnDiagnostics *diagnostics) {
    if (!block) return NULL;
    
    // 遍历函数体中的所有语句，查找return语句
    for (size_t i = 0; i < block->stmt_count; i++) {
        CnType *ret_type = infer_return_from_stmt(block->stmts[i]);
        if (ret_type) {
            return ret_type;
        }
    }
    
    // 如果没有找到return语句,返回int类型(默认)
    return cn_type_new_primitive(CN_TYPE_INT);
}
