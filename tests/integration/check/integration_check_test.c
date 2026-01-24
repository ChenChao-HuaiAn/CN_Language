#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#define popen _popen
#define pclose _pclose
#endif

// 全局变量：保存 cncheck 可执行文件路径
static const char *g_cncheck_path = "cncheck";

// 辅助函数：执行命令并获取返回码
static int run_command(const char *cmd)
{
    int result = system(cmd);
#ifdef _WIN32
    return result;
#else
    return WEXITSTATUS(result);
#endif
}

// 辅助函数：创建临时测试文件
static void create_test_file(const char *filename, const char *content)
{
    FILE *fp = fopen(filename, "w");
    assert(fp != NULL);
    fprintf(fp, "%s", content);
    fclose(fp);
}

// 测试1：检查单个文件（有问题）
static void test_check_file_with_issues(void)
{
    const char *test_file = "test_unused_var.cn";
    const char *content = 
        "函数 测试() {\n"
        "    变量 未使用 = 10;\n"
        "    变量 使用了 = 20;\n"
        "    打印(使用了);\n"
        "};\n";
    
    create_test_file(test_file, content);
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "%s %s", g_cncheck_path, test_file);
    
    int result = run_command(cmd);
    
    // 应该返回非零（发现问题）
    assert(result != 0);
    
    remove(test_file);
    printf("测试 test_check_file_with_issues: 通过\n");
}

// 测试2：检查单个文件（无问题）
static void test_check_file_clean(void)
{
    const char *test_file = "test_clean.cn";
    const char *content = 
        "函数 计算(a: 整数) -> 整数 {\n"
        "    返回 a + 1;\n"
        "};\n";
    
    create_test_file(test_file, content);
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "%s %s", g_cncheck_path, test_file);
    
    int result = run_command(cmd);
    
    // 应该返回零（无问题）
    assert(result == 0);
    
    remove(test_file);
    printf("测试 test_check_file_clean: 通过\n");
}

// 测试3：check-only 模式（CI 模式）
static void test_check_only_mode(void)
{
    const char *test_file = "test_ci_mode.cn";
    const char *content = 
        "函数 测试(未使用参数: 整数) {\n"
        "    打印(123);\n"
        "}\n";
    
    create_test_file(test_file, content);
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "%s --check-only %s", g_cncheck_path, test_file);
    
    int result = run_command(cmd);
    
    // 应该返回非零（发现未使用参数）
    assert(result != 0);
    
    remove(test_file);
    printf("测试 test_check_only_mode: 通过\n");
}

// 测试4：自定义复杂度阈值
static void test_custom_thresholds(void)
{
    const char *test_file = "test_complexity.cn";
    const char *content = 
        "函数 复杂函数() {\n"
        "    变量 a = 1;\n"
        "    变量 b = 2;\n"
        "    变量 c = 3;\n"
        "    打印(a);\n"
        "    打印(b);\n"
        "    打印(c);\n"
        "}\n";
    
    create_test_file(test_file, content);
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "%s --max-statements 3 %s", g_cncheck_path, test_file);
    
    int result = run_command(cmd);
    
    // 应该返回非零（超过语句数限制）
    assert(result != 0);
    
    remove(test_file);
    printf("测试 test_custom_thresholds: 通过\n");
}

// 测试5：禁用特定规则
static void test_disable_rule(void)
{
    const char *test_file = "test_disable.cn";
    const char *content = 
        "函数 测试() {\n"
        "    变量 未使用 = 10;\n"
        "}\n";
    
    create_test_file(test_file, content);
    
    // 禁用未使用变量检查
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "%s --disable-unused-var %s", g_cncheck_path, test_file);
    
    int result = run_command(cmd);
    
    // 应该返回零（规则被禁用）
    assert(result == 0);
    
    remove(test_file);
    printf("测试 test_disable_rule: 通过\n");
}

int main(int argc, char *argv[])
{
    // 如果提供了参数，使用它作为 cncheck 路径
    if (argc > 1) {
        g_cncheck_path = argv[1];
    }
    
    printf("=== 运行静态检查工具集成测试 ===\n\n");
    printf("使用 cncheck 路径: %s\n\n", g_cncheck_path);

    test_check_file_with_issues();
    test_check_file_clean();
    test_check_only_mode();
    test_custom_thresholds();
    test_disable_rule();

    printf("\n所有集成测试通过!\n");
    return 0;
}
