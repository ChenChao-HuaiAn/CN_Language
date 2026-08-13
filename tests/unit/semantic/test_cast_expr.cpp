// 强制类型转换语义单元测试（Task 2.10）
// 覆盖：整↔浮、浮→整截断、指针↔整数、字符↔整、窄化显式合法、
//       非法转换（字符串→整、结构体→整）报错、类型名(...) 解析歧义
// 测试方式：Lexer + Parser + SemanticAnalyzer 全链路（非Mock）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::SemanticAnalyzer;

namespace {

struct SemanticResult {
    bool ok = false;
    int errorCount = 0;
    std::string messages;
};

SemanticResult analyzeSource(const std::string& source) {
    SemanticResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "强制转换语义测试.cn", diagnostics);
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

// 整↔浮显式转换：浮64(3)/4、整32(3.99)（截断）、浮64(整32(7))
TEST(CastExprTest, IntFloatConversions) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    浮64 比例 = 浮64(3) / 4
    整32 截断 = 整32(3.99)
    浮64 转回 = 浮64(整32(7))
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 指针↔整数显式转换（位重解释）
TEST(CastExprTest, PointerIntConversions) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整64 地址值 = 123456
    指针 通用指针 = 指针(地址值)
    整64 回读 = 整64(通用指针)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 窄化显式合法（整64 → 整32、浮64 → 浮32）
TEST(CastExprTest, NarrowingExplicitOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整64 大 = 10000000000L
    整32 小 = 整32(大)
    浮64 d = 3.14159
    浮32 f = 浮32(d)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 字符↔整显式转换
TEST(CastExprTest, CharIntConversion) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 码 = 整32('A')
    字符 字 = 字符(码)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 非法：字符串 → 整32（非数值/指针族）报错
TEST(CastExprTest, StringToIntError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 x = 整32("abc")
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 非法：布尔 → 整32？—— 布尔为 i1，数值族，允许显式（不报错控制组）
// 非法：结构体 → 整32 报错
TEST(CastExprTest, StructToIntError) {
    auto r = analyzeSource(R"CN(
结构体 点 { 整32 x; 整32 y }
函数 主() -> 整32 {
    点 p = 点{ x = 1, y = 2 }
    整32 v = 整32(p)
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 解析歧义：类型名(表达式) 判为 Cast；函数名(实参) 判为 Call（不冲突）
TEST(CastExprTest, ParseAmbiguityResolved) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 x = 整32(3.99)     // 类型关键字 -> Cast
    打印("值", x)           // 函数名 -> Call
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 回归（审查修复 BUG#2）：布尔 -> 整数 显式转换合法
//   原实现 srcNumeric 判断漏"布尔"，导致 整32(真)/整64(假) 被误拒。
TEST(CastExprTest, BoolToIntConversion) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 a = 整32(真)
    整64 b = 整64(假)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 强制转换在表达式上下文（函数实参内嵌）
TEST(CastExprTest, CastInArgContext) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    打印("比例", 浮64(3) / 4)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}
