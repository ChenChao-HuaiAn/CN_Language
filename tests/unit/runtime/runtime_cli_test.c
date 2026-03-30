#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "cnlang/runtime/cli.h"

/*
 * 命令行参数运行时库单元测试
 * 测试覆盖：参数获取、选项查找、位置参数
 */

// 测试计数器
static int tests_run = 0;
static int tests_passed = 0;

// 测试宏
#define TEST(name) \
    static void name(void); \
    static void name##_wrapper(void) { \
        printf("运行测试: %s\n", #name); \
        tests_run++; \
        name(); \
        tests_passed++; \
        printf("  ✓ %s 通过\n", #name); \
    } \
    static void name(void)

#define RUN_TEST(test) test##_wrapper()

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("  ✗ 断言失败: %s\n", message); \
            printf("    位置: %s:%d\n", __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

#define ASSERT_EQ(a, b, message) \
    do { \
        if ((a) != (b)) { \
            printf("  ✗ 断言失败: %s\n", message); \
            printf("    期望: %d, 实际: %d\n", (int)(b), (int)(a)); \
            printf("    位置: %s:%d\n", __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

#define ASSERT_STR_EQ(a, b, message) \
    do { \
        if (strcmp((a), (b)) != 0) { \
            printf("  ✗ 断言失败: %s\n", message); \
            printf("    期望: \"%s\", 实际: \"%s\"\n", (b), (a)); \
            printf("    位置: %s:%d\n", __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

#define ASSERT_NULL(a, message) \
    do { \
        if ((a) != NULL) { \
            printf("  ✗ 断言失败: %s\n", message); \
            printf("    期望: NULL, 实际: %p\n", (void*)(a)); \
            printf("    位置: %s:%d\n", __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

// =============================================================================
// 基础参数获取测试
// =============================================================================

TEST(test_参数个数)
{
    char *argv[] = {"program", "arg1", "arg2"};
    cn_rt_cli_init(3, argv);
    
    ASSERT_EQ(cn_rt_cli_argc(), 3, "参数个数应为3");
    
    cn_rt_cli_cleanup();
}

TEST(test_获取参数)
{
    char *argv[] = {"program", "arg1", "arg2"};
    cn_rt_cli_init(3, argv);
    
    ASSERT_STR_EQ(cn_rt_cli_argv(0), "program", "第0个参数应为program");
    ASSERT_STR_EQ(cn_rt_cli_argv(1), "arg1", "第1个参数应为arg1");
    ASSERT_STR_EQ(cn_rt_cli_argv(2), "arg2", "第2个参数应为arg2");
    ASSERT_NULL(cn_rt_cli_argv(3), "越界索引应返回NULL");
    ASSERT_NULL(cn_rt_cli_argv(-1), "负索引应返回NULL");
    
    cn_rt_cli_cleanup();
}

TEST(test_获取程序名称)
{
    char *argv[] = {"/path/to/program", "arg1"};
    cn_rt_cli_init(2, argv);
    
    ASSERT_STR_EQ(cn_rt_cli_program_name(), "/path/to/program", "程序名称应为第0个参数");
    
    cn_rt_cli_cleanup();
}

TEST(test_空参数)
{
    cn_rt_cli_init(0, NULL);
    
    ASSERT_EQ(cn_rt_cli_argc(), 0, "参数个数应为0");
    ASSERT_NULL(cn_rt_cli_program_name(), "无参数时程序名称应为NULL");
    ASSERT_NULL(cn_rt_cli_argv(0), "无参数时获取参数应返回NULL");
    
    cn_rt_cli_cleanup();
}

// =============================================================================
// 短选项测试
// =============================================================================

TEST(test_短选项存在)
{
    char *argv[] = {"program", "-v", "-d"};
    cn_rt_cli_init(3, argv);
    
    ASSERT_EQ(cn_rt_cli_has_option("v"), 1, "-v 选项应存在");
    ASSERT_EQ(cn_rt_cli_has_option("d"), 1, "-d 选项应存在");
    ASSERT_EQ(cn_rt_cli_has_option("x"), 0, "-x 选项不应存在");
    
    cn_rt_cli_cleanup();
}

TEST(test_短选项值)
{
    char *argv[] = {"program", "-o", "output.txt", "-I", "/include"};
    cn_rt_cli_init(5, argv);
    
    ASSERT_STR_EQ(cn_rt_cli_find_option("o"), "output.txt", "-o 选项值应为output.txt");
    ASSERT_STR_EQ(cn_rt_cli_find_option("I"), "/include", "-I 选项值应为/include");
    ASSERT_NULL(cn_rt_cli_find_option("x"), "-x 选项不应存在");
    
    cn_rt_cli_cleanup();
}

TEST(test_短选项等号形式)
{
    char *argv[] = {"program", "-o=output.txt", "-I=/usr/include"};
    cn_rt_cli_init(3, argv);
    
    ASSERT_STR_EQ(cn_rt_cli_find_option("o"), "output.txt", "-o=output.txt 应解析正确");
    ASSERT_STR_EQ(cn_rt_cli_find_option("I"), "/usr/include", "-I=/usr/include 应解析正确");
    
    cn_rt_cli_cleanup();
}

// =============================================================================
// 长选项测试
// =============================================================================

TEST(test_长选项存在)
{
    char *argv[] = {"program", "--verbose", "--debug"};
    cn_rt_cli_init(3, argv);
    
    ASSERT_EQ(cn_rt_cli_has_option("verbose"), 1, "--verbose 选项应存在");
    ASSERT_EQ(cn_rt_cli_has_option("debug"), 1, "--debug 选项应存在");
    ASSERT_EQ(cn_rt_cli_has_option("help"), 0, "--help 选项不应存在");
    
    cn_rt_cli_cleanup();
}

TEST(test_长选项值)
{
    char *argv[] = {"program", "--output", "result.cn", "--input", "source.cn"};
    cn_rt_cli_init(5, argv);
    
    ASSERT_STR_EQ(cn_rt_cli_find_option("output"), "result.cn", "--output 选项值应为result.cn");
    ASSERT_STR_EQ(cn_rt_cli_find_option("input"), "source.cn", "--input 选项值应为source.cn");
    
    cn_rt_cli_cleanup();
}

TEST(test_长选项等号形式)
{
    char *argv[] = {"program", "--output=result.cn", "--input=source.cn"};
    cn_rt_cli_init(3, argv);
    
    ASSERT_STR_EQ(cn_rt_cli_find_option("output"), "result.cn", "--output=result.cn 应解析正确");
    ASSERT_STR_EQ(cn_rt_cli_find_option("input"), "source.cn", "--input=source.cn 应解析正确");
    
    cn_rt_cli_cleanup();
}

// =============================================================================
// 混合选项测试
// =============================================================================

TEST(test_混合短长选项)
{
    char *argv[] = {"program", "-v", "--verbose", "-o", "out.cn", "--input", "in.cn"};
    cn_rt_cli_init(7, argv);
    
    ASSERT_EQ(cn_rt_cli_has_option("v"), 1, "-v 选项应存在");
    ASSERT_EQ(cn_rt_cli_has_option("verbose"), 1, "--verbose 选项应存在");
    ASSERT_STR_EQ(cn_rt_cli_find_option("o"), "out.cn", "-o 选项值应为out.cn");
    ASSERT_STR_EQ(cn_rt_cli_find_option("input"), "in.cn", "--input 选项值应为in.cn");
    
    cn_rt_cli_cleanup();
}

TEST(test_选项存在扩展)
{
    char *argv[] = {"program", "-v", "--verbose"};
    cn_rt_cli_init(3, argv);
    
    // 同时检查短选项和长选项
    ASSERT_EQ(cn_rt_cli_has_option_ex("v", "verbose"), 1, "短或长选项存在时应返回1");
    ASSERT_EQ(cn_rt_cli_has_option_ex("v", NULL), 1, "仅短选项存在时应返回1");
    ASSERT_EQ(cn_rt_cli_has_option_ex(NULL, "verbose"), 1, "仅长选项存在时应返回1");
    ASSERT_EQ(cn_rt_cli_has_option_ex("x", "xyz"), 0, "都不存在时应返回0");
    
    cn_rt_cli_cleanup();
}

TEST(test_选项值扩展)
{
    char *argv[] = {"program", "-o", "short.cn", "--output", "long.cn"};
    cn_rt_cli_init(5, argv);
    
    // 短选项优先
    ASSERT_STR_EQ(cn_rt_cli_find_option_ex("o", "output"), "short.cn", "短选项值应优先");
    
    // 仅长选项
    char *argv2[] = {"program", "--output", "long.cn"};
    cn_rt_cli_init(3, argv2);
    ASSERT_STR_EQ(cn_rt_cli_find_option_ex("o", "output"), "long.cn", "仅长选项时应返回长选项值");
    
    cn_rt_cli_cleanup();
}

// =============================================================================
// 位置参数测试
// =============================================================================

TEST(test_位置参数基本)
{
    char *argv[] = {"program", "file1.cn", "file2.cn", "file3.cn"};
    cn_rt_cli_init(4, argv);
    
    ASSERT_EQ(cn_rt_cli_get_positional_arg_count(), 3, "位置参数个数应为3");
    ASSERT_STR_EQ(cn_rt_cli_get_positional_arg(0), "file1.cn", "第0个位置参数应为file1.cn");
    ASSERT_STR_EQ(cn_rt_cli_get_positional_arg(1), "file2.cn", "第1个位置参数应为file2.cn");
    ASSERT_STR_EQ(cn_rt_cli_get_positional_arg(2), "file3.cn", "第2个位置参数应为file3.cn");
    ASSERT_NULL(cn_rt_cli_get_positional_arg(3), "越界位置参数应返回NULL");
    
    cn_rt_cli_cleanup();
}

TEST(test_位置参数跳过选项)
{
    // 使用 -- 分隔选项和位置参数
    char *argv[] = {"program", "-v", "-o", "out.c", "--", "file1.cn", "file2.cn", "file3.cn"};
    cn_rt_cli_init(8, argv);
    
    ASSERT_EQ(cn_rt_cli_get_positional_arg_count(), 3, "位置参数个数应为3（跳过选项）");
    ASSERT_STR_EQ(cn_rt_cli_get_positional_arg(0), "file1.cn", "第0个位置参数应为file1.cn");
    ASSERT_STR_EQ(cn_rt_cli_get_positional_arg(1), "file2.cn", "第1个位置参数应为file2.cn");
    ASSERT_STR_EQ(cn_rt_cli_get_positional_arg(2), "file3.cn", "第2个位置参数应为file3.cn");
    
    cn_rt_cli_cleanup();
}

TEST(test_位置参数无)
{
    char *argv[] = {"program", "-v", "--debug"};
    cn_rt_cli_init(3, argv);
    
    ASSERT_EQ(cn_rt_cli_get_positional_arg_count(), 0, "仅有选项时位置参数个数应为0");
    ASSERT_NULL(cn_rt_cli_get_positional_arg(0), "无位置参数时应返回NULL");
    
    cn_rt_cli_cleanup();
}

// =============================================================================
// 中文函数名测试
// =============================================================================

TEST(test_中文函数名)
{
    char *argv[] = {"程序", "-v", "--输出", "结果.cn", "文件.cn"};
    初始化命令行参数(5, argv);
    
    ASSERT_EQ(获取参数个数(), 5, "中文：参数个数应为5");
    ASSERT_STR_EQ(获取参数(0), "程序", "中文：第0个参数应为程序");
    ASSERT_EQ(选项存在("v"), 1, "中文：-v 选项应存在");
    ASSERT_STR_EQ(查找选项("输出"), "结果.cn", "中文：--输出 选项值应为结果.cn");
    ASSERT_EQ(获取位置参数个数(), 1, "中文：位置参数个数应为1");
    ASSERT_STR_EQ(获取位置参数(0), "文件.cn", "中文：第0个位置参数应为文件.cn");
    
    清理命令行参数();
}

// =============================================================================
// 边界情况测试
// =============================================================================

TEST(test_选项值是下一个选项)
{
    // -o 后面紧跟 -v，所以 -o 没有值
    char *argv[] = {"program", "-o", "-v"};
    cn_rt_cli_init(3, argv);
    
    ASSERT_EQ(cn_rt_cli_has_option("o"), 1, "-o 选项应存在");
    ASSERT_NULL(cn_rt_cli_find_option("o"), "-o 后面是选项，值应为NULL");
    ASSERT_EQ(cn_rt_cli_has_option("v"), 1, "-v 选项应存在");
    
    cn_rt_cli_cleanup();
}

TEST(test_重复选项)
{
    char *argv[] = {"program", "-v", "-v", "--verbose", "--verbose"};
    cn_rt_cli_init(5, argv);
    
    ASSERT_EQ(cn_rt_cli_has_option("v"), 1, "重复选项应识别为存在");
    ASSERT_EQ(cn_rt_cli_has_option("verbose"), 1, "重复长选项应识别为存在");
    
    cn_rt_cli_cleanup();
}

TEST(test_选项值覆盖)
{
    // 多次指定同一选项，返回第一个值
    char *argv[] = {"program", "-o", "first.cn", "-o", "second.cn"};
    cn_rt_cli_init(5, argv);
    
    ASSERT_STR_EQ(cn_rt_cli_find_option("o"), "first.cn", "重复选项应返回第一个值");
    
    cn_rt_cli_cleanup();
}

// =============================================================================
// 主函数
// =============================================================================

int main(void) {
    printf("\n========================================\n");
    printf("命令行参数运行时库单元测试\n");
    printf("========================================\n\n");
    
    // 基础参数获取测试
    RUN_TEST(test_参数个数);
    RUN_TEST(test_获取参数);
    RUN_TEST(test_获取程序名称);
    RUN_TEST(test_空参数);
    
    // 短选项测试
    RUN_TEST(test_短选项存在);
    RUN_TEST(test_短选项值);
    RUN_TEST(test_短选项等号形式);
    
    // 长选项测试
    RUN_TEST(test_长选项存在);
    RUN_TEST(test_长选项值);
    RUN_TEST(test_长选项等号形式);
    
    // 混合选项测试
    RUN_TEST(test_混合短长选项);
    RUN_TEST(test_选项存在扩展);
    RUN_TEST(test_选项值扩展);
    
    // 位置参数测试
    RUN_TEST(test_位置参数基本);
    RUN_TEST(test_位置参数跳过选项);
    RUN_TEST(test_位置参数无);
    
    // 中文函数名测试
    RUN_TEST(test_中文函数名);
    
    // 边界情况测试
    RUN_TEST(test_选项值是下一个选项);
    RUN_TEST(test_重复选项);
    RUN_TEST(test_选项值覆盖);
    
    printf("\n========================================\n");
    printf("测试结果: %d 通过, %d 失败\n", tests_passed, tests_run - tests_passed);
    printf("========================================\n\n");
    
    return (tests_run == tests_passed) ? 0 : 1;
}
