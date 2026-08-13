// 运行时补充字符串API单元测试（Task 2.8）
// 覆盖：子串/字典序/大小写/前后缀/包含/修剪/反转/数字转换/字符转换/释放
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "runtime/runtime.hpp"

namespace {

// 辅助：动态分配字符串拷贝断言（使用 __cn_str_free 释放）
void expectStr(const char* actual, const char* expected) {
    EXPECT_STREQ(actual, expected);
}

} // namespace

// ==================== 1. 子串 ====================

// 子串：基本取段
TEST(StringApiExtraTest, SubBasic) {
    char* r = __cn_str_sub("Hello World", 6, 5);
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "World");
    __cn_str_free(r);
}

// 子串：UTF-8 中文（"你好世界" 字节：你=3 好=3 世=3 界=3）
TEST(StringApiExtraTest, SubUtf8) {
    char* r = __cn_str_sub("你好世界", 3, 3);
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "好");
    __cn_str_free(r);
}

// 子串：start 越界返回空串
TEST(StringApiExtraTest, SubStartOutOfRange) {
    char* r = __cn_str_sub("abc", 10, 3);
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "");
    __cn_str_free(r);
}

// 子串：len 超过剩余长度时截断
TEST(StringApiExtraTest, SubLenTruncate) {
    char* r = __cn_str_sub("abcdef", 2, 100);
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "cdef");
    __cn_str_free(r);
}

// 子串：负数按0处理
TEST(StringApiExtraTest, SubNegativeStart) {
    char* r = __cn_str_sub("abc", -1, 2);
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "ab");
    __cn_str_free(r);
}

// ==================== 2. 字典序比较 ====================

TEST(StringApiExtraTest, CmpLess) { EXPECT_LT(__cn_str_cmp("abc", "abd"), 0); }
TEST(StringApiExtraTest, CmpEqual) { EXPECT_EQ(__cn_str_cmp("abc", "abc"), 0); }
TEST(StringApiExtraTest, CmpGreater) { EXPECT_GT(__cn_str_cmp("abd", "abc"), 0); }

// 空指针视为空串
TEST(StringApiExtraTest, CmpNull) {
    EXPECT_EQ(__cn_str_cmp(nullptr, ""), 0);
    EXPECT_GT(__cn_str_cmp("a", nullptr), 0);
}

// ==================== 3. 大小写 ====================

TEST(StringApiExtraTest, UpperBasic) {
    char* r = __cn_str_upper("hello");
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "HELLO");
    __cn_str_free(r);
}

// 大写：非ASCII字节原样保留（"你好" 不变）
TEST(StringApiExtraTest, UpperUtf8Preserved) {
    char* r = __cn_str_upper("你好");
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "你好");
    __cn_str_free(r);
}

TEST(StringApiExtraTest, LowerBasic) {
    char* r = __cn_str_lower("WORLD");
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "world");
    __cn_str_free(r);
}

// ==================== 4. 前后缀 / 包含 ====================

TEST(StringApiExtraTest, StartsWithTrue) {
    EXPECT_EQ(__cn_str_starts_with("hello", "he"), 1);
}

TEST(StringApiExtraTest, StartsWithFalse) {
    EXPECT_EQ(__cn_str_starts_with("hello", "lo"), 0);
}

TEST(StringApiExtraTest, EndsWithTrue) {
    EXPECT_EQ(__cn_str_ends_with("hello", "lo"), 1);
}

TEST(StringApiExtraTest, EndsWithFalse) {
    EXPECT_EQ(__cn_str_ends_with("hello", "he"), 0);
}

TEST(StringApiExtraTest, ContainsTrue) {
    EXPECT_EQ(__cn_str_contains("hello", "ell"), 1);
}

TEST(StringApiExtraTest, ContainsFalse) {
    EXPECT_EQ(__cn_str_contains("hello", "xyz"), 0);
}

// 空前缀/空后缀/空needle 均匹配
TEST(StringApiExtraTest, PrefixSuffixEmpty) {
    EXPECT_EQ(__cn_str_starts_with("abc", ""), 1);
    EXPECT_EQ(__cn_str_ends_with("abc", ""), 1);
    EXPECT_EQ(__cn_str_contains("abc", ""), 1);
}

// ==================== 5. 修剪 ====================

// 修剪：去首尾空白
TEST(StringApiExtraTest, TrimBasic) {
    char* r = __cn_str_trim("  你好  ");
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "你好");
    __cn_str_free(r);
}

// 修剪：制表/换行/回车
TEST(StringApiExtraTest, TrimTabsNewlines) {
    char* r = __cn_str_trim("\t\nabc\r\n ");
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "abc");
    __cn_str_free(r);
}

// 修剪：全空白 -> 空串
TEST(StringApiExtraTest, TrimAllWhitespace) {
    char* r = __cn_str_trim("   ");
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "");
    __cn_str_free(r);
}

// ==================== 6. 反转 ====================

TEST(StringApiExtraTest, ReverseAscii) {
    char* r = __cn_str_reverse("abc");
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "cba");
    __cn_str_free(r);
}

// 反转：UTF-8 安全（"你好" -> "好你"，不拆多字节字符）
TEST(StringApiExtraTest, ReverseUtf8) {
    char* r = __cn_str_reverse("你好");
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "好你");
    __cn_str_free(r);
}

// 反转：空串
TEST(StringApiExtraTest, ReverseEmpty) {
    char* r = __cn_str_reverse("");
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "");
    __cn_str_free(r);
}

// ==================== 7. 数字转字符串 ====================

TEST(StringApiExtraTest, FromIntPositive) {
    char* r = __cn_str_from_int(12345);
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "12345");
    __cn_str_free(r);
}

TEST(StringApiExtraTest, FromIntNegative) {
    char* r = __cn_str_from_int(-42);
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "-42");
    __cn_str_free(r);
}

TEST(StringApiExtraTest, FromIntZero) {
    char* r = __cn_str_from_int(0);
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "0");
    __cn_str_free(r);
}

TEST(StringApiExtraTest, FromFloat) {
    char* r = __cn_str_from_float(3.5);
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "3.500000");
    __cn_str_free(r);
}

// ==================== 8. 字符转字符串 ====================

TEST(StringApiExtraTest, FromChar) {
    char* r = __cn_str_from_char('A');
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "A");
    __cn_str_free(r);
}

TEST(StringApiExtraTest, FromCharNul) {
    char* r = __cn_str_from_char(0);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r[0], '\0');
    EXPECT_EQ(r[1], '\0');
    __cn_str_free(r);
}

// ==================== 9. 释放 ====================

// 释放：nullptr 安全（free 语义）
TEST(StringApiExtraTest, FreeNullSafe) {
    __cn_str_free(nullptr);  // 不崩溃即通过
}

// 释放：动态分配串释放后无泄漏（配合连接/子串使用）
TEST(StringApiExtraTest, FreeAfterConcat) {
    char* r = __cn_str_concat("A", "B");
    ASSERT_NE(r, nullptr);
    EXPECT_STREQ(r, "AB");
    __cn_str_free(r);
}
