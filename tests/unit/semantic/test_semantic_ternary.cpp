// 三元表达式语义检查单元测试（Task 2.9）
// 覆盖：条件必须布尔（合法/非法）、两分支数值宽化合并、两分支字符串一致、
//       分支类型不一致报错、三元嵌套、字符串+数值拼接合法
// 测试方式：Lexer + Parser + SemanticAnalyzer 全链路（非Mock）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
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

// 辅助：解析+语义分析源码，返回是否成功与诊断引擎
struct SemanticResult {
    bool ok = false;         // 语义分析是否成功
    int errorCount = 0;      // 错误数量
    std::string messages;    // 全部诊断消息（拼接）
};

SemanticResult analyzeSource(const std::string& source) {
    SemanticResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "三元语义测试.cn", diagnostics);
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

// 合法：条件为布尔 + 两分支字符串一致
TEST(SemanticTernaryTest, BoolCondStringBranchesOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 x = 10
    变量 t = x > 0 ? "正" : "负"
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 合法：数值分支宽化合并（整32 与 浮64 -> 浮64）
TEST(SemanticTernaryTest, NumericWidenBranchesOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 x = 10
    变量 t = x > 5 ? 100 : 2.5
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 合法：布尔字面量作条件 + 数值分支
TEST(SemanticTernaryTest, BoolLiteralCondOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    变量 n = 真 ? 1 : 2
    变量 m = 假 ? 3 : 4
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 合法：嵌套三元（外层条件为比较，内层条件为比较）
TEST(SemanticTernaryTest, NestedTernaryOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 x = 10
    变量 t = x > 0 ? (x > 5 ? "大" : "小") : "非正"
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 非法：条件非布尔（整数条件）
TEST(SemanticTernaryTest, NonBoolCondError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    变量 t = 5 ? "a" : "b"
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    // 错误消息应提到 三元
    EXPECT_NE(r.messages.find("三元"), std::string::npos) << r.messages;
}

// 非法：两分支类型不一致（字符串 vs 整32）
TEST(SemanticTernaryTest, MismatchedBranchTypesError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    变量 t = 真 ? "a" : 42
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 合法：字符串 + 数值 隐式拼接（Task 2.9）
TEST(SemanticTernaryTest, StringPlusNumberOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    打印("值" + 42)
    打印("浮" + 3.5)
    打印("布" + 真)
    打印("字" + 'A')
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 合法：打印 / 打印行 / 格式化 内置函数注册
TEST(SemanticTernaryTest, PrintAndFormatBuiltinsOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    打印("你好")
    打印行("不换行")
    字符串 s = 格式化("值%d", 42)
    字符串释放(s)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 非法：字符串 + 指针 拼接（右操作数为指针不拼接）
TEST(SemanticTernaryTest, StringPlusPointerError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32* p = 无
    打印("x" + p)
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}
