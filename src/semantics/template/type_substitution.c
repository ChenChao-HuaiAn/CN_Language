/**
 * @file type_substitution.c
 * @brief 类型替换实现
 *
 * 实现模板参数到具体类型的替换算法，支持类型、表达式和语句的类型替换。
 */

#include "cnlang/semantics/template.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * 类型替换实现
 * ============================================================================ */

/**
 * @brief 检查类型名称是否为模板参数名
 *
 * 约定：单字母或双字母大写名称为模板参数（如 T, K, V, T1）
 */
static bool is_template_param_name(const char *name, size_t len) {
    if (!name || len == 0) return false;
    
    // 单字母大写
    if (len == 1 && name[0] >= 'A' && name[0] <= 'Z') {
        return true;
    }
    
    // 双字母大写（如 T1, T2）
    if (len == 2 && name[0] >= 'A' && name[0] <= 'Z') {
        // 第二个字符可以是数字或大写字母
        return (name[1] >= 'A' && name[1] <= 'Z') ||
               (name[1] >= '0' && name[1] <= '9');
    }
    
    return false;
}

/**
 * @brief 替换类型中的模板参数
 *
 * 递归处理复合类型（指针、数组、函数等）
 */
CnType *cn_type_substitute(CnType *type, const CnTypeMap *type_map) {
    if (!type || !type_map) return type;
    
    // 检查是否是模板参数类型
    // 模板参数在类型系统中用结构体类型表示，名称为参数名
    if (type->kind == CN_TYPE_STRUCT && type->as.struct_type.name) {
        const char *name = type->as.struct_type.name;
        size_t len = type->as.struct_type.name_length;
        
        // 检查是否是模板参数名
        if (is_template_param_name(name, len)) {
            // 在映射表中查找替换类型
            CnType *replacement = cn_type_map_lookup(type_map, name, len);
            if (replacement) {
                return replacement;
            }
        }
    }
    
    // 处理指针类型
    if (type->kind == CN_TYPE_POINTER) {
        CnType *base_type = cn_type_substitute(type->as.pointer_to, type_map);
        if (base_type != type->as.pointer_to) {
            // 创建新的指针类型
            return cn_type_new_pointer(base_type);
        }
        return type;
    }
    
    // 处理数组类型
    if (type->kind == CN_TYPE_ARRAY) {
        CnType *elem_type = cn_type_substitute(type->as.array.element_type, type_map);
        if (elem_type != type->as.array.element_type) {
            // 创建新的数组类型
            return cn_type_new_array(elem_type, type->as.array.length);
        }
        return type;
    }
    
    // 处理函数类型
    if (type->kind == CN_TYPE_FUNCTION) {
        bool changed = false;
        
        // 替换返回类型
        CnType *ret_type = cn_type_substitute(type->as.function.return_type, type_map);
        if (ret_type != type->as.function.return_type) {
            changed = true;
        }
        
        // 替换参数类型
        CnType **param_types = NULL;
        if (type->as.function.param_count > 0) {
            param_types = (CnType **)malloc(
                type->as.function.param_count * sizeof(CnType *));
            if (!param_types) return type;
            
            for (size_t i = 0; i < type->as.function.param_count; i++) {
                param_types[i] = cn_type_substitute(
                    type->as.function.param_types[i], type_map);
                if (param_types[i] != type->as.function.param_types[i]) {
                    changed = true;
                }
            }
        }
        
        if (changed) {
            return cn_type_new_function(ret_type, param_types, 
                                        type->as.function.param_count);
        }
        
        if (param_types) free(param_types);
        return type;
    }
    
    // 其他类型（基本类型、枚举等）不需要替换
    return type;
}

/* ============================================================================
 * 表达式类型替换实现
 * ============================================================================ */

/**
 * @brief 替换二元表达式中的类型
 */
static bool cn_type_substitute_binary_expr(CnAstBinaryExpr *expr, 
                                           const CnTypeMap *type_map) {
    if (!expr) return true;
    
    // 替换表达式类型
    if (expr->left) {
        cn_type_substitute_expr(expr->left, type_map);
    }
    if (expr->right) {
        cn_type_substitute_expr(expr->right, type_map);
    }
    
    return true;
}

/**
 * @brief 替换调用表达式中的类型
 */
