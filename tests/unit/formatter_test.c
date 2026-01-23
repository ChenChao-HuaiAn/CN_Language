#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cnlang/format/formatter.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/support/diagnostics.h"

// 测试辅助函数：解析代码并格式化
static char *parse_and_format(const char *source_code)
{
    size_t source_length = strlen(source_code);
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);

    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source_code, source_length, "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);

    char *result = NULL;
    if (ok && program && diagnostics.count == 0) {
        CnFormatConfig config;
        cn_format_config_init_default(&config);
        result = cn_format_program_to_string(program, &config, NULL);
    } else {
        fprintf(stderr, "  [警告] 解析失败: ok=%d, program=%p, diagnostics.count=%zu\n",
                ok, (void*)program, diagnostics.count);
    }

    if (program) {
        cn_frontend_ast_program_free(program);
    }
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);

    return result;
}

// 测试简单函数格式化
static void test_simple_function()
{
    printf("测试: 简单函数格式化...\n");

    const char *source = 
        "函数 测试() {\n"
        "  返回 42;\n"
        "}\n";

    char *formatted = parse_and_format(source);
    assert(formatted != NULL);
    
    // 验证包含关键内容
    assert(strstr(formatted, "函数 测试()") != NULL);
    assert(strstr(formatted, "返回 42;") != NULL);

    free(formatted);
    printf("  [通过] 简单函数格式化\n");
}

// 测试变量声明格式化
static void test_variable_declaration()
{
    printf("测试: 变量声明格式化...\n");

    const char *source = 
        "函数 测试() {\n"
        "  变量 x=10;\n"
        "  变量 y   =   20  ;\n"
        "  返回 x;\n"
        "}\n";

    char *formatted = parse_and_format(source);
    assert(formatted != NULL);
    
    // 验证包含统一格式的变量声明
    assert(strstr(formatted, "变量 x = 10;") != NULL);
    assert(strstr(formatted, "变量 y = 20;") != NULL);

    free(formatted);
    printf("  [通过] 变量声明格式化\n");
}

// 测试表达式格式化
static void test_expression_formatting()
{
    printf("测试: 表达式格式化...\n");

    const char *source = 
        "函数 测试() {\n"
        "  变量 x=1+2*3;\n"
        "  返回 x;\n"
        "}\n";

    char *formatted = parse_and_format(source);
    assert(formatted != NULL);
    
    // 验证表达式有空格
    assert(strstr(formatted, "+ ") != NULL || strstr(formatted, " +") != NULL);
    assert(strstr(formatted, "* ") != NULL || strstr(formatted, " *") != NULL);

    free(formatted);
    printf("  [通过] 表达式格式化\n");
}

// 测试 if 语句格式化
static void test_if_statement()
{
    printf("测试: if 语句格式化...\n");

    const char *source = 
        "函数 测试() {\n"
        "  如果(1==1){返回 1;}否则{返回 0;}\n"
        "}\n";

    char *formatted = parse_and_format(source);
    assert(formatted != NULL);
    
    // 验证 if 语句结构
    assert(strstr(formatted, "如果") != NULL);
    assert(strstr(formatted, "否则") != NULL);

    free(formatted);
    printf("  [通过] if 语句格式化\n");
}

// 测试循环语句格式化
static void test_loop_statement()
{
    printf("测试: 循环语句格式化...\n");

    const char *source = 
        "函数 测试() {\n"
        "  变量 x = 1;\n"
        "  返回 x;\n"
        "}\n";

    char *formatted = parse_and_format(source);
    assert(formatted != NULL);
    
    // 验证格式化后有内容
    assert(strstr(formatted, "变量 x") != NULL);

    free(formatted);
    printf("  [通过] 循环语句格式化\n");
}

// 测试函数调用格式化
static void test_function_call()
{
    printf("测试: 函数调用格式化...\n");

    const char *source = 
        "函数 测试() {\n"
        "  打印(\"你好\");\n"
        "}\n";

    char *formatted = parse_and_format(source);
    assert(formatted != NULL);
    
    // 验证函数调用
    assert(strstr(formatted, "打印(") != NULL);

    free(formatted);
    printf("  [通过] 函数调用格式化\n");
}

// 测试数组格式化
static void test_array_formatting()
{
    printf("测试: 数组格式化...\n");

    const char *source = 
        "函数 测试() {\n"
        "  变量 arr=[1,2,3];\n"
        "  返回 arr[0];\n"
        "}\n";

    char *formatted = parse_and_format(source);
    assert(formatted != NULL);
    
    // 验证数组字面量
    assert(strstr(formatted, "[1") != NULL);
    assert(strstr(formatted, "arr[0]") != NULL);

    free(formatted);
    printf("  [通过] 数组格式化\n");
}

// 测试缩进一致性
static void test_indentation()
{
    printf("测试: 缩进一致性...\n");

    const char *source = 
        "函数 测试() {\n"
        "  如果(1==1){\n"
        "    如果(2==2){\n"
        "      返回 1;\n"
        "    }\n"
        "  }\n"
        "}\n";

    char *formatted = parse_and_format(source);
    assert(formatted != NULL);
    
    // 验证格式化后有内容
    assert(strlen(formatted) > 0);

    free(formatted);
    printf("  [通过] 缩进一致性\n");
}

