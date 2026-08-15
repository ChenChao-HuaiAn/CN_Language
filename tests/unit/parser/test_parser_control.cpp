// 语法分析器控制流增量单元测试（Task 2.1：选择/情况/默认）
// 覆盖：switch 解析、嵌套、case 缺中断 fallthrough、默认缺省、错误恢复
// 测试方式：通过 Lexer + Parser 得到真实AST（全链路，非Mock）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"

using cn_compiler::CaseLabel;
using cn_compiler::DefaultLabel;
using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::NodeType;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::Stmt;
using cn_compiler::SwitchStmt;

namespace {

// 辅助：解析源码，返回程序AST与诊断引擎
struct ParseResult {
    std::unique_ptr<Program> program;
    Diagnostics diagnostics;
};

ParseResult parseProgram(const std::string& source) {
    ParseResult result;
    Lexer lexer(source, "控制流测试.cn", result.diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(result.diagnostics);
    result.program = parser.parse(tokens);
    return result;
}

// 取第一个函数的第n条语句（部分用例未使用，GCC -Wunused-function 兼容）
[[maybe_unused]] Stmt* firstStmt(Program* program, std::size_t index = 0) {
    if (program == nullptr || program->declarations.empty()) return nullptr;
    auto& func = program->declarations[0];
    if (func->body == nullptr) return nullptr;
    if (index >= func->body->statements.size()) return nullptr;
    return func->body->statements[index].get();
}

// 便捷：遍历函数体查找第 index 个 SwitchStmt（前面可能有变量声明等语句）
SwitchStmt* firstSwitch(Program* program, std::size_t index = 0) {
    if (program == nullptr || program->declarations.empty()) return nullptr;
    auto& func = program->declarations[0];
    if (func->body == nullptr) return nullptr;
    std::size_t found = 0;
    for (auto& stmt : func->body->statements) {
        if (stmt->getType() == NodeType::SwitchStmt) {
            if (found == index) return static_cast<SwitchStmt*>(stmt.get());
            found++;
        }
    }
    return nullptr;
}

} // namespace

// ==================== 选择语句基本解析 ====================

// 基本 switch：情况+中断+默认 完整结构
TEST(ParserSwitchTest, BasicSwitch) {
    auto result = parseProgram(R"CN(
函数 主() -> 整32 {
    整数 v = 2
    选择(v) {
        情况 1:
            打印行("一")
            中断
        情况 2:
            打印行("二")
            中断
        默认:
            打印行("其他")
    }
    返回 0
}
)CN");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    SwitchStmt* sw = firstSwitch(result.program.get());
    ASSERT_NE(sw, nullptr);
    EXPECT_EQ(sw->cases.size(), 2u);
    ASSERT_NE(sw->defaultCase, nullptr);
    // 第一个情况：值1，语句数2（打印行 + 中断）
    EXPECT_EQ(sw->cases[0]->value, 1);
    EXPECT_EQ(sw->cases[0]->statements.size(), 2u);
    // 第二个情况：值2
    EXPECT_EQ(sw->cases[1]->value, 2);
    EXPECT_EQ(sw->cases[1]->statements.size(), 2u);
    // 默认分支：语句数1
    EXPECT_EQ(sw->defaultCase->statements.size(), 1u);
}

// 默认缺省：无默认分支
TEST(ParserSwitchTest, SwitchWithoutDefault) {
    auto result = parseProgram(R"CN(
函数 主() -> 整32 {
    整数 v = 1
    选择(v) {
        情况 1:
            打印行("一")
            中断
        情况 2:
            打印行("二")
            中断
    }
    返回 0
}
)CN");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    SwitchStmt* sw = firstSwitch(result.program.get());
    ASSERT_NE(sw, nullptr);
    EXPECT_EQ(sw->cases.size(), 2u);
    EXPECT_EQ(sw->defaultCase, nullptr);
}

// 情况值支持十六进制/二进制/八进制
TEST(ParserSwitchTest, CaseValueBases) {
    auto result = parseProgram(R"CN(
函数 主() -> 整32 {
    整数 v = 0x10
    选择(v) {
        情况 0x10:
            中断
        情况 0b1010:
            中断
        情况 0o17:
            中断
        默认:
            中断
    }
    返回 0
}
)CN");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    SwitchStmt* sw = firstSwitch(result.program.get());
    ASSERT_NE(sw, nullptr);
    EXPECT_EQ(sw->cases.size(), 3u);
    EXPECT_EQ(sw->cases[0]->value, 16);   // 0x10
    EXPECT_EQ(sw->cases[1]->value, 10);   // 0b1010
    EXPECT_EQ(sw->cases[2]->value, 15);   // 0o17
}

// 情况值支持字符字面量
TEST(ParserSwitchTest, CaseValueChar) {
    auto result = parseProgram(R"CN(
函数 主() -> 整32 {
    字符 c = 'A'
    选择(c) {
        情况 'A':
            打印行("A")
            中断
        默认:
            打印行("其他")
    }
    返回 0
}
)CN");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    SwitchStmt* sw = firstSwitch(result.program.get());
    ASSERT_NE(sw, nullptr);
    EXPECT_EQ(sw->cases.size(), 1u);
    EXPECT_EQ(sw->cases[0]->value, static_cast<std::int64_t>('A'));
}

// ==================== fallthrough 与嵌套 ====================

// case 缺中断：fallthrough（语句贯穿到下一情况）
TEST(ParserSwitchTest, FallthroughWithoutBreak) {
    auto result = parseProgram(R"CN(
函数 主() -> 整32 {
    整数 w = 1
    选择(w) {
        情况 1:
            打印行("fall1")
        情况 2:
            打印行("fall2")
            中断
        默认:
            打印行("fall默认")
    }
    返回 0
}
)CN");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    SwitchStmt* sw = firstSwitch(result.program.get());
    ASSERT_NE(sw, nullptr);
    // 情况1只有1条语句（打印行，无中断）——fallthrough 由 IR 层处理
    EXPECT_EQ(sw->cases[0]->statements.size(), 1u);
    EXPECT_EQ(sw->cases[1]->statements.size(), 2u);
}

// switch 内嵌套 if / 当循环 / 内层 switch
TEST(ParserSwitchTest, NestedSwitchAndControl) {
    auto result = parseProgram(R"CN(
函数 主() -> 整32 {
    整数 v = 2
    选择(v) {
        情况 2:
            整数 n = 0
            当 (n < 2) {
                打印行("嵌套")
                n++
            }
            如果 (v == 2) {
                打印行("内层如果")
            }
            选择(n) {
                情况 1:
                    打印行("内层选择")
                    中断
                默认:
                    中断
            }
            中断
        默认:
            打印行("默认")
    }
    返回 0
}
)CN");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    SwitchStmt* sw = firstSwitch(result.program.get());
    ASSERT_NE(sw, nullptr);
    // 情况2 包含：变量声明 + 当循环 + 如果 + 内层选择 + 中断 = 5条
    EXPECT_EQ(sw->cases[0]->statements.size(), 5u);
}

// ==================== 错误恢复 ====================

// 缺少右括号：报告错误但尽力恢复
TEST(ParserSwitchTest, MissingRightParen) {
    auto result = parseProgram(R"CN(
函数 主() -> 整32 {
    整数 v = 1
    选择(v {
        情况 1:
            中断
    }
    返回 0
}
)CN");
    EXPECT_TRUE(result.diagnostics.hasErrors());
    // 仍能解析出 switch 节点（错误恢复）
    SwitchStmt* sw = firstSwitch(result.program.get());
    ASSERT_NE(sw, nullptr);
}

// 缺少左花括号：报告错误
TEST(ParserSwitchTest, MissingLeftBrace) {
    auto result = parseProgram(R"CN(
函数 主() -> 整32 {
    整数 v = 1
    选择(v)
        情况 1:
            中断
    返回 0
}
)CN");
    EXPECT_TRUE(result.diagnostics.hasErrors());
}

// C-4（2026-08）：情况 裸标识符（变量名/枚举成员候选）——语法层接受
//   （枚举成员候选，如 情况 红:），语义层按 选择 条件枚举类型解析；
//   非枚举上下文的变量名由语义层报错（test_pattern_match 覆盖）
TEST(ParserSwitchTest, CaseValueNotConstant) {
    auto result = parseProgram(R"CN(
函数 主() -> 整32 {
    整数 v = 1
    选择(v) {
        情况 v:
            中断
    }
    返回 0
}
)CN");
    EXPECT_FALSE(result.diagnostics.hasErrors());
}

// 空 switch 体：合法（无分支）
TEST(ParserSwitchTest, EmptySwitchBody) {
    auto result = parseProgram(R"CN(
函数 主() -> 整32 {
    整数 v = 1
    选择(v) {
    }
    返回 0
}
)CN");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    SwitchStmt* sw = firstSwitch(result.program.get());
    ASSERT_NE(sw, nullptr);
    EXPECT_TRUE(sw->cases.empty());
    EXPECT_EQ(sw->defaultCase, nullptr);
}
