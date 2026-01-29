/**
 * @file package_module_import_test.c
 * @brief 测试包导入与模块导入的识别功能
 * 
 * 验证：
 * 1. `导入 ./包名;` 被解析为包导入 (target_type = CN_IMPORT_TARGET_PACKAGE)
 * 2. `导入 模块名;` 被解析为模块导入 (target_type = CN_IMPORT_TARGET_MODULE)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/support/diagnostics.h"

#define TEST_START(name) printf("测试: %s ... ", name)
#define TEST_PASS() printf("通过\n")
#define TEST_FAIL(msg) do { printf("失败: %s\n", msg); failures++; } while(0)

static int failures = 0;

// 解析源代码
static CnAstProgram *parse_source(const char *source, CnDiagnostics *diag) {
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, diag);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, diag);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    cn_frontend_parser_free(parser);
    
    return program;
}

// 测试：包导入语法 (导入 ./包名;)
static void test_package_import_syntax(void) {
    TEST_START("包导入语法解析 (导入 ./包名;)");
    
    const char *source = 
        "导入 ./工具;\n"
        "函数 主程序() -> 整数 {\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);
    
    CnAstProgram *program = parse_source(source, &diag);
    
    if (!program) {
        TEST_FAIL("解析失败");
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    if (program->import_count < 1) {
        TEST_FAIL("没有找到导入语句");
        cn_frontend_ast_program_free(program);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    CnAstStmt *import_stmt = program->imports[0];
    if (import_stmt->kind != CN_AST_STMT_IMPORT) {
        TEST_FAIL("语句类型不是导入");
        cn_frontend_ast_program_free(program);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    CnAstImportStmt *import = &import_stmt->as.import_stmt;
    
    // 验证：应该是包导入 (target_type = CN_IMPORT_TARGET_PACKAGE = 1)
    if (import->target_type != CN_IMPORT_TARGET_PACKAGE) {
        TEST_FAIL("target_type 应该是 CN_IMPORT_TARGET_PACKAGE");
        cn_frontend_ast_program_free(program);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    // 验证：模块路径应该是相对路径
    if (!import->module_path || !import->module_path->is_relative) {
        TEST_FAIL("应该是相对路径导入");
        cn_frontend_ast_program_free(program);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    TEST_PASS();
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diag);
}

// 测试：模块导入语法 (导入 模块名;)
static void test_module_import_syntax(void) {
    TEST_START("模块导入语法解析 (导入 模块名;)");
    
    const char *source = 
        "导入 数学模块;\n"
        "函数 主程序() -> 整数 {\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);
    
    CnAstProgram *program = parse_source(source, &diag);
    
    if (!program) {
        TEST_FAIL("解析失败");
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    if (program->import_count < 1) {
        TEST_FAIL("没有找到导入语句");
        cn_frontend_ast_program_free(program);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    CnAstStmt *import_stmt = program->imports[0];
    if (import_stmt->kind != CN_AST_STMT_IMPORT) {
        TEST_FAIL("语句类型不是导入");
        cn_frontend_ast_program_free(program);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    CnAstImportStmt *import = &import_stmt->as.import_stmt;
    
    // 验证：应该是模块导入 (target_type = CN_IMPORT_TARGET_MODULE = 0)
    if (import->target_type != CN_IMPORT_TARGET_MODULE) {
        TEST_FAIL("target_type 应该是 CN_IMPORT_TARGET_MODULE");
        cn_frontend_ast_program_free(program);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    // 验证：应该不是路径导入，而是传统模块名导入
    if (import->module_path != NULL) {
        TEST_FAIL("传统模块导入不应该有 module_path");
        cn_frontend_ast_program_free(program);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    TEST_PASS();
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diag);
}

// 测试：从包导入语法 (从 ./包名 导入 { 成员 };)
static void test_from_package_import_syntax(void) {
    TEST_START("从包导入语法解析 (从 ./包名 导入 { 成员 };)");
    
    const char *source = 
        "从 ./工具 导入 { 加法 };\n"
        "函数 主程序() -> 整数 {\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);
    
    CnAstProgram *program = parse_source(source, &diag);
    
    if (!program) {
        TEST_FAIL("解析失败");
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    if (program->import_count < 1) {
        TEST_FAIL("没有找到导入语句");
        cn_frontend_ast_program_free(program);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    CnAstStmt *import_stmt = program->imports[0];
    CnAstImportStmt *import = &import_stmt->as.import_stmt;
    
    // 验证：应该是包导入 (target_type = CN_IMPORT_TARGET_PACKAGE = 1)
    if (import->target_type != CN_IMPORT_TARGET_PACKAGE) {
        TEST_FAIL("target_type 应该是 CN_IMPORT_TARGET_PACKAGE");
        cn_frontend_ast_program_free(program);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    // 验证：使用从...导入语法
    if (!import->use_from_syntax) {
        TEST_FAIL("应该使用 从...导入 语法");
        cn_frontend_ast_program_free(program);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    TEST_PASS();
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diag);
}

// 测试：从模块导入语法 (从 模块名 导入 { 成员 };)
static void test_from_module_import_syntax(void) {
    TEST_START("从模块导入语法解析 (从 模块名 导入 { 成员 };)");
    
    const char *source = 
        "从 数学 导入 { 加法 };\n"
        "函数 主程序() -> 整数 {\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);
    
    CnAstProgram *program = parse_source(source, &diag);
    
    if (!program) {
        TEST_FAIL("解析失败");
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    if (program->import_count < 1) {
        TEST_FAIL("没有找到导入语句");
        cn_frontend_ast_program_free(program);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    CnAstStmt *import_stmt = program->imports[0];
    CnAstImportStmt *import = &import_stmt->as.import_stmt;
    
    // 验证：应该是模块导入 (target_type = CN_IMPORT_TARGET_MODULE = 0)
    if (import->target_type != CN_IMPORT_TARGET_MODULE) {
        TEST_FAIL("target_type 应该是 CN_IMPORT_TARGET_MODULE");
        cn_frontend_ast_program_free(program);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    // 验证：使用从...导入语法
    if (!import->use_from_syntax) {
        TEST_FAIL("应该使用 从...导入 语法");
        cn_frontend_ast_program_free(program);
        cn_support_diagnostics_free(&diag);
        return;
    }
    
    TEST_PASS();
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diag);
}

int main(void) {
    printf("\n============================================\n");
    printf("包导入与模块导入识别功能测试\n");
    printf("============================================\n\n");
    
    test_package_import_syntax();
    test_module_import_syntax();
    test_from_package_import_syntax();
    test_from_module_import_syntax();
    
    printf("\n============================================\n");
    if (failures == 0) {
        printf("所有测试通过!\n");
    } else {
        printf("失败测试数: %d\n", failures);
    }
    printf("============================================\n");
    
    return failures;
}
