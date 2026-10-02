// 灰色点②方案A（2026-09-04 用户裁决立案）：字符串比较运算符显式拒绝 单元测试
// 覆盖：负形态精确消息断言（字符串==/!=/</>/<=/>= 字符串、字符* 交叉、字面量直比）+
//       合法形态零误伤（字符串比较/字符串字典序 内置、字符串 != 无 判空（133 EOF
//       载体）、真指针类型间判空比较、字符串+数值 拼接语境不受影响）。
// 规范依据：plans/001 Task 2.8——比较运算符仅定义整型与浮点变体，字符串不参与
//           运算符比较；等价能力由 字符串比较（相等）与 字符串字典序（全序）提供。
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

struct StrCmpResult {
    bool ok = false;
    int errorCount = 0;
    std::string messages;  // 全部诊断消息（拼接，含提示行）
};

StrCmpResult analyzeSource(const std::string& source) {
    StrCmpResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "字符串比较检查测试.cn", diagnostics);
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

} // namespace

// ==================== 962 轮（166 立法·用户裁决 2026-10-02）升级说明 ====================
// 字符串 ==/!= 内容比较+四序字典序立法（001 §比较语义修订版）——原 5 个拒绝
// 断言（灰色点②方案A）中 4 个字符串×字符串形态反转为放行；字符*×字符串 混合
// 维持拒绝（真指针语义收 不安全 域·新消息）。4 个 Allow 断言不受影响。

// ==================== 正形态：166 立法放行（原拒绝反转） ====================

// 变量 == 变量：内容比较（拼接产物静默陷阱已根治——IR 层 __cn_str_eq）
TEST(StringCompareCheckTest, AllowStringEqString166) {
    auto r = analyzeSource(
        R"CN(函数 好() -> 整32 { 字符串 a = "甲"; 字符串 b = "甲"; 如果 (a == b) { 返回 1; } 返回 0; })CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

TEST(StringCompareCheckTest, AllowStringNeString166) {
    auto r = analyzeSource(
        R"CN(函数 好() -> 整32 { 字符串 a = "甲"; 字符串 b = "乙"; 当 (a != b) { 返回 1; } 返回 0; })CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

TEST(StringCompareCheckTest, AllowLiteralEqLiteral166) {
    auto r = analyzeSource(
        R"CN(函数 好() -> 整32 { 如果 ("甲" == "乙") { 返回 1; } 返回 0; })CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

TEST(StringCompareCheckTest, AllowStringLtString166) {
    auto r = analyzeSource(
        R"CN(函数 好() -> 整32 { 字符串 a = "甲"; 字符串 b = "乙"; 如果 (a < b) { 返回 1; } 返回 0; })CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// ==================== 负形态：精确消息断言 ====================

// 字符*×字符串 混合维持拒绝（962：真指针语义收 不安全 域·字符串值比较请用 字符串 类型）
TEST(StringCompareCheckTest, RejectCharPtrEqString) {
    auto r = analyzeSource(
        R"CN(函数 坏(字符* p) -> 整32 { 如果 (p == "甲") { 返回 1; } 返回 0; })CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("字符指针之间不支持"), std::string::npos);
}

// ==================== 合法形态：零误伤 ====================

// 字符串比较/字符串字典序 内置（等价能力正路）
TEST(StringCompareCheckTest, AllowBuiltinStrEqAndCmp) {
    auto r = analyzeSource(
        R"CN(函数 好() -> 整32 { 字符串 a = "甲"; 字符串 b = "甲"; 如果 (字符串比较(a, b)) { 返回 1; } 如果 (字符串字典序(a, b) < 0) { 返回 2; } 返回 0; })CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 字符串 != 无 判空保留（133 EOF 载体、stdlib 读行判定）
TEST(StringCompareCheckTest, AllowStringNeNull) {
    auto r = analyzeSource(
        R"CN(函数 好(字符串 行) -> 整32 { 如果 (行 != 无) { 返回 1; } 返回 0; })CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 真指针类型间判空比较保留（p == 无 / p != 无）
TEST(StringCompareCheckTest, AllowPointerNullCompare) {
    auto r = analyzeSource(
        R"CN(函数 好(整64* p) -> 整32 { 如果 (p == 无) { 返回 1; } 如果 (p != 无) { 返回 2; } 返回 0; })CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 字符串拼接 + 数值语境不受影响（拼接语义与比较检查正交）
TEST(StringCompareCheckTest, AllowConcatUnaffected) {
    auto r = analyzeSource(
        R"CN(函数 好() -> 整32 { 字符串 s = "值=" + 5; 如果 (字符串长度(s) > 0) { 返回 1; } 返回 0; })CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}
