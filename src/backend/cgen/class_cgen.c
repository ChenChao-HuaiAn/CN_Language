/**
 * @file class_cgen.c
 * @brief CN语言面向对象编程 - 类代码生成实现
 *
 * 本文件实现了CN语言类和接口的C代码生成功能，将类AST节点转换为C代码。
 */

#include "cnlang/backend/cgen/class_cgen.h"
#include "cnlang/backend/cgen.h"
#include "cnlang/frontend/ast/class_node.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/frontend/token.h"
#include "cnlang/semantics/vtable_builder.h"
#include "cnlang/semantics/class_analyzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 获取C类型字符串（从CnType）
 *
 * 注意：此函数声明在cgen.c中，这里通过extern引用
 */
extern const char *get_c_type_string(CnType *type);

/**
 * @brief 生成初始化表达式的C代码（简化版）
 *
 * 前向声明，实现在后面
 */
static void cgen_init_expr_simple(FILE *out, CnAstExpr *expr);

/**
 * @brief 输出缩进空格
 */
static void print_indent(FILE *out, int indent) {
    for (int i = 0; i < indent; i++) {
        fprintf(out, "    ");
    }
}

/**
 * @brief 检查类是否有虚函数
 */
static bool class_has_virtual_methods(CnAstClassDecl *class_decl) {
    if (!class_decl) return false;
    
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD && member->is_virtual) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 检查类是否有虚函数或继承自含有虚函数的基类
 */
static bool class_needs_vtable(CnAstClassDecl *class_decl) {
    // 使用vtable_builder模块的函数
    return cn_vtable_needs_vtable(class_decl);
}

/**
 * @brief 生成成员变量的C类型声明
 *
 * 注意：静态成员变量不放在结构体中，单独生成全局变量
 */
static void cgen_member_field(FILE *out, CnClassMember *member, int indent) {
    if (!member || member->kind != CN_MEMBER_FIELD) return;
    
    // 静态成员变量不放在结构体中
    if (member->is_static) {
        return;
    }
    
    print_indent(out, indent);
    
    // 生成类型和名称
    const char *type_str = "int";  // 默认类型
    if (member->type) {
        type_str = get_c_type_string(member->type);
    }
    
    fprintf(out, "%s %.*s", type_str,
            (int)member->name_length, member->name);
    
    // 如果是常量且有初始值，可以在这里处理
    // 但C结构体不支持直接初始化，需要构造函数处理
    
    fprintf(out, ";\n");
}

/**
 * @brief 生成静态成员变量的全局变量声明
 *
 * 静态成员变量生成为全局变量，命名格式：类名_静态成员名
 * 例如：静态 整数 实例数; -> static int Counter_实例数;
 */
static void cgen_static_field_decl(FILE *out, CnAstClassDecl *class_decl,
                                    CnClassMember *member) {
    if (!out || !member || member->kind != CN_MEMBER_FIELD || !member->is_static) return;
    
    // 生成类型
    const char *type_str = "int";  // 默认类型
    if (member->type) {
        type_str = get_c_type_string(member->type);
    }
    
    // 静态成员变量命名：类名_成员名
    fprintf(out, "static %s %.*s_%.*s",
            type_str,
            (int)class_decl->name_length, class_decl->name,
            (int)member->name_length, member->name);
    
    // 如果有初始值，生成初始化
    if (member->init_expr) {
        fprintf(out, " = ");
        cgen_init_expr_simple(out, member->init_expr);
    }
    
    fprintf(out, ";\n");
}

/**
 * @brief 生成所有静态成员变量的声明
 */
static void cgen_static_fields(FILE *out, CnAstClassDecl *class_decl) {
    if (!out || !class_decl) return;
    
    bool has_static = false;
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_FIELD && member->is_static) {
            has_static = true;
            break;
        }
    }
    
    if (!has_static) return;
    
    fprintf(out, "/* 静态成员变量 */\n");
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_FIELD && member->is_static) {
            cgen_static_field_decl(out, class_decl, member);
        }
    }
    fprintf(out, "\n");
}

/**
 * @brief 生成构造函数的前向声明
 */
static void cgen_constructor_forward_decl(FILE *out, CnAstClassDecl *class_decl,
                                           CnClassMember *constructor) {
    if (!constructor || constructor->kind != CN_MEMBER_CONSTRUCTOR) return;
    
    // 构造函数返回类指针类型
    fprintf(out, "%.*s* %.*s_construct(",
            (int)class_decl->name_length, class_decl->name,
            (int)class_decl->name_length, class_decl->name);
    
    // 第一个参数是 self 指针（构造函数需要返回新对象）
    fprintf(out, "%.*s* self",
            (int)class_decl->name_length, class_decl->name);
    
    // 其他参数
    for (size_t i = 0; i < constructor->parameter_count; i++) {
        CnAstParameter *param = &constructor->parameters[i];
        const char *param_type = "int";
        if (param->declared_type) {
            param_type = get_c_type_string(param->declared_type);
        }
        fprintf(out, ", %s %.*s", param_type,
                (int)param->name_length, param->name);
    }
    
    fprintf(out, ");\n");
}

/**
 * @brief 生成析构函数的前向声明
 */
static void cgen_destructor_forward_decl(FILE *out, CnAstClassDecl *class_decl,
                                          CnClassMember *destructor) {
    if (!destructor || destructor->kind != CN_MEMBER_DESTRUCTOR) return;
    
    // 析构函数返回void，参数只有self指针
    fprintf(out, "void %.*s_destruct(%.*s* self);\n",
            (int)class_decl->name_length, class_decl->name,
            (int)class_decl->name_length, class_decl->name);
}

/**
 * @brief 生成成员函数的前向声明
 *
 * 静态方法不需要 self 指针参数
 */
static void cgen_method_forward_decl(FILE *out, CnAstClassDecl *class_decl,
                                       CnClassMember *method) {
    if (!method || method->kind != CN_MEMBER_METHOD) return;
    
    // 返回类型
    const char *return_type = "void";
    if (method->type) {
        return_type = get_c_type_string(method->type);
    }
    
    // 函数名：类名_方法名
    fprintf(out, "%s %.*s_%.*s(",
            return_type,
            (int)class_decl->name_length, class_decl->name,
            (int)method->name_length, method->name);
    
    // 静态方法不需要 self 指针参数
    if (!method->is_static) {
        // 参数列表：第一个参数是 self 指针
        fprintf(out, "struct %.*s* self",
                (int)class_decl->name_length, class_decl->name);
    }
    
    // 其他参数
    for (size_t i = 0; i < method->parameter_count; i++) {
        CnAstParameter *param = &method->parameters[i];
        const char *param_type = "int";
        if (param->declared_type) {
            param_type = get_c_type_string(param->declared_type);
        }
        // 非静态方法需要逗号分隔，静态方法第一个参数不需要逗号
        if (!method->is_static || i > 0) {
            fprintf(out, ", ");
        }
        fprintf(out, "%s %.*s", param_type,
                (int)param->name_length, param->name);
    }
    
    // 静态方法如果没有参数，需要添加 void
    if (method->is_static && method->parameter_count == 0) {
        fprintf(out, "void");
    }
    
    fprintf(out, ");\n");
}

/**
 * @brief 生成初始化表达式的C代码（简化版）
 *
 * @param out 输出文件
 * @param expr 表达式节点
 */
