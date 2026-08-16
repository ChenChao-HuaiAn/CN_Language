// AST打印器：访问者模式递归打印AST树（Task 1.10 ast 命令调试输出）
// 设计要点：
//   1. 继承 AstVisitor，按节点类型输出节点名+源码位置+关键字段（字面量值/变量名/类型名等）
//   2. 缩进表示树层级，递归访问子节点（表达式/语句/声明）
//   3. 独立实现于本文件，避免 driver.cpp 超长，也便于单元测试单独引用
//   4. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释与输出文本
#pragma once
#include <iostream>

#include "cn_compiler/parser/ast.hpp"

namespace cn_compiler {

// AST打印器：将AST树打印到指定输出流
class AstPrinter : public AstVisitor {
public:
    explicit AstPrinter(std::ostream& out) : out_(out) {}

    // 主入口：打印完整程序AST
    void print(Program* program);

    // ==================== AstVisitor 接口实现 ====================
    void visitProgram(Program* node) override;
    void visitFunctionDecl(FunctionDecl* node) override;
    void visitParamDecl(ParamDecl* node) override;
    void visitVarDecl(VarDecl* node) override;
    void visitStructDecl(StructDecl* node) override;
    void visitEnumDecl(EnumDecl* node) override;
    // 阶段3 OOP/错误处理/模块/泛型 声明节点（Task 3.1/3.3/3.6/3.8）
    void visitClassDecl(ClassDecl* node) override;
    void visitClassMember(ClassMember* node) override;
    void visitInterfaceDecl(InterfaceDecl* node) override;
    void visitImportDecl(ImportDecl* node) override;
    void visitGenericDecl(GenericDecl* node) override;
    // 语句节点
    void visitBlockStmt(BlockStmt* node) override;
    void visitExprStmt(ExprStmt* node) override;
    void visitIfStmt(IfStmt* node) override;
    void visitWhileStmt(WhileStmt* node) override;
    void visitForStmt(ForStmt* node) override;
    void visitRangeForStmt(RangeForStmt* node) override;  // C-2：遍历...中每个
    void visitReturnStmt(ReturnStmt* node) override;
    void visitBreakStmt(BreakStmt* node) override;
    void visitContinueStmt(ContinueStmt* node) override;
    void visitSwitchStmt(SwitchStmt* node) override;
    void visitCaseLabel(CaseLabel* node) override;
    void visitDefaultLabel(DefaultLabel* node) override;
    // 表达式节点
    void visitIntegerLiteral(IntegerLiteral* node) override;
    void visitFloatLiteral(FloatLiteral* node) override;
    void visitStringLiteral(StringLiteral* node) override;
    void visitCharLiteral(CharLiteral* node) override;
    void visitBoolLiteral(BoolLiteral* node) override;
    void visitNullLiteral(NullLiteral* node) override;
    void visitIdentifierExpr(IdentifierExpr* node) override;
    void visitSelfExpr(SelfExpr* node) override;
    void visitSuperExpr(SuperExpr* node) override;
    void visitBinaryExpr(BinaryExpr* node) override;
    void visitUnaryExpr(UnaryExpr* node) override;
    void visitAssignmentExpr(AssignmentExpr* node) override;
    void visitCallExpr(CallExpr* node) override;
    void visitMemberExpr(MemberExpr* node) override;
    void visitIndexExpr(IndexExpr* node) override;
    void visitInitListExpr(InitListExpr* node) override;
    void visitStructInitExpr(StructInitExpr* node) override;
    void visitTernaryExpr(TernaryExpr* node) override;
    void visitCastExpr(CastExpr* node) override;
    void visitLambdaExpr(LambdaExpr* node) override;
    void visitSizeofExpr(SizeofExpr* node) override;  // 类型大小（A-3 2026-08）
    // 类型节点
    void visitType(Type* node) override;

private:
    // 运算符 -> 中文符号文本（调试输出）
    static const char* operatorText(Operator op);
    // 打印带缩进与位置的节点头：缩进 + 类型名 + 位置 + 附加信息
    void printHeader(const std::string& typeName, const SourceLocation& loc,
                     const std::string& detail = "");

    std::ostream& out_;  // 输出流
    int depth_ = 0;      // 当前缩进层级
};

} // namespace cn_compiler
