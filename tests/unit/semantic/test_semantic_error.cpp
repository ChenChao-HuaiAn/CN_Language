// 阶段3 语义分析器 错误处理 单元测试（Task 3.5，规格书07）
// 覆盖：结果<T,E>/可选<T> 类型降级（合成结构体）、内置构造器（正常/错误/某些）、
//       3条强制检查规则正反用例：
//         规则1：结果<T,E> 返回值被丢弃未检查 -> 错误
//         规则2：检查 .正常 后未处理错误分支 -> 警告
//         规则3：可选<T> 访问 .值 前必须检查 .有值 -> 错误
//       无 在可选上下文为空可选值
// 测试方式：通过 Lexer + Parser 得到真实AST，再交给 SemanticAnalyzer 分析（全链路）
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
using cn_compiler::Program;
using cn_compiler::SemanticAnalyzer;

namespace {

struct SemanticResult {
    bool ok = false;         // 语义分析是否成功
    int errorCount = 0;      // 错误数量
    int warningCount = 0;    // 警告数量
    std::string messages;    // 全部诊断消息（拼接）
};

SemanticResult analyzeSource(const std::string& source) {
    SemanticResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "语义错误处理测试.cn", diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(diagnostics);
    auto program = parser.parse(tokens);
    SemanticAnalyzer analyzer(diagnostics);
    result.ok = analyzer.analyze(program.get());
    result.errorCount = diagnostics.getErrorCount();
    result.warningCount = diagnostics.getWarningCount();
    for (const auto& d : diagnostics.getAll()) {
        result.messages += d.message + "\n";
    }
    return result;
}

} // namespace

// ==================== 结果/可选 类型降级 + 内置构造器（Task 3.5） ====================

// 结果<T,E> 完整使用：正常()/错误() 构造 + .正常 检查 + .值/.错误 访问 -> 合法
TEST(SemanticErrorTest, ResultTypeFullFlow) {
    auto r = analyzeSource(R"CN(
函数 除法(整32 a, 整32 b) -> 结果<整32, 整32> {
    如果 b == 0 {
        返回 错误(1)
    }
    返回 正常(a / b)
}
函数 主() -> 整32 {
    变量 除法结果 = 除法(10, 2)
    如果 除法结果.正常 {
        打印行(除法结果.值)
    } 否则 {
        打印行(除法结果.错误)
    }
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
    EXPECT_EQ(r.warningCount, 0);
}

// 可选<T> 完整使用：某些()/无 构造 + .有值 检查 + .值 访问 -> 合法
TEST(SemanticErrorTest, OptionalTypeFullFlow) {
    auto r = analyzeSource(R"CN(
函数 查找(整32 键) -> 可选<整32> {
    如果 键 > 0 {
        返回 某些(键 * 2)
    }
    返回 无
}
函数 主() -> 整32 {
    变量 查找结果 = 查找(42)
    如果 查找结果.有值 {
        打印行(查找结果.值)
    } 否则 {
        打印行("未找到")
    }
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 内置构造器重定义：用户定义 正常 函数 -> 报错
TEST(SemanticErrorTest, BuiltinConstructorRedefine) {
    auto r = analyzeSource(R"CN(
函数 正常(整32 x) -> 整32 {
    返回 x
}
函数 主() -> 整32 {
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(r.messages.find("不可重定义"), std::string::npos);
}

// ==================== 规则1：结果返回值被丢弃未检查 -> 错误 ====================

TEST(SemanticErrorTest, Rule1ResultDiscarded) {
    auto r = analyzeSource(R"CN(
函数 除法(整32 a, 整32 b) -> 结果<整32, 整32> {
    返回 正常(a / b)
}
函数 主() -> 整32 {
    除法(10, 2)
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(r.messages.find("被丢弃未检查"), std::string::npos);
}

// 规则1合规：结果赋值给变量（随后可检查）不触发
TEST(SemanticErrorTest, Rule1ResultAssigned) {
    auto r = analyzeSource(R"CN(
函数 除法(整32 a, 整32 b) -> 结果<整32, 整32> {
    返回 正常(a / b)
}
函数 主() -> 整32 {
    变量 除法结果 = 除法(10, 2)
    如果 除法结果.正常 {
        打印行(除法结果.值)
    } 否则 {
        打印行(除法结果.错误)
    }
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// ==================== 规则2：检查 .正常 后未处理错误分支 -> 警告 ====================

TEST(SemanticErrorTest, Rule2MissingElseWarning) {
    auto r = analyzeSource(R"CN(
函数 除法(整32 a, 整32 b) -> 结果<整32, 整32> {
    返回 正常(a / b)
}
函数 主() -> 整32 {
    变量 除法结果 = 除法(10, 2)
    如果 除法结果.正常 {
        打印行(除法结果.值)
    }
    返回 0
}
)CN");
    // 规则2 为警告：不阻断编译（ok 仍为 true，无错误）
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
    EXPECT_GT(r.warningCount, 0);
    EXPECT_NE(r.messages.find("未处理错误分支"), std::string::npos);
}

// ==================== 规则3：可选 访问 .值 前必须检查 .有值 -> 错误 ====================

TEST(SemanticErrorTest, Rule3OptionalValueBeforeCheck) {
    auto r = analyzeSource(R"CN(
函数 查找(整32 键) -> 可选<整32> {
    返回 某些(键)
}
函数 主() -> 整32 {
    变量 查找结果 = 查找(42)
    打印行(查找结果.值)
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(r.messages.find("必须检查 可选.有值"), std::string::npos);
}

// 规则3合规：检查 .有值 后访问 .值（if 真分支内）-> 合法
TEST(SemanticErrorTest, Rule3OptionalValueAfterCheck) {
    auto r = analyzeSource(R"CN(
函数 查找(整32 键) -> 可选<整32> {
    返回 某些(键)
}
函数 主() -> 整32 {
    变量 查找结果 = 查找(42)
    如果 查找结果.有值 {
        打印行(查找结果.值)
    }
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 结果.值 未检查直接访问 -> 错误（规则3 的结果变体）
TEST(SemanticErrorTest, Rule3ResultValueBeforeCheck) {
    auto r = analyzeSource(R"CN(
函数 除法(整32 a, 整32 b) -> 结果<整32, 整32> {
    返回 正常(a / b)
}
函数 主() -> 整32 {
    变量 除法结果 = 除法(10, 2)
    打印行(除法结果.值)
    返回 0
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(r.messages.find("必须检查 结果.正常"), std::string::npos);
}

// 结果.错误 在否则分支内访问 -> 合法；在外部访问 -> 错误
TEST(SemanticErrorTest, Rule3ResultErrorAccess) {
    // 否则分支内访问 .错误：合法
    auto r = analyzeSource(R"CN(
函数 除法(整32 a, 整32 b) -> 结果<整32, 整32> {
    返回 正常(a / b)
}
函数 主() -> 整32 {
    变量 除法结果 = 除法(10, 2)
    如果 除法结果.正常 {
        打印行(除法结果.值)
    } 否则 {
        打印行(除法结果.错误)
    }
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// ==================== 无 在可选上下文（空可选值） ====================

// 可选<T> 变量赋 无：合法（空可选值）
TEST(SemanticErrorTest, OptionalNullAssignment) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    可选<整32> 可选结果 = 无
    返回 0
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}
