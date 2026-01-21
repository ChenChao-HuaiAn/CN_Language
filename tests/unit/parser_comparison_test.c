#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    /* 测试比较运算符：包含 if 条件中的比较表达式 */
    const char *source = "函数 测试比较() { 如果 (1 < 2) { 返回 1; } 如果 (3 == 3) { 返回 2; } 返回 0; }";
    size_t length = strlen(source);
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    bool ok;

    cn_frontend_lexer_init(&lexer, source, length, "<memory>");

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "parser_comparison_test: 创建解析器失败\n");
        return 1;
    }

    ok = cn_frontend_parse_program(parser, &program);
    if (!ok || !program) {
        fprintf(stderr, "parser_comparison_test: 解析失败\n");
        cn_frontend_parser_free(parser);
        return 1;
    }

    if (program->function_count != 1) {
        fprintf(stderr, "parser_comparison_test: 期望 1 个函数，实际为 %zu\n",
                program->function_count);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        return 1;
    }

    if (!program->functions[0]->body || program->functions[0]->body->stmt_count != 3) {
        size_t actual = 0;
        if (program->functions[0]->body) {
            actual = program->functions[0]->body->stmt_count;
        }
        fprintf(stderr,
                "parser_comparison_test: 期望函数体内有 3 条语句，实际为 %zu\n",
                actual);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        return 1;
    }

    {
        CnAstBlockStmt *body = program->functions[0]->body;
        CnAstStmt *first_if = body->stmts[0];
        CnAstStmt *second_if = body->stmts[1];

        if (first_if->kind != CN_AST_STMT_IF) {
            fprintf(stderr, "parser_comparison_test: 第一条语句应为 if 语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (!first_if->as.if_stmt.condition) {
            fprintf(stderr, "parser_comparison_test: 第一个 if 缺少条件表达式\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (first_if->as.if_stmt.condition->kind != CN_AST_EXPR_BINARY) {
            fprintf(stderr, "parser_comparison_test: 第一个 if 条件应为二元表达式\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (first_if->as.if_stmt.condition->as.binary.op != CN_AST_BINARY_OP_LT) {
            fprintf(stderr, "parser_comparison_test: 第一个 if 条件应为 < 运算符\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (second_if->kind != CN_AST_STMT_IF) {
            fprintf(stderr, "parser_comparison_test: 第二条语句应为 if 语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (second_if->as.if_stmt.condition->kind != CN_AST_EXPR_BINARY) {
            fprintf(stderr, "parser_comparison_test: 第二个 if 条件应为二元表达式\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (second_if->as.if_stmt.condition->as.binary.op != CN_AST_BINARY_OP_EQ) {
            fprintf(stderr, "parser_comparison_test: 第二个 if 条件应为 == 运算符\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }
    }

    printf("parser_comparison_test: OK\n");

    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);

    return 0;
}
