#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/token.h"
#include "cnlang/support/diagnostics.h"

#include <stdio.h>
#include <string.h>

// 测试简单表达式的 Token 序列
static int test_simple_expression(void)
{
    const char *source = "1 + 2 * 3";
    size_t length = strlen(source);
    CnLexer lexer;
    CnToken token;
    CnDiagnostics diagnostics;

    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source, length, "<test-expr>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    // 期望: INTEGER(1) PLUS INTEGER(2) STAR INTEGER(3) EOF
    
    // Token 1: INTEGER(1)
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_token_test: 无法获取 token 1\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_INTEGER) {
        fprintf(stderr, "lexer_token_test: token 1 应为 INTEGER，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // Token 2: PLUS
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_token_test: 无法获取 token 2\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_PLUS) {
        fprintf(stderr, "lexer_token_test: token 2 应为 PLUS，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // Token 3: INTEGER(2)
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_token_test: 无法获取 token 3\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_INTEGER) {
        fprintf(stderr, "lexer_token_test: token 3 应为 INTEGER，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // Token 4: STAR
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_token_test: 无法获取 token 4\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_STAR) {
        fprintf(stderr, "lexer_token_test: token 4 应为 STAR，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // Token 5: INTEGER(3)
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_token_test: 无法获取 token 5\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_INTEGER) {
        fprintf(stderr, "lexer_token_test: token 5 应为 INTEGER，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // Token 6: EOF
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_token_test: 无法获取 token 6\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_EOF) {
        fprintf(stderr, "lexer_token_test: token 6 应为 EOF，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // 应该没有诊断错误
    if (diagnostics.count > 0) {
        fprintf(stderr, "lexer_token_test: 不应产生诊断错误\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    cn_support_diagnostics_free(&diagnostics);
    return 0;
}

// 测试中文关键字识别
static int test_chinese_keywords(void)
{
    const char *source = "函数 主程序 返回 变量 整数";
    size_t length = strlen(source);
    CnLexer lexer;
    CnToken token;
    CnDiagnostics diagnostics;

    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source, length, "<test-keywords>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    // 期望: KEYWORD_FN KEYWORD_MAIN KEYWORD_RETURN KEYWORD_VAR KEYWORD_INT EOF

    // Token 1: 函数
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_token_test: 无法获取 token (函数)\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_KEYWORD_FN) {
        fprintf(stderr, "lexer_token_test: '函数' 应为 KEYWORD_FN，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // Token 2: 主程序
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_token_test: 无法获取 token (主程序)\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_KEYWORD_MAIN) {
        fprintf(stderr, "lexer_token_test: '主程序' 应为 KEYWORD_MAIN，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // Token 3: 返回
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_token_test: 无法获取 token (返回)\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_KEYWORD_RETURN) {
        fprintf(stderr, "lexer_token_test: '返回' 应为 KEYWORD_RETURN，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // Token 4: 变量
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_token_test: 无法获取 token (变量)\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_KEYWORD_VAR) {
        fprintf(stderr, "lexer_token_test: '变量' 应为 KEYWORD_VAR，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // Token 5: 整数
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_token_test: 无法获取 token (整数)\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_KEYWORD_INT) {
        fprintf(stderr, "lexer_token_test: '整数' 应为 KEYWORD_INT，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // Token 6: EOF
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_token_test: 无法获取 token (EOF)\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_EOF) {
        fprintf(stderr, "lexer_token_test: 最后应为 EOF，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    cn_support_diagnostics_free(&diagnostics);
    return 0;
}

int main(void)
{
    if (test_simple_expression() != 0) {
        return 1;
    }

    if (test_chinese_keywords() != 0) {
        return 1;
    }

    printf("lexer_token_test: OK\n");
    return 0;
}
