#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/token.h"

#include <stdio.h>
#include <stdlib.h>

static char *read_file_to_buffer(const char *path, size_t *out_length)
{
    FILE *fp;
    long size;
    char *buffer;

    fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "无法打开文件: %s\n", path);
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
    if (buffer == NULL) {
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

    if (out_length != NULL) {
        *out_length = (size_t)size;
    }

    return buffer;
}

int main(void)
{
    const char *path = "../../../examples/function_examples.cn";
    size_t length;
    char *source;
    CnLexer lexer;
    CnToken token;

    source = read_file_to_buffer(path, &length);
    if (source == NULL) {
        return 1;
    }

    cn_frontend_lexer_init(&lexer, source, length, path);

    for (;;) {
        if (!cn_frontend_lexer_next_token(&lexer, &token)) {
            /* 出现内部错误，中止 */
            break;
        }

        cn_frontend_token_print(&token);

        if (token.kind == CN_TOKEN_EOF) {
            break;
        }
    }

    free(source);
    return 0;
}
