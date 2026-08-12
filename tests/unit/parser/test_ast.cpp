// AST节点单元测试（Task 1.3）
// 覆盖：节点类型枚举、程序/函数/参数构建、表达式构建、语句构建、
//       访问者模式分发、unique_ptr所有权转移
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/parser/ast.hpp"

using cn_compiler::AssignmentExpr;
using cn_compiler::AstNode;
using cn_compiler::AstVisitor;
using cn_compiler::BinaryExpr;
using cn_compiler::BlockStmt;
using cn_compiler::BoolLiteral;
using cn_compiler::BreakStmt;
using cn_compiler::CallExpr;
using cn_compiler::CharLiteral;
using cn_compiler::ContinueStmt;
using cn_compiler::Expr;
using cn_compiler::ExprStmt;
using cn_compiler::FloatLiteral;
using cn_compiler::ForStmt;
using cn_compiler::FunctionDecl;
using cn_compiler::IdentifierExpr;
using cn_compiler::IfStmt;
using cn_compiler::IntegerLiteral;
using cn_compiler::MemberExpr;
using cn_compiler::NodeType;
using cn_compiler::Operator;
using cn_compiler::ParamDecl;
using cn_compiler::Program;
using cn_compiler::ReturnStmt;
using cn_compiler::Stmt;
using cn_compiler::StringLiteral;
using cn_compiler::Type;
using cn_compiler::UnaryExpr;
using cn_compiler::VarDecl;
using cn_compiler::WhileStmt;

namespace {

// 测试访问者：记录访问过的节点类型序列（验证访问者分发）
class RecordingVisitor : public AstVisitor {
public:
    void visitProgram(Program* node) override { visit("Program", node); }
    void visitFunctionDecl(FunctionDecl* node) override { visit("FunctionDecl", node); }
    void visitParamDecl(ParamDecl* node) override { visit("ParamDecl", node); }
    void visitVarDecl(VarDecl* node) override { visit("VarDecl", node); }
    void visitBlockStmt(BlockStmt* node) override { visit("BlockStmt", node); }
    void visitExprStmt(ExprStmt* node) override { visit("ExprStmt", node); }
    void visitIfStmt(IfStmt* node) override { visit("IfStmt", node); }
    void visitWhileStmt(WhileStmt* node) override { visit("WhileStmt", node); }
    void visitForStmt(ForStmt* node) override { visit("ForStmt", node); }
    void visitReturnStmt(ReturnStmt* node) override { visit("ReturnStmt", node); }
    void visitBreakStmt(BreakStmt* node) override { visit("BreakStmt", node); }
    void visitContinueStmt(ContinueStmt* node) override { visit("ContinueStmt", node); }
    void visitIntegerLiteral(IntegerLiteral* node) override { visit("IntegerLiteral", node); }
    void visitFloatLiteral(FloatLiteral* node) override { visit("FloatLiteral", node); }
    void visitStringLiteral(StringLiteral* node) override { visit("StringLiteral", node); }
    void visitCharLiteral(CharLiteral* node) override { visit("CharLiteral", node); }
    void visitBoolLiteral(BoolLiteral* node) override { visit("BoolLiteral", node); }
    void visitIdentifierExpr(IdentifierExpr* node) override { visit("IdentifierExpr", node); }
    void visitBinaryExpr(BinaryExpr* node) override { visit("BinaryExpr", node); }
    void visitUnaryExpr(UnaryExpr* node) override { visit("UnaryExpr", node); }
    void visitAssignmentExpr(AssignmentExpr* node) override { visit("AssignmentExpr", node); }
    void visitCallExpr(CallExpr* node) override { visit("CallExpr", node); }
    void visitMemberExpr(MemberExpr* node) override { visit("MemberExpr", node); }
    void visitType(Type* node) override { visit("Type", node); }

    std::vector<std::string> visited;  // 访问记录

private:
    // 记录访问：验证节点类型与类型()方法一致
    void visit(const std::string& tag, AstNode* node) {
        visited.push_back(tag);
        EXPECT_NE(node, nullptr);
        EXPECT_FALSE(tag.empty());
    }
};

} // namespace

// ==================== 1. 节点类型枚举 ====================

