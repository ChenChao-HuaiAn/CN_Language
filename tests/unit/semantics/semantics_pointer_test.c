#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// 测试指针声明与类型推导
static void test_pointer_declaration(void) {
    const char *source = "函数 测试() { 整数 x = 42; 整数* p = &x; 返回 0; }";
    
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
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("测试指针声明: 失败 (有语义错误)\n");
        cn_support_diagnostics_print(&diagnostics);
    } else if (program && program->function_count > 0) {
        CnAstFunctionDecl *fn = program->functions[0];
        if (fn->body && fn->body->stmt_count >= 2) {
            CnAstStmt *stmt2 = fn->body->stmts[1];
            if (stmt2->kind == CN_AST_STMT_VAR_DECL) {
                CnType *decl_type = stmt2->as.var_decl.declared_type;
                if (decl_type && decl_type->kind == CN_TYPE_POINTER &&
                    decl_type->as.pointer_to->kind == CN_TYPE_INT) {
                    printf("测试指针声明: 成功\n");
                } else {
                    printf("测试指针声明: 失败 (类型不正确)\n");
                }
            } else {
                printf("测试指针声明: 失败 (不是变量声明)\n");
            }
        } else {
            printf("测试指针声明: 失败 (函数体语句不足)\n");
        }
    } else {
        printf("测试指针声明: 失败 (没有函数)\n");
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试取地址运算符 &
static void test_address_of_operator(void) {
    const char *source = "函数 测试() { 整数 x = 10; 变量 p = &x; 返回 0; }";
    
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
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("测试取地址运算符: 失败 (有语义错误)\n");
        cn_support_diagnostics_print(&diagnostics);
    } else if (program && program->function_count > 0) {
        CnAstFunctionDecl *fn = program->functions[0];
        if (fn->body && fn->body->stmt_count >= 2) {
            CnAstStmt *stmt2 = fn->body->stmts[1];
            if (stmt2->kind == CN_AST_STMT_VAR_DECL) {
                CnAstExpr *init = stmt2->as.var_decl.initializer;
                if (init && init->type && init->type->kind == CN_TYPE_POINTER &&
                    init->type->as.pointer_to->kind == CN_TYPE_INT) {
                    printf("测试取地址运算符: 成功\n");
                } else {
                    printf("测试取地址运算符: 失败 (类型不正确)\n");
                }
            } else {
                printf("测试取地址运算符: 失败 (不是变量声明)\n");
            }
        } else {
            printf("测试取地址运算符: 失败 (函数体语句不足)\n");
        }
    } else {
        printf("测试取地址运算符: 失败 (没有函数)\n");
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试解引用运算符 *
static void test_dereference_operator(void) {
    const char *source = "函数 测试() { 整数 x = 5; 整数* p = &x; 变量 y = *p; 返回 0; }";
    
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
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("测试解引用运算符: 失败 (有语义错误)\n");
        cn_support_diagnostics_print(&diagnostics);
    } else if (program && program->function_count > 0) {
        CnAstFunctionDecl *fn = program->functions[0];
        if (fn->body && fn->body->stmt_count >= 3) {
            CnAstStmt *stmt3 = fn->body->stmts[2];
            if (stmt3->kind == CN_AST_STMT_VAR_DECL) {
                CnAstExpr *init = stmt3->as.var_decl.initializer;
                if (init && init->type && init->type->kind == CN_TYPE_INT) {
                    printf("测试解引用运算符: 成功\n");
                } else {
                    printf("测试解引用运算符: 失败 (类型不正确)\n");
                }
            } else {
                printf("测试解引用运算符: 失败 (不是变量声明)\n");
            }
        } else {
            printf("测试解引用运算符: 失败 (函数体语句不足)\n");
        }
    } else {
        printf("测试解引用运算符: 失败 (没有函数)\n");
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试指针算术运算
static void test_pointer_arithmetic(void) {
    const char *source = "函数 测试() { 整数 arr = 0; 整数* p = &arr; 整数* q = p + 1; 返回 0; }";
    
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
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("测试指针算术运算: 失败 (有语义错误)\n");
        cn_support_diagnostics_print(&diagnostics);
    } else if (program && program->function_count > 0) {
        CnAstFunctionDecl *fn = program->functions[0];
        if (fn->body && fn->body->stmt_count >= 3) {
            CnAstStmt *stmt3 = fn->body->stmts[2];
            if (stmt3->kind == CN_AST_STMT_VAR_DECL) {
                CnType *decl_type = stmt3->as.var_decl.declared_type;
                CnAstExpr *init = stmt3->as.var_decl.initializer;
                if (init && init->type && init->type->kind == CN_TYPE_POINTER &&
                    init->type->as.pointer_to->kind == CN_TYPE_INT) {
                    printf("测试指针算术运算: 成功\n");
                } else {
                    printf("测试指针算术运算: 失败 (类型不正确)\n");
                }
            } else {
                printf("测试指针算术运算: 失败 (不是变量声明)\n");
            }
        } else {
            printf("测试指针算术运算: 失败 (函数体语句不足)\n");
        }
    } else {
        printf("测试指针算术运算: 失败 (没有函数)\n");
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

int main(void) {
    printf("=== 指针语义分析单元测试 ===\n");
    test_pointer_declaration();
    test_address_of_operator();
    test_dereference_operator();
    test_pointer_arithmetic();
    printf("semantics_pointer_test: 测试完成\n");
    return 0;
}
