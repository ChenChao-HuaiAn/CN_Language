// 缺陷②根治（2026-09-03 用户裁决立案）：赋值目标非左值统一拒绝 单元测试
// 覆盖：八种负形态精确消息断言（二元/复合/字面量/自增非左值/常量赋值/常量自增/
//       一元负号/非引用返回调用）+ 换行粘连形态「想写分号」诊断③提示串 +
//       合法形态零误伤（变量/成员/下标/解引用 × 简单/复合/自增全形态 + 常量读取）。
// 权威锚定：Rust E0070（invalid left-hand side of assignment，编译期硬错误）；
//           消息文本以 cn check 实测输出为准（2026-09-03 家机 win-x64 锚定）。
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

struct LvalueResult {
    bool ok = false;
    int errorCount = 0;
    std::string messages;  // 全部诊断消息（拼接，含提示行）
};

LvalueResult analyzeSource(const std::string& source) {
    LvalueResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "左值检查测试.cn", diagnostics);
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

// 二元运算结果赋值：a + 1 = 7（Rust E0070 同款硬错误）
TEST(LvalueCheckTest, RejectBinaryResultAssignment) {
    auto r = analyzeSource(R"CN(不安全 函数 坏(整64 a) -> 整32 { a + 1 = 7; 返回 0; })CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find(
                  "赋值目标必须是可赋值的左值（变量/成员/下标/解引用/引用返回调用），"
                  "不能给运算结果表达式赋值"),
              std::string::npos);
}

// 复合赋值目标同为非左值：a + 1 += 2
TEST(LvalueCheckTest, RejectCompoundOnBinaryResult) {
    auto r = analyzeSource(R"CN(不安全 函数 坏(整64 a) -> 整32 { a + 1 += 2; 返回 0; })CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find(
                  "赋值目标必须是可赋值的左值（变量/成员/下标/解引用/引用返回调用），"
                  "不能给运算结果表达式赋值"),
              std::string::npos);
}

// 字面量赋值：5 = a
TEST(LvalueCheckTest, RejectLiteralAssignment) {
    auto r = analyzeSource(R"CN(不安全 函数 坏(整64 a) -> 整32 { 5 = a; 返回 0; })CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("不能给字面量赋值"), std::string::npos);
}

// 自增目标非左值：(a + 1)++（自增是隐式赋值）
TEST(LvalueCheckTest, RejectIncrementOnNonLvalue) {
    auto r = analyzeSource(R"CN(不安全 函数 坏(整64 a) -> 整32 { (a + 1)++; 返回 0; })CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find(
                  "自增/自减目标必须是可赋值的左值（变量/成员/下标/解引用）"),
              std::string::npos);
}

// 一元负号结果赋值：-a = 7（非解引用一元不可赋值）
TEST(LvalueCheckTest, RejectUnaryResultAssignment) {
    auto r = analyzeSource(R"CN(不安全 函数 坏(整64 a) -> 整32 { -a = 7; 返回 0; })CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("不能给一元运算结果赋值"), std::string::npos);
}

// 非引用返回调用赋值：取值() = 5
TEST(LvalueCheckTest, RejectNonRefCallAssignment) {
    auto r = analyzeSource(R"CN(
不安全 函数 取值() -> 整32 { 返回 1; }
不安全 函数 坏() -> 整32 { 取值() = 5; 返回 0; })CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find(
                  "赋值目标须为可写左值（标识符/下标/解引用/成员/引用返回调用）"),
              std::string::npos);
}

// 局部常量重赋值：常量 c = 1; c = 2（[03] 规范：常量初始化后不可修改）
TEST(LvalueCheckTest, RejectConstReassignment) {
    auto r = analyzeSource(R"CN(不安全 函数 坏() -> 整32 { 常量 c = 1; c = 2; 返回 0; })CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("不能给常量 'c' 赋值（常量初始化后不可修改）"),
              std::string::npos);
}

// 常量自增：常量 c = 1; c++
TEST(LvalueCheckTest, RejectConstIncrement) {
    auto r = analyzeSource(R"CN(不安全 函数 坏() -> 整32 { 常量 c = 1; c++; 返回 0; })CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("不能对常量 'c' 自增/自减（常量初始化后不可修改）"),
              std::string::npos);
}

// ==================== 诊断③：跨行赋值「想写分号」提示 ====================

// 换行≡空格粘连形态：b = a 换行 *a = 7 粘连为 b = ((a*a) = 7)——
// 内层非左值赋值拒绝 + 附 rustc「you might have meant to write a semicolon
// here」同款提示（赋值号行号 > 目标起始行号）
TEST(LvalueCheckTest, CrossLineAssignmentHint) {
    auto r = analyzeSource(
        "不安全 函数 坏(整64 a) -> 整32 {\n"
        "    变量 b = 0;\n"
        "    b = a\n"
        "    *a = 7;\n"
        "    返回 0;\n"
        "}\n");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("不能给运算结果表达式赋值"), std::string::npos);
    EXPECT_NE(r.messages.find("请在上一行末尾加分号"), std::string::npos);
}

// ==================== 合法形态零误伤 ====================

// 可写左值全形态：变量/成员/下标/解引用 × 简单/复合/自增 + 常量读取 + 引用参数——
// 132 用例同族正例（v2 闭环已绿；此处宿主语义层零误伤断言）
TEST(LvalueCheckTest, LegalLvalueFormsNoFalsePositive) {
    auto r = analyzeSource(
        "结构体 点 {\n"
        "    整64 x;\n"
        "    整64 y;\n"
        "}\n"
        "不安全 函数 写透(整64* 目标) -> 整32 { 目标[0] = 目标[0] + 1; 返回 0; }\n"
        "不安全 函数 主() -> 整32 {\n"
        "    整64 甲 = 10;\n"
        "    甲 = 20;\n"
        "    甲 += 5;\n"
        "    甲++;\n"
        "    点 p;\n"
        "    p.x = 7;\n"
        "    p.y = 3;\n"
        "    p.x += p.y;\n"
        "    p.y--;\n"
        "    整64[3] 数组;\n"
        "    数组[1] = 5;\n"
        "    数组[2] = 5;\n"       // 010（def-init）：下标左值 += 的前提=目标已初始化
        "    数组[2] += 数组[1];\n"
        "    数组[1]++;\n"
        "    整64 v = 8;\n"
        "    整64* q = &v;\n"
        "    *q = 100;\n"
        "    (*q)++;\n"
        "    写透(q);\n"
        "    常量 系数 = 2;\n"
        "    整64 乙 = 系数 * 甲;\n"
        "    返回 0;\n"
        "}\n");
    EXPECT_EQ(r.errorCount, 0) << "合法形态被误伤，诊断：\n" << r.messages;
}

// 顶层常量跨函数读取合法 + 内层变量遮蔽外层常量按内层解析（可赋值）
TEST(LvalueCheckTest, ConstReadAndShadowingLegal) {
    auto r = analyzeSource(
        "常量 系数 = 3;\n"
        "不安全 函数 主() -> 整32 {\n"
        "    整32 甲 = 系数 * 2;\n"
        "    返回 甲;\n"
        "}\n");
    EXPECT_EQ(r.errorCount, 0) << "顶层常量读取被误伤，诊断：\n" << r.messages;
}