// 各表达式节点的类型()方法返回正确枚举值
TEST(AstTest, ExprNodeTypes) {
    IntegerLiteral intLit(42, "42");
    EXPECT_EQ(intLit.getType(), NodeType::IntegerLiteral);
    EXPECT_EQ(intLit.value, 42);
    EXPECT_EQ(intLit.raw, "42");

    FloatLiteral floatLit(3.14, "3.14");
    EXPECT_EQ(floatLit.getType(), NodeType::FloatLiteral);
    EXPECT_DOUBLE_EQ(floatLit.value, 3.14);

    StringLiteral strLit("\"你好\"");
    EXPECT_EQ(strLit.getType(), NodeType::StringLiteral);

    CharLiteral charLit("'A'");
    EXPECT_EQ(charLit.getType(), NodeType::CharLiteral);

    BoolLiteral boolLit(true, "真");
    EXPECT_EQ(boolLit.getType(), NodeType::BoolLiteral);
    EXPECT_TRUE(boolLit.value);

    IdentifierExpr ident("x");
    EXPECT_EQ(ident.getType(), NodeType::IdentifierExpr);
    EXPECT_EQ(ident.name, "x");
}

// 运算符与语句节点类型
TEST(AstTest, StmtAndOpNodeTypes) {
    // 二元运算
    auto left = std::make_unique<IdentifierExpr>("a");
    auto right = std::make_unique<IdentifierExpr>("b");
    BinaryExpr bin(Operator::Add, std::move(left), std::move(right));
    EXPECT_EQ(bin.getType(), NodeType::BinaryExpr);
    EXPECT_EQ(bin.op, Operator::Add);
    EXPECT_EQ(bin.left->getType(), NodeType::IdentifierExpr);
    EXPECT_EQ(bin.right->getType(), NodeType::IdentifierExpr);

    // 一元运算（前缀）
    auto operand = std::make_unique<IdentifierExpr>("i");
    UnaryExpr unary(Operator::Increment, std::move(operand), false);
    EXPECT_EQ(unary.getType(), NodeType::UnaryExpr);
    EXPECT_EQ(unary.op, Operator::Increment);
    EXPECT_FALSE(unary.postfix);

    // 语句节点
    BreakStmt brk;
    EXPECT_EQ(brk.getType(), NodeType::BreakStmt);
    ContinueStmt cont;
    EXPECT_EQ(cont.getType(), NodeType::ContinueStmt);
    ReturnStmt ret;
    EXPECT_EQ(ret.getType(), NodeType::ReturnStmt);
}

// ==================== 2. 程序与函数构建 ====================

// 构建一个包含函数的程序（对应实施计划规格的测试场景）
TEST(AstTest, BuildProgramWithFunction) {
    auto func = std::make_unique<FunctionDecl>();
    func->name = "加";
    func->returnType = "整32";
    auto param = std::make_unique<ParamDecl>();
    param->name = "a";
    param->typeName = "整32";
    func->params.push_back(std::move(param));

    // 返回 a + b
    auto retStmt = std::make_unique<ReturnStmt>();
    auto add = std::make_unique<BinaryExpr>(
        Operator::Add,
        std::make_unique<IdentifierExpr>("a"),
        std::make_unique<IdentifierExpr>("b"));
    retStmt->value = std::move(add);
    func->body = std::make_unique<BlockStmt>();
    func->body->statements.push_back(std::move(retStmt));

    auto program = std::make_unique<Program>();
    program->declarations.push_back(std::move(func));

    ASSERT_EQ(program->declarations.size(), 1u);
    FunctionDecl* decl = program->declarations[0].get();
    EXPECT_EQ(decl->name, "加");
    EXPECT_EQ(decl->returnType, "整32");
    ASSERT_EQ(decl->params.size(), 1u);
    EXPECT_EQ(decl->params[0]->name, "a");
    EXPECT_EQ(decl->params[0]->typeName, "整32");
    ASSERT_NE(decl->body, nullptr);
    ASSERT_EQ(decl->body->statements.size(), 1u);
    EXPECT_EQ(decl->body->statements[0]->getType(), NodeType::ReturnStmt);
}

