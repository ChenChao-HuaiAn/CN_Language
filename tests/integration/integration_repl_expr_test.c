/*
 * integration_repl_expr_test.c
 * REPL 单表达式求值场景集成测试
 * 
 * 测试场景：
 * - 算术表达式求值
 * - 字符串字面量
 * - 逻辑表达式
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

/* 辅助函数：包装表达式为完整程序 */
static char *wrap_expression_as_program(const char *expr)
{
    size_t expr_len = strlen(expr);
    size_t wrapper_size = 256 + expr_len;
    char *wrapped = (char *)malloc(wrapper_size);

    if (!wrapped) {
        return NULL;
    }

    snprintf(wrapped, wrapper_size,
             "函数 主程序() { 打印(%s); 返回 0; }",
             expr);

    return wrapped;
}

/* 辅助函数：测试单个表达式是否能成功解析和编译 */
static int test_expression(const char *expr_description, const char *expr)
{
    printf("测试: %s\n", expr_description);
    printf("  表达式: %s\n", expr);

    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    CnTargetTriple target_triple;
    char *code = NULL;
    int result = 0;

    // 包装表达式
    code = wrap_expression_as_program(expr);
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
    
    // 语义分析
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    if (!global_scope) {
        fprintf(stderr, "  ✗ 构建作用域失败\n");
        result = 1;
        goto cleanup;
    }

    // 生成 IR
    target_triple = cn_support_target_triple_make(
        CN_TARGET_ARCH_X86_64,
        CN_TARGET_VENDOR_PC,
        CN_TARGET_OS_NONE,
        CN_TARGET_ABI_ELF);

    CnIrModule *ir_module = cn_ir_gen_program(program, global_scope, target_triple, CN_COMPILE_MODE_HOSTED);
    if (!ir_module) {
        fprintf(stderr, "  ✗ IR 生成失败\n");
        result = 1;
        goto cleanup;
    }

    printf("  ✓ 测试通过\n\n");
    cn_ir_module_free(ir_module);
    cn_sem_scope_free(global_scope);

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

    printf("=== REPL 单表达式求值集成测试 ===\n\n");

    // 测试算术表达式
    failed += test_expression("简单加法", "1 + 2");
    failed += test_expression("复杂算术表达式", "10 + 20 * 3 - 5");
    failed += test_expression("带括号的表达式", "(5 + 3) * 2");
    failed += test_expression("除法表达式", "100 / 5");
    failed += test_expression("取模运算", "17 % 5");
    failed += test_expression("负数运算", "-10 + 5");

    // 测试字符串字面量
    failed += test_expression("字符串字面量", "\"你好，世界！\"");
    failed += test_expression("空字符串", "\"\"");
    
    // 测试整数字面量
    failed += test_expression("整数字面量", "42");
    failed += test_expression("大整数", "999999");
    failed += test_expression("零", "0");

    // 测试比较表达式
    failed += test_expression("大于比较", "10 > 5");
    failed += test_expression("小于比较", "3 < 7");
    failed += test_expression("等于比较", "3 == 3");
    failed += test_expression("不等于比较", "5 != 3");
    failed += test_expression("大于等于", "10 >= 10");
    failed += test_expression("小于等于", "5 <= 10");

    // 测试逻辑表达式
    failed += test_expression("逻辑与", "1 && 1");
    failed += test_expression("逻辑或", "0 || 1");
    failed += test_expression("复杂逻辑表达式", "(10 > 5) && (3 < 7)");

    // 测试混合表达式
    failed += test_expression("算术与比较混合", "(1 + 2) > (3 - 1)");
    failed += test_expression("复杂嵌套表达式", "((10 + 5) * 2) / 3");

    // 总结
    printf("=================================\n");
    if (failed == 0) {
        printf("所有测试通过！\n");
    } else {
        printf("失败测试数: %d\n", failed);
    }

    return (failed > 0) ? 1 : 0;
}
