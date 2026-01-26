#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <string.h>

void print_type(CnType *type, int depth) {
    if (!type) {
        printf("NULL\n");
        return;
    }
    
    for (int i = 0; i < depth; i++) printf("  ");
    
    switch (type->kind) {
        case CN_TYPE_INT:
            printf("INT\n");
            break;
        case CN_TYPE_FLOAT:
            printf("FLOAT\n");
            break;
        case CN_TYPE_ARRAY:
            printf("ARRAY[%zu] of:\n", type->as.array.length);
            print_type(type->as.array.element_type, depth + 1);
            break;
        default:
            printf("OTHER(%d)\n", type->kind);
            break;
    }
}

int main(void) {
    const char *source = "函数 测试() { 整数 matrix[3][4]; 返回 0; }";
    
    printf("测试源码: %s\n\n", source);
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (success && cn_support_diagnostics_error_count(&diagnostics) == 0) {
        CnAstFunctionDecl *func = program->functions[0];
        CnAstStmt *stmt = func->body->stmts[0];
        
        printf("变量类型结构:\n");
        print_type(stmt->as.var_decl.declared_type, 0);
    } else {
        printf("解析失败\n");
        for (size_t i = 0; i < diagnostics.count; i++) {
            printf("  错误: %s\n", diagnostics.items[i].message);
        }
    }
    
    if (program) cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    return 0;
}
