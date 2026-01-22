#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 读取文件到缓冲区
static char *read_file_to_buffer(const char *path, size_t *out_length)
{
    FILE *fp;
    long size;
    char *buffer;

    fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "integration_semantic_error_test: 无法打开文件: %s\n", path);
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
    /* 集成测试：读取包含语义错误的文件，验证语义分析失败并产生正确诊断 */
    const char *filename = "test_semantic_errors.cn";
    char *source;
    size_t length;
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    CnDiagnostics diagnostics;
    bool ok;
    size_t i;

    int has_undefined_identifier = 0;
    int has_duplicate_symbol = 0;
    int has_arg_mismatch = 0;
    int has_break_outside_loop = 0;

    source = read_file_to_buffer(filename, &length);
    if (!source) {
        fprintf(stderr, "integration_semantic_error_test: 无法读取文件 %s\n", filename);
        return 1;
    }

    cn_support_diagnostics_init(&diagnostics);

    cn_frontend_lexer_init(&lexer, source, length, filename);
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    parser = cn_frontend_parser_new(&lexer);
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    ok = cn_frontend_parse_program(parser, &program);
    if (!ok || diagnostics.count > 0) {
        fprintf(stderr, "integration_semantic_error_test: 语法分析应该成功但失败了或产生了错误\n");
        cn_support_diagnostics_print(&diagnostics);
        goto cleanup;
    }

    // 执行语义分析
    CnSemScope *global_scope = cn_sem_build_scopes(program, &diagnostics);
    if (global_scope) {
        cn_sem_resolve_names(global_scope, program, &diagnostics);
        cn_sem_check_types(global_scope, program, &diagnostics);
        cn_sem_scope_free(global_scope);
    }

    /* 验证：应该产生特定的语义错误 */
    for (i = 0; i < diagnostics.count; i++) {
        CnDiagnostic *d = &diagnostics.items[i];
        if (d->code == CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER) {
            has_undefined_identifier++;
        }
        if (d->code == CN_DIAG_CODE_SEM_DUPLICATE_SYMBOL) {
            has_duplicate_symbol++;
        }
        if (d->code == CN_DIAG_CODE_SEM_ARGUMENT_COUNT_MISMATCH) {
            has_arg_mismatch++;
        }
        if (d->code == CN_DIAG_CODE_SEM_BREAK_CONTINUE_OUTSIDE_LOOP) {
            has_break_outside_loop++;
        }
    }

    printf("语义分析集成测试结果：\n");
    printf("- 未定义标识符: %d (期望 >= 1)\n", has_undefined_identifier);
    printf("- 重复定义符号: %d (期望 >= 2)\n", has_duplicate_symbol);
    printf("- 参数个数不匹配: %d (期望 >= 1)\n", has_arg_mismatch);
    printf("- 循环外中断: %d (期望 >= 1)\n", has_break_outside_loop);

    if (has_undefined_identifier >= 1 && has_duplicate_symbol >= 2 && 
        has_arg_mismatch >= 1 && has_break_outside_loop >= 1) {
        printf("integration_semantic_error_test: OK\n");
        ok = true;
    } else {
        fprintf(stderr, "integration_semantic_error_test: 某些期望的语义错误未被检测到\n");
        cn_support_diagnostics_print(&diagnostics);
        ok = false;
    }

cleanup:
    if (program) {
        cn_frontend_ast_program_free(program);
    }
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    free(source);

    return ok ? 0 : 1;
}
