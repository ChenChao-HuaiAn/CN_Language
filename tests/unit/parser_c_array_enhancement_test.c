#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

// 测试未初始化的数组声明
static void test_uninitialized_array(void) {
    const char *source = "函数 测试() { 整数 numbers[10]; 返回 0; }";
    
    printf("测试: 未初始化的数组声明\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✗ 解析失败或有错误\n");
        for (size_t i = 0; i < diagnostics.count; i++) {
            printf("    错误: %s\n", diagnostics.items[i].message);
        }
    } else {
        printf("  ✓ 解析成功\n");
        
        CnAstFunctionDecl *func = program->functions[0];
        CnAstStmt *stmt = func->body->stmts[0];
        
        printf("  - 变量名: %.*s\n", (int)stmt->as.var_decl.name_length, stmt->as.var_decl.name);
        printf("  - 数组大小: %zu\n", stmt->as.var_decl.declared_type->as.array.length);
        printf("  - 有初始化器: %s\n", stmt->as.var_decl.initializer ? "是" : "否");
    }
    
    if (program) cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

int main(void) {
    printf("=== C风格数组完善测试 ===\n");
    test_uninitialized_array();
    return 0;
}