static void cgen_init_expr_simple(FILE *out, CnAstExpr *expr) {
    if (!out || !expr) {
        fprintf(out, "0");
        return;
    }
    
    switch (expr->kind) {
        case CN_AST_EXPR_INTEGER_LITERAL:
            fprintf(out, "%lld", expr->as.integer_literal.value);
            break;
        case CN_AST_EXPR_FLOAT_LITERAL:
            fprintf(out, "%f", expr->as.float_literal.value);
            break;
        case CN_AST_EXPR_STRING_LITERAL:
            fprintf(out, "\"");
            for (const char *p = expr->as.string_literal.value; p && *p; p++) {
                switch (*p) {
                    case '\n': fprintf(out, "\\n"); break;
                    case '\t': fprintf(out, "\\t"); break;
                    case '\r': fprintf(out, "\\r"); break;
                    case '\\': fprintf(out, "\\\\"); break;
                    case '"': fprintf(out, "\\\""); break;
                    default: fprintf(out, "%c", *p); break;
                }
            }
            fprintf(out, "\"");
            break;
        case CN_AST_EXPR_BOOL_LITERAL:
            fprintf(out, "%s", expr->as.bool_literal.value ? "1" : "0");
            break;
        case CN_AST_EXPR_IDENTIFIER:
            // 标识符引用（参数或变量）
            fprintf(out, "%.*s", (int)expr->as.identifier.name_length, expr->as.identifier.name);
            break;
        case CN_AST_EXPR_BINARY:
            // 二元表达式
            fprintf(out, "(");
            cgen_init_expr_simple(out, expr->as.binary.left);
            switch (expr->as.binary.op) {
                case CN_AST_BINARY_OP_ADD: fprintf(out, " + "); break;
                case CN_AST_BINARY_OP_SUB: fprintf(out, " - "); break;
                case CN_AST_BINARY_OP_MUL: fprintf(out, " * "); break;
                case CN_AST_BINARY_OP_DIV: fprintf(out, " / "); break;
                case CN_AST_BINARY_OP_MOD: fprintf(out, " %% "); break;
                case CN_AST_BINARY_OP_EQ: fprintf(out, " == "); break;
                case CN_AST_BINARY_OP_NE: fprintf(out, " != "); break;
                case CN_AST_BINARY_OP_LT: fprintf(out, " < "); break;
                case CN_AST_BINARY_OP_LE: fprintf(out, " <= "); break;
                case CN_AST_BINARY_OP_GT: fprintf(out, " > "); break;
                case CN_AST_BINARY_OP_GE: fprintf(out, " >= "); break;
                case CN_AST_BINARY_OP_BITWISE_AND: fprintf(out, " & "); break;
                case CN_AST_BINARY_OP_BITWISE_OR: fprintf(out, " | "); break;
                case CN_AST_BINARY_OP_BITWISE_XOR: fprintf(out, " ^ "); break;
                case CN_AST_BINARY_OP_LEFT_SHIFT: fprintf(out, " << "); break;
                case CN_AST_BINARY_OP_RIGHT_SHIFT: fprintf(out, " >> "); break;
                default: fprintf(out, " ? "); break;
            }
            cgen_init_expr_simple(out, expr->as.binary.right);
            fprintf(out, ")");
            break;
        default:
            fprintf(out, "0");
            break;
    }
}

/**
 * @brief 生成语句的C代码（用于构造函数/析构函数/方法体）
 *
 * @param ctx 代码生成上下文
 * @param stmt 语句节点
 * @param indent 缩进级别
 */
static void cgen_stmt_in_method(CnCCodeGenContext *ctx, CnAstStmt *stmt, int indent);

/**
 * @brief 生成语句块的C代码
 *
 * @param ctx 代码生成上下文
 * @param block 语句块节点
 * @param indent 缩进级别
 */
static void cgen_block_in_method(CnCCodeGenContext *ctx, CnAstBlockStmt *block, int indent) {
    if (!ctx || !ctx->output_file || !block) return;
    
    FILE *out = ctx->output_file;
    
    for (size_t i = 0; i < block->stmt_count; i++) {
        cgen_stmt_in_method(ctx, block->stmts[i], indent);
    }
}

/**
 * @brief 生成表达式的C代码（用于方法体内）
 *
 * @param ctx 代码生成上下文
 * @param expr 表达式节点
 */
static void cgen_expr_in_method(CnCCodeGenContext *ctx, CnAstExpr *expr) {
    if (!ctx || !ctx->output_file || !expr) {
        fprintf(ctx->output_file, "0");
        return;
    }
    
    FILE *out = ctx->output_file;
    
    switch (expr->kind) {
        case CN_AST_EXPR_INTEGER_LITERAL:
            fprintf(out, "%lld", expr->as.integer_literal.value);
            break;
        case CN_AST_EXPR_FLOAT_LITERAL:
            fprintf(out, "%f", expr->as.float_literal.value);
            break;
        case CN_AST_EXPR_STRING_LITERAL:
            fprintf(out, "\"");
            for (const char *p = expr->as.string_literal.value; p && *p; p++) {
                switch (*p) {
                    case '\n': fprintf(out, "\\n"); break;
                    case '\t': fprintf(out, "\\t"); break;
                    case '\r': fprintf(out, "\\r"); break;
                    case '\\': fprintf(out, "\\\\"); break;
                    case '"': fprintf(out, "\\\""); break;
                    default: fprintf(out, "%c", *p); break;
                }
            }
            fprintf(out, "\"");
            break;
        case CN_AST_EXPR_BOOL_LITERAL:
            fprintf(out, "%s", expr->as.bool_literal.value ? "1" : "0");
            break;
        case CN_AST_EXPR_IDENTIFIER:
            // 检查是否为自身指针
            if (expr->is_this_pointer) {
                fprintf(out, "self");
            } else {
                fprintf(out, "%.*s", (int)expr->as.identifier.name_length, expr->as.identifier.name);
            }
            break;
        case CN_AST_EXPR_MEMBER_ACCESS:
            // 成员访问：obj.member 或 ptr->member
            if (expr->as.member.object && expr->as.member.object->is_this_pointer) {
                // self->member
                fprintf(out, "self->%.*s",
                        (int)expr->as.member.member_name_length, expr->as.member.member_name);
            } else {
                cgen_expr_in_method(ctx, expr->as.member.object);
                if (expr->as.member.is_arrow) {
                    fprintf(out, "->");
                } else {
                    fprintf(out, ".");
                }
                fprintf(out, "%.*s",
                        (int)expr->as.member.member_name_length, expr->as.member.member_name);
            }
            break;
        case CN_AST_EXPR_BINARY:
            fprintf(out, "(");
            cgen_expr_in_method(ctx, expr->as.binary.left);
            switch (expr->as.binary.op) {
                case CN_AST_BINARY_OP_ADD: fprintf(out, " + "); break;
                case CN_AST_BINARY_OP_SUB: fprintf(out, " - "); break;
                case CN_AST_BINARY_OP_MUL: fprintf(out, " * "); break;
                case CN_AST_BINARY_OP_DIV: fprintf(out, " / "); break;
                case CN_AST_BINARY_OP_MOD: fprintf(out, " %% "); break;
                case CN_AST_BINARY_OP_EQ: fprintf(out, " == "); break;
                case CN_AST_BINARY_OP_NE: fprintf(out, " != "); break;
                case CN_AST_BINARY_OP_LT: fprintf(out, " < "); break;
                case CN_AST_BINARY_OP_LE: fprintf(out, " <= "); break;
                case CN_AST_BINARY_OP_GT: fprintf(out, " > "); break;
                case CN_AST_BINARY_OP_GE: fprintf(out, " >= "); break;
                case CN_AST_BINARY_OP_BITWISE_AND: fprintf(out, " & "); break;
                case CN_AST_BINARY_OP_BITWISE_OR: fprintf(out, " | "); break;
                case CN_AST_BINARY_OP_BITWISE_XOR: fprintf(out, " ^ "); break;
                case CN_AST_BINARY_OP_LEFT_SHIFT: fprintf(out, " << "); break;
                case CN_AST_BINARY_OP_RIGHT_SHIFT: fprintf(out, " >> "); break;
                default: fprintf(out, " ? "); break;
            }
            cgen_expr_in_method(ctx, expr->as.binary.right);
            fprintf(out, ")");
            break;
        case CN_AST_EXPR_LOGICAL:
            // 逻辑表达式
            fprintf(out, "(");
            cgen_expr_in_method(ctx, expr->as.logical.left);
            switch (expr->as.logical.op) {
                case CN_AST_LOGICAL_OP_AND: fprintf(out, " && "); break;
                case CN_AST_LOGICAL_OP_OR: fprintf(out, " || "); break;
                default: fprintf(out, " ? "); break;
            }
            cgen_expr_in_method(ctx, expr->as.logical.right);
            fprintf(out, ")");
            break;
        case CN_AST_EXPR_UNARY:
            switch (expr->as.unary.op) {
                case CN_AST_UNARY_OP_MINUS: fprintf(out, "-"); break;
                case CN_AST_UNARY_OP_NOT: fprintf(out, "!"); break;
                case CN_AST_UNARY_OP_BITWISE_NOT: fprintf(out, "~"); break;
                case CN_AST_UNARY_OP_PRE_INC: fprintf(out, "++"); break;
                case CN_AST_UNARY_OP_PRE_DEC: fprintf(out, "--"); break;
                case CN_AST_UNARY_OP_POST_INC:
                    cgen_expr_in_method(ctx, expr->as.unary.operand);
                    fprintf(out, "++");
                    return;
                case CN_AST_UNARY_OP_POST_DEC:
                    cgen_expr_in_method(ctx, expr->as.unary.operand);
                    fprintf(out, "--");
                    return;
                default: break;
            }
            cgen_expr_in_method(ctx, expr->as.unary.operand);
            break;
        case CN_AST_EXPR_CALL:
            // 函数调用
            cgen_expr_in_method(ctx, expr->as.call.callee);
            fprintf(out, "(");
            for (size_t i = 0; i < expr->as.call.argument_count; i++) {
                if (i > 0) fprintf(out, ", ");
                cgen_expr_in_method(ctx, expr->as.call.arguments[i]);
            }
            fprintf(out, ")");
            break;
        case CN_AST_EXPR_ASSIGN:
            // 赋值表达式
            cgen_expr_in_method(ctx, expr->as.assign.target);
            fprintf(out, " = ");
            cgen_expr_in_method(ctx, expr->as.assign.value);
            break;
        case CN_AST_EXPR_TERNARY:
            fprintf(out, "(");
            cgen_expr_in_method(ctx, expr->as.ternary.condition);
            fprintf(out, " ? ");
            cgen_expr_in_method(ctx, expr->as.ternary.true_expr);
            fprintf(out, " : ");
            cgen_expr_in_method(ctx, expr->as.ternary.false_expr);
            fprintf(out, ")");
            break;
        default:
            fprintf(out, "0");
            break;
    }
}