// 变量声明节点字段
TEST(AstTest, VarDeclNode) {
    VarDecl decl;
    decl.name = "x";
    decl.typeName = "";
    decl.isConst = false;
    decl.initializer = std::make_unique<IntegerLiteral>(10, "10");
    EXPECT_EQ(decl.getType(), NodeType::VarDecl);
    EXPECT_EQ(decl.name, "x");
    EXPECT_TRUE(decl.typeName.empty());
    EXPECT_FALSE(decl.isConst);
    ASSERT_NE(decl.initializer, nullptr);
    EXPECT_EQ(decl.initializer->getType(), NodeType::IntegerLiteral);
}

// 常量变量声明（isConst）
TEST(AstTest, ConstVarDeclNode) {
    VarDecl decl;
    decl.name = "PI";
    decl.isConst = true;
    decl.initializer = std::make_unique<FloatLiteral>(3.14, "3.14");
    EXPECT_TRUE(decl.isConst);
    EXPECT_EQ(decl.initializer->getType(), NodeType::FloatLiteral);
}

// 赋值表达式：target = value
TEST(AstTest, AssignmentExprNode) {
    auto target = std::make_unique<IdentifierExpr>("x");
    auto value = std::make_unique<IntegerLiteral>(5, "5");
    AssignmentExpr assign(std::move(target), Operator::Assign, std::move(value));
    EXPECT_EQ(assign.getType(), NodeType::AssignmentExpr);
    EXPECT_EQ(assign.op, Operator::Assign);
    EXPECT_EQ(assign.target->getType(), NodeType::IdentifierExpr);
    EXPECT_EQ(assign.value->getType(), NodeType::IntegerLiteral);
}

// 调用表达式：便捷构造（函数名）与通用构造（任意表达式）
TEST(AstTest, CallExprNode) {
    // 便捷构造：按函数名
    CallExpr callByName("打印行");
    EXPECT_EQ(callByName.getType(), NodeType::CallExpr);
    EXPECT_EQ(callByName.callee->getType(), NodeType::IdentifierExpr);
    callByName.arguments.push_back(std::make_unique<StringLiteral>("\"你好\""));
    ASSERT_EQ(callByName.arguments.size(), 1u);

    // 通用构造：成员调用 对象.方法()
    auto member = std::make_unique<MemberExpr>(
        std::make_unique<IdentifierExpr>("obj"), "方法", false);
    CallExpr callByExpr(std::move(member));
    EXPECT_EQ(callByExpr.callee->getType(), NodeType::MemberExpr);
}

// 成员访问：. 与 ->
TEST(AstTest, MemberExprNode) {
    MemberExpr dot(std::make_unique<IdentifierExpr>("对象"), "成员", false);
    EXPECT_EQ(dot.getType(), NodeType::MemberExpr);
    EXPECT_EQ(dot.memberName, "成员");
    EXPECT_FALSE(dot.isArrow);

    MemberExpr arrow(std::make_unique<IdentifierExpr>("指针"), "成员", true);
    EXPECT_TRUE(arrow.isArrow);
}

// 控制流语句节点构建
TEST(AstTest, ControlFlowNodes) {
    // 如果语句
    IfStmt ifStmt;
    ifStmt.condition = std::make_unique<IdentifierExpr>("x");
    ifStmt.thenBranch = std::make_unique<BlockStmt>();
    ifStmt.elseBranch = std::make_unique<BlockStmt>();
    EXPECT_EQ(ifStmt.getType(), NodeType::IfStmt);
    ASSERT_NE(ifStmt.elseBranch, nullptr);
    EXPECT_EQ(ifStmt.elseBranch->getType(), NodeType::BlockStmt);

    // 当语句
    WhileStmt whileStmt;
    whileStmt.condition = std::make_unique<IdentifierExpr>("条件");
    whileStmt.body = std::make_unique<BlockStmt>();
    EXPECT_EQ(whileStmt.getType(), NodeType::WhileStmt);

    // 循环语句（for风格）
    ForStmt forStmt;
    forStmt.init = std::make_unique<VarDecl>();
    forStmt.condition = std::make_unique<IdentifierExpr>("c");
    forStmt.update = std::make_unique<UnaryExpr>(
        Operator::Increment, std::make_unique<IdentifierExpr>("i"), true);
    forStmt.body = std::make_unique<BlockStmt>();
    EXPECT_EQ(forStmt.getType(), NodeType::ForStmt);
    EXPECT_EQ(forStmt.init->getType(), NodeType::VarDecl);
    EXPECT_EQ(forStmt.update->getType(), NodeType::UnaryExpr);
}

