#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// 测试函数指针声明解析
static void test_function_pointer_declaration(void) {
    const char *source = 
        "函数 主程序() {\n"
        "    整数(*回调)(整数, 整数);\n"
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 函数指针声明解析\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success);
    assert(program != NULL);
    assert(program->function_count == 1);
    
    // 检查函数体中的语句
    CnAstFunctionDecl *fn = program->functions[0];
    assert(fn->body != NULL);
    assert(fn->body->stmt_count == 2); // 一个变量声明 + 一个返回语句
    
    // 检查第一条语句是变量声明
    CnAstStmt *stmt = fn->body->stmts[0];
    assert(stmt->kind == CN_AST_STMT_VAR_DECL);
    
    // 检查变量类型是函数指针类型
    CnType *var_type = stmt->as.var_decl.declared_type;
    assert(var_type != NULL);
    assert(var_type->kind == CN_TYPE_POINTER); // 函数指针是指针类型
    assert(var_type->as.pointer_to != NULL);
    assert(var_type->as.pointer_to->kind == CN_TYPE_FUNCTION); // 指向函数类型
    
    // 检查函数类型的返回类型和参数
    CnType *func_type = var_type->as.pointer_to;
    assert(func_type->as.function.return_type->kind == CN_TYPE_INT);
    assert(func_type->as.function.param_count == 2);
    assert(func_type->as.function.param_types[0]->kind == CN_TYPE_INT);
    assert(func_type->as.function.param_types[1]->kind == CN_TYPE_INT);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 函数指针声明解析测试通过\n");
}

// 测试函数指针赋值
static void test_function_pointer_assignment(void) {
    const char *source = 
        "函数 加法(整数 a, 整数 b) { 返回 a + b; }\n"
        "函数 主程序() {\n"
        "    整数(*运算)(整数, 整数);\n"
        "    运算 = 加法;\n"
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 函数指针赋值解析\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success);
    assert(program != NULL);
    assert(program->function_count == 2); // 加法 + 主程序
    
    // 检查主程序函数体
    CnAstFunctionDecl *main_fn = program->functions[1];
    assert(main_fn->body != NULL);
    assert(main_fn->body->stmt_count == 3); // 变量声明 + 赋值 + 返回
    
    // 检查赋值语句
    CnAstStmt *assign_stmt = main_fn->body->stmts[1];
    assert(assign_stmt->kind == CN_AST_STMT_EXPR);
    assert(assign_stmt->as.expr.expr->kind == CN_AST_EXPR_ASSIGN);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 函数指针赋值解析测试通过\n");
}

// 测试带初始化的函数指针声明
static void test_function_pointer_with_initializer(void) {
    const char *source = 
        "函数 乘法(整数 a, 整数 b) { 返回 a * b; }\n"
        "函数 主程序() {\n"
        "    整数(*运算)(整数, 整数) = 乘法;\n"
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 带初始化的函数指针声明解析\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success);
    assert(program != NULL);
    assert(program->function_count == 2);
    
    // 检查主程序函数体
    CnAstFunctionDecl *main_fn = program->functions[1];
    assert(main_fn->body != NULL);
    assert(main_fn->body->stmt_count == 2); // 变量声明(带初始化) + 返回
    
    // 检查变量声明有初始化器
    CnAstStmt *decl_stmt = main_fn->body->stmts[0];
    assert(decl_stmt->kind == CN_AST_STMT_VAR_DECL);
    assert(decl_stmt->as.var_decl.initializer != NULL);
    assert(decl_stmt->as.var_decl.initializer->kind == CN_AST_EXPR_IDENTIFIER);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 带初始化的函数指针声明解析测试通过\n");
}

// 测试省略返回类型的函数声明解析
static void test_function_decl_omitted_return_type(void) {
    const char *source = 
        "函数 加法(整数 左, 整数 右) { 返回 左 + 右; }\n"
        "函数 主程序() {\n"
        "    整数 结果 = 加法(1, 2);\n"
        "    返回 0;\n"
        "}\n";

    printf("测试: 省略返回类型的函数声明解析\n");

    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);

    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);

    assert(success);
    assert(program != NULL);
    assert(program->function_count == 2);

    CnAstFunctionDecl *add_fn = program->functions[0];
    assert(add_fn != NULL);
    assert(add_fn->parameter_count == 2);

    CnAstFunctionDecl *main_fn = program->functions[1];
    assert(main_fn != NULL);
    assert(main_fn->parameter_count == 0);

    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);

    printf("  ✓ 省略返回类型的函数声明解析测试通过\n");
}

int main(void) {
    printf("=== 函数指针Parser单元测试 ===\n");
    test_function_pointer_declaration();
    test_function_pointer_assignment();
    test_function_pointer_with_initializer();
    test_function_decl_omitted_return_type();
    printf("parser_function_pointer_test: 测试完成\n");
    return 0;
}