/**
 * @brief 生成语句的C代码（用于构造函数/析构函数/方法体）
 *
 * @param ctx 代码生成上下文
 * @param stmt 语句节点
 * @param indent 缩进级别
 */
static void cgen_stmt_in_method(CnCCodeGenContext *ctx, CnAstStmt *stmt, int indent) {
    if (!ctx || !ctx->output_file || !stmt) return;
    
    FILE *out = ctx->output_file;
    
    // 输出缩进
    print_indent(out, indent);
    
    switch (stmt->kind) {
        case CN_AST_STMT_VAR_DECL: {
            // 变量声明
            CnAstVarDecl *var_decl = &stmt->as.var_decl;
            const char *type_str = "int";
            if (var_decl->declared_type) {
                type_str = get_c_type_string(var_decl->declared_type);
            }
            fprintf(out, "%s %.*s", type_str,
                    (int)var_decl->name_length, var_decl->name);
            if (var_decl->initializer) {
                fprintf(out, " = ");
                cgen_expr_in_method(ctx, var_decl->initializer);
            }
            fprintf(out, ";\n");
            break;
        }
        case CN_AST_STMT_EXPR: {
            // 表达式语句
            cgen_expr_in_method(ctx, stmt->as.expr.expr);
            fprintf(out, ";\n");
            break;
        }
        case CN_AST_STMT_RETURN: {
            // 返回语句
            CnAstReturnStmt *return_stmt = &stmt->as.return_stmt;
            fprintf(out, "return");
            if (return_stmt->expr) {
                fprintf(out, " ");
                cgen_expr_in_method(ctx, return_stmt->expr);
            }
            fprintf(out, ";\n");
            break;
        }
        case CN_AST_STMT_IF: {
            // 条件语句
            CnAstIfStmt *if_stmt = &stmt->as.if_stmt;
            fprintf(out, "if (");
            cgen_expr_in_method(ctx, if_stmt->condition);
            fprintf(out, ") {\n");
            if (if_stmt->then_block) {
                cgen_block_in_method(ctx, if_stmt->then_block, indent + 1);
            }
            print_indent(out, indent);
            fprintf(out, "}");
            if (if_stmt->else_block) {
                fprintf(out, " else {\n");
                cgen_block_in_method(ctx, if_stmt->else_block, indent + 1);
                print_indent(out, indent);
                fprintf(out, "}");
            }
            fprintf(out, "\n");
            break;
        }
        case CN_AST_STMT_WHILE: {
            // while循环
            CnAstWhileStmt *while_stmt = &stmt->as.while_stmt;
            fprintf(out, "while (");
            cgen_expr_in_method(ctx, while_stmt->condition);
            fprintf(out, ") {\n");
            if (while_stmt->body) {
                cgen_block_in_method(ctx, while_stmt->body, indent + 1);
            }
            print_indent(out, indent);
            fprintf(out, "}\n");
            break;
        }
        case CN_AST_STMT_FOR: {
            // for循环
            CnAstForStmt *for_stmt = &stmt->as.for_stmt;
            fprintf(out, "for (");
            if (for_stmt->init) {
                // 简化处理：只支持变量声明初始化
                if (for_stmt->init->kind == CN_AST_STMT_VAR_DECL) {
                    CnAstVarDecl *var_decl = &for_stmt->init->as.var_decl;
                    const char *type_str = "int";
                    if (var_decl->declared_type) {
                        type_str = get_c_type_string(var_decl->declared_type);
                    }
                    fprintf(out, "%s %.*s", type_str,
                            (int)var_decl->name_length, var_decl->name);
                    if (var_decl->initializer) {
                        fprintf(out, " = ");
                        cgen_expr_in_method(ctx, var_decl->initializer);
                    }
                }
            }
            fprintf(out, "; ");
            if (for_stmt->condition) {
                cgen_expr_in_method(ctx, for_stmt->condition);
            }
            fprintf(out, "; ");
            if (for_stmt->update) {
                cgen_expr_in_method(ctx, for_stmt->update);
            }
            fprintf(out, ") {\n");
            if (for_stmt->body) {
                cgen_block_in_method(ctx, for_stmt->body, indent + 1);
            }
            print_indent(out, indent);
            fprintf(out, "}\n");
            break;
        }
        case CN_AST_STMT_BREAK:
            fprintf(out, "break;\n");
            break;
        case CN_AST_STMT_CONTINUE:
            fprintf(out, "continue;\n");
            break;
        case CN_AST_STMT_BLOCK:
            // 嵌套块
            fprintf(out, "{\n");
            cgen_block_in_method(ctx, stmt->as.block, indent + 1);
            print_indent(out, indent);
            fprintf(out, "}\n");
            break;
        default:
            fprintf(out, "/* 未实现的语句类型: %d */\n", stmt->kind);
            break;
    }
}

/**
 * @brief 生成初始化列表代码
 *
 * 将初始化列表转换为成员赋值语句
 * 例如：: x(0), y(0) 生成 self->x = 0; self->y = 0;
 *
 * @param ctx 代码生成上下文
 * @param class_decl 类声明
 * @param constructor 构造函数成员
 */
bool cgen_initializer_list(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl,
                           CnClassMember *constructor) {
    if (!ctx || !ctx->output_file || !constructor) return false;
    if (constructor->kind != CN_MEMBER_CONSTRUCTOR) return false;
    
    FILE *out = ctx->output_file;
    
    /* 如果没有初始化列表，直接返回成功 */
    if (!constructor->initializer_list || constructor->initializer_count == 0) {
        return true;
    }
    
    /* 生成初始化列表注释 */
    fprintf(out, "    // 初始化列表\n");
    
    /* 遍历初始化列表，生成成员赋值语句 */
    for (size_t i = 0; i < constructor->initializer_count; i++) {
        CnAstStructFieldInit *init_item = &constructor->initializer_list[i];
        
        /* 生成 self->成员名 = 初始值; */
        fprintf(out, "    self->%.*s = ",
                (int)init_item->field_name_length, init_item->field_name);
        
        if (init_item->value) {
            cgen_init_expr_simple(out, init_item->value);
        } else {
            fprintf(out, "0");
        }
        
        fprintf(out, ";\n");
    }
    
    return true;
}

