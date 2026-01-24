#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <string.h>

// 测试空数组解析
static void test_empty_array(void) {
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
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("测试空数组: 失败 (有解析错误)\n");
    } else if (!program || program->function_count == 0) {
        printf("测试空数组: 失败 (没有函数)\n");
    } else {
        // 检查函数体中的变量声明
        CnAstFunctionDecl *fn = program->functions[0];
        if (fn->body && fn->body->stmt_count > 0) {
            CnAstStmt *stmt = fn->body->stmts[0];
            if (stmt->kind == CN_AST_STMT_VAR_DECL) {
                CnAstVarDecl *decl = &stmt->as.var_decl;
                if (decl->initializer && decl->initializer->kind == CN_AST_EXPR_ARRAY_LITERAL) {
                    if (decl->initializer->as.array_literal.element_count == 0) {
                        printf("测试空数组: 成功\n");
                    } else {
                        printf("测试空数组: 失败 (元素数量不为0)\n");
                    }
                } else {
                    printf("测试空数组: 失败 (不是数组字面量)\n");
                }
            } else {
                printf("测试空数组: 失败 (不是变量声明)\n");
            }
        } else {
            printf("测试空数组: 失败 (函数体为空)\n");
        }
    }
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试单元素数组解析
static void test_single_element_array(void) {
    const char *source = "函数 测试() { 变量 arr = [42]; 返回 0; }";
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("测试单元素数组: 失败 (有解析错误)\n");
    } else if (!program || program->function_count == 0) {
        printf("测试单元素数组: 失败 (没有函数)\n");
    } else {
        CnAstFunctionDecl *fn = program->functions[0];
        if (fn->body && fn->body->stmt_count > 0) {
            CnAstStmt *stmt = fn->body->stmts[0];
            if (stmt->kind == CN_AST_STMT_VAR_DECL) {
                CnAstVarDecl *decl = &stmt->as.var_decl;
                if (decl->initializer && decl->initializer->kind == CN_AST_EXPR_ARRAY_LITERAL) {
                    if (decl->initializer->as.array_literal.element_count == 1) {
                        // 检查第一个元素是整数字面量
                        CnAstExpr *elem = decl->initializer->as.array_literal.elements[0];
                        if (elem->kind == CN_AST_EXPR_INTEGER_LITERAL && 
                            elem->as.integer_literal.value == 42) {
                            printf("测试单元素数组: 成功\n");
                        } else {
                            printf("测试单元素数组: 失败 (元素值不正确)\n");
                        }
                    } else {
                        printf("测试单元素数组: 失败 (元素数量不为1)\n");
                    }
                } else {
                    printf("测试单元素数组: 失败 (不是数组字面量)\n");
                }
            } else {
                printf("测试单元素数组: 失败 (不是变量声明)\n");
            }
        } else {
            printf("测试单元素数组: 失败 (函数体为空)\n");
        }
    }
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试多元素数组解析
static void test_multiple_elements_array(void) {
    const char *source = "函数 测试() { 变量 arr = [1, 2, 3, 4, 5]; 返回 0; }";
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("测试多元素数组: 失败 (有解析错误)\n");
    } else if (!program || program->function_count == 0) {
        printf("测试多元素数组: 失败 (没有函数)\n");
    } else {
        CnAstFunctionDecl *fn = program->functions[0];
        if (fn->body && fn->body->stmt_count > 0) {
            CnAstStmt *stmt = fn->body->stmts[0];
            if (stmt->kind == CN_AST_STMT_VAR_DECL) {
                CnAstVarDecl *decl = &stmt->as.var_decl;
                if (decl->initializer && decl->initializer->kind == CN_AST_EXPR_ARRAY_LITERAL) {
                    if (decl->initializer->as.array_literal.element_count == 5) {
                        // 检查所有元素
                        int all_correct = 1;
                        for (size_t i = 0; i < 5; i++) {
                            CnAstExpr *elem = decl->initializer->as.array_literal.elements[i];
                            if (elem->kind != CN_AST_EXPR_INTEGER_LITERAL || 
                                elem->as.integer_literal.value != (long)(i + 1)) {
                                all_correct = 0;
                                break;
                            }
                        }
                        if (all_correct) {
                            printf("测试多元素数组: 成功\n");
                        } else {
                            printf("测试多元素数组: 失败 (元素值不正确)\n");
                        }
                    } else {
                        printf("测试多元素数组: 失败 (元素数量不为5)\n");
                    }
                } else {
                    printf("测试多元素数组: 失败 (不是数组字面量)\n");
                }
            } else {
                printf("测试多元素数组: 失败 (不是变量声明)\n");
            }
        } else {
            printf("测试多元素数组: 失败 (函数体为空)\n");
        }
    }
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试字符串数组解析
static void test_string_array(void) {
    const char *source = "函数 测试() { 变量 arr = [\"hello\", \"world\"]; 返回 0; }";
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("测试字符串数组: 失败 (有解析错误)\n");
    } else if (!program || program->function_count == 0) {
        printf("测试字符串数组: 失败 (没有函数)\n");
    } else {
        CnAstFunctionDecl *fn = program->functions[0];
        if (fn->body && fn->body->stmt_count > 0) {
            CnAstStmt *stmt = fn->body->stmts[0];
            if (stmt->kind == CN_AST_STMT_VAR_DECL) {
                CnAstVarDecl *decl = &stmt->as.var_decl;
                if (decl->initializer && decl->initializer->kind == CN_AST_EXPR_ARRAY_LITERAL) {
                    if (decl->initializer->as.array_literal.element_count == 2) {
                        // 检查两个元素都是字符串字面量
                        CnAstExpr *elem0 = decl->initializer->as.array_literal.elements[0];
                        CnAstExpr *elem1 = decl->initializer->as.array_literal.elements[1];
                        if (elem0->kind == CN_AST_EXPR_STRING_LITERAL && 
                            elem1->kind == CN_AST_EXPR_STRING_LITERAL) {
                            printf("测试字符串数组: 成功\n");
                        } else {
                            printf("测试字符串数组: 失败 (元素不是字符串)\n");
                        }
                    } else {
                        printf("测试字符串数组: 失败 (元素数量不为2)\n");
                    }
                } else {
                    printf("测试字符串数组: 失败 (不是数组字面量)\n");
                }
            } else {
                printf("测试字符串数组: 失败 (不是变量声明)\n");
            }
        } else {
            printf("测试字符串数组: 失败 (函数体为空)\n");
        }
    }
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试字符串数组字面量 + 长度运算组合
static void test_string_array_with_length(void) {
    const char *source = "函数 测试() { 变量 arr = [\"hello\", \"world\"]; 变量 len = 长度(arr); 返回 0; }";
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("测试字符串数组+长度: 失败 (有解析错误)\n");
    } else if (!program || program->function_count == 0) {
        printf("测试字符串数组+长度: 失败 (没有函数)\n");
    } else {
        CnAstFunctionDecl *fn = program->functions[0];
        if (fn->body && fn->body->stmt_count >= 2) {
            // 检查第一条语句是字符串数组声明
            CnAstStmt *stmt0 = fn->body->stmts[0];
            if (stmt0->kind != CN_AST_STMT_VAR_DECL) {
                printf("测试字符串数组+长度: 失败 (第一条语句不是变量声明)\n");
            } else {
                // 检查第二条语句是长度调用
                CnAstStmt *stmt1 = fn->body->stmts[1];
                if (stmt1->kind == CN_AST_STMT_VAR_DECL) {
                    CnAstVarDecl *decl = &stmt1->as.var_decl;
                    if (decl->initializer && decl->initializer->kind == CN_AST_EXPR_CALL) {
                        CnAstCallExpr *call = &decl->initializer->as.call;
                        if (call->callee && call->callee->kind == CN_AST_EXPR_IDENTIFIER &&
                            call->argument_count == 1) {
                            printf("测试字符串数组+长度: 成功\n");
                        } else {
                            printf("测试字符串数组+长度: 失败 (函数调用结构不正确)\n");
                        }
                    } else {
                        printf("测试字符串数组+长度: 失败 (初始化器不是函数调用)\n");
                    }
                } else {
                    printf("测试字符串数组+长度: 失败 (第二条语句不是变量声明)\n");
                }
            }
        } else {
            printf("测试字符串数组+长度: 失败 (函数体语句数不足)\n");
        }
    }
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

int main(void) {
    printf("=== 数组字面量Parser单元测试 ===\n");
    test_empty_array();
    test_single_element_array();
    test_multiple_elements_array();
    test_string_array();
    test_string_array_with_length();  // 新增测试
    printf("parser_array_literal_test: 测试完成\n");
    return 0;
}
