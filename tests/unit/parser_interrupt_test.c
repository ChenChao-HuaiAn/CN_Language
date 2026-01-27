#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/support/diagnostics.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// 测试基本中断处理函数解析
void test_basic_interrupt_handler() {
    printf("测试基本中断处理函数解析...\n");
    
    const char *source = 
        "中断处理 0 () {\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    assert(parser != NULL);
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);
    
    if (!ok) {
        printf("  [错误] 解析失败\n");
        cn_support_diagnostics_print(&diagnostics);
    }
    
    if (program) {
        printf("  [调试] 解析到 %zu 个函数\n", program->function_count);
    }
    
    assert(ok && program != NULL);
    
    // 检查是否解析到1个函数（中断处理函数）
    assert(program->function_count == 1);
    
    // 检查中断处理函数属性
    CnAstFunctionDecl *isr = program->functions[0];
    assert(isr->is_interrupt_handler == 1);
    assert(isr->interrupt_vector == 0);
    assert(isr->parameter_count == 0);
    assert(strncmp(isr->name, "__isr_", 6) == 0);
    
    printf("  [通过] 基本中断处理函数解析正确\n");
    
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diagnostics);
    cn_frontend_parser_free(parser);
}

// 测试多个中断处理函数
void test_multiple_interrupt_handlers() {
    printf("测试多个中断处理函数...\n");
    
    const char *source = 
        "中断处理 0 () { }\n"
        "中断处理 1 () { }\n"
        "中断处理 15 () { }\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    
    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);
    assert(ok && program != NULL);
    assert(program->function_count == 3);
    
    // 检查各中断处理函数
    assert(program->functions[0]->is_interrupt_handler == 1);
    assert(program->functions[0]->interrupt_vector == 0);
    
    assert(program->functions[1]->is_interrupt_handler == 1);
    assert(program->functions[1]->interrupt_vector == 1);
    
    assert(program->functions[2]->is_interrupt_handler == 1);
    assert(program->functions[2]->interrupt_vector == 15);
    
    printf("  [通过] 多个中断处理函数解析正确\n");
    
    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
}

// 测试中断处理函数与普通函数混合
void test_mixed_functions() {
    printf("测试中断处理函数与普通函数混合...\n");
    
    const char *source = 
        "函数 测试() { 返回 42; }\n"
        "中断处理 5 () { }\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);
    
    if (!ok) {
        printf("  [错误] 解析失败\n");
        cn_support_diagnostics_print(&diagnostics);
    }
    
    if (program) {
        printf("  [调试] 解析到 %zu 个函数\n", program->function_count);
    }
    
    assert(ok && program != NULL);
    assert(program->function_count == 3);
    
    // 第一个是普通函数
    assert(program->functions[0]->is_interrupt_handler == 0);
    
    // 第二个是中断处理函数
    assert(program->functions[1]->is_interrupt_handler == 1);
    assert(program->functions[1]->interrupt_vector == 5);
    
    // 第三个是普通函数
    assert(program->functions[2]->is_interrupt_handler == 0);
    
    printf("  [通过] 混合函数解析正确\n");
    
    cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diagnostics);
    cn_frontend_parser_free(parser);
}

// 测试中断向量号错误：非整数
void test_error_non_integer_vector() {
    printf("测试中断向量号错误（非整数）...\n");
    
    const char *source = "中断处理 abc () { }";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);
    
    // 应该有解析错误
    assert(!ok);
    
    printf("  [通过] 正确检测到非整数向量号错误\n");
    
    if (program) cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diagnostics);
    cn_frontend_parser_free(parser);
}

// 测试中断向量号错误：超出范围
void test_error_vector_out_of_range() {
    printf("测试中断向量号错误（超出范围）...\n");
    
    const char *source = "中断处理 300 () { }";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);
    
    // 应该有解析错误
    assert(!ok);
    
    printf("  [通过] 正确检测到向量号超出范围错误\n");
    
    if (program) cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diagnostics);
    cn_frontend_parser_free(parser);
}

// 测试中断处理函数错误：带参数
void test_error_handler_with_parameters() {
    printf("测试中断处理函数错误（带参数）...\n");
    
    const char *source = "中断处理 0 (整数 x) { }";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);
    
    // 应该有解析错误
    assert(!ok);
    
    printf("  [通过] 正确检测到中断处理函数带参数错误\n");
    
    if (program) cn_frontend_ast_program_free(program);
    cn_support_diagnostics_free(&diagnostics);
    cn_frontend_parser_free(parser);
}

int main(void) {
    printf("========================================\n");
    printf("CN Language 中断处理语法解析测试\n");
    printf("========================================\n\n");
    
    // 注意：'中断处理' 关键字已在阶段9中被删除
    // 中断处理功能已迁移到运行时函数 cn_rt_interrupt_register()
    // 详见 docs/api/runtime/system_api.md
    printf("跳过: '中断处理' 关键字已废弃\n");
    printf("中断处理已迁移到运行时API: cn_rt_interrupt_register()\n");
    
    printf("\n========================================\n");
    printf("测试跳过（功能已迁移到运行时API）\n");
    printf("========================================\n");
    
    return 0;
}