static bool cn_type_substitute_call_expr(CnAstCallExpr *expr,
                                         const CnTypeMap *type_map) {
    if (!expr) return true;
    
    // 替换被调用表达式
    if (expr->callee) {
        cn_type_substitute_expr(expr->callee, type_map);
    }
    
    // 替换参数表达式
    for (size_t i = 0; i < expr->argument_count; i++) {
        if (expr->arguments[i]) {
            cn_type_substitute_expr(expr->arguments[i], type_map);
        }
    }
    
    return true;
}

/**
 * @brief 替换一元表达式中的类型
 */
static bool cn_type_substitute_unary_expr(CnAstUnaryExpr *expr,
                                          const CnTypeMap *type_map) {
    if (!expr) return true;
    
    if (expr->operand) {
        cn_type_substitute_expr(expr->operand, type_map);
    }
    
    return true;
}

/**
 * @brief 替换逻辑表达式中的类型
 */
static bool cn_type_substitute_logical_expr(CnAstLogicalExpr *expr,
                                            const CnTypeMap *type_map) {
    if (!expr) return true;
    
    if (expr->left) {
        cn_type_substitute_expr(expr->left, type_map);
    }
    if (expr->right) {
        cn_type_substitute_expr(expr->right, type_map);
    }
    
    return true;
}

/**
 * @brief 替换赋值表达式中的类型
 */
static bool cn_type_substitute_assign_expr(CnAstAssignExpr *expr,
                                           const CnTypeMap *type_map) {
    if (!expr) return true;
    
    if (expr->target) {
        cn_type_substitute_expr(expr->target, type_map);
    }
    if (expr->value) {
        cn_type_substitute_expr(expr->value, type_map);
    }
    
    return true;
}

/**
 * @brief 替换三元表达式中的类型
 */
static bool cn_type_substitute_ternary_expr(CnAstTernaryExpr *expr,
                                            const CnTypeMap *type_map) {
    if (!expr) return true;
    
    if (expr->condition) {
        cn_type_substitute_expr(expr->condition, type_map);
    }
    if (expr->true_expr) {
        cn_type_substitute_expr(expr->true_expr, type_map);
    }
    if (expr->false_expr) {
        cn_type_substitute_expr(expr->false_expr, type_map);
    }
    
    return true;
}

/**
 * @brief 替换数组索引表达式中的类型
 */
static bool cn_type_substitute_index_expr(CnAstIndexExpr *expr,
                                          const CnTypeMap *type_map) {
    if (!expr) return true;
    
    if (expr->array) {
        cn_type_substitute_expr(expr->array, type_map);
    }
    if (expr->index) {
        cn_type_substitute_expr(expr->index, type_map);
    }
    
    return true;
}

/**
 * @brief 替换成员访问表达式中的类型
 */
static bool cn_type_substitute_member_expr(CnAstMemberAccessExpr *expr,
                                           const CnTypeMap *type_map) {
    if (!expr) return true;
    
    if (expr->object) {
        cn_type_substitute_expr(expr->object, type_map);
    }
    
    return true;
}

/**
 * @brief 替换数组字面量中的类型
 */
static bool cn_type_substitute_array_literal_expr(CnAstArrayLiteralExpr *expr,
                                                  const CnTypeMap *type_map) {
    if (!expr) return true;
    
    for (size_t i = 0; i < expr->element_count; i++) {
        if (expr->elements[i]) {
            cn_type_substitute_expr(expr->elements[i], type_map);
        }
    }
    
    return true;
}

/**
 * @brief 替换结构体字面量中的类型
 */
static bool cn_type_substitute_struct_literal_expr(CnAstStructLiteralExpr *expr,
                                                   const CnTypeMap *type_map) {
    if (!expr) return true;
    
    for (size_t i = 0; i < expr->field_count; i++) {
        if (expr->fields[i].value) {
            cn_type_substitute_expr(expr->fields[i].value, type_map);
        }
    }
    
    return true;
}

/**
 * @brief 替换表达式中的类型引用
 */