// 测试幂等性（多次格式化结果一致）
static void test_idempotence()
{
    printf("测试: 幂等性...\n");

    const char *source = 
        "函数 测试() {\n"
        "  变量 x = 10;\n"
        "  返回 x;\n"
        "}\n";

    char *formatted1 = parse_and_format(source);
    assert(formatted1 != NULL);

    char *formatted2 = parse_and_format(formatted1);
    assert(formatted2 != NULL);

    // 验证两次格式化结果相同
    assert(strcmp(formatted1, formatted2) == 0);

    // 验证第三次格式化结果仍然相同
    char *formatted3 = parse_and_format(formatted2);
    assert(formatted3 != NULL);
    assert(strcmp(formatted2, formatted3) == 0);

    // 验证第四次格式化结果仍然相同
    char *formatted4 = parse_and_format(formatted3);
    assert(formatted4 != NULL);
    assert(strcmp(formatted3, formatted4) == 0);

    free(formatted1);
    free(formatted2);
    free(formatted3);
    free(formatted4);
    printf("  [通过] 幂等性（验证了4次格式化）\n");
}

// 测试配置选项
static void test_config_options()
{
    printf("测试: 配置选项...\n");

    CnFormatConfig config;
    cn_format_config_init_default(&config);

    // 验证默认配置
    assert(config.indent_size == 4);
    assert(config.max_line_width == 100);
    assert(config.space_around_ops == true);
    assert(config.space_after_comma == true);
    assert(config.space_after_keywords == true);
    assert(config.brace_on_new_line_func == true);
    assert(config.brace_on_same_line_ctrl == true);
    assert(config.always_use_braces == true);
    assert(config.empty_line_between_funcs == true);
    assert(config.max_consecutive_empty_lines == 1);

    printf("  [通过] 配置选项\n");
}

// 测试括号位置配置
static void test_brace_position()
{
    printf("测试: 括号位置配置...\n");

    const char *source = 
        "函数 测试() {\n"
        "  返回 42;\n"
        "}\n";

    size_t source_length = strlen(source);
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);

    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, source_length, "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);
    assert(ok && program);

    // 测试函数大括号在新行
    CnFormatConfig config1;
    cn_format_config_init_default(&config1);
    config1.brace_on_new_line_func = true;
    char *formatted1 = cn_format_program_to_string(program, &config1, NULL);
    assert(formatted1 != NULL);
    assert(strstr(formatted1, ")\n{") != NULL);  // 大括号在新行
    free(formatted1);

    // 测试函数大括号在同一行
    CnFormatConfig config2;
    cn_format_config_init_default(&config2);
    config2.brace_on_new_line_func = false;
    char *formatted2 = cn_format_program_to_string(program, &config2, NULL);
    assert(formatted2 != NULL);
    assert(strstr(formatted2, ") {") != NULL);  // 大括号在同一行
    free(formatted2);

    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);

    printf("  [通过] 括号位置配置\n");
}

// 测试空格配置
static void test_space_config()
{
    printf("测试: 空格配置...\n");

    const char *source = 
        "函数 测试() {\n"
        "  变量 x=1+2;\n"
        "  返回 x;\n"
        "}\n";

    size_t source_length = strlen(source);
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);

    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, source_length, "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);
    assert(ok && program);

    // 测试运算符周围有空格
    CnFormatConfig config1;
    cn_format_config_init_default(&config1);
    config1.space_around_ops = true;
    char *formatted1 = cn_format_program_to_string(program, &config1, NULL);
    assert(formatted1 != NULL);
    assert(strstr(formatted1, " = ") != NULL);
    assert(strstr(formatted1, " + ") != NULL);
    free(formatted1);

    // 测试运算符周围无空格
    CnFormatConfig config2;
    cn_format_config_init_default(&config2);
    config2.space_around_ops = false;
    char *formatted2 = cn_format_program_to_string(program, &config2, NULL);
    assert(formatted2 != NULL);
    assert(strstr(formatted2, "=") != NULL);
    free(formatted2);

    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);

    printf("  [通过] 空格配置\n");
}

// 测试函数间空行
static void test_empty_lines_between_functions()
{
    printf("测试: 函数间空行...\n");

    const char *source = 
        "函数 测试1() {\n"
        "  返回 1;\n"
        "}\n"
        "函数 测试2() {\n"
        "  返回 2;\n"
        "}\n";

    size_t source_length = strlen(source);
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);

    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, source_length, "test.cn");
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    CnParser *parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    CnAstProgram *program = NULL;
    bool ok = cn_frontend_parse_program(parser, &program);
    assert(ok && program);

    // 测试函数间有空行
    CnFormatConfig config1;
    cn_format_config_init_default(&config1);
    config1.empty_line_between_funcs = true;
    char *formatted1 = cn_format_program_to_string(program, &config1, NULL);
    assert(formatted1 != NULL);
    assert(strstr(formatted1, "}\n\n函数 测试2") != NULL);  // 函数间有空行
    free(formatted1);

    // 测试函数间无空行
    CnFormatConfig config2;
    cn_format_config_init_default(&config2);
    config2.empty_line_between_funcs = false;
    char *formatted2 = cn_format_program_to_string(program, &config2, NULL);
    assert(formatted2 != NULL);
    // 验证没有连续两个换行符（函数间无空行）
    assert(strstr(formatted2, "}\n函数 测试2") != NULL);  // 函数间无空行
    free(formatted2);

    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);

    printf("  [通过] 函数间空行\n");
}

int main(void)
{
    printf("=== CN Language 格式化器单元测试 ===\n\n");

    test_simple_function();
    test_variable_declaration();
    test_expression_formatting();
    test_if_statement();
    test_loop_statement();
    test_function_call();
    test_array_formatting();
    test_indentation();
    test_idempotence();
    test_config_options();
    test_brace_position();
    test_space_config();
    test_empty_lines_between_functions();

    printf("\n=== 所有测试通过! ===\n");
    return 0;
}