/**
 * @brief 生成构造函数的实现
 */
static void cgen_constructor_impl(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl,
                                   CnClassMember *constructor) {
    if (!ctx || !ctx->output_file || !constructor) return;
    if (constructor->kind != CN_MEMBER_CONSTRUCTOR) return;
    
    FILE *out = ctx->output_file;
    
    // 构造函数返回类指针类型
    fprintf(out, "%.*s* %.*s_construct(",
            (int)class_decl->name_length, class_decl->name,
            (int)class_decl->name_length, class_decl->name);
    
    // 第一个参数是 self 指针
    fprintf(out, "%.*s* self",
            (int)class_decl->name_length, class_decl->name);
    
    // 其他参数
    for (size_t i = 0; i < constructor->parameter_count; i++) {
        CnAstParameter *param = &constructor->parameters[i];
        const char *param_type = "int";
        if (param->declared_type) {
            param_type = get_c_type_string(param->declared_type);
        }
        fprintf(out, ", %s %.*s", param_type,
                (int)param->name_length, param->name);
    }
    
    fprintf(out, ") {\n");
    
    // 初始化虚函数表指针（如果有虚函数）
    if (class_needs_vtable(class_decl)) {
        fprintf(out, "    // 初始化虚函数表指针\n");
        fprintf(out, "    self->vtable = &_%.*s_vtable;\n",
                (int)class_decl->name_length, class_decl->name);
    }
    
    // 调用基类构造函数（多继承支持，支持虚继承）
    // 构造顺序：
    // 1. 先构造虚基类（由最派生类直接构造）
    // 2. 再构造非虚基类（按继承顺序）
    
    // 第一步：构造虚基类（只有最派生类才构造虚基类）
    // 虚基类子对象放在结构体末尾，通过vbptr访问
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        // 只处理虚基类
        if (!base_info->is_virtual) continue;
        
        fprintf(out, "    // 构造虚基类 %.*s（由最派生类直接构造）\n",
                (int)base_info->base_class_name_length, base_info->base_class_name);
        // 虚基类通过vbptr访问偏移量
        fprintf(out, "    {\n");
        fprintf(out, "        // 获取虚基类子对象地址（通过vbptr）\n");
        fprintf(out, "        struct %.*s* vbptr_%.*s = (struct %.*s*)((char*)self + self->vbptr->%.*s_offset);\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name);
        fprintf(out, "        %.*s_construct(vbptr_%.*s);\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name);
        fprintf(out, "    }\n");
    }
    
    // 第二步：构造非虚基类（按继承顺序）
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        // 跳过虚基类（虚基类已在上面处理）
        if (base_info->is_virtual) continue;
        
        fprintf(out, "    // 初始化非虚基类 %.*s（偏移量: 使用宏 %.*s_%.*s_OFFSET）\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)class_decl->name_length, class_decl->name,
                (int)base_info->base_class_name_length, base_info->base_class_name);
        // 使用基类子对象成员名访问
        fprintf(out, "    %.*s_construct(&self->%.*s_base);  // 调用基类构造函数\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name);
    }
    
    // 生成初始化列表代码
    cgen_initializer_list(ctx, class_decl, constructor);
    
    // 函数体
    if (constructor->body && constructor->body->stmt_count > 0) {
        fprintf(out, "    // 构造函数体\n");
        cgen_block_in_method(ctx, constructor->body, 1);
    }
    
    // 返回 self 指针
    fprintf(out, "    return self;\n");
    
    fprintf(out, "}\n\n");
}

/**
 * @brief 生成成员函数的定义
 *
 * 静态方法不需要 self 指针参数
 */
static void cgen_method_impl(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl,
                              CnClassMember *method) {
    if (!ctx || !ctx->output_file || !method) return;
    if (method->kind != CN_MEMBER_METHOD) return;
    
    FILE *out = ctx->output_file;
    
    // 返回类型
    const char *return_type = "void";
    if (method->type) {
        return_type = get_c_type_string(method->type);
    }
    
    // 函数名：类名_方法名
    fprintf(out, "%s %.*s_%.*s(",
            return_type,
            (int)class_decl->name_length, class_decl->name,
            (int)method->name_length, method->name);
    
    // 静态方法不需要 self 指针参数
    if (!method->is_static) {
        // 参数列表：第一个参数是 self 指针
        fprintf(out, "struct %.*s* self",
                (int)class_decl->name_length, class_decl->name);
    }
    
    // 其他参数
    for (size_t i = 0; i < method->parameter_count; i++) {
        CnAstParameter *param = &method->parameters[i];
        const char *param_type = "int";
        if (param->declared_type) {
            param_type = get_c_type_string(param->declared_type);
        }
        // 非静态方法需要逗号分隔，静态方法第一个参数不需要逗号
        if (!method->is_static || i > 0) {
            fprintf(out, ", ");
        }
        fprintf(out, "%s %.*s", param_type,
                (int)param->name_length, param->name);
    }
    
    // 静态方法如果没有参数，需要添加 void
    if (method->is_static && method->parameter_count == 0) {
        fprintf(out, "void");
    }
    
    fprintf(out, ") {\n");
    
    // 函数体
    if (method->body && method->body->stmt_count > 0) {
        fprintf(out, "    // 方法体\n");
        cgen_block_in_method(ctx, method->body, 1);
    } else if (method->is_pure_virtual) {
        // 纯虚函数：不生成实现
        fprintf(out, "    // 纯虚函数，无实现\n");
    } else {
        // 空函数体
        fprintf(out, "    // 空方法\n");
    }
    
    fprintf(out, "}\n\n");
}

/**
 * @brief 生成析构函数的实现
 */
static void cgen_destructor_impl(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl,
                                  CnClassMember *destructor) {
    if (!ctx || !ctx->output_file || !destructor) return;
    if (destructor->kind != CN_MEMBER_DESTRUCTOR) return;
    
    FILE *out = ctx->output_file;
    
    // 析构函数返回void，参数只有self指针
    fprintf(out, "void %.*s_destruct(%.*s* self) {\n",
            (int)class_decl->name_length, class_decl->name,
            (int)class_decl->name_length, class_decl->name);
    
    // 函数体
    if (destructor->body && destructor->body->stmt_count > 0) {
        fprintf(out, "    // 析构函数体\n");
        cgen_block_in_method(ctx, destructor->body, 1);
    } else {
        // 空函数体
        fprintf(out, "    // 空析构函数\n");
    }
    
    // 调用基类析构函数（多继承支持，析构顺序与构造相反）
    // 析构顺序：
    // 1. 先析构非虚基类（从最后一个到第一个）
    // 2. 最后析构虚基类（由最派生类负责）
    
    // 第一步：析构非虚基类（从最后一个到第一个）
    for (size_t i = class_decl->base_count; i > 0; i--) {
        CnInheritanceInfo *base_info = &class_decl->bases[i - 1];
        
        // 跳过虚基类（虚基类在后面处理）
        if (base_info->is_virtual) continue;
        
        fprintf(out, "    // 析构非虚基类 %.*s（偏移量: 使用宏 %.*s_%.*s_OFFSET）\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)class_decl->name_length, class_decl->name,
                (int)base_info->base_class_name_length, base_info->base_class_name);
        // 使用基类子对象成员名访问
        fprintf(out, "    %.*s_destruct(&self->%.*s_base);  // 调用基类析构函数\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name);
    }
    
    // 第二步：析构虚基类（从最后一个到第一个）
    for (size_t i = class_decl->base_count; i > 0; i--) {
        CnInheritanceInfo *base_info = &class_decl->bases[i - 1];
        
        // 只处理虚基类
        if (!base_info->is_virtual) continue;
        
        fprintf(out, "    // 析构虚基类 %.*s（由最派生类负责）\n",
                (int)base_info->base_class_name_length, base_info->base_class_name);
        // 虚基类通过vbptr访问偏移量
        fprintf(out, "    {\n");
        fprintf(out, "        // 获取虚基类子对象地址（通过vbptr）\n");
        fprintf(out, "        struct %.*s* vbptr_%.*s = (struct %.*s*)((char*)self + self->vbptr->%.*s_offset);\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name);
        fprintf(out, "        %.*s_destruct(vbptr_%.*s);\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name);
        fprintf(out, "    }\n");
    }
    
    fprintf(out, "}\n\n");
}

/* ============================================================================
 * 公开接口实现
 * ============================================================================ */

bool cn_cgen_class_struct(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl) {
    if (!ctx || !ctx->output_file || !class_decl) return false;
    
    FILE *out = ctx->output_file;
    
    // 计算类布局信息
    CnClassLayout layout;
    cn_class_layout_init(&layout);
    cn_class_layout_calculate(&layout, class_decl, NULL, NULL);
    
    // 结构体前向声明
    fprintf(out, "struct %.*s;\n\n",
            (int)class_decl->name_length, class_decl->name);
    
    // 如果有虚基类，先生成vbtable结构体
    if (layout.has_virtual_base && layout.vbtable_count > 0) {
        fprintf(out, "/* 虚基类表（vbtable）- 记录虚基类偏移量 */\n");
        fprintf(out, "typedef struct %.*s_vbtable {\n",
                (int)class_decl->name_length, class_decl->name);
        for (size_t i = 0; i < layout.vbtable_count; i++) {
            print_indent(out, 1);
            fprintf(out, "size_t %.*s_offset;  // 虚基类 %.*s 的偏移量\n",
                    (int)layout.vbtable_name_lengths[i], layout.vbtable_names[i],
                    (int)layout.vbtable_name_lengths[i], layout.vbtable_names[i]);
        }
        fprintf(out, "} %.*s_vbtable;\n\n",
                (int)class_decl->name_length, class_decl->name);
    }
    
    // 结构体定义开始
    fprintf(out, "typedef struct %.*s {\n",
            (int)class_decl->name_length, class_decl->name);
    
    // 第一步：布局非虚基类子对象
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        // 跳过虚基类，虚基类在后面处理
        if (base_info->is_virtual) continue;
        
        print_indent(out, 1);
        fprintf(out, "struct %.*s %.*s_base;  // 基类子对象（偏移量: %zu字节，%s继承）\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name,
                layout.base_offsets[i],
                base_info->access == CN_ACCESS_PUBLIC ? "公开" :
                base_info->access == CN_ACCESS_PROTECTED ? "保护" : "私有");
    }
    
    // 第二步：如果有虚基类，添加vbptr（虚基类指针）
    if (layout.has_virtual_base) {
        print_indent(out, 1);
        fprintf(out, "struct %.*s_vbtable* vbptr;  // 虚基类表指针（偏移量: %zu字节）\n",
                (int)class_decl->name_length, class_decl->name,
                layout.vbptr_offset != SIZE_MAX ? layout.vbptr_offset : 0);
    }
    
    // 第三步：如果需要虚函数表，添加vtable指针
    if (class_needs_vtable(class_decl)) {
        print_indent(out, 1);
        fprintf(out, "struct %.*s_vtable* vtable;  // 虚函数表指针（偏移量: %zu字节）\n",
                (int)class_decl->name_length, class_decl->name,
                layout.has_vtable ? layout.vtable_offset : 0);
    }
    
    // 第四步：生成成员变量（字段）
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_FIELD && member->access != CN_ACCESS_PRIVATE) {
            cgen_member_field(out, member, 1);
        }
    }
    
    // 第五步：虚基类子对象（放在最后，通过vbptr访问）
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        
        // 只处理虚基类
        if (!base_info->is_virtual) continue;
        
        print_indent(out, 1);
        fprintf(out, "struct %.*s %.*s_vbase;  // 虚基类子对象（偏移量: %zu字节，虚继承）\n",
                (int)base_info->base_class_name_length, base_info->base_class_name,
                (int)base_info->base_class_name_length, base_info->base_class_name,
                layout.base_offsets[i]);
    }
    
    // 结构体定义结束
    fprintf(out, "} %.*s;\n\n",
            (int)class_decl->name_length, class_decl->name);
    
    // 生成基类偏移量常量（用于this指针调整）
    if (class_decl->base_count > 0) {
        fprintf(out, "/* 类 %.*s 的基类偏移量常量（用于this指针调整） */\n",
                (int)class_decl->name_length, class_decl->name);
        for (size_t i = 0; i < class_decl->base_count; i++) {
            CnInheritanceInfo *base_info = &class_decl->bases[i];
            const char *virtual_mark = base_info->is_virtual ? "（虚基类）" : "";
            fprintf(out, "#define %.*s_%.*s_OFFSET %zu  /* %s */\n",
                    (int)class_decl->name_length, class_decl->name,
                    (int)base_info->base_class_name_length, base_info->base_class_name,
                    layout.base_offsets[i],
                    virtual_mark);
        }
        fprintf(out, "\n");
        
        // 生成this指针调整宏
        fprintf(out, "/* this指针调整宏 */\n");
        for (size_t i = 0; i < class_decl->base_count; i++) {
            CnInheritanceInfo *base_info = &class_decl->bases[i];
            
            if (base_info->is_virtual) {
                // 虚基类通过vbptr访问
                fprintf(out, "/* 虚基类 %.*s 通过vbptr访问 */\n",
                        (int)base_info->base_class_name_length, base_info->base_class_name);
                fprintf(out, "#define %.*s_TO_%.*s(ptr) ((struct %.*s*)((char*)(ptr) + ((struct %.*s*)(ptr))->vbptr->%.*s_offset))\n",
                        (int)class_decl->name_length, class_decl->name,
                        (int)base_info->base_class_name_length, base_info->base_class_name,
                        (int)base_info->base_class_name_length, base_info->base_class_name,
                        (int)class_decl->name_length, class_decl->name,
                        (int)base_info->base_class_name_length, base_info->base_class_name);
            } else {
                // 非虚基类直接计算偏移
                fprintf(out, "#define %.*s_TO_%.*s(ptr) ((struct %.*s*)((char*)(ptr) + %.*s_%.*s_OFFSET))\n",
                        (int)class_decl->name_length, class_decl->name,
                        (int)base_info->base_class_name_length, base_info->base_class_name,
                        (int)base_info->base_class_name_length, base_info->base_class_name,
                        (int)class_decl->name_length, class_decl->name,
                        (int)base_info->base_class_name_length, base_info->base_class_name);
            }
        }
        fprintf(out, "\n");
    }
    
    // 清理布局信息
    cn_class_layout_cleanup(&layout);
    
    return true;
}

