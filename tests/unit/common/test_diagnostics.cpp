// 测试诊断系统：report、hasErrors、getErrorCount、format、clear
// 注：测试名与变量名使用英文（GCC 7 不支持中文标识符），中文仅用于字符串与注释
#include <gtest/gtest.h>
#include "cn_compiler/common/diagnostics.hpp"

using cn_compiler::Diagnostic;
using cn_compiler::DiagnosticLevel;
using cn_compiler::Diagnostics;
using cn_compiler::SourceLocation;

// 测试报告错误后hasErrors为true且错误计数正确
TEST(DiagnosticsTest, ReportErrorMarksFailed) {
    Diagnostics diag;
    diag.report(DiagnosticLevel::Error, SourceLocation("测试.cn", 1, 1), "语法错误");
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.getErrorCount(), 1);
    EXPECT_EQ(diag.getWarningCount(), 0);
}

// 测试警告不导致hasErrors为true，但警告计数增加
TEST(DiagnosticsTest, WarningDoesNotFail) {
    Diagnostics diag;
    diag.report(DiagnosticLevel::Warning, SourceLocation("测试.cn", 1, 1), "未使用变量");
    EXPECT_FALSE(diag.hasErrors());
    EXPECT_EQ(diag.getErrorCount(), 0);
    EXPECT_EQ(diag.getWarningCount(), 1);
}

// 测试信息级别不影响错误/警告计数
TEST(DiagnosticsTest, InfoLevel) {
    Diagnostics diag;
    diag.report(DiagnosticLevel::Info, SourceLocation("测试.cn", 1, 1), "开始编译");
    EXPECT_FALSE(diag.hasErrors());
    EXPECT_EQ(diag.getErrorCount(), 0);
    EXPECT_EQ(diag.getWarningCount(), 0);
}

// 测试便捷构造器（Diagnostic::error/warning/info）
TEST(DiagnosticsTest, ConvenienceConstructors) {
    Diagnostics diag;
    diag.report(Diagnostic::error("测试.cn", 2, 5, "语法错误"));
    diag.report(Diagnostic::warning("测试.cn", 3, 1, "未使用变量"));
    diag.report(Diagnostic::info("测试.cn", 4, 1, "开始编译"));
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.getErrorCount(), 1);
    EXPECT_EQ(diag.getWarningCount(), 1);
    EXPECT_EQ(diag.getAll().size(), 3u);
}

// 测试format输出包含位置、级别与消息
TEST(DiagnosticsTest, FormatOutput) {
    Diagnostics diag;
    diag.report(DiagnosticLevel::Error, SourceLocation("测试.cn", 1, 1), "语法错误");
    const std::string output = diag.format();
    EXPECT_NE(output.find("测试.cn:1:1"), std::string::npos);  // 包含位置
    EXPECT_NE(output.find("错误"), std::string::npos);          // 包含级别
    EXPECT_NE(output.find("语法错误"), std::string::npos);      // 包含消息
}

// 测试clear清空所有诊断与计数
TEST(DiagnosticsTest, Clear) {
    Diagnostics diag;
    diag.report(DiagnosticLevel::Error, SourceLocation("测试.cn", 1, 1), "语法错误");
    EXPECT_TRUE(diag.hasErrors());
    diag.clear();
    EXPECT_FALSE(diag.hasErrors());
    EXPECT_EQ(diag.getErrorCount(), 0);
    EXPECT_EQ(diag.getWarningCount(), 0);
    EXPECT_TRUE(diag.getAll().empty());
}
