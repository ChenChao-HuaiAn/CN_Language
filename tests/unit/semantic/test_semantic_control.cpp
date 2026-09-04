// 语义分析器控制流增量单元测试（Task 2.1：选择/情况/默认）
// 覆盖：case 重复检测、case 非整型常量、中断在循环外报错、中断跳出switch合法
// 测试方式：Lexer + Parser + SemanticAnalyzer 全链路（非Mock）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
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

// 辅助：解析+语义分析源码，返回是否成功与诊断引擎
struct SemanticResult {
    bool ok = false;         // 语义分析是否成功
    int errorCount = 0;      // 错误数量
    std::string messages;    // 全部诊断消息（拼接）
};

SemanticResult analyzeSource(const std::string& source) {
    SemanticResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "控制流语义测试.cn", diagnostics);
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

// ==================== 选择语句合法场景 ====================

// 基本 switch 语义检查通过（整型条件 + 显式中断）
TEST(SemanticSwitchTest, BasicSwitchOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整数 v = 2;
    选择(v) {
        情况 1:
            打印行("一");
            中断;
        情况 2:
            打印行("二");
            中断;
        默认:
            打印行("其他");
    }
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 中断跳出 switch 合法（switch 内不用循环）
TEST(SemanticSwitchTest, BreakInsideSwitchOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整数 v = 1;
    选择(v) {
        情况 1:
            打印行("一");
            中断;
        默认:
            中断;
    }
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 字符条件的 switch 合法
TEST(SemanticSwitchTest, CharConditionOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符 c = 'A';
    选择(c) {
        情况 'A':
            打印行("A");
            中断;
        默认:
            打印行("其他");
    }
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 中断在循环内跳出循环（原有语义不破坏）
TEST(SemanticSwitchTest, BreakInLoopStillOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整数 i = 0;
    当 (i < 5) {
        如果 (i == 3) {
            中断;
        }
        i++;
    }
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// ==================== 选择语句错误场景 ====================

// 情况值重复：报错
TEST(SemanticSwitchTest, DuplicateCaseValue) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整数 v = 1;
    选择(v) {
        情况 1:
            中断;
        情况 1:
            中断;
        默认:
            中断;
    }
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    // 错误消息应提及重复
    EXPECT_NE(r.messages.find("重复"), std::string::npos);
}

// 选择表达式为字符串：报错（只允许整数/字符）
TEST(SemanticSwitchTest, StringConditionError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 s = "abc";
    选择(s) {
        情况 1:
            中断;
        默认:
            中断;
    }
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 选择表达式为布尔：报错
TEST(SemanticSwitchTest, BoolConditionError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    布尔 b = 真;
    选择(b) {
        情况 1:
            中断;
        默认:
            中断;
    }
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 中断在循环外且 switch 外：报错（原有错误语义保留）
TEST(SemanticSwitchTest, BreakOutsideLoopAndSwitchError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整数 x = 1;
    中断;
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    // 错误消息应提及中断
    EXPECT_NE(r.messages.find("中断"), std::string::npos);
}

// 中断在 switch 内的循环中：跳出循环（最内层中断目标）
TEST(SemanticSwitchTest, BreakInLoopInsideSwitchOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整数 v = 2;
    选择(v) {
        情况 2:
            整数 i = 0;
            当 (i < 3) {
                中断;      // 跳出当循环（不是跳出switch）
                i++;
            }
            中断;          // 跳出switch
        默认:
            中断;
    }
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 继续在 switch 内（无循环）：报错（继续只能跳出循环）
TEST(SemanticSwitchTest, ContinueInsideSwitchError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整数 v = 1;
    选择(v) {
        情况 1:
            继续;
            中断;
        默认:
            中断;
    }
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// switch 嵌套 switch：内层中断只跳出内层，均合法
TEST(SemanticSwitchTest, NestedSwitchBothOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整数 v = 2;
    选择(v) {
        情况 2:
            整数 n = 1;
            选择(n) {
                情况 1:
                    中断;      // 跳出内层switch
                默认:
                    中断;
            }
            中断;            // 跳出外层switch
        默认:
            中断;
    }
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}
