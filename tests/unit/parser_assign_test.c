#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    /* 测试赋值表达式：包含 for 循环中的赋值场景 */
    const char *source = "函数 测试赋值() { 循环 (i = 0; i < 10; i = i + 1) { x = i; } 返回 x; }";
    size_t length = strlen(source);
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    bool ok;

    cn_frontend_lexer_init(&lexer, source, length, "<memory>");

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "parser_assign_test: 创建解析器失败\n");
        return 1;
    }

    ok = cn_frontend_parse_program(parser, &program);
    if (!ok) {
        fprintf(stderr, "parser_assign_test: 解析失败 (ok=false)\n");
        cn_frontend_parser_free(parser);
        return 1;
    }
    
    if (!program) {
        fprintf(stderr, "parser_assign_test: 解析失败 (program=NULL)\n");
        cn_frontend_parser_free(parser);
        return 1;
    }

    if (program->function_count != 1) {
        fprintf(stderr, "parser_assign_test: 期望 1 个函数，实际为 %zu\n",
                program->function_count);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        return 1;
    }

    if (!program->functions[0]->body || program->functions[0]->body->stmt_count != 2) {
        size_t actual = 0;
        if (program->functions[0]->body) {
            actual = program->functions[0]->body->stmt_count;
        }
        fprintf(stderr,
                "parser_assign_test: 期望函数体内有 2 条语句，实际为 %zu\n",
                actual);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        return 1;
    }

    {
        CnAstBlockStmt *body = program->functions[0]->body;
        CnAstStmt *for_stmt = body->stmts[0];
        CnAstStmt *return_stmt = body->stmts[1];

        if (for_stmt->kind != CN_AST_STMT_FOR) {
            fprintf(stderr, "parser_assign_test: 第一条语句应为 for 循环\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证 for 的初始化部分是赋值表达式语句：i = 0
        if (!for_stmt->as.for_stmt.init) {
            fprintf(stderr, "parser_assign_test: for 循环缺少初始化语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (for_stmt->as.for_stmt.init->kind != CN_AST_STMT_EXPR) {
            fprintf(stderr, "parser_assign_test: for 初始化应为表达式语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        CnAstExpr *init_expr = for_stmt->as.for_stmt.init->as.expr.expr;
        if (!init_expr || init_expr->kind != CN_AST_EXPR_ASSIGN) {
            fprintf(stderr, "parser_assign_test: for 初始化应为赋值表达式\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证 for 的更新部分是赋值表达式：i = i + 1
        if (!for_stmt->as.for_stmt.update) {
            fprintf(stderr, "parser_assign_test: for 循环缺少更新表达式\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (for_stmt->as.for_stmt.update->kind != CN_AST_EXPR_ASSIGN) {
            fprintf(stderr, "parser_assign_test: for 更新应为赋值表达式\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证 for 循环体内的赋值语句：x = i
        if (!for_stmt->as.for_stmt.body || for_stmt->as.for_stmt.body->stmt_count != 1) {
            fprintf(stderr, "parser_assign_test: for 循环体应有 1 条语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        CnAstStmt *body_stmt = for_stmt->as.for_stmt.body->stmts[0];
        if (body_stmt->kind != CN_AST_STMT_EXPR) {
            fprintf(stderr, "parser_assign_test: for 循环体语句应为表达式语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        CnAstExpr *body_expr = body_stmt->as.expr.expr;
        if (!body_expr || body_expr->kind != CN_AST_EXPR_ASSIGN) {
            fprintf(stderr, "parser_assign_test: for 循环体应包含赋值表达式\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证第二条语句是 return
        if (return_stmt->kind != CN_AST_STMT_RETURN) {
            fprintf(stderr, "parser_assign_test: 第二条语句应为 return\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }
    }

    printf("parser_assign_test: OK\n");

    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);

    return 0;
}
