#include <stdio.h>
#include <stdlib.h>

#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/support/diagnostics.h"

// 读取整个源文件到内存缓冲区
static char *read_file_to_buffer(const char *filename, size_t *out_length)
{
    FILE *fp;
    char *buffer;
    long size;
    size_t length;

    fp = fopen(filename, "rb");
    if (!fp) {
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

    length = fread(buffer, 1, (size_t)size, fp);
    fclose(fp);

    buffer[length] = '\0';

    if (out_length) {
        *out_length = length;
    }

    return buffer;
}

// 打印函数列表和语句数
static void print_function_summary(const CnAstProgram *program)
{
    size_t i;

    if (!program) {
        return;
    }

    for (i = 0; i < program->function_count; ++i) {
        const CnAstFunctionDecl *fn = program->functions[i];
        const char *name = fn->name;
        size_t name_length = fn->name_length;
        size_t stmt_count = 0;

        if (fn->body) {
            stmt_count = fn->body->stmt_count;
        }

        printf("函数: %.*s，语句数: %zu\n",
               (int)name_length,
               name,
               stmt_count);
    }
}

// 打印诊断信息
static void print_diagnostics(const CnDiagnostics *diagnostics)
{
    size_t i;

    if (!diagnostics || diagnostics->count == 0) {
        return;
    }

    for (i = 0; i < diagnostics->count; ++i) {
        const CnDiagnostic *d = &diagnostics->items[i];
        const char *severity_str = (d->severity == CN_DIAG_SEVERITY_ERROR) ? "错误" : "警告";
        const char *filename = d->filename ? d->filename : "<未知文件>";
        const char *message = d->message ? d->message : "<无消息>";
        int line = d->line;
        int column = d->column;

        fprintf(stderr,
                "%s(%d): %s:%d:%d: %s\n",
                severity_str,
                (int)d->code,
                filename,
                line,
                column,
                message);
    }
}

// 检查诊断中是否存在错误
static bool diagnostics_has_error(const CnDiagnostics *diagnostics)
{
    size_t i;

    if (!diagnostics) {
        return false;
    }

    for (i = 0; i < diagnostics->count; ++i) {
        const CnDiagnostic *d = &diagnostics->items[i];
        if (d->severity == CN_DIAG_SEVERITY_ERROR) {
            return true;
        }
    }

    return false;
}

int main(int argc, char **argv)
{
    const char *filename;
    char *source;
    size_t source_length = 0;
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
    CnDiagnostics diagnostics;
    bool ok;

    if (argc < 2) {
        fprintf(stderr, "用法: %s <源文件.cn>\n", argv[0]);
        return 1;
    }

    filename = argv[1];

    source = read_file_to_buffer(filename, &source_length);
    if (!source) {
        fprintf(stderr, "无法读取文件: %s\n", filename);
        return 1;
    }

    cn_support_diagnostics_init(&diagnostics);

    cn_frontend_lexer_init(&lexer, source, source_length, filename);
    cn_frontend_lexer_set_diagnostics(&lexer, &diagnostics);

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "创建解析器失败\n");
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }
    cn_frontend_parser_set_diagnostics(parser, &diagnostics);

    ok = cn_frontend_parse_program(parser, &program);
    if (!ok || !program) {
        fprintf(stderr, "解析失败\n");
        print_diagnostics(&diagnostics);
        cn_support_diagnostics_free(&diagnostics);
        cn_frontend_parser_free(parser);
        free(source);
        return 1;
    }

    if (diagnostics_has_error(&diagnostics)) {
        fprintf(stderr, "解析失败\n");
        print_diagnostics(&diagnostics);
        cn_frontend_ast_program_free(program);
        cn_frontend_parser_free(parser);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }

    print_function_summary(program);
    print_diagnostics(&diagnostics);

    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(&diagnostics);
    free(source);

    return 0;
}
