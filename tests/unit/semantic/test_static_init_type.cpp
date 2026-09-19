// 静态标量初值判定单测（331-a·T50 根治）
// 背景：函数内静态局部的 IR 类型名（"i64"，mapType 产物）与顶层静态的中文名
//   （"整64"，语义层产物）走同一 .data 初值直存判定——原三后端口径分叉
//   （linux_x64/arm64 用 types::isInteger 只认中文名 → 函数内静态初值恒 .quad 0
//   静默丢；win 用「非浮点文本」判定而幸存）。修复=单一归属
//   types::isStaticScalarInitType（双口径）。
// 注意：GCC 无 UTF-8 标识符，测试名用英文（注释/字符串可中文）
#include <gtest/gtest.h>

#include "cn_compiler/semantic/type_system.hpp"

using cn_compiler::types::isStaticScalarInitType;

// 中文类型名（顶层静态路径）：整族 + 字符/布尔 → 可直存
TEST(StaticInitTypeTest, AcceptsChineseScalarNames) {
    EXPECT_TRUE(isStaticScalarInitType("整8"));
    EXPECT_TRUE(isStaticScalarInitType("整16"));
    EXPECT_TRUE(isStaticScalarInitType("整32"));
    EXPECT_TRUE(isStaticScalarInitType("整64"));
    EXPECT_TRUE(isStaticScalarInitType("正8"));
    EXPECT_TRUE(isStaticScalarInitType("正64"));
    EXPECT_TRUE(isStaticScalarInitType("整128"));
    EXPECT_TRUE(isStaticScalarInitType("正128"));
    EXPECT_TRUE(isStaticScalarInitType("字符"));
    EXPECT_TRUE(isStaticScalarInitType("布尔"));
    // 别名口径（canonical 规范化）
    EXPECT_TRUE(isStaticScalarInitType("整数"));
}

// IR 类型名（函数内静态局部路径·T50 根因面）→ 可直存
TEST(StaticInitTypeTest, AcceptsIrScalarNames) {
    EXPECT_TRUE(isStaticScalarInitType("i8"));
    EXPECT_TRUE(isStaticScalarInitType("i16"));
    EXPECT_TRUE(isStaticScalarInitType("i32"));
    EXPECT_TRUE(isStaticScalarInitType("i64"));
    EXPECT_TRUE(isStaticScalarInitType("u8"));
    EXPECT_TRUE(isStaticScalarInitType("u16"));
    EXPECT_TRUE(isStaticScalarInitType("u32"));
    EXPECT_TRUE(isStaticScalarInitType("u64"));
    EXPECT_TRUE(isStaticScalarInitType("i1"));
}

// 非标量（浮点/i128/字符串/指针）→ 不可直存（各走专用分支或运行期注入）
TEST(StaticInitTypeTest, RejectsNonScalarInit) {
    EXPECT_FALSE(isStaticScalarInitType("浮32"));
    EXPECT_FALSE(isStaticScalarInitType("浮64"));
    EXPECT_FALSE(isStaticScalarInitType("i128"));
    EXPECT_FALSE(isStaticScalarInitType("u128"));
    EXPECT_FALSE(isStaticScalarInitType("字符串"));
    EXPECT_FALSE(isStaticScalarInitType("整32*"));
    EXPECT_FALSE(isStaticScalarInitType(""));
}