/**
 * @brief 生成单个虚函数条目的函数指针声明
 */
static void cgen_vtable_entry_decl(FILE *out, CnVTableEntry *entry,
                                    const char *class_name, size_t class_name_len) {
    if (!entry || !entry->method) return;
    
    CnClassMember *method = entry->method;
    
    // 返回类型
    const char *return_type = "void";
    if (method->type) {
        return_type = get_c_type_string(method->type);
    }
    
    // 函数指针
    fprintf(out, "%s (*%.*s)(struct %.*s* self",
            return_type,
            (int)entry->method_name_length, entry->method_name,
            (int)class_name_len, class_name);
    
    // 其他参数
    for (size_t j = 0; j < method->parameter_count; j++) {
        CnAstParameter *param = &method->parameters[j];
        const char *param_type = "int";
        if (param->declared_type) {
            param_type = get_c_type_string(param->declared_type);
        }
        fprintf(out, ", %s %.*s", param_type,
                (int)param->name_length, param->name);
    }
    
    fprintf(out, ");\n");
}

/**
 * @brief 生成纯虚函数调用错误处理函数
 *
 * 当运行时调用纯虚函数时，输出错误信息并终止程序
 */
static void cgen_pure_virtual_error_func(FILE *out, CnAstClassDecl *class_decl,
                                          CnClassMember *method) {
    if (!out || !method) return;
    
    // 返回类型
    const char *return_type = "void";
    if (method->type) {
        return_type = get_c_type_string(method->type);
    }
    
    // 函数名：类名_方法名_pure_virtual_error
    fprintf(out, "static %s %.*s_%.*s_pure_virtual_error(struct %.*s* self) {\n",
            return_type,
            (int)class_decl->name_length, class_decl->name,
            (int)method->name_length, method->name,
            (int)class_decl->name_length, class_decl->name);
    
    // 输出错误信息
    fprintf(out, "    fprintf(stderr, \"错误: 调用了纯虚函数 %.*s::%.*s\\n\");\n",
            (int)class_decl->name_length, class_decl->name,
            (int)method->name_length, method->name);
    fprintf(out, "    fprintf(stderr, \"  类: %.*s\\n\");\n",
            (int)class_decl->name_length, class_decl->name);
    fprintf(out, "    fprintf(stderr, \"  方法: %.*s\\n\");\n",
            (int)method->name_length, method->name);
    fprintf(out, "    fprintf(stderr, \"  请在派生类中实现此方法\\n\");\n");
    fprintf(out, "    abort();\n");
    fprintf(out, "}\n\n");
}

