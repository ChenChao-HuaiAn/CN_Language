/*
 * integration_repl_statement_test.c
 * REPL 小段程序场景集成测试
 * 
 * 测试场景：
 * - 变量声明语句
 * - 简单函数调用
 * - 多条语句组合
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/support/diagnostics.h"
#include "cnlang/ir/ir.h"
#include "cnlang/ir/irgen.h"
#include "cnlang/support/config.h"

/* 辅助函数：包装语句为完整程序 */
static char *wrap_statement_as_program(const char *stmt)
{
    size_t stmt_len = strlen(stmt);
    size_t wrapper_size = 256 + stmt_len;
    char *wrapped = (char *)malloc(wrapper_size);

    if (!wrapped) {
        return NULL;
    }

    snprintf(wrapped, wrapper_size,
             "函数 主程序() { %s 返回 0; }",
             stmt);

    return wrapped;
}

/* 辅助函数：测试单个语句是否能成功解析和编译 */
static int test_statement(const char *stmt_description, const char *stmt)
{
    printf("测试: %s\n", stmt_description);
    printf("  语句: %s\n", stmt);

    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    CnTargetTriple target_triple;
    char *code = NULL;
    int result = 0;

    // 包装语句
    code = wrap_statement_as_program(stmt);
    if (!code) {
        fprintf(stderr, "  ✗ 内存分配失败\n");
        return 1;
    }

    // 初始化诊断系统
    cn_support_diagnostics_init(&diagnostics);

    // 词法分析
    cn_frontend_lexer_init(&lexer, code, strlen(code), "<test>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    // 语法分析
    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "  ✗ 创建解析器失败\n");
        result = 1;
        goto cleanup_early;
    }
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    bool ok = cn_frontend_parse_program(parser, &program);
    if (!ok || !program) {
        fprintf(stderr, "  ✗ 解析失败\n");
        result = 1;
        goto cleanup;
    }

    // 检查错误
    for (size_t i = 0; i < diagnostics.count; ++i) {
        if (diagnostics.items[i].severity == CN_DIAG_SEVERITY_ERROR) {
            fprintf(stderr, "  ✗ 语义错误: %s\n", diagnostics.items[i].message);
            result = 1;
            goto cleanup;
        }
    }

    // 生成 IR
    target_triple = cn_support_target_triple_make(
        CN_TARGET_ARCH_X86_64,
        CN_TARGET_VENDOR_PC,
        CN_TARGET_OS_NONE,
        CN_TARGET_ABI_ELF);

    CnIrModule *ir_module = cn_ir_gen_program(program, target_triple, CN_COMPILE_MODE_HOSTED);
    if (!ir_module) {
        fprintf(stderr, "  ✗ IR 生成失败\n");
        result = 1;
        goto cleanup;
    }

    printf("  ✓ 测试通过\n\n");
    cn_ir_module_free(ir_module);

cleanup:
    if (program) {
        cn_frontend_ast_program_free(program);
    }
    if (parser) {
        cn_frontend_parser_free(parser);
    }
cleanup_early:
    cn_support_diagnostics_free(&diagnostics);
    free(code);

    return result;
}

int main(void)
{
    int failed = 0;

    printf("=== REPL 小段程序集成测试 ===\n\n");

    // 测试变量声明
    failed += test_statement("整数变量声明", "整数 x = 10;");
    failed += test_statement("类型推断变量声明", "变量 y = 42;");

    // 测试函数调用
    failed += test_statement("打印函数调用", "打印(\"Hello\");");
    failed += test_statement("打印整数", "打印(100);");

    // 测试多条语句（通过多行输入模拟）
    failed += test_statement(
        "变量声明和使用",
        "变量 a = 5; 变量 b = 10; 打印(a + b);"
    );

    failed += test_statement(
        "连续赋值",
        "整数 x = 1; x = x + 2; 打印(x);"
    );

    // 总结
    printf("=================================\n");
    if (failed == 0) {
        printf("所有测试通过！\n");
    } else {
        printf("失败测试数: %d\n", failed);
    }

    return (failed > 0) ? 1 : 0;
}
