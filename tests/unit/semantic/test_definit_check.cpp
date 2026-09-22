// 010（2026-09-23·任务 plans/021 010·用户裁决甲）：局部变量未初始化读编译期拒绝
//   （def-init·Rust E0381 对照·零运行时成本）单元测试。
// 覆盖：拒绝矩阵（裸标量读/非常量下标读/字段读/复合赋值读/自增读/部分分支赋值
//       后读/循环位无初值读）+ 放行矩阵（先赋后读/两支赋值/数组元素先写后读/
//       拥有型裸声明豁免/静态豁免）+ 精确消息锚定。
// 权威锚定：Rust E0381（use of possibly-uninitialized variable，编译期硬错误）；
//           消息文本以 cn check 实测输出为准（2026-09-23 家机 win-x64 锚定）。
// 测试方式：Lexer + Parser 真实 AST -> SemanticAnalyzer 全链路（防虚假验收）。
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::SemanticAnalyzer;

namespace {

struct DefInitResult {
    bool ok = false;
    int errorCount = 0;
    std::string messages;
};

DefInitResult analyzeSource(const std::string& source) {
    DefInitResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "未初始化检查测试.cn", diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(diagnostics);
    auto program = parser.parse(tokens);
    SemanticAnalyzer analyzer(diagnostics);
    result.ok = analyzer.analyze(program.get());
    result.errorCount = diagnostics.getErrorCount();
    for (const auto& d : diagnostics.getAll()) {
        result.messages += d.message + "\n";
    }
    return result;
}

TEST(DefInitCheckTest, UninitScalarReadRejected) {
    auto r = analyzeSource(
        "函数 主() -> 整32 {\n"
        "    整32 x;\n"
        "    返回 x;\n"
        "}\n");
    EXPECT_EQ(r.errorCount, 1);
    EXPECT_NE(r.messages.find("未初始化，不能读取"), std::string::npos) << r.messages;
    EXPECT_NE(r.messages.find("'x'"), std::string::npos) << r.messages;
}

TEST(DefInitCheckTest, AssignThenReadAllowed) {
    auto r = analyzeSource(
        "函数 主() -> 整32 {\n"
        "    整32 x;\n"
        "    x = 5;\n"
        "    返回 x;\n"
        "}\n");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

TEST(DefInitCheckTest, ArrayElementWriteThenReadAllowed) {
    auto r = analyzeSource(
        "函数 主() -> 整32 {\n"
        "    整32[3] a;\n"
        "    a[0] = 1;\n"
        "    返回 a[0];\n"
        "}\n");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

TEST(DefInitCheckTest, ArrayUninitElementReadRejected) {
    auto r = analyzeSource(
        "函数 主() -> 整32 {\n"
        "    整32[3] a;\n"
        "    返回 a[0];\n"
        "}\n");
    EXPECT_EQ(r.errorCount, 1);
    EXPECT_NE(r.messages.find("'a[0]' 未初始化"), std::string::npos) << r.messages;
}

TEST(DefInitCheckTest, StructFieldAssignThenReadAllowed) {
    auto r = analyzeSource(
        "结构体 点 {\n"
        "    整32 横;\n"
        "    整32 纵;\n"
        "}\n"
        "函数 主() -> 整32 {\n"
        "    点 s;\n"
        "    s.横 = 1;\n"
        "    返回 s.横;\n"
        "}\n");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

TEST(DefInitCheckTest, StructBareDeclExemptZeroInit) {
    // 结构体裸声明=编译器入口零初始化注入（257 形二「字段数组零初始化」实锤·
    //   两侧既有防御）——字段读有定义行为，def-init 豁免（与拥有型同通道）
    auto r = analyzeSource(
        "结构体 点 {\n"
        "    整32 横;\n"
        "    整32 纵;\n"
        "}\n"
        "函数 主() -> 整32 {\n"
        "    点 s;\n"
        "    s.横 = 1;\n"
        "    返回 s.纵;\n"
        "}\n");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

TEST(DefInitCheckTest, CompoundAssignReadsUninitRejected) {
    auto r = analyzeSource(
        "函数 主() -> 整32 {\n"
        "    整32 x;\n"
        "    x += 1;\n"
        "    返回 0;\n"
        "}\n");
    EXPECT_EQ(r.errorCount, 1);
    EXPECT_NE(r.messages.find("未初始化"), std::string::npos) << r.messages;
}

TEST(DefInitCheckTest, IncDecReadsUninitRejected) {
    auto r = analyzeSource(
        "函数 主() -> 整32 {\n"
        "    整32 x;\n"
        "    x++;\n"
        "    返回 0;\n"
        "}\n");
    EXPECT_EQ(r.errorCount, 1);
    EXPECT_NE(r.messages.find("未初始化"), std::string::npos) << r.messages;
}

TEST(DefInitCheckTest, IfPartialAssignThenReadRejected) {
    auto r = analyzeSource(
        "函数 主() -> 整32 {\n"
        "    整32 x;\n"
        "    如果 (1 == 1) { x = 1; }\n"
        "    返回 x;\n"
        "}\n");
    EXPECT_EQ(r.errorCount, 1);
    EXPECT_NE(r.messages.find("未初始化"), std::string::npos) << r.messages;
}

TEST(DefInitCheckTest, IfBothBranchesAssignAllowed) {
    auto r = analyzeSource(
        "函数 主() -> 整32 {\n"
        "    整32 x;\n"
        "    如果 (1 == 1) { x = 1; } 否则 { x = 2; }\n"
        "    返回 x;\n"
        "}\n");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

TEST(DefInitCheckTest, LoopDeclWithoutInitRejected) {
    auto r = analyzeSource(
        "函数 主() -> 整32 {\n"
        "    循环 (整32 j; j < 3; j++) { }\n"
        "    返回 0;\n"
        "}\n");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("未初始化"), std::string::npos) << r.messages;
}

TEST(DefInitCheckTest, OwnedTypesBareDeclExempt) {
    // 拥有型（类/字符串/结果/可选）裸声明=默认初始化语义（入口零初始化/默认
    // 构造防御既有）——def-init 豁免面
    auto r = analyzeSource(
        "类 盒子 {\n"
        "    公开: 整32 值;\n"
        "}\n"
        "函数 主() -> 整32 {\n"
        "    盒子 b;\n"
        "    字符串 s;\n"
        "    s = \"文本\";\n"
        "    结果<整32, 整32> res;\n"
        "    b.值 = 5;\n"
        "    返回 b.值;\n"
        "}\n");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

TEST(DefInitCheckTest, ArrayArgPassThroughAllowed) {
    // 数组名作实参=指针退化（出参惯用法·38_tool 铁证）——不在读判定面
    auto r = analyzeSource(
        "不安全 函数 填充(整32* 缓冲, 整32 数) -> 整32 {\n"
        "    循环 (整32 i = 0; i < 数; i++) { 缓冲[i] = i; }\n"
        "    返回 0;\n"
        "}\n"
        "函数 主() -> 整32 {\n"
        "    整32[4] 数据;\n"
        "    填充(数据, 4);\n"
        "    返回 数据[0];\n"
        "}\n");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

TEST(DefInitCheckTest, UninitReadAfterTerminatingBranchAllowed) {
    // 一支以 返回 终止时，汇合状态只取另一支（return 终止支不参与归并）
    auto r = analyzeSource(
        "函数 主() -> 整32 {\n"
        "    整32 x;\n"
        "    如果 (2 > 1) { x = 1; } 否则 { 返回 9; }\n"
        "    返回 x;\n"
        "}\n");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

} // namespace
