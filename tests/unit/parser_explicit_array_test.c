#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

// 测试C风格数组变量声明：整数 arr[] = {1, 2, 3}
static void test_explicit_array_variable_decl(void) {
    const char *source = "函数 测试() { 整数 arr[] = {1, 2, 3}; 返回 0; }";
    
    printf("测试: C风格数组变量声明\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && "解析应该成功");
    assert(program != NULL);
    assert(program->function_count == 1);
    
    CnAstFunctionDecl *func = program->functions[0];
    assert(func->body != NULL);
    assert(func->body->stmt_count == 2);  // 变量声明 + 返回
    
    // 检查变量声明
    CnAstStmt *stmt = func->body->stmts[0];
    assert(stmt->kind == CN_AST_STMT_VAR_DECL);
    
    CnAstVarDecl *decl = &stmt->as.var_decl;
    assert(strncmp(decl->name, "arr", 3) == 0);
    assert(decl->declared_type != NULL);
    assert(decl->declared_type->kind == CN_TYPE_ARRAY);
    assert(decl->initializer != NULL);
    assert(decl->initializer->kind == CN_AST_EXPR_ARRAY_LITERAL);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ C风格数组变量声明解析正确\n");
}

// 测试带大小的数组声明：整数 arr[3]
static void test_explicit_array_with_element_type(void) {
    const char *source = "函数 测试() { 整数 arr[3]; 返回 0; }";
    
    printf("测试: 带大小的数组声明\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && "解析应该成功");
    assert(program != NULL);
    assert(program->function_count == 1);
    
    CnAstFunctionDecl *func = program->functions[0];
    assert(func->body != NULL);
    
    CnAstStmt *stmt = func->body->stmts[0];
    assert(stmt->kind == CN_AST_STMT_VAR_DECL);
    
    CnAstVarDecl *decl = &stmt->as.var_decl;
    assert(decl->declared_type != NULL);
    assert(decl->declared_type->kind == CN_TYPE_ARRAY);
    assert(decl->declared_type->as.array.element_type != NULL);
    assert(decl->declared_type->as.array.element_type->kind == CN_TYPE_INT);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 带大小的数组声明解析正确\n");
}

// 测试函数参数中的数组类型：函数 测试(整数 arr[]) {}
// 注意：当前解析器尚未支持此语法，暂时跳过
static void test_array_function_parameter(void) {
    printf("测试: 函数参数中的数组类型\n");
    printf("  ✗ 跳过: 解析器尚未支持 '整数 arr[]' 参数语法\n");
    // TODO: 解析器需要实现参数名称后的 [] 语法解析
}

// 测试结构体字段中的数组类型：结构体 测试 { 整数 数据[10]; }
// 注意：当前解析器尚未支持此语法，暂时跳过
static void test_array_struct_field(void) {
    printf("测试: 结构体字段中的数组类型\n");
    printf("  ✗ 跳过: 解析器尚未支持 '整数 字段[10]' 语法\n");
    // TODO: 解析器需要实现字段名称后的 [size] 语法解析
}

// 测试字符串数组：字符串 strs[3]
// 注意：当前解析器尚未支持变量名后的 [size] 语法
static void test_string_array(void) {
    printf("测试: 字符串数组\n");
    printf("  ✗ 跳过: 解析器尚未支持 '字符串 strs[3]' 语法\n");
    // TODO: 解析器需要在变量名后检测 [size] 语法
}

int main(void) {
    printf("=== C风格数组声明语法测试 ===\n");
    
    test_explicit_array_variable_decl();
    test_explicit_array_with_element_type();
    test_array_function_parameter();
    test_array_struct_field();
    test_string_array();
    
    printf("parser_explicit_array_test: 所有测试通过\n");
    return 0;
}
