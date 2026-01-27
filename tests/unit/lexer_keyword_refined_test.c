#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/token.h"
#include "cnlang/support/diagnostics.h"

#include <stdio.h>
#include <string.h>

// 测试已删除的关键字应被识别为普通标识符
static int test_deleted_keywords_as_identifiers(void)
{
    // 测试已删除的关键字: 主程序、数组、与、或、为、从
    const char *source = "主程序 数组 与 或 为 从";
    size_t length = strlen(source);
    CnLexer lexer;
    CnToken token;
    CnDiagnostics diagnostics;

    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source, length, "<test-deleted-keywords>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    // 所有已删除的关键字都应该被识别为 CN_TOKEN_IDENT

    // Token 1: 主程序
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_keyword_refined_test: 无法获取 token (主程序)\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_IDENT) {
        fprintf(stderr, "lexer_keyword_refined_test: '主程序' 应为 IDENT，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // Token 2: 数组
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_keyword_refined_test: 无法获取 token (数组)\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_IDENT) {
        fprintf(stderr, "lexer_keyword_refined_test: '数组' 应为 IDENT，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // Token 3: 与
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_keyword_refined_test: 无法获取 token (与)\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_IDENT) {
        fprintf(stderr, "lexer_keyword_refined_test: '与' 应为 IDENT，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // Token 4: 或
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_keyword_refined_test: 无法获取 token (或)\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_IDENT) {
        fprintf(stderr, "lexer_keyword_refined_test: '或' 应为 IDENT，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // Token 5: 为
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_keyword_refined_test: 无法获取 token (为)\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_IDENT) {
        fprintf(stderr, "lexer_keyword_refined_test: '为' 应为 IDENT，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    // Token 6: 从
    if (!cn_frontend_lexer_next_token(&lexer, &token)) {
        fprintf(stderr, "lexer_keyword_refined_test: 无法获取 token (从)\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    if (token.kind != CN_TOKEN_IDENT) {
        fprintf(stderr, "lexer_keyword_refined_test: '从' 应为 IDENT，实际为 %d\n", token.kind);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }

    cn_support_diagnostics_free(&diagnostics);
    return 0;
}

// 测试保留关键字仍然被正确识别
static int test_preserved_keywords(void)
{
    const char *source = "函数 返回 变量 整数 小数 布尔 字符串 结构体 枚举 常量 模块 导入 如果 否则 当 循环 选择 情况 默认 中断 继续 真 假 空 无 公开 私有";
    size_t length = strlen(source);
    CnLexer lexer;
    CnToken token;
    CnDiagnostics diagnostics;

    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source, length, "<test-preserved-keywords>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    // 定义预期的Token类型
    CnTokenKind expected_tokens[] = {
        CN_TOKEN_KEYWORD_FN,
        CN_TOKEN_KEYWORD_RETURN,
        CN_TOKEN_KEYWORD_VAR,
        CN_TOKEN_KEYWORD_INT,
        CN_TOKEN_KEYWORD_FLOAT,
        CN_TOKEN_KEYWORD_BOOL,
        CN_TOKEN_KEYWORD_STRING,
        CN_TOKEN_KEYWORD_STRUCT,
        CN_TOKEN_KEYWORD_ENUM,
        CN_TOKEN_KEYWORD_CONST,
        CN_TOKEN_KEYWORD_MODULE,
        CN_TOKEN_KEYWORD_IMPORT,
        CN_TOKEN_KEYWORD_IF,
        CN_TOKEN_KEYWORD_ELSE,
        CN_TOKEN_KEYWORD_WHILE,
        CN_TOKEN_KEYWORD_FOR,
        CN_TOKEN_KEYWORD_SWITCH,
        CN_TOKEN_KEYWORD_CASE,
        CN_TOKEN_KEYWORD_DEFAULT,
        CN_TOKEN_KEYWORD_BREAK,
        CN_TOKEN_KEYWORD_CONTINUE,
        CN_TOKEN_KEYWORD_TRUE,
        CN_TOKEN_KEYWORD_FALSE,
        CN_TOKEN_KEYWORD_NULL,
        CN_TOKEN_KEYWORD_VOID,
        CN_TOKEN_KEYWORD_PUBLIC,
        CN_TOKEN_KEYWORD_PRIVATE,
    };

    size_t expected_count = sizeof(expected_tokens) / sizeof(expected_tokens[0]);

    for (size_t i = 0; i < expected_count; i++) {
        if (!cn_frontend_lexer_next_token(&lexer, &token)) {
            fprintf(stderr, "lexer_keyword_refined_test: 无法获取 token %zu\n", i + 1);
            cn_support_diagnostics_free(&diagnostics);
            return 1;
        }
        if (token.kind != expected_tokens[i]) {
            fprintf(stderr, "lexer_keyword_refined_test: token %zu 应为 %d，实际为 %d\n", 
                    i + 1, expected_tokens[i], token.kind);
            cn_support_diagnostics_free(&diagnostics);
            return 1;
        }
    }

    cn_support_diagnostics_free(&diagnostics);
    return 0;
}

// 测试预留关键字仍然被识别为关键字(词法层)
static int test_reserved_keywords(void)
{
    const char *source = "类 接口 模板 命名空间 静态 保护 虚拟 重写 抽象";
    size_t length = strlen(source);
    CnLexer lexer;
    CnToken token;
    CnDiagnostics diagnostics;

    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source, length, "<test-reserved-keywords>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    // 定义预期的Token类型
    CnTokenKind expected_tokens[] = {
        CN_TOKEN_KEYWORD_CLASS,
        CN_TOKEN_KEYWORD_INTERFACE,
        CN_TOKEN_KEYWORD_TEMPLATE,
        CN_TOKEN_KEYWORD_NAMESPACE,
        CN_TOKEN_KEYWORD_STATIC,
        CN_TOKEN_KEYWORD_PROTECTED,
        CN_TOKEN_KEYWORD_VIRTUAL,
        CN_TOKEN_KEYWORD_OVERRIDE,
        CN_TOKEN_KEYWORD_ABSTRACT,
    };

    size_t expected_count = sizeof(expected_tokens) / sizeof(expected_tokens[0]);

    for (size_t i = 0; i < expected_count; i++) {
        if (!cn_frontend_lexer_next_token(&lexer, &token)) {
            fprintf(stderr, "lexer_keyword_refined_test: 无法获取预留关键字 token %zu\n", i + 1);
            cn_support_diagnostics_free(&diagnostics);
            return 1;
        }
        if (token.kind != expected_tokens[i]) {
            fprintf(stderr, "lexer_keyword_refined_test: 预留关键字 token %zu 应为 %d，实际为 %d\n", 
                    i + 1, expected_tokens[i], token.kind);
            cn_support_diagnostics_free(&diagnostics);
            return 1;
        }
    }

    cn_support_diagnostics_free(&diagnostics);
    return 0;
}

int main(void)
{
    printf("运行精简关键字词法测试...\n");

    if (test_deleted_keywords_as_identifiers() != 0) {
        fprintf(stderr, "测试失败: test_deleted_keywords_as_identifiers\n");
        return 1;
    }
    printf("✓ 已删除关键字被正确识别为标识符\n");

    if (test_preserved_keywords() != 0) {
        fprintf(stderr, "测试失败: test_preserved_keywords\n");
        return 1;
    }
    printf("✓ 保留关键字被正确识别\n");

    if (test_reserved_keywords() != 0) {
        fprintf(stderr, "测试失败: test_reserved_keywords\n");
        return 1;
    }
    printf("✓ 预留关键字被正确识别\n");

    printf("lexer_keyword_refined_test: 所有测试通过 ✅\n");
    return 0;
}
