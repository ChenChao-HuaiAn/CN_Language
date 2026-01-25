// 测试选择性导入功能
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"

#include <stdio.h>
#include <string.h>

static bool compile_and_check(const char *source, bool expect_success) {
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);

    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    if (!program) {
        printf("Parse failed\n");
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return !expect_success;
    }

    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    
    bool has_errors = (cn_support_diagnostics_error_count(&diagnostics) > 0);
    
    if (has_errors) {
        printf("Semantic errors: %zu\n", cn_support_diagnostics_error_count(&diagnostics));
        for (size_t i = 0; i < diagnostics.count; i++) {
            if (diagnostics.items[i].severity == CN_DIAG_SEVERITY_ERROR) {
                printf("  %s\n", diagnostics.items[i].message);
            }
        }
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    return expect_success ? !has_errors : has_errors;
}

// 测试1：基本选择性导入
static void test_basic_selective_import(void) {
    printf("测试1: 基本选择性导入\n");
    
    const char *source = 
        "模块 工具 {\n"
        "    公开: 整数 A = 1;\n"
        "    公开: 整数 B = 2;\n"
        "    公开: 整数 C = 3;\n"
        "}\n"
        "导入 工具 { A, B };\n"
        "函数 主程序() {\n"
        "    变量 x = A;\n"
        "    变量 y = B;\n"
        "    返回 x + y;\n"
        "}\n";
    
    if (compile_and_check(source, true)) {
        printf("  ✓ 通过\n");
    } else {
        printf("  ✗ 失败\n");
    }
}

// 测试2：导入不存在的成员
static void test_import_nonexistent_member(void) {
    printf("测试2: 导入不存在的成员\n");
    
    const char *source = 
        "模块 工具 {\n"
        "    公开: 整数 A = 1;\n"
        "}\n"
        "导入 工具 { A, 不存在的成员 };\n"
        "函数 主程序() {\n"
        "    返回 0;\n"
        "}\n";
    
    if (compile_and_check(source, false)) {
        printf("  ✓ 通过（正确检测到不存在的成员）\n");
    } else {
        printf("  ✗ 失败（未检测到错误）\n");
    }
}

// 测试3：尝试导入私有成员
static void test_import_private_member(void) {
    printf("测试3: 尝试导入私有成员\n");
    
    const char *source = 
        "模块 工具 {\n"
        "    公开: 整数 公开成员 = 1;\n"
        "    私有: 整数 私有成员 = 2;\n"
        "}\n"
        "导入 工具 { 公开成员, 私有成员 };\n"
        "函数 主程序() {\n"
        "    返回 0;\n"
        "}\n";
    
    if (compile_and_check(source, false)) {
        printf("  ✓ 通过（正确检测到私有成员）\n");
    } else {
        printf("  ✗ 失败（未检测到私有成员访问错误）\n");
    }
}

// 测试4：选择性导入后仍可用模块前缀访问其他成员
static void test_selective_import_with_prefix(void) {
    printf("测试4: 选择性导入后仍可用模块前缀访问其他成员\n");
    
    const char *source = 
        "模块 工具 {\n"
        "    公开: 整数 A = 1;\n"
        "    公开: 整数 B = 2;\n"
        "}\n"
        "导入 工具 { A };\n"
        "函数 主程序() {\n"
        "    变量 x = A;\n"
        "    变量 y = 工具.B;\n"
        "    返回 x + y;\n"
        "}\n";
    
    if (compile_and_check(source, true)) {
        printf("  ✓ 通过\n");
    } else {
        printf("  ✗ 失败\n");
    }
}

// 测试5：空导入列表（全量导入对比）
static void test_full_import_vs_selective(void) {
    printf("测试5: 全量导入与选择性导入对比\n");
    
    // 全量导入
    const char *source1 = 
        "模块 工具 {\n"
        "    公开: 整数 A = 1;\n"
        "    公开: 整数 B = 2;\n"
        "}\n"
        "导入 工具;\n"
        "函数 主程序() {\n"
        "    变量 x = A;\n"
        "    变量 y = B;\n"
        "    返回 x + y;\n"
        "}\n";
    
    // 选择性导入
    const char *source2 = 
        "模块 工具 {\n"
        "    公开: 整数 A = 1;\n"
        "    公开: 整数 B = 2;\n"
        "}\n"
        "导入 工具 { A, B };\n"
        "函数 主程序() {\n"
        "    变量 x = A;\n"
        "    变量 y = B;\n"
        "    返回 x + y;\n"
        "}\n";
    
    bool test1 = compile_and_check(source1, true);
    bool test2 = compile_and_check(source2, true);
    
    if (test1 && test2) {
        printf("  ✓ 通过（全量导入和选择性导入都有效）\n");
    } else {
        printf("  ✗ 失败\n");
    }
}

// 测试6：导入函数成员
static void test_import_function_member(void) {
    printf("测试6: 导入函数成员\n");
    
    const char *source = 
        "模块 工具 {\n"
        "    公开:\n"  // 显式标记公开块
        "    函数 加法(整数 x, 整数 y) {\n"
        "        返回 x + y;\n"
        "    }\n"
        "    函数 减法(整数 x, 整数 y) {\n"
        "        返回 x - y;\n"
        "    }\n"
        "}\n"
        "导入 工具 { 加法 };\n"
        "函数 主程序() {\n"
        "    变量 result = 加法(10, 20);\n"
        "    返回 result;\n"
        "}\n";
    
    if (compile_and_check(source, true)) {
        printf("  ✓ 通过\n");
    } else {
        printf("  ✗ 失败\n");
    }
}

int main(void) {
    printf("========== 选择性导入测试 ==========\n\n");
    
    test_basic_selective_import();
    test_import_nonexistent_member();
    test_import_private_member();
    test_selective_import_with_prefix();
    test_full_import_vs_selective();
    test_import_function_member();
    
    printf("\n========== 测试完成 ==========\n");
    return 0;
}
