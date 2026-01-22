#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/support/diagnostics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    /* 测试最小 hello_world 示例是否能被完整解析 */
    const char *source = "函数 主程序() { 打印(\"你好，世界！\"); 返回 0; }";
    size_t length = strlen(source);
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    CnDiagnostics diagnostics;
    bool ok;

    cn_support_diagnostics_init(&diagnostics);

    cn_frontend_lexer_init(&lexer, source, length, "<memory>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "parser_hello_world_test: 创建解析器失败\n");
        return 1;
    }

    ok = cn_frontend_parse_program(parser, &program);
    if (!ok || !program) {
        fprintf(stderr, "parser_hello_world_test: 解析失败\n");
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    /* 1. 程序应当只有一个函数 */
    if (program->function_count != 1) {
        fprintf(stderr,
                "parser_hello_world_test: 期望 1 个函数，实际为 %zu\n",
                program->function_count);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        return 1;
    }

    {
        CnAstFunctionDecl *fn = program->functions[0];
        CnAstBlockStmt *body = fn->body;

        /* 2. 函数名应为 `主程序`（此处只检查长度是否大于 0） */
        if (!fn->name || fn->name_length == 0) {
            fprintf(stderr, "parser_hello_world_test: 主程序函数名为空\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        /* 3. 函数体内应有两条语句：打印 + 返回 */
        if (!body || body->stmt_count != 2) {
            size_t actual = 0;
            if (body) {
                actual = body->stmt_count;
            }
            fprintf(stderr,
                    "parser_hello_world_test: 期望主程序函数体内有 2 条语句，实际为 %zu\n",
                    actual);
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        /* 第一条语句：打印(字符串字面量); */
        {
            CnAstStmt *first_stmt = body->stmts[0];
            if (first_stmt->kind != CN_AST_STMT_EXPR) {
                fprintf(stderr, "parser_hello_world_test: 第一条语句应为表达式语句\n");
                cn_frontend_ast_program_free(program);
                cn_frontend_parser_free(parser);
                return 1;
            }

            CnAstExpr *expr = first_stmt->as.expr.expr;
            if (!expr || expr->kind != CN_AST_EXPR_CALL) {
                fprintf(stderr, "parser_hello_world_test: 第一条语句应为函数调用\n");
                cn_frontend_ast_program_free(program);
                cn_frontend_parser_free(parser);
                return 1;
            }

            CnAstCallExpr *call = &expr->as.call;
            if (!call->callee || call->callee->kind != CN_AST_EXPR_IDENTIFIER) {
                fprintf(stderr, "parser_hello_world_test: 打印调用的 callee 应为标识符\n");
                cn_frontend_ast_program_free(program);
                cn_frontend_parser_free(parser);
                return 1;
            }

            if (call->argument_count != 1) {
                fprintf(stderr,
                        "parser_hello_world_test: 打印应有 1 个实参，实际为 %zu\n",
                        call->argument_count);
                cn_frontend_ast_program_free(program);
                cn_frontend_parser_free(parser);
                return 1;
            }

            if (!call->arguments[0] ||
                call->arguments[0]->kind != CN_AST_EXPR_STRING_LITERAL) {
                fprintf(stderr, "parser_hello_world_test: 打印的实参应为字符串字面量\n");
                cn_frontend_ast_program_free(program);
                cn_frontend_parser_free(parser);
                return 1;
            }
        }

        /* 第二条语句：返回 0; */
        {
            CnAstStmt *second_stmt = body->stmts[1];
            if (second_stmt->kind != CN_AST_STMT_RETURN) {
                fprintf(stderr, "parser_hello_world_test: 第二条语句应为 return 语句\n");
                cn_frontend_ast_program_free(program);
                cn_frontend_parser_free(parser);
                return 1;
            }

            if (!second_stmt->as.return_stmt.expr ||
                second_stmt->as.return_stmt.expr->kind != CN_AST_EXPR_INTEGER_LITERAL ||
                second_stmt->as.return_stmt.expr->as.integer_literal.value != 0) {
                fprintf(stderr, "parser_hello_world_test: return 语句应返回整数 0\n");
                cn_frontend_ast_program_free(program);
                cn_frontend_parser_free(parser);
                return 1;
            }
        }
    }

    printf("parser_hello_world_test: OK\n");

    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);

    return 0;
}
