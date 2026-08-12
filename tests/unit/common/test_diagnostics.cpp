// 测试诊断系统：report、hasErrors、getErrorCount、format、clear
#include <gtest/gtest.h>
#include "cn_compiler/common/diagnostics.hpp"

using cn_compiler::Diagnostic;
using cn_compiler::DiagnosticLevel;
using cn_compiler::Diagnostics;
using cn_compiler::SourceLocation;

// 测试报告错误后hasErrors为true且错误计数正确
TEST(DiagnosticsTest, 报告错误后标记失败) {
    Diagnostics 引擎;
    引擎.report(DiagnosticLevel::Error, SourceLocation("测试.cn", 1, 1), "语法错误");
    EXPECT_TRUE(引擎.hasErrors());
    EXPECT_EQ(引擎.getErrorCount(), 1);
    EXPECT_EQ(引擎.getWarningCount(), 0);
}

// 测试警告不导致hasErrors为true，但警告计数增加
TEST(DiagnosticsTest, 警告不导致失败) {
    Diagnostics 引擎;
    引擎.report(DiagnosticLevel::Warning, SourceLocation("测试.cn", 1, 1), "未使用变量");
    EXPECT_FALSE(引擎.hasErrors());
    EXPECT_EQ(引擎.getErrorCount(), 0);
    EXPECT_EQ(引擎.getWarningCount(), 1);
}

// 测试信息级别不影响错误/警告计数
TEST(DiagnosticsTest, 信息级别) {
    Diagnostics 引擎;
    引擎.report(DiagnosticLevel::Info, SourceLocation("测试.cn", 1, 1), "开始编译");
    EXPECT_FALSE(引擎.hasErrors());
    EXPECT_EQ(引擎.getErrorCount(), 0);
    EXPECT_EQ(引擎.getWarningCount(), 0);
}

// 测试便捷构造器（Diagnostic::error/warning/info）
TEST(DiagnosticsTest, 便捷构造器) {
    Diagnostics 引擎;
    引擎.report(Diagnostic::error("测试.cn", 2, 5, "语法错误"));
    引擎.report(Diagnostic::warning("测试.cn", 3, 1, "未使用变量"));
    引擎.report(Diagnostic::info("测试.cn", 4, 1, "开始编译"));
    EXPECT_TRUE(引擎.hasErrors());
    EXPECT_EQ(引擎.getErrorCount(), 1);
    EXPECT_EQ(引擎.getWarningCount(), 1);
    EXPECT_EQ(引擎.getAll().size(), 3u);
}

// 测试format输出包含位置、级别与消息
TEST(DiagnosticsTest, 格式化输出) {
    Diagnostics 引擎;
    引擎.report(DiagnosticLevel::Error, SourceLocation("测试.cn", 1, 1), "语法错误");
    const std::string 输出 = 引擎.format();
    EXPECT_NE(输出.find("测试.cn:1:1"), std::string::npos);  // 包含位置
    EXPECT_NE(输出.find("错误"), std::string::npos);          // 包含级别
    EXPECT_NE(输出.find("语法错误"), std::string::npos);      // 包含消息
}

// 测试clear清空所有诊断与计数
TEST(DiagnosticsTest, 清空) {
    Diagnostics 引擎;
    引擎.report(DiagnosticLevel::Error, SourceLocation("测试.cn", 1, 1), "语法错误");
    EXPECT_TRUE(引擎.hasErrors());
    引擎.clear();
    EXPECT_FALSE(引擎.hasErrors());
    EXPECT_EQ(引擎.getErrorCount(), 0);
    EXPECT_EQ(引擎.getWarningCount(), 0);
    EXPECT_TRUE(引擎.getAll().empty());
}