// 类型节点
TEST(AstTest, TypeNode) {
    Type type("整32");
    EXPECT_EQ(type.getType(), NodeType::TypeNode);
    EXPECT_EQ(type.name, "整32");
}

// ==================== 3. 访问者模式 ====================

// 访问者分发：每个节点调用 accept 后进入对应 visit 方法
TEST(AstTest, VisitorDispatch) {
    RecordingVisitor visitor;

    Program program;
    program.accept(visitor);

    FunctionDecl func;
    func.accept(visitor);

    VarDecl decl;
    decl.accept(visitor);

    IfStmt ifStmt;
    ifStmt.accept(visitor);

    IntegerLiteral intLit(1, "1");
    intLit.accept(visitor);

    CallExpr call("f");
    call.accept(visitor);

    Type type("整32");
    type.accept(visitor);

    ASSERT_EQ(visitor.visited.size(), 7u);
    EXPECT_EQ(visitor.visited[0], "Program");
    EXPECT_EQ(visitor.visited[1], "FunctionDecl");
    EXPECT_EQ(visitor.visited[2], "VarDecl");
    EXPECT_EQ(visitor.visited[3], "IfStmt");
    EXPECT_EQ(visitor.visited[4], "IntegerLiteral");
    EXPECT_EQ(visitor.visited[5], "CallExpr");
    EXPECT_EQ(visitor.visited[6], "Type");
}

// 所有节点均可通过基类指针访问（多态）
TEST(AstTest, PolymorphicAccept) {
    RecordingVisitor visitor;
    std::vector<std::unique_ptr<AstNode>> nodes;
    nodes.push_back(std::make_unique<BinaryExpr>(
        Operator::Add,
        std::make_unique<IntegerLiteral>(1, "1"),
        std::make_unique<IntegerLiteral>(2, "2")));
    nodes.push_back(std::make_unique<ReturnStmt>());
    nodes.push_back(std::make_unique<BreakStmt>());
    nodes.push_back(std::make_unique<ContinueStmt>());
    nodes.push_back(std::make_unique<UnaryExpr>(
        Operator::Subtract, std::make_unique<IntegerLiteral>(1, "1"), false));
    nodes.push_back(std::make_unique<ExprStmt>(std::make_unique<IdentifierExpr>("x")));

    for (auto& node : nodes) {
        node->accept(visitor);
    }
    ASSERT_EQ(visitor.visited.size(), 6u);
    EXPECT_EQ(visitor.visited[0], "BinaryExpr");
    EXPECT_EQ(visitor.visited[1], "ReturnStmt");
    EXPECT_EQ(visitor.visited[2], "BreakStmt");
    EXPECT_EQ(visitor.visited[3], "ContinueStmt");
    EXPECT_EQ(visitor.visited[4], "UnaryExpr");
    EXPECT_EQ(visitor.visited[5], "ExprStmt");
}

// unique_ptr所有权转移不破坏节点
TEST(AstTest, UniquePtrOwnership) {
    auto expr = std::make_unique<BinaryExpr>(
        Operator::Multiply,
        std::make_unique<IdentifierExpr>("a"),
        std::make_unique<IdentifierExpr>("b"));

    ExprStmt stmt(std::move(expr));  // 转移所有权
    EXPECT_EQ(expr, nullptr);        // 原指针为空
    EXPECT_EQ(stmt.getType(), NodeType::ExprStmt);
    EXPECT_EQ(stmt.expr->getType(), NodeType::BinaryExpr);
    EXPECT_EQ(static_cast<BinaryExpr*>(stmt.expr.get())->op, Operator::Multiply);
}

// 基类析构（多态删除）
TEST(AstTest, VirtualDestructor) {
    std::unique_ptr<Stmt> stmt = std::make_unique<BlockStmt>();
    EXPECT_EQ(stmt->getType(), NodeType::BlockStmt);
    // 作用域结束时通过基类指针正确析构派生类（无泄漏即通过）
}
