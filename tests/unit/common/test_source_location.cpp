// 测试源码位置类：构造、toString、比较运算符
// 注：测试名与变量名使用英文（GCC 7 不支持中文标识符），中文仅用于字符串与注释
#include <gtest/gtest.h>
#include "cn_compiler/common/source_location.hpp"

using cn_compiler::SourceLocation;

// 测试默认构造与显式构造及getter访问
TEST(SourceLocationTest, ConstructionAndAccess) {
    SourceLocation loc("测试.cn", 3, 7);
    EXPECT_EQ(loc.getFileName(), "测试.cn");
    EXPECT_EQ(loc.getLine(), 3);
    EXPECT_EQ(loc.getColumn(), 7);
}

// 测试toString格式化输出"文件:行:列"
TEST(SourceLocationTest, Formatting) {
    SourceLocation loc("主.cn", 1, 1);
    EXPECT_EQ(loc.toString(), "主.cn:1:1");
}

// 测试无位置（默认构造）输出"未知位置"
TEST(SourceLocationTest, DefaultLocation) {
    SourceLocation loc;
    EXPECT_EQ(loc.toString(), "未知位置");
}

// 测试比较运算符（相等/不等）
TEST(SourceLocationTest, ComparisonOperators) {
    SourceLocation loc1("a.cn", 2, 3);
    SourceLocation loc2("a.cn", 2, 3);
    SourceLocation loc3("a.cn", 2, 4);
    SourceLocation loc4("b.cn", 2, 3);
    EXPECT_TRUE(loc1 == loc2);  // 完全相同
    EXPECT_TRUE(loc1 != loc3);  // 列不同
    EXPECT_TRUE(loc1 != loc4);  // 文件不同
}
