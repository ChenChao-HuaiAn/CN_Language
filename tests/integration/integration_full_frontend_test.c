#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_full_frontend_type_info() {
    const char *source = "函数 加法(整数 a, 整数 b) { 返回 a + b; }";
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);

    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diag);

    CnParser *parser = cn_frontend_parser_new(&lexer);
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    assert(program != NULL);

    CnSemScope *global_scope = cn_sem_build_scopes(program, &diag);
    assert(global_scope != NULL);
    bool resolve_ok = cn_sem_resolve_names(global_scope, program, &diag);
    assert(resolve_ok);
    bool check_ok = cn_sem_check_types(global_scope, program, &diag);
    assert(check_ok);

    // 检查函数声明的 AST 节点
    assert(program->function_count == 1);
    CnAstFunctionDecl *fn = program->functions[0];
    
    // 检查函数体内的返回语句表达式是否有类型
    assert(fn->body->stmt_count == 1);
    CnAstStmt *ret_stmt = fn->body->stmts[0];
    assert(ret_stmt->kind == CN_AST_STMT_RETURN);
    CnAstExpr *ret_expr = ret_stmt->as.return_stmt.expr;
    assert(ret_expr != NULL);
    assert(ret_expr->type != NULL);
    assert(ret_expr->type->kind == CN_TYPE_INT);

    // 检查加法操作的操作数
    assert(ret_expr->kind == CN_AST_EXPR_BINARY);
    assert(ret_expr->as.binary.left->type != NULL);
    assert(ret_expr->as.binary.left->type->kind == CN_TYPE_INT);
    assert(ret_expr->as.binary.right->type != NULL);
    assert(ret_expr->as.binary.right->type->kind == CN_TYPE_INT);

    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diag);
    printf("test_full_frontend_type_info: PASSED\n");
}

int main() {
    test_full_frontend_type_info();
    return 0;
}
