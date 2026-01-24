#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cnlang/frontend/lexer.h"
#include "cnlang/support/diagnostics.h"
#include "cnlang/support/memory/arena.h"
#include "cnlang/support/memory_profiler.h"

/*
 * 关键优化路径回归测试
 * 确保性能优化不会破坏原有功能
 */

/* 测试1: 词法分析器基本功能回归 */
static void test_lexer_basic_regression(void)
{
    const char *source = "变量 x = 42;";
    
    CnLexer lexer;
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    // 验证每个 token
    CnToken token;
    
    assert(cn_frontend_lexer_next_token(&lexer, &token));
    assert(token.kind == CN_TOKEN_KEYWORD_VAR);
    
    assert(cn_frontend_lexer_next_token(&lexer, &token));
    assert(token.kind == CN_TOKEN_IDENT);
    
    assert(cn_frontend_lexer_next_token(&lexer, &token));
    assert(token.kind == CN_TOKEN_EQUAL);
    
    assert(cn_frontend_lexer_next_token(&lexer, &token));
    assert(token.kind == CN_TOKEN_INTEGER);
    
    assert(cn_frontend_lexer_next_token(&lexer, &token));
    assert(token.kind == CN_TOKEN_SEMICOLON);
    
    assert(cn_frontend_lexer_next_token(&lexer, &token));
    assert(token.kind == CN_TOKEN_EOF);
    
    cn_support_diagnostics_free(&diagnostics);
    printf("[PASS] test_lexer_basic_regression\n");
}

/* 测试2: Arena 分配器功能回归 */
static void test_arena_functionality_regression(void)
{
    CnArena *arena = cn_arena_new(0);
    assert(arena != NULL);
    
    // 基本分配
    void *p1 = cn_arena_alloc(arena, 100);
    assert(p1 != NULL);
    
    // 对齐分配
    void *p2 = cn_arena_alloc_aligned(arena, 100, 16);
    assert(p2 != NULL);
    assert(((uintptr_t)p2 % 16) == 0);
    
    // 大对象分配
    void *p3 = cn_arena_alloc(arena, 1024 * 1024);
    assert(p3 != NULL);
    
    // 统计信息
    size_t total, block_count;
    cn_arena_get_stats(arena, &total, &block_count);
    assert(total > 0);
    assert(block_count > 0);
    
    // Reset 功能
    cn_arena_reset(arena);
    cn_arena_get_stats(arena, &total, &block_count);
    assert(total == 0);
    assert(block_count == 1);
    
    cn_arena_free(arena);
    printf("[PASS] test_arena_functionality_regression\n");
}

/* 测试3: 诊断系统功能回归 */
static void test_diagnostics_functionality_regression(void)
{
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    // 添加错误
    cn_support_diagnostics_report_error(&diagnostics, CN_DIAG_CODE_UNKNOWN,
                                        "test.cn", 1, 1, "测试错误");
    
    // 添加警告
    cn_support_diagnostics_report_warning(&diagnostics, CN_DIAG_CODE_UNKNOWN,
                                          "test.cn", 2, 1, "测试警告");
    
    // 验证错误计数
    size_t error_count = cn_support_diagnostics_error_count(&diagnostics);
    assert(error_count == 1);
    
    cn_support_diagnostics_free(&diagnostics);
    printf("[PASS] test_diagnostics_functionality_regression\n");
}

/* 测试4: 内存统计功能回归 */
static void test_memory_stats_functionality_regression(void)
{
    CnMemStats stats;
    cn_mem_stats_init(&stats);
    cn_mem_stats_set_enabled(&stats, true);
    
    // 记录分配
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_AST, 1000);
    assert(stats.current_memory_usage == 1000);
    assert(stats.total_allocation_count == 1);
    
    // 记录释放
    cn_mem_stats_record_free(&stats, CN_MEM_CATEGORY_AST, 1000);
    assert(stats.current_memory_usage == 0);
    
    // 峰值追踪
    cn_mem_stats_record_alloc(&stats, CN_MEM_CATEGORY_SYMBOL, 5000);
    assert(stats.peak_memory_usage == 5000);
    
    printf("[PASS] test_memory_stats_functionality_regression\n");
}

/* 测试5: UTF-8 中文处理回归 */
static void test_utf8_handling_regression(void)
{
    const char *source = "变量 中文变量 = 100;";
    
    CnLexer lexer;
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnToken token;
    
    assert(cn_frontend_lexer_next_token(&lexer, &token));
    assert(token.kind == CN_TOKEN_KEYWORD_VAR);
    
    assert(cn_frontend_lexer_next_token(&lexer, &token));
    assert(token.kind == CN_TOKEN_IDENT);
    // 验证标识符长度正确（中文字符）
    assert(token.lexeme_length > 0);
    
    assert(cn_frontend_lexer_next_token(&lexer, &token));
    assert(token.kind == CN_TOKEN_EQUAL);
    
    assert(cn_frontend_lexer_next_token(&lexer, &token));
    assert(token.kind == CN_TOKEN_INTEGER);
    
    cn_support_diagnostics_free(&diagnostics);
    printf("[PASS] test_utf8_handling_regression\n");
}

/* 测试6: 多个token处理回归 */
static void test_multiple_tokens_regression(void)
{
    const char *source = "变量 a = 1; 变量 b = 2;";
    
    CnLexer lexer;
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnToken token;
    int token_count = 0;
    
    while (cn_frontend_lexer_next_token(&lexer, &token) && token.kind != CN_TOKEN_EOF) {
        token_count++;
    }
    
    // 应该有: 变量 a = 1 ; 变量 b = 2 ;  => 10个token
    assert(token_count == 10);
    
    cn_support_diagnostics_free(&diagnostics);
    printf("[PASS] test_multiple_tokens_regression\n");
}

int main(void)
{
    printf("========== 关键优化路径回归测试 ==========\n\n");
    
    test_lexer_basic_regression();
    test_arena_functionality_regression();
    test_diagnostics_functionality_regression();
    test_memory_stats_functionality_regression();
    test_utf8_handling_regression();
    test_multiple_tokens_regression();
    
    printf("\n==========================================\n");
    printf("所有回归测试通过！✓\n");
    printf("==========================================\n");
    
    return 0;
}
