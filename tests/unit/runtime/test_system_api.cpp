// 系统运行时 API 单元测试（Task 6.5，规格书10.4 命令行参数；对标 C++ argv）
// 覆盖：__cn_argc/__cn_argv 全局缓存读写（经 __cn_set_argv_for_test 注入固定参数）/
//       越界返回 nullptr / entry 入口自动缓存
// 测试名英文（GCC 7 不支持中文标识符）；中文仅注释
#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "runtime/runtime.hpp"

namespace {

// 固定测试参数（生命周期覆盖整个测试：argv 数组由 CRT/测试持有）
char g_argv0[] = "test_program.exe";
char g_argv1[] = "hello";
char g_argv2[] = "CN语言";
char* g_argv[3] = { g_argv0, g_argv1, g_argv2 };

// 注入固定 argc/argv（测试专用 setter，生产由 entry 缓存真实参数）
void injectFixedArgs(int argc, char** argv) {
    __cn_set_argv_for_test(argc, argv);
}

} // namespace

// ==================== 1. 参数个数 ====================

// 注入 3 个参数后 argc=3
TEST(SystemApiTest, ArgcCount) {
    injectFixedArgs(3, g_argv);
    EXPECT_EQ(__cn_argc(), 3LL);
}

// 注入 1 个参数（仅程序名）后 argc=1
TEST(SystemApiTest, ArgcSingle) {
    injectFixedArgs(1, g_argv);
    EXPECT_EQ(__cn_argc(), 1LL);
}

// 未注入时 argc=0（全局初始态，防御）
TEST(SystemApiTest, ArgcUninitialized) {
    __cn_set_argv_for_test(0, nullptr);
    EXPECT_EQ(__cn_argc(), 0LL);
}

// ==================== 2. 参数按索引取 argv ====================

// argv[0]：程序名
TEST(SystemApiTest, ArgvZero) {
    injectFixedArgs(3, g_argv);
    char* arg0 = __cn_argv(0);
    ASSERT_NE(arg0, nullptr);
    EXPECT_STREQ(arg0, "test_program.exe");
}

// argv[1]：英文参数
TEST(SystemApiTest, ArgvFirst) {
    injectFixedArgs(3, g_argv);
    char* arg1 = __cn_argv(1);
    ASSERT_NE(arg1, nullptr);
    EXPECT_STREQ(arg1, "hello");
}

// argv[2]：中文参数（UTF-8，CRT 持有非动态分配）
TEST(SystemApiTest, ArgvChinese) {
    injectFixedArgs(3, g_argv);
    char* arg2 = __cn_argv(2);
    ASSERT_NE(arg2, nullptr);
    EXPECT_STREQ(arg2, "CN语言");
}

// 越界（索引 ≥ argc）→ nullptr
TEST(SystemApiTest, ArgvOutOfRange) {
    injectFixedArgs(3, g_argv);
    EXPECT_EQ(__cn_argv(3), nullptr);
    EXPECT_EQ(__cn_argv(99), nullptr);
}

// 负索引 → nullptr
TEST(SystemApiTest, ArgvNegativeIndex) {
    injectFixedArgs(3, g_argv);
    EXPECT_EQ(__cn_argv(-1), nullptr);
}

// 未注入（argv=nullptr）→ nullptr
TEST(SystemApiTest, ArgvUninitialized) {
    __cn_set_argv_for_test(0, nullptr);
    EXPECT_EQ(__cn_argv(0), nullptr);
}

// ==================== 3. entry 入口自动缓存 ====================

// entry(argc, argv) 会缓存参数（entry 内部调 __cn_cache_argv；cn_main 未定义
//   时链接失败——本测试经 __cn_set_argv_for_test 验证缓存读写，entry 的
//   自动缓存由 E2E 37_system 端到端验证）
TEST(SystemApiTest, SetterRoundTrip) {
    injectFixedArgs(2, g_argv);
    EXPECT_EQ(__cn_argc(), 2LL);
    EXPECT_STREQ(__cn_argv(1), "hello");
}
