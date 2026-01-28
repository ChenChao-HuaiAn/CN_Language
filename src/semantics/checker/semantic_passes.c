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

    // 处理全局函数
    for (size_t i = 0; i < program->function_count; ++i) {
        CnAstFunctionDecl *fn = program->functions[i];
        
        // 进入函数作用域
        CnSemScope *fn_scope = cn_sem_scope_new(CN_SEM_SCOPE_FUNCTION, global_scope);
        
        // 插入参数以构建正确的函数内部作用域环境
        for (size_t j = 0; j < fn->parameter_count; j++) {
            CnSemSymbol *sym = cn_sem_scope_insert_symbol(fn_scope, fn->parameters[j].name, fn->parameters[j].name_length, CN_SEM_SYMBOL_VARIABLE);
            if (sym) {
                sym->is_const = fn->parameters[j].is_const;  // 传递常量参数标记
            }
        }

        resolve_block_names(fn_scope, fn->body, diagnostics);
        
        cn_sem_scope_free(fn_scope);
    }

    // 处理模块内函数
    for (size_t i = 0; i < program->module_count; ++i) {
        CnAstStmt *module_stmt = program->modules[i];
        if (module_stmt && module_stmt->kind == CN_AST_STMT_MODULE_DECL) {
            CnAstModuleDecl *module_decl = &module_stmt->as.module_decl;
            
            // 获取模块符号
            CnSemSymbol *module_sym = cn_sem_scope_lookup_shallow(
                global_scope,
                module_decl->name,
                module_decl->name_length);
            
            if (!module_sym || module_sym->kind != CN_SEM_SYMBOL_MODULE || !module_sym->as.module_scope) {
                continue;
            }
            
            CnSemScope *module_scope = module_sym->as.module_scope;
            
            // 处理模块内的每个函数
            for (size_t j = 0; j < module_decl->function_count; ++j) {
                CnAstFunctionDecl *fn = module_decl->functions[j];
                if (!fn) continue;
                
                // 进入函数作用域（父作用域是模块作用域）
                CnSemScope *fn_scope = cn_sem_scope_new(CN_SEM_SCOPE_FUNCTION, module_scope);
                
                // 插入参数
                for (size_t k = 0; k < fn->parameter_count; k++) {
                    CnSemSymbol *sym = cn_sem_scope_insert_symbol(fn_scope, fn->parameters[k].name, fn->parameters[k].name_length, CN_SEM_SYMBOL_VARIABLE);
                    if (sym) {
                        sym->is_const = fn->parameters[k].is_const;  // 传递常量参数标记
                    }
                }
                
                resolve_block_names(fn_scope, fn->body, diagnostics);
                
                cn_sem_scope_free(fn_scope);
            }
        }
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
            // 解析成员访问表达式：支持结构体和模块成员访问
            // 先解析对象部分
            resolve_expr_names(scope, expr->as.member.object, diagnostics);
            
            // 如果对象是模块，验证成员是否存在
            if (expr->as.member.object->kind == CN_AST_EXPR_IDENTIFIER) {
                CnSemSymbol *obj_sym = cn_sem_scope_lookup(
                    scope,
                    expr->as.member.object->as.identifier.name,
                    expr->as.member.object->as.identifier.name_length);
                
                // 如果对象是模块，检查成员是否在模块作用域中存在
                if (obj_sym && obj_sym->kind == CN_SEM_SYMBOL_MODULE && obj_sym->as.module_scope) {
                    CnSemSymbol *member_sym = cn_sem_scope_lookup_shallow(
                        obj_sym->as.module_scope,
                        expr->as.member.member_name,
                        expr->as.member.member_name_length);
                    
                    if (!member_sym) {
                        cn_support_diag_semantic_error_undefined_identifier(
                            diagnostics, NULL, 0, 0, expr->as.member.member_name);
                    }
                }
            }
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

    // 推断全局变量的类型
    for (size_t i = 0; i < program->global_var_count; ++i) {
        CnAstStmt *var_stmt = program->global_vars[i];
        if (!var_stmt || var_stmt->kind != CN_AST_STMT_VAR_DECL) {
            continue;
        }
        
        CnAstVarDecl *var_decl = &var_stmt->as.var_decl;
        
        // 如果没有显式类型且有初始化表达式，从初始化表达式推断类型
        if (!var_decl->declared_type && var_decl->initializer) {
            CnType *init_type = infer_expr_type(global_scope, var_decl->initializer, diagnostics);
            if (init_type) {
                var_decl->declared_type = init_type;
                
                // 同时更新全局作用域中该变量的类型
                CnSemSymbol *sym = cn_sem_scope_lookup_shallow(global_scope, 
                                                               var_decl->name, 
                                                               var_decl->name_length);
                if (sym) {
                    sym->type = init_type;
                }
            }
        }
    }

    // 阶段1：仅推断所有函数的返回类型，不进行函数体内部的类型检查
    // 这确保在检查函数调用时，所有函数的返回类型已知

    // 推断全局函数的返回类型
    for (size_t i = 0; i < program->function_count; ++i) {
        CnAstFunctionDecl *fn = program->functions[i];
        
        CnSemScope *fn_scope = cn_sem_scope_new(CN_SEM_SCOPE_FUNCTION, global_scope);
        for (size_t j = 0; j < fn->parameter_count; j++) {
            CnSemSymbol *sym = cn_sem_scope_insert_symbol(fn_scope, fn->parameters[j].name, fn->parameters[j].name_length, CN_SEM_SYMBOL_VARIABLE);
            if (sym) {
                sym->type = fn->parameters[j].declared_type;
                sym->is_const = fn->parameters[j].is_const;  // 传递常量参数标记
            }
        }
        
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

    // 推断模块函数的返回类型
    for (size_t i = 0; i < program->module_count; ++i) {
        CnAstStmt *module_stmt = program->modules[i];
        if (module_stmt && module_stmt->kind == CN_AST_STMT_MODULE_DECL) {
            CnAstModuleDecl *module_decl = &module_stmt->as.module_decl;
            
            // 获取模块符号
            CnSemSymbol *module_sym = cn_sem_scope_lookup_shallow(
                global_scope,
                module_decl->name,
                module_decl->name_length);
            
            if (!module_sym || module_sym->kind != CN_SEM_SYMBOL_MODULE || !module_sym->as.module_scope) {
                continue;
            }
            
            CnSemScope *module_scope = module_sym->as.module_scope;
            
            // 处理模块内的每个函数
            for (size_t j = 0; j < module_decl->function_count; ++j) {
                CnAstFunctionDecl *fn = module_decl->functions[j];
                if (!fn) continue;
                
                // 进入函数作用域（父作用域是模块作用域）
                CnSemScope *fn_scope = cn_sem_scope_new(CN_SEM_SCOPE_FUNCTION, module_scope);
                for (size_t k = 0; k < fn->parameter_count; k++) {
                    CnSemSymbol *sym = cn_sem_scope_insert_symbol(fn_scope, fn->parameters[k].name, fn->parameters[k].name_length, CN_SEM_SYMBOL_VARIABLE);
                    if (sym) {
                        sym->type = fn->parameters[k].declared_type;
                        sym->is_const = fn->parameters[k].is_const;  // 传递常量参数标记
                    }
                }
                
                // 推断函数返回类型
                CnType *inferred_return_type = infer_function_return_type(fn_scope, fn->body, diagnostics);
                if (inferred_return_type) {
                    // 更新函数符号的返回类型
                    CnSemSymbol *fn_sym = cn_sem_scope_lookup_shallow(module_scope, fn->name, fn->name_length);
                    if (fn_sym && fn_sym->type && fn_sym->type->kind == CN_TYPE_FUNCTION) {
                        fn_sym->type->as.function.return_type = inferred_return_type;
                    }
                }
                
                cn_sem_scope_free(fn_scope);
            }
        }
    }

    // 阶段2：进行函数体内部的完整类型检查
    // 此时所有函数的返回类型已经推断完成

    // 检查全局函数
    for (size_t i = 0; i < program->function_count; ++i) {
        CnAstFunctionDecl *fn = program->functions[i];
        
        CnSemScope *fn_scope = cn_sem_scope_new(CN_SEM_SCOPE_FUNCTION, global_scope);
        cn_sem_scope_set_name(fn_scope, fn->name, fn->name_length);  // 设置函数作用域名称
        for (size_t j = 0; j < fn->parameter_count; j++) {
            CnSemSymbol *sym = cn_sem_scope_insert_symbol(fn_scope, fn->parameters[j].name, fn->parameters[j].name_length, CN_SEM_SYMBOL_VARIABLE);
            if (sym) {
                sym->type = fn->parameters[j].declared_type;
                sym->is_const = fn->parameters[j].is_const;  // 传递常量参数标记
            }
        }

        check_block_types(fn_scope, fn->body, diagnostics, false);
        
        cn_sem_scope_free(fn_scope);
    }

    // 检查模块函数
    for (size_t i = 0; i < program->module_count; ++i) {
        CnAstStmt *module_stmt = program->modules[i];
        if (module_stmt && module_stmt->kind == CN_AST_STMT_MODULE_DECL) {
            CnAstModuleDecl *module_decl = &module_stmt->as.module_decl;
            
            // 获取模块符号
            CnSemSymbol *module_sym = cn_sem_scope_lookup_shallow(
                global_scope,
                module_decl->name,
                module_decl->name_length);
            
            if (!module_sym || module_sym->kind != CN_SEM_SYMBOL_MODULE || !module_sym->as.module_scope) {
                continue;
            }
            
            CnSemScope *module_scope = module_sym->as.module_scope;
            
            // 处理模块内的每个函数
            for (size_t j = 0; j < module_decl->function_count; ++j) {
                CnAstFunctionDecl *fn = module_decl->functions[j];
                if (!fn) continue;
                
                // 进入函数作用域（父作用域是模块作用域）
                CnSemScope *fn_scope = cn_sem_scope_new(CN_SEM_SCOPE_FUNCTION, module_scope);
                cn_sem_scope_set_name(fn_scope, fn->name, fn->name_length);  // 设置函数作用域名称
                for (size_t k = 0; k < fn->parameter_count; k++) {
                    CnSemSymbol *sym = cn_sem_scope_insert_symbol(fn_scope, fn->parameters[k].name, fn->parameters[k].name_length, CN_SEM_SYMBOL_VARIABLE);
                    if (sym) {
                        sym->type = fn->parameters[k].declared_type;
                        sym->is_const = fn->parameters[k].is_const;  // 传递常量参数标记
                    }
                }
                
                check_block_types(fn_scope, fn->body, diagnostics, false);
                
                cn_sem_scope_free(fn_scope);
            }
        }
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
                sym->is_const = decl->is_const;
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
        case CN_AST_STMT_SWITCH: {
            // 检查 switch 表达式的类型（必须是整数或枚举）
            CnType *switch_type = infer_expr_type(scope, stmt->as.switch_stmt.expr, diagnostics);
            if (switch_type && switch_type->kind != CN_TYPE_INT && switch_type->kind != CN_TYPE_ENUM) {
                cn_support_diag_semantic_error_generic(
                    diagnostics,
                    CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                    NULL, 0, 0,
                    "语义错误：switch 表达式必须为整数或枚举类型");
            }

            // 检查每个 case 的值表达式是否为常量且类型匹配
            int has_default = 0;
            for (size_t i = 0; i < stmt->as.switch_stmt.case_count; i++) {
                CnAstSwitchCase *case_stmt = &stmt->as.switch_stmt.cases[i];

                if (case_stmt->value == NULL) {
                    // default 分支
                    if (has_default) {
                        cn_support_diag_semantic_error_generic(
                            diagnostics,
                            CN_DIAG_CODE_SEM_DUPLICATE_SYMBOL,
                            NULL, 0, 0,
                            "语义错误：switch 语句中有多个 default 分支");
                    }
                    has_default = 1;
                } else {
                    // case 分支：检查值表达式类型
                    CnType *case_type = infer_expr_type(scope, case_stmt->value, diagnostics);
                    if (case_type && switch_type && !cn_type_compatible(case_type, switch_type)) {
                        cn_support_diag_semantic_error_generic(
                            diagnostics,
                            CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                            NULL, 0, 0,
                            "语义错误：case 值类型与 switch 表达式类型不匹配");
                    }

                    // TODO: 检查 case 值是否为常量表达式
                    // TODO: 检查是否有重复的 case 值
                }

                // 检查 case 体的语句块（在 switch 中 break 是允许的）
                check_block_types(scope, case_stmt->body, diagnostics, true);
            }
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
        case CN_AST_STMT_STRUCT_DECL: {
            // 局部结构体定义：注册到当前作用域
            CnAstStructDecl *struct_decl = &stmt->as.struct_decl;
            CnSemSymbol *sym = cn_sem_scope_insert_symbol(scope,
                                       struct_decl->name,
                                       struct_decl->name_length,
                                       CN_SEM_SYMBOL_STRUCT);
            if (sym) {
                // 创建结构体类型，包含字段信息
                CnStructField *fields = NULL;
                if (struct_decl->field_count > 0) {
                    fields = (CnStructField *)malloc(sizeof(CnStructField) * struct_decl->field_count);
                    for (size_t j = 0; j < struct_decl->field_count; j++) {
                        fields[j].name = struct_decl->fields[j].name;
                        fields[j].name_length = struct_decl->fields[j].name_length;
                        fields[j].field_type = struct_decl->fields[j].field_type;
                        fields[j].is_const = struct_decl->fields[j].is_const;
                    }
                }
                
                // 局部结构体应该绑定到函数作用域,向上查找直到函数作用域
                CnSemScope *decl_scope = scope;
                while (decl_scope && cn_sem_scope_get_kind(decl_scope) != CN_SEM_SCOPE_FUNCTION 
                       && cn_sem_scope_get_kind(decl_scope) != CN_SEM_SCOPE_GLOBAL
                       && cn_sem_scope_get_kind(decl_scope) != CN_SEM_SCOPE_MODULE) {
                    decl_scope = cn_sem_scope_parent(decl_scope);
                }
                
                // 获取函数名(如果是局部结构体)
                const char *owner_func_name = NULL;
                size_t owner_func_name_length = 0;
                if (decl_scope && cn_sem_scope_get_kind(decl_scope) == CN_SEM_SCOPE_FUNCTION) {
                    owner_func_name = cn_sem_scope_get_name(decl_scope, &owner_func_name_length);
                }
                
                sym->type = cn_type_new_struct(struct_decl->name,
                                              struct_decl->name_length,
                                              fields,
                                              struct_decl->field_count,
                                              decl_scope,
                                              owner_func_name,
                                              owner_func_name_length);
            } else {
                // 报告重复定义
                cn_support_diag_semantic_error_duplicate_symbol(
                    diagnostics, NULL, 0, 0, struct_decl->name);
            }
            break;
        }
        case CN_AST_STMT_ENUM_DECL: {
            // 局部枚举定义：注册到当前作用域
            CnAstEnumDecl *enum_decl = &stmt->as.enum_decl;
            CnSemSymbol *sym = cn_sem_scope_insert_symbol(scope,
                                       enum_decl->name,
                                       enum_decl->name_length,
                                       CN_SEM_SYMBOL_ENUM);
            if (sym) {
                // 创建枚举类型
                sym->type = cn_type_new_enum(enum_decl->name, enum_decl->name_length);
                
                // 为枚举创建一个作用域来存储其成员
                CnSemScope *enum_scope = cn_sem_scope_new(CN_SEM_SCOPE_ENUM, scope);
                if (enum_scope && sym->type) {
                    sym->type->as.enum_type.enum_scope = enum_scope;
                    
                    // 注册枚举成员到枚举作用域
                    for (size_t j = 0; j < enum_decl->member_count; j++) {
                        CnAstEnumMember *member = &enum_decl->members[j];
                        CnSemSymbol *member_sym = cn_sem_scope_insert_symbol(enum_scope,
                                                           member->name,
                                                           member->name_length,
                                                           CN_SEM_SYMBOL_ENUM_MEMBER);
                        if (member_sym) {
                            // 枚举成员的类型是整数
                            member_sym->type = cn_type_new_primitive(CN_TYPE_INT);
                            // 保存枚举成员的值
                            member_sym->as.enum_value = member->value;
                        } else {
                            // 报告重复定义
                            cn_support_diag_semantic_error_duplicate_symbol(
                                diagnostics, NULL, 0, 0, member->name);
                        }
                    }
                }
            } else {
                // 报告重复定义
                cn_support_diag_semantic_error_duplicate_symbol(
                    diagnostics, NULL, 0, 0, enum_decl->name);
            }
            break;
        }
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
            
            // 特殊处理：字符串拼接（字符串 + 任何类型）
            if (expr->as.binary.op == CN_AST_BINARY_OP_ADD) {
                // 如果任一操作数是字符串，则结果为字符串类型
                if ((left && left->kind == CN_TYPE_STRING) || (right && right->kind == CN_TYPE_STRING)) {
                    expr->type = cn_type_new_primitive(CN_TYPE_STRING);
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
            
            // 检查左值是否合法：只能是标识符、索引访问、成员访问或解引用表达式
            CnAstExpr *target_expr = expr->as.assign.target;
            bool is_valid_lvalue = (target_expr->kind == CN_AST_EXPR_IDENTIFIER) ||
                                    (target_expr->kind == CN_AST_EXPR_INDEX) ||
                                    (target_expr->kind == CN_AST_EXPR_MEMBER_ACCESS) ||
                                    (target_expr->kind == CN_AST_EXPR_UNARY && 
                                     target_expr->as.unary.op == CN_AST_UNARY_OP_DEREFERENCE);
            
            if (!is_valid_lvalue) {
                cn_support_diag_semantic_error_generic(
                    diagnostics,
                    CN_DIAG_CODE_SEM_INVALID_ASSIGNMENT,
                    expr->loc.filename,
                    expr->loc.line,
                    expr->loc.column,
                    "语义错误：赋值目标必须是变量、数组索引访问、结构体成员访问或解引用表达式");
                expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                break;
            }
            
            // 检查是否向常量变量赋值
            if (target_expr->kind == CN_AST_EXPR_IDENTIFIER) {
                CnAstIdentifierExpr *id = &target_expr->as.identifier;
                CnSemSymbol *sym = cn_sem_scope_lookup(scope, id->name, id->name_length);
                if (sym && sym->kind == CN_SEM_SYMBOL_VARIABLE && sym->is_const) {
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_INVALID_ASSIGNMENT,
                        expr->loc.filename,
                        expr->loc.line,
                        expr->loc.column,
                        "语义错误：不能给常量变量赋值");
                    expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                    break;
                }
            }
            // 检查是否向常量字段赋值
            else if (target_expr->kind == CN_AST_EXPR_MEMBER_ACCESS) {
                // 先计算对象类型
                CnType *object_type = infer_expr_type(scope, target_expr->as.member.object, diagnostics);
                
                // 如果是箭头访问，获取指针指向的类型
                if (target_expr->as.member.is_arrow && object_type && object_type->kind == CN_TYPE_POINTER) {
                    object_type = object_type->as.pointer_to;
                }
                
                // 检查是否为结构体类型
                if (object_type && object_type->kind == CN_TYPE_STRUCT) {
                    CnStructField *field = cn_type_struct_find_field(
                        object_type,
                        target_expr->as.member.member_name,
                        target_expr->as.member.member_name_length);
                    
                    if (field && field->is_const) {
                        cn_support_diag_semantic_error_generic(
                            diagnostics,
                            CN_DIAG_CODE_SEM_INVALID_ASSIGNMENT,
                            expr->loc.filename,
                            expr->loc.line,
                            expr->loc.column,
                            "语义错误：不能给常量字段赋值");
                        expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                        break;
                    }
                }
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
                            expr->loc.filename,
                            expr->loc.line,
                            expr->loc.column,
                            "语义错误：函数类型与函数指针类型不匹配");
                        expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                        break;
                    }
                } else {
                    // 赋值的不是函数类型,报错
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                        expr->loc.filename,
                        expr->loc.line,
                        expr->loc.column,
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
                    expr->loc.filename,
                    expr->loc.line,
                    expr->loc.column,
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
                case CN_AST_UNARY_OP_PRE_INC:
                case CN_AST_UNARY_OP_PRE_DEC:
                case CN_AST_UNARY_OP_POST_INC:
                case CN_AST_UNARY_OP_POST_DEC:
                    // 自增/自减运算符：要求操作数是数值类型，返回相同类型
                    if (inner && (inner->kind == CN_TYPE_INT || inner->kind == CN_TYPE_FLOAT)) {
                        expr->type = inner;
                    } else {
                        cn_support_diag_semantic_error_generic(
                            diagnostics,
                            CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                            NULL, 0, 0,
                            "语义错误：自增/自减运算符的操作数必须是数值类型");
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
            // 支持两种形式：长度(arr) 和 arr.长度()
            bool is_length_builtin = false;
            CnAstExpr *length_target = NULL;  // 用于存储要获取长度的对象
            
            // 检查函数风格：长度(arr)
            if (expr->as.call.callee->kind == CN_AST_EXPR_IDENTIFIER &&
                expr->as.call.callee->as.identifier.name_length == strlen("长度") &&
                strncmp(expr->as.call.callee->as.identifier.name, "长度", 
                        expr->as.call.callee->as.identifier.name_length) == 0) {
                is_length_builtin = true;
                
                if (expr->as.call.argument_count != 1) {
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_ARGUMENT_COUNT_MISMATCH,
                        NULL, 0, 0,
                        "语义错误：长度函数需要一个参数");
                } else {
                    length_target = expr->as.call.arguments[0];
                }
            }
            // 检查方法风格：arr.长度()
            else if (expr->as.call.callee->kind == CN_AST_EXPR_MEMBER_ACCESS &&
                     expr->as.call.callee->as.member.member_name_length == strlen("长度") &&
                     strncmp(expr->as.call.callee->as.member.member_name, "长度",
                             expr->as.call.callee->as.member.member_name_length) == 0) {
                is_length_builtin = true;
                
                if (expr->as.call.argument_count != 0) {
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_ARGUMENT_COUNT_MISMATCH,
                        NULL, 0, 0,
                        "语义错误：方法风格长度调用不接受参数");
                } else {
                    length_target = expr->as.call.callee->as.member.object;
                }
            }
            
            // 特殊处理：内置函数 "打印" 可以接受任何类型（根据参数类型调用不同的运行时函数）
            bool is_print_builtin = false;
            if (expr->as.call.callee->kind == CN_AST_EXPR_IDENTIFIER &&
                expr->as.call.callee->as.identifier.name_length == strlen("打印") &&
                strncmp(expr->as.call.callee->as.identifier.name, "打印",
                        expr->as.call.callee->as.identifier.name_length) == 0) {
                is_print_builtin = true;
                
                if (expr->as.call.argument_count != 1) {
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_ARGUMENT_COUNT_MISMATCH,
                        NULL, 0, 0,
                        "语义错误：打印函数需要一个参数");
                } else {
                    // 推断参数类型
                    CnType *arg_type = infer_expr_type(scope, expr->as.call.arguments[0], diagnostics);
                    // 打印函数接受任何类型，无需类型检查
                    (void)arg_type;  // 避免未使用变量警告
                }
                // 打印函数返回 void 类型
                expr->type = cn_type_new_primitive(CN_TYPE_VOID);
            }
            
            // 如果是长度内建函数，进行类型检查
            if (is_length_builtin && length_target) {
                CnType *target_type = infer_expr_type(scope, length_target, diagnostics);
                if (target_type && target_type->kind != CN_TYPE_STRING && target_type->kind != CN_TYPE_ARRAY) {
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_ARGUMENT_TYPE_MISMATCH,
                        NULL, 0, 0,
                        "语义错误：长度函数参数必须是字符串或数组类型");
                }
                expr->type = cn_type_new_primitive(CN_TYPE_INT);
            }
            // 如果是打印内建函数，已经处理完毕
            else if (is_print_builtin) {
                // 已经设置了 expr->type = CN_TYPE_VOID，无需额外处理
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
            // 成员访问类型推导：支持结构体 obj.member、模块 module.member、枚举 enum.member 和内建方法 arr.长度()
            // 首先检查左操作数是否为模块或枚举
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
                        // 检查可见性：私有成员只能在模块内访问
                        // 对于模块外部的访问（通过 模块名.成员名 的方式），
                        // 检查成员是否为公开
                        if (member_sym->is_public == 0) {
                            // 私有成员，报错
                            cn_support_diag_semantic_error_generic(
                                diagnostics,
                                CN_DIAG_CODE_SEM_ACCESS_DENIED,
                                NULL, 0, 0,
                                "语义错误：不能访问模块的私有成员");
                        }
                        // 模块成员访问的类型是成员的类型
                        expr->type = member_sym->type;
                        // 设置对象表达式的类型（标记为 VOID，表示模块）
                        expr->as.member.object->type = cn_type_new_primitive(CN_TYPE_VOID);
                    }
                    break;
                }
                
                // 如果是枚举符号，在枚举作用域中查找成员
                if (sym && sym->kind == CN_SEM_SYMBOL_ENUM && sym->type && 
                    sym->type->kind == CN_TYPE_ENUM && sym->type->as.enum_type.enum_scope) {
                    CnSemSymbol *member_sym = cn_type_enum_find_member(
                        sym->type,
                        expr->as.member.member_name,
                        expr->as.member.member_name_length);
                    
                    if (!member_sym) {
                        cn_support_diag_semantic_error_generic(
                            diagnostics,
                            CN_DIAG_CODE_SEM_MEMBER_NOT_FOUND,
                            NULL, 0, 0,
                            "语义错误：枚举中不存在该成员");
                        expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                    } else {
                        // 枚举成员访问的类型是成员的类型（整数）
                        expr->type = member_sym->type;
                        // 设置对象表达式的类型（标记为枚举类型）
                        expr->as.member.object->type = sym->type;
                    }
                    break;
                }
            }
            
            // 否则按照结构体成员访问处理
            CnType *object_type = infer_expr_type(scope, expr->as.member.object, diagnostics);
            
            // 特殊处理：内建方法 "长度"，支持数组和字符串
            if (expr->as.member.member_name_length == strlen("长度") &&
                strncmp(expr->as.member.member_name, "长度",
                        expr->as.member.member_name_length) == 0) {
                // 检查对象类型是否为数组或字符串
                if (object_type && 
                    (object_type->kind == CN_TYPE_ARRAY || object_type->kind == CN_TYPE_STRING)) {
                    // "长度"内建方法访问，类型为函数（在调用时会特殊处理）
                    // 暂时标记为 UNKNOWN，在 CALL 节点会特殊处理
                    expr->type = cn_type_new_primitive(CN_TYPE_UNKNOWN);
                    break;
                }
            }
            
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
            size_t positional_index = 0;
            int saw_named = 0;
            CnType *struct_type = struct_sym->type;
            for (size_t i = 0; i < expr->as.struct_lit.field_count; i++) {
                CnAstStructFieldInit *init = &expr->as.struct_lit.fields[i];
                CnStructField *field = NULL;

                if (init->field_name && init->field_name_length > 0) {
                    // 指定成员初始化：按名称查找字段
                    saw_named = 1;
                    field = cn_type_struct_find_field(
                        struct_type,
                        init->field_name,
                        init->field_name_length);
                } else {
                    // 位置初始化：按结构体定义顺序匹配字段
                    if (saw_named) {
                        cn_support_diag_semantic_error_generic(
                            diagnostics,
                            CN_DIAG_CODE_SEM_TYPE_MISMATCH,
                            NULL, 0, 0,
                            "语义错误：不能在指定成员初始化之后使用位置初始化");
                    }

                    if (!struct_type || struct_type->kind != CN_TYPE_STRUCT ||
                        positional_index >= struct_type->as.struct_type.field_count) {
                        cn_support_diag_semantic_error_generic(
                            diagnostics,
                            CN_DIAG_CODE_SEM_MEMBER_NOT_FOUND,
                            NULL, 0, 0,
                            "语义错误：结构体位置初始化的字段数量超出定义");
                    } else {
                        field = &struct_type->as.struct_type.fields[positional_index++];
                        // 将位置初始化回写成具名初始化，方便后续 C 代码生成
                        init->field_name = field->name;
                        init->field_name_length = field->name_length;
                    }
                }
                
                if (!field) {
                    cn_support_diag_semantic_error_generic(
                        diagnostics,
                        CN_DIAG_CODE_SEM_MEMBER_NOT_FOUND,
                        NULL, 0, 0,
                        "语义错误：结构体中不存在该成员");
                } else {
                    CnType *init_type = infer_expr_type(
                        scope,
                        init->value,
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
static CnType *infer_return_from_stmt(CnSemScope *scope, CnAstStmt *stmt, CnDiagnostics *diagnostics) {
    if (!stmt) return NULL;
    
    switch (stmt->kind) {
        case CN_AST_STMT_RETURN:
            // 找到return语句,推断并返回其表达式的类型
            if (stmt->as.return_stmt.expr) {
                CnType *ret_type = infer_expr_type(scope, stmt->as.return_stmt.expr, diagnostics);
                if (ret_type) {
                    return ret_type;
                }
            }
            return cn_type_new_primitive(CN_TYPE_VOID);
        
        case CN_AST_STMT_BLOCK:
            // 需要创建块作用域并处理变量声明
            if (stmt->as.block) {
                CnSemScope *block_scope = cn_sem_scope_new(CN_SEM_SCOPE_BLOCK, scope);
                for (size_t i = 0; i < stmt->as.block->stmt_count; i++) {
                    CnAstStmt *block_stmt = stmt->as.block->stmts[i];
                    
                    // 如果是变量声明，需要先添加到作用域
                    if (block_stmt && block_stmt->kind == CN_AST_STMT_VAR_DECL) {
                        CnAstVarDecl *decl = &block_stmt->as.var_decl;
                        CnType *init_type = infer_expr_type(block_scope, decl->initializer, diagnostics);
                        CnSemSymbol *sym = cn_sem_scope_insert_symbol(block_scope, decl->name, decl->name_length, CN_SEM_SYMBOL_VARIABLE);
                        if (sym) {
                            sym->type = decl->declared_type ? decl->declared_type : init_type;
                        }
                    }
                    
                    // 查找return语句
                    CnType *ret = infer_return_from_stmt(block_scope, block_stmt, diagnostics);
                    if (ret) {
                        cn_sem_scope_free(block_scope);
                        return ret;
                    }
                }
                cn_sem_scope_free(block_scope);
            }
            break;
        
        case CN_AST_STMT_IF:
            // 检查then分支
            if (stmt->as.if_stmt.then_block) {
                for (size_t i = 0; i < stmt->as.if_stmt.then_block->stmt_count; i++) {
                    CnType *ret = infer_return_from_stmt(scope, stmt->as.if_stmt.then_block->stmts[i], diagnostics);
                    if (ret) return ret;
                }
            }
            // 检查else分支
            if (stmt->as.if_stmt.else_block) {
                for (size_t i = 0; i < stmt->as.if_stmt.else_block->stmt_count; i++) {
                    CnType *ret = infer_return_from_stmt(scope, stmt->as.if_stmt.else_block->stmts[i], diagnostics);
                    if (ret) return ret;
                }
            }
            break;
        
        case CN_AST_STMT_WHILE:
            if (stmt->as.while_stmt.body) {
                for (size_t i = 0; i < stmt->as.while_stmt.body->stmt_count; i++) {
                    CnType *ret = infer_return_from_stmt(scope, stmt->as.while_stmt.body->stmts[i], diagnostics);
                    if (ret) return ret;
                }
            }
            break;
        
        case CN_AST_STMT_FOR:
            if (stmt->as.for_stmt.body) {
                for (size_t i = 0; i < stmt->as.for_stmt.body->stmt_count; i++) {
                    CnType *ret = infer_return_from_stmt(scope, stmt->as.for_stmt.body->stmts[i], diagnostics);
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
    
    // 创建一个临时的块作用域用于推断过程
    CnSemScope *block_scope = cn_sem_scope_new(CN_SEM_SCOPE_BLOCK, scope);
    
    // 遍历函数体中的所有语句，处理变量声明、结构体/枚举定义并查找return语句
    for (size_t i = 0; i < block->stmt_count; i++) {
        CnAstStmt *stmt = block->stmts[i];
        
        // 如果是变量声明，需要先添加到作用域
        if (stmt && stmt->kind == CN_AST_STMT_VAR_DECL) {
            CnAstVarDecl *decl = &stmt->as.var_decl;
            CnType *init_type = infer_expr_type(block_scope, decl->initializer, diagnostics);
            CnSemSymbol *sym = cn_sem_scope_insert_symbol(block_scope, decl->name, decl->name_length, CN_SEM_SYMBOL_VARIABLE);
            if (sym) {
                sym->type = decl->declared_type ? decl->declared_type : init_type;
            }
        }
        // 如果是结构体定义，需要先注册到作用域
        else if (stmt && stmt->kind == CN_AST_STMT_STRUCT_DECL) {
            CnAstStructDecl *struct_decl = &stmt->as.struct_decl;
            CnSemSymbol *sym = cn_sem_scope_insert_symbol(block_scope,
                                       struct_decl->name,
                                       struct_decl->name_length,
                                       CN_SEM_SYMBOL_STRUCT);
            if (sym) {
                CnStructField *fields = NULL;
                if (struct_decl->field_count > 0) {
                    fields = (CnStructField *)malloc(sizeof(CnStructField) * struct_decl->field_count);
                    for (size_t j = 0; j < struct_decl->field_count; j++) {
                        fields[j].name = struct_decl->fields[j].name;
                        fields[j].name_length = struct_decl->fields[j].name_length;
                        fields[j].field_type = struct_decl->fields[j].field_type;
                        fields[j].is_const = struct_decl->fields[j].is_const;
                    }
                }
                
                // 局部结构体应该绑定到函数作用域,向上查找直到函数作用域
                CnSemScope *decl_scope = block_scope;
                while (decl_scope && cn_sem_scope_get_kind(decl_scope) != CN_SEM_SCOPE_FUNCTION 
                       && cn_sem_scope_get_kind(decl_scope) != CN_SEM_SCOPE_GLOBAL
                       && cn_sem_scope_get_kind(decl_scope) != CN_SEM_SCOPE_MODULE) {
                    decl_scope = cn_sem_scope_parent(decl_scope);
                }
                
                // 获取函数名(如果是局部结构体)
                const char *owner_func_name = NULL;
                size_t owner_func_name_length = 0;
                if (decl_scope && cn_sem_scope_get_kind(decl_scope) == CN_SEM_SCOPE_FUNCTION) {
                    owner_func_name = cn_sem_scope_get_name(decl_scope, &owner_func_name_length);
                }
                
                sym->type = cn_type_new_struct(struct_decl->name,
                                              struct_decl->name_length,
                                              fields,
                                              struct_decl->field_count,
                                              decl_scope,
                                              owner_func_name,
                                              owner_func_name_length);
            }
        }
        // 如果是枚举定义，需要先注册到作用域
        else if (stmt && stmt->kind == CN_AST_STMT_ENUM_DECL) {
            CnAstEnumDecl *enum_decl = &stmt->as.enum_decl;
            CnSemSymbol *sym = cn_sem_scope_insert_symbol(block_scope,
                                       enum_decl->name,
                                       enum_decl->name_length,
                                       CN_SEM_SYMBOL_ENUM);
            if (sym) {
                sym->type = cn_type_new_enum(enum_decl->name, enum_decl->name_length);
                CnSemScope *enum_scope = cn_sem_scope_new(CN_SEM_SCOPE_ENUM, block_scope);
                if (enum_scope && sym->type) {
                    sym->type->as.enum_type.enum_scope = enum_scope;
                    for (size_t j = 0; j < enum_decl->member_count; j++) {
                        CnAstEnumMember *member = &enum_decl->members[j];
                        CnSemSymbol *member_sym = cn_sem_scope_insert_symbol(enum_scope,
                                                           member->name,
                                                           member->name_length,
                                                           CN_SEM_SYMBOL_ENUM_MEMBER);
                        if (member_sym) {
                            member_sym->type = cn_type_new_primitive(CN_TYPE_INT);
                            member_sym->as.enum_value = member->value;
                        }
                    }
                }
            }
        }
        
        // 查找return语句
        CnType *ret_type = infer_return_from_stmt(block_scope, stmt, diagnostics);
        if (ret_type) {
            cn_sem_scope_free(block_scope);
            return ret_type;
        }
    }
    
    cn_sem_scope_free(block_scope);
    // 如果没有找到return语句,返回void类型(默认)
    return cn_type_new_primitive(CN_TYPE_VOID);
}
