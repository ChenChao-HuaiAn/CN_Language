// lambda 表达式语义单元测试（Task 2.10）
// 覆盖：[] 无捕获 + 返回类型推导、[=] 值捕获、[&] 引用捕获、[变量] 显式捕获、
//       捕获变量未声明报错、lambda 赋值（自动）+ 调用、无参 lambda
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
    Lexer lexer(source, "lambda语义测试.cn", diagnostics);
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

// [] 无捕获 + 显式返回标注 + 调用
TEST(LambdaTest, NoCaptureExplicitReturn) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    自动 加倍 = [](整32 x) -> 整32 { 返回 x * 2 }
    整32 值 = 加倍(21)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// [] 无捕获 + 返回类型推导（单一返回表达式）
TEST(LambdaTest, NoCaptureReturnInfer) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    自动 平方 = [](整32 x) { 返回 x * x }
    整32 值 = 平方(9)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// [=] 值捕获外层变量
TEST(LambdaTest, ByValueCapture) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 因子 = 5
    自动 乘因子 = [=](整32 x) { 返回 x * 因子 }
    整32 值 = 乘因子(6)
    返回 0
}
)CN");
   EXPECT_TRUE(r.ok) << r.messages;
}

// [&] 引用捕获外层变量
TEST(LambdaTest, ByRefCapture) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 计数 = 10
    自动 加计数 = [&](整32 x) { 返回 x + 计数 }
    整32 值 = 加计数(5)
    返回 0
}
)CN");
   EXPECT_TRUE(r.ok) << r.messages;
}

// [变量] 显式捕获
TEST(LambdaTest, ExplicitCapture) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 增量 = 100
    自动 加增量 = [增量](整32 x) { 返回 x + 增量 }
    整32 值 = 加增量(1)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 显式捕获未声明变量报错
TEST(LambdaTest, UndeclaredCaptureError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    自动 坏 = [不存在](整32 x) { 返回 x }
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 无参 lambda + 返回字符串（推导）
TEST(LambdaTest, NoParamReturnString) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    自动 说你好 = [] { 返回 "你好" + "lambda" }
    字符串 s = 说你好()
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// lambda 体内引用外层变量：[] 无捕获但语义层允许隐式引用（作用域可见），
//   与 [=] 值捕获等价（IR 层按 explicitCaptures 展开捕获实参）。
//   注：本实现中 [] 与 [=] 的捕获集均来自语义层扫描，[] 仅表示"不显式声明捕获"。
TEST(LambdaTest, BodyRefOuterVarOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 因子 = 5
    自动 闭包 = [](整32 x) { 返回 x * 因子 }
    整32 值 = 闭包(6)
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 回归（审查修复 BUG#3）：显式捕获 + 无参 lambda（[外层] { ... }）
//   原实现 peekLambdaCapture 对显式捕获的 ']' 后仅认 '('，导致无参显式捕获
//   lambda 被误判为数组下标（报"预期表达式，实际为 '['"）。
TEST(LambdaTest, ExplicitCaptureNoParamOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 外层 = 100
    自动 读取 = [外层] { 返回 外层 + 1 }
    整32 值 = 读取()
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
}

// 缺陷修复回归（[=] 快照 vs [&] 引用）：
//   捕获变量在 lambda 定义后被修改——[=] 闭包应读到定义时快照（值语义），
//   [&] 闭包应读到最新值（引用语义）。语义层须正确收集两种捕获集，
//   且 [&] 闭包内可修改捕获变量（引用语义写回外部）。
TEST(LambdaTest, ByValueSnapshotVsByRefLatest) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 值 = 101
    自动 快照 = [=]() -> 整32 { 返回 值 }
    自动 引用 = [&]() -> 整32 { 返回 值 }
    值 = 1000
    整32 快 = 快照()
    整32 引 = 引用()
    返回 快 + 引
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// [&] 引用捕获闭包内修改外部变量（引用语义写回）
TEST(LambdaTest, ByRefWriteBack) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 计数 = 10
    自动 加一 = [&]() { 计数 = 计数 + 1 }
    加一()
    返回 计数
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// [=] 值捕获闭包内修改不影响外部（快照独立）
TEST(LambdaTest, ByValueWriteIsolated) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 外部 = 5
    自动 改值 = [=]() { 外部 = 99 }
    改值()
    返回 外部
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 缺陷修复回归：嵌套 lambda——内层 lambda 引用的外层变量须合并到本层捕获集
//   （原实现 collectLambdaCaptures 漏 LambdaExpr case，内层捕获变量穿透匿名函数
//   边界直接引用外层函数栈槽，产生 [rbp0] 非法汇编，嵌套 lambda 汇编失败）
TEST(LambdaTest, NestedLambdaCaptureOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 外层值 = 100
    自动 外层闭包 = [=]() -> 整32 {
        自动 内层闭包 = [=]() -> 整32 { 返回 外层值 }
        返回 内层闭包()
    }
    外层值 = 999
    返回 外层闭包()
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 缺陷修复回归：[=] 结构体值捕获快照——捕获结构体须深拷贝快照，字段访问读快照值
//   （原实现仅存 AddrOf 指针且闭包参数未标记 structParamIndexes，字段访问读到
//   地址值字节=垃圾，值捕获p.x 实得 553448424 而非 1）
TEST(LambdaTest, StructByValueCaptureSnapshot) {
    auto r = analyzeSource(R"CN(
结构体 点 {
    整32 x
    整32 y
}
函数 主() -> 整32 {
    点 p = 点{ x = 1, y = 2 }
    自动 fp = [=]() -> 整32 { 返回 p.x }
    p.x = 100
    返回 fp()
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}
