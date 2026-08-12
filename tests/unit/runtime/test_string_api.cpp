// 运行时字符串API单元测试（Task 2.5）
// 覆盖：字符串长度/比较/连接/复制/查找、打印行多参数辅助
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "runtime/runtime.hpp"

namespace {

// 辅助：捕获函数输出
std::string captureOutput(void (*func)()) {
    testing::internal::CaptureStdout();
    func();
    return testing::internal::GetCapturedStdout();
}

} // namespace

// ==================== 1. 字符串长度 ====================

// 长度：ASCII 字符串
TEST(StringApiTest, LenAscii) {
    EXPECT_EQ(__cn_str_len("hello"), 5);
}

// 长度：UTF-8 中文字符串（按字节数，"你好"=6 字节）
TEST(StringApiTest, LenUtf8) {
    EXPECT_EQ(__cn_str_len("你好"), 6);
}

// 长度：空字符串
TEST(StringApiTest, LenEmpty) {
    EXPECT_EQ(__cn_str_len(""), 0);
}

// 长度：空指针视为空串
TEST(StringApiTest, LenNull) {
    EXPECT_EQ(__cn_str_len(nullptr), 0);
}

// ==================== 2. 字符串比较 ====================

// 相等返回1
TEST(StringApiTest, EqEqual) {
    EXPECT_EQ(__cn_str_eq("abc", "abc"), 1);
}

// 不等返回0
TEST(StringApiTest, EqNotEqual) {
    EXPECT_EQ(__cn_str_eq("abc", "abd"), 0);
}

// 空字符串比较
TEST(StringApiTest, EqEmpty) {
    EXPECT_EQ(__cn_str_eq("", ""), 1);
    EXPECT_EQ(__cn_str_eq("", "a"), 0);
}

// 空指针视为空串
TEST(StringApiTest, EqNull) {
    EXPECT_EQ(__cn_str_eq(nullptr, ""), 1);
    EXPECT_EQ(__cn_str_eq("a", nullptr), 0);
}

// ==================== 3. 字符串连接 ====================

// 连接：动态分配新串
TEST(StringApiTest, ConcatBasic) {
    char* result = __cn_str_concat("你", "好");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "你好");
    cn_free(result);
}

// 连接：空串与普通串
TEST(StringApiTest, ConcatWithEmpty) {
    char* result = __cn_str_concat("", "abc");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "abc");
    cn_free(result);
}

// 连接：空指针视为空串
TEST(StringApiTest, ConcatNull) {
    char* result = __cn_str_concat(nullptr, "x");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "x");
    cn_free(result);
}

// ==================== 4. 字符串复制 ====================

// 复制：深拷贝（内容一致，指针不同）
TEST(StringApiTest, CopyDeepCopy) {
    char* result = __cn_str_copy("副本");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "副本");
    // 修改原串不影响副本（深拷贝验证）
    EXPECT_NE(result, static_cast<void*>(const_cast<char*>("副本")));
    cn_free(result);
}

// 复制：空字符串
TEST(StringApiTest, CopyEmpty) {
    char* result = __cn_str_copy("");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "");
    cn_free(result);
}

// ==================== 5. 字符串查找 ====================

// 查找：子串位置
TEST(StringApiTest, FindSubstring) {
    EXPECT_EQ(__cn_str_find("hello world", "world"), 6);
    EXPECT_EQ(__cn_str_find("hello world", "hello"), 0);
}

// 查找：未找到返回 -1
TEST(StringApiTest, FindNotFound) {
    EXPECT_EQ(__cn_str_find("hello", "xyz"), -1);
}

// 查找：空子串返回0（C strstr 语义）
TEST(StringApiTest, FindEmptyNeedle) {
    EXPECT_EQ(__cn_str_find("hello", ""), 0);
}

// 查找：中文字符串
TEST(StringApiTest, FindUtf8) {
    EXPECT_EQ(__cn_str_find("你好世界", "世界"), 6);  // 你好=6字节，世界从偏移6起
}

// ==================== 6. 打印行多参数辅助 ====================

// 打印字符串（不换行）
TEST(StringApiTest, PrintStr) {
    std::string out = captureOutput([] { __cn_print_str("值:"); });
    EXPECT_EQ(out, "值:");
}

// 打印整数（不换行）
TEST(StringApiTest, PrintInt) {
    std::string out = captureOutput([] { __cn_print_int(42); });
    EXPECT_EQ(out, "42");
}

// 打印浮点（不换行，%f 语义）
TEST(StringApiTest, PrintFloat) {
    std::string out = captureOutput([] { __cn_print_float(3.5); });
    EXPECT_EQ(out, "3.500000");
}

// 换行
TEST(StringApiTest, PrintNewline) {
    std::string out = captureOutput([] { __cn_print_newline(); });
    EXPECT_EQ(out, "\n");
}

// 组合：模拟 打印行("值:", 42) 展开
TEST(StringApiTest, PrintLineMultiArgsExpansion) {
    std::string out = captureOutput([] {
        __cn_print_str("值:");
        __cn_print_int(42);
        __cn_print_newline();
    });
    EXPECT_EQ(out, "值:42\n");
}
