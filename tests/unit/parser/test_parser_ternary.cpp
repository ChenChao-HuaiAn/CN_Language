// 三元表达式语法解析单元测试（Task 2.9）
// 覆盖：基本三元（条件 ? 真 : 假）、数值/字符串分支、嵌套三元、右结合、优先级
//       （三元 1.5 级高于赋值、低于逻辑或）、真值完整表达式（含逗号分隔？CN 无逗号表达式）
// 测试方式：通过 Lexer + Parser 得到真实AST（全链路，非Mock）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"

using cn_compiler::BinaryExpr;
using cn_compiler::Diagnostics;
using cn_compiler::Expr;
using cn_compiler::ExprStmt;
using cn_compiler::FunctionDecl;
using cn_compiler::Lexer;
using cn_compiler::NodeType;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::Stmt;
using cn_compiler::TernaryExpr;
using cn_compiler::VarDecl;

namespace {

// 辅助：解析源码，返回程序AST与诊断引擎
struct ParseResult {
    std::unique_ptr<Program> program;
    Diagnostics diagnostics;
};

ParseResult parseProgram(const std::string& source) {
    ParseResult result;
    Lexer lexer(source, "三元解析测试.cn", result.diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(result.diagnostics);
    result.program = parser.parse(tokens);
    return result;
}

// 辅助：取函数体第一条语句（表达式语句）
Expr* firstExpr(const Program* program) {
    const FunctionDecl* fn = program->declarations[0].get();
    Stmt* stmt = fn->body->statements[0].get();
    return static_cast<ExprStmt*>(stmt)->expr.get();
}

// 辅助：取变量声明初始值表达式
Expr* varInit(const Program* program, std::size_t index = 0) {
    const FunctionDecl* fn = program->declarations[0].get();
    Stmt* stmt = fn->body->statements[index].get();
    return static_cast<VarDecl*>(stmt)->initializer.get();
}

} // namespace

// 基本三元：x > 0 ? "正" : "负"
TEST(ParserTernaryTest, BasicTernary) {
    ParseResult r = parseProgram(
        "函数 主() -> 整32 {\n"
        "    变量 t = x > 0 ? \"正\" : \"负\"\n"
        "    返回 0\n"
        "}\n");
    ASSERT_EQ(r.program->declarations.size(), 1u);
    Expr* init = varInit(r.program.get());
    ASSERT_EQ(init->getType(), NodeType::TernaryExpr);
    TernaryExpr* tern = static_cast<TernaryExpr*>(init);
    // 条件：x > 0（二元比较）
    ASSERT_EQ(tern->condition->getType(), NodeType::BinaryExpr);
    EXPECT_EQ(static_cast<BinaryExpr*>(tern->condition.get())->op,
              cn_compiler::Operator::Greater);
    // 真值/假值：字符串字面量
    EXPECT_EQ(tern->trueValue->getType(), NodeType::StringLiteral);
    EXPECT_EQ(tern->falseValue->getType(), NodeType::StringLiteral);
}

// 数值分支 + 赋值上下文（优先级：三元高于赋值）
TEST(ParserTernaryTest, NumericBranchAndAssignPriority) {
    ParseResult r = parseProgram(
        "函数 主() -> 整32 {\n"
        "    变量 n = x > 5 ? 100 : 200\n"
        "    返回 0\n"
        "}\n");
    Expr* init = varInit(r.program.get());
    ASSERT_EQ(init->getType(), NodeType::TernaryExpr);
    TernaryExpr* tern = static_cast<TernaryExpr*>(init);
    EXPECT_EQ(tern->trueValue->getType(), NodeType::IntegerLiteral);
    EXPECT_EQ(tern->falseValue->getType(), NodeType::IntegerLiteral);
}

// 嵌套三元（括号内层）
TEST(ParserTernaryTest, NestedTernary) {
    ParseResult r = parseProgram(
        "函数 主() -> 整32 {\n"
        "    变量 t = x > 0 ? (x > 5 ? \"大\" : \"小\") : \"非正\"\n"
        "    返回 0\n"
        "}\n");
    Expr* init = varInit(r.program.get());
    ASSERT_EQ(init->getType(), NodeType::TernaryExpr);
    TernaryExpr* tern = static_cast<TernaryExpr*>(init);
    // 真值：内层三元
    EXPECT_EQ(tern->trueValue->getType(), NodeType::TernaryExpr);
}

// 右结合：a ? b : c ? d : e = a ? b : (c ? d : e)
TEST(ParserTernaryTest, RightAssociative) {
    ParseResult r = parseProgram(
        "函数 主() -> 整32 {\n"
        "    变量 t = 假 ? \"一\" : 假 ? \"二\" : \"三\"\n"
        "    返回 0\n"
        "}\n");
    Expr* init = varInit(r.program.get());
    ASSERT_EQ(init->getType(), NodeType::TernaryExpr);
    TernaryExpr* tern = static_cast<TernaryExpr*>(init);
    // 外层假值 = 内层三元（右结合）；真值 = "一"
    EXPECT_EQ(tern->trueValue->getType(), NodeType::StringLiteral);
    ASSERT_EQ(tern->falseValue->getType(), NodeType::TernaryExpr);
}

// 优先级：三元低于逻辑或（x > 0 || y ? ... ）条件整体为 || 表达式
TEST(ParserTernaryTest, PriorityBelowLogicalOr) {
    ParseResult r = parseProgram(
        "函数 主() -> 整32 {\n"
        "    变量 t = x > 0 || y ? \"a\" : \"b\"\n"
        "    返回 0\n"
        "}\n");
    Expr* init = varInit(r.program.get());
    ASSERT_EQ(init->getType(), NodeType::TernaryExpr);
    TernaryExpr* tern = static_cast<TernaryExpr*>(init);
    // 条件 = (x > 0) || y（逻辑或，优先级2 高于三元 1.5）
    ASSERT_EQ(tern->condition->getType(), NodeType::BinaryExpr);
    EXPECT_EQ(static_cast<BinaryExpr*>(tern->condition.get())->op,
              cn_compiler::Operator::OrOr);
}

// 三元作为函数实参（惰性求值测试的基础）
TEST(ParserTernaryTest, TernaryAsArg) {
    ParseResult r = parseProgram(
        "函数 主() -> 整32 {\n"
        "    打印(x > 0 ? \"正\" : \"负\")\n"
        "    返回 0\n"
        "}\n");
    const FunctionDecl* fn = r.program->declarations[0].get();
    Stmt* stmt = fn->body->statements[0].get();
    Expr* call = static_cast<ExprStmt*>(stmt)->expr.get();
    ASSERT_EQ(call->getType(), NodeType::CallExpr);
    cn_compiler::CallExpr* callExpr = static_cast<cn_compiler::CallExpr*>(call);
    ASSERT_EQ(callExpr->arguments.size(), 1u);
    EXPECT_EQ(callExpr->arguments[0]->getType(), NodeType::TernaryExpr);
}
