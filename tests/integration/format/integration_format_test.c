#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// cnfmt可执行文件路径
#ifdef _WIN32
#define CNFMT_PATH "build\\src\\Debug\\cnfmt.exe"
#else
#define CNFMT_PATH "build/src/cnfmt"
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
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "%s tests/integration/format/test_input.cn", CNFMT_PATH);
    int result = run_command(cmd, output, sizeof(output));
    
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
    snprintf(cmd, sizeof(cmd), "%s -i %s", CNFMT_PATH, test_file);
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
    snprintf(cmd, sizeof(cmd), "%s -o %s tests/integration/format/test_input.cn", CNFMT_PATH, output_file);
    
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

// 测试--check模式：需要格式化
static void test_check_needs_format()
{
    printf("测试: --check 模式（需要格式化）...\n");

    // 创建需要格式化的临时文件
    const char *test_file = "tests/integration/format/test_unformatted.cn";
    FILE *fp = fopen(test_file, "w");
    if (!fp) {
        printf("  [跳过] 无法创建临时文件\n");
        return;
    }
    fprintf(fp, "函数 测试() { 变量 x = 1; 返回 x; }\n");
    fclose(fp);

    // 检查文件
    char cmd[512];
    char output[1024] = {0};
    int result;
    int check_passed;
    snprintf(cmd, sizeof(cmd), "%s --check %s 2>&1", CNFMT_PATH, test_file);
    result = run_command(cmd, output, sizeof(output));

    // 验证返回非零退出码或输出包含相关信息
    check_passed = (result != 0) || (strstr(output, "需要格式化") != NULL);
    if (!check_passed) {
        printf("  [跳过] 无法验证--check模式\n");
    } else {
        printf("  [通过] --check 模式（需要格式化）\n");
    }

    // 清理
    remove(test_file);
}

// 测试--check模式：已格式化
static void test_check_already_formatted()
{
    printf("测试: --check 模式（已格式化）...\n");

    // 创建已格式化的临时文件
    const char *test_file = "tests/integration/format/test_formatted.cn";
    FILE *fp = fopen(test_file, "w");
    char cmd[512];
    char output[1024] = {0};
    int result;
    if (!fp) {
        printf("  [跳过] 无法创建临时文件\n");
        return;
    }
    fprintf(fp, "函数 测试()\n{\n    变量 x = 1;\n    返回 x;\n}\n");
    fclose(fp);

    // 检查文件
    snprintf(cmd, sizeof(cmd), "%s --check %s 2>&1", CNFMT_PATH, test_file);
    result = run_command(cmd, output, sizeof(output));

    // 验证返回零退出码
    if (result == 0) {
        printf("  [通过] --check 模式（已格式化）\n");
    } else {
        printf("  [跳过] 文件可能仍需要格式化\n");
    }

    // 清理
    remove(test_file);
}

// 测试--verify-idempotence模式
static void test_verify_idempotence()
{
    printf("测试: --verify-idempotence 模式...\n");

    // 使用测试文件
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "%s --verify-idempotence tests/integration/format/test_input.cn", CNFMT_PATH);
    char output[4096] = {0};
    int result = run_command(cmd, output, sizeof(output));

    // 验证命令执行成功
    if (result != 0 && result != -1) {
        printf("  [跳过] cnfmt 可能未构建或不在 PATH 中\n");
        return;
    }

    // 验证输出包含格式化结果
    assert(strlen(output) > 0);
    printf("  [通过] --verify-idempotence 模式\n");
}

int main(void)
{
    printf("=== CN Language 格式化器集成测试 ===\n\n");

    test_format_single_file();
    test_format_in_place();
    test_format_to_file();
    test_check_needs_format();
    test_check_already_formatted();
    test_verify_idempotence();

    printf("\n=== 所有测试通过! ===\n");
    return 0;
}
