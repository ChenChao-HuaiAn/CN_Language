#include "cnlang/frontend/preprocessor.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.cn>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "无法打开文件: %s\n", filename);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *source = (char *)malloc(file_size + 1);
    if (!source) {
        fclose(f);
        return 1;
    }

    fread(source, 1, file_size, f);
    source[file_size] = '\0';
    fclose(f);

    CnPreprocessor preprocessor;
    CnDiagnostics diagnostics;

    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_preprocessor_init(&preprocessor, source, file_size, filename);
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);

    if (!cn_frontend_preprocessor_process(&preprocessor)) {
        fprintf(stderr, "预处理失败\n");
        cn_frontend_preprocessor_free(&preprocessor);
        cn_support_diagnostics_free(&diagnostics);
        free(source);
        return 1;
    }

    printf("=== 预处理后的输出 ===\n");
    
    /* 逐行输出,显示行号 */
    const char *p = preprocessor.output;
    int line = 1;
    int col = 1;
    printf("Line %3d: ", line);
    
    for (size_t i = 0; i < preprocessor.output_length; i++) {
        char c = p[i];
        if (c == '\n') {
            printf(" [EOL at col %d]\n", col);
            line++;
            col = 1;
            if (i + 1 < preprocessor.output_length) {
                printf("Line %3d: ", line);
            }
        } else if (c == '#') {
            printf("[#<%d>]", col);
            col++;
        } else if (c >= 32 && c < 127) {
            putchar(c);
            col++;
        } else if ((unsigned char)c >= 0x80) {
            printf("<%02X>", (unsigned char)c);
            col++;
        } else {
            printf("[0x%02X]", (unsigned char)c);
            col++;
        }
    }
    
    printf("\n=== 输出结束 (总长度: %zu 字节, %d 行) ===\n", 
           preprocessor.output_length, line);

    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    free(source);

    return 0;
}
