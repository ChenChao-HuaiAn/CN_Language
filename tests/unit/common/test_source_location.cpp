// 测试源码位置类：构造、toString、比较运算符
#include <gtest/gtest.h>
#include "cn_compiler/common/source_location.hpp"

using cn_compiler::SourceLocation;

// 测试默认构造与显式构造及getter访问
TEST(SourceLocationTest, 构造与访问) {
    SourceLocation 位置("测试.cn", 3, 7);
    EXPECT_EQ(位置.getFileName(), "测试.cn");
    EXPECT_EQ(位置.getLine(), 3);
    EXPECT_EQ(位置.getColumn(), 7);
}

// 测试toString格式化输出"文件:行:列"
TEST(SourceLocationTest, 格式化) {
    SourceLocation 位置("主.cn", 1, 1);
    EXPECT_EQ(位置.toString(), "主.cn:1:1");
}

// 测试无位置（默认构造）输出"未知位置"
TEST(SourceLocationTest, 默认位置) {
    SourceLocation 位置;
    EXPECT_EQ(位置.toString(), "未知位置");
}

// 测试比较运算符（相等/不等）
TEST(SourceLocationTest, 比较运算符) {
    SourceLocation 位置1("a.cn", 2, 3);
    SourceLocation 位置2("a.cn", 2, 3);
    SourceLocation 位置3("a.cn", 2, 4);
    SourceLocation 位置4("b.cn", 2, 3);
    EXPECT_TRUE(位置1 == 位置2);  // 完全相同
    EXPECT_TRUE(位置1 != 位置3);  // 列不同
    EXPECT_TRUE(位置1 != 位置4);  // 文件不同
}
