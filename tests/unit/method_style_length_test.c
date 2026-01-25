/**
 * 方法风格长度调用测试
 * 测试 arr.长度() 和 str.长度() 方法风格调用
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include "cnlang/ir/irgen.h"
#include "cnlang/backend/cgen.h"
#include "cnlang/support/config.h"

// 测试数组的方法风格长度调用
static void test_array_method_style_length(void) {
    const char *source = 
        "函数 主程序() {\n"
        "    数组 arr = [1, 2, 3, 4, 5];\n"
        "    变量 len = arr.长度();\n"
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 数组方法风格长度调用 (arr.长度())\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✗ 解析阶段失败\n");
        cn_support_diagnostics_print(&diagnostics);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return;
    }
    
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✗ 语义分析失败\n");
        cn_support_diagnostics_print(&diagnostics);
        cn_sem_scope_free(global_scope);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return;
    }
    
    printf("  ✓ 语义分析通过\n");
    
    CnTargetTriple target = cn_support_target_triple_make(
        CN_TARGET_ARCH_X86_64,
        CN_TARGET_VENDOR_PC,
        CN_TARGET_OS_NONE,
        CN_TARGET_ABI_ELF);
    CnIrModule *ir_module = cn_ir_gen_program(program, global_scope, target, CN_COMPILE_MODE_HOSTED);
    
    if (!ir_module) {
        printf("  ✗ IR 生成失败\n");
        cn_sem_scope_free(global_scope);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return;
    }
    
    printf("  ✓ IR 生成成功\n");
    
    cn_ir_module_free(ir_module);
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试字符串的方法风格长度调用
static void test_string_method_style_length(void) {
    const char *source = 
        "函数 主程序() {\n"
        "    变量 str = \"hello\";\n"
        "    变量 len = str.长度();\n"
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 字符串方法风格长度调用 (str.长度())\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✗ 解析阶段失败\n");
        cn_support_diagnostics_print(&diagnostics);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return;
    }
    
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✗ 语义分析失败\n");
        cn_support_diagnostics_print(&diagnostics);
        cn_sem_scope_free(global_scope);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return;
    }
    
    printf("  ✓ 语义分析通过\n");
    
    CnTargetTriple target = cn_support_target_triple_make(
        CN_TARGET_ARCH_X86_64,
        CN_TARGET_VENDOR_PC,
        CN_TARGET_OS_NONE,
        CN_TARGET_ABI_ELF);
    CnIrModule *ir_module = cn_ir_gen_program(program, global_scope, target, CN_COMPILE_MODE_HOSTED);
    
    if (!ir_module) {
        printf("  ✗ IR 生成失败\n");
        cn_sem_scope_free(global_scope);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return;
    }
    
    printf("  ✓ IR 生成成功\n");
    
    cn_ir_module_free(ir_module);
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试方法风格调用不接受参数
static void test_method_style_no_params(void) {
    const char *source = 
        "函数 主程序() {\n"
        "    数组 arr = [1, 2, 3];\n"
        "    变量 len = arr.长度(10);\n"  // 错误：方法风格不接受参数
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 方法风格长度调用不接受参数 (负面测试)\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✗ 解析阶段失败\n");
        cn_support_diagnostics_print(&diagnostics);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return;
    }
    
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    // 应该检测到错误
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✓ 正确检测到错误（方法风格调用不应接受参数）\n");
    } else {
        printf("  ✗ 未能检测到错误\n");
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试方法风格调用用于非字符串/数组类型
static void test_method_style_invalid_type(void) {
    const char *source = 
        "函数 主程序() {\n"
        "    变量 x = 42;\n"
        "    变量 len = x.长度();\n"  // 错误：整数不支持长度方法
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 方法风格长度调用用于非字符串/数组类型 (负面测试)\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✗ 解析阶段失败\n");
        cn_support_diagnostics_print(&diagnostics);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return;
    }
    
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    // 应该检测到类型错误
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✓ 正确检测到类型错误\n");
    } else {
        printf("  ✗ 未能检测到类型错误\n");
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试函数风格仍然有效
static void test_function_style_still_works(void) {
    const char *source = 
        "函数 主程序() {\n"
        "    数组 arr = [1, 2, 3];\n"
        "    变量 len = 长度(arr);\n"  // 函数风格调用
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 函数风格长度调用仍然有效\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✗ 解析阶段失败\n");
        cn_support_diagnostics_print(&diagnostics);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return;
    }
    
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✗ 语义分析失败\n");
        cn_support_diagnostics_print(&diagnostics);
        cn_sem_scope_free(global_scope);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return;
    }
    
    printf("  ✓ 函数风格长度调用仍然正常工作\n");
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

int main(void) {
    printf("\n=== 方法风格长度调用测试套件 ===\n\n");
    
    test_array_method_style_length();
    printf("\n");
    
    test_string_method_style_length();
    printf("\n");
    
    test_method_style_no_params();
    printf("\n");
    
    test_method_style_invalid_type();
    printf("\n");
    
    test_function_style_still_works();
    printf("\n");
    
    printf("=== 测试完成 ===\n");
    return 0;
}
