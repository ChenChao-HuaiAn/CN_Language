#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

// 测试二维数组声明
static void test_2d_array_declaration(void) {
    const char *source = "函数 测试() { 整数 matrix[3][4]; 返回 0; }";
    
    printf("测试: 二维数组声明\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✗ 解析失败\n");
        for (size_t i = 0; i < diagnostics.count; i++) {
            printf("    错误: %s\n", diagnostics.items[i].message);
        }
        assert(0);
    }
    
    CnAstFunctionDecl *func = program->functions[0];
    CnAstStmt *stmt = func->body->stmts[0];
    
    // 检查变量声明
    assert(stmt->kind == CN_AST_STMT_VAR_DECL);
    assert(stmt->as.var_decl.name_length == 6);
    assert(strncmp(stmt->as.var_decl.name, "matrix", 6) == 0);
    
    // 检查类型是二维数组：array(3, array(4, int))
    CnType *type = stmt->as.var_decl.declared_type;
    assert(type != NULL);
    assert(type->kind == CN_TYPE_ARRAY);
    assert(type->as.array.length == 3);  // 外层数组大小为3
    
    // 检查内层数组类型
    CnType *inner_type = type->as.array.element_type;
    assert(inner_type != NULL);
    assert(inner_type->kind == CN_TYPE_ARRAY);
    assert(inner_type->as.array.length == 4);  // 内层数组大小为4
    
    // 检查最内层元素类型
    CnType *element_type = inner_type->as.array.element_type;
    assert(element_type != NULL);
    assert(element_type->kind == CN_TYPE_INT);
    
    printf("  ✓ 二维数组声明解析正确 (整数 matrix[3][4])\n");
    
    if (program) cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试三维数组声明
