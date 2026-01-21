#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    /* 测试 while 和 for 循环：包含两种循环语句 */
    const char *source = "函数 测试循环() { 当 (i < 10) { 返回 1; } 循环 (j; j < 5; j) { 返回 j; } }";
    size_t length = strlen(source);
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    bool ok;

    cn_frontend_lexer_init(&lexer, source, length, "<memory>");

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "parser_loop_test: 创建解析器失败\n");
        return 1;
    }

    ok = cn_frontend_parse_program(parser, &program);
    if (!ok) {
        fprintf(stderr, "parser_loop_test: 解析失败 (ok=false)\n");
        cn_frontend_parser_free(parser);
        return 1;
    }
    
    if (!program) {
        fprintf(stderr, "parser_loop_test: 解析失败 (program=NULL)\n");
        cn_frontend_parser_free(parser);
        return 1;
    }

    if (program->function_count != 1) {
        fprintf(stderr, "parser_loop_test: 期望 1 个函数，实际为 %zu\n",
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
                "parser_loop_test: 期望函数体内有 2 条语句，实际为 %zu\n",
                actual);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        return 1;
    }

    {
        CnAstBlockStmt *body = program->functions[0]->body;
        CnAstStmt *first_loop = body->stmts[0];
        CnAstStmt *second_loop = body->stmts[1];

        if (first_loop->kind != CN_AST_STMT_WHILE) {
            fprintf(stderr, "parser_loop_test: 第一条语句应为 while 循环\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (!first_loop->as.while_stmt.condition) {
            fprintf(stderr, "parser_loop_test: while 循环缺少条件表达式\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (!first_loop->as.while_stmt.body) {
            fprintf(stderr, "parser_loop_test: while 循环缺少循环体\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (second_loop->kind != CN_AST_STMT_FOR) {
            fprintf(stderr, "parser_loop_test: 第二条语句应为 for 循环\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (!second_loop->as.for_stmt.init) {
            fprintf(stderr, "parser_loop_test: for 循环缺少初始化语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }
    }

    printf("parser_loop_test: OK\n");

    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);

    return 0;
}
