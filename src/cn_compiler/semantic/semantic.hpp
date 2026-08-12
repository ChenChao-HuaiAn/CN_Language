// 语义分析器：遍历AST进行符号收集与类型检查（Task 1.5）
// 设计要点：
//   1. 继承 AstVisitor 访问者模式遍历AST（与语法分析器解耦）
//   2. 两趟处理：先注册全部函数符号（支持前向调用），再逐个检查函数体
//   3. 变量作用域栈：支持嵌套作用域（代码块/循环体）与同名遮蔽
//   4. 类型系统：CN语言基本类型（整8~整128/正8~正128/浮32/浮64/字符/布尔/字符串/空类型）
//   5. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释/字符串/输出
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/parser/ast.hpp"

namespace cn_compiler {

// 函数符号信息：返回类型 + 参数类型列表 + 是否有函数体
struct FunctionInfo {
    std::string returnType;                // 返回类型（"空类型"表示无返回值）
    std::vector<std::string> paramTypes;   // 参数类型列表
    bool hasBody = false;                  // 是否有函数体（函数原型声明无体）
};

// 语义分析器：构建符号表并做类型检查，产出诊断
class SemanticAnalyzer : public AstVisitor {
public:
    // 构造函数：绑定诊断引擎引用
    explicit SemanticAnalyzer(Diagnostics& diagnostics) : diagnostics_(diagnostics) {}

    // 主入口：分析程序AST，返回是否成功（无错误）
    bool analyze(Program* program);

    // ==================== AstVisitor 接口实现 ====================
    // 声明节点
    void visitProgram(Program* node) override;
    void visitFunctionDecl(FunctionDecl* node) override;
    void visitParamDecl(ParamDecl* node) override;
    void visitVarDecl(VarDecl* node) override;
    // 语句节点
    void visitBlockStmt(BlockStmt* node) override;
    void visitExprStmt(ExprStmt* node) override;
    void visitIfStmt(IfStmt* node) override;
    void visitWhileStmt(WhileStmt* node) override;
    void visitForStmt(ForStmt* node) override;
    void visitReturnStmt(ReturnStmt* node) override;
    void visitBreakStmt(BreakStmt* node) override;
    void visitContinueStmt(ContinueStmt* node) override;
    // 表达式节点
    void visitIntegerLiteral(IntegerLiteral* node) override;
    void visitFloatLiteral(FloatLiteral* node) override;
    void visitStringLiteral(StringLiteral* node) override;
    void visitCharLiteral(CharLiteral* node) override;
    void visitBoolLiteral(BoolLiteral* node) override;
    void visitIdentifierExpr(IdentifierExpr* node) override;
    void visitBinaryExpr(BinaryExpr* node) override;
    void visitUnaryExpr(UnaryExpr* node) override;
    void visitAssignmentExpr(AssignmentExpr* node) override;
    void visitCallExpr(CallExpr* node) override;
    void visitMemberExpr(MemberExpr* node) override;
    // 类型节点
    void visitType(Type* node) override;

private:
    // ==================== 符号表管理 ====================
    void pushScope();                              // 进入新作用域
    void popScope();                               // 退出当前作用域
    // 声明变量（同作用域重复声明返回false并报告错误）
    bool declareVar(const std::string& name, const std::string& type, const SourceLocation& loc);
    // 从内到外查找变量类型（未找到返回false）
    bool lookupVar(const std::string& name, std::string& type) const;

    // ==================== 类型工具（静态） ====================
    static bool isNumeric(const std::string& type);  // 是否数值类型（整/正/浮）
    static bool isInteger(const std::string& type);  // 是否整数类型（整N/正N/整数）
    static bool isFloat(const std::string& type);    // 是否浮点类型（浮32/浮64/小数）
    static bool canConvert(const std::string& from, const std::string& to); // 能否隐式转换
    // 数值运算的结果类型（整型取宽、整浮混合取浮）
    static std::string commonNumericType(const std::string& a, const std::string& b);
    // 是否复合赋值运算符（+= -= *= /= %= 等）
    static bool isCompoundAssign(Operator op);

    // ==================== 检查辅助 ====================
    std::string checkExpr(Expr* node);             // 检查表达式，返回推断类型
    void checkStmt(Stmt* node);                    // 检查语句（分发到visit）
    void checkBlock(BlockStmt* node);              // 检查代码块（含作用域进出）
    // 检查条件表达式是否为布尔类型
    void checkCondition(const std::string& type, const SourceLocation& loc, const std::string& ctx);
    // 注册CN语言内置函数符号（打印行/打印行整数/打印行浮点，供函数调用检查）
    void registerBuiltins();
    void registerFunction(FunctionDecl* node);     // 第一趟：注册函数符号
    void checkFunctionBody(FunctionDecl* node);    // 第二趟：检查函数体
    // 函数体是否保证有返回（最后一条为返回语句或无限循环）
    bool bodyGuaranteesReturn(BlockStmt* body) const;

    // ==================== 成员状态 ====================
    Diagnostics& diagnostics_;                     // 诊断引擎引用
    std::unordered_map<std::string, FunctionInfo> functions_;   // 函数符号表
    std::vector<std::unordered_map<std::string, std::string>> scopes_; // 变量作用域栈
    std::string lastType_;                         // 最近一次表达式推断的类型
    std::string currentReturnType_;                // 当前函数返回类型（空表示顶层）
    int loopDepth_ = 0;                            // 循环嵌套深度（中断/继续合法性）
};

} // namespace cn_compiler
