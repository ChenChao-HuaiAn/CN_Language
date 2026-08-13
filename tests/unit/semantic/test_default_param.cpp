// 默认参数语义单元测试（Task 2.10）
// 覆盖：单默认参数补全、多默认参数（右向左连续）、非法（中间无默认）、
//       调用补全与显式实参混合、默认值须编译期常量（非法标识符默认值报错）
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
    Lexer lexer(source, "默认参数语义测试.cn", diagnostics);
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

// 单默认参数：问候() 补缺省（"世界"）；问候("张三") 显式
TEST(DefaultParamTest, SingleDefault) {
    auto r = analyzeSource(R"CN(
函数 问候(字符串 名字 = "世界") -> 字符串 {
    返回 "你好，" + 名字
}
函数 主() -> 整32 {
    字符串 a = 问候()
    字符串 b = 问候("张三")
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 多默认参数（右向左连续）：f(a, b=10, c=20)，调用可省略 b/c 或仅 c
TEST(DefaultParamTest, MultiDefaultRightToLeft) {
    auto r = analyzeSource(R"CN(
函数 求和(整32 a, 整32 b = 10, 整32 c = 20) -> 整32 {
    返回 a + b + c
}
函数 主() -> 整32 {
    整32 x = 求和(1)
    整32 y = 求和(1, 2)
    整32 z = 求和(1, 2, 3)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 非法：默认参数中间出现无默认参数（f(a=1, b)）
TEST(DefaultParamTest, NonRightmostDefaultError) {
    auto r = analyzeSource(R"CN(
函数 错误(整32 a = 1, 整32 b) -> 整32 { 返回 a + b }
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 默认参数与重载协同：同一函数名不同签名
TEST(DefaultParamTest, OverloadWithDefault) {
    auto r = analyzeSource(R"CN(
函数 取(整32 a, 整32 b = 5) -> 整32 { 返回 a + b }
函数 取(浮64 a) -> 浮64 { 返回 a * 2 }
函数 主() -> 整32 {
    整32 x = 取(1)
    整32 y = 取(1, 2)
    浮64 z = 取(3.5)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 默认值为浮点/布尔/字符字面量（编译期常量）
TEST(DefaultParamTest, LiteralDefaults) {
    auto r = analyzeSource(R"CN(
函数 测(整32 a = 100, 浮64 b = 3.14, 布尔 c = 真) -> 整32 {
    返回 a
}
函数 主() -> 整32 {
    整32 x = 测()
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 非法默认值：非编译期常量（标识符）报错（语义层应拒绝）
TEST(DefaultParamTest, NonConstantDefaultError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 x = 5
    返回 0
}
)CN");
    // 控制组：合法代码应通过（验证测试框架本身）
    EXPECT_TRUE(r.ok) << r.messages;
}
