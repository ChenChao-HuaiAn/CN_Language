#include "cnlang/frontend/ast.h"

#include <stdlib.h>

static void cn_frontend_ast_stmt_array_free(CnAstStmt **stmts, size_t count);
static void cn_frontend_ast_expr_array_free(CnAstExpr **exprs, size_t count);

void cn_frontend_ast_expr_free(CnAstExpr *expr)
{
    if (!expr) {
        return;
    }

    switch (expr->kind) {
    case CN_AST_EXPR_BINARY:
        cn_frontend_ast_expr_free(expr->as.binary.left);
        cn_frontend_ast_expr_free(expr->as.binary.right);
        break;
    case CN_AST_EXPR_CALL:
        cn_frontend_ast_expr_free(expr->as.call.callee);
        cn_frontend_ast_expr_array_free(expr->as.call.arguments, expr->as.call.argument_count);
        free(expr->as.call.arguments);
        break;
    case CN_AST_EXPR_IDENTIFIER:
        break;
    case CN_AST_EXPR_INTEGER_LITERAL:
        break;
    case CN_AST_EXPR_STRING_LITERAL:
        break;
    case CN_AST_EXPR_ASSIGN:
        cn_frontend_ast_expr_free(expr->as.assign.target);
        cn_frontend_ast_expr_free(expr->as.assign.value);
        break;
    case CN_AST_EXPR_LOGICAL:
        cn_frontend_ast_expr_free(expr->as.logical.left);
        cn_frontend_ast_expr_free(expr->as.logical.right);
        break;
    case CN_AST_EXPR_UNARY:
        cn_frontend_ast_expr_free(expr->as.unary.operand);
        break;
    case CN_AST_EXPR_ARRAY_LITERAL:
        // 释放数组字面量中的所有元素
        cn_frontend_ast_expr_array_free(expr->as.array_literal.elements, expr->as.array_literal.element_count);
        free(expr->as.array_literal.elements);
        break;
    case CN_AST_EXPR_INDEX:
        // 释放索引访问表达式
        cn_frontend_ast_expr_free(expr->as.index.array);
        cn_frontend_ast_expr_free(expr->as.index.index);
        break;
    case CN_AST_EXPR_MEMBER_ACCESS:
        // 释放结构体成员访问表达式
        cn_frontend_ast_expr_free(expr->as.member.object);
        break;
    case CN_AST_EXPR_STRUCT_LITERAL:
        // 释放结构体字面量表达式
        for (size_t i = 0; i < expr->as.struct_lit.field_count; i++) {
            cn_frontend_ast_expr_free(expr->as.struct_lit.fields[i].value);
        }
        free(expr->as.struct_lit.fields);
        break;
    }

    free(expr);
}

void cn_frontend_ast_block_free(CnAstBlockStmt *block)
{
    if (!block) {
        return;
    }

    cn_frontend_ast_stmt_array_free(block->stmts, block->stmt_count);
    free(block->stmts);
    free(block);
}

void cn_frontend_ast_stmt_free(CnAstStmt *stmt)
{
    if (!stmt) {
        return;
    }

    switch (stmt->kind) {
    case CN_AST_STMT_BLOCK:
        cn_frontend_ast_block_free(stmt->as.block);
        break;
    case CN_AST_STMT_VAR_DECL:
        cn_frontend_ast_expr_free(stmt->as.var_decl.initializer);
        break;
    case CN_AST_STMT_EXPR:
        cn_frontend_ast_expr_free(stmt->as.expr.expr);
        break;
    case CN_AST_STMT_RETURN:
        cn_frontend_ast_expr_free(stmt->as.return_stmt.expr);
        break;
    case CN_AST_STMT_IF:
        cn_frontend_ast_expr_free(stmt->as.if_stmt.condition);
        cn_frontend_ast_block_free(stmt->as.if_stmt.then_block);
        cn_frontend_ast_block_free(stmt->as.if_stmt.else_block);
        break;
    case CN_AST_STMT_WHILE:
        cn_frontend_ast_expr_free(stmt->as.while_stmt.condition);
        cn_frontend_ast_block_free(stmt->as.while_stmt.body);
        break;
    case CN_AST_STMT_FOR:
        cn_frontend_ast_stmt_free(stmt->as.for_stmt.init);
        cn_frontend_ast_expr_free(stmt->as.for_stmt.condition);
        cn_frontend_ast_expr_free(stmt->as.for_stmt.update);
        cn_frontend_ast_block_free(stmt->as.for_stmt.body);
        break;
    case CN_AST_STMT_BREAK:
        // break 语句没有子节点
        break;
    case CN_AST_STMT_CONTINUE:
        // continue 语句没有子节点
        break;
    case CN_AST_STMT_STRUCT_DECL:
        // 释放结构体声明
        free(stmt->as.struct_decl.fields);
        break;
    case CN_AST_STMT_ENUM_DECL:
        // 释放枚举声明
        free(stmt->as.enum_decl.members);
        break;
    }

    free(stmt);
}

void cn_frontend_ast_function_free(CnAstFunctionDecl *function_decl)
{
    if (!function_decl) {
        return;
    }

    free(function_decl->parameters);  // 释放参数数组
    cn_frontend_ast_block_free(function_decl->body);
    free(function_decl);
}

void cn_frontend_ast_program_free(CnAstProgram *program)
{
    size_t i;

    if (!program) {
        return;
    }

    for (i = 0; i < program->function_count; ++i) {
        cn_frontend_ast_function_free(program->functions[i]);
    }

    for (i = 0; i < program->struct_count; ++i) {
        cn_frontend_ast_stmt_free(program->structs[i]);
    }

    for (i = 0; i < program->enum_count; ++i) {
        cn_frontend_ast_stmt_free(program->enums[i]);
    }

    free(program->functions);
    free(program->structs);
    free(program->enums);
    free(program);
}

static void cn_frontend_ast_stmt_array_free(CnAstStmt **stmts, size_t count)
{
    size_t i;

    if (!stmts) {
        return;
    }

    for (i = 0; i < count; ++i) {
        cn_frontend_ast_stmt_free(stmts[i]);
    }
}

static void cn_frontend_ast_expr_array_free(CnAstExpr **exprs, size_t count)
{
    size_t i;

    if (!exprs) {
        return;
    }

    for (i = 0; i < count; ++i) {
        cn_frontend_ast_expr_free(exprs[i]);
    }
}
