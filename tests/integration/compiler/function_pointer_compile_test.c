#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// 集成测试：完整的函数指针编译流程（到语义分析）
static void test_function_pointer_compilation(void) {
    const char *source = 
        "函数 加法(整数 a, 整数 b) {\n"
        "    返回 a + b;\n"
        "}\n"
        "\n"
        "函数 减法(整数 a, 整数 b) {\n"
        "    返回 a - b;\n"
        "}\n"
        "\n"
        "函数 主程序() {\n"
        "    整数(*运算)(整数, 整数);\n"
        "    运算 = 加法;\n"
        "    变量 结果1 = 运算(10, 5);\n"
        "    运算 = 减法;\n"
        "    变量 结果2 = 运算(10, 5);\n"
        "    返回 0;\n"
        "}\n";
    
    printf("=== 函数指针集成编译测试 ===\n");
    printf("源代码:\n%s\n", source);
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    // 1. 词法分析
    printf("\n[1] 词法分析...\n");
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_function_pointer.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    // 2. 语法分析
    printf("[2] 语法分析...\n");
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool parse_ok = cn_frontend_parse_program(parser, &program);
    
    if (!parse_ok || !program) {
        printf("  ✗ 语法分析失败\n");
        cn_support_diagnostics_print(&diagnostics);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return;
    }
    printf("  ✓ 语法分析成功 (函数数: %zu)\n", program->function_count);
    
    // 3. 语义分析
    printf("[3] 语义分析...\n");
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    cn_sem_resolve_names(global_scope, program, &diagnostics);
    cn_sem_check_types(global_scope, program, &diagnostics);
    
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        printf("  ✗ 语义分析失败\n");
        cn_support_diagnostics_print(&diagnostics);
        cn_sem_scope_free(global_scope);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return;
    }
    printf("  ✓ 语义分析成功\n");
    
    // 清理
    cn_sem_scope_free(global_scope);
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("\n✓ 函数指针集成编译测试通过\n");
}

int main(void) {
    test_function_pointer_compilation();
    return 0;
}
