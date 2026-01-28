/**
 * 全局变量单元测试
 * 测试解析器对全局变量声明的支持
 */

#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <string.h>

// 测试1：基本全局变量声明
static void test_basic_global_var(void) {
    const char *source = 
        "变量 x = 42;\n"
        "函数 主程序() { 返回 x; }";
    
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
        printf("测试基本全局变量: 失败 (解析错误)\n");
    } else if (!program || program->global_var_count != 1) {
        printf("测试基本全局变量: 失败 (全局变量数量不正确: %zu)\n", 
               program ? program->global_var_count : 0);
    } else {
        CnAstStmt *var_stmt = program->global_vars[0];
        if (var_stmt->kind == CN_AST_STMT_VAR_DECL) {
            CnAstVarDecl *var_decl = &var_stmt->as.var_decl;
            if (strncmp(var_decl->name, "x", var_decl->name_length) == 0) {
                printf("测试基本全局变量: 成功\n");
            } else {
                printf("测试基本全局变量: 失败 (变量名不匹配)\n");
            }
        } else {
            printf("测试基本全局变量: 失败 (不是变量声明)\n");
        }
    }
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试2：常量全局变量
static void test_const_global_var(void) {
    const char *source = 
        "常量 整数 MAX = 100;\n"
        "函数 主程序() { 返回 MAX; }";
    
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
        printf("测试常量全局变量: 失败 (解析错误)\n");
    } else if (!program || program->global_var_count != 1) {
        printf("测试常量全局变量: 失败 (全局变量数量不正确)\n");
    } else {
        CnAstStmt *var_stmt = program->global_vars[0];
        if (var_stmt->kind == CN_AST_STMT_VAR_DECL) {
            CnAstVarDecl *var_decl = &var_stmt->as.var_decl;
            if (var_decl->is_const) {
                printf("测试常量全局变量: 成功\n");
            } else {
                printf("测试常量全局变量: 失败 (常量标记丢失)\n");
            }
        } else {
            printf("测试常量全局变量: 失败 (不是变量声明)\n");
        }
    }
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试3：多个全局变量
static void test_multiple_global_vars(void) {
    const char *source = 
        "变量 x = 10;\n"
        "变量 y = 20;\n"
        "常量 整数 z = 30;\n"
        "函数 主程序() { 返回 x + y + z; }";
    
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
        printf("测试多个全局变量: 失败 (解析错误)\n");
    } else if (!program || program->global_var_count != 3) {
        printf("测试多个全局变量: 失败 (全局变量数量不正确: %zu, 期望3)\n", 
               program ? program->global_var_count : 0);
    } else {
        printf("测试多个全局变量: 成功\n");
    }
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试4：全局变量类型推断
static void test_global_var_type_inference(void) {
    const char *source = 
        "变量 x = 42;\n"
        "函数 主程序() { 返回 0; }";
    
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
        printf("测试全局变量类型推断: 失败 (解析错误)\n");
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return;
    }
    
    // 构建作用域和类型检查
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    
    if (!cn_sem_check_types(global_scope, program, &diagnostics)) {
        printf("测试全局变量类型推断: 失败 (类型检查失败)\n");
    } else if (!program || program->global_var_count != 1) {
        printf("测试全局变量类型推断: 失败 (全局变量数量不正确)\n");
    } else {
        CnAstStmt *var_stmt = program->global_vars[0];
        if (var_stmt->kind == CN_AST_STMT_VAR_DECL) {
            CnAstVarDecl *var_decl = &var_stmt->as.var_decl;
            if (var_decl->declared_type && var_decl->declared_type->kind == CN_TYPE_INT) {
                printf("测试全局变量类型推断: 成功\n");
            } else {
                printf("测试全局变量类型推断: 失败 (类型推断不正确)\n");
            }
        } else {
            printf("测试全局变量类型推断: 失败 (不是变量声明)\n");
        }
    }
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试5：结构体全局变量声明时初始化
static void test_struct_global_var_with_init(void) {
    const char *source =
        "结构体 点 {\n"
        "    整数 x;\n"
        "    整数 y;\n"
        "}\n"
        "结构体 点 G = {10, 20};\n"
        "函数 主程序() { 返回 0; }";

    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);

    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_struct_global.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);

    if (cn_support_diagnostics_error_count(&diagnostics) > 0 || !program) {
        printf("测试结构体全局变量初始化: 失败 (解析错误)\n");
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return;
    }

    // 构建作用域和类型检查
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);

    if (!cn_sem_check_types(global_scope, program, &diagnostics)) {
        printf("测试结构体全局变量初始化: 失败 (类型检查失败)\n");
    } else if (program->global_var_count != 1) {
        printf("测试结构体全局变量初始化: 失败 (全局变量数量不正确: %zu)\n",
               program->global_var_count);
    } else {
        CnAstStmt *var_stmt = program->global_vars[0];
        if (var_stmt->kind != CN_AST_STMT_VAR_DECL) {
            printf("测试结构体全局变量初始化: 失败 (不是变量声明)\n");
        } else {
            CnAstVarDecl *var_decl = &var_stmt->as.var_decl;
            if (!var_decl->declared_type || var_decl->declared_type->kind != CN_TYPE_STRUCT) {
                printf("测试结构体全局变量初始化: 失败 (声明类型不是结构体)\n");
            } else if (!var_decl->initializer ||
                       var_decl->initializer->kind != CN_AST_EXPR_STRUCT_LITERAL) {
                printf("测试结构体全局变量初始化: 失败 (初始化表达式不是结构体字面量)\n");
            } else {
                printf("测试结构体全局变量初始化: 成功\n");
            }
        }
    }

    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

int main(void) {
    printf("=================================================\n");
    printf("      全局变量单元测试\n");
    printf("=================================================\n");
    
    test_basic_global_var();
    test_const_global_var();
    test_multiple_global_vars();
    test_global_var_type_inference();
    test_struct_global_var_with_init();
    
    printf("=================================================\n");
    printf("测试完成\n");
    printf("=================================================\n");
    
    return 0;
}
