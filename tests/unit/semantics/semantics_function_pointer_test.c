#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// 测试函数指针类型检查
static void test_function_pointer_type_check(void) {
    const char *source = 
        "函数 加法(整数 a, 整数 b) { 返回 a + b; }\n"
        "函数 主程序() {\n"
        "    整数(*运算)(整数, 整数) = 加法;\n"
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 函数指针类型检查\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    // 应该没有语义错误
    if (cn_support_diagnostics_error_count(&diagnostics) == 0) {
        printf("  ✓ 函数指针类型检查测试通过\n");
    } else {
        printf("  ✗ 函数指针类型检查测试失败\n");
        cn_support_diagnostics_print(&diagnostics);
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试函数指针调用类型检查
static void test_function_pointer_call_type_check(void) {
    const char *source = 
        "函数 加法(整数 a, 整数 b) { 返回 a + b; }\n"
        "函数 主程序() {\n"
        "    整数(*运算)(整数, 整数) = 加法;\n"
        "    变量 结果 = 运算(1, 2);\n"
        "    返回 结果;\n"
        "}\n";
    
    printf("测试: 函数指针调用类型检查\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    // 应该没有语义错误
    if (cn_support_diagnostics_error_count(&diagnostics) == 0) {
        printf("  ✓ 函数指针调用类型检查测试通过\n");
    } else {
        printf("  ✗ 函数指针调用类型检查测试失败\n");
        cn_support_diagnostics_print(&diagnostics);
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试函数指针参数类型不匹配检测
static void test_function_pointer_param_mismatch(void) {
    const char *source = 
        "函数 加法(整数 a, 整数 b) { 返回 a + b; }\n"
        "函数 主程序() {\n"
        "    整数(*运算)(整数, 整数) = 加法;\n"
        "    变量 结果 = 运算(1);\n"  // 参数个数不匹配
        "    返回 结果;\n"
        "}\n";
    
    printf("测试: 函数指针参数个数不匹配检测\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    // 应该检测到参数个数不匹配错误
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✓ 函数指针参数个数不匹配检测测试通过\n");
    } else {
        printf("  ✗ 函数指针参数个数不匹配检测测试失败（未检测到错误）\n");
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

int main(void) {
    printf("=== 函数指针语义检查单元测试 ===\n");
    test_function_pointer_type_check();
    test_function_pointer_call_type_check();
    test_function_pointer_param_mismatch();
    printf("semantics_function_pointer_test: 测试完成\n");
    return 0;
}
