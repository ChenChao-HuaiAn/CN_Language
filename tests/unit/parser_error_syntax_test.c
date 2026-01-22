#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/support/diagnostics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 测试括号不匹配
static int test_unmatched_parenthesis(void)
{
    /* 缺少右括号 */
    const char *source = "函数 测试() { 返回 (1 + 2; }";
    size_t length = strlen(source);
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    CnDiagnostics diagnostics;
    bool ok;

    cn_support_diagnostics_init(&diagnostics);

    cn_frontend_lexer_init(&lexer, source, length, "<test-unmatched-paren>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "parser_error_syntax_test: 创建解析器失败(括号不匹配用例)\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    ok = cn_frontend_parse_program(parser, &program);
    if (ok) {
        fprintf(stderr, "parser_error_syntax_test: 括号不匹配用例应当解析失败\n");
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    /* 应该产生至少一个错误 */
    if (diagnostics.count == 0) {
        fprintf(stderr, "parser_error_syntax_test: 括号不匹配用例未产生错误\n");
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    if (program) {
        cn_frontend_ast_program_free(program);
    }
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    return 0;
}

// 测试缺少花括号
static int test_missing_brace(void)
{
    /* 缺少右花括号 */
    const char *source = "函数 测试() { 返回 0;";
    size_t length = strlen(source);
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    CnDiagnostics diagnostics;
    bool ok;

    cn_support_diagnostics_init(&diagnostics);

    cn_frontend_lexer_init(&lexer, source, length, "<test-missing-brace>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "parser_error_syntax_test: 创建解析器失败(缺少花括号用例)\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    ok = cn_frontend_parse_program(parser, &program);
    if (ok) {
        fprintf(stderr, "parser_error_syntax_test: 缺少花括号用例应当解析失败\n");
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    /* 应该产生至少一个错误 */
    if (diagnostics.count == 0) {
        fprintf(stderr, "parser_error_syntax_test: 缺少花括号用例未产生错误\n");
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    if (program) {
        cn_frontend_ast_program_free(program);
    }
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    return 0;
}

// 测试多个语法错误
static int test_multiple_errors(void)
{
    /* 缺少分号和参数名无效 */
    const char *source = "函数 测试(123) { 返回 0 }";
    size_t length = strlen(source);
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    CnDiagnostics diagnostics;
    bool ok;

    cn_support_diagnostics_init(&diagnostics);

    cn_frontend_lexer_init(&lexer, source, length, "<test-multiple-errors>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "parser_error_syntax_test: 创建解析器失败(多个错误用例)\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    ok = cn_frontend_parse_program(parser, &program);
    if (ok) {
        fprintf(stderr, "parser_error_syntax_test: 多个错误用例应当解析失败\n");
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    /* 应该产生至少一个错误 */
    if (diagnostics.count == 0) {
        fprintf(stderr, "parser_error_syntax_test: 多个错误用例未产生错误\n");
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    if (program) {
        cn_frontend_ast_program_free(program);
    }
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    return 0;
}

int main(void)
{
    if (test_unmatched_parenthesis() != 0) {
        return 1;
    }

    if (test_missing_brace() != 0) {
        return 1;
    }

    if (test_multiple_errors() != 0) {
        return 1;
    }

    printf("parser_error_syntax_test: OK\n");
    return 0;
}
