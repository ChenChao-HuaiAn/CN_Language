// 128 位静态初值解析单测（T46·467-a 根治）
// 背景：全局 128 位标量静态初始化发射缺失——原三后端 .data 128 分支硬编码
//   零占位（dq/.quad 0×2·无视 initText），负数字面量走 staticCtor 注入时
//   StorePtr 类型误标 "ptr" 半槽写（-5 读出 2^64−5）。修复=128 位 ±字面量
//   折叠进直存通道（发射层 parseInt128InitText 解析双 quad）+ StorePtr 传
//   IR 名触达三后端既有 i128 双槽写分支（含常量子分支）。
// 注意：GCC 无 UTF-8 标识符，测试名用英文（注释/字符串可中文）
#include <gtest/gtest.h>

#include <cstdio>
#include <string>

#include "cn_compiler/semantic/type_system.hpp"

using cn_compiler::types::isInt128Signed;
using cn_compiler::types::isInt128Type;
using cn_compiler::types::parseInt128InitText;

namespace {

// 解析并以 "lo:hi" 十六进制文本返回（便于断言）
std::string parseText(const std::string& text, bool isSigned, bool* okPtr) {
    unsigned long long lo = 0, hi = 0;
    const bool ok = parseInt128InitText(text, isSigned, lo, hi);
    if (okPtr != nullptr) *okPtr = ok;
    char buf[40];
    std::snprintf(buf, sizeof(buf), "%llx:%llx", lo, hi);
    return buf;
}

} // namespace

// 类型判定：中文/IR 名双口径 + 符号口径
TEST(Int128StaticInitTest, TypePredicates) {
    EXPECT_TRUE(isInt128Type("整128"));
    EXPECT_TRUE(isInt128Type("正128"));
    EXPECT_TRUE(isInt128Type("i128"));
    EXPECT_TRUE(isInt128Type("u128"));
    EXPECT_FALSE(isInt128Type("整64"));
    EXPECT_FALSE(isInt128Type("i64"));
    EXPECT_FALSE(isInt128Type("字符串"));
    EXPECT_TRUE(isInt128Signed("整128"));
    EXPECT_TRUE(isInt128Signed("i128"));
    EXPECT_FALSE(isInt128Signed("正128"));
    EXPECT_FALSE(isInt128Signed("u128"));
}

// 十进制直存（T46 实弹形态：42/-5/100）
TEST(Int128StaticInitTest, DecimalBasics) {
    bool ok = false;
    EXPECT_EQ(parseText("42", true, &ok), "2a:0");
    EXPECT_TRUE(ok);
    EXPECT_EQ(parseText("-5", true, &ok), "fffffffffffffffb:ffffffffffffffff");
    EXPECT_TRUE(ok);
    EXPECT_EQ(parseText("100", false, &ok), "64:0");
    EXPECT_TRUE(ok);
    EXPECT_EQ(parseText("0", true, &ok), "0:0");
    EXPECT_TRUE(ok);
    // 前导正号
    EXPECT_EQ(parseText("+7", true, &ok), "7:0");
    EXPECT_TRUE(ok);
}

// 前缀字面量：0x / 0b / 0o（IntegerLiteral raw 原文保留形态）
TEST(Int128StaticInitTest, PrefixedLiterals) {
    bool ok = false;
    EXPECT_EQ(parseText("0xDEADBEEFCAFEBABE", true, &ok),
              "deadbeefcafebabe:0");
    EXPECT_TRUE(ok);
    EXPECT_EQ(parseText("0xFF", true, &ok), "ff:0");
    EXPECT_TRUE(ok);
    // 0b101 = 5
    EXPECT_EQ(parseText("0b101", true, &ok), "5:0");
    EXPECT_TRUE(ok);
    // 0o17 = 15
    EXPECT_EQ(parseText("0o17", true, &ok), "f:0");
    EXPECT_TRUE(ok);
}

// 超出 int64 表示域的大字面量（128 位直存的核心价值面）
TEST(Int128StaticInitTest, BeyondInt64) {
    bool ok = false;
    // i128 最大值 = 2^127 − 1
    EXPECT_EQ(parseText("170141183460469231731687303715884105727", true, &ok),
              "ffffffffffffffff:7fffffffffffffff");
    EXPECT_TRUE(ok);
    // i128 最小值 = −2^127（two's complement 特例：|v| = 2^127 合法）
    EXPECT_EQ(parseText("-170141183460469231731687303715884105728", true, &ok),
              "0:8000000000000000");
    EXPECT_TRUE(ok);
    // u128 最大值 = 2^128 − 1（无符号口径）
    EXPECT_EQ(parseText("340282366920938463463374607431768211455", false, &ok),
              "ffffffffffffffff:ffffffffffffffff");
    EXPECT_TRUE(ok);
    // 2^64 + 1（低 64 位进位到高组）
    EXPECT_EQ(parseText("18446744073709551617", false, &ok), "1:1");
    EXPECT_TRUE(ok);
}

// 超范围/非法文本 → false（发射层保持零占位；超界拒绝归 T9 方案 D 辖区）
TEST(Int128StaticInitTest, OutOfRangeAndInvalid) {
    bool ok = true;
    // 有符号：正值 ≥ 2^127 超界
    parseText("170141183460469231731687303715884105728", true, &ok);
    EXPECT_FALSE(ok);
    // 有符号：负值 < −2^127 超界
    parseText("-170141183460469231731687303715884105729", true, &ok);
    EXPECT_FALSE(ok);
    // 无符号：负数直接非法
    parseText("-1", false, &ok);
    EXPECT_FALSE(ok);
    // 非法字符 / 空文本 / 纯符号（"12g8"：十进制上下文遇 g 非法）
    parseText("12g8", true, &ok);
    EXPECT_FALSE(ok);
    parseText("", true, &ok);
    EXPECT_FALSE(ok);
    parseText("-", true, &ok);
    EXPECT_FALSE(ok);
    parseText("0x", true, &ok);
    EXPECT_FALSE(ok);
}

// IR i128 常量池约定形态 "lo:hi"（两段十六进制·符号无关直通）
TEST(Int128StaticInitTest, ConstPoolForm) {
    bool ok = false;
    EXPECT_EQ(parseText("ff:0", true, &ok), "ff:0");
    EXPECT_TRUE(ok);
    EXPECT_EQ(parseText("0:8000000000000000", true, &ok),
              "0:8000000000000000");
    EXPECT_TRUE(ok);
}
