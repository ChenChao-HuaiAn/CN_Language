#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "cnlang/frontend/lexer.h"
#include "cnlang/support/diagnostics.h"

/*
 * 词法分析器性能测试 - 热点路径测试
 * 针对阶段7性能优化的回归测试
 */

/* 生成测试源码 */
static char *generate_test_source(int var_count, size_t *out_size)
{
    // 估算大小: 每行约 20 字符
    size_t estimated_size = var_count * 20 + 100;
    char *source = (char *)malloc(estimated_size);
    assert(source != NULL);
    
    size_t pos = 0;
    
    // 生成变量声明
    for (int i = 0; i < var_count; i++) {
        int written = snprintf(source + pos, estimated_size - pos,
                              "变量 x%d = %d;\n", i, i * 2);
        if (written < 0 || pos + written >= estimated_size) break;
        pos += written;
    }
    
    *out_size = pos;
    return source;
}

/* 测试1: 小文件词法分析性能 */
static void test_lexer_small_file(void)
{
    const char *source = 
        "变量 x = 42;\n"
        "变量 y = x + 10;\n";
    
    CnLexer lexer;
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    clock_t start = clock();
    
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    int token_count = 0;
    CnToken token;
    while (cn_frontend_lexer_next_token(&lexer, &token) && token.kind != CN_TOKEN_EOF) {
        token_count++;
    }
    token_count++; // 计入 EOF
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
    
    printf("小文件词法分析:\n");
    printf("  源码大小: %zu 字节\n", strlen(source));
    printf("  Token 数量: %d\n", token_count);
    printf("  耗时: %.3f ms\n", time_spent);
    if (time_spent > 0) {
        printf("  吞吐量: %.2f tokens/ms\n", token_count / time_spent);
    }
    
    cn_support_diagnostics_free(&diagnostics);
    printf("[PASS] test_lexer_small_file\n\n");
}

/* 测试2: 中等文件词法分析性能 */
static void test_lexer_medium_file(void)
{
    size_t source_size;
    char *source = generate_test_source(100, &source_size);  // 100个变量
    
    CnLexer lexer;
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    clock_t start = clock();
    
    cn_frontend_lexer_init(&lexer, source, source_size, "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    int token_count = 0;
    CnToken token;
    while (cn_frontend_lexer_next_token(&lexer, &token) && token.kind != CN_TOKEN_EOF) {
        token_count++;
    }
    token_count++; // 计入 EOF
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
    
    printf("中等文件词法分析:\n");
    printf("  源码大小: %zu 字节\n", source_size);
    printf("  Token 数量: %d\n", token_count);
    printf("  耗时: %.3f ms\n", time_spent);
    if (time_spent > 0) {
        printf("  吞吐量: %.2f tokens/ms\n", token_count / time_spent);
    }
    
    free(source);
    cn_support_diagnostics_free(&diagnostics);
    printf("[PASS] test_lexer_medium_file\n\n");
}

/* 测试3: UTF-8 中文处理性能 */
static void test_lexer_utf8_performance(void)
{
    const char *source = 
        "变量 中文变量一 = 100;\n"
        "变量 中文变量二 = 200;\n";
    
    CnLexer lexer;
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    clock_t start = clock();
    
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    int token_count = 0;
    CnToken token;
    while (cn_frontend_lexer_next_token(&lexer, &token) && token.kind != CN_TOKEN_EOF) {
        token_count++;
    }
    token_count++;
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
    
    printf("UTF-8 中文处理性能:\n");
    printf("  源码大小: %zu 字节\n", strlen(source));
    printf("  Token 数量: %d\n", token_count);
    printf("  耗时: %.3f ms\n", time_spent);
    
    cn_support_diagnostics_free(&diagnostics);
    printf("[PASS] test_lexer_utf8_performance\n\n");
}

/* 测试4: 热点路径 - 连续调用 next_token */
static void test_lexer_hotpath_next_token(void)
{
    size_t source_size;
    char *source = generate_test_source(500, &source_size);  // 500个变量
    
    CnLexer lexer;
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    cn_frontend_lexer_init(&lexer, source, source_size, "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    // 测量纯 next_token 调用性能
    clock_t start = clock();
    int token_count = 0;
    CnToken token;
    
    while (cn_frontend_lexer_next_token(&lexer, &token) && token.kind != CN_TOKEN_EOF) {
        token_count++;
    }
    token_count++;
    
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC * 1000000.0;  // 微秒
    
    printf("热点路径 - next_token 性能:\n");
    printf("  Token 数量: %d\n", token_count);
    printf("  总耗时: %.3f us\n", time_spent);
    if (token_count > 0) {
        printf("  平均每次调用: %.3f us\n", time_spent / token_count);
    }
    
    free(source);
    cn_support_diagnostics_free(&diagnostics);
    printf("[PASS] test_lexer_hotpath_next_token\n\n");
}

int main(void)
{
    printf("========== 词法分析器性能测试 ==========\n\n");
    
    test_lexer_small_file();
    test_lexer_medium_file();
    test_lexer_utf8_performance();
    test_lexer_hotpath_next_token();
    
    printf("=========================================\n");
    printf("所有性能测试通过！✓\n");
    printf("=========================================\n");
    
    return 0;
}