/**
 * @brief 使用vtable_builder生成虚函数表
 */
bool cn_cgen_vtable(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl) {
    if (!ctx || !ctx->output_file || !class_decl) return false;
    
    // 如果不需要虚函数表，直接返回成功
    if (!cn_vtable_needs_vtable(class_decl)) {
        return true;
    }
    
    FILE *out = ctx->output_file;
    
    // 创建临时vtable用于代码生成
    CnVTable *vtable = cn_vtable_create(class_decl->name, class_decl->name_length);
    if (!vtable) return false;
    
    // 添加当前类的虚函数
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD && member->is_virtual) {
            cn_vtable_add_entry_ex(vtable, member,
                                    class_decl->name, class_decl->name_length);
        }
    }
    
    // 生成纯虚函数的错误处理函数
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD &&
            member->is_virtual && member->is_pure_virtual) {
            cgen_pure_virtual_error_func(out, class_decl, member);
        }
    }
    
    // 虚函数表结构体定义
    fprintf(out, "typedef struct %.*s_vtable {\n",
            (int)class_decl->name_length, class_decl->name);
    
    // 遍历vtable条目，生成函数指针
    for (size_t i = 0; i < vtable->entry_count; i++) {
        CnVTableEntry *entry = &vtable->entries[i];
        print_indent(out, 1);
        cgen_vtable_entry_decl(out, entry, class_decl->name, class_decl->name_length);
    }
    
    fprintf(out, "} %.*s_vtable;\n\n",
            (int)class_decl->name_length, class_decl->name);
    
    // 生成虚函数表实例（静态变量）
    fprintf(out, "static %.*s_vtable _%.*s_vtable = {\n",
            (int)class_decl->name_length, class_decl->name,
            (int)class_decl->name_length, class_decl->name);
    
    // 初始化虚函数表
    bool first = true;
    for (size_t i = 0; i < vtable->entry_count; i++) {
        CnVTableEntry *entry = &vtable->entries[i];
        if (!first) {
            fprintf(out, ",\n");
        }
        first = false;
        
        print_indent(out, 1);
        
        // 检查是否为纯虚函数
        if (entry->is_pure_virtual) {
            // 纯虚函数指向错误处理函数
            fprintf(out, ".%.*s = %.*s_%.*s_pure_virtual_error",
                    (int)entry->method_name_length, entry->method_name,
                    (int)class_decl->name_length, class_decl->name,
                    (int)entry->method_name_length, entry->method_name);
        } else {
            // 普通虚函数：使用定义该方法的类名作为函数前缀
            fprintf(out, ".%.*s = %.*s_%.*s",
                    (int)entry->method_name_length, entry->method_name,
                    (int)entry->defined_in_class_len, entry->defined_in_class,
                    (int)entry->method_name_length, entry->method_name);
        }
    }
    
    fprintf(out, "\n};\n\n");
    
    // 清理临时vtable
    cn_vtable_destroy(vtable);
    
    return true;
}

bool cn_cgen_class_methods(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl) {
    if (!ctx || !ctx->output_file || !class_decl) return false;
    
    FILE *out = ctx->output_file;
    
    // 首先生成所有构造函数和方法的前向声明
    fprintf(out, "// 类 %.*s 的构造函数和方法声明\n",
            (int)class_decl->name_length, class_decl->name);
    
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_CONSTRUCTOR) {
            cgen_constructor_forward_decl(out, class_decl, member);
        } else if (member->kind == CN_MEMBER_DESTRUCTOR) {
            cgen_destructor_forward_decl(out, class_decl, member);
        } else if (member->kind == CN_MEMBER_METHOD) {
            cgen_method_forward_decl(out, class_decl, member);
        }
    }
    
    fprintf(out, "\n");
    
    // 然后生成所有构造函数和方法的实现
    fprintf(out, "// 类 %.*s 的构造函数和方法实现\n",
            (int)class_decl->name_length, class_decl->name);
    
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_CONSTRUCTOR) {
            cgen_constructor_impl(ctx, class_decl, member);
        } else if (member->kind == CN_MEMBER_DESTRUCTOR) {
            cgen_destructor_impl(ctx, class_decl, member);
        } else if (member->kind == CN_MEMBER_METHOD) {
            cgen_method_impl(ctx, class_decl, member);
        }
    }
    
    return true;
}

bool cn_cgen_class_decl(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl) {
    if (!ctx || !ctx->output_file || !class_decl) return false;
    
    FILE *out = ctx->output_file;
    
    // 类注释
    fprintf(out, "// ========================================\n");
    fprintf(out, "// 类定义: %.*s\n",
            (int)class_decl->name_length, class_decl->name);
    if (class_decl->is_abstract) {
        fprintf(out, "// (抽象类)\n");
    }
    if (class_decl->is_final) {
        fprintf(out, "// (最终类)\n");
    }
    fprintf(out, "// ========================================\n\n");
    
    // 1. 生成静态成员变量（全局变量）
    cgen_static_fields(out, class_decl);
    
    // 2. 生成结构体定义
    if (!cn_cgen_class_struct(ctx, class_decl)) {
        return false;
    }
    
    // 3. 生成虚函数表（如果需要）
    if (!cn_cgen_vtable(ctx, class_decl)) {
        return false;
    }
    
    // 4. 生成成员函数
    if (!cn_cgen_class_methods(ctx, class_decl)) {
        return false;
    }
    
    return true;
}

