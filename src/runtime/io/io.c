#include "cnlang/runtime/io.h"
#include "cnlang/runtime/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * CN Language 运行时 I/O 实现
 */

char* cn_rt_read_line(void) {
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return NULL;
    }
    
    // 移除换行符
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
        len--;
    }
    
    char* result = (char*)cn_rt_malloc(len + 1);
    if (result) {
        memcpy(result, buffer, len + 1);
    }
    return result;
}

int cn_rt_read_int(long long* out_val) {
    if (out_val == NULL) return 0;
    return scanf("%lld", out_val) == 1;
}

int cn_rt_read_float(double* out_val) {
    if (out_val == NULL) return 0;
    return scanf("%lf", out_val) == 1;
}

int cn_rt_read_string(char* buffer, size_t size) {
    if (buffer == NULL || size == 0) return 0;
    
    // 读取一行到缓冲区，最多读取 size-1 个字符
    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
        return 0;
    }
    
    // 移除末尾的换行符
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
        len--;
    }
    
    return 1;
}

int cn_rt_read_char(char* out_char) {
    if (out_char == NULL) return 0;
    int c = getchar();
    if (c == EOF) return 0;
    *out_char = (char)c;
    return 1;
}

CnRtFile cn_rt_file_open(const char* path, const char* mode) {
    if (path == NULL || mode == NULL) return NULL;
    return (CnRtFile)fopen(path, mode);
}

void cn_rt_file_close(CnRtFile file) {
    if (file) {
        fclose((FILE*)file);
    }
}

size_t cn_rt_file_read(CnRtFile file, void* buffer, size_t size) {
    if (file == NULL || buffer == NULL) return 0;
    return fread(buffer, 1, size, (FILE*)file);
}

size_t cn_rt_file_write(CnRtFile file, const void* buffer, size_t size) {
    if (file == NULL || buffer == NULL) return 0;
    return fwrite(buffer, 1, size, (FILE*)file);
}

int cn_rt_file_eof(CnRtFile file) {
    if (file == NULL) return 1;
    return feof((FILE*)file);
}

int cn_rt_file_setbuf(CnRtFile file, char* buffer, int mode, size_t size) {
    if (file == NULL) return -1;
    
    FILE* fp = (FILE*)file;
    
    // 根据缓冲模式设置
    if (mode == 2) {  // 无缓冲
        setvbuf(fp, NULL, _IONBF, 0);
    } else if (mode == 1) {  // 行缓冲
        setvbuf(fp, buffer, _IOLBF, size);
    } else {  // 全缓冲
        setvbuf(fp, buffer, _IOFBF, size);
    }
    
    return 0;
}

int cn_rt_flush_all(void) {
    return fflush(NULL);
}
