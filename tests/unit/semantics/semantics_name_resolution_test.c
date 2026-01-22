#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"

static CnAstProgram *parse_source(const char *source, CnDiagnostics *diag) {
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, diag);

    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, diag);

    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    cn_frontend_parser_free(parser);
    return program;
}

void test_name_resolution_success() {
    const char *source = "函数 主函数() { 整数 a = 1; a = 2; }";
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);

    CnAstProgram *program = parse_source(source, &diag);
    assert(program != NULL);
    assert(diag.count == 0);

    CnSemScope *global_scope = cn_sem_build_scopes(program, &diag);
    assert(global_scope != NULL);
    assert(diag.count == 0);

    bool success = cn_sem_resolve_names(global_scope, program, &diag);
    assert(success);
    assert(diag.count == 0);

    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diag);
    printf("test_name_resolution_success: PASSED\n");
}

void test_undefined_variable() {
    const char *source = "函数 主函数() { a = 1; }";
    CnDiagnostics diag;
    cn_support_diagnostics_init(&diag);

    CnAstProgram *program = parse_source(source, &diag);
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diag);
    cn_sem_resolve_names(global_scope, program, &diag);

    bool found = false;
    for (size_t i = 0; i < diag.count; i++) {
        if (diag.items[i].code == CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER) {
            found = true;
            break;
        }
    }
    assert(found);

    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diag);
    printf("test_undefined_variable: PASSED\n");
}

int main() {
    test_name_resolution_success();
    test_undefined_variable();
    return 0;
}
