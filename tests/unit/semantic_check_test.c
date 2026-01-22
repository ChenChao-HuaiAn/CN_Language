#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"

static void test_duplicate_function() {
    const char *source = "函数 测试() {} 函数 测试() {}";
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
    
    bool found = false;
    for (size_t i = 0; i < diagnostics.count; i++) {
        if (diagnostics.items[i].code == CN_DIAG_CODE_SEM_DUPLICATE_SYMBOL) {
            found = true;
            break;
        }
    }

    if (found) {
        printf("测试重复函数定义: 成功 (检测到错误)\n");
    } else {
        printf("测试重复函数定义: 失败 (未检测到错误)\n");
    }

    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

static void test_undefined_identifier() {
    const char *source = "函数 主函数() { a = 1; }";
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
    
    bool found = false;
    for (size_t i = 0; i < diagnostics.count; i++) {
        if (diagnostics.items[i].code == CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER) {
            found = true;
            break;
        }
    }

    if (found) {
        printf("测试未定义标识符: 成功 (检测到错误)\n");
    } else {
        printf("测试未定义标识符: 失败 (未检测到错误)\n");
    }

    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

static void test_break_outside_loop() {
    const char *source = "函数 主函数() { 中断; }";
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
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    bool found = false;
    for (size_t i = 0; i < diagnostics.count; i++) {
        if (diagnostics.items[i].code == CN_DIAG_CODE_SEM_BREAK_CONTINUE_OUTSIDE_LOOP) {
            found = true;
            break;
        }
    }

    if (found) {
        printf("测试退出(break)在循环外: 成功 (检测到错误)\n");
    } else {
        printf("测试退出(break)在循环外: 失败 (未检测到错误)\n");
    }

    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

static void test_func_call_arg_mismatch() {
    const char *source = "函数 被调用(整数 a) {} 函数 主函数() { 被调用(); }";
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
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    bool found = false;
    for (size_t i = 0; i < diagnostics.count; i++) {
        if (diagnostics.items[i].code == CN_DIAG_CODE_SEM_ARGUMENT_COUNT_MISMATCH) {
            found = true;
            break;
        }
    }

    if (found) {
        printf("测试函数参数个数不匹配: 成功 (检测到错误)\n");
    } else {
        printf("测试函数参数个数不匹配: 失败 (未检测到错误)\n");
    }

    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

int main() {
    test_duplicate_function();
    test_undefined_identifier();
    test_break_outside_loop();
    test_func_call_arg_mismatch();
    return 0;
}
