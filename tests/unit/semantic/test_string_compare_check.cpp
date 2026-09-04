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

// ==================== 负形态：精确消息断言 ====================

// 变量 == 变量：同内容异地址恒假（拼接产物静默陷阱——方案A 根治目标形态）
TEST(StringCompareCheckTest, RejectStringEqString) {
    auto r = analyzeSource(
        R"CN(函数 坏() -> 整32 { 字符串 a = "甲" 字符串 b = "甲" 如果 (a == b) { 返回 1 } 返回 0 })CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("字符串不支持 ==/!=/</> 等比较运算符（按地址比较而非内容）——"
                              "相等用 字符串比较(a,b)，全序用 字符串字典序(a,b)"),
              std::string::npos);
}

// 变量 != 变量（不等形态同拒）
TEST(StringCompareCheckTest, RejectStringNeString) {
    auto r = analyzeSource(
        R"CN(函数 坏() -> 整32 { 字符串 a = "甲" 字符串 b = "乙" 当 (a != b) { 返回 1 } 返回 0 })CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("字符串不支持 ==/!=/</> 等比较运算符"), std::string::npos);
}

// 字面量直比："甲" == "乙"（两侧均字符串字面量）
TEST(StringCompareCheckTest, RejectLiteralEqLiteral) {
    auto r = analyzeSource(
        R"CN(函数 坏() -> 整32 { 如果 ("甲" == "乙") { 返回 1 } 返回 0 })CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("字符串不支持 ==/!=/</> 等比较运算符"), std::string::npos);
}

// 顺序比较同拒（</> 家族——规范未定义变体，等价 字符串字典序）
TEST(StringCompareCheckTest, RejectStringLtString) {
    auto r = analyzeSource(
        R"CN(函数 坏() -> 整32 { 字符串 a = "甲" 字符串 b = "乙" 如果 (a < b) { 返回 1 } 返回 0 })CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("字符串不支持 ==/!=/</> 等比较运算符"), std::string::npos);
}

// 字符* 交叉同拒（字符* 承载字符串语义）
TEST(StringCompareCheckTest, RejectCharPtrEqString) {
    auto r = analyzeSource(
        R"CN(函数 坏(字符* p) -> 整32 { 如果 (p == "甲") { 返回 1 } 返回 0 })CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("字符串不支持 ==/!=/</> 等比较运算符"), std::string::npos);
}

// ==================== 合法形态：零误伤 ====================

// 字符串比较/字符串字典序 内置（等价能力正路）
TEST(StringCompareCheckTest, AllowBuiltinStrEqAndCmp) {
    auto r = analyzeSource(
        R"CN(函数 好() -> 整32 { 字符串 a = "甲" 字符串 b = "甲" 如果 (字符串比较(a, b)) { 返回 1 } 如果 (字符串字典序(a, b) < 0) { 返回 2 } 返回 0 })CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 字符串 != 无 判空保留（133 EOF 载体、stdlib 读行判定）
TEST(StringCompareCheckTest, AllowStringNeNull) {
    auto r = analyzeSource(
        R"CN(函数 好(字符串 行) -> 整32 { 如果 (行 != 无) { 返回 1 } 返回 0 })CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 真指针类型间判空比较保留（p == 无 / p != 无）
TEST(StringCompareCheckTest, AllowPointerNullCompare) {
    auto r = analyzeSource(
        R"CN(函数 好(整64* p) -> 整32 { 如果 (p == 无) { 返回 1 } 如果 (p != 无) { 返回 2 } 返回 0 })CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 字符串拼接 + 数值语境不受影响（拼接语义与比较检查正交）
TEST(StringCompareCheckTest, AllowConcatUnaffected) {
    auto r = analyzeSource(
        R"CN(函数 好() -> 整32 { 字符串 s = "值=" + 5 如果 (字符串长度(s) > 0) { 返回 1 } 返回 0 })CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}
