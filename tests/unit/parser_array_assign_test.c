#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// 测试数组索引赋值解析
static void test_array_index_assign(void) {
    const char *source = 
        "函数 主程序() {\n"
        "    变量 arr = [1, 2, 3];\n"
        "    arr[0] = 10;\n"
        "    arr[1] = 20;\n"
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 数组索引赋值解析\n");
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    assert(parser != NULL);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && "解析应该成功");
    assert(program != NULL);
    assert(program->function_count == 1);
    
    CnAstFunctionDecl *func = program->functions[0];
    assert(func->body != NULL);
    assert(func->body->stmt_count == 4);  // 变量声明 + 2个赋值 + 返回
    
    // 检查第一个赋值语句 arr[0] = 10
    CnAstStmt *stmt1 = func->body->stmts[1];
    assert(stmt1->kind == CN_AST_STMT_EXPR);
    assert(stmt1->as.expr.expr->kind == CN_AST_EXPR_ASSIGN);
    
    CnAstAssignExpr *assign1 = &stmt1->as.expr.expr->as.assign;
    assert(assign1->target->kind == CN_AST_EXPR_INDEX);
    assert(assign1->value->kind == CN_AST_EXPR_INTEGER_LITERAL);
    assert(assign1->value->as.integer_literal.value == 10);
    
    // 检查索引表达式
    CnAstIndexExpr *index1 = &assign1->target->as.index;
    assert(index1->array->kind == CN_AST_EXPR_IDENTIFIER);
    assert(strncmp(index1->array->as.identifier.name, "arr", 3) == 0);
    assert(index1->index->kind == CN_AST_EXPR_INTEGER_LITERAL);
    assert(index1->index->as.integer_literal.value == 0);
    
    // 检查第二个赋值语句 arr[1] = 20
    CnAstStmt *stmt2 = func->body->stmts[2];
    assert(stmt2->kind == CN_AST_STMT_EXPR);
    assert(stmt2->as.expr.expr->kind == CN_AST_EXPR_ASSIGN);
    
    CnAstAssignExpr *assign2 = &stmt2->as.expr.expr->as.assign;
    assert(assign2->target->kind == CN_AST_EXPR_INDEX);
    assert(assign2->value->as.integer_literal.value == 20);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    
    printf("  ✓ 数组索引赋值解析正确\n");
}

// 测试多维数组索引赋值
static void test_multi_index_assign(void) {
    const char *source = 
        "函数 测试() {\n"
        "    变量 arr = [1, 2, 3];\n"
        "    变量 i = 1;\n"
        "    arr[i] = arr[0] + 5;\n"
        "}\n";
    
    printf("测试: 表达式作为索引和值\n");
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success);
    assert(program->function_count == 1);
    
    CnAstFunctionDecl *func = program->functions[0];
    CnAstStmt *stmt = func->body->stmts[2];  // arr[i] = arr[0] + 5
    
    assert(stmt->kind == CN_AST_STMT_EXPR);
    assert(stmt->as.expr.expr->kind == CN_AST_EXPR_ASSIGN);
    
    CnAstAssignExpr *assign = &stmt->as.expr.expr->as.assign;
    
    // 左值：arr[i]
    assert(assign->target->kind == CN_AST_EXPR_INDEX);
    assert(assign->target->as.index.index->kind == CN_AST_EXPR_IDENTIFIER);
    
    // 右值：arr[0] + 5
    assert(assign->value->kind == CN_AST_EXPR_BINARY);
    assert(assign->value->as.binary.left->kind == CN_AST_EXPR_INDEX);
    assert(assign->value->as.binary.right->kind == CN_AST_EXPR_INTEGER_LITERAL);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    
    printf("  ✓ 表达式作为索引和值解析正确\n");
}

int main(void) {
    printf("=== Parser 数组索引赋值测试 ===\n\n");
    
    test_array_index_assign();
    test_multi_index_assign();
    
    printf("\n所有测试通过!\n");
    return 0;
}
