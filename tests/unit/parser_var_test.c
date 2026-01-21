#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    /* 测试变量声明：包含初始化和未初始化，以及在 for 循环中的应用 */
    const char *source = "函数 测试变量() { 变量 x; 变量 sum = 0; 循环 (变量 i = 0; i < 10; i = i + 1) { sum = sum + i; } 返回 sum; }";
    size_t length = strlen(source);
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    bool ok;

    cn_frontend_lexer_init(&lexer, source, length, "<memory>");

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "parser_var_test: 创建解析器失败\n");
        return 1;
    }

    ok = cn_frontend_parse_program(parser, &program);
    if (!ok) {
        fprintf(stderr, "parser_var_test: 解析失败 (ok=false)\n");
        cn_frontend_parser_free(parser);
        return 1;
    }
    
    if (!program) {
        fprintf(stderr, "parser_var_test: 解析失败 (program=NULL)\n");
        cn_frontend_parser_free(parser);
        return 1;
    }

    if (program->function_count != 1) {
        fprintf(stderr, "parser_var_test: 期望 1 个函数，实际为 %zu\n",
                program->function_count);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        return 1;
    }

    if (!program->functions[0]->body || program->functions[0]->body->stmt_count != 4) {
        size_t actual = 0;
        if (program->functions[0]->body) {
            actual = program->functions[0]->body->stmt_count;
        }
        fprintf(stderr,
                "parser_var_test: 期望函数体内有 4 条语句，实际为 %zu\n",
                actual);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        return 1;
    }

    {
        CnAstBlockStmt *body = program->functions[0]->body;
        CnAstStmt *var_x = body->stmts[0];
        CnAstStmt *var_sum = body->stmts[1];
        CnAstStmt *for_stmt = body->stmts[2];
        CnAstStmt *return_stmt = body->stmts[3];

        // 验证第一个变量声明：变量 x; (未初始化)
        if (var_x->kind != CN_AST_STMT_VAR_DECL) {
            fprintf(stderr, "parser_var_test: 第一条应为变量声明\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (var_x->as.var_decl.initializer != NULL) {
            fprintf(stderr, "parser_var_test: 变量 x 不应有初始化值\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证第二个变量声明：变量 sum = 0; (有初始化)
        if (var_sum->kind != CN_AST_STMT_VAR_DECL) {
            fprintf(stderr, "parser_var_test: 第二条应为变量声明\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (var_sum->as.var_decl.initializer == NULL) {
            fprintf(stderr, "parser_var_test: 变量 sum 应有初始化值\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (var_sum->as.var_decl.initializer->kind != CN_AST_EXPR_INTEGER_LITERAL) {
            fprintf(stderr, "parser_var_test: sum 的初始化值应为整数字面量\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证 for 循环
        if (for_stmt->kind != CN_AST_STMT_FOR) {
            fprintf(stderr, "parser_var_test: 第三条应为 for 循环\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证 for 循环的初始化部分是变量声明：变量 i = 0
        if (!for_stmt->as.for_stmt.init) {
            fprintf(stderr, "parser_var_test: for 循环缺少初始化\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (for_stmt->as.for_stmt.init->kind != CN_AST_STMT_VAR_DECL) {
            fprintf(stderr, "parser_var_test: for 初始化应为变量声明\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        CnAstVarDecl *loop_var = &for_stmt->as.for_stmt.init->as.var_decl;
        if (!loop_var->initializer) {
            fprintf(stderr, "parser_var_test: for 循环变量 i 应有初始化值\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (loop_var->initializer->kind != CN_AST_EXPR_INTEGER_LITERAL) {
            fprintf(stderr, "parser_var_test: i 的初始化值应为整数字面量\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证 return 语句
        if (return_stmt->kind != CN_AST_STMT_RETURN) {
            fprintf(stderr, "parser_var_test: 第四条应为 return 语句\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }
    }

    printf("parser_var_test: OK\n");

    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);

    return 0;
}
