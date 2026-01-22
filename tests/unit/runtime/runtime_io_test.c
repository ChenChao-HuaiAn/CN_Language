#include "cnlang/runtime/io.h"
#include "cnlang/runtime/memory.h"
#include <stdio.h>
#include <string.h>

static int test_file_io(void) {
    const char* filename = "test_io.tmp";
    CnRtFile f = cn_rt_file_open(filename, "w");
    if (f == NULL) return 1;
    
    const char* data = "CN_Language IO Test";
    cn_rt_file_write(f, data, strlen(data));
    cn_rt_file_close(f);
    
    f = cn_rt_file_open(filename, "r");
    if (f == NULL) return 1;
    
    char buffer[64];
    memset(buffer, 0, sizeof(buffer));
    cn_rt_file_read(f, buffer, strlen(data));
    
    if (strcmp(buffer, data) != 0) {
        fprintf(stderr, "test_file_io: data mismatch. Got '%s'\n", buffer);
        cn_rt_file_close(f);
        return 1;
    }
    
    cn_rt_file_close(f);
    remove(filename);
    return 0;
}

int main(void) {
    // 由于控制台输入难以自动化测试，目前仅测试文件 I/O
    if (test_file_io() != 0) return 1;
    
    printf("runtime_io_test: OK\n");
    return 0;
}
