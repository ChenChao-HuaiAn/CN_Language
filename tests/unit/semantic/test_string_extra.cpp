// 语义分析器补充字符串API单元测试（Task 2.8）
// 覆盖：新 API 注册（子串/字典序/大小写/前后缀/包含/修剪/反转/数字转换/字符转换/释放）
//       类型检查、参数数量检查
// 测试方式：通过 Lexer + Parser 得到真实AST，再交给 SemanticAnalyzer 分析（全链路）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;
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
    Lexer lexer(source, "字符串完善语义测试.cn", diagnostics);
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

// ==================== 1. 新 API 注册与类型检查 ====================

// 子串：字符串子串(字符串, 整64, 整64) -> 字符串
TEST(StringExtraSemanticTest, SubRegister) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 s = 字符串子串("Hello", 1, 2)
    字符串释放(s)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 字典序：字符串字典序(字符串, 字符串) -> 整64
TEST(StringExtraSemanticTest, CmpRegister) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整64 c = 字符串字典序("a", "b")
    打印(c)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 大小写/修剪/反转：返回字符串，可释放
TEST(StringExtraSemanticTest, TransformRegister) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 a = 字符串大写("hi")
    字符串 b = 字符串小写("HI")
    字符串 c = 字符串修剪("  x  ")
    字符串 d = 字符串反转("abc")
    字符串释放(a)
    字符串释放(b)
    字符串释放(c)
    字符串释放(d)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 前后缀/包含：返回布尔，可用于 如果 条件
TEST(StringExtraSemanticTest, PredicateRegister) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    如果 (字符串前缀("hello", "he")) { 打印行("前") }
    如果 (字符串后缀("hello", "lo")) { 打印行("后") }
    如果 (字符串包含("hello", "ell")) { 打印行("含") }
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 数字/字符转换：返回字符串
TEST(StringExtraSemanticTest, ConvertRegister) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 i = 整数转字符串(42)
    字符串 f = 浮点转字符串(3.5)
    字符串 c = 字符转字符串('A')
    字符串释放(i)
    字符串释放(f)
    字符串释放(c)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// ==================== 2. 类型检查错误 ====================

// 子串：第一个参数必须为字符串（传整数报错）
TEST(StringExtraSemanticTest, SubTypeError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 s = 字符串子串(42, 1, 2)
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 子串：参数数量错误（只给2个）
TEST(StringExtraSemanticTest, SubArityError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 s = 字符串子串("Hello", 1)
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 释放：参数必须为字符串（传整数报错）
TEST(StringExtraSemanticTest, FreeTypeError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串释放(42)
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 释放：返回空类型，不能赋值
TEST(StringExtraSemanticTest, FreeVoidReturn) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 s = 字符串释放("x")
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 未注册的函数名调用报错（防御：非内置函数名）
TEST(StringExtraSemanticTest, UnknownFunctionError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 s = 字符串不存在("x")
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}
