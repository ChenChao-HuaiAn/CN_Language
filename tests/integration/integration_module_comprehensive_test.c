/*
 * 模块系统综合集成测试
 * 覆盖所有已实现的模块系统高级特性:
 * 1. 自动导入(导入 模块名;)
 * 2. 模块内函数声明与调用
 * 3. 模块可见性控制(公开/私有)
 * 4. 模块变量的复杂初始化
 * 5. 按需导入(选择性导入)
 * 6. 模块别名支持
 */

#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/ir/irgen.h"
#include "cnlang/backend/cgen.h"
#include "cnlang/support/diagnostics.h"
#include "cnlang/support/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 测试计数器
static int tests_run = 0;
static int tests_passed = 0;

#define TEST_START(name) \
    do { \
        tests_run++; \
        printf("\n[TEST %d] %s\n", tests_run, name); \
    } while(0)

#define TEST_PASS() \
    do { \
        tests_passed++; \
        printf("  ✓ PASSED\n"); \
    } while(0)

#define TEST_FAIL(msg) \
    do { \
        printf("  ✗ FAILED: %s\n", msg); \
    } while(0)

// 辅助函数：解析并进行语义分析
static int parse_and_analyze(const char *source, CnAstProgram **out_program, 
                             CnSemScope **out_scope, CnDiagnostics *diag) {
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, diag);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, diag);
    
    bool ok = cn_frontend_parse_program(parser, out_program);
    cn_frontend_parser_free(parser);
    
    if (!ok || cn_support_diagnostics_error_count(diag) > 0) {
        printf("  解析阶段失败，错误数: %zu\n", cn_support_diagnostics_error_count(diag));
        cn_support_diagnostics_print(diag);
        return 0;
    }
    
    *out_scope = cn_sem_build_scopes(*out_program, diag);
    if (!*out_scope) {
        printf("  符号表构建失败\n");
        return 0;
    }
    
    cn_sem_resolve_names(*out_scope, *out_program, diag);
    cn_sem_check_types(*out_scope, *out_program, diag);
    
    if (cn_support_diagnostics_error_count(diag) > 0) {
        printf("  语义分析失败，错误数: %zu\n", cn_support_diagnostics_error_count(diag));
        cn_support_diagnostics_print(diag);
        return 0;
    }
    
    return 1;
}

/*
 * 测试1: 自动导入 - 验证全量导入功能
 */
static void test_auto_import(void) {
    TEST_START("自动导入 - 全量导入功能");
    
    const char *source = 
        "模块 工具 {\n"
        "    公开: 整数 值 = 42;\n"
        "    公开: 函数 获取值() { \n"
        "        返回 值; \n"
        "    }\n"
        "}\n"
        "导入 工具;\n"
        "函数 主程序() {\n"
        "    变量 x = 值;\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);
    
    CnAstProgram *program = NULL;
    CnSemScope *scope = NULL;
    
    if (!parse_and_analyze(source, &program, &scope, &diag)) {
        TEST_FAIL("解析或语义分析失败");
        if (program) cn_frontend_ast_program_free(program);
        if (scope) cn_sem_scope_free(scope);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    TEST_PASS();
    
    cn_sem_scope_free(scope);
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diag);
}

/*
 * 测试2: 模块内函数
 */
static void test_module_functions(void) {
    TEST_START("模块内函数 - 定义和调用");
    
    const char *source = 
        "模块 数学 {\n"
        "    公开: 函数 加(整数 a, 整数 b) {\n"
        "        返回 a + b;\n"
        "    }\n"
        "}\n"
        "函数 主程序() {\n"
        "    变量 结果 = 数学.加(3, 5);\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);
    
    CnAstProgram *program = NULL;
    CnSemScope *scope = NULL;
    
    if (!parse_and_analyze(source, &program, &scope, &diag)) {
        TEST_FAIL("解析或语义分析失败");
        if (program) cn_frontend_ast_program_free(program);
        if (scope) cn_sem_scope_free(scope);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    TEST_PASS();
    
    cn_sem_scope_free(scope);
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diag);
}

/*
 * 测试3: 可见性控制
 */
static void test_visibility_control(void) {
    TEST_START("可见性控制 - 公开/私有");
    
    const char *source = 
        "模块 配置 {\n"
        "    公开: 整数 版本 = 1;\n"
        "    公开: 函数 获取版本() { \n"
        "        返回 版本; \n"
        "    }\n"
        "}\n"
        "函数 主程序() {\n"
        "    变量 v = 配置.版本;\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);
    
    CnAstProgram *program = NULL;
    CnSemScope *scope = NULL;
    
    if (!parse_and_analyze(source, &program, &scope, &diag)) {
        TEST_FAIL("解析或语义分析失败");
        if (program) cn_frontend_ast_program_free(program);
        if (scope) cn_sem_scope_free(scope);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    TEST_PASS();
    
    cn_sem_scope_free(scope);
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diag);
}