bool cn_type_substitute_expr(CnAstExpr *expr, const CnTypeMap *type_map) {
    if (!expr || !type_map) return true;
    
    // 替换表达式本身的类型
    if (expr->type) {
        expr->type = cn_type_substitute(expr->type, type_map);
    }
    
    // 根据表达式类型递归处理
    switch (expr->kind) {
        case CN_AST_EXPR_BINARY:
            return cn_type_substitute_binary_expr(&expr->as.binary, type_map);
            
        case CN_AST_EXPR_CALL:
            return cn_type_substitute_call_expr(&expr->as.call, type_map);
            
        case CN_AST_EXPR_UNARY:
            return cn_type_substitute_unary_expr(&expr->as.unary, type_map);
            
        case CN_AST_EXPR_LOGICAL:
            return cn_type_substitute_logical_expr(&expr->as.logical, type_map);
            
        case CN_AST_EXPR_ASSIGN:
            return cn_type_substitute_assign_expr(&expr->as.assign, type_map);
            
        case CN_AST_EXPR_TERNARY:
            return cn_type_substitute_ternary_expr(&expr->as.ternary, type_map);
            
        case CN_AST_EXPR_INDEX:
            return cn_type_substitute_index_expr(&expr->as.index, type_map);
            
        case CN_AST_EXPR_MEMBER_ACCESS:
            return cn_type_substitute_member_expr(&expr->as.member, type_map);
            
        case CN_AST_EXPR_ARRAY_LITERAL:
            return cn_type_substitute_array_literal_expr(&expr->as.array_literal, type_map);
            
        case CN_AST_EXPR_STRUCT_LITERAL:
            return cn_type_substitute_struct_literal_expr(&expr->as.struct_lit, type_map);
            
        // 简单表达式不需要递归处理
        case CN_AST_EXPR_IDENTIFIER:
        case CN_AST_EXPR_INTEGER_LITERAL:
        case CN_AST_EXPR_FLOAT_LITERAL:
        case CN_AST_EXPR_STRING_LITERAL:
        case CN_AST_EXPR_BOOL_LITERAL:
            return true;
            
        // 模板实例化表达式需要特殊处理
        case CN_AST_EXPR_TEMPLATE_INSTANTIATION:
            // 模板实例化表达式的类型参数在解析时已确定
            // 这里只替换结果类型
            return true;
            
        default:
            return true;
    }
}

/* ============================================================================
 * 语句类型替换实现
 * ============================================================================ */

/**
 * @brief 替换变量声明中的类型
 */
static bool cn_type_substitute_var_decl(CnAstVarDecl *decl,
                                        const CnTypeMap *type_map) {
    if (!decl) return true;
    
    // 替换声明类型
    if (decl->declared_type) {
        decl->declared_type = cn_type_substitute(decl->declared_type, type_map);
    }
    
    // 替换初始化表达式
    if (decl->initializer) {
        cn_type_substitute_expr(decl->initializer, type_map);
    }
    
    return true;
}

/**
 * @brief 替换返回语句中的类型
 */
static bool cn_type_substitute_return_stmt(CnAstReturnStmt *stmt,
                                           const CnTypeMap *type_map) {
    if (!stmt) return true;
    
    if (stmt->expr) {
        cn_type_substitute_expr(stmt->expr, type_map);
    }
    
    return true;
}

/**
 * @brief 替换条件语句中的类型
 */
static bool cn_type_substitute_if_stmt(CnAstIfStmt *stmt,
                                       const CnTypeMap *type_map) {
    if (!stmt) return true;
    
    // 替换条件表达式
    if (stmt->condition) {
        cn_type_substitute_expr(stmt->condition, type_map);
    }
    
    // 替换 then 块
    if (stmt->then_block) {
        cn_type_substitute_block(stmt->then_block, type_map);
    }
    
    // 替换 else 块
    if (stmt->else_block) {
        cn_type_substitute_block(stmt->else_block, type_map);
    }
    
    return true;
}

/**
 * @brief 替换 while 循环语句中的类型
 */
static bool cn_type_substitute_while_stmt(CnAstWhileStmt *stmt,
                                          const CnTypeMap *type_map) {
    if (!stmt) return true;
    
    if (stmt->condition) {
        cn_type_substitute_expr(stmt->condition, type_map);
    }
    
    if (stmt->body) {
        cn_type_substitute_block(stmt->body, type_map);
    }
    
    return true;
}

/**
 * @brief 替换 for 循环语句中的类型
 */
