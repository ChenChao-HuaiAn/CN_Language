// 数组与指针语法解析单元测试（Task 2.4）
// 覆盖：数组类型解析（整32[5]）、指针类型解析（整32*）、下标访问（数据[i]）、
//       取地址/解引用（&x / *p）、优先级（*p + 1 vs *(p+1)）、-> 语法解析、
//       空指针字面量（无）、初始化列表（{ 1, 2, 3 }）
// 测试方式：通过 Lexer + Parser 得到真实AST（全链路，非Mock）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"

using cn_compiler::AssignmentExpr;
using cn_compiler::BinaryExpr;
using cn_compiler::Diagnostics;
using cn_compiler::Expr;
using cn_compiler::ExprStmt;
using cn_compiler::FunctionDecl;
using cn_compiler::IndexExpr;
using cn_compiler::InitListExpr;
using cn_compiler::Lexer;
using cn_compiler::MemberExpr;
using cn_compiler::NodeType;
using cn_compiler::NullLiteral;
using cn_compiler::Operator;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::Stmt;
using cn_compiler::UnaryExpr;
using cn_compiler::VarDecl;

namespace {

// 辅助：解析源码，返回程序AST与诊断引擎
struct ParseResult {
    std::unique_ptr<Program> program;
    Diagnostics diagnostics;
};

ParseResult parseProgram(const std::string& source) {
    ParseResult result;
    Lexer lexer(source, "数组指针测试.cn", result.diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(result.diagnostics);
    result.program = parser.parse(tokens);
    return result;
}

// 取第一个函数的第n条语句
Stmt* firstStmt(Program* program, std::size_t index = 0) {
    if (program == nullptr || program->declarations.empty()) return nullptr;
    auto& func = program->declarations[0];
    if (func->body == nullptr) return nullptr;
    if (index >= func->body->statements.size()) return nullptr;
    return func->body->statements[index].get();
}

// 便捷：解析单条语句（包裹在函数内）
Stmt* parseStmt(const std::string& stmt, ParseResult& out) {
    std::string source = "函数 主() -> 整32 {\n" + stmt + "\n返回 0\n}";
    out = parseProgram(source);
    return firstStmt(out.program.get());
}

// 解包 ExprStmt 内部的表达式（赋值语句解析为 ExprStmt 包裹）
Expr* exprOf(Stmt* stmt) {
    if (stmt != nullptr && stmt->getType() == NodeType::ExprStmt) {
        return static_cast<ExprStmt*>(stmt)->expr.get();
    }
    return nullptr;
}

// 解包并转型为赋值表达式
AssignmentExpr* asAssign(Stmt* stmt) {
    Expr* e = exprOf(stmt);
    if (e != nullptr && e->getType() == NodeType::AssignmentExpr) {
        return static_cast<AssignmentExpr*>(e);
    }
    return nullptr;
}

} // namespace

// ==================== 数组类型解析 ====================

// 整32[5] 数据 = { 1, 2, 3 }：类型名为 整32[5]，初始值为初始化列表
TEST(ParserArrayPtrTest, ArrayDeclType) {
    ParseResult result;
    Stmt* stmt = parseStmt("整32[5] 数据 = { 1, 2, 3 }", result);
    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->getType(), NodeType::VarDecl);
    VarDecl* decl = static_cast<VarDecl*>(stmt);
    EXPECT_EQ(decl->typeName, "整32[5]");
    ASSERT_NE(decl->initializer, nullptr);
    EXPECT_EQ(decl->initializer->getType(), NodeType::InitListExpr);
    InitListExpr* list = static_cast<InitListExpr*>(decl->initializer.get());
    EXPECT_EQ(list->elements.size(), 3u);
}

// 指针类型 整32* p
TEST(ParserArrayPtrTest, PointerDeclType) {
    ParseResult result;
    Stmt* stmt = parseStmt("整32* p = 无", result);
    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->getType(), NodeType::VarDecl);
    VarDecl* decl = static_cast<VarDecl*>(stmt);
    EXPECT_EQ(decl->typeName, "整32*");
    ASSERT_NE(decl->initializer, nullptr);
    EXPECT_EQ(decl->initializer->getType(), NodeType::NullLiteral);
}

