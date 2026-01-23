#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <string.h>

// 测试数组类型推导 - 整数数组
static void test_array_type_inference_int(void) {
    const char *source = "函数 测试() { 变量 arr = [1, 2, 3]; 返回 0; }";
    
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
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("测试整数数组类型推导: 失败 (有语义错误)\n");
    } else if (program && program->function_count > 0) {
        CnAstFunctionDecl *fn = program->functions[0];
        if (fn->body && fn->body->stmt_count > 0) {
            CnAstStmt *stmt = fn->body->stmts[0];
            if (stmt->kind == CN_AST_STMT_VAR_DECL) {
                CnAstExpr *init = stmt->as.var_decl.initializer;
                if (init && init->type && init->type->kind == CN_TYPE_ARRAY) {
                    if (init->type->as.array.element_type->kind == CN_TYPE_INT &&
                        init->type->as.array.length == 3) {
                        printf("测试整数数组类型推导: 成功\n");
                    } else {
                        printf("测试整数数组类型推导: 失败 (类型不正确)\n");
                    }
                } else {
                    printf("测试整数数组类型推导: 失败 (不是数组类型)\n");
                }
            } else {
                printf("测试整数数组类型推导: 失败 (不是变量声明)\n");
            }
        } else {
            printf("测试整数数组类型推导: 失败 (函数体为空)\n");
        }
    } else {
        printf("测试整数数组类型推导: 失败 (没有函数)\n");
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试空数组类型推导
static void test_empty_array_type(void) {
    const char *source = "函数 测试() { 变量 arr = []; 返回 0; }";
    
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
        printf("测试空数组类型推导: 失败 (有语义错误)\n");
    } else if (program && program->function_count > 0) {
        CnAstFunctionDecl *fn = program->functions[0];
        if (fn->body && fn->body->stmt_count > 0) {
            CnAstStmt *stmt = fn->body->stmts[0];
            if (stmt->kind == CN_AST_STMT_VAR_DECL) {
                CnAstExpr *init = stmt->as.var_decl.initializer;
                if (init && init->type && init->type->kind == CN_TYPE_ARRAY) {
                    // 空数组默认为 int 数组，长度为 0
                    if (init->type->as.array.length == 0) {
                        printf("测试空数组类型推导: 成功\n");
                    } else {
                        printf("测试空数组类型推导: 失败 (长度不为0)\n");
                    }
                } else {
                    printf("测试空数组类型推导: 失败 (不是数组类型)\n");
                }
            } else {
                printf("测试空数组类型推导: 失败 (不是变量声明)\n");
            }
        } else {
            printf("测试空数组类型推导: 失败 (函数体为空)\n");
        }
    } else {
        printf("测试空数组类型推导: 失败 (没有函数)\n");
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试数组元素类型不一致错误检测
static void test_array_mixed_types_error(void) {
    const char *source = "函数 测试() { 变量 arr = [1, \"hello\"]; 返回 0; }";
    
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
    
    // 应该检测到类型不一致错误
    int found_error = 0;
    for (size_t i = 0; i < diagnostics.count; i++) {
        if (diagnostics.items[i].code == CN_DIAG_CODE_SEM_TYPE_MISMATCH) {
            found_error = 1;
            break;
        }
    }
    
    if (found_error) {
        printf("测试混合类型数组错误检测: 成功 (检测到类型不一致)\n");
    } else {
        printf("测试混合类型数组错误检测: 失败 (未检测到错误)\n");
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试数组类型兼容性
static void test_array_type_compatibility(void) {
    const char *source = "函数 测试() { 变量 arr1 = [1, 2]; 变量 arr2 = [3, 4]; 返回 0; }";
    
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
        printf("测试数组类型兼容性: 失败 (有语义错误)\n");
    } else if (program && program->function_count > 0) {
        CnAstFunctionDecl *fn = program->functions[0];
        if (fn->body && fn->body->stmt_count >= 2) {
            CnAstStmt *stmt1 = fn->body->stmts[0];
            CnAstStmt *stmt2 = fn->body->stmts[1];
            if (stmt1->kind == CN_AST_STMT_VAR_DECL && 
                stmt2->kind == CN_AST_STMT_VAR_DECL) {
                CnType *type1 = stmt1->as.var_decl.initializer->type;
                CnType *type2 = stmt2->as.var_decl.initializer->type;
                
                // 两个都应该是数组类型
                if (type1 && type1->kind == CN_TYPE_ARRAY &&
                    type2 && type2->kind == CN_TYPE_ARRAY) {
                    // 元素类型应该兼容（都是int）
                    if (cn_type_compatible(type1->as.array.element_type, 
                                          type2->as.array.element_type)) {
                        printf("测试数组类型兼容性: 成功\n");
                    } else {
                        printf("测试数组类型兼容性: 失败 (元素类型不兼容)\n");
                    }
                } else {
                    printf("测试数组类型兼容性: 失败 (不是数组类型)\n");
                }
            } else {
                printf("测试数组类型兼容性: 失败 (不是变量声明)\n");
            }
        } else {
            printf("测试数组类型兼容性: 失败 (语句数量不足)\n");
        }
    } else {
        printf("测试数组类型兼容性: 失败 (没有函数)\n");
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

int main(void) {
    printf("=== 数组语义分析单元测试 ===\n");
    test_array_type_inference_int();
    test_empty_array_type();
    test_array_mixed_types_error();
    test_array_type_compatibility();
    printf("semantics_array_test: 测试完成\n");
    return 0;
}
