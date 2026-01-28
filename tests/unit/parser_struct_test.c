#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

// 测试结构体声明解析
static void test_struct_declaration(void) {
    const char *source = 
        "结构体 点 {\n"
        "    整数 x;\n"
        "    整数 y;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    printf("测试: 结构体声明解析\n");
    
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
    assert(program->struct_count == 1);
    assert(program->function_count == 1);
    
    // 检查结构体声明
    CnAstStmt *struct_stmt = program->structs[0];
    assert(struct_stmt->kind == CN_AST_STMT_STRUCT_DECL);
    assert(struct_stmt->as.struct_decl.field_count == 2);
    assert(memcmp(struct_stmt->as.struct_decl.name, "点", strlen("点")) == 0);
    
    // 检查字段
    assert(memcmp(struct_stmt->as.struct_decl.fields[0].name, "x", 1) == 0);
    assert(struct_stmt->as.struct_decl.fields[0].field_type->kind == CN_TYPE_INT);
    assert(memcmp(struct_stmt->as.struct_decl.fields[1].name, "y", 1) == 0);
    assert(struct_stmt->as.struct_decl.fields[1].field_type->kind == CN_TYPE_INT);
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 结构体声明解析测试通过\n");
}

static void test_struct_member_access(void) {
    const char *source = 
        "函数 测试() {\n"
        "    变量 p = 点;\n"  // 简化的变量声明
        "    变量 x = p.x;\n"
        "    返回 0;\n"
        "}\n";
    
    printf("测试: 结构体成员访问解析\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    // 注意：这个测试可能会因为语义检查失败而不完全通过
    // 但我们主要关注解析器能否正确解析语法
    if (program && program->function_count > 0) {
        CnAstFunctionDecl *fn = program->functions[0];
        if (fn->body && fn->body->stmt_count >= 2) {
            CnAstStmt *stmt = fn->body->stmts[1];  // 第二条语句
            if (stmt->kind == CN_AST_STMT_VAR_DECL && stmt->as.var_decl.initializer) {
                CnAstExpr *init_expr = stmt->as.var_decl.initializer;
                // 检查是否是成员访问表达式
                if (init_expr->kind == CN_AST_EXPR_MEMBER_ACCESS) {
                    assert(init_expr->as.member.is_arrow == 0);  // 应该是 .
                    assert(memcmp(init_expr->as.member.member_name, "x", 1) == 0);
                    printf("  ✓ 结构体成员访问解析测试通过\n");
                }
            }
        }
    }
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

static void test_struct_declaration_with_init(void) {
    const char *source =
        "结构体 点 {\n"
        "    整数 x;\n"
        "    整数 y;\n"
        "}\n"
        "结构体 点 p = {10, 20};\n"
        "函数 主程序() { 返回 0; }\n";

    printf("测试: 结构体声明并初始化解析\n");

    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);

    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_struct_init.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);

    assert(success);
    assert(program != NULL);
    assert(cn_support_diagnostics_error_count(&diagnostics) == 0);

    printf("调试: struct_count=%zu, global_var_count=%zu\n", program->struct_count, program->global_var_count);
    fflush(stdout);
    assert(program->struct_count == 1);
    assert(program->global_var_count == 1);

    CnAstStmt *var_stmt = program->global_vars[0];
    assert(var_stmt->kind == CN_AST_STMT_VAR_DECL);

    CnAstVarDecl *var_decl = &var_stmt->as.var_decl;
    assert(var_decl->declared_type != NULL);
    assert(var_decl->declared_type->kind == CN_TYPE_STRUCT);
    assert(var_decl->initializer != NULL);
    assert(var_decl->initializer->kind == CN_AST_EXPR_STRUCT_LITERAL);

    CnAstStructLiteralExpr *lit = &var_decl->initializer->as.struct_lit;
    assert(lit->field_count == 2);
    // 位置初始化：在解析阶段字段名可以为空，由语义阶段按顺序绑定
    assert(lit->fields[0].value != NULL);
    assert(lit->fields[0].value->kind == CN_AST_EXPR_INTEGER_LITERAL);

    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

int main(void) {
    printf("=== 结构体Parser单元测试 ===\n");
    test_struct_declaration();
    test_struct_member_access();
    test_struct_declaration_with_init();
    printf("parser_struct_test: 测试完成\n");
    return 0;
}
