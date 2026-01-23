#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    const char *source = 
        "函数 主程序() {\n"
        "    变量 arr = [1, 2, 3, 4, 5];\n"
        "    变量 len = 长度(arr);\n"
        "    返回 0;\n"
        "}\n";
    
    printf("测试代码:\n%s\n", source);
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    cn_frontend_parse_program(parser, &program);
    
    printf("解析错误数: %zu\n", cn_support_diagnostics_error_count(&diagnostics));
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("解析失败\n");
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    
    printf("名称解析错误数: %zu\n", cn_support_diagnostics_error_count(&diagnostics));
    
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    printf("类型检查错误数: %zu\n", cn_support_diagnostics_error_count(&diagnostics));
    
    // 打印所有诊断信息
    cn_support_diagnostics_print(&diagnostics);
    
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    return 0;
}