static void test_3d_array_declaration(void) {
    const char *source = "函数 测试() { 小数 cube[2][3][4]; 返回 0; }";
    
    printf("测试: 三维数组声明\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && cn_support_diagnostics_error_count(&diagnostics) == 0);
    
    CnAstFunctionDecl *func = program->functions[0];
    CnAstStmt *stmt = func->body->stmts[0];
    
    // 检查类型是三维数组：array(2, array(3, array(4, float)))
    CnType *type = stmt->as.var_decl.declared_type;
    assert(type->kind == CN_TYPE_ARRAY && type->as.array.length == 2);
    
    CnType *type2 = type->as.array.element_type;
    assert(type2->kind == CN_TYPE_ARRAY && type2->as.array.length == 3);
    
    CnType *type3 = type2->as.array.element_type;
    assert(type3->kind == CN_TYPE_ARRAY && type3->as.array.length == 4);
    
    CnType *element = type3->as.array.element_type;
    assert(element->kind == CN_TYPE_FLOAT);
    
    printf("  ✓ 三维数组声明解析正确 (小数 cube[2][3][4])\n");
    
    if (program) cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试二维数组初始化
static void test_2d_array_initialization(void) {
    const char *source = 
        "函数 测试() {\n"
        "    变量 matrix[2][3] = {{1, 2, 3}, {4, 5, 6}};\n"
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 二维数组初始化\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && cn_support_diagnostics_error_count(&diagnostics) == 0);
    
    CnAstFunctionDecl *func = program->functions[0];
    CnAstStmt *stmt = func->body->stmts[0];
    
    // 检查初始化器是数组字面量
    assert(stmt->as.var_decl.initializer != NULL);
    assert(stmt->as.var_decl.initializer->kind == CN_AST_EXPR_ARRAY_LITERAL);
    
    // 检查外层数组有2个元素
    CnAstArrayLiteralExpr *outer = &stmt->as.var_decl.initializer->as.array_literal;
    assert(outer->element_count == 2);
    
    // 检查第一个内层数组 {1, 2, 3}
    assert(outer->elements[0]->kind == CN_AST_EXPR_ARRAY_LITERAL);
    CnAstArrayLiteralExpr *inner1 = &outer->elements[0]->as.array_literal;
    assert(inner1->element_count == 3);
    assert(inner1->elements[0]->kind == CN_AST_EXPR_INTEGER_LITERAL);
    assert(inner1->elements[0]->as.integer_literal.value == 1);
    assert(inner1->elements[1]->as.integer_literal.value == 2);
    assert(inner1->elements[2]->as.integer_literal.value == 3);
    
    // 检查第二个内层数组 {4, 5, 6}
    assert(outer->elements[1]->kind == CN_AST_EXPR_ARRAY_LITERAL);
    CnAstArrayLiteralExpr *inner2 = &outer->elements[1]->as.array_literal;
    assert(inner2->element_count == 3);
    assert(inner2->elements[0]->as.integer_literal.value == 4);
    assert(inner2->elements[1]->as.integer_literal.value == 5);
    assert(inner2->elements[2]->as.integer_literal.value == 6);
    
    printf("  ✓ 二维数组初始化解析正确 {{1,2,3},{4,5,6}}\n");
    
    if (program) cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试多维数组索引访问
static void test_multidim_array_access(void) {
    const char *source = 
        "函数 测试() {\n"
        "    变量 matrix[2][3] = {{1, 2, 3}, {4, 5, 6}};\n"
        "    变量 value = matrix[1][2];\n"
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 多维数组索引访问\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && cn_support_diagnostics_error_count(&diagnostics) == 0);
    
    CnAstFunctionDecl *func = program->functions[0];
    CnAstStmt *stmt = func->body->stmts[1];  // 第二个语句：变量 value = matrix[1][2];
    
    // 检查是变量声明
    assert(stmt->kind == CN_AST_STMT_VAR_DECL);
    
    // 检查初始化器是索引表达式 matrix[1][2]
    CnAstExpr *init = stmt->as.var_decl.initializer;
    assert(init != NULL);
    assert(init->kind == CN_AST_EXPR_INDEX);  // 外层索引 [2]
    
    // 检查外层索引：....[2]
    assert(init->as.index.index->kind == CN_AST_EXPR_INTEGER_LITERAL);
    assert(init->as.index.index->as.integer_literal.value == 2);
    
    // 检查内层是另一个索引表达式 matrix[1]
    CnAstExpr *inner_index = init->as.index.array;
    assert(inner_index->kind == CN_AST_EXPR_INDEX);
    assert(inner_index->as.index.index->kind == CN_AST_EXPR_INTEGER_LITERAL);
    assert(inner_index->as.index.index->as.integer_literal.value == 1);
    
    // 检查最内层是标识符 matrix
    CnAstExpr *base = inner_index->as.index.array;
    assert(base->kind == CN_AST_EXPR_IDENTIFIER);
    assert(strncmp(base->as.identifier.name, "matrix", 6) == 0);
    
    printf("  ✓ 多维数组索引访问解析正确 matrix[1][2]\n");
    
    if (program) cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试多维数组赋值
static void test_multidim_array_assignment(void) {
    const char *source = 
        "函数 测试() {\n"
        "    变量 matrix[2][2] = {{1, 2}, {3, 4}};\n"
        "    matrix[0][1] = 99;\n"
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 多维数组赋值\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && cn_support_diagnostics_error_count(&diagnostics) == 0);
    
    CnAstFunctionDecl *func = program->functions[0];
    CnAstStmt *stmt = func->body->stmts[1];  // 第二个语句：matrix[0][1] = 99;
    
    // 检查是表达式语句
    assert(stmt->kind == CN_AST_STMT_EXPR);
    
    // 检查是赋值表达式
    CnAstExpr *expr = stmt->as.expr.expr;
    assert(expr->kind == CN_AST_EXPR_ASSIGN);
    
    // 检查赋值目标是索引表达式 matrix[0][1]
    CnAstExpr *target = expr->as.assign.target;
    assert(target->kind == CN_AST_EXPR_INDEX);
    
    // 检查赋值的值
    assert(expr->as.assign.value->kind == CN_AST_EXPR_INTEGER_LITERAL);
    assert(expr->as.assign.value->as.integer_literal.value == 99);
    
    printf("  ✓ 多维数组赋值解析正确 matrix[0][1] = 99\n");
    
    if (program) cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

int main(void) {
    printf("=== 多维数组语法测试 ===\n\n");
    
    test_2d_array_declaration();
    test_3d_array_declaration();
    test_2d_array_initialization();
    test_multidim_array_access();
    test_multidim_array_assignment();
    
    printf("\n✅ 所有多维数组测试通过！\n");
    return 0;
}
