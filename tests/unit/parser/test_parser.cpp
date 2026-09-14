// 语法分析器单元测试（Task 1.4）
// 覆盖：函数声明、变量声明（变量/常量/类型前置/冒号后置）、表达式优先级与结合性、
//       控制流（如果/否则如果/否则、当、循环、无限循环、返回、中断、继续）、
//       代码块、错误处理（缺少分号/括号不匹配/无效语法）、混合代码
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"

using cn_compiler::AstNode;
using cn_compiler::AssignmentExpr;
using cn_compiler::BinaryExpr;
using cn_compiler::BlockStmt;
using cn_compiler::BoolLiteral;
using cn_compiler::BreakStmt;
using cn_compiler::CallExpr;
using cn_compiler::ContinueStmt;
using cn_compiler::Diagnostics;
using cn_compiler::Expr;
using cn_compiler::ExprStmt;
using cn_compiler::FloatLiteral;
using cn_compiler::ForStmt;
using cn_compiler::FunctionDecl;
using cn_compiler::IdentifierExpr;
using cn_compiler::IfStmt;
using cn_compiler::IntegerLiteral;
using cn_compiler::Lexer;
using cn_compiler::MemberExpr;
using cn_compiler::NodeType;
using cn_compiler::Operator;
using cn_compiler::ParamDecl;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::ReturnStmt;
using cn_compiler::Stmt;
using cn_compiler::UnaryExpr;
using cn_compiler::VarDecl;
using cn_compiler::WhileStmt;