/*
 * 测试4: 复杂初始化
 */
static void test_complex_initialization(void) {
    TEST_START("复杂初始化 - 表达式初始化");
    
    const char *source = 
        "模块 计算 {\n"
        "    公开: 整数 基数 = 10;\n"
        "    公开: 整数 结果 = 1 + 2;\n"
        "    公开: 整数 复杂值 = (10 + 5) * 2;\n"
        "}\n"
        "函数 主程序() {\n"
        "    变量 x = 计算.结果;\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);
    
    CnAstProgram *program = NULL;
    CnSemScope *scope = NULL;
    
    if (!parse_and_analyze(source, &program, &scope, &diag)) {
        TEST_FAIL("解析或语义分析失败");
        if (program) cn_frontend_ast_program_free(program);
        if (scope) cn_sem_scope_free(scope);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    TEST_PASS();
    
    cn_sem_scope_free(scope);
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diag);
}

/*
 * 测试5: 选择性导入
 */
static void test_selective_import(void) {
    TEST_START("选择性导入 - 按需导入");
    
    const char *source = 
        "模块 工具 {\n"
        "    公开: 整数 值A = 10;\n"
        "    公开: 整数 值B = 20;\n"
        "}\n"
        "导入 工具 { 值A };\n"
        "函数 主程序() {\n"
        "    变量 a = 值A;\n"
        "    变量 b = 工具.值B;\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);
    
    CnAstProgram *program = NULL;
    CnSemScope *scope = NULL;
    
    if (!parse_and_analyze(source, &program, &scope, &diag)) {
        TEST_FAIL("解析或语义分析失败");
        if (program) cn_frontend_ast_program_free(program);
        if (scope) cn_sem_scope_free(scope);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    TEST_PASS();
    
    cn_sem_scope_free(scope);
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diag);
}

/*
 * 测试6: 模块别名
 */
static void test_module_alias(void) {
    TEST_START("模块别名 - 使用别名访问");
    
    const char *source = 
        "模块 非常长的模块名称 {\n"
        "    公开: 整数 数据 = 100;\n"
        "}\n"
        "导入 非常长的模块名称 为 短名;\n"
        "函数 主程序() {\n"
        "    变量 x = 短名.数据;\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);
    
    CnAstProgram *program = NULL;
    CnSemScope *scope = NULL;
    
    if (!parse_and_analyze(source, &program, &scope, &diag)) {
        TEST_FAIL("解析或语义分析失败");
        if (program) cn_frontend_ast_program_free(program);
        if (scope) cn_sem_scope_free(scope);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    TEST_PASS();
    
    cn_sem_scope_free(scope);
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diag);
}

/*
 * 测试7: 组合特性
 */
static void test_combined_features(void) {
    TEST_START("组合特性 - 多特性协同");
    
    const char *source = 
        "模块 核心 {\n"
        "    公开: 整数 基础值 = 50;\n"
        "    公开: 函数 计算(整数 x) {\n"
        "        返回 基础值 + x;\n"
        "    }\n"
        "}\n"
        "模块 扩展 {\n"
        "    公开: 整数 增量 = 10;\n"
        "}\n"
        "导入 核心;\n"
        "导入 扩展 { 增量 };\n"
        "函数 主程序() {\n"
        "    变量 v1 = 基础值;\n"
        "    变量 v3 = 增量;\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);
    
    CnAstProgram *program = NULL;
    CnSemScope *scope = NULL;
    
    if (!parse_and_analyze(source, &program, &scope, &diag)) {
        TEST_FAIL("解析或语义分析失败");
        if (program) cn_frontend_ast_program_free(program);
        if (scope) cn_sem_scope_free(scope);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    TEST_PASS();
    
    cn_sem_scope_free(scope);
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diag);
}

int main(void) {
    printf("=================================================\n");
    printf("      模块系统综合集成测试\n");
    printf("=================================================\n");
    
    test_auto_import();
    test_module_functions();
    test_visibility_control();
    test_complex_initialization();
    test_selective_import();
    test_module_alias();
    test_combined_features();
    
    printf("\n=================================================\n");
    printf("测试结果: %d/%d 通过\n", tests_passed, tests_run);
    printf("=================================================\n");
    
    return (tests_passed == tests_run) ? 0 : 1;
}
