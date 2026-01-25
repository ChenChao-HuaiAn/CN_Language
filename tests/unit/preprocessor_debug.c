#include "cnlang/frontend/preprocessor.h"
#include "cnlang/support/diagnostics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file_to_buffer(const char *path, size_t *out_length)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buffer = (char *)malloc(size + 1);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }

    size_t length = fread(buffer, 1, size, fp);
    buffer[length] = '\0';
    fclose(fp);

    if (out_length) {
        *out_length = length;
    }

    return buffer;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "用法: %s <文件.cn>\n", argv[0]);
        return 1;
    }

    size_t length;
    char *source = read_file_to_buffer(argv[1], &length);
    if (!source) {
        fprintf(stderr, "无法读取文件: %s\n", argv[1]);
        return 1;
    }

    CnPreprocessor preprocessor;
    CnDiagnostics diagnostics;

    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_preprocessor_init(&preprocessor, source, length, argv[1]);
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);

    if (!cn_frontend_preprocessor_process(&preprocessor)) {
        fprintf(stderr, "预处理失败\n");
        cn_frontend_preprocessor_free(&preprocessor);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }

    printf("预处理成功!\n");
    printf("输出长度: %zu\n", preprocessor.output_length);
    printf("输出内容:\n");
    printf("---BEGIN---\n");
    fwrite(preprocessor.output, 1, preprocessor.output_length, stdout);
    printf("\n---END---\n");

    /* 打印每个字节的十六进制值 */
    printf("\n十六进制输出:\n");
    for (size_t i = 0; i < preprocessor.output_length; ++i) {
        unsigned char c = (unsigned char)preprocessor.output[i];
        if (c == '\n') {
            printf("\\n\n");
        } else if (c >= 32 && c < 127) {
            printf("%c", c);
        } else {
            printf("<%02X>", c);
        }
    }
    printf("\n");

    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    free(source);

    return 0;
}
