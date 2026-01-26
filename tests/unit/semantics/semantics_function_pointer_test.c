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

// 测试省略返回类型的函数声明语义：通过返回语句推断返回类型
static void test_function_decl_omitted_return_type_semantics(void) {
    const char *source = 
        "函数 加法(整数 左, 整数 右) { 返回 左 + 右; }\n"
        "函数 求平均值(小数 总和, 整数 数量) { 返回 总和 / 数量; }\n"
        "函数 主程序() {\n"
        "    整数 a = 加法(1, 2);\n"
        "    小数 b = 求平均值(3.0, 2);\n"
        "    返回 0;\n"
        "}\n";

    printf("测试: 省略返回类型的函数声明语义检查\n");

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

    // 检查函数返回类型已经根据 return 语句推断出来
    CnSemSymbol *add_sym = cn_sem_scope_lookup(global_scope, "加法", strlen("加法"));
    CnSemSymbol *avg_sym = cn_sem_scope_lookup(global_scope, "求平均值", strlen("求平均值"));

    assert(add_sym != NULL);
    assert(add_sym->kind == CN_SEM_SYMBOL_FUNCTION);
    assert(add_sym->type != NULL);
    assert(add_sym->type->kind == CN_TYPE_FUNCTION);
    assert(add_sym->type->as.function.return_type != NULL);
    assert(add_sym->type->as.function.return_type->kind == CN_TYPE_INT);

    assert(avg_sym != NULL);
    assert(avg_sym->kind == CN_SEM_SYMBOL_FUNCTION);
    assert(avg_sym->type != NULL);
    assert(avg_sym->type->kind == CN_TYPE_FUNCTION);
    assert(avg_sym->type->as.function.return_type != NULL);
    assert(avg_sym->type->as.function.return_type->kind == CN_TYPE_FLOAT);

    if (cn_support_diagnostics_error_count(&diagnostics) == 0) {
        printf("  ✓ 省略返回类型的函数声明语义检查测试通过\n");
    } else {
        printf("  ✗ 省略返回类型的函数声明语义检查测试失败\n");
        cn_support_diagnostics_print(&diagnostics);
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
    test_function_decl_omitted_return_type_semantics();
    printf("semantics_function_pointer_test: 测试完成\n");
    return 0;
}
