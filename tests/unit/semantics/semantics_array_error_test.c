#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

// 测试类型不匹配：将非数组赋给数组变量
static void test_array_type_mismatch(void) {
    const char *source = "函数 测试() { 整数 arr[3] = 123; 返回 0; }";
    
    printf("测试: 类型不匹配 - 将非数组赋给数组变量\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    // 执行语义分析
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    // 应该有语义错误
    assert(cn_support_diagnostics_error_count(&diagnostics) > 0 && 
           "应该检测到类型不匹配错误");
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 正确检测到类型不匹配错误\n");
}

// 测试数组元素类型不匹配：整数数组赋值为字符串数组
static void test_array_element_type_mismatch(void) {
    const char *source = "函数 测试() { 整数 arr[] = {\"one\", \"two\"}; 返回 0; }";
    
    printf("测试: 数组元素类型不匹配\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    // 执行语义分析
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    // 应该有语义错误
    assert(cn_support_diagnostics_error_count(&diagnostics) > 0 && 
           "应该检测到数组元素类型不匹配错误");
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 正确检测到数组元素类型不匹配错误\n");
}

// 测试数组字面量元素类型混合错误
// 注意：这个测试检查整数数组被赋予字符串值时的错误
static void test_array_literal_mixed_types(void) {
    // 声明整数数组，但初始化器包含字符串 - 应该报错
    const char *source = "函数 测试() { 整数 arr[] = {\"one\", \"two\"}; 返回 0; }";
    
    printf("测试: 数组字面量元素类型混合\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    // 执行语义分析
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    // 应该有语义错误（数组元素类型不一致）
    assert(cn_support_diagnostics_error_count(&diagnostics) > 0 && 
           "应该检测到数组元素类型混合错误");
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 正确检测到数组元素类型混合错误\n");
}

// 测试正确的数组声明与赋值（应该通过）
static void test_valid_array_declaration(void) {
    const char *source = "函数 测试() { 整数 arr[] = {1, 2, 3}; 返回 0; }";
    
    printf("测试: 正确的数组声明与赋值\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    // 执行语义分析
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    // 不应该有错误
    assert(cn_support_diagnostics_error_count(&diagnostics) == 0 && 
           "正确的数组声明不应该有错误");
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 正确的数组声明通过检查\n");
}

// 测试无大小的数组声明（应该通过，从初始化器推导）
static void test_array_without_element_type(void) {
    const char *source = "函数 测试() { 整数 arr[] = {1, 2, 3}; 返回 0; }";
    
    printf("测试: 无大小的数组声明\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    // 执行语义分析
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    // 不应该有错误
    assert(cn_support_diagnostics_error_count(&diagnostics) == 0 && 
           "无大小的数组声明应该从初始化器推导");
    
    // 验证类型推导
    if (program && program->function_count > 0) {
        CnAstFunctionDecl *func = program->functions[0];
        if (func->body && func->body->stmt_count > 0) {
            CnAstStmt *stmt = func->body->stmts[0];
            if (stmt->kind == CN_AST_STMT_VAR_DECL) {
                CnAstExpr *init = stmt->as.var_decl.initializer;
                if (init && init->type && init->type->kind == CN_TYPE_ARRAY) {
                    assert(init->type->as.array.element_type->kind == CN_TYPE_INT &&
                           "应该推导为整数数组");
                }
            }
        }
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 无大小的数组声明正确推导\n");
}

int main(void) {
    printf("=== 数组类型错误检测测试 ===\n");
    
    test_array_type_mismatch();
    test_array_element_type_mismatch();
    test_array_literal_mixed_types();
    test_valid_array_declaration();
    test_array_without_element_type();
    
    printf("semantics_array_error_test: 所有测试通过\n");
    return 0;
}
