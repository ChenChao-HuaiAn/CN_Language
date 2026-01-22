#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/support/diagnostics.h"

#include <stdio.h>
#include <stdlib.h>

// 读取文件到缓冲区
static char *read_file_to_buffer(const char *path, size_t *out_length)
{
    FILE *fp;
    long size;
    char *buffer;

    fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "integration_parse_success_test: 无法打开文件: %s\n", path);
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }

    buffer = (char *)malloc((size_t)size + 1);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }

    if (fread(buffer, 1, (size_t)size, fp) != (size_t)size) {
        free(buffer);
        fclose(fp);
        return NULL;
    }

    buffer[size] = '\0';
    fclose(fp);

    if (out_length) {
        *out_length = (size_t)size;
    }

    return buffer;
}

int main(void)
{
    /* 集成测试：读取真实的 hello_world.cn 文件并完整解析 */
    const char *filename = "../../../examples/hello_world.cn";
    char *source;
    size_t length;
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    CnDiagnostics diagnostics;
    bool ok;

    source = read_file_to_buffer(filename, &length);
    if (!source) {
        fprintf(stderr, "integration_parse_success_test: 无法读取文件 %s\n", filename);
        return 1;
    }

    cn_support_diagnostics_init(&diagnostics);

    cn_frontend_lexer_init(&lexer, source, length, filename);
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "integration_parse_success_test: 创建解析器失败\n");
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    ok = cn_frontend_parse_program(parser, &program);
    if (!ok || !program) {
        fprintf(stderr, "integration_parse_success_test: 解析失败\n");
        cn_support_diagnostics_print(&diagnostics);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }

    /* 验证：应该没有错误 */
    if (cn_support_diagnostics_error_count(&diagnostics) > 0) {
        fprintf(stderr, "integration_parse_success_test: 不应产生错误\n");
        cn_support_diagnostics_print(&diagnostics);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }

    /* 验证：应该有且仅有一个函数 */
    if (program->function_count != 1) {
        fprintf(stderr,
                "integration_parse_success_test: 期望 1 个函数，实际为 %zu\n",
                program->function_count);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }

    /* 验证：函数名应为 "主程序" */
    {
        CnAstFunctionDecl *fn = program->functions[0];
        const char *expected_name = "主程序";
        size_t expected_len = strlen(expected_name);

        if (fn->name_length != expected_len ||
            strncmp(fn->name, expected_name, expected_len) != 0) {
            fprintf(stderr, "integration_parse_success_test: 函数名应为 '主程序'\n");
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            cn_support_diagnostics_free(&diagnostics);
            free(source);
            return 1;
        }

        /* 验证：函数体应有 2 条语句 */
        if (!fn->body || fn->body->stmt_count != 2) {
            fprintf(stderr,
                    "integration_parse_success_test: 函数体应有 2 条语句，实际为 %zu\n",
                    fn->body ? fn->body->stmt_count : 0);
            cn_frontend_ast_program_free(program);
            cn_frontend_parser_free(parser);
            cn_support_diagnostics_free(&diagnostics);
            free(source);
            return 1;
        }
    }

    printf("integration_parse_success_test: OK\n");

    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    free(source);

    return 0;
}
