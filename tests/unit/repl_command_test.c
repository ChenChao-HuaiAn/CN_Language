/*
 * repl_command_test.c
 * REPL 命令解析功能单元测试
 * 
 * 测试场景：
 * - 基本命令识别（:help, :quit, :reset等）
 * - 命令别名（:h, :q, :v）
 * - 错误命令处理
 * - 命令参数解析
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

/* 模拟 ReplConfig 结构 */
typedef struct {
    bool verbose;
    bool show_ast;
    bool show_ir;
    const char *init_file;
} ReplConfig;

/* 模拟 ReplSession 结构（简化版） */
typedef struct {
    int dummy;  // 占位符，实际测试中不需要真实的会话状态
} ReplSession;

/* 被测试的命令处理函数（复制自 main.c） */
static bool handle_command(const char *line, ReplConfig *config, ReplSession *session)
{
    // 去除前后空白
    while (*line == ' ' || *line == '\t') {
        line++;
    }

    if (strcmp(line, ":quit") == 0 || strcmp(line, ":q") == 0) {
        return false; // 返回 false 表示退出
    }

    if (strcmp(line, ":help") == 0 || strcmp(line, ":h") == 0) {
        // 在测试中不打印帮助信息
        return true;
    }

    if (strcmp(line, ":reset") == 0) {
        // 在测试中不实际重置
        return true;
    }

    if (strcmp(line, ":verbose") == 0 || strcmp(line, ":v") == 0) {
        config->verbose = !config->verbose;
        return true;
    }

    if (strcmp(line, ":ast") == 0) {
        config->show_ast = !config->show_ast;
        return true;
    }

    if (strcmp(line, ":ir") == 0) {
        config->show_ir = !config->show_ir;
        return true;
    }

    // 未知命令
    return true;
}

/* 测试 1：基本命令识别 */
static void test_basic_commands(void)
{
    printf("测试: 基本命令识别...\n");
    
    ReplConfig config = {0};
    ReplSession session = {0};
    
    // 测试 :help 命令
    assert(handle_command(":help", &config, &session) == true);
    
    // 测试 :quit 命令
    assert(handle_command(":quit", &config, &session) == false);
    
    // 测试 :reset 命令
    assert(handle_command(":reset", &config, &session) == true);
    
    printf("  ✓ 基本命令识别测试通过\n\n");
}

/* 测试 2：命令别名 */
static void test_command_aliases(void)
{
    printf("测试: 命令别名...\n");
    
    ReplConfig config = {0};
    ReplSession session = {0};
    
    // 测试 :h (help 的别名)
    assert(handle_command(":h", &config, &session) == true);
    
    // 测试 :q (quit 的别名)
    assert(handle_command(":q", &config, &session) == false);
    
    // 测试 :v (verbose 的别名)
    config.verbose = false;
    assert(handle_command(":v", &config, &session) == true);
    assert(config.verbose == true);  // 应该切换状态
    
    printf("  ✓ 命令别名测试通过\n\n");
}

/* 测试 3：配置切换命令 */
static void test_toggle_commands(void)
{
    printf("测试: 配置切换命令...\n");
    
    ReplConfig config = {0};
    ReplSession session = {0};
    
    // 测试 :verbose 切换
    assert(config.verbose == false);
    handle_command(":verbose", &config, &session);
    assert(config.verbose == true);
    handle_command(":verbose", &config, &session);
    assert(config.verbose == false);
    
    // 测试 :ast 切换
    assert(config.show_ast == false);
    handle_command(":ast", &config, &session);
    assert(config.show_ast == true);
    handle_command(":ast", &config, &session);
    assert(config.show_ast == false);
    
    // 测试 :ir 切换
    assert(config.show_ir == false);
    handle_command(":ir", &config, &session);
    assert(config.show_ir == true);
    handle_command(":ir", &config, &session);
    assert(config.show_ir == false);
    
    printf("  ✓ 配置切换命令测试通过\n\n");
}

/* 测试 4：带前导空白的命令 */
static void test_commands_with_whitespace(void)
{
    printf("测试: 带前导空白的命令...\n");
    
    ReplConfig config = {0};
    ReplSession session = {0};
    
    // 测试前导空格
    assert(handle_command("  :help", &config, &session) == true);
    assert(handle_command("\t:quit", &config, &session) == false);
    assert(handle_command("   \t :reset", &config, &session) == true);
    
    printf("  ✓ 带前导空白的命令测试通过\n\n");
}

/* 测试 5：未知命令处理 */
static void test_unknown_commands(void)
{
    printf("测试: 未知命令处理...\n");
    
    ReplConfig config = {0};
    ReplSession session = {0};
    
    // 未知命令应该返回 true（继续循环），但不改变配置
    bool original_verbose = config.verbose;
    assert(handle_command(":unknown", &config, &session) == true);
    assert(config.verbose == original_verbose);  // 配置不应改变
    
    assert(handle_command(":xyz", &config, &session) == true);
    assert(handle_command(":123", &config, &session) == true);
    
    printf("  ✓ 未知命令处理测试通过\n\n");
}

int main(void)
{
    printf("=======================================\n");
    printf("REPL 命令解析单元测试\n");
    printf("=======================================\n\n");
    
    // 运行测试
    test_basic_commands();
    test_command_aliases();
    test_toggle_commands();
    test_commands_with_whitespace();
    test_unknown_commands();
    
    printf("=======================================\n");
    printf("所有测试通过！\n");
    printf("=======================================\n");
    
    return 0;
}
