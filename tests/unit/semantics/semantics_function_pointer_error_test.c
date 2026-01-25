// 函数指针错误场景单元测试
// 测试各种不合法的函数指针使用场景

#include <stdio.h>
#include <string.h>
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"

// 测试1: 函数类型不匹配
static void test_function_pointer_type_mismatch(void) {
    printf("测试: 函数指针类型不匹配检测\n");
    
    const char *source = 
        "函数 加法(整数 a, 整数 b) { 返回 a + b; }\n"
        "函数 主程序() {\n"
        "    整数(*回调)(整数);  // 期望1个参数\n"
        "    回调 = 加法;        // 实际2个参数,应该报错\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    // 词法分析
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_mismatch.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    // 语法分析
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    // 语义分析(应该检测到类型不匹配错误)
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✓ 函数指针类型不匹配检测测试通过(正确检测到错误)\n");
    } else {
        printf("  ✗ 函数指针类型不匹配检测测试失败(应该检测到错误但未检测到)\n");
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试2: 非函数赋值给函数指针
static void test_non_function_to_pointer(void) {
    printf("测试: 非函数赋值给函数指针检测\n");
    
    const char *source = 
        "函数 主程序() {\n"
        "    整数 x = 10;\n"
        "    整数(*回调)(整数);\n"
        "    回调 = x;  // 不能将整数赋值给函数指针\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_non_func.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✓ 非函数赋值检测测试通过(正确检测到错误)\n");
    } else {
        printf("  ✗ 非函数赋值检测测试失败(应该检测到错误但未检测到)\n");
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试3: 返回类型不匹配
static void test_return_type_mismatch(void) {
    printf("测试: 函数指针返回类型不匹配检测\n");
    
    const char *source = 
        "函数 获取整数() { 返回 42; }\n"
        "函数 获取字符串() { 返回 \"测试\"; }\n"
        "函数 主程序() {\n"
        "    整数(*回调)();  // 期望返回整数\n"
        "    回调 = 获取字符串;  // 实际返回字符串,应该报错\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_return.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    // cn_type_equals会检查返回类型,所以应该能检测到错误
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✓ 返回类型不匹配检测测试通过(正确检测到错误)\n");
    } else {
        printf("  ✗ 返回类型不匹配检测测试失败(应该检测到错误但未检测到)\n");
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试4: 未初始化的函数指针调用(语义层面无法检测,需运行时检测)
static void test_null_function_pointer_call(void) {
    printf("测试: 未初始化函数指针调用(语义检查应通过)\n");
    
    const char *source = 
        "函数 主程序() {\n"
        "    整数(*回调)(整数);\n"
        "    变量 结果 = 回调(10);  // 未初始化,但语义检查应通过\n"
        "    返回 结果;\n"
        "}\n";
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_null.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    if (cn_support_diagnostics_error_count(&diagnostics) == 0) {
        printf("  ✓ 未初始化函数指针调用测试通过(语义检查正确通过)\n");
    } else {
        printf("  ✗ 未初始化函数指针调用测试失败(语义检查不应报错)\n");
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试5: 参数类型不匹配
static void test_parameter_type_mismatch(void) {
    printf("测试: 函数指针参数类型不匹配检测\n");
    
    const char *source = 
        "函数 字符串处理(整数* p) { 返回 0; }\n"
        "函数 主程序() {\n"
        "    整数(*处理器)(整数);  // 期望整数参数\n"
        "    处理器 = 字符串处理;  // 实际是指针参数,应该报错\n"
        "    返回 0;\n"
        "}\n";
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_param.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✓ 参数类型不匹配检测测试通过(正确检测到错误)\n");
    } else {
        printf("  ✗ 参数类型不匹配检测测试失败(应该检测到错误但未检测到)\n");
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

int main(void) {
    printf("=== 函数指针错误场景单元测试 ===\n");
    
    test_function_pointer_type_mismatch();
    test_non_function_to_pointer();
    test_return_type_mismatch();
    test_null_function_pointer_call();
    test_parameter_type_mismatch();
    
    printf("\n测试完成\n");
    return 0;
}
