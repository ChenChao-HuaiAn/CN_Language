// semantics_import_test.c - 测试导入语句的自动符号导入功能
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// 辅助函数：编译并检查是否成功
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

// 测试1：基本导入功能 - 可以直接访问导入的符号
static void test_basic_import(void) {
    printf("测试1: 基本导入功能\n");
    
    const char *source = 
        "模块 数学 {\n"
        "    整数 圆周率 = 314;\n"
        "}\n"
        "导入 数学;\n"
        "函数 主程序() {\n"
        "    变量 x = 圆周率;\n"  // 直接访问，无需 数学.圆周率
        "    返回 x;\n"
        "}\n";
    
    if (compile_and_check(source, true)) {
        printf("  ✓ 通过\n");
    } else {
        printf("  ✗ 失败\n");
    }
}

// 测试2：名称冲突检测 - 导入的符号与现有符号冲突
static void test_import_name_conflict(void) {
    printf("测试2: 导入名称冲突检测\n");
    
    const char *source = 
        "模块 工具A {\n"
        "    整数 值 = 100;\n"
        "}\n"
        "模块 工具B {\n"
        "    整数 值 = 200;\n"
        "}\n"
        "导入 工具A;\n"
        "导入 工具B;\n"  // 应该报错：值 已存在
        "函数 主程序() {\n"
        "    返回 值;\n"
        "}\n";
    
    if (compile_and_check(source, false)) {
        printf("  ✓ 通过（正确检测到冲突）\n");
    } else {
        printf("  ✗ 失败（未检测到冲突）\n");
    }
}

// 测试3：导入不存在的模块
static void test_import_nonexistent_module(void) {
    printf("测试3: 导入不存在的模块\n");
    
    const char *source = 
        "导入 不存在的模块;\n"
        "函数 主程序() {\n"
        "    返回 0;\n"
        "}\n";
    
    if (compile_and_check(source, false)) {
        printf("  ✓ 通过（正确检测到模块不存在）\n");
    } else {
        printf("  ✗ 失败（未检测到模块不存在）\n");
    }
}

// 测试4：导入非模块符号
static void test_import_non_module(void) {
    printf("测试4: 导入非模块符号\n");
    
    const char *source = 
        "函数 某函数() { 返回 0; }\n"
        "导入 某函数;\n"  // 某函数不是模块
        "函数 主程序() {\n"
        "    返回 0;\n"
        "}\n";
    
    if (compile_and_check(source, false)) {
        printf("  ✓ 通过（正确检测到不是模块）\n");
    } else {
        printf("  ✗ 失败（未检测到不是模块）\n");
    }
}

// 测试5：多个符号导入
static void test_multiple_symbols_import(void) {
    printf("测试5: 导入模块的多个符号\n");
    
    const char *source = 
        "模块 常量 {\n"
        "    整数 A = 1;\n"
        "    整数 B = 2;\n"
        "    整数 C = 3;\n"
        "}\n"
        "导入 常量;\n"
        "函数 主程序() {\n"
        "    变量 x = A;\n"
        "    变量 y = B;\n"
        "    变量 z = C;\n"
        "    返回 x + y + z;\n"
        "}\n";
    
    if (compile_and_check(source, true)) {
        printf("  ✓ 通过\n");
    } else {
        printf("  ✗ 失败\n");
    }
}

// 测试6：导入后仍可使用模块前缀访问
static void test_import_with_prefix_access(void) {
    printf("测试6: 导入后仍可使用模块前缀访问\n");
    
    const char *source = 
        "模块 数学 {\n"
        "    整数 值 = 100;\n"
        "}\n"
        "导入 数学;\n"
        "函数 主程序() {\n"
        "    变量 x = 值;\n"           // 直接访问
        "    变量 y = 数学.值;\n"      // 模块前缀访问
        "    返回 x + y;\n"
        "}\n";
    
    if (compile_and_check(source, true)) {
        printf("  ✓ 通过\n");
    } else {
        printf("  ✗ 失败\n");
    }
}

int main(void) {
    printf("========================================\n");
    printf("导入语句自动符号导入功能测试\n");
    printf("========================================\n\n");
    
    test_basic_import();
    test_import_name_conflict();
    test_import_nonexistent_module();
    test_import_non_module();
    test_multiple_symbols_import();
    test_import_with_prefix_access();
    
    printf("\n========================================\n");
    printf("测试完成\n");
    printf("========================================\n");
    
    return 0;
}
