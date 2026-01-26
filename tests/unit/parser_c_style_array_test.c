#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

// 测试C风格数组声明：整数 arr[5] = {1, 2, 3, 4, 5};
static void test_c_style_array_with_size(void) {
    const char *source = "函数 测试() { 整数 arr[5] = {1, 2, 3, 4, 5}; 返回 0; }";
    
    printf("测试: C风格数组声明（指定大小）\n");
    
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
    assert(stmt->as.var_decl.name_length == 3);
    assert(strncmp(stmt->as.var_decl.name, "arr", 3) == 0);
    
    // 检查类型是数组类型
    assert(stmt->as.var_decl.declared_type != NULL);
    assert(stmt->as.var_decl.declared_type->kind == CN_TYPE_ARRAY);
    assert(stmt->as.var_decl.declared_type->as.array.length == 5);
    assert(stmt->as.var_decl.declared_type->as.array.element_type->kind == CN_TYPE_INT);
    
    // 检查初始化表达式是数组字面量
    assert(stmt->as.var_decl.initializer != NULL);
    assert(stmt->as.var_decl.initializer->kind == CN_AST_EXPR_ARRAY_LITERAL);
    assert(stmt->as.var_decl.initializer->as.array_literal.element_count == 5);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ C风格数组声明（指定大小）解析正确\n");
}

// 测试C风格数组声明：变量 arr[] = {1, 2, 3};
static void test_c_style_array_without_size(void) {
    const char *source = "函数 测试() { 变量 arr[] = {1, 2, 3}; 返回 0; }";
    
    printf("测试: C风格数组声明（自动推断大小）\n");
    
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
    assert(func->body->stmt_count == 2);
    
    // 检查变量声明
    CnAstStmt *stmt = func->body->stmts[0];
    assert(stmt->kind == CN_AST_STMT_VAR_DECL);
    assert(stmt->as.var_decl.name_length == 3);
    assert(strncmp(stmt->as.var_decl.name, "arr", 3) == 0);
    
    // 检查类型是数组类型（大小为0表示自动推断）
    assert(stmt->as.var_decl.declared_type != NULL);
    assert(stmt->as.var_decl.declared_type->kind == CN_TYPE_ARRAY);
    assert(stmt->as.var_decl.declared_type->as.array.length == 0);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ C风格数组声明（自动推断大小）解析正确\n");
}

// 测试C风格数组部分初始化：整数 arr[5] = {1, 2};
static void test_c_style_array_partial_init(void) {
    const char *source = "函数 测试() { 整数 arr[5] = {1, 2}; 返回 0; }";
    
    printf("测试: C风格数组部分初始化\n");
    
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
    
    CnAstFunctionDecl *func = program->functions[0];
    CnAstStmt *stmt = func->body->stmts[0];
    
    // 检查数组大小为5，但只有2个元素初始化
    assert(stmt->as.var_decl.declared_type->as.array.length == 5);
    assert(stmt->as.var_decl.initializer->as.array_literal.element_count == 2);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ C风格数组部分初始化解析正确\n");
}

// 测试不同基础类型的C风格数组
static void test_c_style_array_different_types(void) {
    const char *source = 
        "函数 测试() {\n"
        "    小数 floats[3] = {1.0, 2.0, 3.0};\n"
        "    字符串 strs[2] = {\"hello\", \"world\"};\n"
        "    返回 0;\n"
        "}";
    
    printf("测试: 不同类型的C风格数组\n");
    
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
    
    CnAstFunctionDecl *func = program->functions[0];
    
    // 检查第一个数组（小数类型）
    CnAstStmt *stmt1 = func->body->stmts[0];
    assert(stmt1->as.var_decl.declared_type->as.array.element_type->kind == CN_TYPE_FLOAT);
    assert(stmt1->as.var_decl.declared_type->as.array.length == 3);
    
    // 检查第二个数组（字符串类型）
    CnAstStmt *stmt2 = func->body->stmts[1];
    assert(stmt2->as.var_decl.declared_type->as.array.element_type->kind == CN_TYPE_STRING);
    assert(stmt2->as.var_decl.declared_type->as.array.length == 2);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 不同类型的C风格数组解析正确\n");
}

// 测试空的C风格数组初始化
static void test_c_style_empty_array_init(void) {
    const char *source = "函数 测试() { 整数 arr[100] = {0}; 返回 0; }";
    
    printf("测试: C风格空数组初始化\n");
    
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
    
    CnAstFunctionDecl *func = program->functions[0];
    CnAstStmt *stmt = func->body->stmts[0];
    
    // 检查数组大小为100，但只有1个元素（0）初始化
    assert(stmt->as.var_decl.declared_type->as.array.length == 100);
    assert(stmt->as.var_decl.initializer->as.array_literal.element_count == 1);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ C风格空数组初始化解析正确\n");
}

int main(void) {
    printf("=== C风格数组语法测试 ===\n");
    
    test_c_style_array_with_size();
    test_c_style_array_without_size();
    test_c_style_array_partial_init();
    test_c_style_array_different_types();
    test_c_style_empty_array_init();
    
    printf("parser_c_style_array_test: 所有测试通过\n");
    return 0;
}