static bool cn_type_substitute_for_stmt(CnAstForStmt *stmt,
                                        const CnTypeMap *type_map) {
    if (!stmt) return true;
    
    // 替换初始化语句
    if (stmt->init) {
        cn_type_substitute_stmt(stmt->init, type_map);
    }
    
    // 替换条件表达式
    if (stmt->condition) {
        cn_type_substitute_expr(stmt->condition, type_map);
    }
    
    // 替换更新表达式
    if (stmt->update) {
        cn_type_substitute_expr(stmt->update, type_map);
    }
    
    // 替换循环体
    if (stmt->body) {
        cn_type_substitute_block(stmt->body, type_map);
    }
    
    return true;
}

/**
 * @brief 替换 switch 语句中的类型
 */
static bool cn_type_substitute_switch_stmt(CnAstSwitchStmt *stmt,
                                           const CnTypeMap *type_map) {
    if (!stmt) return true;
    
    // 替换判断表达式
    if (stmt->expr) {
        cn_type_substitute_expr(stmt->expr, type_map);
    }
    
    // 替换各个 case 分支
    for (size_t i = 0; i < stmt->case_count; i++) {
        CnAstSwitchCase *case_stmt = &stmt->cases[i];
        if (case_stmt->value) {
            cn_type_substitute_expr(case_stmt->value, type_map);
        }
        if (case_stmt->body) {
            cn_type_substitute_block(case_stmt->body, type_map);
        }
    }
    
    return true;
}

/**
 * @brief 替换表达式语句中的类型
 */
static bool cn_type_substitute_expr_stmt(CnAstExprStmt *stmt,
                                         const CnTypeMap *type_map) {
    if (!stmt) return true;
    
    if (stmt->expr) {
        cn_type_substitute_expr(stmt->expr, type_map);
    }
    
    return true;
}

/**
 * @brief 替换语句中的类型引用
 */
bool cn_type_substitute_stmt(CnAstStmt *stmt, const CnTypeMap *type_map) {
    if (!stmt || !type_map) return true;
    
    switch (stmt->kind) {
        case CN_AST_STMT_VAR_DECL:
            return cn_type_substitute_var_decl(&stmt->as.var_decl, type_map);
            
        case CN_AST_STMT_RETURN:
            return cn_type_substitute_return_stmt(&stmt->as.return_stmt, type_map);
            
        case CN_AST_STMT_IF:
            return cn_type_substitute_if_stmt(&stmt->as.if_stmt, type_map);
            
        case CN_AST_STMT_WHILE:
            return cn_type_substitute_while_stmt(&stmt->as.while_stmt, type_map);
            
        case CN_AST_STMT_FOR:
            return cn_type_substitute_for_stmt(&stmt->as.for_stmt, type_map);
            
        case CN_AST_STMT_SWITCH:
            return cn_type_substitute_switch_stmt(&stmt->as.switch_stmt, type_map);
            
        case CN_AST_STMT_EXPR:
            return cn_type_substitute_expr_stmt(&stmt->as.expr, type_map);
            
        case CN_AST_STMT_BLOCK:
            return cn_type_substitute_block(stmt->as.block, type_map);
            
        // break 和 continue 不需要类型替换
        case CN_AST_STMT_BREAK:
        case CN_AST_STMT_CONTINUE:
            return true;
            
        // 结构体和枚举声明在顶层处理
        case CN_AST_STMT_STRUCT_DECL:
        case CN_AST_STMT_ENUM_DECL:
            return true;
            
        // 导入语句不需要类型替换
        case CN_AST_STMT_IMPORT:
            return true;
            
        // 模板声明本身不替换（在实例化时处理）
        case CN_AST_STMT_TEMPLATE_FUNCTION_DECL:
        case CN_AST_STMT_TEMPLATE_STRUCT_DECL:
            return true;
            
        default:
            return true;
    }
}

/**
 * @brief 替换语句块中的类型引用
 */
bool cn_type_substitute_block(CnAstBlockStmt *block, const CnTypeMap *type_map) {
    if (!block || !type_map) return true;
    
    for (size_t i = 0; i < block->stmt_count; i++) {
        if (block->stmts[i]) {
            cn_type_substitute_stmt(block->stmts[i], type_map);
        }
    }
    
    return true;
}