namespace {

// 辅助函数：解析源码，返回程序AST与诊断引擎
struct ParseResult {
    std::unique_ptr<Program> program;
    Diagnostics diagnostics;
};

ParseResult parseProgram(const std::string& source) {
    ParseResult result;
    Lexer lexer(source, "测试.cn", result.diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(result.diagnostics);
    result.program = parser.parse(tokens);
    return result;
}

// 便捷访问：取第一个函数声明
FunctionDecl* firstFunction(Program* program) {
    if (program == nullptr || program->declarations.empty()) return nullptr;
    return program->declarations[0].get();
}

// 便捷访问：取第一个函数的第n条语句
Stmt* firstStmt(Program* program, std::size_t index = 0) {
    FunctionDecl* func = firstFunction(program);
    if (func == nullptr || func->body == nullptr) return nullptr;
    if (index >= func->body->statements.size()) return nullptr;
    return func->body->statements[index].get();
}

// 便捷访问：取第一个函数的第一个表达式（用于表达式测试）
Expr* firstExpr(Program* program) {
    Stmt* stmt = firstStmt(program);
    if (stmt == nullptr || stmt->getType() != NodeType::ExprStmt) return nullptr;
    return static_cast<ExprStmt*>(stmt)->expr.get();
}

// 便捷访问：取返回语句的值表达式
Expr* returnExpr(Program* program) {
    Stmt* stmt = firstStmt(program);
    if (stmt == nullptr || stmt->getType() != NodeType::ReturnStmt) return nullptr;
    return static_cast<ReturnStmt*>(stmt)->value.get();
}

} // namespace

// ==================== 1. 函数声明解析 ====================

// 无参无返回类型函数
TEST(ParserTest, FunctionNoParamNoReturn) {
    auto result = parseProgram("函数 主() { 打印行(\"你好\"); }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    FunctionDecl* func = firstFunction(result.program.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->name, "主");
    EXPECT_TRUE(func->params.empty());
    EXPECT_TRUE(func->returnType.empty());   // 无返回类型
    ASSERT_NE(func->body, nullptr);
    EXPECT_EQ(func->body->statements.size(), 1u);
}

// 有参有返回类型函数
TEST(ParserTest, FunctionWithParamsAndReturn) {
    auto result = parseProgram("函数 加(整32 a, 整32 b) -> 整32 { 返回 a + b; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    FunctionDecl* func = firstFunction(result.program.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->name, "加");
    ASSERT_EQ(func->params.size(), 2u);
    EXPECT_EQ(func->params[0]->typeName, "整32");
    EXPECT_EQ(func->params[0]->name, "a");
    EXPECT_EQ(func->params[1]->typeName, "整32");
    EXPECT_EQ(func->params[1]->name, "b");
    EXPECT_EQ(func->returnType, "整32");
    ASSERT_NE(func->body, nullptr);
    EXPECT_EQ(func->body->statements.size(), 1u);
}

// 函数原型声明（无函数体）
TEST(ParserTest, FunctionPrototype) {
    auto result = parseProgram("函数 计算(整32 n) -> 整32");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    FunctionDecl* func = firstFunction(result.program.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->name, "计算");
    EXPECT_EQ(func->params.size(), 1u);
    EXPECT_EQ(func->returnType, "整32");
    EXPECT_EQ(func->body, nullptr);  // 原型无函数体
}

// 显式空类型返回
TEST(ParserTest, FunctionVoidReturn) {
    auto result = parseProgram("函数 打印() -> 空类型 { }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    FunctionDecl* func = firstFunction(result.program.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->returnType, "空类型");
    ASSERT_NE(func->body, nullptr);
    EXPECT_TRUE(func->body->statements.empty());
}

// 冒号后置参数（兼容写法：a: 整32）
TEST(ParserTest, FunctionColonParams) {
    // A8 收口（2026-09-14）：冒号后置参数标注为规范外语法（spec 03 否决），必须报错
    // （原「兼容解析」断言随语法移除而迁移为负断言；E2E 268 同步负测锚定）
    auto result = parseProgram("函数 加(a: 整32, b: 整32) -> 整32 { 返回 a + b; }");
    EXPECT_TRUE(result.diagnostics.hasErrors());
}

// 多函数顶层声明
TEST(ParserTest, MultipleFunctions) {
    auto result = parseProgram("函数 甲() { } 函数 乙() -> 整32 { 返回 1; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ASSERT_EQ(result.program->declarations.size(), 2u);
    EXPECT_EQ(result.program->declarations[0]->name, "甲");
    EXPECT_EQ(result.program->declarations[1]->name, "乙");
}

// ==================== 2. 变量声明解析 ====================

// 变量声明（类型推断）
TEST(ParserTest, VarDeclInferred) {
    auto result = parseProgram("函数 测试() { 变量 x = 10; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Stmt* stmt = firstStmt(result.program.get());
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->getType(), NodeType::VarDecl);
    VarDecl* decl = static_cast<VarDecl*>(stmt);
    EXPECT_EQ(decl->name, "x");
    EXPECT_TRUE(decl->typeName.empty());  // 类型推断
    EXPECT_FALSE(decl->isConst);
    ASSERT_NE(decl->initializer, nullptr);
    EXPECT_EQ(decl->initializer->getType(), NodeType::IntegerLiteral);
}

// 变量声明（显式类型前置）
TEST(ParserTest, VarDeclExplicitType) {
    auto result = parseProgram("函数 测试() { 整32 y = 20; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Stmt* stmt = firstStmt(result.program.get());
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->getType(), NodeType::VarDecl);
    VarDecl* decl = static_cast<VarDecl*>(stmt);
    EXPECT_EQ(decl->name, "y");
    EXPECT_EQ(decl->typeName, "整32");
    ASSERT_NE(decl->initializer, nullptr);
}

// 变量声明（无初始值）
TEST(ParserTest, VarDeclNoInit) {
    auto result = parseProgram("函数 测试() { 变量 x; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Stmt* stmt = firstStmt(result.program.get());
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->getType(), NodeType::VarDecl);
    VarDecl* decl = static_cast<VarDecl*>(stmt);
    EXPECT_EQ(decl->name, "x");
    EXPECT_EQ(decl->initializer, nullptr);
}

// 常量声明（常量关键字，词法层为标识符文本）
TEST(ParserTest, ConstDecl) {
    auto result = parseProgram("函数 测试() { 常量 PI = 3.14; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Stmt* stmt = firstStmt(result.program.get());
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->getType(), NodeType::VarDecl);
    VarDecl* decl = static_cast<VarDecl*>(stmt);
    EXPECT_TRUE(decl->isConst);
    EXPECT_EQ(decl->name, "PI");
    ASSERT_NE(decl->initializer, nullptr);
    EXPECT_EQ(decl->initializer->getType(), NodeType::FloatLiteral);
}

// 冒号后置类型声明（变量 x: 整32 = 10，兼容写法）
TEST(ParserTest, VarDeclColonType) {
    // A8 收口（2026-09-14）：变量冒号后置类型标注为规范外语法，必须报错
    // （原「兼容解析」断言随语法移除而迁移为负断言；E2E 269 同步负测锚定）
    auto result = parseProgram("函数 测试() { 变量 x: 整32 = 10; }");
    EXPECT_TRUE(result.diagnostics.hasErrors());
}

// 多变量声明
TEST(ParserTest, MultipleVarDecls) {
    auto result = parseProgram("函数 测试() { 变量 x = 1; 整32 y = 2; 字符串 s = \"a\"; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    EXPECT_EQ(firstFunction(result.program.get())->body->statements.size(), 3u);
    EXPECT_EQ(firstStmt(result.program.get(), 0)->getType(), NodeType::VarDecl);
    EXPECT_EQ(firstStmt(result.program.get(), 1)->getType(), NodeType::VarDecl);
    EXPECT_EQ(firstStmt(result.program.get(), 2)->getType(), NodeType::VarDecl);
}

// ==================== 3. 表达式解析 ====================

// 优先级：乘法高于加法 a + b * c
TEST(ParserTest, ExprMultiplicativePrecedence) {
    auto result = parseProgram("函数 测试() -> 整32 { 返回 a + b * c; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Expr* expr = returnExpr(result.program.get());
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getType(), NodeType::BinaryExpr);
    BinaryExpr* top = static_cast<BinaryExpr*>(expr);
    EXPECT_EQ(top->op, Operator::Add);
    ASSERT_EQ(top->right->getType(), NodeType::BinaryExpr);
    EXPECT_EQ(static_cast<BinaryExpr*>(top->right.get())->op, Operator::Multiply);
    EXPECT_EQ(top->left->getType(), NodeType::IdentifierExpr);
}

// 括号改变优先级 (a + b) * c
TEST(ParserTest, ExprParenPrecedence) {
    auto result = parseProgram("函数 测试() -> 整32 { 返回 (a + b) * c; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Expr* expr = returnExpr(result.program.get());
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getType(), NodeType::BinaryExpr);
    BinaryExpr* top = static_cast<BinaryExpr*>(expr);
    EXPECT_EQ(top->op, Operator::Multiply);
    ASSERT_EQ(top->left->getType(), NodeType::BinaryExpr);
    EXPECT_EQ(static_cast<BinaryExpr*>(top->left.get())->op, Operator::Add);
}

// 逻辑与优先级高于逻辑或 a || b && c
TEST(ParserTest, ExprLogicalPrecedence) {
    auto result = parseProgram("函数 测试() -> 布尔 { 返回 a || b && c; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Expr* expr = returnExpr(result.program.get());
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getType(), NodeType::BinaryExpr);
    BinaryExpr* top = static_cast<BinaryExpr*>(expr);
    EXPECT_EQ(top->op, Operator::OrOr);
    ASSERT_EQ(top->right->getType(), NodeType::BinaryExpr);
    EXPECT_EQ(static_cast<BinaryExpr*>(top->right.get())->op, Operator::AndAnd);
}

// 比较优先级高于逻辑与 a && b == c
TEST(ParserTest, ExprComparisonPrecedence) {
    auto result = parseProgram("函数 测试() -> 布尔 { 返回 a && b == c; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Expr* expr = returnExpr(result.program.get());
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getType(), NodeType::BinaryExpr);
    BinaryExpr* top = static_cast<BinaryExpr*>(expr);
    EXPECT_EQ(top->op, Operator::AndAnd);
    ASSERT_EQ(top->right->getType(), NodeType::BinaryExpr);
    EXPECT_EQ(static_cast<BinaryExpr*>(top->right.get())->op, Operator::EqualEqual);
}

// 左结合性：a - b - c => (a - b) - c
TEST(ParserTest, ExprLeftAssoc) {
    auto result = parseProgram("函数 测试() -> 整32 { 返回 a - b - c; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Expr* expr = returnExpr(result.program.get());
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getType(), NodeType::BinaryExpr);
    BinaryExpr* top = static_cast<BinaryExpr*>(expr);
    EXPECT_EQ(top->op, Operator::Subtract);
    ASSERT_EQ(top->left->getType(), NodeType::BinaryExpr);
    EXPECT_EQ(static_cast<BinaryExpr*>(top->left.get())->op, Operator::Subtract);
    EXPECT_EQ(top->right->getType(), NodeType::IdentifierExpr);
}

// 右结合性：a = b = c => a = (b = c)
TEST(ParserTest, ExprAssignRightAssoc) {
    auto result = parseProgram("函数 测试() { 变量 a = 1; 变量 b = 1; 变量 c = 1; a = b = c; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Expr* expr = firstExpr(result.program.get());
    // firstStmt是变量声明，取第3条语句（赋值表达式语句）
    Stmt* stmt = firstStmt(result.program.get(), 3);
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->getType(), NodeType::ExprStmt);
    expr = static_cast<ExprStmt*>(stmt)->expr.get();
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getType(), NodeType::AssignmentExpr);
    AssignmentExpr* assign = static_cast<AssignmentExpr*>(expr);
    EXPECT_EQ(assign->op, Operator::Assign);
    EXPECT_EQ(assign->target->getType(), NodeType::IdentifierExpr);
    ASSERT_EQ(assign->value->getType(), NodeType::AssignmentExpr);  // 右结合
}

// 一元前缀：-x !x ~x
TEST(ParserTest, ExprUnaryPrefix) {
    auto result = parseProgram("函数 测试() -> 整32 { 返回 -x + !b; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Expr* expr = returnExpr(result.program.get());
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getType(), NodeType::BinaryExpr);
    BinaryExpr* top = static_cast<BinaryExpr*>(expr);
    EXPECT_EQ(top->op, Operator::Add);
    ASSERT_EQ(top->left->getType(), NodeType::UnaryExpr);
    EXPECT_EQ(static_cast<UnaryExpr*>(top->left.get())->op, Operator::Subtract);
    ASSERT_EQ(top->right->getType(), NodeType::UnaryExpr);
    EXPECT_EQ(static_cast<UnaryExpr*>(top->right.get())->op, Operator::Bang);
}

// 前缀自增 ++i 与 --i
TEST(ParserTest, ExprPrefixIncDec) {
    auto result = parseProgram("函数 测试() { 变量 i = 0; ++i; --i; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    // 第二条语句：++i（前缀自增）
    Stmt* stmt = firstStmt(result.program.get(), 1);
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->getType(), NodeType::ExprStmt);
    Expr* expr = static_cast<ExprStmt*>(stmt)->expr.get();
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getType(), NodeType::UnaryExpr);
    UnaryExpr* unary = static_cast<UnaryExpr*>(expr);
    EXPECT_EQ(unary->op, Operator::Increment);
    EXPECT_FALSE(unary->postfix);  // 前缀
    // 第三条语句：--i（前缀自减）
    Stmt* stmt2 = firstStmt(result.program.get(), 2);
    ASSERT_NE(stmt2, nullptr);
    ASSERT_EQ(stmt2->getType(), NodeType::ExprStmt);
    Expr* expr2 = static_cast<ExprStmt*>(stmt2)->expr.get();
    ASSERT_NE(expr2, nullptr);
    ASSERT_EQ(expr2->getType(), NodeType::UnaryExpr);
    UnaryExpr* unary2 = static_cast<UnaryExpr*>(expr2);
    EXPECT_EQ(unary2->op, Operator::Decrement);
    EXPECT_FALSE(unary2->postfix);  // 前缀
}

// 后缀自增 i++
TEST(ParserTest, ExprPostfixIncDec) {
    auto result = parseProgram("函数 测试() { 变量 i = 0; i++; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Stmt* stmt = firstStmt(result.program.get(), 1);
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->getType(), NodeType::ExprStmt);
    Expr* expr = static_cast<ExprStmt*>(stmt)->expr.get();
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getType(), NodeType::UnaryExpr);
    UnaryExpr* unary = static_cast<UnaryExpr*>(expr);
    EXPECT_EQ(unary->op, Operator::Increment);
    EXPECT_TRUE(unary->postfix);  // 后缀
}

// 函数调用
TEST(ParserTest, ExprCall) {
    auto result = parseProgram("函数 测试() -> 整32 { 返回 加(10, 20); }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Expr* expr = returnExpr(result.program.get());
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getType(), NodeType::CallExpr);
    CallExpr* call = static_cast<CallExpr*>(expr);
    ASSERT_EQ(call->callee->getType(), NodeType::IdentifierExpr);
    EXPECT_EQ(static_cast<IdentifierExpr*>(call->callee.get())->name, "加");
    ASSERT_EQ(call->arguments.size(), 2u);
    EXPECT_EQ(call->arguments[0]->getType(), NodeType::IntegerLiteral);
    EXPECT_EQ(call->arguments[1]->getType(), NodeType::IntegerLiteral);
}

// 成员访问 obj.member
TEST(ParserTest, ExprMemberAccess) {
    auto result = parseProgram("函数 测试() -> 整32 { 返回 对象.成员; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Expr* expr = returnExpr(result.program.get());
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getType(), NodeType::MemberExpr);
    MemberExpr* member = static_cast<MemberExpr*>(expr);
    EXPECT_EQ(member->memberName, "成员");
    EXPECT_FALSE(member->isDerefAccess);
    ASSERT_EQ(member->object->getType(), NodeType::IdentifierExpr);
}

// 箭头成员访问 obj->member 已废除（v2.1，2026-09-03）：硬错误+迁移提示，
// 恢复路径按 . 折叠 MemberExpr（isDerefAccess=false）
TEST(ParserTest, ExprArrowAccessRejected) {
    auto result = parseProgram("函数 测试() -> 整32 { 返回 指针->成员; }");
    EXPECT_TRUE(result.diagnostics.hasErrors());
    Expr* expr = returnExpr(result.program.get());
    if (expr != nullptr) {  // 恢复路径仍折叠出 MemberExpr（防级联报错）
        ASSERT_EQ(expr->getType(), NodeType::MemberExpr);
        MemberExpr* member = static_cast<MemberExpr*>(expr);
        EXPECT_EQ(member->memberName, "成员");
        EXPECT_FALSE(member->isDerefAccess);
    }
}

// 混合表达式：含多种运算符与括号
TEST(ParserTest, ExprMixed) {
    auto result = parseProgram(
        "函数 测试() -> 整32 { 返回 (a + b) * 2 - c / d % e; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Expr* expr = returnExpr(result.program.get());
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getType(), NodeType::BinaryExpr);
    BinaryExpr* top = static_cast<BinaryExpr*>(expr);
    EXPECT_EQ(top->op, Operator::Subtract);
    // 左侧：(a + b) * 2
    ASSERT_EQ(top->left->getType(), NodeType::BinaryExpr);
    EXPECT_EQ(static_cast<BinaryExpr*>(top->left.get())->op, Operator::Multiply);
    // 右侧：c / d % e（% 优先级最低，/ 和 % 同层左结合）
    ASSERT_EQ(top->right->getType(), NodeType::BinaryExpr);
    BinaryExpr* right = static_cast<BinaryExpr*>(top->right.get());
    EXPECT_EQ(right->op, Operator::Modulo);
    EXPECT_EQ(right->left->getType(), NodeType::BinaryExpr);
    EXPECT_EQ(static_cast<BinaryExpr*>(right->left.get())->op, Operator::Divide);
}

// ==================== 4. 控制流解析 ====================

// 如果语句（无否则）
TEST(ParserTest, IfStmtBasic) {
    auto result = parseProgram("函数 测试() { 如果 (x > 0) { 返回 1; } }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Stmt* stmt = firstStmt(result.program.get());
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->getType(), NodeType::IfStmt);
    IfStmt* ifStmt = static_cast<IfStmt*>(stmt);
    ASSERT_NE(ifStmt->condition, nullptr);
    EXPECT_EQ(ifStmt->condition->getType(), NodeType::BinaryExpr);
    ASSERT_NE(ifStmt->thenBranch, nullptr);
    EXPECT_EQ(ifStmt->thenBranch->statements.size(), 1u);
    EXPECT_EQ(ifStmt->elseBranch, nullptr);
}

// 如果-否则语句
TEST(ParserTest, IfElseStmt) {
    auto result = parseProgram(
        "函数 测试() { 如果 (x > 0) { 返回 1; } 否则 { 返回 2; } }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    IfStmt* ifStmt = static_cast<IfStmt*>(firstStmt(result.program.get()));
    ASSERT_NE(ifStmt, nullptr);
    ASSERT_NE(ifStmt->elseBranch, nullptr);
    EXPECT_EQ(ifStmt->elseBranch->getType(), NodeType::BlockStmt);
}

// 如果-否则如果-否则链
TEST(ParserTest, IfElseIfChain) {
    auto result = parseProgram(
        "函数 测试() { 如果 (x > 0) { 返回 1; } "
        "否则 如果 (x < 0) { 返回 -1; } 否则 { 返回 0; } }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    IfStmt* ifStmt = static_cast<IfStmt*>(firstStmt(result.program.get()));
    ASSERT_NE(ifStmt, nullptr);
    // 否则分支应为嵌套IfStmt
    ASSERT_NE(ifStmt->elseBranch, nullptr);
    EXPECT_EQ(ifStmt->elseBranch->getType(), NodeType::IfStmt);
    IfStmt* nestedIf = static_cast<IfStmt*>(ifStmt->elseBranch.get());
    ASSERT_NE(nestedIf->elseBranch, nullptr);
    EXPECT_EQ(nestedIf->elseBranch->getType(), NodeType::BlockStmt);
}

// 当循环
TEST(ParserTest, WhileStmt) {
    auto result = parseProgram(
        "函数 测试() { 当 (i < 10) { i++; } }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Stmt* stmt = firstStmt(result.program.get());
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->getType(), NodeType::WhileStmt);
    WhileStmt* whileStmt = static_cast<WhileStmt*>(stmt);
    ASSERT_NE(whileStmt->condition, nullptr);
    ASSERT_NE(whileStmt->body, nullptr);
    EXPECT_EQ(whileStmt->body->statements.size(), 1u);
}

// 循环语句（for风格）
TEST(ParserTest, ForStmt) {
    auto result = parseProgram(
        "函数 测试() { 循环 (整32 i = 0; i < 10; i++) { 打印(i); } }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Stmt* stmt = firstStmt(result.program.get());
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->getType(), NodeType::ForStmt);
    ForStmt* forStmt = static_cast<ForStmt*>(stmt);
    ASSERT_NE(forStmt->init, nullptr);
    EXPECT_EQ(forStmt->init->getType(), NodeType::VarDecl);
    ASSERT_NE(forStmt->condition, nullptr);
    ASSERT_NE(forStmt->update, nullptr);
    ASSERT_NE(forStmt->body, nullptr);
    EXPECT_EQ(forStmt->body->statements.size(), 1u);
}

// 循环语句（无限循环）
TEST(ParserTest, InfiniteLoop) {
    auto result = parseProgram("函数 测试() { 循环 { 中断; } }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Stmt* stmt = firstStmt(result.program.get());
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->getType(), NodeType::ForStmt);
    ForStmt* forStmt = static_cast<ForStmt*>(stmt);
    EXPECT_EQ(forStmt->init, nullptr);
    EXPECT_EQ(forStmt->condition, nullptr);
    EXPECT_EQ(forStmt->update, nullptr);
    ASSERT_NE(forStmt->body, nullptr);
}

// 返回语句（带值/不带值）
TEST(ParserTest, ReturnStmt) {
    auto result = parseProgram("函数 测试() -> 整32 { 返回 42; } 函数 空() { 返回; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ASSERT_EQ(result.program->declarations.size(), 2u);
    // 第一个函数：返回带值
    Stmt* stmt = firstStmt(result.program.get());
    ASSERT_EQ(stmt->getType(), NodeType::ReturnStmt);
    ReturnStmt* ret = static_cast<ReturnStmt*>(stmt);
    ASSERT_NE(ret->value, nullptr);
    EXPECT_EQ(ret->value->getType(), NodeType::IntegerLiteral);
    // 第二个函数：返回无值
    FunctionDecl* func2 = result.program->declarations[1].get();
    Stmt* stmt2 = func2->body->statements[0].get();
    ASSERT_EQ(stmt2->getType(), NodeType::ReturnStmt);
    EXPECT_EQ(static_cast<ReturnStmt*>(stmt2)->value, nullptr);
}

// 中断语句
TEST(ParserTest, BreakStmt) {
    auto result = parseProgram("函数 测试() { 循环 { 中断; } }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ForStmt* forStmt = static_cast<ForStmt*>(firstStmt(result.program.get()));
    ASSERT_NE(forStmt, nullptr);
    Stmt* inner = forStmt->body->statements[0].get();
    ASSERT_EQ(inner->getType(), NodeType::BreakStmt);
}

// 继续语句
TEST(ParserTest, ContinueStmt) {
    auto result = parseProgram(
        "函数 测试() { 循环 (整32 i = 0; i < 10; i++) { 如果 (i % 2 == 0) { 继续; } } }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ForStmt* forStmt = static_cast<ForStmt*>(firstStmt(result.program.get()));
    ASSERT_NE(forStmt, nullptr);
    IfStmt* ifStmt = static_cast<IfStmt*>(forStmt->body->statements[0].get());
    ASSERT_NE(ifStmt, nullptr);
    Stmt* inner = ifStmt->thenBranch->statements[0].get();
    ASSERT_EQ(inner->getType(), NodeType::ContinueStmt);
}

// ==================== 5. 代码块解析 ====================

// 嵌套代码块
TEST(ParserTest, NestedBlock) {
    auto result = parseProgram(
        "函数 测试() { 如果 (x) { { 变量 y = 1; } } }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    IfStmt* ifStmt = static_cast<IfStmt*>(firstStmt(result.program.get()));
    ASSERT_NE(ifStmt, nullptr);
    // 真分支内第一个语句是代码块
    Stmt* inner = ifStmt->thenBranch->statements[0].get();
    ASSERT_EQ(inner->getType(), NodeType::BlockStmt);
    BlockStmt* block = static_cast<BlockStmt*>(inner);
    ASSERT_EQ(block->statements.size(), 1u);
    EXPECT_EQ(block->statements[0]->getType(), NodeType::VarDecl);
}

// 空代码块
TEST(ParserTest, EmptyBlock) {
    auto result = parseProgram("函数 测试() { }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    FunctionDecl* func = firstFunction(result.program.get());
    ASSERT_NE(func->body, nullptr);
    EXPECT_TRUE(func->body->statements.empty());
}

// ==================== 6. 错误处理 ====================

// 括号不匹配：缺少右括号
TEST(ParserTest, ErrorMissingRightParen) {
    auto result = parseProgram("函数 测试() -> 整32 { 返回 (a + b }");
    EXPECT_TRUE(result.diagnostics.hasErrors());
    // 错误恢复后应仍能产出函数
    FunctionDecl* func = firstFunction(result.program.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->name, "测试");
}

// 缺少右花括号
TEST(ParserTest, ErrorMissingRightBrace) {
    auto result = parseProgram("函数 测试() { 变量 x = 1;");
    EXPECT_TRUE(result.diagnostics.hasErrors());
    // 到达EOF也应有函数
    FunctionDecl* func = firstFunction(result.program.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->name, "测试");
}

// 函数名缺失
TEST(ParserTest, ErrorMissingFunctionName) {
    auto result = parseProgram("函数 () -> 整32 { 返回 1; };");
    EXPECT_TRUE(result.diagnostics.hasErrors());
}

// 非法顶层声明
TEST(ParserTest, ErrorInvalidTopLevel) {
    auto result = parseProgram("这不是合法声明");
    EXPECT_TRUE(result.diagnostics.hasErrors());
    // 错误恢复后到达EOF，不崩溃
}

// 表达式后跟无关Token（x = 1 + ）
TEST(ParserTest, ErrorInvalidExpression) {
    auto result = parseProgram("函数 测试() -> 整32 { 返回 a + }");
    EXPECT_TRUE(result.diagnostics.hasErrors());
    // 不应崩溃，程序可产出
    FunctionDecl* func = firstFunction(result.program.get());
    ASSERT_NE(func, nullptr);
}

// 错误恢复：多个错误后继续解析后续函数
TEST(ParserTest, ErrorRecoveryContinue) {
    auto result = parseProgram("函数 坏() { 返回 (a } 函数 好() { 返回 1 }");
    EXPECT_TRUE(result.diagnostics.hasErrors());
    // 第二个函数仍应被解析
    ASSERT_EQ(result.program->declarations.size(), 2u);
    EXPECT_EQ(result.program->declarations[1]->name, "好");
}

// ==================== 7. 混合代码测试 ====================

// 完整示例：阶乘（递归 + 条件 + 循环）
TEST(ParserTest, MixedFactorial) {
    auto result = parseProgram(
        "函数 阶乘(整32 n) -> 整32 {"
        "  如果 (n <= 1) { 返回 1; }"
        "  返回 n * 阶乘(n - 1)"
        ";}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    FunctionDecl* func = firstFunction(result.program.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->name, "阶乘");
    ASSERT_EQ(func->params.size(), 1u);
    ASSERT_EQ(func->body->statements.size(), 2u);
    EXPECT_EQ(func->body->statements[0]->getType(), NodeType::IfStmt);
    EXPECT_EQ(func->body->statements[1]->getType(), NodeType::ReturnStmt);
}

// 完整示例：循环求和（for风格 + 赋值 + 自增）
TEST(ParserTest, MixedSumLoop) {
    auto result = parseProgram(
        "函数 求和(整32 n) -> 整32 {"
        "  整32 总和 = 0"
        ";  循环 (整32 i = 1; i <= n; i++) {"
        "    总和 += i"
        ";  }"
        "  返回 总和"
        ";}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    FunctionDecl* func = firstFunction(result.program.get());
    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->body->statements.size(), 3u);
    // 第一条：变量声明
    EXPECT_EQ(func->body->statements[0]->getType(), NodeType::VarDecl);
    // 第二条：循环
    EXPECT_EQ(func->body->statements[1]->getType(), NodeType::ForStmt);
    // 第三条：返回
    EXPECT_EQ(func->body->statements[2]->getType(), NodeType::ReturnStmt);
}

// 完整示例：主函数调用其他函数
TEST(ParserTest, MixedMain) {
    auto result = parseProgram(
        "函数 打印问候() { 打印行(\"你好\"); }"
        "函数 主() -> 整32 {"
        "  打印问候()"
        ";  返回 0"
        ";}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ASSERT_EQ(result.program->declarations.size(), 2u);
    // 第二个函数主
    FunctionDecl* main = result.program->declarations[1].get();
    ASSERT_EQ(main->body->statements.size(), 2u);
    // 第一条：表达式语句（调用）
    EXPECT_EQ(main->body->statements[0]->getType(), NodeType::ExprStmt);
    // 第二条：返回
    EXPECT_EQ(main->body->statements[1]->getType(), NodeType::ReturnStmt);
}

// 混合：冒号后置 + 类型前置混用——A8 收口（2026-09-14）后冒号后置一律报错
// （原「混合兼容解析」断言随语法移除而迁移为负断言；E2E 268/269 同步负测锚定）
TEST(ParserTest, MixedTypeSyntax) {
    auto result = parseProgram(
        "函数 测试(a: 整32, 整32 b) -> 整32 {"
        "  变量 x: 整32 = a"
        ";  整32 y = b"
        ";  返回 x + y"
        ";}");
    EXPECT_TRUE(result.diagnostics.hasErrors());
}

// 布尔字面量
TEST(ParserTest, BoolLiteralExpr) {
    auto result = parseProgram("函数 测试() -> 布尔 { 返回 真 && 假; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Expr* expr = returnExpr(result.program.get());
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getType(), NodeType::BinaryExpr);
    BinaryExpr* top = static_cast<BinaryExpr*>(expr);
    EXPECT_EQ(top->op, Operator::AndAnd);
    EXPECT_EQ(top->left->getType(), NodeType::BoolLiteral);
    EXPECT_EQ(static_cast<BoolLiteral*>(top->left.get())->value, true);
    EXPECT_EQ(static_cast<BoolLiteral*>(top->right.get())->value, false);
}

// 整数/浮点字面量值
TEST(ParserTest, LiteralValues) {
    auto result = parseProgram(
        "函数 测试() -> 浮64 { 返回 3.14; } 函数 整() -> 整32 { 返回 42; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ASSERT_EQ(result.program->declarations.size(), 2u);
    // 第一个函数返回浮点
    Stmt* stmt0 = result.program->declarations[0]->body->statements[0].get();
    ASSERT_EQ(stmt0->getType(), NodeType::ReturnStmt);
    Expr* floatExpr = static_cast<ReturnStmt*>(stmt0)->value.get();
    ASSERT_NE(floatExpr, nullptr);
    ASSERT_EQ(floatExpr->getType(), NodeType::FloatLiteral);
    EXPECT_DOUBLE_EQ(static_cast<FloatLiteral*>(floatExpr)->value, 3.14);
    // 第二个函数返回整数
    Stmt* stmt1 = result.program->declarations[1]->body->statements[0].get();
    ASSERT_EQ(stmt1->getType(), NodeType::ReturnStmt);
    Expr* intExpr = static_cast<ReturnStmt*>(stmt1)->value.get();
    ASSERT_NE(intExpr, nullptr);
    ASSERT_EQ(intExpr->getType(), NodeType::IntegerLiteral);
    EXPECT_EQ(static_cast<IntegerLiteral*>(intExpr)->value, 42);
}

// ==================== 8. 语句终止与续行（cn-language-spec 01a第三节 / 03第零节） ====================

// ---- 规则1：语句终止（plans/015 方案B 反转：强制分号终结） ----

// 变量声明分号终结（原「无分号合法」断言随规范反转，源码已迁移）
TEST(ParserTest, StmtSemicolonTerminatedVarDecl) {
    auto result = parseProgram("函数 测试() { 变量 a = 1; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ASSERT_EQ(firstFunction(result.program.get())->body->statements.size(), 1u);
    EXPECT_EQ(firstStmt(result.program.get())->getType(), NodeType::VarDecl);
}

// 返回语句分号终结（原「无分号合法」断言随规范反转，源码已迁移）
TEST(ParserTest, StmtSemicolonTerminatedReturn) {
    auto result = parseProgram("函数 测试() -> 整32 { 返回 1; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ASSERT_EQ(firstStmt(result.program.get())->getType(), NodeType::ReturnStmt);
}

// 表达式语句分号终结（原「无分号合法」断言随规范反转，源码已迁移）
TEST(ParserTest, StmtSemicolonTerminatedExpr) {
    auto result = parseProgram("函数 测试() { 打印行(\"你好\"); }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ASSERT_EQ(firstStmt(result.program.get())->getType(), NodeType::ExprStmt);
}

// 两条语句均分号终结，语句数一致（原「可选分号」断言随规范反转）
TEST(ParserTest, StmtSemicolonTerminatedPair) {
    auto withSemi = parseProgram("函数 测试() { 变量 a = 1; 变量 b = 2; }");
    ASSERT_FALSE(withSemi.diagnostics.hasErrors());
    EXPECT_EQ(firstFunction(withSemi.program.get())->body->statements.size(), 2u);

    auto withoutSemi = parseProgram("函数 测试() { 变量 a = 1; 变量 b = 2; }");
    ASSERT_FALSE(withoutSemi.diagnostics.hasErrors());
    EXPECT_EQ(firstFunction(withoutSemi.program.get())->body->statements.size(), 2u);
}

// 单行双语句分号分隔解析为两条声明（原「关键字边界」形态随规范反转改为分号分隔）
TEST(ParserTest, StmtSemicolonSeparatedTwoDecls) {
    auto result = parseProgram("函数 测试() { 变量 x = 1; 整32 y = 2; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    FunctionDecl* func = firstFunction(result.program.get());
    ASSERT_EQ(func->body->statements.size(), 2u);
    // 第一条：变量 x = 1（类型推断 VarDecl）
    EXPECT_EQ(func->body->statements[0]->getType(), NodeType::VarDecl);
    // 第二条：整32 y = 2（类型前置 VarDecl）
    EXPECT_EQ(func->body->statements[1]->getType(), NodeType::VarDecl);
    VarDecl* second = static_cast<VarDecl*>(func->body->statements[1].get());
    EXPECT_EQ(second->name, "y");
    EXPECT_EQ(second->typeName, "整32");
}

// ---- 规则2：续行（换行≡空格，天然无条件续行） ----

// 续行-括号：多行函数调用参数
TEST(ParserTest, StmtContinuationCallMultiLine) {
    auto result = parseProgram(
        "函数 测试() -> 整32 {\n"
        "  返回 加(10,\n"
        "            20);\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Expr* expr = returnExpr(result.program.get());
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getType(), NodeType::CallExpr);
    CallExpr* call = static_cast<CallExpr*>(expr);
    ASSERT_EQ(call->arguments.size(), 2u);
}

// 续行-括号：多行下标访问
TEST(ParserTest, StmtContinuationIndexMultiLine) {
    auto result = parseProgram(
        "函数 测试() -> 整32 {\n"
        "  返回 数组[\n"
        "      1 + 2\n"
        "  ];\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Expr* expr = returnExpr(result.program.get());
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getType(), NodeType::IndexExpr);
}

// 续行-括号：多行括号表达式分组
TEST(ParserTest, StmtContinuationParenMultiLine) {
    auto result = parseProgram(
        "函数 测试() -> 整32 {\n"
        "  返回 (1 +\n"
        "          2) * 3;\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Expr* expr = returnExpr(result.program.get());
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getType(), NodeType::BinaryExpr);
    BinaryExpr* top = static_cast<BinaryExpr*>(expr);
    EXPECT_EQ(top->op, Operator::Multiply);
    // 左侧是括号分组 (1+2)
    ASSERT_EQ(top->left->getType(), NodeType::BinaryExpr);
    EXPECT_EQ(static_cast<BinaryExpr*>(top->left.get())->op, Operator::Add);
}

// 续行-行尾运算符：变量 s = 1 +\n 2 合法且等于 3
TEST(ParserTest, StmtContinuationOperatorEndOfLine) {
    auto result = parseProgram(
        "函数 测试() -> 整32 {\n"
        "  变量 s = 1 +\n"
        "          2;\n"
        "  返回 s;\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    FunctionDecl* func = firstFunction(result.program.get());
    ASSERT_EQ(func->body->statements.size(), 2u);
    // 第一条：变量声明，初始化器是 1 + 2（Add）
    VarDecl* decl = static_cast<VarDecl*>(func->body->statements[0].get());
    ASSERT_EQ(decl->getType(), NodeType::VarDecl);
    ASSERT_NE(decl->initializer, nullptr);
    ASSERT_EQ(decl->initializer->getType(), NodeType::BinaryExpr);
    BinaryExpr* init = static_cast<BinaryExpr*>(decl->initializer.get());
    EXPECT_EQ(init->op, Operator::Add);
    // 第二条：返回
    EXPECT_EQ(func->body->statements[1]->getType(), NodeType::ReturnStmt);
}

// 行首运算符并入上一行：返回 a\n- b 实为 返回 a - b（换行≡空格）
TEST(ParserTest, StmtContinuationLeadingOperator) {
    auto result = parseProgram(
        "函数 测试() -> 整32 {\n"
        "  返回 a\n"
        "         - b;\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Expr* expr = returnExpr(result.program.get());
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getType(), NodeType::BinaryExpr);
    EXPECT_EQ(static_cast<BinaryExpr*>(expr)->op, Operator::Subtract);
}

// ---- 规则3：单行多语句（显式分号支持） ----

// 单行多语句：变量 i = 0; ++i; --i; 三条语句
TEST(ParserTest, StmtMultipleOnOneLine) {
    auto result = parseProgram("函数 测试() { 变量 i = 0; ++i; --i; }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    FunctionDecl* func = firstFunction(result.program.get());
    ASSERT_EQ(func->body->statements.size(), 3u);
    EXPECT_EQ(func->body->statements[0]->getType(), NodeType::VarDecl);
    EXPECT_EQ(func->body->statements[1]->getType(), NodeType::ExprStmt);
    EXPECT_EQ(func->body->statements[2]->getType(), NodeType::ExprStmt);
}

// ---- 规则4：空语句与空循环体（裸分号非法） ----

// 空语句非法：裸 ; 报"预期表达式"
TEST(ParserTest, StmtBareSemicolonIllegal) {
    auto result = parseProgram("函数 测试() { ; }");
    EXPECT_TRUE(result.diagnostics.hasErrors());
}

// 连续分号 ;; 报错
TEST(ParserTest, StmtDoubleSemicolonIllegal) {
    auto result = parseProgram("函数 测试() { ;; }");
    EXPECT_TRUE(result.diagnostics.hasErrors());
}

// 当(x); 是语法错误（循环体强制 {）
TEST(ParserTest, StmtWhileSemicolonIllegal) {
    auto result = parseProgram("函数 测试() { 当(x); }");
    EXPECT_TRUE(result.diagnostics.hasErrors());
}

// 合法空循环体：当(x) { }（空块合法）
TEST(ParserTest, StmtWhileEmptyBodyOk) {
    auto result = parseProgram("函数 测试() { 当(x) { } }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Stmt* stmt = firstStmt(result.program.get());
    ASSERT_EQ(stmt->getType(), NodeType::WhileStmt);
    WhileStmt* whileStmt = static_cast<WhileStmt*>(stmt);
    ASSERT_NE(whileStmt->body, nullptr);
    EXPECT_TRUE(whileStmt->body->statements.empty());
}

// 循环头三段分号合法：循环(整32 j = 0; j < 10; j++) { }
TEST(ParserTest, StmtForHeaderSemicolonsOk) {
    auto result = parseProgram("函数 测试() { 循环(整32 j = 0; j < 10; j++) { } }");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    Stmt* stmt = firstStmt(result.program.get());
    ASSERT_EQ(stmt->getType(), NodeType::ForStmt);
    ForStmt* forStmt = static_cast<ForStmt*>(stmt);
    // 三段齐全：init（VarDecl）、condition（j < 10）、update（j++）
    ASSERT_NE(forStmt->init, nullptr);
    EXPECT_EQ(forStmt->init->getType(), NodeType::VarDecl);
    ASSERT_NE(forStmt->condition, nullptr);
    ASSERT_EQ(forStmt->condition->getType(), NodeType::BinaryExpr);
    ASSERT_NE(forStmt->update, nullptr);
    ASSERT_EQ(forStmt->update->getType(), NodeType::UnaryExpr);
    UnaryExpr* upd = static_cast<UnaryExpr*>(forStmt->update.get());
    EXPECT_EQ(upd->op, Operator::Increment);
    EXPECT_TRUE(upd->postfix);  // j++ 后缀
}

// 循环头缺分号报错：循环(整32 j = 0; j < 10) { }
TEST(ParserTest, StmtForHeaderMissingSemicolonIllegal) {
    auto result = parseProgram("函数 测试() { 循环(整32 j = 0; j < 10) { } }");
    EXPECT_TRUE(result.diagnostics.hasErrors());
}

// ---- 规则1（2026-09-04 plans/015 方案B 反转）：语句必须 ';' 终结 ----

// 缺分号变量声明报错（原「无分号合法」断言随规范反转废止，见 plans/015）
TEST(ParserTest, StmtMissingSemicolonVarDeclIllegal) {
    auto result = parseProgram("函数 测试() { 变量 a = 1 }");
    ASSERT_TRUE(result.diagnostics.hasErrors());
    // 诊断信息含「语句缺少分号」
    bool found = false;
    for (const auto& d : result.diagnostics.getAll()) {
        if (d.message.find("语句缺少分号") != std::string::npos) found = true;
    }
    EXPECT_TRUE(found);
}

// 缺分号返回语句报错（} 前语句同样须终结）
TEST(ParserTest, StmtMissingSemicolonReturnIllegal) {
    auto result = parseProgram("函数 测试() -> 整32 { 返回 1 }");
    EXPECT_TRUE(result.diagnostics.hasErrors());
}

// 缺分号表达式语句报错（两条相邻语句间无分号——原静默粘连家族现显式诊断）
TEST(ParserTest, StmtMissingSemicolonExprIllegal) {
    auto result = parseProgram("函数 测试() { 打印行(\"你好\") 返回 0; }");
    EXPECT_TRUE(result.diagnostics.hasErrors());
}

// 顶层常量声明缺分号报错（plans/015 裁决：常量 T = 2; 强制）
TEST(ParserTest, TopLevelConstMissingSemicolonIllegal) {
    auto result = parseProgram("常量 上限 = 10 函数 测试() -> 整32 { 返回 上限; }");
    EXPECT_TRUE(result.diagnostics.hasErrors());
}

// 顶层静态声明缺分号报错（plans/015 裁决：静态 类型 名 = 值; 强制）
TEST(ParserTest, TopLevelStaticMissingSemicolonIllegal) {
    auto result = parseProgram("静态 整32 计数 = 0 函数 测试() -> 整32 { 返回 计数; }");
    EXPECT_TRUE(result.diagnostics.hasErrors());
}

// 导入语句缺分号报错（plans/015 裁决：导入 路径; 强制，Rust use 同款）
TEST(ParserTest, ImportMissingSemicolonIllegal) {
    auto result = parseProgram("导入 容器::向量 函数 测试() -> 整32 { 返回 0; }");
    EXPECT_TRUE(result.diagnostics.hasErrors());
}

// 带分号全形态合法（正测锚定：分号终结 + } 自终结不加分号）
TEST(ParserTest, StmtSemicolonTerminatedAllLegal) {
    auto result = parseProgram(
        "常量 上限 = 10;\n"
        "导入 容器::向量;\n"
        "函数 测试() -> 整32 {\n"
        "  变量 a = 1;\n"
        "  如果 (a > 0) { a = 2; }\n"
        "  返回 a;\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    EXPECT_EQ(firstFunction(result.program.get())->body->statements.size(), 3u);
}
