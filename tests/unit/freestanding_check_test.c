// Freestanding 模式特性检查测试
#include <stdio.h>
#include <string.h>
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/semantics/freestanding_check.h"
#include "cnlang/support/diagnostics.h"

// 测试：允许使用打印函数
void test_freestanding_allowed_print() {
    const char *source = 
        "函数 主程序() {\n"
        "    打印(\"Hello\");\n"
        "    返回 0;\n"
        "}\n";
    
    size_t source_len = strlen(source);
    CnLexer lexer;
    CnDiagnostics diagnostics;
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source, source_len, "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);
    
    if (ok && program) {
        // Freestanding 检查应该通过（打印函数是允许的）
        bool check_result = cn_sem_check_freestanding(program, &diagnostics, true);
        
        if (check_result) {
            printf("[通过] Freestanding 模式允许使用打印函数\n");
        } else {
            printf("[失败] Freestanding 模式不应禁止打印函数\n");
            for (size_t i = 0; i < diagnostics.count; i++) {
                printf("  - %s\n", diagnostics.items[i].message);
            }
        }
        
        cn_frontend_ast_program_free(program);
    } else {
        printf("[失败] 解析失败\n");
    }
    
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试：禁止使用文件I/O函数
void test_freestanding_forbidden_file_io() {
    const char *source = 
        "函数 主程序() {\n"
        "    变量 文件 = 文件打开(\"test.txt\");\n"
        "    返回 0;\n"
        "}\n";
    
    size_t source_len = strlen(source);
    CnLexer lexer;
    CnDiagnostics diagnostics;
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source, source_len, "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);
    
    if (ok && program) {
        // Freestanding 检查应该失败（文件I/O是禁止的）
        bool check_result = cn_sem_check_freestanding(program, &diagnostics, true);
        
        if (!check_result && diagnostics.count > 0) {
            printf("[通过] Freestanding 模式正确禁止文件I/O函数\n");
            printf("  - 错误信息: %s\n", diagnostics.items[diagnostics.count - 1].message);
        } else {
            printf("[失败] Freestanding 模式应该禁止文件I/O函数\n");
        }
        
        cn_frontend_ast_program_free(program);
    } else {
        printf("[失败] 解析失败\n");
    }
    
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试：禁止使用控制台输入函数
void test_freestanding_forbidden_console_input() {
    const char *source = 
        "函数 主程序() {\n"
        "    变量 输入 = 读取行();\n"
        "    返回 0;\n"
        "}\n";
    
    size_t source_len = strlen(source);
    CnLexer lexer;
    CnDiagnostics diagnostics;
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source, source_len, "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);
    
    if (ok && program) {
        // Freestanding 检查应该失败（控制台输入是禁止的）
        bool check_result = cn_sem_check_freestanding(program, &diagnostics, true);
        
        if (!check_result && diagnostics.count > 0) {
            printf("[通过] Freestanding 模式正确禁止控制台输入函数\n");
            printf("  - 错误信息: %s\n", diagnostics.items[diagnostics.count - 1].message);
        } else {
            printf("[失败] Freestanding 模式应该禁止控制台输入函数\n");
        }
        
        cn_frontend_ast_program_free(program);
    } else {
        printf("[失败] 解析失败\n");
    }
    
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

// 测试：未启用 freestanding 模式时不检查
void test_freestanding_disabled() {
    const char *source = 
        "函数 主程序() {\n"
        "    变量 文件 = 文件打开(\"test.txt\");\n"
        "    返回 0;\n"
        "}\n";
    
    size_t source_len = strlen(source);
    CnLexer lexer;
    CnDiagnostics diagnostics;
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source, source_len, "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);
    
    if (ok && program) {
        // Freestanding 检查未启用时应该通过
        bool check_result = cn_sem_check_freestanding(program, &diagnostics, false);
        
        if (check_result) {
            printf("[通过] 未启用 freestanding 模式时不进行检查\n");
        } else {
            printf("[失败] 未启用 freestanding 模式时不应报错\n");
        }
        
        cn_frontend_ast_program_free(program);
    } else {
        printf("[失败] 解析失败\n");
    }
    
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
}

int main() {
    printf("=== Freestanding 模式特性检查测试 ===\n\n");
    
    test_freestanding_allowed_print();
    test_freestanding_forbidden_file_io();
    test_freestanding_forbidden_console_input();
    test_freestanding_disabled();
    
    printf("\n测试完成\n");
    return 0;
}
