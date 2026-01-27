#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/support/diagnostics.h"

#include <stdio.h>
#include <string.h>

// 测试预留关键字在语法层触发错误
static int test_reserved_keyword_error(void)
{
    // 测试使用预留关键字 '类' 应该触发语法错误
    const char *source = "类 测试类 { }";
    size_t length = strlen(source);
    
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    CnDiagnostics diagnostics;

    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source, length, "<test-reserved>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "parser_reserved_keyword_test: 无法创建 parser\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    // 解析应该失败，并产生关于预留关键字的错误
    int parse_result = cn_frontend_parse_program(parser, &program);
    
    // 期望解析失败
    if (parse_result) {
        fprintf(stderr, "parser_reserved_keyword_test: 使用预留关键字应该解析失败\n");
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    
    // 期望有诊断错误
    if (diagnostics.count == 0) {
        fprintf(stderr, "parser_reserved_keyword_test: 应该产生关于预留关键字的错误\n");
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    
    // 检查错误消息是否包含"预留"
    int found_reserved_error = 0;
    for (size_t i = 0; i < diagnostics.count; i++) {
        if (strstr(diagnostics.entries[i].message, "预留") != NULL) {
            found_reserved_error = 1;
            break;
        }
    }
    
    if (!found_reserved_error) {
        fprintf(stderr, "parser_reserved_keyword_test: 错误消息应该提到'预留'\n");
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    
    if (program) {
        cn_frontend_ast_program_free(program);
    }
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    return 0;
}

// 测试删除的关键字在语法层不会被特殊处理
static int test_deleted_keywords_as_normal_identifiers(void)
{
    // 测试使用"数组"作为变量名应该正常工作
    const char *source = "函数 测试() { 变量 数组 = 10; 返回 数组; }";
    size_t length = strlen(source);
    
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    CnDiagnostics diagnostics;

    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source, length, "<test-deleted>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "parser_reserved_keyword_test: 无法创建 parser\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    // 解析应该成功
    int parse_result = cn_frontend_parse_program(parser, &program);
    
    if (!parse_result) {
        fprintf(stderr, "parser_reserved_keyword_test: 使用已删除关键字作为标识符应该成功\n");
        fprintf(stderr, "诊断错误数量: %zu\n", diagnostics.count);
        for (size_t i = 0; i < diagnostics.count; i++) {
            fprintf(stderr, "  错误 %zu: %s\n", i + 1, diagnostics.entries[i].message);
        }
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    
    // 期望没有诊断错误
    if (diagnostics.count > 0) {
        fprintf(stderr, "parser_reserved_keyword_test: 不应该产生诊断错误\n");
        for (size_t i = 0; i < diagnostics.count; i++) {
            fprintf(stderr, "  错误 %zu: %s\n", i + 1, diagnostics.entries[i].message);
        }
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    
    if (program) {
        cn_frontend_ast_program_free(program);
    }
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    return 0;
}

// 测试"主程序"作为普通标识符被正确处理
static int test_main_as_identifier(void)
{
    // 测试"主程序"现在是普通标识符，应该能够作为函数名
    const char *source = "函数 主程序() { 返回 0; }";
    size_t length = strlen(source);
    
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    CnDiagnostics diagnostics;

    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_lexer_init(&lexer, source, length, "<test-main>");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);
    
    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "parser_reserved_keyword_test: 无法创建 parser\n");
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);
    
    // 解析应该成功
    int parse_result = cn_frontend_parse_program(parser, &program);
    
    if (!parse_result) {
        fprintf(stderr, "parser_reserved_keyword_test: '主程序'作为函数名应该解析成功\n");
        if (diagnostics.count > 0) {
            fprintf(stderr, "诊断错误:\n");
            for (size_t i = 0; i < diagnostics.count; i++) {
                fprintf(stderr, "  %s\n", diagnostics.entries[i].message);
            }
        }
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    
    // 验证程序有一个函数
    if (!program || program->function_count != 1) {
        fprintf(stderr, "parser_reserved_keyword_test: 应该有一个函数\n");
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        return 1;
    }
    
    if (program) {
        cn_frontend_ast_program_free(program);
    }
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    
    return 0;
}

int main(void)
{
    printf("运行精简关键字语法测试...\n");

    if (test_reserved_keyword_error() != 0) {
        fprintf(stderr, "测试失败: test_reserved_keyword_error\n");
        return 1;
    }
    printf("✓ 预留关键字触发语法错误\n");

    if (test_deleted_keywords_as_normal_identifiers() != 0) {
        fprintf(stderr, "测试失败: test_deleted_keywords_as_normal_identifiers\n");
        return 1;
    }
    printf("✓ 已删除关键字可作为普通标识符\n");

    if (test_main_as_identifier() != 0) {
        fprintf(stderr, "测试失败: test_main_as_identifier\n");
        return 1;
    }
    printf("✓ '主程序'作为函数名正常工作\n");

    printf("parser_reserved_keyword_test: 所有测试通过 ✅\n");
    return 0;
}
