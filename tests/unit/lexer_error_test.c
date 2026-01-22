#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/token.h"
#include "cnlang/support/diagnostics.h"

#include <stdio.h>
#include <string.h>

// 测试非法字符
static int test_invalid_char(void)
{
    const char *source = "变量 x = 123 @ 456;";
    size_t length = strlen(source);
    CnLexer lexer;
    CnToken token;
    CnDiagnostics diagnostics;
    size_t i;

    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source, length, "<test-invalid-char>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    // 扫描所有 token
    while (1) {
        if (!cn_frontend_lexer_next_token(&lexer, &token)) {
            break;
        }
        if (token.kind == CN_TOKEN_EOF) {
            break;
        }
    }

    // 应该至少产生一个错误（非法字符 @）
    if (diagnostics.count == 0) {
        fprintf(stderr, "lexer_error_test: 非法字符应产生错误\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // 检查是否有 LEX_INVALID_CHAR 错误
    for (i = 0; i < diagnostics.count; i++) {
        if (diagnostics.items[i].code == CN_DIAG_CODE_LEX_INVALID_CHAR) {
            cn_support_diagnostics_free(&diagnostics);
            return 0;  // 找到预期错误
        }
    }

    fprintf(stderr, "lexer_error_test: 未找到 LEX_INVALID_CHAR 错误\n");
    cn_support_diagnostics_free(&diagnostics);
    return 1;
}

// 测试未终止的字符串
static int test_unterminated_string(void)
{
    const char *source = "变量 s = \"这是一个未终止的字符串";
    size_t length = strlen(source);
    CnLexer lexer;
    CnToken token;
    CnDiagnostics diagnostics;
    size_t i;

    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source, length, "<test-unterminated-string>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    // 扫描所有 token
    while (1) {
        if (!cn_frontend_lexer_next_token(&lexer, &token)) {
            break;
        }
        if (token.kind == CN_TOKEN_EOF) {
            break;
        }
    }

    // 应该至少产生一个错误（未终止的字符串）
    if (diagnostics.count == 0) {
        fprintf(stderr, "lexer_error_test: 未终止的字符串应产生错误\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // 检查是否有 LEX_UNTERMINATED_STRING 错误
    for (i = 0; i < diagnostics.count; i++) {
        if (diagnostics.items[i].code == CN_DIAG_CODE_LEX_UNTERMINATED_STRING) {
            cn_support_diagnostics_free(&diagnostics);
            return 0;  // 找到预期错误
        }
    }

    fprintf(stderr, "lexer_error_test: 未找到 LEX_UNTERMINATED_STRING 错误\n");
    cn_support_diagnostics_free(&diagnostics);
    return 1;
}

int main(void)
{
    if (test_invalid_char() != 0) {
        return 1;
    }

    if (test_unterminated_string() != 0) {
        return 1;
    }

    printf("lexer_error_test: OK\n");
    return 0;
}
