#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    /* 测试 break 和 continue：包含在循环中的应用 */
    const char *source = "函数 测试跳转() { 当 (i < 10) { 如果 (i == 5) { 中断; } 如果 (i == 3) { 继续; } x = i; } }";
    size_t length = strlen(source);
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    bool ok;

    cn_frontend_lexer_init(&lexer, source, length, "<memory>");

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "parser_break_test: 创建解析器失败\n");
        return 1;
    }

    ok = cn_frontend_parse_program(parser, &program);
    if (!ok) {
        fprintf(stderr, "parser_break_test: 解析失败 (ok=false)\n");
        cn_frontend_parser_free(parser);
        return 1;
    }
    
    if (!program) {
        fprintf(stderr, "parser_break_test: 解析失败 (program=NULL)\n");
        cn_frontend_parser_free(parser);
        return 1;
    }

    if (program->function_count != 1) {
        fprintf(stderr, "parser_break_test: 期望 1 个函数，实际为 %zu\n",
                program->function_count);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        return 1;
    }

    if (!program->functions[0]->body || program->functions[0]->body->stmt_count != 1) {
        size_t actual = 0;
        if (program->functions[0]->body) {
            actual = program->functions[0]->body->stmt_count;
        }
        fprintf(stderr,
                "parser_break_test: 期望函数体内有 1 条语句，实际为 %zu\n",
                actual);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        return 1;
    }

    {
        CnAstBlockStmt *body = program->functions[0]->body;
        CnAstStmt *while_stmt = body->stmts[0];

        if (while_stmt->kind != CN_AST_STMT_WHILE) {
            fprintf(stderr, "parser_break_test: 应为 while 循环\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证 while 循环体有 3 条语句
        CnAstBlockStmt *loop_body = while_stmt->as.while_stmt.body;
        if (!loop_body || loop_body->stmt_count != 3) {
            size_t actual = 0;
            if (loop_body) {
                actual = loop_body->stmt_count;
            }
            fprintf(stderr,
                    "parser_break_test: while 循环体应有 3 条语句，实际为 %zu\n",
                    actual);
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        CnAstStmt *first_if = loop_body->stmts[0];
        CnAstStmt *second_if = loop_body->stmts[1];
        CnAstStmt *assign = loop_body->stmts[2];

        // 验证第一个 if：包含 break
        if (first_if->kind != CN_AST_STMT_IF) {
            fprintf(stderr, "parser_break_test: 第一条应为 if 语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (!first_if->as.if_stmt.then_block || first_if->as.if_stmt.then_block->stmt_count != 1) {
            fprintf(stderr, "parser_break_test: 第一个 if 块应有 1 条语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        CnAstStmt *break_stmt = first_if->as.if_stmt.then_block->stmts[0];
        if (break_stmt->kind != CN_AST_STMT_BREAK) {
            fprintf(stderr, "parser_break_test: 应为 break 语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证第二个 if：包含 continue
        if (second_if->kind != CN_AST_STMT_IF) {
            fprintf(stderr, "parser_break_test: 第二条应为 if 语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (!second_if->as.if_stmt.then_block || second_if->as.if_stmt.then_block->stmt_count != 1) {
            fprintf(stderr, "parser_break_test: 第二个 if 块应有 1 条语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        CnAstStmt *continue_stmt = second_if->as.if_stmt.then_block->stmts[0];
        if (continue_stmt->kind != CN_AST_STMT_CONTINUE) {
            fprintf(stderr, "parser_break_test: 应为 continue 语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证第三条语句是赋值
        if (assign->kind != CN_AST_STMT_EXPR) {
            fprintf(stderr, "parser_break_test: 第三条应为表达式语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }
    }

    printf("parser_break_test: OK\n");

    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);

    return 0;
}