// 空类型* 指针
TEST(ParserArrayPtrTest, VoidPtrDecl) {
    ParseResult result;
    Stmt* stmt = parseStmt("空类型* vp = 无", result);
    ASSERT_NE(stmt, nullptr);
    VarDecl* decl = static_cast<VarDecl*>(stmt);
    EXPECT_EQ(decl->typeName, "空类型*");
}

// ==================== 下标访问 ====================

// 数据[i]：IndexExpr（对象=标识符，index=标识符）
TEST(ParserArrayPtrTest, IndexAccess) {
    ParseResult result;
    Stmt* stmt = parseStmt("x = 数据[i]", result);
    AssignmentExpr* assign = asAssign(stmt);
    ASSERT_NE(assign, nullptr);
    // 右值为 IndexExpr
    ASSERT_EQ(assign->value->getType(), NodeType::IndexExpr);
    IndexExpr* idx = static_cast<IndexExpr*>(assign->value.get());
    EXPECT_EQ(idx->object->getType(), NodeType::IdentifierExpr);
    EXPECT_EQ(idx->index->getType(), NodeType::IdentifierExpr);
}

// 数据[2] 常量下标
TEST(ParserArrayPtrTest, IndexConstant) {
    ParseResult result;
    Stmt* stmt = parseStmt("x = 数据[2]", result);
    AssignmentExpr* assign = asAssign(stmt);
    ASSERT_NE(assign, nullptr);
    ASSERT_EQ(assign->value->getType(), NodeType::IndexExpr);
    IndexExpr* idx = static_cast<IndexExpr*>(assign->value.get());
    EXPECT_EQ(idx->index->getType(), NodeType::IntegerLiteral);
}

// ==================== 取地址/解引用 ====================

// &x：UnaryExpr(AddressOf)
TEST(ParserArrayPtrTest, AddressOf) {
    ParseResult result;
    Stmt* stmt = parseStmt("p = &x", result);
    AssignmentExpr* assign = asAssign(stmt);
    ASSERT_NE(assign, nullptr);
    ASSERT_EQ(assign->value->getType(), NodeType::UnaryExpr);
    UnaryExpr* unary = static_cast<UnaryExpr*>(assign->value.get());
    EXPECT_EQ(unary->op, Operator::AddressOf);
    EXPECT_EQ(unary->operand->getType(), NodeType::IdentifierExpr);
}

// *p：UnaryExpr(Deref)
TEST(ParserArrayPtrTest, Deref) {
    ParseResult result;
    Stmt* stmt = parseStmt("x = *p", result);
    AssignmentExpr* assign = asAssign(stmt);
    ASSERT_NE(assign, nullptr);
    ASSERT_EQ(assign->value->getType(), NodeType::UnaryExpr);
    UnaryExpr* unary = static_cast<UnaryExpr*>(assign->value.get());
    EXPECT_EQ(unary->op, Operator::Deref);
}

// *p = 值：解引用左值赋值
TEST(ParserArrayPtrTest, DerefAssign) {
    ParseResult result;
    Stmt* stmt = parseStmt("*p = 42", result);
    AssignmentExpr* assign = asAssign(stmt);
    ASSERT_NE(assign, nullptr);
    ASSERT_EQ(assign->target->getType(), NodeType::UnaryExpr);
    UnaryExpr* unary = static_cast<UnaryExpr*>(assign->target.get());
    EXPECT_EQ(unary->op, Operator::Deref);
}

// ==================== 优先级 ====================

// *p + 1：Deref 绑定更紧（一元优先级12 > 加减10），结构为 (Deref p) + 1
TEST(ParserArrayPtrTest, PrecedenceDerefAdd) {
    ParseResult result;
    Stmt* stmt = parseStmt("x = *p + 1", result);
    AssignmentExpr* assign = asAssign(stmt);
    ASSERT_NE(assign, nullptr);
    ASSERT_EQ(assign->value->getType(), NodeType::BinaryExpr);
    BinaryExpr* bin = static_cast<BinaryExpr*>(assign->value.get());
    EXPECT_EQ(bin->op, Operator::Add);
    // 左操作数为 Deref
    ASSERT_EQ(bin->left->getType(), NodeType::UnaryExpr);
    UnaryExpr* unary = static_cast<UnaryExpr*>(bin->left.get());
    EXPECT_EQ(unary->op, Operator::Deref);
}

