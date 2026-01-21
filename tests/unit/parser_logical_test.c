#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    /* 测试逻辑运算符：包含 &&, ||, ! 的复杂条件 */
    const char *source = "函数 测试逻辑() { 如果 (i > 0 && i < 10 || !flag) { 返回 1; } 返回 0; }";
    size_t length = strlen(source);
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    bool ok;

    cn_frontend_lexer_init(&lexer, source, length, "<memory>");

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "parser_logical_test: 创建解析器失败\n");
        return 1;
    }

    ok = cn_frontend_parse_program(parser, &program);
    if (!ok) {
        fprintf(stderr, "parser_logical_test: 解析失败 (ok=false)\n");
        cn_frontend_parser_free(parser);
        return 1;
    }
    
    if (!program) {
        fprintf(stderr, "parser_logical_test: 解析失败 (program=NULL)\n");
        cn_frontend_parser_free(parser);
        return 1;
    }

    if (program->function_count != 1) {
        fprintf(stderr, "parser_logical_test: 期望 1 个函数，实际为 %zu\n",
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
                "parser_logical_test: 期望函数体内有 2 条语句，实际为 %zu\n",
                actual);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        return 1;
    }

    {
        CnAstBlockStmt *body = program->functions[0]->body;
        CnAstStmt *if_stmt = body->stmts[0];

        if (if_stmt->kind != CN_AST_STMT_IF) {
            fprintf(stderr, "parser_logical_test: 第一条语句应为 if\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证 if 条件：i > 0 && i < 10 || !flag
        // 应解析为：(i > 0 && i < 10) || !flag
        CnAstExpr *condition = if_stmt->as.if_stmt.condition;
        if (!condition || condition->kind != CN_AST_EXPR_LOGICAL) {
            fprintf(stderr, "parser_logical_test: if 条件应为逻辑表达式\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (condition->as.logical.op != CN_AST_LOGICAL_OP_OR) {
            fprintf(stderr, "parser_logical_test: 顶层逻辑运算应为 OR (||)\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证左侧：i > 0 && i < 10
        CnAstExpr *left = condition->as.logical.left;
        if (!left || left->kind != CN_AST_EXPR_LOGICAL) {
            fprintf(stderr, "parser_logical_test: OR 左侧应为逻辑 AND 表达式\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (left->as.logical.op != CN_AST_LOGICAL_OP_AND) {
            fprintf(stderr, "parser_logical_test: 应为 AND (&&) 运算\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证右侧：!flag
        CnAstExpr *right = condition->as.logical.right;
        if (!right || right->kind != CN_AST_EXPR_UNARY) {
            fprintf(stderr, "parser_logical_test: OR 右侧应为一元 NOT 表达式\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        if (right->as.unary.op != CN_AST_UNARY_OP_NOT) {
            fprintf(stderr, "parser_logical_test: 应为 NOT (!) 运算\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }

        // 验证 !flag 的操作数是标识符
        CnAstExpr *operand = right->as.unary.operand;
        if (!operand || operand->kind != CN_AST_EXPR_IDENTIFIER) {
            fprintf(stderr, "parser_logical_test: NOT 的操作数应为标识符\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            return 1;
        }
    }

    printf("parser_logical_test: OK\n");

    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);

    return 0;
}