bool cn_cgen_interface_decl(CnCCodeGenContext *ctx, CnAstInterfaceDecl *interface_decl) {
    if (!ctx || !ctx->output_file || !interface_decl) return false;
    
    FILE *out = ctx->output_file;
    
    // 接口注释
    fprintf(out, "// ========================================\n");
    fprintf(out, "// 接口定义: %.*s\n",
            (int)interface_decl->name_length, interface_decl->name);
    
    // 如果有基接口，显示继承关系
    if (interface_decl->base_interface_count > 0) {
        fprintf(out, "// 继承接口: ");
        for (size_t i = 0; i < interface_decl->base_interface_count; i++) {
            CnInheritanceInfo *base = &interface_decl->base_interfaces[i];
            if (i > 0) fprintf(out, ", ");
            fprintf(out, "%.*s", (int)base->base_class_name_length, base->base_class_name);
        }
        fprintf(out, "\n");
    }
    fprintf(out, "// ========================================\n\n");
    
    // 接口转换为虚函数表结构体
    fprintf(out, "typedef struct %.*s_vtable {\n",
            (int)interface_decl->name_length, interface_decl->name);
    
    // 如果有基接口，先包含基接口的vtable
    if (interface_decl->base_interface_count > 0) {
        for (size_t i = 0; i < interface_decl->base_interface_count; i++) {
            CnInheritanceInfo *base = &interface_decl->base_interfaces[i];
            print_indent(out, 1);
            fprintf(out, "struct %.*s_vtable %.*s_base;  // 基接口 %.*s\n",
                    (int)base->base_class_name_length, base->base_class_name,
                    (int)base->base_class_name_length, base->base_class_name,
                    (int)base->base_class_name_length, base->base_class_name);
        }
    }
    
    // 生成所有接口方法的函数指针
    for (size_t i = 0; i < interface_decl->method_count; i++) {
        CnClassMember *method = &interface_decl->methods[i];
        
        print_indent(out, 1);
        
        // 返回类型
        const char *return_type = "void";
        if (method->type) {
            return_type = get_c_type_string(method->type);
        }
        
        // 函数指针（接口方法都是纯虚函数）
        fprintf(out, "%s (*%.*s)(void* self",
                return_type,
                (int)method->name_length, method->name);
        
        // 其他参数
        for (size_t j = 0; j < method->parameter_count; j++) {
            CnAstParameter *param = &method->parameters[j];
            const char *param_type = "int";
            if (param->declared_type) {
                param_type = get_c_type_string(param->declared_type);
            }
            fprintf(out, ", %s %.*s", param_type,
                    (int)param->name_length, param->name);
        }
        
        fprintf(out, ");\n");
    }
    
    fprintf(out, "} %.*s_vtable;\n\n",
            (int)interface_decl->name_length, interface_decl->name);
    
    return true;
}

/* ============================================================================
 * 虚函数调用代码生成实现
 * ============================================================================ */

/**
 * @brief 在类层次结构中查找成员方法
 *
 * @param class_decl 类声明
 * @param method_name 方法名
 * @param method_name_len 方法名长度
 * @return 找到的成员指针，未找到返回NULL
 */
static CnClassMember *find_method_in_class(CnAstClassDecl *class_decl,
                                            const char *method_name,
                                            size_t method_name_len) {
    if (!class_decl || !method_name) return NULL;
    
    for (size_t i = 0; i < class_decl->member_count; i++) {
        CnClassMember *member = &class_decl->members[i];
        if (member->kind == CN_MEMBER_METHOD &&
            member->name_length == method_name_len &&
            memcmp(member->name, method_name, method_name_len) == 0) {
            return member;
        }
    }
    
    return NULL;
}

bool cn_cgen_is_virtual_method(CnAstClassDecl *class_decl,
                                const char *method_name, size_t method_name_len,
                                CnClassMember **out_method) {
    if (!class_decl || !method_name) {
        return false;
    }
    
    // 在当前类中查找方法
    CnClassMember *method = find_method_in_class(class_decl, method_name, method_name_len);
    
    // 如果在当前类找到，检查是否为虚函数
    if (method) {
        if (out_method) {
            *out_method = method;
        }
        return method->is_virtual;
    }
    
    // 如果当前类没有找到，在基类中查找（继承的方法也可能是虚函数）
    // 注意：基类的方法如果被标记为虚函数，派生类继承后仍然是虚函数
    for (size_t i = 0; i < class_decl->base_count; i++) {
        CnInheritanceInfo *base_info = &class_decl->bases[i];
        // 这里需要通过符号表查找基类的声明
        // 目前简化处理：如果方法在当前类中未找到，返回false
        // 完整实现需要遍历继承链
    }
    
    return false;
}

/**
 * @brief 生成表达式的C代码（简化版，用于参数）
 */
static void cgen_expr_for_arg(FILE *out, CnAstExpr *expr) {
    if (!out || !expr) {
        fprintf(out, "0");
        return;
    }
    
    switch (expr->kind) {
        case CN_AST_EXPR_INTEGER_LITERAL:
            fprintf(out, "%lld", expr->as.integer_literal.value);
            break;
        case CN_AST_EXPR_FLOAT_LITERAL:
            fprintf(out, "%f", expr->as.float_literal.value);
            break;
        case CN_AST_EXPR_STRING_LITERAL:
            fprintf(out, "\"");
            for (const char *p = expr->as.string_literal.value; p && *p; p++) {
                switch (*p) {
                    case '\n': fprintf(out, "\\n"); break;
                    case '\t': fprintf(out, "\\t"); break;
                    case '\r': fprintf(out, "\\r"); break;
                    case '\\': fprintf(out, "\\\\"); break;
                    case '"': fprintf(out, "\\\""); break;
                    default: fprintf(out, "%c", *p); break;
                }
            }
            fprintf(out, "\"");
            break;
        case CN_AST_EXPR_BOOL_LITERAL:
            fprintf(out, "%s", expr->as.bool_literal.value ? "1" : "0");
            break;
        case CN_AST_EXPR_IDENTIFIER:
            fprintf(out, "%.*s", (int)expr->as.identifier.name_length, expr->as.identifier.name);
            break;
        case CN_AST_EXPR_MEMBER_ACCESS:
            // 成员访问：obj.member 或 ptr->member
            cgen_expr_for_arg(out, expr->as.member.object);
            if (expr->as.member.is_arrow) {
                fprintf(out, "->");
            } else {
                fprintf(out, ".");
            }
            fprintf(out, "%.*s", (int)expr->as.member.member_name_length, expr->as.member.member_name);
            break;
        case CN_AST_EXPR_BINARY:
            fprintf(out, "(");
            cgen_expr_for_arg(out, expr->as.binary.left);
            switch (expr->as.binary.op) {
                case CN_AST_BINARY_OP_ADD: fprintf(out, " + "); break;
                case CN_AST_BINARY_OP_SUB: fprintf(out, " - "); break;
                case CN_AST_BINARY_OP_MUL: fprintf(out, " * "); break;
                case CN_AST_BINARY_OP_DIV: fprintf(out, " / "); break;
                case CN_AST_BINARY_OP_MOD: fprintf(out, " %% "); break;
                case CN_AST_BINARY_OP_EQ: fprintf(out, " == "); break;
                case CN_AST_BINARY_OP_NE: fprintf(out, " != "); break;
                case CN_AST_BINARY_OP_LT: fprintf(out, " < "); break;
                case CN_AST_BINARY_OP_LE: fprintf(out, " <= "); break;
                case CN_AST_BINARY_OP_GT: fprintf(out, " > "); break;
                case CN_AST_BINARY_OP_GE: fprintf(out, " >= "); break;
                default: fprintf(out, " ? "); break;
            }
            cgen_expr_for_arg(out, expr->as.binary.right);
            fprintf(out, ")");
            break;
        default:
            fprintf(out, "0");
            break;
    }
}

bool cn_cgen_virtual_call(CnCCodeGenContext *ctx,
                          const char *class_name, size_t class_name_len,
                          const char *method_name, size_t method_name_len,
                          const char *object_expr,
                          CnAstExpr **args, size_t arg_count) {
    if (!ctx || !ctx->output_file || !class_name || !method_name || !object_expr) {
        return false;
    }
    
    FILE *out = ctx->output_file;
    
    // 生成虚函数调用：obj->vtable->method(obj, args...)
    // 格式：object_expr->vtable->method_name(object_expr, arg1, arg2, ...)
    fprintf(out, "%s->vtable->%.*s(%s",
            object_expr,
            (int)method_name_len, method_name,
            object_expr);
    
    // 输出参数
    if (args && arg_count > 0) {
        for (size_t i = 0; i < arg_count; i++) {
            fprintf(out, ", ");
            cgen_expr_for_arg(out, args[i]);
        }
    }
    
    fprintf(out, ")");
    
    return true;
}

