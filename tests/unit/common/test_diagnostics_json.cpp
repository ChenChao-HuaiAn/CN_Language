// F2-35（556-a）：诊断 JSON 机器可读输出测试——formatJson 序列化全形态锚定
//   （空诊断/单错误中文消息/多诊断/警告级别/JSON 特殊字符转义）
// 注：测试名与变量名使用英文（GCC 7 不支持中文标识符），中文仅用于字符串与注释
#include <gtest/gtest.h>
#include "cn_compiler/common/diagnostics.hpp"

using cn_compiler::Diagnostic;
using cn_compiler::DiagnosticLevel;
using cn_compiler::Diagnostics;
using cn_compiler::SourceLocation;

// 空诊断列表 -> 空数组
TEST(DiagnosticsJsonTest, EmptyDiagnosticsYieldEmptyArray) {
    Diagnostics diag;
    EXPECT_EQ(diag.formatJson(), "[]");
}

// 单错误（中文消息）-> 结构化对象：level/file/line/column/message 字段齐全
TEST(DiagnosticsJsonTest, SingleErrorWithChineseMessage) {
    Diagnostics diag;
    diag.report(DiagnosticLevel::Error, SourceLocation("测试.cn", 6, 13),
                "未声明的标识符 '未定义变量'");
    EXPECT_EQ(diag.formatJson(),
              "[{\"level\":\"错误\",\"file\":\"测试.cn\",\"line\":6,"
              "\"column\":13,\"message\":\"未声明的标识符 '未定义变量'\"}]");
}

// 多诊断（错误+警告混合）-> 逗号分隔数组且顺序保持
TEST(DiagnosticsJsonTest, MultipleDiagnosticsKeepOrder) {
    Diagnostics diag;
    diag.report(DiagnosticLevel::Warning, SourceLocation("甲.cn", 1, 1), "警告一");
    diag.report(DiagnosticLevel::Error, SourceLocation("乙.cn", 2, 5), "错误二");
    diag.report(DiagnosticLevel::Info, SourceLocation("丙.cn", 3, 9), "信息三");
    EXPECT_EQ(diag.formatJson(),
              "[{\"level\":\"警告\",\"file\":\"甲.cn\",\"line\":1,\"column\":1,"
              "\"message\":\"警告一\"},"
              "{\"level\":\"错误\",\"file\":\"乙.cn\",\"line\":2,\"column\":5,"
              "\"message\":\"错误二\"},"
              "{\"level\":\"信息\",\"file\":\"丙.cn\",\"line\":3,\"column\":9,"
              "\"message\":\"信息三\"}]");
}

// JSON 特殊字符转义：消息含引号/反斜杠/换行时按 RFC 8259 转义
TEST(DiagnosticsJsonTest, EscapesJsonSpecialCharacters) {
    Diagnostics diag;
    diag.report(DiagnosticLevel::Error, SourceLocation("引\"号\\.cn", 1, 1),
                "消息含\"引号\"与\\反斜杠与\n换行");
    const std::string json = diag.formatJson();
    // 引号转义：\"；反斜杠转义：\\；换行转义：\n（两字符序列）
    EXPECT_NE(json.find("\\\"引号\\\""), std::string::npos);
    EXPECT_NE(json.find("\\\\反斜杠"), std::string::npos);
    EXPECT_NE(json.find("\\n换行"), std::string::npos);
    // 转义后整体仍是合法 JSON 形态（以 [{ 开头、}] 结尾）
    EXPECT_EQ(json.substr(0, 2), "[{");
    EXPECT_EQ(json.substr(json.size() - 2), "}]");
}

// 文件名含 JSON 特殊字符时同样转义（file 字段）
TEST(DiagnosticsJsonTest, EscapesSpecialCharactersInFileName) {
    Diagnostics diag;
    diag.report(Diagnostic::error("带\"引号\"的文件.cn", 3, 7, "错误消息"));
    const std::string json = diag.formatJson();
    EXPECT_NE(json.find("带\\\"引号\\\"的文件.cn"), std::string::npos);
}
