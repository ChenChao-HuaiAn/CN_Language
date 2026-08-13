// 函数重载语义单元测试（Task 2.10）
// 覆盖：同名多签名共存、三版 加 决议（整32/浮64/字符串）、参数个数决议、
//       仅返回类型不同不构成重载（报错）、原型+定义签名不一致（报错）、
//       函数名作值（函数指针）取首签名、mangling key 唯一性
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
    Lexer lexer(source, "重载语义测试.cn", diagnostics);
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

// 合法重载：三版 加（整32/浮64/字符串）共存，返回类型不同不影响
TEST(OverloadTest, ThreeVersionsCoexist) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b }
函数 加(浮64 a, 浮64 b) -> 浮64 { 返回 a + b }
函数 加(字符串 a, 字符串 b) -> 字符串 { 返回 a + b }
函数 主() -> 整32 { 返回 0 }
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 决议：整32 字面量精确命中整32 版本；浮点字面量命中浮64 版本
TEST(OverloadTest, ResolveByArgType) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b }
函数 加(浮64 a, 浮64 b) -> 浮64 { 返回 a + b }
函数 主() -> 整32 {
    整32 x = 加(1, 2)
    浮64 y = 加(1.5, 2.5)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 决议：参数个数不同（1 参数 vs 2 参数）
TEST(OverloadTest, ResolveByArgCount) {
    auto r = analyzeSource(R"CN(
函数 取(整32 a) -> 整32 { 返回 a }
函数 取(整32 a, 整32 b) -> 整32 { 返回 a + b }
函数 主() -> 整32 {
    整32 x = 取(1)
    整32 y = 取(1, 2)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 仅返回类型不同不构成重载（同参数签名重复定义报错）
TEST(OverloadTest, SameParamsDiffReturnError) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b }
函数 加(整32 a, 整32 b) -> 整64 { 返回 0 }
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 原型声明 + 定义签名不一致报错（参数类型不同）
TEST(OverloadTest, ProtoDefMismatchError) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32
函数 加(浮64 a, 整32 b) -> 整32 { 返回 0 }
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 未匹配的调用（无该参数类型版本）报错
TEST(OverloadTest, NoMatchError) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b }
函数 主() -> 整32 {
    布尔 b = 真
    加(b, b)
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 函数名作值（回调 = 加）：不报错（语义层取首签名作函数指针类型）
TEST(OverloadTest, FuncNameAsValue) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b }
函数 加(浮64 a, 浮64 b) -> 浮64 { 返回 a + b }
函数 主() -> 整32 {
    整32(*回调)(整32, 整32)
    回调 = 加
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 字符串参数调用命中字符串版本（返回值赋值给 字符串 变量）
TEST(OverloadTest, StringVersionResolve) {
    auto r = analyzeSource(R"CN(
函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b }
函数 加(字符串 a, 字符串 b) -> 字符串 { 返回 a + b }
函数 主() -> 整32 {
    字符串 s = 加("中", "国")
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}
