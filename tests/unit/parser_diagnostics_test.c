#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/support/diagnostics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_missing_semicolon(void)
{
    /* 缺少分号："返回 0" 后没有 ";" */
    const char *source = "函数 主程序() { 返回 0 }";
    size_t length = strlen(source);
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    CnDiagnostics diagnostics;
    bool ok;

    cn_support_diagnostics_init(&diagnostics);

    cn_frontend_lexer_init(&lexer, source, length, "<memory-missing-semicolon>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "parser_diagnostics_test: 创建解析器失败(缺少分号用例)\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    ok = cn_frontend_parse_program(parser, &program);
    if (ok) {
        fprintf(stderr, "parser_diagnostics_test: 缺少分号用例应当解析失败\n");
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    if (diagnostics.count == 0) {
        fprintf(stderr, "parser_diagnostics_test: 缺少分号用例未产生日志\n");
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    {
        CnDiagnostic *d = &diagnostics.items[0];
        if (d->code != CN_DIAG_CODE_PARSE_EXPECTED_TOKEN) {
            fprintf(stderr,
                    "parser_diagnostics_test: 缺少分号用例错误码应为 CN_DIAG_CODE_PARSE_EXPECTED_TOKEN，实际为 %d\n",
                    (int)d->code);
            cn_frontend_parser_free(parser);
            cn_support_diagnostics_free(&diagnostics);
            return 1;
        }
        if (!d->message || strcmp(d->message, "语法错误：缺少预期的标记") != 0) {
            fprintf(stderr,
                    "parser_diagnostics_test: 缺少分号用例 message 不匹配，实际为: %s\n",
                    d->message ? d->message : "<NULL>");
            cn_frontend_parser_free(parser);
            cn_support_diagnostics_free(&diagnostics);
            return 1;
        }
    }

    if (program) {
        cn_frontend_ast_program_free(program);
    }
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    return 0;
}

static int test_invalid_function_name(void)
{
    /* 函数名使用关键字 "返回"，应视为无效函数名 */
    const char *source = "函数 返回() { 返回 0; }";
    size_t length = strlen(source);
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    CnDiagnostics diagnostics;
    bool ok;

    cn_support_diagnostics_init(&diagnostics);

    cn_frontend_lexer_init(&lexer, source, length, "<memory-invalid-fn-name>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "parser_diagnostics_test: 创建解析器失败(函数名非法用例)\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    ok = cn_frontend_parse_program(parser, &program);
    if (ok) {
        fprintf(stderr, "parser_diagnostics_test: 函数名非法用例应当解析失败\n");
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    if (diagnostics.count == 0) {
        fprintf(stderr, "parser_diagnostics_test: 函数名非法用例未产生日志\n");
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    {
        CnDiagnostic *d = &diagnostics.items[0];
        if (d->code != CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME) {
            fprintf(stderr,
                    "parser_diagnostics_test: 函数名非法用例错误码应为 CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME，实际为 %d\n",
                    (int)d->code);
            cn_frontend_parser_free(parser);
            cn_support_diagnostics_free(&diagnostics);
            return 1;
        }
        if (!d->message || strcmp(d->message, "语法错误：函数名无效，期望标识符") != 0) {
            fprintf(stderr,
                    "parser_diagnostics_test: 函数名非法用例 message 不匹配，实际为: %s\n",
                    d->message ? d->message : "<NULL>");
            cn_frontend_parser_free(parser);
            cn_support_diagnostics_free(&diagnostics);
            return 1;
        }
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
    if (test_missing_semicolon() != 0) {
        return 1;
    }

    if (test_invalid_function_name() != 0) {
        return 1;
    }

    printf("parser_diagnostics_test: OK\n");
    return 0;
}
