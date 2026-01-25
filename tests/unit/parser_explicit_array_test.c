#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

// 测试显式数组变量声明：数组 arr = [1, 2, 3]
static void test_explicit_array_variable_decl(void) {
    const char *source = "函数 测试() { 数组 arr = [1, 2, 3]; 返回 0; }";
    
    printf("测试: 显式数组变量声明\n");
    
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
    
    printf("  ✓ 显式数组变量声明解析正确\n");
}

// 测试带元素类型的显式数组声明：数组 整数 arr = [1, 2, 3]
static void test_explicit_array_with_element_type(void) {
    const char *source = "函数 测试() { 数组 整数 arr = [1, 2, 3]; 返回 0; }";
    
    printf("测试: 带元素类型的显式数组声明\n");
    
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
    
    printf("  ✓ 带元素类型的显式数组声明解析正确\n");
}

// 测试函数参数中的数组类型：函数 测试(数组 整数 arr) {}
static void test_array_function_parameter(void) {
    const char *source = "函数 测试(数组 整数 arr) { 返回 0; }";
    
    printf("测试: 函数参数中的数组类型\n");
    
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
    assert(func->parameter_count == 1);
    
    CnAstParameter *param = &func->parameters[0];
    assert(strncmp(param->name, "arr", 3) == 0);
    assert(param->declared_type != NULL);
    assert(param->declared_type->kind == CN_TYPE_ARRAY);
    assert(param->declared_type->as.array.element_type != NULL);
    assert(param->declared_type->as.array.element_type->kind == CN_TYPE_INT);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 函数参数中的数组类型解析正确\n");
}

// 测试结构体字段中的数组类型：结构体 测试 { 数组 整数 数据; }
static void test_array_struct_field(void) {
    const char *source = "结构体 测试 { 数组 整数 数据; 整数 长度; }";
    
    printf("测试: 结构体字段中的数组类型\n");
    
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
    assert(program->struct_count == 1);
    
    CnAstStmt *struct_stmt = program->structs[0];
    assert(struct_stmt->kind == CN_AST_STMT_STRUCT_DECL);
    
    CnAstStructDecl *struct_decl = &struct_stmt->as.struct_decl;
    assert(struct_decl->field_count == 2);
    
    // 检查第一个字段（数组 整数 数据）
    CnAstStructField *field = &struct_decl->fields[0];
    assert(strncmp(field->name, "数据", strlen("数据")) == 0);
    assert(field->field_type != NULL);
    assert(field->field_type->kind == CN_TYPE_ARRAY);
    assert(field->field_type->as.array.element_type != NULL);
    assert(field->field_type->as.array.element_type->kind == CN_TYPE_INT);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 结构体字段中的数组类型解析正确\n");
}

// 测试字符串数组：数组 字符串 strs = ["a", "b", "c"]
static void test_string_array(void) {
    const char *source = "函数 测试() { 数组 字符串 strs = [\"一\", \"二\", \"三\"]; 返回 0; }";
    
    printf("测试: 字符串数组\n");
    
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
    assert(stmt->kind == CN_AST_STMT_VAR_DECL);
    
    CnAstVarDecl *decl = &stmt->as.var_decl;
    assert(decl->declared_type != NULL);
    assert(decl->declared_type->kind == CN_TYPE_ARRAY);
    assert(decl->declared_type->as.array.element_type->kind == CN_TYPE_STRING);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 字符串数组解析正确\n");
}

int main(void) {
    printf("=== 显式数组声明语法测试 ===\n");
    
    test_explicit_array_variable_decl();
    test_explicit_array_with_element_type();
    test_array_function_parameter();
    test_array_struct_field();
    test_string_array();
    
    printf("parser_explicit_array_test: 所有测试通过\n");
    return 0;
}
