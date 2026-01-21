#include <stdio.h>
#include <stdlib.h>

#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/ast.h"

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

int main(int argc, char **argv)
{
    const char *filename;
    char *source;
    size_t source_length = 0;
    CnLexer lexer;
    CnParser *parser;
    CnAstProgram *program = NULL;
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

    cn_frontend_lexer_init(&lexer, source, source_length, filename);

    parser = cn_frontend_parser_new(&lexer);
    if (!parser) {
        fprintf(stderr, "创建解析器失败\n");
        free(source);
        return 1;
    }

    ok = cn_frontend_parse_program(parser, &program);
    if (!ok || !program) {
        fprintf(stderr, "解析失败\n");
        cn_frontend_parser_free(parser);
        free(source);
        return 1;
    }

    print_function_summary(program);

    cn_frontend_ast_program_free(program);
    cn_frontend_parser_free(parser);
    free(source);

    return 0;
}
