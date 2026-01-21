#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    /* 最小语法子集示例：一个函数，函数体内两条算术表达式语句 */
    const char *source = "函数 测试函数() { 如果 (1) { 返回 2; } 返回 3; }";
    size_t length = strlen(source);
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    bool ok;

    cn_frontend_lexer_init(&lexer, source, length, "<memory>");

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "parser_minimal_test: 创建解析器失败\n");
        return 1;
    }

    ok = cn_frontend_parse_program(parser, &program);
    if (!ok || !program) {
        fprintf(stderr, "parser_minimal_test: 解析失败\n");
        cn_frontend_parser_free(parser);
        return 1;
    }

    if (program->function_count != 1) {
        fprintf(stderr, "parser_minimal_test: 期望 1 个函数，实际为 %zu\n",
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
                "parser_minimal_test: 期望主程序函数体内有 2 条语句，实际为 %zu\n",
                actual);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        return 1;
    }

    {
        CnAstBlockStmt *body = program->functions[0]->body;
        CnAstStmt *first_stmt = body->stmts[0];
        CnAstStmt *second_stmt = body->stmts[1];

        if (first_stmt->kind != CN_AST_STMT_IF) {
            fprintf(stderr, "parser_minimal_test: 第一条语句应为 if 语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (!first_stmt->as.if_stmt.condition || !first_stmt->as.if_stmt.then_block) {
            fprintf(stderr, "parser_minimal_test: if 语句的条件或 then 块为空\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (second_stmt->kind != CN_AST_STMT_RETURN) {
            fprintf(stderr, "parser_minimal_test: 第二条语句应为 return 语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (!second_stmt->as.return_stmt.expr) {
            fprintf(stderr, "parser_minimal_test: return 语句缺少返回表达式\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }
    }

    printf("parser_minimal_test: OK\n");

    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);

    return 0;
}
