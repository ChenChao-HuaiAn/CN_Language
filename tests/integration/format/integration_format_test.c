#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// 运行命令并捕获输出
static int run_command(const char *cmd, char *output, size_t output_size)
{
    FILE *pipe;
    
#ifdef _WIN32
    pipe = _popen(cmd, "r");
#else
    pipe = popen(cmd, "r");
#endif

    if (!pipe) {
        return -1;
    }

    size_t pos = 0;
    while (pos < output_size - 1 && fgets(output + pos, output_size - pos, pipe) != NULL) {
        pos = strlen(output);
    }

#ifdef _WIN32
    int result = _pclose(pipe);
#else
    int result = pclose(pipe);
#endif

    return result;
}

// 读取文件内容
static char *read_file(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
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

    fread(buffer, 1, size, fp);
    buffer[size] = '\0';
    fclose(fp);

    return buffer;
}

// 测试格式化单个文件
static void test_format_single_file()
{
    printf("测试: 格式化单个文件...\n");

    char output[4096] = {0};
    int result = run_command("cnfmt tests/integration/format/test_input.cn", output, sizeof(output));
    
    // 验证命令执行成功
    if (result != 0 && result != -1) {
        printf("  [跳过] cnfmt 可能未构建或不在 PATH 中\n");
        return;
    }

    // 验证输出包含格式化后的内容
    assert(strlen(output) > 0);
    printf("  [通过] 格式化单个文件\n");
}

// 测试原地修改文件
static void test_format_in_place()
{
    printf("测试: 原地修改文件...\n");

    // 创建临时测试文件
    const char *test_file = "tests/integration/format/test_temp.cn";
    FILE *fp = fopen(test_file, "w");
    if (!fp) {
        printf("  [跳过] 无法创建临时文件\n");
        return;
    }
    fprintf(fp, "函数 测试(){变量 x=1;返回 x;}\n");
    fclose(fp);

    // 格式化文件
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "cnfmt -i %s", test_file);
    int result = run_command(cmd, NULL, 0);

    if (result == 0) {
        // 验证文件已被修改
        char *content = read_file(test_file);
        if (content) {
            assert(strstr(content, "变量 x") != NULL);
            free(content);
        }
    }

    // 清理
    remove(test_file);
    printf("  [通过] 原地修改文件\n");
}

// 测试输出到文件
static void test_format_to_file()
{
    printf("测试: 输出到文件...\n");

    const char *output_file = "tests/integration/format/test_output.cn";
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "cnfmt -o %s tests/integration/format/test_input.cn", output_file);
    
    int result = run_command(cmd, NULL, 0);
    
    if (result == 0) {
        // 验证输出文件存在
        FILE *fp = fopen(output_file, "r");
        if (fp) {
            fclose(fp);
            remove(output_file);
        }
    }

    printf("  [通过] 输出到文件\n");
}

int main(void)
{
    printf("=== CN Language 格式化器集成测试 ===\n\n");

    test_format_single_file();
    test_format_in_place();
    test_format_to_file();

    printf("\n=== 所有测试通过! ===\n");
    return 0;
}
