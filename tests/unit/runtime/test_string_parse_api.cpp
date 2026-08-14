// 字符串解析运行时 API 单元测试（Task 6.5，规格书10.1 字符串扩展）
// 覆盖：__cn_str_to_int（合法/非法/前导空格/空串/范围错误）/
//       __cn_str_to_double（合法/非法/前导空格/负号）/
//       __cn_str_to_bool（真/假/true/false/大小写/非法）
// 测试名英文（GCC 7 不支持中文标识符）；中文仅注释
#include <gtest/gtest.h>

#include <climits>
#include <cstring>
#include <string>

#include "runtime/runtime.hpp"

namespace {

// 合法输入解析辅助：断言成功标志=1 且值正确
void expectOkInt(const char* text, long long expected) {
    int ok = 0;
    const long long value = __cn_str_to_int(text, &ok);
    EXPECT_EQ(ok, 1) << "input: " << text;
    EXPECT_EQ(value, expected);
}

// 非法输入解析辅助：断言成功标志=0 且值归零
void expectFailInt(const char* text) {
    int ok = 1;
    const long long value = __cn_str_to_int(text, &ok);
    EXPECT_EQ(ok, 0) << "input: " << text;
    EXPECT_EQ(value, 0LL);
}

} // namespace

// ==================== 1. 字符串转整数 ====================

// 合法正整数
TEST(StringParseApiTest, StrToIntPositive) {
    expectOkInt("123", 123LL);
}

// 合法负整数
TEST(StringParseApiTest, StrToIntNegative) {
    expectOkInt("-42", -42LL);
}

// 前导空格 + 尾部空白（strtoll 允许首尾空白，C 语义）
TEST(StringParseApiTest, StrToIntLeadingTrailingSpace) {
    expectOkInt("  99  ", 99LL);
    expectOkInt("\t-7\n", -7LL);
}

// 整串范围最大值（LLONG_MAX）
TEST(StringParseApiTest, StrToIntMax) {
    expectOkInt("9223372036854775807", LLONG_MAX);
}

// 整串范围最小值（LLONG_MIN）
TEST(StringParseApiTest, StrToIntMin) {
    expectOkInt("-9223372036854775808", LLONG_MIN);
}

// 非法输入：纯字母
TEST(StringParseApiTest, StrToIntInvalidAlpha) {
    expectFailInt("abc");
}

// 非法输入：部分数字部分字母（拒绝部分解析）
TEST(StringParseApiTest, StrToIntPartialNumeric) {
    expectFailInt("123abc");
}

// 非法输入：空串
TEST(StringParseApiTest, StrToIntEmpty) {
    expectFailInt("");
}

// 非法输入：仅空白
TEST(StringParseApiTest, StrToIntWhitespaceOnly) {
    expectFailInt("   ");
}

// 范围错误：超出 LLONG_MAX（strtoll errno=ERANGE，成功标志0）
TEST(StringParseApiTest, StrToIntOverflow) {
    expectFailInt("9223372036854775808");
}

// 空指针输入视为空串（非法）
TEST(StringParseApiTest, StrToIntNull) {
    expectFailInt(nullptr);
}

// ==================== 2. 字符串转浮点 ====================

// 合法浮点（含小数）
TEST(StringParseApiTest, StrToDoubleBasic) {
    int ok = 0;
    const double value = __cn_str_to_double("3.14", &ok);
    EXPECT_EQ(ok, 1);
    EXPECT_DOUBLE_EQ(value, 3.14);
}

// 负数浮点
TEST(StringParseApiTest, StrToDoubleNegative) {
    int ok = 0;
    const double value = __cn_str_to_double("-0.5", &ok);
    EXPECT_EQ(ok, 1);
    EXPECT_DOUBLE_EQ(value, -0.5);
}

// 前导空格 + 尾部空白
TEST(StringParseApiTest, StrToDoubleSpaces) {
    int ok = 0;
    const double value = __cn_str_to_double("  2.5  ", &ok);
    EXPECT_EQ(ok, 1);
    EXPECT_DOUBLE_EQ(value, 2.5);
}

// 整数形态（strtod 接受无小数点）
TEST(StringParseApiTest, StrToDoubleIntegerForm) {
    int ok = 0;
    const double value = __cn_str_to_double("42", &ok);
    EXPECT_EQ(ok, 1);
    EXPECT_DOUBLE_EQ(value, 42.0);
}

// 非法输入：字母
TEST(StringParseApiTest, StrToDoubleInvalidAlpha) {
    int ok = 1;
    EXPECT_DOUBLE_EQ(__cn_str_to_double("abc", &ok), 0.0);
    EXPECT_EQ(ok, 0);
}

// 非法输入：部分数字（拒绝部分解析）
TEST(StringParseApiTest, StrToDoublePartialNumeric) {
    int ok = 1;
    EXPECT_DOUBLE_EQ(__cn_str_to_double("1.5xyz", &ok), 0.0);
    EXPECT_EQ(ok, 0);
}

// 非法输入：空串
TEST(StringParseApiTest, StrToDoubleEmpty) {
    int ok = 1;
    EXPECT_DOUBLE_EQ(__cn_str_to_double("", &ok), 0.0);
    EXPECT_EQ(ok, 0);
}

// ==================== 3. 字符串转布尔 ====================

// 中文 真 → 1
TEST(StringParseApiTest, StrToBoolChineseTrue) {
    int ok = 0;
    EXPECT_EQ(__cn_str_to_bool("真", &ok), 1);
    EXPECT_EQ(ok, 1);
}

// 中文 假 → 0（成功标志仍为1）
TEST(StringParseApiTest, StrToBoolChineseFalse) {
    int ok = 0;
    EXPECT_EQ(__cn_str_to_bool("假", &ok), 0);
    EXPECT_EQ(ok, 1);
}

// 英文小写 true/false
TEST(StringParseApiTest, StrToBoolEnglishLower) {
    int ok = 0;
    EXPECT_EQ(__cn_str_to_bool("true", &ok), 1);
    EXPECT_EQ(ok, 1);
    ok = 0;
    EXPECT_EQ(__cn_str_to_bool("false", &ok), 0);
    EXPECT_EQ(ok, 1);
}

// 英文大小写混合（True/False）
TEST(StringParseApiTest, StrToBoolEnglishMixedCase) {
    int ok = 0;
    EXPECT_EQ(__cn_str_to_bool("True", &ok), 1);
    EXPECT_EQ(ok, 1);
    ok = 0;
    EXPECT_EQ(__cn_str_to_bool("FALSE", &ok), 0);
    EXPECT_EQ(ok, 1);
}

// 非法输入：其他文本（成功标志0，值归零）
TEST(StringParseApiTest, StrToBoolInvalid) {
    int ok = 1;
    EXPECT_EQ(__cn_str_to_bool("yes", &ok), 0);
    EXPECT_EQ(ok, 0);
    ok = 1;
    EXPECT_EQ(__cn_str_to_bool("1", &ok), 0);
    EXPECT_EQ(ok, 0);
}

// 非法输入：空串/空指针
TEST(StringParseApiTest, StrToBoolEmptyAndNull) {
    int ok = 1;
    EXPECT_EQ(__cn_str_to_bool("", &ok), 0);
    EXPECT_EQ(ok, 0);
    ok = 1;
    EXPECT_EQ(__cn_str_to_bool(nullptr, &ok), 0);
    EXPECT_EQ(ok, 0);
}