void cn_cgen_member_access_call(CnCCodeGenContext *ctx,
                                 CnAstExpr *object_expr,
                                 const char *member_name, size_t member_name_len,
                                 bool is_arrow,
                                 CnAstExpr **args, size_t arg_count,
                                 CnAstClassDecl *class_decl) {
    if (!ctx || !ctx->output_file || !object_expr || !member_name) {
        return;
    }
    
    FILE *out = ctx->output_file;
    
    // 检查是否为虚函数调用
    CnClassMember *method = NULL;
    bool is_virtual = false;
    
    if (class_decl) {
        is_virtual = cn_cgen_is_virtual_method(class_decl, member_name, member_name_len, &method);
    }
    
    if (is_virtual && method) {
        // 虚函数调用：通过vtable调用
        // 首先生成对象表达式
        char object_buf[256] = {0};
        
        // 使用临时缓冲区捕获对象表达式
        // 注意：这里简化处理，直接生成到输出文件
        // 完整实现需要使用字符串缓冲区
        
        // 生成对象表达式到临时变量
        fprintf(out, "({ ");
        fprintf(out, "struct %.*s* _obj = ", (int)class_decl->name_length, class_decl->name);
        
        // 生成对象表达式
        if (object_expr->is_this_pointer) {
            fprintf(out, "self");
        } else {
            cgen_expr_for_arg(out, object_expr);
        }
        fprintf(out, "; ");
        
        // 生成虚函数调用
        fprintf(out, "_obj->vtable->%.*s(_obj", (int)member_name_len, member_name);
        
        // 输出参数
        if (args && arg_count > 0) {
            for (size_t i = 0; i < arg_count; i++) {
                fprintf(out, ", ");
                cgen_expr_for_arg(out, args[i]);
            }
        }
        
        fprintf(out, "); })");
    } else {
        // 普通成员访问或非虚函数调用
        if (object_expr->is_this_pointer) {
            // 自身指针访问
            fprintf(out, "self->%.*s", (int)member_name_len, member_name);
        } else {
            // 普通对象访问
            cgen_expr_for_arg(out, object_expr);
            if (is_arrow) {
                fprintf(out, "->");
            } else {
                fprintf(out, ".");
            }
            fprintf(out, "%.*s", (int)member_name_len, member_name);
        }
        
        // 如果有参数，生成函数调用
        if (args && arg_count > 0) {
            fprintf(out, "(");
            
            // 非虚成员函数需要传递self指针作为第一个参数
            if (class_decl && method && !method->is_static) {
                if (object_expr->is_this_pointer) {
                    fprintf(out, "self");
                } else {
                    cgen_expr_for_arg(out, object_expr);
                }
            }
            
            for (size_t i = 0; i < arg_count; i++) {
                if (i > 0 || (class_decl && method && !method->is_static)) {
                    fprintf(out, ", ");
                }
                cgen_expr_for_arg(out, args[i]);
            }
            fprintf(out, ")");
        }
    }
}

/* ============================================================================
 * 多继承this指针调整函数实现
 * ============================================================================ */

/**
 * @brief 生成基类方法调用时的this指针调整代码
 *
 * 当调用基类的方法时，需要将派生类的this指针调整为基类子对象的指针
 * 例如：C类继承A和B，调用B的方法时需要：
 *   B* b_ptr = (B*)((char*)c_ptr + B_OFFSET);
 *   b_ptr->method(b_ptr);
 *
 * @param ctx 代码生成上下文
 * @param class_decl 派生类声明
 * @param base_name 基类名称
 * @param base_name_len 基类名称长度
 * @param method_name 方法名称
 * @param method_name_len 方法名称长度
 * @param args 参数表达式数组
 * @param arg_count 参数数量
 */
void cn_cgen_base_method_call(CnCCodeGenContext *ctx,
                               CnAstClassDecl *class_decl,
                               const char *base_name, size_t base_name_len,
                               const char *method_name, size_t method_name_len,
                               CnAstExpr **args, size_t arg_count) {
    if (!ctx || !ctx->output_file || !class_decl || !base_name || !method_name) {
        return;
    }
    
    FILE *out = ctx->output_file;
    
    // 查找基类信息
    CnInheritanceInfo *base_info = NULL;
    size_t base_index = 0;
    for (size_t i = 0; i < class_decl->base_count; i++) {
        if (class_decl->bases[i].base_class_name_length == base_name_len &&
            memcmp(class_decl->bases[i].base_class_name, base_name, base_name_len) == 0) {
            base_info = &class_decl->bases[i];
            base_index = i;
            break;
        }
    }
    
    if (!base_info) {
        // 基类未找到，生成错误占位符
        fprintf(out, "/* 错误: 基类 %.*s 未找到 */", (int)base_name_len, base_name);
        return;
    }
    
    // 生成this指针调整和方法调用
    // 使用语句表达式 ({ ... }) 来实现临时变量
    fprintf(out, "({ ");
    
    // 声明基类指针临时变量
    fprintf(out, "struct %.*s* _base_ptr = (struct %.*s*)((char*)self + %.*s_%.*s_OFFSET); ",
            (int)base_name_len, base_name,
            (int)base_name_len, base_name,
            (int)class_decl->name_length, class_decl->name,
            (int)base_name_len, base_name);
    
    // 调用基类方法
    fprintf(out, "%.*s_%.*s(_base_ptr",
            (int)base_name_len, base_name,
            (int)method_name_len, method_name);
    
    // 输出参数
    if (args && arg_count > 0) {
        for (size_t i = 0; i < arg_count; i++) {
            fprintf(out, ", ");
            cgen_expr_for_arg(out, args[i]);
        }
    }
    
    fprintf(out, "); })");
}

/**
 * @brief 生成向上转型代码（派生类指针转基类指针）
 *
 * @param out 输出文件
 * @param class_decl 派生类声明
 * @param base_name 基类名称
 * @param base_name_len 基类名称长度
 * @param expr 派生类对象表达式
 */
void cn_cgen_upcast(FILE *out, CnAstClassDecl *class_decl,
                    const char *base_name, size_t base_name_len,
                    const char *expr) {
    if (!out || !class_decl || !base_name || !expr) {
        return;
    }
    
    // 使用宏进行向上转型
    fprintf(out, "%.*s_TO_%.*s(%s)",
            (int)class_decl->name_length, class_decl->name,
            (int)base_name_len, base_name,
            expr);
}

/**
 * @brief 生成向下转型代码（基类指针转派生类指针）
 *
 * 注意：向下转型需要运行时类型检查（RTTI），这里生成简化版本
 *
 * @param out 输出文件
 * @param class_decl 派生类声明
 * @param base_name 基类名称
 * @param base_name_len 基类名称长度
 * @param expr 基类对象表达式
 */
void cn_cgen_downcast(FILE *out, CnAstClassDecl *class_decl,
                      const char *base_name, size_t base_name_len,
                      const char *expr) {
    if (!out || !class_decl || !base_name || !expr) {
        return;
    }
    
    // 向下转型需要减去偏移量
    fprintf(out, "((struct %.*s*)((char*)(%s) - %.*s_%.*s_OFFSET))",
            (int)class_decl->name_length, class_decl->name,
            expr,
            (int)class_decl->name_length, class_decl->name,
            (int)base_name_len, base_name);
}

/**
 * @brief 生成基类成员访问代码
 *
 * 访问基类的成员变量时，需要正确计算偏移量
 *
 * @param out 输出文件
 * @param class_decl 派生类声明
 * @param base_name 基类名称
 * @param base_name_len 基类名称长度
 * @param member_name 成员名称
 * @param member_name_len 成员名称长度
 * @param is_arrow 是否使用箭头操作符
 */
void cn_cgen_base_member_access(FILE *out, CnAstClassDecl *class_decl,
                                 const char *base_name, size_t base_name_len,
                                 const char *member_name, size_t member_name_len,
                                 bool is_arrow) {
    if (!out || !class_decl || !base_name || !member_name) {
        return;
    }
    
    // 直接通过基类子对象成员访问
    // self->base_name_base.member 或 self->base_name_base->member
    if (is_arrow) {
        fprintf(out, "self->%.*s_base.%.*s",
                (int)base_name_len, base_name,
                (int)member_name_len, member_name);
    } else {
        fprintf(out, "self->%.*s_base.%.*s",
                (int)base_name_len, base_name,
                (int)member_name_len, member_name);
    }
}