// *(p + 1)：显式括号，结构为 Deref(Add p 1)
TEST(ParserArrayPtrTest, PrecedenceParen) {
    ParseResult result;
    Stmt* stmt = parseStmt("x = *(p + 1)", result);
    AssignmentExpr* assign = asAssign(stmt);
    ASSERT_NE(assign, nullptr);
    ASSERT_EQ(assign->value->getType(), NodeType::UnaryExpr);
    UnaryExpr* unary = static_cast<UnaryExpr*>(assign->value.get());
    EXPECT_EQ(unary->op, Operator::Deref);
    // 操作数为 Add 表达式
    ASSERT_EQ(unary->operand->getType(), NodeType::BinaryExpr);
    BinaryExpr* bin = static_cast<BinaryExpr*>(unary->operand.get());
    EXPECT_EQ(bin->op, Operator::Add);
}

// &数组[0]：AddressOf 绑定 IndexExpr（取首元素地址）
TEST(ParserArrayPtrTest, AddressOfIndex) {
    ParseResult result;
    Stmt* stmt = parseStmt("p = &数据[0]", result);
    AssignmentExpr* assign = asAssign(stmt);
    ASSERT_NE(assign, nullptr);
    ASSERT_EQ(assign->value->getType(), NodeType::UnaryExpr);
    UnaryExpr* unary = static_cast<UnaryExpr*>(assign->value.get());
    EXPECT_EQ(unary->op, Operator::AddressOf);
    EXPECT_EQ(unary->operand->getType(), NodeType::IndexExpr);
}

// ==================== 成员访问（v2.1 统一 .） ====================

// -> 成员访问已废除（v2.1，2026-09-03）：解析层硬错误 + 迁移提示；
// 恢复策略按 . 折叠 MemberExpr（isDerefAccess=false）避免级联报错
TEST(ParserArrayPtrTest, ArrowMemberRejected) {
    ParseResult result;
    Stmt* stmt = parseStmt("x = p->字段", result);
    EXPECT_TRUE(result.diagnostics.hasErrors());
    // 恢复路径：仍折叠出 MemberExpr（产物不进入后续阶段，仅防级联）
    AssignmentExpr* assign = asAssign(stmt);
    ASSERT_NE(assign, nullptr);
    ASSERT_EQ(assign->value->getType(), NodeType::MemberExpr);
    MemberExpr* member = static_cast<MemberExpr*>(assign->value.get());
    EXPECT_FALSE(member->isDerefAccess);
    EXPECT_EQ(member->memberName, "字段");
}

// 点访问：. 成员访问（解析层 isDerefAccess 恒 false——语义层按对象类型置位）
TEST(ParserArrayPtrTest, DotMember) {
    ParseResult result;
    Stmt* stmt = parseStmt("x = 对象.字段", result);
    AssignmentExpr* assign = asAssign(stmt);
    ASSERT_NE(assign, nullptr);
    ASSERT_EQ(assign->value->getType(), NodeType::MemberExpr);
    MemberExpr* member = static_cast<MemberExpr*>(assign->value.get());
    EXPECT_FALSE(member->isDerefAccess);
}

// ==================== 初始化列表 ====================

// 空初始化列表 { } 允许（数组全零）
TEST(ParserArrayPtrTest, EmptyInitList) {
    ParseResult result;
    Stmt* stmt = parseStmt("整32[3] 数据 = { }", result);
    ASSERT_NE(stmt, nullptr);
    VarDecl* decl = static_cast<VarDecl*>(stmt);
    EXPECT_EQ(decl->typeName, "整32[3]");
    ASSERT_NE(decl->initializer, nullptr);
    EXPECT_EQ(decl->initializer->getType(), NodeType::InitListExpr);
    InitListExpr* list = static_cast<InitListExpr*>(decl->initializer.get());
    EXPECT_TRUE(list->elements.empty());
}
