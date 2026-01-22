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
        fprintf(stderr, "integration_parse_failure_test: 无法打开文件: %s\n", path);
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
    /* 集成测试：读取包含错误的文件，验证解析失败并产生诊断 */
    const char *filename = "test_error_example.cn";
    char *source;
    size_t length;
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    CnDiagnostics diagnostics;
    bool ok;
    size_t i;
    int has_lex_error = 0;
    int has_parse_error = 0;

    source = read_file_to_buffer(filename, &length);
    if (!source) {
        fprintf(stderr, "integration_parse_failure_test: 无法读取文件 %s\n", filename);
        return 1;
    }

    cn_support_diagnostics_init(&diagnostics);

    cn_frontend_lexer_init(&lexer, source, length, filename);
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "integration_parse_failure_test: 创建解析器失败\n");
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    ok = cn_frontend_parse_program(parser, &program);
    
    /* 验证1：解析应该失败 */
    if (ok) {
        fprintf(stderr, "integration_parse_failure_test: 解析应该失败但却成功了\n");
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }

    /* 验证2：应该产生错误 */
    if (diagnostics.count == 0) {
        fprintf(stderr, "integration_parse_failure_test: 应该产生至少一个错误\n");
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }

    /* 验证3：应该有词法错误和语法错误 */
    for (i = 0; i < diagnostics.count; i++) {
        CnDiagnostic *d = &diagnostics.items[i];
        if (d->code == CN_DIAG_CODE_LEX_UNTERMINATED_STRING ||
            d->code == CN_DIAG_CODE_LEX_INVALID_CHAR) {
            has_lex_error = 1;
        }
        if (d->code == CN_DIAG_CODE_PARSE_EXPECTED_TOKEN ||
            d->code == CN_DIAG_CODE_PARSE_INVALID_EXPR) {
            has_parse_error = 1;
        }
    }

    if (!has_lex_error) {
        fprintf(stderr, "integration_parse_failure_test: 应该产生词法错误\n");
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }

    if (!has_parse_error) {
        fprintf(stderr, "integration_parse_failure_test: 应该产生语法错误\n");
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }

    /* 验证4：错误数量应该合理（至少3个：未终止字符串、非法字符、缺少分号） */
    if (diagnostics.count < 3) {
        fprintf(stderr,
                "integration_parse_failure_test: 错误数量不足，期望至少 3 个，实际为 %zu\n",
                diagnostics.count);
        if (program) {
            cn_frontend_ast_program_free(program);
        }
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }

    printf("integration_parse_failure_test: OK\n");
    printf("  检测到 %zu 个错误（预期行为）\n", diagnostics.count);

    if (program) {
        cn_frontend_ast_program_free(program);
    }
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    free(source);

    return 0;
}
