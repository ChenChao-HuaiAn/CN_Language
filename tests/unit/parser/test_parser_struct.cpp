// 结构体/枚举/联合体语法解析单元测试（Task 2.7）
// 覆盖：结构体定义解析（字段列表）、联合体定义解析、枚举定义解析（显式赋值/负数）、
//       结构体初始化（点{ x = 1, y = 2 }）、. 与 -> 成员访问解析、嵌套结构体初始化
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
using cn_compiler::Diagnostics;
using cn_compiler::EnumDecl;
using cn_compiler::Expr;
using cn_compiler::ExprStmt;
using cn_compiler::FunctionDecl;
using cn_compiler::Lexer;
using cn_compiler::MemberExpr;
using cn_compiler::NodeType;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::Stmt;
using cn_compiler::StructDecl;
using cn_compiler::StructInitExpr;
using cn_compiler::VarDecl;

namespace {

// 辅助：解析源码，返回程序AST与诊断引擎
struct ParseResult {
    std::unique_ptr<Program> program;
    Diagnostics diagnostics;
};

ParseResult parseProgram(const std::string& source) {
    ParseResult result;
    Lexer lexer(source, "结构体解析测试.cn", result.diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(result.diagnostics);
    result.program = parser.parse(tokens);
    return result;
}

} // namespace

// 结构体定义解析：字段列表
TEST(ParserStructTest, StructDecl) {
    ParseResult r = parseProgram(
        "结构体 点 {\n"
        "    整32 x;\n"
        "    整32 y;\n"
        "}\n"
        "函数 主() -> 整32 { 返回 0; }\n");
    ASSERT_EQ(r.program->structs.size(), 1u);
    StructDecl* decl = r.program->structs[0].get();
    EXPECT_EQ(decl->name, "点");
    EXPECT_FALSE(decl->isUnion);
    ASSERT_EQ(decl->fields.size(), 2u);
    EXPECT_EQ(decl->fields[0].name, "x");
    EXPECT_EQ(decl->fields[0].type, "整32");
    EXPECT_EQ(decl->fields[1].name, "y");
}

// 结构体嵌套字段（字段类型为另一个结构体名）
TEST(ParserStructTest, StructNestedField) {
    ParseResult r = parseProgram(
        "结构体 点 { 整32 x; }\n"
        "结构体 矩形 { 点 左上; }\n"
        "函数 主() -> 整32 { 返回 0; }\n");
    ASSERT_EQ(r.program->structs.size(), 2u);
    StructDecl* rect = r.program->structs[1].get();
    EXPECT_EQ(rect->name, "矩形");
    ASSERT_EQ(rect->fields.size(), 1u);
    EXPECT_EQ(rect->fields[0].type, "点");
}

// 联合体定义解析：isUnion 标记
TEST(ParserStructTest, UnionDecl) {
    ParseResult r = parseProgram(
        "联合体 数值 {\n"
        "    整32 整数部分;\n"
        "    浮64 浮点部分;\n"
        "}\n"
        "函数 主() -> 整32 { 返回 0; }\n");
    ASSERT_EQ(r.program->structs.size(), 1u);
    StructDecl* decl = r.program->structs[0].get();
    EXPECT_EQ(decl->name, "数值");
    EXPECT_TRUE(decl->isUnion);
    ASSERT_EQ(decl->fields.size(), 2u);
    EXPECT_EQ(decl->fields[0].name, "整数部分");
    EXPECT_EQ(decl->fields[1].name, "浮点部分");
}

// 枚举定义解析：自动递增成员（无显式值）
TEST(ParserStructTest, EnumDeclAuto) {
    ParseResult r = parseProgram(
        "枚举 颜色 {\n"
        "    红,\n"
        "    绿,\n"
        "    蓝\n"
        "}\n"
        "函数 主() -> 整32 { 返回 0; }\n");
    ASSERT_EQ(r.program->enums.size(), 1u);
    EnumDecl* decl = r.program->enums[0].get();
    EXPECT_EQ(decl->name, "颜色");
    ASSERT_EQ(decl->members.size(), 3u);
    EXPECT_EQ(decl->members[0].name, "红");
    EXPECT_EQ(decl->members[1].name, "绿");
    EXPECT_EQ(decl->members[2].name, "蓝");
}

// 枚举定义解析：显式赋值 + 负数
TEST(ParserStructTest, EnumDeclExplicitAndNegative) {
    ParseResult r = parseProgram(
        "枚举 方向 {\n"
        "    上 = -1,\n"
        "    中 = 0,\n"
        "    下 = 1\n"
        "}\n"
        "函数 主() -> 整32 { 返回 0; }\n");
    ASSERT_EQ(r.program->enums.size(), 1u);
    EnumDecl* decl = r.program->enums[0].get();
    ASSERT_EQ(decl->members.size(), 3u);
    EXPECT_EQ(decl->members[0].name, "上");
    EXPECT_TRUE(decl->members[0].explicitValue);
    EXPECT_EQ(decl->members[0].value, -1);
    EXPECT_EQ(decl->members[2].value, 1);
}

// 结构体初始化表达式：类型名{ 字段 = 值, ... }
TEST(ParserStructTest, StructInitExpr) {
    ParseResult r = parseProgram(
        "函数 主() -> 整32 {\n"
        "    点 p = 点{ x = 10, y = 20 };\n"
        "    返回 0;\n"
        "}\n");
    ASSERT_EQ(r.program->declarations.size(), 1u);
    FunctionDecl* func = r.program->declarations[0].get();
    ASSERT_GE(func->body->statements.size(), 1u);
    Stmt* stmt = func->body->statements[0].get();
    ASSERT_EQ(stmt->getType(), NodeType::VarDecl);
    VarDecl* varDecl = static_cast<VarDecl*>(stmt);
    EXPECT_EQ(varDecl->typeName, "点");
    ASSERT_EQ(varDecl->initializer->getType(), NodeType::StructInitExpr);
    StructInitExpr* init = static_cast<StructInitExpr*>(varDecl->initializer.get());
    EXPECT_EQ(init->typeName, "点");
    ASSERT_EQ(init->fields.size(), 2u);
    EXPECT_EQ(init->fields[0].first, "x");
    EXPECT_EQ(init->fields[1].first, "y");
}

// 成员访问解析：. 与 ->（对象.字段 / 对象->字段）
TEST(ParserStructTest, MemberAccessDotAndArrow) {
    ParseResult r = parseProgram(
        "函数 主() -> 整32 {\n"
        "    整32 a = p.x;\n"
        "    整32 b = ptr.y;\n"
        "    返回 0;\n"
        "}\n");
    ASSERT_EQ(r.program->declarations.size(), 1u);
    FunctionDecl* func = r.program->declarations[0].get();
    ASSERT_GE(func->body->statements.size(), 2u);
    // p.x（值对象）
    VarDecl* v1 = static_cast<VarDecl*>(func->body->statements[0].get());
    ASSERT_EQ(v1->initializer->getType(), NodeType::MemberExpr);
    MemberExpr* m1 = static_cast<MemberExpr*>(v1->initializer.get());
    EXPECT_EQ(m1->memberName, "x");
    EXPECT_FALSE(m1->isDerefAccess);
    // ptr.y（v2.1 统一 .：解析层恒 false，语义层按对象是否为指针置位）
    VarDecl* v2 = static_cast<VarDecl*>(func->body->statements[1].get());
    ASSERT_EQ(v2->initializer->getType(), NodeType::MemberExpr);
    MemberExpr* m2 = static_cast<MemberExpr*>(v2->initializer.get());
    EXPECT_EQ(m2->memberName, "y");
    EXPECT_FALSE(m2->isDerefAccess);
}

// 嵌套结构体初始化：矩形{ 左上 = 点{ x = 1, y = 2 }, ... }
TEST(ParserStructTest, NestedStructInit) {
    ParseResult r = parseProgram(
        "函数 主() -> 整32 {\n"
        "    矩形 r = 矩形{ 左上 = 点{ x = 1, y = 2 } };\n"
        "    返回 0;\n"
        "}\n");
    FunctionDecl* func = r.program->declarations[0].get();
    VarDecl* varDecl = static_cast<VarDecl*>(func->body->statements[0].get());
    ASSERT_EQ(varDecl->initializer->getType(), NodeType::StructInitExpr);
    StructInitExpr* outer = static_cast<StructInitExpr*>(varDecl->initializer.get());
    ASSERT_EQ(outer->fields.size(), 1u);
    EXPECT_EQ(outer->fields[0].first, "左上");
    // 嵌套字段值：点{ x = 1, y = 2 }
    ASSERT_EQ(outer->fields[0].second->getType(), NodeType::StructInitExpr);
    StructInitExpr* inner = static_cast<StructInitExpr*>(outer->fields[0].second.get());
    EXPECT_EQ(inner->typeName, "点");
    EXPECT_EQ(inner->fields.size(), 2u);
}

// 结构体声明错误恢复：缺少右花括号报告错误（不会崩溃）
TEST(ParserStructTest, StructDeclMissingBrace) {
    ParseResult r = parseProgram(
        "结构体 点 { 整32 x;\n"
        "函数 主() -> 整32 { 返回 0; }\n");
    // 缺失 } 触发 consume 错误；解析器尽力恢复（结构体声明已部分构建）
    EXPECT_TRUE(r.diagnostics.hasErrors());
}
